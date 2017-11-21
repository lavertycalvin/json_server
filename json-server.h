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


/*types to fill the write buffer with! */
#define TYPE_IMPLEMENTED 	1
#define TYPE_STATUS 		2
#define TYPE_FORTUNE 		3
#define TYPE_ABOUT 		4
#define TYPE_404		5

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
	int   response_size;
	int   bytes_written;
	int   current_step;
};

#define ERROR_404 	"HTTP/1.0 404 Not Found\n"\
			"Content-Type: text/html\n"\
			"Content-Length: 162\n"\
			"\n"\
			"<HTML><HEAD><TITLE>HTTP ERROR 404</TITLE></HEAD><BODY>404 Not Found.  "\
			"Your request could not be completed due to encountering HTTP error number 404."\
			"</BODY></HTML>\n\0"

#define IMPLEMENTED_REQUEST_LENGTH 26
#define IMPLEMENTED_REQUEST "GET /json/implemented.json\n"

#define IMPLEMENTED_RESPONSE ""\
			"HTTP/1.0 200 OK\n"\
			"Content-Type: application/json\n"\
			"Content-Length: 193\n"\
			"\n"\
			"[\n"\
			"{ \"feature\": \"about\", \"URL\": \"/json/about.json\"},"\
			"{ \"feature\": \"quit\", \"URL\": \"/json/quit\"},"\
			"{ \"feature\": \"status\", \"URL\": \"/json/status.json\"},"\
			"{ \"feature\": \"fortune\", \"URL\": \"/json/fortune\"}]\n\0"

#define ABOUT_REQUEST_LENGTH 20
#define ABOUT_REQUEST "GET /json/about.json\n"

#define ABOUT_RESPONSE_LENGTH 
#define ABOUT_RESPONSE  "HTTP/1.0 200 OK\n"\
			"Content-Type: application/json\n"\
			"Content-Length: 81\n"\
			"\n"\
			"{\n"\
			"  \"author\": \"Calvin Laverty\",  \"email\": \"claverty@calpoly.edu\",  \"major\": \"CPE\"}\n\0"

#define STATUS_REQUEST_LENGTH 21
#define STATUS_REQUEST "GET /json/status.json\n"

#define FORTUNE_REQUEST_LENGTH 17
#define FORTUNE_REQUEST "GET /json/fortune\n"


/*end json-server.h */
#endif
