#pragma once

#define QUEUE_SIZE 32

typedef struct rthreadpool rthreadpool_t;
typedef struct rthreadwork rthreadwork_t;
typedef struct rthreadqueue rthreadqueue_t;

rthreadpool_t *rthreadpool_init(int threads_count);

void rthreadpool_term(rthreadpool_t *pool);

int rthreadpool_add_work(rthreadpool_t *pool, void (*function)(void*), void* arg);

void rthreadpool_join(rthreadpool_t *pool);

void *thread_loop(void *arg);
