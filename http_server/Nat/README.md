Design Doc notes:

~~~Multi-Threading~~~
USE THREAD-POOL DESIGN:
- A thread pool has two types of threads: 
    - worker thread, which actually process requests                                            (#threads worker threads)
    - dispatcher thread, which listens for connections and dispatches them to the workers       (up to 3 dispatcher threads/background work)
- use the threadsafe queue you created in Assignment 2 to connect workers and dispatchers

Requirements:
1. Your server must create exactly ~threads~ worker threads
2. can have up to 3 dispatcher threads


Design
Dispatcher:
1 thread, accepts connection to socket and offloads fd to worker thread by enqueuing the file descriptor in the worker thread's queue
Each worker thread has a queue for requests to handle: they dequeu the next fd and 

// shared memory
queue q;

Dispatcher:
while(1):
    fd = accept()
    q.enqueue(fd)

Worker threads:
while fd = q.dequeue():
    request = fd.parse()

    lock;
    fufill(request)
    unlock;