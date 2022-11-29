// http_object implementation
// This module is a convenient object representation of an HTTP request
// Functionality:
//  1. Parses HTTP requests
//  2. Stores parsed metadata for server use

#include "http_object.h"

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>

#define MAX_HEADER_LEN 2048
#define DELIM          "\r\n"

// HTTP object structure
struct http_object {
    // Socket file descriptor
    int fd;

    // HTTP request fields
    HTTP_METHOD method;
    char *uri;
    char *message_body;

    // Header Fields
    uint32_t content_length;
    uint32_t request_id;
};

/** Create http_object
 * @param fd Socket file descriptor. Used strictly for reading information from a client
 * @return http_object
*/
http_object *create_http(int fd) {
    http_object *obj = (http_object *) calloc(1, sizeof(http_object));
    if (obj == NULL) {
        return NULL;
    }
    obj->fd = fd;
    // HTTP request fields
    obj->method = NONE;
    obj->uri = NULL;
    obj->message_body = NULL;

    // Header Fields
    obj->content_length = -1;
    obj->request_id = 0;

    return obj;
}

/** Delete http_object
 * @param obj A pointer to a pointer for an http_object
 * @return Void
*/
void delete_http(http_object **obj) {
    if (obj && *obj) {
        http_object *O = *obj;
        if (O->uri) {
            free(O->uri);
            O->uri = NULL;
        }
        if (O->message_body) {
            free(O->message_body);
            O->message_body = NULL;
        }
        free(*obj);
        *obj = NULL;
    }
    return;
}

// Getter Methdos
// Get Method name
HTTP_METHOD get_method(http_object *obj) {
    return obj->method;
}
// Get Message Body
char *get_message(http_object *obj) {
    return obj->message_body;
}
// Get Content Length
int get_contentLength(http_object *obj) {
    return obj->content_length;
}
// Get URI
char *get_URI(http_object *obj) {
    return obj->uri;
}
// Get Request ID
int get_request_id(http_object *obj) {
    return obj->request_id;
}

// Private helper functionality
/** Split
 * Attempts to match a given delimiter in a given input
 * If matched, replace the delimiter with a \0 (null terminator) and return a pointer to the element following that delimiter
 * If not matched, return NULL
 * @param input Input string
 * @param delim Delimiter
 * @param n number of bytes in inpit to consider
 * @return pointer to a value in input or NULL
*/
char *split(char *input, char *delim, int n) {
    int len = strlen(delim);
    int num_matches = 0;
    char *ptr = NULL;
    for (int i = len - 1; i < n; i++) {
        for (int j = len - 1; j >= 0; j--) {
            // Did not match
            if (delim[j] != input[i - (len - 1 - j)]) {
                break;
            }
            // Match a single byte, save the pointer
            ptr = input + (i - (len - 1 - j));
            num_matches++;
        }
        if (num_matches == len) {
            // Replace delimiter with null character.
            *ptr = '\0';
            // Offset pointer to the remainder of buffer past the delimiter
            return ptr + len;
        }
        num_matches = 0;
    }
    return NULL;
}

/** Process Segment
 * Attempts to process a segment of a given input
 * Given a buffer, continously read bytes into it from a file until a delimiter has been encountered
 * If a delimiter is never encountered, return HTTP 400 Error.
 * A caller of this funciton assumes that a delimiter MUST be read, otherwise an error has occured 
 * @param buff Input buffer. Used for receiving HTTP request line
 * @param end a pointer to a pointer of a byte sequence
 * @param file input file descriptor
 * @param bytes global number of bytes that have already been read from file
 * @param delimiter Delimiter
 * @param read_incomplete tracks whether read() read the last byte from the input stream (read() returned 0)
 * 
 * @return val >= 0: Returns the global number of bytes read from a request
 * @return val < 0: Returns a negative error code for an HTTP error that occured (actual error number == val * (-1))
*/
int process_segment(
    char buff[], char **end, int file, int bytes, char *delimiter, bool *read_incomplete) {
    int bytes_read = 0, redundancy_offset = 0;
    char *sub;
    int delim_len = strlen(delimiter);
    do {
        bytes_read = read(file, buff + bytes, MAX_HEADER_LEN - bytes);
        // Use redundancy_offset to make sure we rescan bytes that could be a part of the delimiter
        // Eg. if we are scanning 1 byte at a time, we want to offset bytes scanned backwards in case read() cuts off in the middle of the delimiter
        //     \r | \n  ->  ('|' is the read() cutoff point) we want to offset to rescan '\r' after we read '\n' in the next read() call
        redundancy_offset = (bytes > delim_len - 1) ? delim_len - 1 : bytes;
        sub = split(buff + bytes - redundancy_offset, delimiter, bytes_read + redundancy_offset);
        bytes += bytes_read;
    } while (sub == NULL && bytes_read > 0);
    if (sub == NULL) {
        // 400 Error - Parsing grammar couldn't match input
        return -400;
    }
    // buff now acts as a request line string
    // Offset buffer pointer to the beginning of Header Fields
    *end = sub;
    *read_incomplete = bytes_read;
    return bytes;
}

/** Parse HTTP Status Line
 * @param obj http_object
 * @param status_line a string containing an unparsed HTTP status line
 * 
 * @return 0: sucess
 * @return otherwise: HTTP Error code
*/
int parse_status_line(http_object *obj, char *status_line) {
    int rl_len = strlen(status_line);
    int remaining_bytes = rl_len;
    char *ptr = NULL;
    // parse method name
    ptr = split(status_line, " ", remaining_bytes);
    if (ptr == NULL) {
        // 400 Error - Missing URI field
        return 400;
    }
    if (strcmp(status_line, "PUT") == 0) {
        obj->method = PUT;
    } else if (strcmp(status_line, "GET") == 0) {
        obj->method = GET;
    } else if (strcmp(status_line, "HEAD") == 0) {
        obj->method = HEAD;
    } else {
        // 501 Error - Method not implemented
        return 501;
    }
    // parse URI (simply record the path as a string, no file validation at this point)
    remaining_bytes = rl_len - (ptr - status_line);
    ptr = split(ptr, " ", remaining_bytes);
    if (ptr == NULL) {
        // 400 Error - Missing Version field
        return 400;
    }
    char *uri = status_line + (rl_len - remaining_bytes);
    // URI must start with a / character
    if (strlen(uri) <= 1 || *uri != '/') {
        // 400 Error - URI must start with / symbol
        return 400;
    }
    uri += 1;
    int uri_len = strlen(uri);
    // Check that no slashes are present in URI (after the initial /)
    // forward slash '/'
    char *temp = uri;
    temp = split(temp, "/", uri_len);
    if (temp != NULL) {
        // 400 Error - URI file path cannot contain slashes '/'
        return 400;
    }
    temp = uri;
    // backward slash '\'
    temp = split(temp, "\\", uri_len);
    if (temp != NULL) {
        // 400 Error - URI file path cannot contain slashes '\'
        return 400;
    }
    obj->uri = (char *) malloc((uri_len + 1) * sizeof(char));
    memcpy(obj->uri, uri, uri_len + 1);
    // parse HTTP protocol
    // the remaining substring in ptr should exactly match HTTP/1.1
    if (strcmp(ptr, HTTP_PROTOCOL) != 0) {
        // 400 Error - Invalid HTTP protocol
        return 400;
    }
    return 0;
}

/** Parse HTTP Header Fields
 * @param obj http_object
 * @param header_field a string containing a single unparsed HTTP header field (k: v)
 * 
 * @return val == 1: More Header fields are expected to be processed
 * @return val == 0: The LAST Header field has been processed (no more expected)
 * @return val < 0: Returns a negative error code for an HTTP error that occured (actual error number == val * (-1))
*/

int parse_header_field(http_object *obj, char *header_field) {
    unsigned long len = strlen(header_field);
    // Check if Header Field parsing finished
    if (len == 0) {
        return 0;
    }
    // Every valid header field must contain a ": " substring, separating key from value
    char *val = split(header_field, ": ", len);
    if (val == NULL) {
        // 400 Error - Ill formatted Header field
        return -400;
    }
    // Match a respective header field
    // Content-Length
    if (strcmp(header_field, "Content-Length") == 0) {
        // Parse integer value
        obj->content_length = strtoul(val, NULL, 10);
        if (errno) {
            // 400 Error - Invalid Header value
            return -400;
        }
    }
    // RequestID
    else if (strcmp(header_field, "RequestID") == 0) {
        // Parse integer value
        obj->request_id = strtoul(val, NULL, 10);
        if (errno) {
            // 400 Error - Invalid Header value
            return -400;
        }
    }
    return 1;
}

/** Process HTTP request
 * Used directly on the server side to parse clients' requests
 * @param obj http_object
 * @return 0: Success
 * @return otherwise: HTTP Error code
*/
int process_request(http_object *obj) {
    char buff[MAX_HEADER_LEN];
    char *buff_end = buff + MAX_HEADER_LEN - 1;
    int bytes = 0, bytes_unscanned = 0, fd = obj->fd;
    char *request_line = NULL, *header_field = NULL, *ptr = NULL, *message_body = NULL;
    bool read_incomplete = true;

    // Request Line
    // Extract
    bytes = process_segment(buff, &header_field, fd, bytes, DELIM, &read_incomplete);
    if (bytes < 0) {
        // Error occured while parsing request
        return -1 * bytes;
    }

    // Check that the buffer hasn't been filled by this point
    if (header_field - buff_end > 0) {
        // 400 Error - Request length exceeded (buffer limit exceeded)
        return 400;
    }
    request_line = buff;
    bytes_unscanned = (buff + bytes) - header_field;
    // Parse
    int status_line_errno = parse_status_line(obj, request_line);
    if (status_line_errno != 0) {
        return status_line_errno;
    }
    // Header Field
    // Extract and Parse
    // The Header Field ends with a delimiter \r\n.
    // We will continously chop off each header (delimited by \r\n), until we chop off a header with length 0
    int more_headers = 1;
    do {
        // Flush out unscanned read bytes first
        if (bytes_unscanned > 0) {
            ptr = split(header_field, DELIM, bytes_unscanned);
            bytes_unscanned -= (ptr - header_field);
        }
        // Keep reading the data
        else {
            if (!read_incomplete) {
                // 400 Error - all bytes were read
                return 400;
            }
            bytes = process_segment(buff, &ptr, fd, bytes, DELIM, &read_incomplete);
            if (bytes < 0) {
                // Error occured while parsing request header
                return -1 * bytes;
            }
            bytes_unscanned = (buff + bytes) - ptr;
        }
        // Parse the next header pair
        if (ptr != NULL) {
            more_headers = parse_header_field(obj, header_field);
            header_field = ptr;
            ptr = NULL;
        } else {
            bytes_unscanned = 0;
        }
    } while (more_headers > 0);
    // Check if error occured while parsing headers
    if (more_headers < 0) {
        // Error occured while parsing request header
        return -1 * more_headers;
    }
    // Message Body
    // Extract
    // Unscanned/read message body bytes
    bytes = (buff + bytes) - header_field;
    int content_length = obj->content_length;
    content_length = (content_length == -1) ? 0 : content_length;
    // Check if more bytes than necessary have been read
    if (bytes > content_length) {
        // 400 Error - Content Length did not match Message Body length
        return 400;
    }
    message_body = (char *) malloc(content_length * sizeof(char));
    // Copy message body bytes that have already been read
    memcpy(message_body, header_field, bytes);
    // read the remaining bytes
    int remaining_bytes = bytes_unscanned = content_length - bytes;
    if (!read_incomplete) {
        return 0;
    }
    while (remaining_bytes > 0 && bytes_unscanned > 0) {
        bytes_unscanned = read(fd, message_body + bytes, remaining_bytes);
        remaining_bytes -= bytes_unscanned;
        bytes += bytes_unscanned;
    }
    // Message Body size Policy: Content-Length value and number of bytes in message body MUST agree, otherwise return Error 400.
    if (bytes != content_length) {
        free(message_body);
        // 400 Error - Content Length did not match Message Body length
        return 400;
    }
    // Update http object values
    obj->message_body = message_body;

    return 0;
}
