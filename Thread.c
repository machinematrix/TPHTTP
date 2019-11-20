#include "Thread.h"

#ifdef __linux__
#include <pthread.h>
#include <netdb.h>
#endif

#ifdef _WIN32
#include <Windows.h>
#endif

#include <stdlib.h>
#include <stdio.h>

struct MutexInfo
{
	#ifdef __linux__
	pthread_mutex_t mutexHandle;
	#elif defined(_WIN32)
	HANDLE mutexHandle;
	#endif
};

struct ThreadInfo
{
	#ifdef __linux__
	pthread_t threadHandle;
	#elif defined(_WIN32)
	HANDLE threadHandle;
	#endif
};

#ifdef _WIN32
struct ThreadArgs
{
	void *(*fn)(void *);
	void *arg;
};

DWORD WINAPI ThreadProc(_In_ LPVOID lpParameter)
{
	struct ThreadArgs *args = lpParameter;
	args->fn(args->arg);
	free(args);

	return 1;
}
#endif

Mutex createMutex()
{
	Mutex result = calloc(1, sizeof(struct MutexInfo));

	if (result)
	{
		#ifdef __linux__
		if (pthread_mutex_init(&(result->mutexHandle), NULL)) {
			free(result);
			result = NULL;
		}
		#elif defined(_WIN32)
		result->mutexHandle = CreateMutex(NULL, FALSE, NULL);
		if (!result->mutexHandle) {
			free(result);
			result = NULL;
		}
		#endif
	}

	return result;
}

void destroyMutex(Mutex mutex)
{
	if (mutex)
	{
		#ifdef __linux__
		pthread_mutex_destroy(&(mutex->mutexHandle));
		#elif defined(_WIN32)
		CloseHandle(mutex->mutexHandle);
		#endif

		free(mutex);
	}
}

void lockMutex(Mutex mutex)
{
	#ifdef __linux__
	pthread_mutex_lock(&(mutex->mutexHandle));
	#elif defined(_WIN32)
	WaitForSingleObject(mutex->mutexHandle, INFINITE);
	#endif
}

void unlockMutex(Mutex mutex)
{
	#ifdef __linux__
	pthread_mutex_unlock(&(mutex->mutexHandle));
	#elif defined(_WIN32)
	ReleaseMutex(mutex->mutexHandle);
	#endif
}

void joinThread(Thread thread)
{
	#ifdef __linux__
	pthread_join(thread->threadHandle, 0);
	#elif defined(_WIN32)
	WaitForSingleObject(thread->threadHandle, INFINITE);
	#endif
}

Thread createThread(void*(*fn)(void*), void *arg)
{
	Thread result = calloc(1, sizeof(struct ThreadInfo));

	if (result)
	{
		#ifdef __linux__
		pthread_attr_t attribute = { 0 };
		if (!pthread_attr_init(&attribute) && !pthread_attr_setdetachstate(&attribute, PTHREAD_CREATE_JOINABLE)) {
			pthread_create(&(result->threadHandle), &attribute, fn, arg);
		}
		#elif defined(_WIN32)
		struct ThreadArgs *args = malloc(sizeof(struct ThreadArgs));
		if (args)
		{
			args->fn = fn;
			args->arg = arg;
			result->threadHandle = CreateThread(NULL, 0, ThreadProc, args, 0, NULL);
		}
		else {
			free(result);
			result = NULL;
		}
		#endif
	}

	return result;
}

void destroyThread(Thread thread)
{
	if (thread)
	{
		#ifdef _WIN32
		CloseHandle(thread->threadHandle);
		#endif

		free(thread);
	}
}