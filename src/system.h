#ifndef SYSTEM_H
#define SYSTEM_H

#define LCDK 0			// macro that should be set to 1 when running onthe LCDK

#include <stdlib.h>

void* MemAllocate(size_t size);

void FreeMemory(void* ptr);


#endif
