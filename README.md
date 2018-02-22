# RThreadPool

A lightweight easy to use implementation of a thread pool in C using
POSIX threads (pthreads)

## Example snippet
```c++
    // Initialize the pool
    rthreadpool_t *pool = rthreadpool_init(NB_THREADS);
    if (!pool) {
        // Handle error
        return;
    }

    for (int i = 0; i < WORK_COUNT; i++) {
        // Add your tasks
        rthreadpool_add_work(pool, myfunc[i], myargs[i]);
    }

    // Wait for all the task to complete
    rthreadpool_join(pool);

    // Exit the pool properly
    rthreadpool_term(pool);
```
