Assignment 2 - CSE130 UCSC Fall 2022
Natalie Valett, nvalett

Assignment: make a thread-safe bounded buffer API
    What does this mean?
    *Thread Safe*: a function is thread-safe only if it will ALWAYS produce correct results when called repeatedly from multiple concurrent threads
        Dealing with shared variables - in this case our queue will be shared
        We should allow multiple threads to write to and read from the queue simultaneously, while ensuring:
            1) *Thread Order*: enforce FIFO order is respected, even though different consumers might dequeue different items.
            2) *Validity*: Not reading from non-existant or bad memory 
            3) *Completeness*: Not losing data

So... how do we do that?

To generalize, we want to manage shared state in a way that only one thread can be in the "critical region" at once
Examples:
    - Basically, before *writing*, we must wait until no other thread is *reading or writing*
        - If one other reader is in the CR at any given time, we don't want our writer to come in
        - If more than one writer, we dont want 2 writers at the same time
So when a reader comes in, we want a flag that won't allow writers
    - the first one to enter "closes" the door for writers
    - last one to leave "opens" the door for writers


Approach:
Start by making a single threaded queue, 
then think about when thread hazards occur, 
and locked down sections or changed up my code a bit to prevent these from happening

Design:
1. What to do when writing to full queue?
    while (q.size >= q.capacity) {} // wait until size < capacity
2. What to do when reading from empty queue?
    while (q.size <= 0) {} // wait until size > 0



Psuedocode:

pthread_mutex(&do_work)
pthread_cond(&not_full)     // signals when buffer's no longer full
pthread_cond(&not_empty)    // signals when buffer's no longer empty

queue_push(q, elem) {
    if q or elem invalid pointers:
        return false
    // wait until no one is reading or writing!
    lock(&do_work) 
    // our turn to write, first check if q is full...
    while (q.size == q.capacity) {
        // unlock our mutex for other threads to POP until q is not full
        pthread_cond_wait(&not_full, &do_work) 
    }
    q.num_entries++
    q[head] = elem
    q.head++
    pthread_cond_signal(&not_empty)
    unlock(&do_work)
    return true
}

queue_pop(q, elem) {
    if q or elem invalid pointers:
        return false
    
    lock(&do_work)

    while(q.size == 0) {
        // unlock our mutex for other threads to POP until q is not full
        pthread_cond_wait(&not_empty, &do_work) 
    }
    q.num_entries--
    elem = q[tail]
    q.tail--
    pthread_cond_signal(&not_full)
    pthread_mutex_unlock(&do_work)
    return true
}
