#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "json-server.h"

struct sockaddr_in listening_socket_v4;
struct sockaddr_in6 listening_socket_v6;
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

}

int parse_address_for_version(char *address){
	int ret = 1;	
	if(!inet_pton(AF_INET, address, &listening_socket_v4.sin_addr)){
		//try v6 address
		ret = inet_pton(AF_INET6, address, &listening_socket_v6.sin6_addr);
	}


	return ret;				
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
	if(argc == 2){
		fprintf(stderr, "Setting TCP port to bind to this address: %s\n", argv[1]);
		if(!parse_address_for_version(argv[1])){
			fprintf(stderr, "Unable to assign address.....\n");
			exit(-1);
		}
	}
	else{
		//we were not provided with a binding address, bind to all
		
	}

	
	create_listening_socket();	
	
	fprintf(stdout, "HTTP server is using TCP port %d\n"
			"HTTPS server is using TCP port -1\n", listening_socket_fd);

	fflush(stdout);

	get_memory_usage_linux();
	select_loop();
	
	return 1;
}
