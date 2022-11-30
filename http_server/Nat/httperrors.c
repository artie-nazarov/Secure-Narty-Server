#include "httperrors.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>

/*
    given an error number, outputs HTTP response to the given file desctriptor and closes the connection 
*/
void fail(int statusCode, int in) {
    char * response;
    switch (statusCode) {
        case 400:
            response = "HTTP/1.1 400 Bad Request\r\nContent-Length: 11\r\n\r\nBad Request\n";
            break;
        case 403:
            response = "HTTP/1.1 403 Forbidden\r\nContent-Length: 9\r\n\r\nForbidden\n";
            break;
        case 404:
            response = "HTTP/1.1 404 Not Found\r\nContent-Length: 9\r\n\r\nNot Found\n";
            break;
        case 500:
            response = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 21\r\n\r\nInternal Server Error\n";
            break;
        case 501:
            response = "HTTP/1.1 501 Not Implemented\r\nContent-Length: 15\r\n\r\nNot Implemented\n";
            break;
        default:
            response = "HTTP/1.1 501 Not Implemented\r\nContent-Length: 15\r\n\r\nNot Implemented\n";
            break;
    }
    write(in, response, strlen(response));
    close(in);
}
