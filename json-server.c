#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "json-server.h"

struct addrinfo hints, *matches, *possible;
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
	//loop through returned link list
	for(possible = matches; possible != NULL; possible = possible->ai_next){
		//try and create a socket
	
	}

	fprintf(stderr, "No Sockets bound! Exiting...\n");
	exit(-2);
}

void select_loop(){
	fprintf(stderr, "Starting select loop\n");
	while(1);
}

int main(int argc, char **argv){

	
	if(argc > 2){
		fprintf(stderr, "Usage: \n");
		exit(-1);
	}
	
	
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family   = AF_UNSPEC; //either v4 or v6
	hints.ai_socktype = SOCK_STREAM;
	
	
	if(argc == 2){
		fprintf(stderr, "Setting TCP port to bind to this address: %s\n", argv[1]);
		if((get_info_ret = getaddrinfo(argv[1], "http", &hints, &matches)) != 0){
			fprintf(stderr, "Error with addresses: %s\n", gai_strerror(get_info_ret));
		}	
	}
	else{
		//we were not provided with a binding address, bind to all
		fprintf(stderr, "Setting TCP port to bind any address!\n");
		hints.ai_flags    = AI_PASSIVE;
		if((get_info_ret = getaddrinfo(NULL, "http", &hints, &matches)) != 0){
			fprintf(stderr, "Error with addresses: %s\n", gai_strerror(get_info_ret));
		}	
	}

	
	create_listening_socket();	
	
	fprintf(stdout, "HTTP server is using TCP port %d\n"
			"HTTPS server is using TCP port -1\n", listening_socket_fd);

	fflush(stdout);

	get_memory_usage_linux();
	select_loop();
	
	return 1;
}
