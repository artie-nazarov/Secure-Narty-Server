
#pragma once

#include <stdbool.h>

#define HTTP_PROTOCOL "HTTP/1.1"

// HTTP object helps build the HTTP object from a user request, by reading and parsing request header information

// HTTP valid Methods
typedef enum { NONE, PUT, GET, HEAD } HTTP_METHOD;

// HTTP object struct
typedef struct http_object http_object;

// Create object
http_object *create_http();

// Delete object
void delete_http(http_object **obj);

// Parse HTTP request
int process_request(http_object *obj);

// Get Method name
HTTP_METHOD get_method(http_object *obj);

// Get Message Body
char *get_message(http_object *obj);

// Get Content Length
int get_contentLength(http_object *obj);

// Get URI
char *get_URI(http_object *obj);
