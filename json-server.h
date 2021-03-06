/*json-server.h
 *
 * 
 *
 * holds all constants necessary for an event-driven json web server
 * ---- NOTE: it's not done yet... 
 */
#ifndef JSON_SERVER_H
#define JSON_SERVER_H


#define READ_STATE  	20 
#define WRITE_STATE 	30
#define FORTUNE_STATE 	40
#define QUIT_STATE	50
#define DYING_STATE     60

#define RESIZE_READ_BUFFER   444
#define RESIZE_WRITE_BUFFER  555


/*types to fill the write buffer with! */
#define TYPE_IMPLEMENTED 	1
#define TYPE_STATUS 		2
#define TYPE_FORTUNE 		3
#define TYPE_ABOUT 		4
#define TYPE_QUIT		5
#define TYPE_404		6

struct server_stats{
	int num_clients;
	int num_requests;
	int errors;
	long uptime;
	long cpu_time;
	long memory_used;
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
	float response_http_version;

	//lol why not add more to this struct!
	int   fortune_fd;
};

#define GENERIC_HEADER 	"HTTP/%.1f 200 OK\r\n"\
			"Content-Type: application/json\r\n"\
			"Content-Length: %d\r\n"\
			"\r\n"

#define ERROR_404 	"HTTP/%.1f 404 Not Found\r\n"\
			"Content-Type: text/html\r\n"\
			"Content-Length: 162\r\n"\
			"\r\n"\
			"<HTML><HEAD><TITLE>HTTP ERROR 404</TITLE></HEAD><BODY>404 Not Found.  "\
			"Your request could not be completed due to encountering HTTP error number 404."\
			"</BODY></HTML>\r\n"

#define IMPLEMENTED_REQUEST_SPACE 	"/json/implemented.json "
#define IMPLEMENTED_REQUEST_NEWLINE 	"/json/implemented.json\r\n"
#define IMPLEMENTED_RESPONSE ""\
			"HTTP/%.1f 200 OK\r\n"\
			"Content-Type: application/json\r\n"\
			"Content-Length: 193\r\n"\
			"\r\n"\
			"[\r\n"\
			"{ \"feature\": \"about\", \"URL\": \"/json/about.json\"},\r\n"\
			"{ \"feature\": \"quit\", \"URL\": \"/json/quit\"},\r\n"\
			"{ \"feature\": \"status\", \"URL\": \"/json/status.json\"},\r\n"\
			"{ \"feature\": \"fortune\", \"URL\": \"/json/fortune\"}\r\n"\
			"]\r\n"

#define ABOUT_REQUEST_SPACE 	"/json/about.json "
#define ABOUT_REQUEST_NEWLINE 	"/json/about.json\r\n"

#define ABOUT_RESPONSE  "HTTP/%.1f 200 OK\r\n"\
			"Content-Type: application/json\r\n"\
			"Content-Length: 81\r\n"\
			"\r\n"\
			"{\r\n"\
			"  \"author\": \"Calvin Laverty\",  \"email\": \"claverty@calpoly.edu\",  \"major\": \"CPE\"\r\n"\
			"}\r\n"

#define STATUS_REQUEST_SPACE 	"/json/status.json "
#define STATUS_REQUEST_NEWLINE 	"/json/status.json\r\n"

#define FORTUNE_REQUEST_SPACE 	"/json/fortune "
#define FORTUNE_REQUEST_NEWLINE "/json/fortune\r\n"
#define FORTUNE_HEADER 	"{ \"fortune\": \""

#define QUIT_REQUEST_SPACE 	"/json/quit "
#define QUIT_REQUEST_NEWLINE 	"/json/quit\r\n"
#define QUIT_RESPONSE 	"HTTP/%.1f 200 OK\r\n"\
			"Content-Type: application/json\r\n"\
			"Content-Length: 26\r\n"\
			"\r\n"\
			"{\r\n"\
			"  \"result\": \"success\"\r\n"\
			"}\r\n"
/*end json-server.h */
#endif
