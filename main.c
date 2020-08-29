#include <stdio.h>

#define BCM2708_PERI_BASE 0x20000000
#define GPIO_BASE (BCM2708_PERI_BASE + 0x200000)

// prevent the compiler from doing anything unwanted through optimization
// prevent this variable from being cached
volatile unsigned* gpio;

#define INP_GPIO(g) *(gpio + ((g) / 10)) &= ~(7 << (((g) % 10) * 3))
#define OUT_GPIO(g) *(gpio + ((g) / 10)) |= (1 << (((g) % 10) * 3))
#define SET_GPIO_ALT(g, a) *(gpio + ((g) / 10)) |= (((a) <= 3 ? (a) + 4 : (a) == 4 ? 3 : 2) << (((g) % 10) * 3))
#define GET_GPIO(g) (*(gpio +  13) & (1 << g))

#define GPIO_SET *(gpio + 7)
#define GPIO_CLR *(gpio + 10)
#define GPIO_PULL *(gpio + 37)
#define GPIO_PULLCLK0 *(gpio + 38)


int main(int argc, char* argv)
{
    printf("Setting up GPIOs...\n");
}