#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>
#include <stdint.h>

void* map_mem(uint32_t base, size_t size);
void unmap_mem(void* addr, size_t size);

#endif