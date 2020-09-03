#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "mailbox/mailbox.h"
#include "util.h"
#include "vc-memory.h"

#define BCM2708_PERI_PHYSICAL_BASE  0x20000000
#define BCM2708_PERI_BUS_BASE       0x7e000000

#define GPIO_BASE_OFFSET    0x200000
#define GPIO_PHYSICAL_BASE  (BCM2708_PERI_PHYSICAL_BASE + GPIO_BASE_OFFSET)
#define GPIO_BUS_BASE       (BCM2708_PERI_BUS_BASE + GPIO_BASE_OFFSET)
#define GPIO_REGISTER_SIZE  0xb4
#define GPIO_SET_OFFSET     0x1c
#define GPIO_CLR_OFFSET     0x28

#define DMA_BASE_OFFSET     0x007000
#define DMA_PHYSICAL_BASE   (BCM2708_PERI_PHYSICAL_BASE + DMA_BASE_OFFSET)
#define DMA_REGISTER_SIZE   0xff4
#define DMA_CHANNEL         5
#define DMA_CHANNEL_OFFSET  0x100

#define DMA_CB_TI_DISABLE_WIDE_BURSTS   (1 << 26)
#define DMA_CB_TI_ENABLE_SRC_INC        (1 << 8)
#define DMA_CB_TI_ENABLE_DEST_INC       (1 << 4)
#define DMA_CB_TI_ENABLE_2DMODE         (1 << 1)
#define DMA_CB_TI_WAITS(num)            (((num) & 0x1f) << 21) // & 0x1f to contrain to 5 bits (max num = 31)

#define DMA_CB_TXFR_YLEN(yLen)  (((yLen) & 0x3fff) << 16) // & 0x3fff to constrain to 14 bits
#define DMA_CB_TXFR_XLEN(xLen)  ((xLen) & 0xffff) // & 0xffff to constrain to 16 bits

#define DMA_CB_STRIDE_SRC(srcStride)    ((srcStride) & 0xffff) // & 0xffff to constrain to 16 bits
#define DMA_CB_STRIDE_DEST(destStride)  (((destStride) & 0xffff) << 16) // & 0xffff to constrain to 16 bits

#define DMA_CS_RESET    (1 << 31)
#define DMA_CS_ABORT    (1 << 30)
#define DMA_CS_END      (1 << 1)
#define DMA_CS_ACTIVE   (1 << 0)


volatile uint32_t* gpio;
volatile uint32_t* dma;

#define INP_GPIO(g) *(gpio + ((g) / 10)) &= ~(7 << (((g) % 10) * 3))
#define OUT_GPIO(g) *(gpio + ((g) / 10)) |= (1 << (((g) % 10) * 3))

#define GPIO_SET *(gpio + 7)
#define GPIO_CLR *(gpio + 10)

#define PIN_RED 18
#define PIN_GREEN 23
#define PIN_BLUE 24


void setupGPIOPins();
void clearGPIOPins();
void startDMA();

int main(int argc, char* argv)
{
    gpio = map_mem(GPIO_PHYSICAL_BASE, GPIO_REGISTER_SIZE);
    dma = map_mem(DMA_PHYSICAL_BASE, DMA_REGISTER_SIZE);

    setupGPIOPins();
    clearGPIOPins();

    startDMA();

    clearGPIOPins();

    return 0;
}

void setupGPIOPins()
{
    INP_GPIO(PIN_RED);
    OUT_GPIO(PIN_RED);

    INP_GPIO(PIN_GREEN);
    OUT_GPIO(PIN_GREEN);

    INP_GPIO(PIN_BLUE);
    OUT_GPIO(PIN_BLUE);
}

void clearGPIOPins()
{
    GPIO_CLR = 1 << PIN_RED;
    GPIO_CLR = 1 << PIN_GREEN;
    GPIO_CLR = 1 << PIN_BLUE;
}

void startDMA()
{
    struct ControlBlock {
        uint32_t transferInfo;
        uint32_t srcAddr;
        // bus address (not physical)
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
        uint32_t TDModeStride;

        // bus address (not physical)
        uint32_t nextCBAddr;
        uint32_t reserved[2];
    };

    struct DmaChannelHeader {
        /*
            controlAndStatus bits:
                31: RESET
                30: ABORT
                    1 = abort current control block and load the next block
                 1: END
                 0: ACTIVE
        */
        uint32_t controlAndStatus;

        // bus address (not physical)
        uint32_t CBAddr;
    };

    
    struct GPIOData {
        uint32_t set[2];
        uint32_t clr[2];
    };


    // set buffer
    struct VCMemory gpioSetMem = alloc_vc_uncached(sizeof(struct GPIOData), 1);
    struct GPIOData* gpioSet = (struct GPIOData*) gpioSetMem.virtual_addr;
    gpioSet->set[0] = (1 << PIN_RED);
    gpioSet->clr[0] = (1 << PIN_GREEN) | (1 << PIN_BLUE);

    // clear buffer
    struct VCMemory gpioClrMem = alloc_vc_uncached(sizeof(struct GPIOData), 1);
    struct GPIOData* gpioClr = (struct GPIOData*) gpioClrMem.virtual_addr;
    gpioClr->set[0] = 0;
    gpioClr->clr[0] = (1 << PIN_RED) | (1 << PIN_GREEN) | (1 << PIN_BLUE);


    unsigned int num_frames = 100;
    unsigned int brightness = 50;
    unsigned int split = num_frames - brightness;
    struct VCMemory frames[num_frames];

    for (unsigned int i = 0; i < split; i++)
    {
        frames[num_frames - i - 1] = alloc_vc_uncached(sizeof(struct ControlBlock), 32);
        struct ControlBlock* cb = (struct ControlBlock*) frames[num_frames - i - 1].virtual_addr;
        cb->transferInfo = DMA_CB_TI_ENABLE_2DMODE | DMA_CB_TI_ENABLE_SRC_INC | DMA_CB_TI_ENABLE_DEST_INC;
        cb->srcAddr = gpioClrMem.bus_addr;
        cb->destAddr = GPIO_BUS_BASE + GPIO_SET_OFFSET;
        cb->transferLen = DMA_CB_TXFR_YLEN(2) | DMA_CB_TXFR_XLEN(8);
        cb->TDModeStride = DMA_CB_STRIDE_DEST(GPIO_CLR_OFFSET - GPIO_SET_OFFSET - 8) | DMA_CB_STRIDE_SRC(0);
        if (i > 0)
            cb->nextCBAddr = frames[num_frames - i].bus_addr;
    }

    for (unsigned int i = 0; i < num_frames - split; i++)
    {
        frames[num_frames - split - i - 1] = alloc_vc_uncached(sizeof(struct ControlBlock), 32);
        struct ControlBlock* cb = (struct ControlBlock*) frames[num_frames - split - i - 1].virtual_addr;
        cb->transferInfo = DMA_CB_TI_ENABLE_2DMODE | DMA_CB_TI_ENABLE_SRC_INC | DMA_CB_TI_ENABLE_DEST_INC;
        cb->srcAddr = gpioSetMem.bus_addr;
        cb->destAddr = GPIO_BUS_BASE + GPIO_SET_OFFSET;
        cb->transferLen = DMA_CB_TXFR_YLEN(2) | DMA_CB_TXFR_XLEN(8);
        cb->TDModeStride = DMA_CB_STRIDE_DEST(GPIO_CLR_OFFSET - GPIO_SET_OFFSET - 8) | DMA_CB_STRIDE_SRC(0);
        cb->nextCBAddr = frames[num_frames - split - i].bus_addr;
    }

    // loop back
    ((struct ControlBlock*) frames[num_frames - 1].virtual_addr)->nextCBAddr = frames[0].bus_addr;


    // struct VCMemory cbMem = alloc_vc_uncached(sizeof(struct ControlBlock), 32);
    // struct ControlBlock* cb = (struct ControlBlock*) cbMem.virtual_addr;
    // cb->transferInfo = DMA_CB_TI_ENABLE_2DMODE | DMA_CB_TI_ENABLE_SRC_INC | DMA_CB_TI_ENABLE_DEST_INC;
    // cb->srcAddr = gpioDataMem.bus_addr;
    // cb->destAddr = GPIO_BUS_BASE + GPIO_SET_OFFSET;
    // // transfer 8 bytes two times - two set registers and two clear registers respectively (see GPIOData)
    // cb->transferLen = DMA_CB_TXFR_YLEN(2) | DMA_CB_TXFR_XLEN(8);
    // // write gpioData.set to GPIO_SET registers, then increment destAddr in order to write gpioData.clr to GPIO_CLR registers
    // cb->TDModeStride = DMA_CB_STRIDE_DEST(GPIO_CLR_OFFSET - GPIO_SET_OFFSET - 8) | DMA_CB_STRIDE_SRC(0);
    // cb->nextCBAddr = cbMem.bus_addr;

    struct DmaChannelHeader* dmaChannel = (struct DmaChannelHeader*) (dma + (DMA_CHANNEL * DMA_CHANNEL_OFFSET) / 4);
    dmaChannel->controlAndStatus |= DMA_CS_END;
    dmaChannel->CBAddr = frames[0].bus_addr;
    dmaChannel->controlAndStatus |= DMA_CS_ACTIVE;

    for (unsigned int i = 0; i < 20; i++)
    {
        printf("%u\n", i);
        sleep(1);
    }

    dmaChannel->controlAndStatus |= DMA_CS_ABORT;
    dmaChannel->controlAndStatus |= DMA_CS_RESET;

    free_vc_uncached(&gpioSetMem);
    free_vc_uncached(&gpioClrMem);
    for (unsigned int i = 0; i < num_frames; i++)
    {
        free_vc_uncached(&frames[i]);
    }
}