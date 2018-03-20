Eileen Feng
HW4_ThreadLib

1. How to use this library
    - 'thread_create(int policy)' allows initializing a thread library with the corresponding policy 'policy'
    - use 'thread_yield()' to yield or 'thread_join(int tid)' to join a specific thread
    - 'thread_terminate()' to terminate the library


2. Limitations
   -  when 'thread_join' is called when currently no threads is running, 'thread_join' will make the main thread join the first thread in the ready queue; this means no matter what 'tid' is passed into 'thread_join' during the very first call to 'thread_join', it is always the head thread of the queue be scheduled.
   - If a thread is suspended in the middle of a system call in priority scheduling, switch back to this thread later cannot resume the system call; for instance, if a thread A call 'poll(NULL, 0, 300)', thread A will be interrupted once and when A is rescheduled, it cannot go back to the middle of executing 'poll(NULL, 0, 300)', where it got stopped last time.