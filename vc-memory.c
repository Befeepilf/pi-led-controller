#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "mailbox/mailbox.h"
#include "mailbox/util.h"
#include "util.h"
#include "vc-memory.h"

#define UNCACHED_BUS_TO_PHYSICAL(bus) ((bus) & ~0xc0000000)


struct VCMemory alloc_vc_uncached(size_t size, uint32_t alignment)
{
    struct VCMemory mem;

    mem.size = align_to_block(size, alignment);
    mem.mb_handle = alloc_vc_mem(mem.size, alignment, MB_MEM_FLAG_DIRECT);
    mem.bus_addr = lock_vc_mem(mem.mb_handle);
    mem.virtual_addr = map_mem(UNCACHED_BUS_TO_PHYSICAL(mem.bus_addr), mem.size);

    return mem;
}

void free_vc_uncached(struct VCMemory* mem)
{
    unmap_mem(mem->virtual_addr, mem->size);
    free_vc_mem(mem->mb_handle);
}