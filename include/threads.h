#ifndef THREADS_H
#define THREADS_H

#include <pthread.h>

#define THR_NUM			3

extern pthread_t thr_ids[THR_NUM];
extern pthread_cond_t cond_ids[THR_NUM - 1];
extern pthread_mutex_t lock;
extern pthread_mutex_t condlock;

#endif /* THREADS_H */
