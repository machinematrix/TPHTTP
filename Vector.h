#ifndef __VECTOR__
#define __VECTOR__
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
	#endif

	typedef struct Vector *VectorHandle;

	VectorHandle Vector_Create(size_t elemSize);
	void Vector_Destroy(VectorHandle vector);
	size_t Vector_Size(VectorHandle vector);
	size_t Vector_Capacity(VectorHandle vector);
	void *Vector_Insert(VectorHandle vector, size_t position, const void *val);
	void *Vector_InsertRange(VectorHandle vector, size_t position, const void *first, const void *last);
	void Vector_PushBack(VectorHandle vector, void *elem);
	void Vector_PopBack(VectorHandle vector);
	void Vector_Erase(VectorHandle vector, size_t elemIndex);
	void Vector_Clear(VectorHandle vector);
	void Vector_Reserve(VectorHandle vector, size_t n);
	void *Vector_At(VectorHandle vector, size_t i);

	int Vector_AllocFailed(VectorHandle vector);
	void Vector_ClearErrorFlag(VectorHandle vector);

	#ifdef __cplusplus
}
#endif

#endif