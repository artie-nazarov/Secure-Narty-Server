#include "httprequest.h"
#include <stdio.h>

// handles HTTP requests to server, returns status code

// char *handle_get(char *URI);
// char *handle_put(char *URI, char *messageBody, int messageLength);
// char *handle_head(char *URI);
int handle_get(httprequest *httpobj, int sock);
int handle_put(httprequest *httpobj, int sock);
int handle_head(httprequest *httpobj, int sock);

// helper functions
char *strnstr(const char *s, const char *find, size_t slen);
char *parse(char *str, const char *delim, size_t len);
int is_dir(const char* path);
int get_size(int fd);
int wr_ok(const char *path);
