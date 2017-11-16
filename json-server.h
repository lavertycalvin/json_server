/*json-server.h
 * holds all constants necessary for an event-driven json web server
 * ---- NOTE: it's not done yet... 
 */
#ifndef JSON_SERVER_H
#define JSON_SERVER_H

struct server_stats{
	int num_clients;
	int num_requests;
	int errors;
	float uptime;
	float cpu_time;
	int memory_used;
};


/*end json-server.h */
#endif
