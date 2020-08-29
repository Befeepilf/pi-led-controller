#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#define BCM2708_PERI_BASE 0x20000000
#define GPIO_BASE (BCM2708_PERI_BASE + 0x200000)
#define DMA_BASE (BCM2708_PERI_BASE + 0x007000)
#define DMA_CHANNEL_OFFSET 0x100

#define BLOCK_SIZE (4 * 1024)

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

#define PIN_RED 18
#define PIN_GREEN 23
#define PIN_BLUE 24


void setupGPIO();
void setipDMA();

int main(int argc, char* argv)
{
    printf("Setting up GPIOs...\n");

    setupGPIO();

    INP_GPIO(PIN_RED);
    OUT_GPIO(PIN_RED);

    GPIO_SET = 1 << PIN_RED;
    GPIO_CLR = 1 << PIN_GREEN;
    GPIO_CLR = 1 << PIN_BLUE;
    while (1) {

    }

    return 0;
}

void setupGPIO()
{
    int mem_fd;
    if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC)) < 0)
    {
        printf("Cannot open /dev/mem: %s\n", strerror(errno));
        exit(-1);
    }

    void *gpio_map = mmap(
        NULL,
        BLOCK_SIZE,
        PROT_READ|PROT_WRITE,
        MAP_SHARED,
        mem_fd,
        GPIO_BASE
    );

    close(mem_fd);

    if (gpio_map == MAP_FAILED)
    {
        printf("mmap error %d\n", (int) gpio_map);
        exit(-1);
    }

    gpio = (volatile unsigned*) gpio_map;
}

void setupDMA()
{
    struct ControlBlock {
        uint32_t transferInfo;
        uint32_t sourceAddr;
        uint32_t destinationAddr;
        uint32_t transferLen;
        uint32_t 2dModeStride;
        uint32_t nextCBAddr;
        uint32_t padding; // reserved; set to 0
    }
}