#ifndef SYSTEM_H
#define SYSTEM_H

#define LCDK  0			// macro that should be set to 1 when running onthe LCDK

#if LCDK == 1
#include "m_mem.h
#include "bmp.h"
#endif

#include <stdlib.h>

void* MemAllocate(size_t size);

void FreeMemory(void* ptr);

#endif
