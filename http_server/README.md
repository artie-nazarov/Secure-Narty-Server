# Assignment 1 - HTTP Server
## Description
The following program implements an HTTP server. The server will support HTTP methods: **PUT**, **GET**, **HEAD**
<br>**Note:** the current implementation of HTTP server does not use multithreading.

## Files
* httpserver.c: Driver implementation for the HTTP server. Contains main() and several helper functions.
* http_object.c: Implements an HTTP request parser. A convenient wrapper for client's request metadata.
* http_object.h: A header file for http_object.
* bind.c: Socket creation/connection module.
* bind.h: Header for bind.c
* Makefile: Compiles httpserver
* test_files: contains files for testing scripts
* test_scripts: contains test scripts 

## Getting started
Execute the following on a **UNIX** machine
#### httpserver
**Usage:**
`./httpserver <port_num>`

## Server structure
### Server driver
`httpserver` is the main driver for our HTTP server.
#### Functionality
* Establishes connections with clients
* Creates sockets in the form of a file descriptor for each client
* Invoke request parsing
* Invoke requested HTTP methods
* Produce and send a response to the client

### HTTP Object
`http_object` is a convenient object representation of an HTTP request
#### Functionality
* Parses HTTP requests
* Stores parsed request metadata for server use

## Design Tenets
### ***Parsing as we go***
There are serveral approaches to parsing an request that is continously being read form a socket:
1. Read the entire request line into a buffer, and then parse it using Context-Free-Grammar, or other techniques.
2. Parse the request as we go
    - A read() syscall may not read the entire request line; we may have to read in chunks
    - We can continuosly process chunks of the request to dynamically make decisions whether we need to keep reading or if the request is already malformed.

`htt_object` is an HTTP parser that uses approach 2 - **Parsing as we go**.
<br>Parsing the request in chunks allows us to reason about our request early on, before we read the entire input stream. This approach can help us catch malformed requests early on and prevent unnecessary processing time.

### PUT - Content-Length/Message-Body-Length Dilemas
This section covers several policies for PUT requests concerning missmatches in the values of Content-Length and Message-Body Length
<br>**From the Assignment specs:** `Valid PUT requests must include a message-body`
#### Missing a Body
This policy ensures that every `PUT` request **MUST** have a message body. Our server follows the following rules:
* If the Content-Length header is missing, we have an invalid request `Error 400`
* If the Content-Length == 0 and our message body is an empty string, that is a valid request. An empty string is a valid body as long as it agrees with a content length of 0
#### Content-Length / message body length missmatch
This policy ensures that every `PUT` request **MUST** have a Content-Length and message body length agree with eachother. Our server follows the following rules:
* If Content-Length > message body length -> `Error 400`
* If Content-length < message body length -> `Error 400`

> **_NOTE:_**  The provided reference implementation accepts the case where Content-Length > message body length. However from the statement about valid PUT requests in the specs this shouldn't be the case. In general, it seems like a poor approach to accpet such a server behavior because a greater Content-Length suggests that we may want to read more bytes than we are actually given.
