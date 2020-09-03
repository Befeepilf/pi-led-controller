#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "util.h"

#define PAGE_SIZE sysconf(_SC_PAGESIZE)


void* map_mem(uint32_t base, size_t size)
{
    int mem_fd;
    if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC)) < 0)
    {
        printf("Cannot open /dev/mem: %s\n", strerror(errno));
        exit(-1);
    }

    uint32_t offset = base % PAGE_SIZE;
    base -= offset;
    size += offset;

    void* map = mmap(
        NULL,
        size,
        PROT_READ|PROT_WRITE,
        MAP_SHARED,
        mem_fd,
        base
    );

    close(mem_fd);

    if (map == MAP_FAILED)
    {
        printf("mmap error %d: %s\n", (int) map, strerror(errno));
        exit(-1);
    }

    return map + offset;
}

void unmap_mem(void* addr, size_t size)
{
    uint32_t offset = ((uint32_t) addr) % PAGE_SIZE;
    addr -= offset;
    size += offset;

    if (munmap(addr, size) < 0)
    {
        printf("mmap error: %s\n", strerror(errno));
        exit(-1);
    }
}