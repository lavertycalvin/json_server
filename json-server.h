/*json-server.h
 * 
 *
 * holds all constants necessary for an event-driven json web server
 * ---- NOTE: it's not done yet... 
 */
#ifndef JSON_SERVER_H
#define JSON_SERVER_H


#define READ_STATE  20 
#define WRITE_STATE 30

#define RESIZE_READ_BUFFER   444
#define RESIZE_WRITE_BUFFER  555


struct server_stats{
	int num_clients;
	int num_requests;
	int errors;
	float uptime;
	float cpu_time;
	int memory_used;
};

struct client{
	int   is_alive;
	int   socket_fd;
	char *read_buffer;
	int   read_buffer_size;
	int   bytes_read;
	char *write_buffer;
	int   write_buffer_size;
	int   bytes_written;
	int   current_step;
};


/*end json-server.h */
#endif
