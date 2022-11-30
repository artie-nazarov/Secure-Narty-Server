#include "operators.h"
#include "httperrors.h"
#include "httprequest.h"
#include <stdio.h>
#include <fcntl.h> 
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <err.h>
#include <errno.h>

/************************ 
HELPER FUNCTIONS 
************************/
char *strnstr(const char *s, const char *find, size_t slen) {
	char c, sc;
	size_t len;

	if ((c = *find++) != '\0') {
		len = strlen(find);
		do {
			do {
				if (slen-- < 1 || (sc = *s++) == '\0')
					return (NULL);
			} while (sc != c);
			if (len > slen)
				return (NULL);
		} while (strncmp(s, find, len) != 0);
		s--;
	}
	return ((char *)s);
}

// Takes in string str and returns the substring from index 0 to the delimiter given
// returns substring and clips str to after delim
char *parse(char *str, const char *delim, size_t len) {
    char *p = strnstr(str, delim, len);

    if (p == NULL) {
        return NULL;     // delimiter not found
    }

    *p = '\0';                      // terminate string after head
    return p + strlen(delim);
}

int is_dir(const char* path) {
    struct stat statbuf;
    if( stat(path,&statbuf) == 0 ) {
        if (S_ISDIR(statbuf.st_mode) ) {
            return 1;
        }
    }
    return 0;
}

int get_size(int fd) {
    struct stat st;
    fstat(fd, &st);
    return st.st_size;
}

int wr_ok(const char *path) {
    return (access(path, W_OK) == 0);
}

// char *handle_get(char *URI) {
int handle_get(httprequest *httpobj, int sock) {
    int bufferLen = 2048;
    int in;
    ssize_t bytesRead;
    int retContentLen = 0;
    int filesize;
    char *retStatusLine = "HTTP/1.1 200 OK\r\n";
    char *retHeaderField = NULL;
    char *retBody = NULL;
    char *response = NULL;

    if (is_dir(httpobj->URI + 1)) {
        fail(403, sock);
        return 403;
    }
    // open URI
    in = open(httpobj->URI + 1, O_RDONLY);
    if (in < 0) {
        // file not accessable
        if (errno == EACCES || errno == EPERM || errno == EISDIR) {
            fail(403, sock);
            return 403;
        }
        // file doesn't exist
        else {
            fail(404, sock);
            return 404;
        }
    }
    filesize = get_size(in);
    retBody = malloc( filesize + 1 );
    // read
    while ((bytesRead = read(in, retBody, bufferLen)) != 0) {
        if (bytesRead < 0) {
            if (errno == EAGAIN)
                continue;
            fail(500, sock);
            return 500;
        }
        if (retContentLen + bytesRead > filesize) {
            fail(500, sock);
            return 500;
        }
        retContentLen += bytesRead;
    }
    close(in);

    int length = snprintf( NULL, 0, "%d", retContentLen );
    char* retContentLenStr = malloc( length + 1 );
    snprintf( retContentLenStr, length + 1, "%d", retContentLen );

    retHeaderField = malloc(strlen("Content-Length: ") + strlen(retContentLenStr) + strlen("\\r\\n\\r\\n") + 1);
    strcpy(retHeaderField, "Content-Length: ");
    strcat(retHeaderField, retContentLenStr);
    strcat(retHeaderField, "\r\n\r\n");

    response = malloc( strlen(retStatusLine) + strlen(retHeaderField) + retContentLen + 1);
    strcpy(response, retStatusLine);
    strcat(response, retHeaderField);
    strcat(response, retBody);
    free(retHeaderField);
    free(retContentLenStr);
    free(retBody);

    write(sock, response, strlen(response));
    free(response);
    return 200;
}

// char *handle_put(char *URI, char *messageBody, int messageLength) {
int handle_put(httprequest *httpobj, int sock) {
    int out;
    int statusCode = 200;
    size_t bytesWritten;
    char *response = NULL;
    char *retStatusLine = "HTTP/1.1 200 OK\r\nContent-Length: 3\r\n\r\nOK\n";
    
    if (is_dir(httpobj->URI + 1)) {
        fail(403, sock);
        return 403;
    }

    out = open(httpobj->URI + 1, O_WRONLY|O_TRUNC);
    if (out < 0 && errno == ENOENT) {
        out = open(httpobj->URI + 1, O_CREAT | O_WRONLY, 0666);
        retStatusLine = "HTTP/1.1 201 Created\r\nContent-Length: 7\r\n\r\nCreated\n";
        statusCode = 201;
    }
    if (out < 0) {
        // file not accessable
        if (errno == EACCES || errno == EPERM || errno == EISDIR) {
            fail(403, sock);
            return 403;
        }
        // if folder or parent dir doesn't exist
        else if (errno == ENOENT) {
            fail(404, sock);
            return 404;
        }
        // other file problem??
        else {
            fail(500, sock);
            return 500;
        }
    }
    // write the recieved messageBody to file
    bytesWritten = write(out, httpobj->messageBody, httpobj->contentLength);
    close(out);
    
    response = malloc(strlen(retStatusLine) + 1);
    strcpy(response, retStatusLine);

    write(sock, response, strlen(response));
    free(response);
    return statusCode;
}

int handle_head(httprequest *httpobj, int sock) {
    int in;
    int filesize;
    char *retStatusLine = "HTTP/1.1 200 OK\r\n";
    char *retHeaderField = NULL;
    char *response = NULL;
    
    if (is_dir(httpobj->URI + 1)) {
        fail(403, sock);
        return 403;
    }
    // open URI
    in = open(httpobj->URI + 1, O_RDONLY);
    if (in < 0) {
        // file not accessable
        if (errno == EACCES || errno == EPERM || errno == EISDIR) {
            fail(403, sock);
            return 403;
        }
        // file doesn't exist
        else {
            fail(404, sock);
            return 404;
        }
    }
    filesize = get_size(in);
    close(in);
    
    int length = snprintf( NULL, 0, "%d", filesize );
    char* retContentLenStr = malloc( length + 1 );
    snprintf( retContentLenStr, length + 1, "%d", filesize);

    retHeaderField = malloc( strlen("Content-Length: ") + strlen(retContentLenStr) + strlen("\\r\\n\\r\\n") + 1 );
    strcat(retHeaderField, "Content-Length: ");
    strcat(retHeaderField, retContentLenStr);
    strcat(retHeaderField, "\r\n\r\n");

    response = malloc( strlen(retStatusLine) + strlen(retHeaderField) + 1 );
    strcat(response, retStatusLine);
    strcat(response, retHeaderField);
    free(retHeaderField);
    free(retContentLenStr);

    write(sock, response, strlen(response));
    free(response);
    return 200;
}
