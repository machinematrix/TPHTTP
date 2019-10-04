#include "Vector.h"
#include <stdlib.h>
#include <string.h>

struct Vector
{
	char *elements;
	char *firstFree;
	char *cap;
	size_t elemSize;
	char allocFailedFlag;
};

//sets allocFailedFlag if it fails to allocate memory
static void reallocate(VectorHandle vector)
{
	if (vector->elements)
	{
		size_t newSize = 2 * (vector->cap - vector->elements);
		char *newElements = realloc(vector->elements, newSize);

		if (newElements)
		{
			vector->firstFree = newElements + (vector->firstFree - vector->elements);
			vector->elements = newElements;
			vector->cap = vector->elements + newSize;
		}
		else 
			vector->allocFailedFlag = 1;
	}
	else
	{
		vector->firstFree = vector->elements = malloc(vector->elemSize * 2);
		if (vector->elements)
			vector->cap = vector->elements + 2 * vector->elemSize;
		else
			vector->allocFailedFlag = 1;
	}
}

//returns non-zero if it succeeds
static int checkAndAlloc(VectorHandle vector)
{
	if (Vector_Size(vector) == Vector_Capacity(vector)) {
		reallocate(vector);
		return !vector->allocFailedFlag;
	}
	else return 1;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

VectorHandle Vector_Create(size_t elemSize)
{
	VectorHandle result = NULL;
	if (elemSize)
	{
		result = calloc(1, sizeof(struct Vector));

		if (result) {
			result->elemSize = elemSize;
			result->allocFailedFlag = 0;
		}

		return result;
	}
	return result;
}

void Vector_Destroy(VectorHandle vector)
{
	if (vector) {
		free(vector->elements);
		free(vector);
	}
}

size_t Vector_Size(VectorHandle vector)
{
	return (vector->firstFree - vector->elements) / vector->elemSize;
}

size_t Vector_Capacity(VectorHandle vector)
{
	return (vector->cap - vector->elements) / vector->elemSize;
}

void* Vector_Insert(VectorHandle vector, size_t position, const void *val)
{
	size_t i = Vector_Size(vector);
	void *result = NULL;

	if (checkAndAlloc(vector))
	{
		vector->firstFree += vector->elemSize;

		for (; i > position; --i) {
			memcpy(Vector_At(vector, i), Vector_At(vector, i - 1), vector->elemSize);
		}

		result = Vector_At(vector, position);
		memcpy(result, val, vector->elemSize);
	}
	
	return result;
}

void* Vector_InsertRange(VectorHandle vector, size_t position, const void *first, const void *last)
{
	const char *begin = first, *end = last;

	Vector_Reserve(vector, Vector_Size(vector) + (end - begin) / vector->elemSize * vector->elemSize);

	if (!vector->allocFailedFlag)
	{
		while (begin < end)
		{
			Vector_Insert(vector, position++, begin);
			begin += vector->elemSize;
		}

		return Vector_At(vector, position);
	}

	return NULL;
}

void Vector_PushBack(VectorHandle vector, void *elem)
{
	if (checkAndAlloc(vector)) {
		memcpy(vector->firstFree, elem, vector->elemSize);
		vector->firstFree += vector->elemSize;
	}
}

void Vector_PopBack(VectorHandle vector)
{
	vector->firstFree -= vector->elemSize;
}

void Vector_Erase(VectorHandle vector, size_t elemIndex)
{
	size_t i, size = Vector_Size(vector);
	for (i = elemIndex; i < size - 1; ++i) {
		memcpy(Vector_At(vector, i), Vector_At(vector, i + 1), vector->elemSize);
	}
	vector->firstFree -= vector->elemSize;
}

void Vector_Clear(VectorHandle vector)
{
	vector->firstFree = vector->elements;
}

void Vector_Reserve(VectorHandle vector, size_t n)
{
	while (n > Vector_Capacity(vector))
	{
		vector->allocFailedFlag = 0;
		reallocate(vector);
		if (vector->allocFailedFlag)
			return;
	}
}

void* Vector_At(VectorHandle vector, size_t i)
{
	void *result = NULL;

	if (i < Vector_Size(vector))
	{
		result = vector->elements + i * vector->elemSize;
	}

	return result;
}

int Vector_AllocFailed(VectorHandle vector)
{
	return vector->allocFailedFlag;
}

void Vector_ClearErrorFlag(VectorHandle vector)
{
	vector->allocFailedFlag = 0;
}