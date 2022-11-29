# Assignment 2 - Bounded Buffer
## Description
The following program implements a Multi-Threaded Bounded Buffer. We implement a Queue ADT that supports multi-consumer and multi-producer operations (concurrent push and pop).

## Files
* queue.c: Multi-Threaded Queue implementation.
* queue.h: A header file for queue.c
* Makefile: Compiles queue.c
* test_files: contains files for testing scripts
* test_scripts: contains test scripts

## Getting started
### Compiling queue
`make all`
Produces a queue object file, to be linked to other implementations using queue ADT.

## Implementation Details
### Single-Threaded Queue
We started off with implementing a simple single-threaded bounded buffer, which keeps track of the tail and the head of a queue.
### Multithreading with pthread
We then added support for multiple threads interacting with the queue via `pthreads` C library.
<br>In order to support multiple consumers and producers at the same time, we needed to provide the following functionality
* Bound critical regions (internal queue data mutation) by mutexes.
* Resolve hanging threads - reffers to a thread continously waiting to enter a critical region (acquire a mutex)
* pop() hanging - if a queue is empty and threads attempt to pop more elements, the queue will hang until another threads pushes new elements to the queue.
    - uses condition variables to begin waiting `pthread_cond_wait` until some producer thread pushes a value and notifies `pthread_cond_broadcast` all "hanging" threads that the queue is no longer empty, proceed with lock acquisition.
* push() hanging - if a queue is full and threads attempt to push more elements, the queue will hang until another threads pops an element off of the queue.
    - uses condition variables to begin waiting `pthread_cond_wait` until some consumer thread pops a value and notifies all "hanging" threads that the queue is no longer full, proceed with lock acquisition.

## Testing Multithreaded Bounded Buffer
### Test 1 - Thread Order
Tests the following scenario:
> If a thread T1 enqueues item a and then enqueues b, any thread T2 that dequeues both a and b must dequeue a first. This test will first complete push operations by thread A, and only then invoke pop operations by thread B.

### Test 2 - Validity
Tests the following scenario:
> If a consumer thread T2 dequeues some item a, then a was enqueued by some thread T1. Intuitively, no consumers may read junk.

### Test 3 - Completeness
Tests the following scenario:
> If a thread T1 enqueues item a, some consumer T2 will dequeue a after a finite number of dequeues.

### Test 4 - Thread Order (Multiple Producer threads)
> Thread A pushes negative numbers, concurrently with thread B pushing positive values. Thread C concurrently pops the values as they come in. This test checks FIFO properties.

### Test 5 - Test whether our queue "hangs" when full and a producer attempts to push, until a consumer frees up the queue
> * Step 1: Launch thread A attempting to push onto a FULL queue. Add some delay before launching thread B, to give A time to start hanging.
> * Step 2: Launch another thread B that will free up space on the queue
> * Step 3: End time of thread B should be smaller than end time of A (thread B must finish before A).

### Test 6 - Test whether our queue "hangs" when empty and a consumer attempts to pop, until a producer pushes a new value.
> * Step 1: Launch thread A attempting to pop off of an EMPTY queue. Add some delay before launching thread B, to give A time to start hanging.
> * Step 2: Launch another thread B that will push a single item onto the queue
> * Step 3: End time of thread B should be smaller than end time of A (thread B must finish before A).

### Test 7 - Thread Order (Multiple Consumer threads)
> Thread C pushes positive values. Threads A and B concurrently pop the values as they come in. This test checks FIFO properties.

### Test 8 - Thread Order (Multiple Producer and Consumer threads)
> Thread A pushes negative numbers, concurrently with thread B pushing positive values. Threads C1, C2, C3, C4 concurrently pop the values as they come in. This test checks FIFO properties.

### Test 9 - Thread after thread
Tests the following scenario:
> If thread A pushes A and finishes returning before thread B pushes B, then if thread C pops from the queue and finishes before thread D attempts to pop, then thread C must pop A and thread D will have popped B.
