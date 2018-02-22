#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdatomic.h>

#include "rthreadpool.h"

struct rthreadwork {
    void (*fun)(void*);
    void *arg;
};

struct rthreadqueue {
    rthreadwork_t work[QUEUE_SIZE];
    int start;
    int count;

    pthread_cond_t push_notif;
    pthread_cond_t pop_notif;
};

struct rthreadpool {
    int threads_count;
    pthread_t *threads;
    rthreadqueue_t queue;

    pthread_mutex_t mtx;

    int running;
    int running_threads;
    int active_threads;

    pthread_cond_t exited_notif;
    pthread_cond_t alldone_notif;
};

static inline void *thread_loop(void *arg);
static inline void rthreadqueue_push_unsafe(rthreadqueue_t *queue,
                                            rthreadwork_t *work);
static inline void rthreadqueue_pop_unsafe(rthreadwork_t *work,
                                           rthreadqueue_t *queue);

rthreadpool_t *rthreadpool_init(int threads_count) {

    rthreadpool_t *pool = (rthreadpool_t*) malloc(sizeof(rthreadpool_t));
    if (pool == NULL) {
        goto err0;
    }

    rthreadqueue_t *queue = &pool->queue;

    queue->start = 0;
    queue->count = 0;

    pool->running = 1;
    pool->threads_count = threads_count;

    pthread_cond_init(&queue->push_notif, NULL);
    pthread_cond_init(&queue->pop_notif, NULL);

    pool->running_threads = 0;
    pool->active_threads = threads_count;

    pthread_mutex_init(&pool->mtx, NULL);
    pthread_cond_init(&pool->exited_notif, NULL);
    pthread_cond_init(&pool->alldone_notif, NULL);

    pool->threads = (pthread_t *) malloc(sizeof(pthread_t) * threads_count);
    if (pool->threads == NULL) {
        goto err1;
    }

    for (int i = 0; i < threads_count; i++) {
        // TODO(RL) Handle pthread error
        pthread_create(&pool->threads[i], NULL, thread_loop, (void*) pool);
    }

    return pool;

err1:
    free(pool);
err0:
    return NULL;
}

void rthreadpool_term(rthreadpool_t *pool) {

    if (pool == NULL) {
        return;
    }

    pthread_mutex_lock(&pool->mtx);

    pool->running = 0;

    rthreadqueue_t *queue = &pool->queue;

    pthread_cond_broadcast(&queue->pop_notif);

    if (pool->active_threads > 0) {
        pthread_cond_wait(&pool->exited_notif, &pool->mtx);
    }

    pthread_mutex_unlock(&pool->mtx);

    for (int i = 0; i < pool->threads_count; i++) {
        pthread_join(pool->threads[i], NULL);
    }

    pthread_cond_destroy(&queue->push_notif);
    pthread_cond_destroy(&queue->pop_notif);

    pthread_mutex_destroy(&pool->mtx);
    pthread_cond_destroy(&pool->exited_notif);
    pthread_cond_destroy(&pool->alldone_notif);

    free(pool->threads);

    free(pool);

}

static inline void rthreadqueue_push_unsafe(rthreadqueue_t *queue,
                                            rthreadwork_t *work) {

    int id = (queue->start + queue->count) % QUEUE_SIZE;

    queue->work[id] = *work;

    queue->count++;
}

static inline void rthreadqueue_pop_unsafe(rthreadwork_t *work,
                                           rthreadqueue_t *queue) {

    int id = queue->start % QUEUE_SIZE;

    *work = queue->work[id];

    queue->start = (queue->start + 1) % QUEUE_SIZE;
    queue->count--;
}

int rthreadpool_add_work(rthreadpool_t *pool,
                         void (*function)(void*),void* arg) {

    rthreadwork_t work = {function, arg};

    pthread_mutex_lock(&pool->mtx);

    rthreadqueue_t *queue = &pool->queue;

    while (queue->count == QUEUE_SIZE && pool->running) {
        pthread_cond_wait(&queue->push_notif, &pool->mtx);
    }

    if (!pool->running) {
        pthread_mutex_unlock(&pool->mtx);
        return 1;
    }

    rthreadqueue_push_unsafe(queue, &work);

    pthread_cond_signal(&queue->pop_notif);

    pthread_mutex_unlock(&pool->mtx);

    return 0;

}

void rthreadpool_join(rthreadpool_t *pool) {

    pthread_mutex_lock(&pool->mtx);

    rthreadqueue_t *queue = &pool->queue;

    while (pool->running_threads > 0 && queue->count > 0) {

        pthread_cond_wait(&pool->alldone_notif, &pool->mtx);

    }

    pthread_mutex_unlock(&pool->mtx);
}

void *thread_loop(void *arg) {

    rthreadpool_t *pool = (rthreadpool_t *) arg;
    rthreadqueue_t *queue = (rthreadqueue_t *) &pool->queue;

    for (;;) {

        pthread_mutex_lock(&pool->mtx);

        while (queue->count == 0 && pool->running) {
            pthread_cond_wait(&queue->pop_notif, &pool->mtx);
        }

        if (!pool->running) {
            pthread_mutex_unlock(&pool->mtx);
            break;
        }

        pool->running_threads++;

        rthreadwork_t work;
        rthreadqueue_pop_unsafe(&work, queue);

        pthread_cond_signal(&queue->push_notif);

        pthread_mutex_unlock(&pool->mtx);

        // Do the work !
        work.fun(work.arg);

        pthread_mutex_lock(&pool->mtx);

        pool->running_threads--;

        if (pool->running_threads == 0 && queue->count == 0) {
            pthread_cond_broadcast(&pool->alldone_notif);
        }

        pthread_mutex_unlock(&pool->mtx);

    }

    pthread_mutex_lock(&pool->mtx);

    pool->active_threads--;

    if (pool->active_threads == 0) {
        pthread_cond_broadcast(&pool->exited_notif);
    }

    pthread_mutex_unlock(&pool->mtx);

    pthread_exit(NULL);

}
