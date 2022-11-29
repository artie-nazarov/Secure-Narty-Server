// httpserver
// This module implements an HTTP server
// The server supports operations: PUT, GET, HEAD

#include "bind.h"
#include "http_object.h"

#include <err.h> // warnx
#include <errno.h>
#include <fcntl.h> // open()
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h> // uint16_t
#include <stdio.h> // printf
#include <stdlib.h> // strtol
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h> // read, write

#define PERM 770
#define CMDL_OPTIONS "t:l:"
#define LOG_BUFF_SIZE 128 // 4 Kb
// Log entry size = 8 (max method_name len) + 20 (max URI len) + 3 (3-digit status_code) + 10 (max len RequestID - uint32) + 3 (commas)
//                -> ceil(44) = 64 bytes
#define LOG_ENTRY_SIZE 64

// Global propoerites
// Determines whether the server is requested to be terminated (SIGTERM)
volatile int terminated = 0;

// Prints a help message
void help_message(char *program_name) {
    printf("usage: %s [-t threads] [-l logfile] <port>\n", program_name);
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
 * @param uri URI for the clients request
 * @param fd Server file descriptor used to recieve and send messages to the client
 * @return status_code
*/
int execute_PUT(http_object *obj, char *uri, int fd) {
    // Check that HTTP object complies with PUT format:
    //  1. Content-Length > 0
    int content_length = get_contentLength(obj);
    if (content_length < 0) {
        http_response(fd, HTTP_PROTOCOL, 400, 0, NULL);
    }
    //  2. Valid URI - open a file
    char *file_path = uri;
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
            return 403;
        } else {
            http_response(fd, HTTP_PROTOCOL, 500, 0, NULL);
            return 500;
        }
    }
    // Write the Message Body to file
    char *message = get_message(obj);
    flush(file, message, content_length);
    // Close file
    close(file);

    if (exists) {
        http_response(fd, HTTP_PROTOCOL, 200, 0, NULL);
        return 200;
    }
    http_response(fd, HTTP_PROTOCOL, 201, 0, NULL);
    return 201;
}

/** GET/HEAD
 * 
 * @param uri URI for the clients request
 * @param fd Server file descriptor used to recieve and send messages to the client
 * @param head_only A boolean option to only print out the head of a GET request
 * @return status_code
*/
int execute_GET(char *uri, int fd, bool head_only) {
    // Check that HTTP object complies with GET format:
    //  1. Valid URI - open a file
    char *file_path = uri;
    // Open File if it exists
    int file;
    file = open(file_path, O_RDONLY, PERM);
    if (file == -1) {
        if (errno == ENOENT) {
            http_response(fd, HTTP_PROTOCOL, 404, 0, NULL);
            return 404;
        } else if (errno == EACCES || errno == EPERM) {
            http_response(fd, HTTP_PROTOCOL, 403, 0, NULL);
            return 403;
        } else {
            http_response(fd, HTTP_PROTOCOL, 500, 0, NULL);
            return 500;
        }
    }
    // read the contents of a requested file into a buffer
    struct stat st;
    if (stat(file_path, &st) == -1) {
        http_response(fd, HTTP_PROTOCOL, 500, 0, NULL);
        return 500;
    }
    // Check if the file is a directory
    if (!S_ISREG(st.st_mode)) {
        http_response(fd, HTTP_PROTOCOL, 403, 0, NULL);
        close(file);
        return 403;
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

    return 200;
}

/** Handle SIGTERM
 * A signal handler function.
 * @param sig signal type recieved
 * @return Void
*/
void sigterm_handler(int sig) {
    printf("Need to terminate here!%d\n", sig);
    terminated = 1;
    return;
}

// HTTP server driver
int main(int argc, char **argv) {
    char *pr_name = argv[0] + 2;
    // Parse command line argument: uint16 port number
    // TODO: check with ref implementation what the message should look like
    if (argc < 2) {
        warnx("wrong arguments: %s port_num", argv[0]);
        help_message(pr_name);
        exit(1);
    }
    // Parse command line arguments
    int opt;
    int threads = 4;
    int logfile = STDERR_FILENO;
    while ((opt = getopt(argc, argv, CMDL_OPTIONS)) != -1) {
        switch (opt) {
            case 't':
                threads = (int) strtol(optarg, NULL, 10);
                if (errno) {
                    // TODO: invalid argument message
                    warnx("invalid number of threads");
                    exit(1);
                }
                break;
            case 'l':
                // Open logfile
                logfile = open(optarg, O_WRONLY, O_APPEND);
                if (logfile == -1) {
                    // TODO: invalid file name message
                    warnx("invalid file name");
                    exit(1);
                }
                break;
            default: break;
        }
    }
    // Parse port number
    uint16_t port_num = -1;
    if (optind + 1 == argc) {
        port_num = (uint16_t) strtol(argv[optind], NULL, 10);
        if (errno) {
            warnx("invalid port number: %s", argv[1]);
            exit(1);
        }
    }
    // TODO: figure out how to handle options after positional argument
    else {
        //TODO: message too many or too few arguments provided
        warnx("message too many arguments provided");
        printf("argc: %d optind: %d", argc, optind);
        exit(1);
    }

    printf("t: %d, l: %d, port_num: %d\n", threads, logfile, port_num);
    
    // Signal handler
    struct sigaction sa;
    sa.sa_handler = &cleanup;
    sigaction(SIGTERM, &sa, NULL);

    // Initialize Multithreading properties
    pthread_mutex_t logfile_mutex;
    if (pthread_mutex_init(&logfile_mutex, NULL)) {
        // TODO: see if this message is ok
        warnx("internal server error");
        exit(1);
    }

    // Initialize Logfile buffer
    // This object IS NOT thread-safe and must be used within a logfile_mutex lock
    char log_buff[LOG_BUFF_SIZE];
    int log_cap = 0;

    // Create a server socket descriptor on port_num
    int socketd = create_listen_socket(port_num);

    // Start the server
    int fd;
    while (!terminated) {
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
        char *method_str = "";
        char *uri = get_URI(obj);
        int request_id = get_request_id(obj);
        // Check if an error has been produced while processing a request
        if (response != 0) {
            // Error
            http_response(fd, HTTP_PROTOCOL, response, 0, NULL);
        } else {
            // Perform requested action
            HTTP_METHOD method = get_method(obj);
            switch (method) {
            case PUT: response = execute_PUT(obj, uri, fd);
                      method_str = "PUT";
                      break;
            case GET: response = execute_GET(uri, fd, false);
                      method_str = "GET";
                      break;
            case HEAD: response = execute_GET(uri, fd, true);
                       method_str = "HEAD";
                       break;
            default: break;
            }
        }

        // Logfile processing
        pthread_mutex_lock(&logfile_mutex);
        // Construct log entry
        char log_entry[LOG_ENTRY_SIZE];
        snprintf(log_entry, LOG_ENTRY_SIZE, "%s,%s,%d,%d\n", method_str, uri, response, request_id);
        int log_entry_len = (int)strlen(log_entry);
        // Check if log_buff is ready to be flushed
        if (LOG_BUFF_SIZE - log_cap < log_entry_len) {
            flush(logfile, log_buff, log_cap);
            log_cap = 0;
        }
        // Add log_entry to log_buff
        strcpy(log_buff + log_cap, log_entry);
        log_cap += log_entry_len;
        pthread_mutex_unlock(&logfile_mutex);

        // Free memory
        delete_http(&obj);

        // Reset errno
        errno = 0;

        // Close connection
        close(fd);
    }

    // Close logfile
    if (logfile != STDERR_FILENO) {
        close(logfile);
    }

    // Close socket
    shutdown(socketd, SHUT_RDWR);

    // Here we want to do server "cleanup" - destroy threads, mutexes, flush out logfile data, etc.
    pthread_mutex_destroy(&logfile_mutex);

    return 0;
}
