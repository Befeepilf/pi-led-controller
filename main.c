#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#define BCM2708_PERI_ARM_BASE   0x20000000
#define BCM2708_PERI_VC_BASE    0x7E000000

#define GPIO_BASE_OFFSET    0x200000
#define GPIO_ARM_BASE       (BCM2708_PERI_ARM_BASE + GPIO_BASE_OFFSET)
#define GPIO_VC_BASE        (BCM2708_PERI_VC_BASE + GPIO_BASE_OFFSET)
#define GPIO_SET_OFFSET     0x1C
#define GPIO_CLR_OFFSET     0x28

#define DMA_BASE_OFFSET     0x007000
#define DMA_ARM_BASE        (BCM2708_PERI_ARM_BASE + DMA_BASE_OFFSET)
#define DMA_CHANNEL         5
#define DMA_CHANNEL_OFFSET  0x100

#define DMA_CB_TI_DISABLE_WIDE_BURSTS   (1 << 26)
#define DMA_CB_TI_ENABLE_SRC_INC        (1 << 8)
#define DMA_CB_TI_ENABLE_DEST_INC       (1 << 4)
#define DMA_CB_TI_ENABLE_2DMODE         (1 << 1)

#define DMA_CB_TXFR_YLEN(yLen)  (((yLen) & 0x3fff) << 16) // & 0x3fff to constrain to 14 bits
#define DMA_CB_TXFR_XLEN(xLen)  ((xLen) & 0xffff) // & 0xffff to constrain to 16 bits

#define DMA_CB_STRIDE_SRC(srcStride)    ((srcStride) & 0xffff) // & 0xffff to constrain to 16 bits
#define DMA_CB_STRIDE_DEST(destStride)  (((destStride) & 0xffff) << 16) // & 0xffff to constrain to 16 bits

#define DMA_CS_RESET    (1 << 31)
#define DMA_CS_ABORT    (1 << 30)
#define DMA_CS_END      (1 << 1)
#define DMA_CS_ACTIVE   (1 << 0)

#define BLOCK_SIZE (4 * 1024)

/* volatile:
    prevent the compiler from doing anything unwanted through optimization
    prevent this variable from being cached */
volatile unsigned* gpio;
volatile unsigned* dma;

// #define INP_GPIO(g) *(gpio + ((g) / 10)) &= ~(7 << (((g) % 10) * 3))
// #define OUT_GPIO(g) *(gpio + ((g) / 10)) |= (1 << (((g) % 10) * 3))
// #define SET_GPIO_ALT(g, a) *(gpio + ((g) / 10)) |= (((a) <= 3 ? (a) + 4 : (a) == 4 ? 3 : 2) << (((g) % 10) * 3))
// #define GET_GPIO(g) (*(gpio + 13) & (1 << g))

// #define GPIO_SET *(gpio + 7)
// #define GPIO_CLR *(gpio + 10)
// #define GPIO_PULL *(gpio + 37)
// #define GPIO_PULLCLK0 *(gpio + 38)

#define PIN_RED 18
#define PIN_GREEN 23
#define PIN_BLUE 24


void mmapPeripherals();
void setupDMA();

int main(int argc, char* argv)
{
    mmapPeripherals();

    INP_GPIO(PIN_RED);
    OUT_GPIO(PIN_RED);

    GPIO_SET = 1 << PIN_RED;
    GPIO_CLR = 1 << PIN_GREEN;
    GPIO_CLR = 1 << PIN_BLUE;
    while (1) {

    }

    return 0;
}

void mmapPeripherals()
{
    int mem_fd;
    if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC)) < 0)
    {
        printf("Cannot open /dev/mem: %s\n", strerror(errno));
        exit(-1);
    }

    void* gpio_map = mmap(
        NULL,
        BLOCK_SIZE,
        PROT_READ|PROT_WRITE,
        MAP_SHARED,
        mem_fd,
        GPIO_ARM_BASE
    );

    if (gpio_map == MAP_FAILED)
    {
        printf("mmap error %d\n", (int) gpio_map);
        exit(-1);
    }


    void* dma_map = mmap(
        NULL,
        BLOCK_SIZE,
        PROT_READ|PROT_WRITE,
        MAP_SHARED,
        mem_fd,
        DMA_ARM_BASE
    );

    if (gpio_map == MAP_FAILED)
    {
        printf("mmap error %d\n", (int) dma_map);
        exit(-1);
    }

    close(mem_fd);

    gpio = (volatile unsigned*) gpio_map;
    dma = (volatile unsigned*) dma_map;
}

void setupDMA()
{
    struct ControlBlock {
        uint32_t transferInfo;
        uint32_t srcAddr;
        // address in VC (not ARM)
        uint32_t destAddr;

        /*
            transferLen bits:
                31-30: reserverd, zeros
                2D Mode:
                    29-16: Ylength (how many times XLength)
                     15-0: XLength in bytes
                Normal mode:
                    29:0: transfer length in bytes
        */
        uint32_t transferLen;

        /*
            2dModeStride bits:
                31-16: destination stride
                    signed 2's complement byte increment applied to destAddr at
                    the end of each row in 2D mode
                 15:0: source stride
                    signed 2's complement byte increment applied to srcAddr at
                    the end of each row in 2D mode
        */
        uint32_t 2dModeStride;

        // must be 256 bit aligned -> bottom 5 bits cannot be set
        // address in VC (not ARM)
        uint32_t nextCBAddr;
        uint32_t padding[2]; // reserved; set to 0
    }

    struct DmaChannel {
        /*
            controlAndStatus bits:
                31: RESET
                30: ABORT
                    1 = abort current control block and load the next block
                 1: END
                 0: ACTIVE
        */
        uint32_t controlAndStatus;

        // must be 256 bit aligned -> bottom 5 bits cannot be set
        // address in VC (not ARM)
        uint32_t CBAddr;

        /*
            transferInfo bits for channels 0-6:
                31-27: reserved, zeros
                26: no wide bursts
                    ? ought to be inefficient
                25-21: WAITS
                    number of dummy cycles after each DMA read or write operation
                15:12: Burst transfer length (in number of words)
                    the DMA will attempt to transfer data as bursts of this number of words
                    0 = single transfer
                9: source transfer width
                    1 = use 128 bit source read width
                    0 = use 32 bit source read width
                8: source address increment
                    1 = source address increments after each read by 4 if source transfer width=0 else by 32
                    0 = source address does not change
                5: destination transfer width
                    1 = use 128 bit destination write width
                    0 = use 32 bit destination write width
                4: destination address increment
                    1 = destination address increments after each read by 4 if destination transfer width=0 else by 32
                    0 = destination address does not change
                1: 2D mode
                0:
        */
        uint32_t transferInfo;

        uint32_t srcAddr;
        uint32_t destAddr;
        uint32_t transferLen;
        uint32_t 2dModeStride;
        // address in VC (not ARM)
        uint32_t nextCBAddr;

        uint32_t debug;
    }

    
    struct GPIOData {
        uint32_t set;
        uint32_t clr;
    }


    struct DmaChannel* dmaChannel = DMA_ARM_BASE + DMA_CHANNEL * DMA_CHANNEL_OFFSET;
    
    GPIOData* gpioData = malloc(sizeof(GPIOData));
    gpioData->set = (1 << PIN_RED);
    gpioData->clr = (1 << PIN_RED);

    struct ControlBlock* cb = malloc(sizeof(struct ControlBlock));
    cb->transferInfo = DMA_CB_TI_DISABLE_WIDE_BURSTS | DMA_CB_TI_ENABLE_2DMODE;
    cb->srcAddr = toVCAddr(gpioData);
    cb->destAddr = GPIO_VC_BASE + GPIO_SET_OFFSET;
    // transfer 4 bytes two times - set and clear respectively (see GPIOData)
    cb->transferLen = DMA_CB_TXFR_YLEN(2) | DMA_CB_TRXFR_XLEN(4);
    // write gpioData.set to GPIO_SET register, then increment destAddr in order to write gpioData.clr to GPIO_CLR register
    cb->2dModeStride = DMA_CB_STRIDE_DEST(GPIO_CLR_OFFSET - GPIO_SET_OFFSET) | DMA_CB_STRIDE_SRC(0);
    cb->nextCBAddr = toVCAddr(cb);

    dmaChannel->controlAndStatus |= DMA_CS_END;
    dmaChannel->CBAddr = toVCAddr(cb);
    dmaChannel->controlAndStatus |= DMA_CS_ACTIVE;
}