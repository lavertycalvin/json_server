#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "json-server.h"

struct addrinfo hints, *matches, *server;
int get_info_ret;

int listening_socket_fd = 0;


//Returns the total size of the virtual address space for the running process
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

void create_listening_socket(){
	int bind_ret = 0;
	//loop through returned link list
	for(server = matches; server != NULL; server = server->ai_next){
		//try and create a socket
		listening_socket_fd = socket(server->ai_family, server->ai_socktype, server->ai_protocol);	
		if(listening_socket_fd == -1){
			//error creating the socket
			fprintf(stderr, "Create socket error...\n");
			continue;
		}
		fprintf(stderr, "Designated FD for listening socket: %d\n", listening_socket_fd);
		bind_ret = bind(listening_socket_fd, server->ai_addr, server->ai_addrlen);
		if(bind_ret == -1){
			close(listening_socket_fd);
			fprintf(stderr, "Bind error... \n");
			continue;
		}
		//don't create and bind to more than one socket
		
		return;

	}

	fprintf(stderr, "No Sockets bound! Exiting...\n");
	exit(-2);
}

void select_loop(){
	fprintf(stderr, "Starting select loop\n");
	while(1);
}

/* returns port in host order */
int getport(struct sockaddr *server){
	int port = 0;
	if(server->sa_family == AF_INET){
		port = ((struct sockaddr_in *)server)->sin_port
	}
	else{
	
	}
	return ntohs(port);
}

int main(int argc, char **argv){

	
	if(argc > 2){
		fprintf(stderr, "Usage: \n");
		exit(-1);
	}
	
	
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family   = AF_UNSPEC; //either v4 or v6
	hints.ai_socktype = SOCK_STREAM; //tcp 

	if(argc == 2){
		fprintf(stderr, "Setting TCP port to bind to this address: %s\n", argv[1]);
		if((get_info_ret = getaddrinfo(argv[1], NULL, &hints, &matches)) != 0){
			fprintf(stderr, "Error with addresses: %s\n", gai_strerror(get_info_ret));
		}	
	}
	else{
		//we were not provided with a binding address, bind to all
		fprintf(stderr, "Setting TCP port to bind any address!\n");
		hints.ai_flags    = AI_PASSIVE; //bind to all addresses
		if((get_info_ret = getaddrinfo(NULL, NULL, &hints, &matches)) != 0){
			fprintf(stderr, "Error with addresses: %s\n", gai_strerror(get_info_ret));
		}	
	}

	
	create_listening_socket();
	//done binding address, so we need to free address info
	freeaddrinfo(matches);

	//listen on server socket
	//change this 34 to a better number later!
	if(listen(listening_socket_fd, 34) != 0){
		fprintf(stderr, "Unable to listen on server socket\n");
		exit(-3);
	}

	if(getsockname(listening_socket_fd, (struct sockaddr *)&server->ai_addr, &server->ai_addrlen) == -1){
		fprintf(stderr, "Unable to get socket name!\n");
		exit(-4);
	}

	struct sockaddr_in server_address;
	int port = getport(server->ai_addr);
	fprintf(stdout, "HTTP server is using TCP port %d\n"
			"HTTPS server is using TCP port -1\n", ntohs(server->ai_addr->);

	fflush(stdout);

	get_memory_usage_linux();
	select_loop();
	
	return 1;
}
