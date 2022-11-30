#include "bind.h"
#include "queue.h"
#include "httperrors.h"
#include "httprequest.h"
#include "operators.h"
#include <sys/socket.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h> 
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/syscall.h>
#include <err.h>
#include <errno.h>
#define OPTIONS "t:l:"

volatile sig_atomic_t done = 0;
int queue_size = 0;
pthread_mutex_t workLock;
bool log_enabled = false;
int log_fd = 0;

void term() {
    done = 1;
}

/*
 * Dispatcher thread function
 * args:
 * args[0] = queue with client file descriptors
 * args[1] = socket filedescriptor
 */
void *dispatch(void *args) {
    queue_t *q = ((void **)args)[0];
    int socketfd = (int)((void **)args)[1];
    int in;

    while (!done) {
        in = accept(socketfd, NULL, NULL);
        if (in == -1 && !done) {
            warnx("Connection to socket failed");
            break;
        }
        if (done) {
            break;
        }
        queue_push(q, (void *)(intptr_t)in);
        queue_size++;
    }
    return NULL;
}

/*
 * worker thread function
 */
void *worker(void *args) {
    queue_t *q = ((void **)args)[0];
    void *rv;
    int in;
    char logbuffer[4096];

    while (!done) {
        // will wait till there's something to dequeue...
        queue_pop(q, &rv);
        in = (int)rv;

        httprequest *httpobj = httprequest_init();
        int messageBodyLength = construct_request(httpobj, in);
        // if error encountered
        if (messageBodyLength < 0) {
            pthread_mutex_lock(&workLock);
            if (log_enabled) {
                log_request(httpobj, logbuffer, log_fd);
            }
            pthread_mutex_unlock(&workLock);
            continue;
        }

        // mutex to only let one thread handle request and log at once
        pthread_mutex_lock(&workLock);

        handle_request(httpobj, in);
        if (log_enabled) {
            log_request(httpobj, logbuffer, log_fd);
        }

        pthread_mutex_unlock(&workLock);

        close(in);
        httprequest_delete(&httpobj);
    }

    // SIGTERM has been called
    if (log_enabled) {
        write_logs(logbuffer, log_fd);
        close(log_fd);
    }
    return NULL;
}

int main(int argc, char **argv) {
    uint16_t port;
    int opt = 0;
    int socketfd;
    int threads = 1;
    int reqsPerThread = 10;
    void *rc;

    pthread_mutex_init(&workLock, NULL);

    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = term;
    sigaction(SIGTERM, &action, NULL);

    if (argc < 2) {
        warnx("not enough arguments\nusage: %s [-t threads] [-l logfile] <port>", argv[0]);
        return 1;
    }

    while ((opt = getopt(argc, argv, OPTIONS)) != -1) {
        // threads given
        if (opt == 't') {
            threads = atoi(optarg);
        }
        // enable log file
        if (opt == 'l') {
            log_enabled = true;
            if (strlen(optarg) > 20) {
                warnx("Log filename %s is too long\n", optarg);
                return 1;
            }
            log_fd = open(optarg, O_WRONLY|O_TRUNC);
            if (log_fd < 0) {
                // file not accessable
                if (errno == EACCES || errno == EPERM || errno == EISDIR) {
                    warnx("No access to log file %s\n", optarg);
                    return 1;
                }
                else if (errno == ENOENT) {
                    log_fd = open(optarg, O_CREAT | O_WRONLY, 0666);
                }
                // file doesn't exist
                else {
                    warnx("log file %s does not exist\n", optarg);
                    return 1;
                }
            }
        }
        if (opt != 't' && opt != 'l') {
            warnx("Incorrect usage of %s\nusage: [-t threads] [-l logfile] <port>", argv[0]);
            return 1;
        }
    }

    // convert port from int to uint16_t
    port = strtoul(argv[argc-1], NULL, 10);    
    socketfd = create_listen_socket(port);
    // handle errors in connecting
    if (socketfd < 0) {
        warnx("Cannot bind to socket\n");
        exit(1);
    }

    pthread_t wt[threads];  // worker threads
    pthread_t dt;           // dispatcher thread
    queue_t *q = queue_new(threads * reqsPerThread);
    void *dispatchArgs[2] = {q, (void *)(intptr_t)socketfd};
    void *workerArgs[1] = {q};

    pthread_create(&dt, NULL, dispatch, dispatchArgs);
    for (int i = 0; i < threads; i++) {
        pthread_create(&(wt[i]), NULL, worker, workerArgs);
    }

    pthread_join(dt, &rc);
    if (rc != NULL)
        return 1;
    for (int i = 0; i < threads; i++) {
        pthread_join(wt[i], &rc);
        if (rc != NULL)
        return 1;
    }

    // while (!done) {
    //     in = accept(socketfd, NULL, NULL);
    //     if (in == -1 && !done) {
    //         warnx("Connection to socket failed");
    //         break;
    //     }
    //     if (done) {
    //         break;
    //     }

    //     httprequest *httpobj = httprequest_init();
    //     int messageBodyLength = construct_request(httpobj, in);
    //     if (messageBodyLength < 0) {
    //         if (log_enabled) {
    //             log_request(httpobj, logbuffer, log_fd);
    //         }
    //         continue;
    //     }

    //     // mutex to only let one thread handle request and log at once
    //     pthread_mutex_lock(&process_request);

    //     handle_request(httpobj, in);
    //     if (log_enabled) {
    //         log_request(httpobj, logbuffer, log_fd);
    //     }

    //     pthread_mutex_unlock(&process_request);

    //     close(in);
    //     httprequest_delete(&httpobj);
    // }
    // after SIGTERM
    shutdown(socketfd, SHUT_RDWR);
    pthread_mutex_destroy(&workLock);
}
