#ifndef VC_MEM_H
#define VC_MEM_H

#include <stddef.h>
#include <stdint.h>

struct VCMemory {
    size_t size;
    size_t aligned_size;
    uint32_t mb_handle;
    uint32_t bus_addr;
    void* virtual_addr;
};

struct VCMemory alloc_vc_uncached(size_t size, uint32_t alignment);
void free_vc_uncached(struct VCMemory* mem);

#endif