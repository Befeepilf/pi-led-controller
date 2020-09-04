#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "dma/dma.h"
#include "gpio/gpio.h"
#include "util/util.h"
#include "vc-memory/vc-memory.h"


#define PIN_RED     18
#define PIN_GREEN   23
#define PIN_BLUE    24

#define NUM_FRAMES  256


void startDMA();

struct VCMemory frames[NUM_FRAMES];

int main(int argc, char* argv)
{
    uint32_t* gpio = mapGPIORegister();
    uint32_t* dma = mapDMARegister();

    setPinMode(gpio, PIN_RED, GPIO_PIN_MODE_OUT);
    setPinMode(gpio, PIN_GREEN, GPIO_PIN_MODE_OUT);
    setPinMode(gpio, PIN_BLUE, GPIO_PIN_MODE_OUT);

    setPinOutState(gpio, PIN_RED, GPIO_PIN_OUT_ON);
    setPinOutState(gpio, PIN_GREEN, GPIO_PIN_OUT_OFF);
    setPinOutState(gpio, PIN_BLUE, GPIO_PIN_OUT_OFF);

    startDMA(dma);

    return 0;
}


void generateFrames(uint8_t brightness, struct VCMemory* state1, struct VCMemory* state2)
{
    unsigned int split = NUM_FRAMES - brightness;

    for (unsigned int i = 0; i < split; i++)
    {
        frames[NUM_FRAMES - i - 1] = alloc_vc_uncached(sizeof(struct DMAControlBlock), 32);
        struct DMAControlBlock* cb = (struct DMAControlBlock*) frames[NUM_FRAMES - i - 1].virtual_addr;
        cb->transferInfo = DMA_CB_TI_ENABLE_2DMODE | DMA_CB_TI_ENABLE_SRC_INC | DMA_CB_TI_ENABLE_DEST_INC;
        cb->srcAddr = state2->bus_addr;
        cb->destAddr = GPIO_BUS_BASE + GPIO_SET_OFFSET;
        cb->transferLen = DMA_CB_TXFR_YLEN(2) | DMA_CB_TXFR_XLEN(8);
        cb->TDModeStride = DMA_CB_STRIDE_DEST(GPIO_CLR_OFFSET - GPIO_SET_OFFSET - 8) | DMA_CB_STRIDE_SRC(0);
        if (i > 0)
            cb->nextCBAddr = frames[NUM_FRAMES - i].bus_addr;
    }

    for (unsigned int i = 0; i < brightness; i++)
    {
        frames[brightness - i - 1] = alloc_vc_uncached(sizeof(struct DMAControlBlock), 32);
        struct DMAControlBlock* cb = (struct DMAControlBlock*) frames[brightness - i - 1].virtual_addr;
        cb->transferInfo = DMA_CB_TI_ENABLE_2DMODE | DMA_CB_TI_ENABLE_SRC_INC | DMA_CB_TI_ENABLE_DEST_INC;
        cb->srcAddr = state1->bus_addr;
        cb->destAddr = GPIO_BUS_BASE + GPIO_SET_OFFSET;
        cb->transferLen = DMA_CB_TXFR_YLEN(2) | DMA_CB_TXFR_XLEN(8);
        cb->TDModeStride = DMA_CB_STRIDE_DEST(GPIO_CLR_OFFSET - GPIO_SET_OFFSET - 8) | DMA_CB_STRIDE_SRC(0);
        cb->nextCBAddr = frames[brightness - i].bus_addr;
    }

    // loop back
    ((struct DMAControlBlock*) frames[NUM_FRAMES - 1].virtual_addr)->nextCBAddr = frames[0].bus_addr;
}

void clearFrames()
{
    for (unsigned int i = 0; i < NUM_FRAMES; i++)
    {
        free_vc_uncached(&frames[i]);
    }
}

void startDMA(uint32_t* dma)
{
    struct DMAChannelHeader* dmaChannel = getDMAChannel(dma, 5);

    struct VCMemory gpioSetMem = createUncachedGPIOData(1 << PIN_RED, 0, (1 << PIN_GREEN) | (1 << PIN_BLUE), 0);
    struct VCMemory gpioClrMem = createUncachedGPIOData(0, 0, (1 << PIN_RED) | (1 << PIN_GREEN) | (1 << PIN_BLUE), 0);

    generateFrames(1, &gpioSetMem, &gpioClrMem);

    startDMAChannel(dmaChannel, frames[0].bus_addr);

    // sleep
    for (unsigned int i = 0; i < 10; i++)
    {
        printf("%u\n", i);
        sleep(1);
    }

    stopDMAChannel(dmaChannel);

    clearFrames();
    free_vc_uncached(&gpioSetMem);
    free_vc_uncached(&gpioClrMem);
}