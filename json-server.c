#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>
#include <netdb.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "smartalloc.h"
#include "json-server.h"


struct server_stats server_info;

struct addrinfo hints, *matches, *server;
int get_info_ret;

int listening_socket_fd = 0;

int interrupt = 0;

fd_set read_sockets;
fd_set write_sockets;
fd_set error_sockets;

int largest_fd;
struct client *all_clients;
int max_client_size = 100;



void server_exit(){
	interrupt = 1; 
}

//Returns the total size of the virtual address space for the running process
void make_non_blocking(int fd){
	int current_options = 0;

	current_options = fcntl(fd, F_GETFL); //get fd flags
	if(current_options == -1){
		fprintf(stderr, "Error getting options for fd: %d\n", fd);
	}
	current_options |= O_NONBLOCK; //add nonblocking
	if(fcntl(fd, F_SETFL, current_options) == -1){
		fprintf(stderr, "Error setting options for fd: %d\n", fd);
	}
}

static long get_memory_usage_linux()
{
	//Variables to store all the contents of the stat file
	int pid, ppid, pgrp, session, tty_nr, tpgid;
	char comm[2048], state;
	unsigned int flags;
	unsigned long minflt, cminflt, majflt, cmajflt, vsize;
	unsigned long utime, stime;
	long cutime, cstime, priority, nice, num_threads, itrealvalue, rss;
	unsigned long long starttime;
	// Open the file
	FILE *stat = fopen("/proc/self/stat", "r");
	if (!stat) {
		perror("Failed to open /proc/self/stat");
		return 0;
	}
	// Read the statistics out of the file
	fscanf(stat, 	"%d%s%c%d%d%d%d%d%u%lu%lu%lu%lu"
			"%ld%ld%ld%ld%ld%ld%ld%ld%llu%lu%ld",
			&pid, comm, &state, &ppid, &pgrp, &session, &tty_nr,
			&tpgid, &flags, &minflt, &cminflt, &majflt, &cmajflt,
			&utime, &stime, &cutime, &cstime, &priority, &nice,
			&num_threads, &itrealvalue, &starttime, &vsize, &rss);
	fclose(stat);
	return vsize;
}

void construct_header_response(struct client *needy_client){
	//take in a client and see what they need: 1.1 or 1.0 response
	//leave the content-length empty
	sprintf(needy_client->write_buffer, 
			"HTTP/%.1f 200 OK\n"
			"Content-Type: application/json\n"
			"Content-Length: ", needy_client->response_http_version);

}

void create_server_info_response(struct client *curious_client){
	char temp_info[600];
	int info_length = 0;
	struct rusage times; 
	if(getrusage(RUSAGE_SELF, &times) == -1){
		fprintf(stderr, "Unable to update the user and cpu times!\n");
		fprintf(stdout, "Server exiting cleanly.\n");
		exit(-11);
	}
	
	server_info.memory_used = get_memory_usage_linux();
	fprintf(stderr, "Alright, Let's see if we can do that status thing!\n");	
	
	//can't write into the buffer until we know how long this is!
	sprintf(temp_info, "{\n"
			" \"num_clients\": %d,\n"
			" \"num_requests\": %d,\n"
			" \"errors\": %d,\n"
			" \"uptime\": %ld.%ld,\n"
			" \"cpu_time\": %ld.%ld,\n"
			" \"memory_used\": %ld,\n"
			"}\n",
			server_info.num_clients, 
			server_info.num_requests,
			server_info.errors, 
			times.ru_stime.tv_sec, times.ru_stime.tv_usec, 
			times.ru_utime.tv_sec, times.ru_utime.tv_usec,
			server_info.memory_used);
	
	fprintf(stderr, "Here is the content of temp_info:\n%s\n", temp_info);
	info_length = strlen(temp_info);
	curious_client->response_size += sprintf(curious_client->write_buffer, GENERIC_HEADER, curious_client->response_http_version, info_length); //copy in header	
	curious_client->response_size += sprintf(curious_client->write_buffer + curious_client->response_size, "%s", temp_info); //copy in the status
}

void create_listening_socket(char *char_address){
	int bind_ret = 0;
	//loop through returned link list
	for(server = matches; server != NULL; server = server->ai_next){
		//try and create a socket
		listening_socket_fd = socket(server->ai_family, server->ai_socktype, 0);	
		if(listening_socket_fd == -1){
			//error creating the socket
			//fprintf(stderr, "Create socket error...\n");
			continue;
		}
		//fprintf(stderr, "Designated FD for listening socket: %d\n", listening_socket_fd);
		bind_ret = bind(listening_socket_fd, server->ai_addr, server->ai_addrlen);
		if(bind_ret == -1){
			close(listening_socket_fd);
			//fprintf(stderr, "Bind error... \n");
			continue;
		}
		//don't create and bind to more than one socket
		
		return;

	}
	
	fprintf(stderr, "address '%s' didn't parse (v4 or v6)\n", char_address);
	fprintf(stdout, "Server exiting cleanly.\n");
	exit(-2);
}


void free_client_buffers(struct client *finished){
	free(finished->read_buffer);
	free(finished->write_buffer);
}

void clear_all_clients(){
	int i = 0;
	for(; i < max_client_size; i++){
		if(all_clients[i].is_alive){
			free_client_buffers(&all_clients[i]);
		}
	}
	//finally, free the struct
}

void resize_clients(){
	max_client_size *= 2;
	all_clients = realloc(all_clients, sizeof(struct client) * max_client_size);
	if(all_clients == NULL){
		fprintf(stderr, "Unable to hold %d clients! Exiting...\n", max_client_size);
		server_exit();
	}
}

void setup_new_client(int fd){
	if(server_info.num_clients >= max_client_size){
		resize_clients();
	}
	
	all_clients[server_info.num_clients].is_alive      	= 1;
	all_clients[server_info.num_clients].current_step  	= READ_STATE;
	all_clients[server_info.num_clients].socket_fd     	= fd;
	all_clients[server_info.num_clients].bytes_read    	= 0;
	all_clients[server_info.num_clients].bytes_written 	= 0;
	all_clients[server_info.num_clients].read_buffer_size   = 400;
	all_clients[server_info.num_clients].write_buffer_size  = 400;
	//make sure to change these if initial buffer size changes!
	all_clients[server_info.num_clients].read_buffer   	= calloc(sizeof(char),  400);
	all_clients[server_info.num_clients].write_buffer  	= calloc(sizeof(char),  400);
	if(all_clients[server_info.num_clients].read_buffer == NULL ||
	   all_clients[server_info.num_clients].write_buffer == NULL){
		fprintf(stderr, "Unable to initialize buffers for client %d. Exiting...\n", server_info.num_clients);
		server_exit();
	}
	
	server_info.num_clients++;

	//check to see if we need to change largest fd
	if(fd > largest_fd){
		largest_fd = fd;
	}
}

void create_new_connection(){
	int new_client_fd;

	new_client_fd = accept(listening_socket_fd, NULL, NULL);
	//fprintf(stderr, "FD of client %d: %d\n", server_info.num_clients, new_client_fd);
	setup_new_client(new_client_fd);
}

//returns the position of the character BEFORE the newline character
int is_full_response(struct client *process_client){
	int ret = 0;
	process_client->read_buffer -= process_client->bytes_read; //move pointer to beginning of buffer
	fprintf(stderr, "\nWE HAVE RECEIVED THIS SO FAR:\n%s\n", process_client->read_buffer);
	//if there is a specification of HTTP/1.1 we need to look for two newlines
	if(strstr(process_client->read_buffer, "HTTP/1.1\r\n") != NULL){
		if(strstr(process_client->read_buffer, "\r\n\r\n") != NULL){
			ret = 1;
			process_client->response_http_version = 1.1;
		}
	}
	else{
		if(strstr(process_client->read_buffer, "\r\n") != NULL){
			ret = 1;
			process_client->response_http_version = 1.0;
		}
		
	}
	//else we need to look for just a single newline
	
	//basically loop through until we find a newline
	
	process_client->read_buffer += process_client->bytes_read; //if not a full response, return pointer
	return ret;
}

/* resizes the buffer of a client whose buffer has filled
 * buffer will be one of two values:
 * 	RESIZE_READ_BUFFER
 * 	RESIZE_WRITE_BUFFER
 */
void resize_buffer(struct client *full_client, int buffer){
	fprintf(stderr, "\n"
			"========================================\n"
			"             RESIZING BUFFER!!!         \n"
			"========================================\n"
			"\n");
	
	char *rw_buffer = NULL;
	if(buffer == RESIZE_READ_BUFFER){
		full_client->read_buffer_size *= 2;
		full_client->read_buffer = realloc(full_client->read_buffer, sizeof(char) * full_client->read_buffer_size);
		rw_buffer = full_client->read_buffer;
	}
	else if(buffer == RESIZE_WRITE_BUFFER){
		full_client->write_buffer_size *= 2;
		full_client->write_buffer = realloc(full_client->write_buffer, sizeof(char) * full_client->write_buffer_size);
		rw_buffer = full_client->write_buffer;
	}
	else{
		fprintf(stderr, "What else could we possibly resize?\n");
	}

	//check to see that we succeeded
	if(rw_buffer == NULL){
		fprintf(stderr, "Failed at resizing a buffer. Exiting!\n");
		server_exit();
	}	
}

void fulfill_fortune(struct client *superstitious_client){
	//lol you thought I would implement this so soon???
	
}

/* type indicates what to fill the buffer with:
 * IMPLEMENTED_TYPE
 * ABOUT_TYPE
 * STATUS_TYPE
 * FORTUNE_TYPE
 * 404_TYPE	
 */
void fill_write_buffer(struct client *processed_client, int type){
	if(type == TYPE_IMPLEMENTED){
		sprintf(processed_client->write_buffer, IMPLEMENTED_RESPONSE, processed_client->response_http_version);
		processed_client->current_step = WRITE_STATE;
	}
	else if(type == TYPE_ABOUT){
		sprintf(processed_client->write_buffer, ABOUT_RESPONSE, processed_client->response_http_version);
		processed_client->current_step = WRITE_STATE;
	}
	else if(type == TYPE_STATUS){
		//get all the system info, fill the buffer then return
		create_server_info_response(processed_client);
		processed_client->current_step = WRITE_STATE;
	}
	else if(type == TYPE_FORTUNE){
		//fork and exec here and then return to main loop to read from new fd
		processed_client->current_step = FORTUNE_STATE;
	}
	else if(type == TYPE_QUIT){
		//send the response and then close down the system
		sprintf(processed_client->write_buffer, QUIT_RESPONSE, processed_client->response_http_version);
		processed_client->current_step = QUIT_STATE;
	}
	else if(type == TYPE_404){
		sprintf(processed_client->write_buffer, ERROR_404, processed_client->response_http_version);
		processed_client->current_step = WRITE_STATE;
	}
	else{
		fprintf(stderr, "I don't know how to fill this write buffer yet!\n");
	}
	//fprintf(stderr, "In client write buffer:\n%s\n", processed_client->write_buffer);
	
	processed_client->response_size = strlen(processed_client->write_buffer);
	//fprintf(stderr, "Size of response is %d\n", processed_client->response_size);
	//we have to return to the main select loop	
}

void process_request(struct client *waiting_client){
	int fill_write_buffer_type = 0;
	
	//implemented request
	if(		strstr(waiting_client->read_buffer, IMPLEMENTED_REQUEST_SPACE)   != NULL ||
			strstr(waiting_client->read_buffer, IMPLEMENTED_REQUEST_NEWLINE) != NULL){
		fprintf(stderr, "We have a /json/implemented.json request!\n");
		fill_write_buffer_type = TYPE_IMPLEMENTED;
	}
	//about request
	else if(	strstr(waiting_client->read_buffer, ABOUT_REQUEST_SPACE)   != NULL ||
			strstr(waiting_client->read_buffer, ABOUT_REQUEST_NEWLINE) != NULL){
		fprintf(stderr, "We have a /json/about.json request!\n");
		fill_write_buffer_type = TYPE_ABOUT;
	}
	//status request
	else if(	strstr(waiting_client->read_buffer, STATUS_REQUEST_SPACE)   != NULL ||
			strstr(waiting_client->read_buffer, STATUS_REQUEST_NEWLINE) != NULL){
		fprintf(stderr, "We have a /json/status.json request\n");
		fill_write_buffer_type = TYPE_STATUS;
	}
	//fortune request
	else if(	strstr(waiting_client->read_buffer, FORTUNE_REQUEST_SPACE)   != NULL ||
			strstr(waiting_client->read_buffer, FORTUNE_REQUEST_NEWLINE) != NULL){
		fprintf(stderr, "We have a /json/fortune request!\n");
		fill_write_buffer_type = TYPE_FORTUNE;
	}
	//quit request
	else if(	strstr(waiting_client->read_buffer, QUIT_REQUEST_SPACE)   != NULL || 
			strstr(waiting_client->read_buffer, QUIT_REQUEST_NEWLINE) != NULL){
		fprintf(stderr, "We have a /json/quit request!\n");
		fill_write_buffer_type = TYPE_QUIT;
	}
	else{
		fprintf(stderr, "Send a 404 request because this is WHACKY!\n");
		fill_write_buffer_type = TYPE_404;
      server_info.errors++;
	}
	//add that we got a request
	server_info.num_requests++;	

	//close the socket after receiving a response
	fill_write_buffer(waiting_client, fill_write_buffer_type);
}


void close_client_connection(struct client *finished_client){
	free_client_buffers(finished_client); //free all memory associated with the client's buffers
	finished_client->is_alive = 0; //mark as dead
	close(finished_client->socket_fd);	
}

void write_to_client(struct client *client_response){
	int bytes_sent = 0;
	int bytes_available = client_response->response_size - client_response->bytes_written;
	
	//fprintf(stderr, "We need to write %d more bytes to this client\n", bytes_available);
	bytes_sent = send(client_response->socket_fd, client_response->write_buffer, bytes_available, 0); 
	client_response->bytes_written += bytes_sent;
	client_response->write_buffer  += bytes_sent; //move buffer pointer along!
	//fprintf(stderr, "Bytes received so far: %d\n", client_request->bytes_read);
	
	//check to see if we have finished sending the response
	if(client_response->bytes_written == client_response->response_size){
		client_response->write_buffer -= client_response->bytes_written;//move the buffer back for free
		close_client_connection(client_response);
		if(client_response->current_step == QUIT_STATE){
			//close all other clients and definitely just shut everything down
			clear_all_clients();
			server_exit();
		}
	}
}

void read_from_client(struct client *client_request){
	int bytes_received = 0;
	int bytes_available = client_request->read_buffer_size - client_request->bytes_read;
	//check to see if we need to make our buffer bigger!
	if(bytes_available == 0){
		client_request->read_buffer -= client_request->read_buffer_size;//move the buffer back for realloc
		resize_buffer(client_request, RESIZE_READ_BUFFER);
		client_request->read_buffer += client_request->bytes_read;//and replace it to how much we read
		//also update how much room we have to read since we just resized
		bytes_available = client_request->read_buffer_size - client_request->bytes_read;
	}
	
	//fprintf(stderr, "We have room for %d more bytes in read buffer for this client\n", bytes_available);
	bytes_received = recv(client_request->socket_fd, client_request->read_buffer, bytes_available, 0); 
	client_request->bytes_read  += bytes_received;
	client_request->read_buffer += bytes_received; //move buffer pointer along!
	//fprintf(stderr, "Bytes received so far: %d\n", client_request->bytes_read);
	
	//check if full response
	if(is_full_response(client_request)){
		//make sure to change the status of this fd in process_request
		//fprintf(stderr, "We can move on to process request now!\n");
		//move read buffer back to the beginning for parsing!
		client_request->read_buffer -= client_request->bytes_read; //move buffer back to beginning
		process_request(client_request);
	}
}

//after a return from select, we must handle all the different fds
void handle_all_sockets(){
	if(FD_ISSET(listening_socket_fd, &read_sockets)){
		//fprintf(stderr, "New Connection!\n");
		create_new_connection();
	}
	
	int i = 0;
	for(; i < server_info.num_clients; i++){
		if(all_clients[i].is_alive){
			if(FD_ISSET(all_clients[i].socket_fd, &read_sockets)){
				//can read from this client
				read_from_client(&all_clients[i]);
			}
			else if(FD_ISSET(all_clients[i].socket_fd, &write_sockets)){
				//can write to this client	
				write_to_client(&all_clients[i]);
			}
			//check to see if we need to read from a fortune
			else if(all_clients[i].fortune_fd != 0 && FD_ISSET(all_clients[i].fortune_fd, &read_sockets)){
				//we need to fulfill a fortune!
				fulfill_fortune(&all_clients[i]);
			}	
			else{
				fprintf(stderr, "client is alive but not set for read or write\n");
			}
		}	
	}
}

void remake_select_sets(){
	//zero all_sockets
	FD_ZERO(&read_sockets);
	FD_ZERO(&write_sockets);
	FD_ZERO(&error_sockets);

	//initialize listening socket every time	
	FD_SET(listening_socket_fd, &read_sockets);
	int i = 0; 
	for(; i < max_client_size; i++){
		if(all_clients[i].is_alive){
			//check what state and add to correct socket group
			if(all_clients[i].current_step == READ_STATE){
				FD_SET(all_clients[i].socket_fd, &read_sockets);
			}
			else if(all_clients[i].current_step == WRITE_STATE){
				FD_SET(all_clients[i].socket_fd, &write_sockets);
			}
			else if(all_clients[i].current_step == FORTUNE_STATE){
				//TO DO
				//mark the client's connect fd to the forked fortune as read
			}
			else if(all_clients[i].current_step == QUIT_STATE){
				//still set to write.. not sure if we need to do anything else
				FD_SET(all_clients[i].socket_fd, &write_sockets);
			}
			else{
				fprintf(stderr, "We have a client in an undefined state!\n");
			}
		}
	}
}

void select_loop(){
	int sel_ret = 0;

	while(!interrupt){
		//reset all of our select fd_sets
		remake_select_sets();
		
		
		//all select sets are modified in select
		sel_ret = select(largest_fd + 1, &read_sockets, &write_sockets, &error_sockets, 0);
		//fprintf(stderr, "SOMETHING HAS CHANGED!\n");
		if(sel_ret == -1){
			server_exit();
		}
		else if(sel_ret == 0){
			//nothing has changed
			continue;
		}
		else{
			//we can do things!
			//fprintf(stderr, "HANDLING ALL SOCKETS!\n");
			handle_all_sockets();	
		}
	}
}

/* returns port in host order */
int get_port(struct sockaddr *server){
	uint16_t port = 0;
	if(server->sa_family == AF_INET){
		port = ((struct sockaddr_in *)server)->sin_port;
	}
	else{
		port = ((struct sockaddr_in6 *)server)->sin6_port;
	}
	return ntohs(port);
}


void sigint_handler(int sig){
	if (SIGINT == sig){
		server_exit();
	}
}

void initialize_server_stats(){
	server_info.num_clients = 0;
	server_info.uptime = 0;
	server_info.cpu_time = 0;
	server_info.memory_used = 0;
}

int main(int argc, char **argv){
	struct sigaction sa;
	
	initialize_server_stats();
	
	/* Install the signal handler */
	sa.sa_handler = sigint_handler;
	sigfillset(&sa.sa_mask);
	sa.sa_flags = 0;
	if (-1 == sigaction(SIGINT, &sa, NULL))
	{
		perror("Couldn't set signal handler for SIGINT");
		return 2;
	}

	
	
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family   = AF_UNSPEC; //either v4 or v6
	hints.ai_socktype = SOCK_STREAM; //tcp 

	if(argc >= 2){
		//fprintf(stderr, "Setting TCP port to bind to this address: %s\n", argv[1]);
		if((get_info_ret = getaddrinfo(argv[1], "0", &hints, &matches)) != 0){
			fprintf(stderr, "Error with addresses: %s\n", gai_strerror(get_info_ret));
		}	
	}
	else{
		//we were not provided with a binding address, bind to all
		//fprintf(stderr, "Setting TCP port to bind any address!\n");
		hints.ai_flags    = AI_PASSIVE; //bind to all addresses
		if((get_info_ret = getaddrinfo(NULL, "0", &hints, &matches)) != 0){
			fprintf(stderr, "Error with addresses: %s\n", gai_strerror(get_info_ret));
		}	
	}

	
	create_listening_socket(argv[1]);
	//done binding address, so we need to free address info
	freeaddrinfo(matches);

	make_non_blocking(listening_socket_fd);
	largest_fd = listening_socket_fd;	
	
	
	//listen on server socket
	//change this 34 to a better number later!
	if(listen(listening_socket_fd, 34) != 0){
		fprintf(stderr, "Unable to listen on server socket\n");
		fprintf(stdout, "Server exiting cleanly.\n");
		exit(-3);
	}

	if(getsockname(listening_socket_fd, (struct sockaddr *)server->ai_addr, &server->ai_addrlen) == -1){
		fprintf(stderr, "Unable to get socket name!\n");
		fprintf(stdout, "Server exiting cleanly.\n");
		exit(-4);
	}

	fprintf(stdout, "HTTP server is using TCP port %d\n"
			"HTTPS server is using TCP port -1\n", get_port((struct sockaddr *)server->ai_addr));

	fflush(stdout);

	/* setup all clients struct */
	all_clients = calloc(sizeof(struct client),  max_client_size);
	if(all_clients == NULL){
		fprintf(stderr, "Unable to hold %d clients! Exiting...\n", max_client_size);
		server_exit();
	}
	
	select_loop();
	
	
	free(all_clients);
	fprintf(stdout, "Server exiting cleanly.\n");
	return 0;
}
