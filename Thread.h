#ifndef __THREAD__
#define __THREAD__
#include <pthread.h>
#include <netdb.h>

typedef pthread_mutex_t Mutex;
typedef pthread_t Thread;

#define createMutex(mtx) pthread_mutex_init(&(mtx), NULL)
#define deleteMutex(mtx) pthread_mutex_destroy(&(mtx))
#define lockMutex(mtx) pthread_mutex_lock(&(mtx))
#define unlockMutex(mtx) pthread_mutex_unlock(&(mtx))
#define joinThread(/*pthread_t*/ threadId) pthread_join(threadId, 0)
Thread createThread(void*(*fn)(void*), int detachState, void *arg);

#endif