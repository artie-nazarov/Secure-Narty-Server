#ifndef __HTTPREQUEST_H__
#define __HTTPREQUEST_H__
#include <stdbool.h>

#define BUFFERLEN 2048

typedef struct httprequest {
    char *method;
    char *URI;
    char *version;
    char headerField[BUFFERLEN];
    char *messageBody;
    bool messageBodyAlloced;
    int contentLength;
    int requestID;
    int statusCode;
} httprequest;

httprequest *httprequest_init();
void httprequest_delete(httprequest **httpobj);
int construct_request(httprequest *httpobj, int in_fd);
void handle_request(httprequest *httpobj, int in_fd);
void log_request(httprequest *httpobj, char *logbuffer, int log_fd);
void write_logs(char *logbuffer, int log_fd);

#endif
