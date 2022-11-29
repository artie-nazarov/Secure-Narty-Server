// httpserver
// This module implements an HTTP server
// The server supports operations: PUT, GET, HEAD

#include "bind.h"
#include "http_object.h"

#include <sys/socket.h>
#include <unistd.h> // read, write
#include <stdio.h> // printf
#include <err.h> // warnx
#include <stdint.h> // uint16_t
#include <errno.h>
#include <stdlib.h> // strtol
#include <stdbool.h>
#include <fcntl.h> // open()
#include <sys/stat.h>
#include <string.h>

#define PERM 770

// Prints a help message
void help_message(char *program_name) {
    printf("usage: %s <port>\n", program_name);
    return;
}

// Writes <num_bytes> into file descriptor <out> from a buffer
void flush(int out, char *buff, int num_bytes) {
    int bytes = 0;
    int bytes_flushed = 0;
    do {
        bytes_flushed = write(out, buff + bytes, num_bytes - bytes);
        bytes += bytes_flushed;
    } while (bytes_flushed > 0);
    return;
}

/** Produces and sends an HTTP Response to the client
 * 
 * @param fd Server file descriptor used to recieve and send messages to the client
 * @param protocol HTTP protocol (version)
 * @param status_code HTTP status code produced when parsing HTTP request
 * @param content_length Length of the message sent to the client
 * @param message Response message
 * @return Void
*/
void http_response(int fd, char *protocol, int status_code, int content_length, char *message) {
    // Identify request status-code
    char *status_phrase = "";
    switch (status_code) {
    case 200: status_phrase = "OK"; break;
    case 201: status_phrase = "Created"; break;
    case 400: status_phrase = "Bad Request"; break;
    case 403: status_phrase = "Forbidden"; break;
    case 404: status_phrase = "Not Found"; break;
    case 500: status_phrase = "Internal Server Error"; break;
    case 501: status_phrase = "Not Implemented"; break;
    default: break;
    }
    // HTTP/<version> <3-digit status-code> <status-message>
    char status_buff[64];
    snprintf(status_buff, 64, "%s %d %s\r\n", protocol, status_code, status_phrase);
    // a content_length converted to string can be at most 19 characters (long long max)
    if (message == NULL) {
        content_length = strlen(status_phrase) + 1;
    }
    char cl_buff[64];
    snprintf(cl_buff, 64, "Content-Length: %d\r\n\r\n", content_length);

    flush(fd, status_buff, strlen(status_buff));
    flush(fd, cl_buff, strlen(cl_buff));

    if (message == NULL) {
        flush(fd, status_phrase, content_length);
        flush(fd, "\n", 1);
    } else if (strcmp(message, "") != 0) {
        flush(fd, message, content_length);
    }

    return;
}

// Execute HTTP request methods
/** PUT
 * 
 * @param obj http_object with relevent information about the client's request
 * @param fd Server file descriptor used to recieve and send messages to the client
 * @return Void
*/
void execute_PUT(http_object *obj, int fd) {
    // Check that HTTP object complies with PUT format:
    //  1. Content-Length > 0
    int content_length = get_contentLength(obj);
    if (content_length < 0) {
        http_response(fd, HTTP_PROTOCOL, 400, 0, NULL);
    }
    //  2. Valid URI - open a file
    char *file_path = get_URI(obj);
    // Open File if it exists
    int file;
    bool exists = true;
    file = open(file_path, O_WRONLY | O_TRUNC, PERM);
    if (file == -1 && errno == ENOENT) {
        file = open(file_path, O_CREAT | O_WRONLY, PERM);
        exists = false;
    }
    if (file == -1) {
        if (errno == EACCES || errno == EPERM || errno == EISDIR) {
            http_response(fd, HTTP_PROTOCOL, 403, 0, NULL);
            return;
        } else {
            http_response(fd, HTTP_PROTOCOL, 500, 0, NULL);
            return;
        }
    }
    // Write the Message Body to file
    char *message = get_message(obj);
    flush(file, message, content_length);

    if (exists) {
        http_response(fd, HTTP_PROTOCOL, 200, 0, NULL);
    } else {
        http_response(fd, HTTP_PROTOCOL, 201, 0, NULL);
    }

    // Close file
    close(file);

    return;
}

/** GET/HEAD
 * 
 * @param obj http_object with relevent information about the client's request
 * @param fd Server file descriptor used to recieve and send messages to the client
 * @param head_only A boolean option to only print out the head of a GET request
 * @return Void
*/
void execute_GET(http_object *obj, int fd, bool head_only) {
    // Check that HTTP object complies with GET format:
    //  1. Valid URI - open a file
    char *file_path = get_URI(obj);
    // Open File if it exists
    int file;
    file = open(file_path, O_RDONLY, PERM);
    if (file == -1) {
        if (errno == ENOENT) {
            http_response(fd, HTTP_PROTOCOL, 404, 0, NULL);
            return;
        } else if (errno == EACCES || errno == EPERM) {
            http_response(fd, HTTP_PROTOCOL, 403, 0, NULL);
            return;
        } else {
            http_response(fd, HTTP_PROTOCOL, 500, 0, NULL);
            return;
        }
    }
    // read the contents of a requested file into a buffer
    struct stat st;
    if (stat(file_path, &st) == -1) {
        http_response(fd, HTTP_PROTOCOL, 500, 0, NULL);
        return;
    }
    // Check if the file is a directory
    if (!S_ISREG(st.st_mode)) {
        http_response(fd, HTTP_PROTOCOL, 403, 0, NULL);
        close(file);
        return;
    }
    long long content_length = st.st_size;
    // GET method
    if (!head_only) {
        // Read bytes from URI
        char *buff = (char *) malloc(content_length * sizeof(char));
        int bytes = 0, bytes_read = 0;
        do {
            bytes_read = read(file, buff, content_length - bytes);
            bytes += bytes_read;
        } while (bytes_read > 0);
        // HTTP response
        http_response(fd, HTTP_PROTOCOL, 200, content_length, buff);

        // Free memory
        free(buff);
    }
    // HEAD method
    else {
        // HTTP response
        http_response(fd, HTTP_PROTOCOL, 200, content_length, "");
    }

    // Close file
    close(file);

    return;
}

// HTTP server driver
int main(int argc, char **argv) {
    char *pr_name = argv[0] + 2;
    // Parse command line argument: uint16 port number
    if (argc < 2) {
        warnx("wrong arguments: %s port_num", argv[0]);
        help_message(pr_name);
        exit(1);
    }
    // Parse port number
    uint16_t port_num = (uint16_t) strtol(argv[1], NULL, 10);
    if (errno) {
        warnx("invalid port number: %s", argv[1]);
        exit(1);
    }

    // Create a server socket descriptor on port_num
    int socketd = create_listen_socket(port_num);

    // Start the server
    int fd;
    while (1) {
        // Establish connection with the client, create a file descriptor
        fd = accept(socketd, NULL, NULL);
        // Check if port connection was successful
        if (fd == -1) {
            warnx("bind: Address already in use");
            exit(1);
        }

        // Approach:
        // 1. Process request from the client via http_object
        // 2. If request parsed successfully, invoke an HTTP method that was requested, otherwise respond with an error code
        // 3. Close the connection upon completion

        // Create HTTP object
        http_object *obj = create_http(fd);

        // Process client's request. Create http_object with relevant metadata for a single request
        int response = process_request(obj);
        // Check if an error has been produced while processing a request
        if (response != 0) {
            // Error
            http_response(fd, HTTP_PROTOCOL, response, 0, NULL);
        } else {
            // Perform requested action
            HTTP_METHOD method = get_method(obj);
            switch (method) {
            case PUT: execute_PUT(obj, fd); break;
            case GET: execute_GET(obj, fd, false); break;
            case HEAD: execute_GET(obj, fd, true); break;
            default: break;
            }
        }

        // Free memory
        delete_http(&obj);

        // Reset errno
        errno = 0;

        // Close connection
        close(fd);
    }
    return 0;
}
