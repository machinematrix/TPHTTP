#include "Thread.h"

pthread_t createThread(void*(*fn)(void*), int detachState, void *arg)
{
	pthread_t handle = 0;
	pthread_attr_t attribute = { 0 };
	if(!pthread_attr_init(&attribute) && !pthread_attr_setdetachstate(&attribute, detachState)) {
		pthread_create(&handle, &attribute, fn, arg);
	}
	return handle;
}