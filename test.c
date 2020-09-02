#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


int main(int argc, char* argv)
{
    uint32_t a = 3;
    uint64_t b = (uint64_t) a;

    printf("%u %llu\n", a, (b << 32) | 1);

    return 0;
}