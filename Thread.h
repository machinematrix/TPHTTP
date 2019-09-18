#ifndef __THREAD__
#define __THREAD__
#ifdef __linux__
#include <pthread.h>
#include <netdb.h>
#endif

typedef struct MutexInfo *Mutex;
typedef struct ThreadInfo *Thread;

Mutex createMutex();
void destroyMutex(Mutex mutex);
void lockMutex(Mutex mutex);
void unlockMutex(Mutex mutex);
void joinThread(Thread thread);
Thread createThread(void*(*fn)(void*), void *arg);
void destroyThread(Thread thread);
#endif