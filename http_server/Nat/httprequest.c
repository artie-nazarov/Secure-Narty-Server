#include "operators.h"
#include "httprequest.h"
#include "httperrors.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h> 
#include <unistd.h>
#include <string.h>

static int logBytes = 0;

httprequest *httprequest_init() {
    httprequest *httpobj = (httprequest *) malloc(sizeof(httprequest));
    if (!httpobj) {
        return NULL;
    }
    httpobj->method = "";
    httpobj->URI = "";
    httpobj->version = "";
    strcpy(httpobj->headerField, "");
    httpobj->statusCode = -1;
    httpobj->requestID = 0;
    httpobj->messageBody = "";
    httpobj->messageBodyAlloced = false;
    return httpobj;
}

void httprequest_delete(httprequest **httpobj) {
    if ((*httpobj)->messageBodyAlloced) {
        free((*httpobj)->messageBody);
    }
    if (httpobj) {
        free(*httpobj);
    }
}

/************************ 
    httprequest functions
************************/

/**
 * Listens to client over the given file descriptor, populating httpobj fields based on input
 * @param httpobj pointer to httprequest to be constructed
 * @param in_fd file descriptor of our client
 * */
int construct_request(httprequest *httpobj, int in_fd) {
    int bytesRead;
    int totalBytesRead = 0;
    int messageBodyLength = 0;
    char buffer[BUFFERLEN];
    char requestLine[BUFFERLEN];
    char *messageBodyTemp = NULL;
    char *headerFieldTemp = NULL;
    char* next;
    char* next2;

    // keep reading bytes until \r\n\r\n is read
    do {
        bytesRead = read(in_fd, buffer, BUFFERLEN);
        if (bytesRead <= 0) {
            fail(400, in_fd);
            httpobj->statusCode = 400;
            return -400;
        }
        totalBytesRead += bytesRead;
    } while((messageBodyTemp = parse(buffer, "\r\n\r\n", totalBytesRead)) == NULL);
    messageBodyLength = totalBytesRead - (messageBodyTemp - buffer);

    // get request line + header field
    headerFieldTemp = parse(buffer, "\r\n", bytesRead);
    memcpy(requestLine, buffer, strlen(buffer));
    next = parse(requestLine, " ", strlen(requestLine));
    if (next == NULL) {
        fail(400, in_fd);
        httpobj->statusCode = 400;
        return -400;
    }
    httpobj->method = requestLine;
    next2 = parse(next, " ", strlen(next));
    if (next2 == NULL) {
        fail(400, in_fd);
        return -400;
    }
    httpobj->URI = next;
    httpobj->version = next2;
    if (httpobj->URI[0] != '/' || strstr(httpobj->URI + 1, "/") != NULL || strlen(httpobj->URI) > 20) {
        fail(400, in_fd);
        httpobj->statusCode = 400;
        return -400;
    }

    if (strcmp(httpobj->version, "HTTP/1.1") != 0) {
        fail(400, in_fd);
        httpobj->statusCode = 400;
        return -400;
    }

    // validate header field grammar
    if (headerFieldTemp != NULL) {
        memcpy(httpobj->headerField, headerFieldTemp, strlen(headerFieldTemp) + 1);

        // now, validate header
        while (*httpobj->headerField) { // loop through all header fields
            char *endptr = parse(httpobj->headerField, ": ", strlen(httpobj->headerField));
            if (endptr == NULL) {
                fail(400, in_fd);
                httpobj->statusCode = 400;
                return -400;
            }

            char *endptr2 = parse(endptr, "\r\n", strlen(endptr));
            if (strcmp(httpobj->headerField, "Content-Length") == 0) {
                httpobj->contentLength = atoi(endptr);
            }
            if (strcmp(httpobj->headerField, "Request-Id") == 0) {
                httpobj->requestID = atoi(endptr);
            }
            
            if (endptr2 == NULL) {
                break;
            }
            else {
                endptr++;
            }
            strcpy(httpobj->headerField, endptr2);
        }
    }

    if (strcmp(httpobj->method, "PUT") == 0 && httpobj->contentLength == -1) {
        fail(400, in_fd);
        httpobj->statusCode = 400;
        return -400;
    }

    // if method is PUT, read message of length content-length from buffer
    if (strcmp(httpobj->method, "PUT") == 0) {
        httpobj->messageBody = malloc(httpobj->contentLength);
        httpobj->messageBodyAlloced = true;
        memcpy(httpobj->messageBody, messageBodyTemp, messageBodyLength);
        while(messageBodyLength < httpobj->contentLength) {
            bytesRead = read(in_fd, buffer, (BUFFERLEN < httpobj->contentLength - messageBodyLength ? BUFFERLEN : httpobj->contentLength - messageBodyLength));
            if (bytesRead <= 0) break;
            memcpy(httpobj->messageBody + messageBodyLength, buffer, bytesRead);
            messageBodyLength += bytesRead;
        }
    }

    if (httpobj->method == NULL || httpobj->URI == NULL) {

        fail(400, in_fd);
        httpobj->statusCode = 400;
        return -400;
    }

    // if method not implemented...
    if ((strcmp(httpobj->method, "GET") != 0) && (strcmp(httpobj->method, "PUT") != 0) && (strcmp(httpobj->method, "HEAD") != 0)) {
        fail(501, in_fd);
        httpobj->statusCode = 501;
        return -501;
    }
    if (httpobj->contentLength != -1 && messageBodyLength != httpobj->contentLength) {
        fail(400, in_fd);
        httpobj->statusCode = 400;
        return -400;
    }
    return messageBodyLength;
}

void handle_request(httprequest *httpobj, int sock) {
    if (strcmp(httpobj->method, "GET") == 0) {
        httpobj->statusCode = handle_get(httpobj, sock);
    }
    else if (strcmp(httpobj->method, "PUT") == 0) {
        httpobj->statusCode = handle_put(httpobj, sock);
    }
    else if (strcmp(httpobj->method, "HEAD") == 0) {
        httpobj->statusCode = handle_head(httpobj, sock);
    }
    else {
        fail(400, sock);
        httpobj->statusCode = 400;
    }
}

/*
 * creates and writes to log_fd an audit log entry of the type:
 * <Oper>,<URI>,<Status-Code>,<RequestID header value>\n
 * Ex: GET,/hello.txt,200,1
 */
void log_request(httprequest *httpobj, char *logbuffer, int log_fd) {
    if (httpobj->statusCode != 200 && httpobj->statusCode != 201 && httpobj->statusCode != 404 && httpobj->statusCode != 500) {
        return;
    }
    // get status code string
    int length = snprintf( NULL, 0, "%d", httpobj->statusCode );
    char* statusCodeStr = malloc( length + 1 );
    snprintf( statusCodeStr, length + 1, "%d", httpobj->statusCode );
    // get requestID string
    length = snprintf( NULL, 0, "%d", httpobj->requestID );
    char* requestIDStr = malloc( length + 1 );
    snprintf( requestIDStr, length + 1, "%d", httpobj->requestID );

    char *log = malloc( strlen(httpobj->method) + strlen(httpobj->URI) + strlen(statusCodeStr) + strlen(requestIDStr)  + 4);
    strcpy(log, httpobj->method);
    strcat(log, ",");
    strcat(log, httpobj->URI);
    strcat(log, ",");
    strcat(log, statusCodeStr);
    strcat(log, ",");
    strcat(log, requestIDStr);
    strcat(log, "\n");

    if (logBytes + strlen(log) >= 4096) {
        write_logs(logbuffer, log_fd);
    }
    strcat(logbuffer, log);
    logBytes += strlen(log);

    free(statusCodeStr);
    free(requestIDStr);
    free(log);
    return;
}

void write_logs(char *logbuffer, int log_fd) {
    write(log_fd, logbuffer, logBytes);
    strcpy(logbuffer, "");
    logBytes = 0;
}
