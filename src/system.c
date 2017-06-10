#include "system.h"

#if LCDK == 1
#include "m_mem.h"
#endif

void* MemAllocate(size_t size) {
#if LCDK == 0 
	return malloc(size);
#else
	return m_malloc(size);
#endif
}

void FreeMemory(void* ptr) {
#if LCDK == 0
	free(ptr);
#else
	m_free(ptr);
#endif
}

