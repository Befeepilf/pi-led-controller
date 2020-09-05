#include <stdint.h>
#include <unistd.h>

#include "../dma/dma.h"
#include "../gpio/gpio.h"
#include "led.h"
#include "../util/util.h"
#include "../vc-memory/vc-memory.h"

#define DMA_CHANNEL 5

#define PIN_RED     18
#define PIN_GREEN   23
#define PIN_BLUE    24

#define NUM_FRAMES 256


uint32_t* gpio;
uint32_t* dma;

struct DMAChannelHeader* dmaChannel;

struct Frame {
    struct VCMemory gpioMem;
    struct VCMemory dmaCB;
};
struct Frame frames[NUM_FRAMES];


void initFrames()
{
    for (unsigned int i = 0; i < NUM_FRAMES; i++)
    {
        struct Frame frame;

        frame.gpioMem = createUncachedGPIOData(0, 0, 0, 0);

        frame.dmaCB = alloc_vc_uncached(sizeof(struct DMAControlBlock), 32);
        struct DMAControlBlock* cb = (struct DMAControlBlock*) frame.dmaCB.virtual_addr;
        cb->transferInfo = DMA_CB_TI_ENABLE_2DMODE | DMA_CB_TI_ENABLE_SRC_INC | DMA_CB_TI_ENABLE_DEST_INC;
        cb->srcAddr = frame.gpioMem.bus_addr;
        cb->destAddr = GPIO_BUS_BASE + GPIO_SET_OFFSET;
        cb->transferLen = DMA_CB_TXFR_YLEN(2) | DMA_CB_TXFR_XLEN(8);
        cb->TDModeStride = DMA_CB_STRIDE_DEST(GPIO_CLR_OFFSET - GPIO_SET_OFFSET - 8) | DMA_CB_STRIDE_SRC(0);

        frames[i] = frame;

        if (i > 0)
            ((struct DMAControlBlock*) frames[i - 1].dmaCB.virtual_addr)->nextCBAddr = frame.dmaCB.bus_addr;
    }

    // loop back
    ((struct DMAControlBlock*) frames[NUM_FRAMES - 1].dmaCB.virtual_addr)->nextCBAddr = frames[0].dmaCB.bus_addr;
}

void clearFrames()
{
    for (unsigned int i = 0; i < NUM_FRAMES; i++)
    {
        free_vc_uncached(&frames[i].dmaCB);
        free_vc_uncached(&frames[i].gpioMem);
    }
}


void startLED()
{
    gpio = mapGPIORegister();
    dma = mapDMARegister();

    setPinMode(gpio, PIN_RED, GPIO_PIN_MODE_OUT);
    setPinMode(gpio, PIN_GREEN, GPIO_PIN_MODE_OUT);
    setPinMode(gpio, PIN_BLUE, GPIO_PIN_MODE_OUT);

    dmaChannel = getDMAChannel(dma, DMA_CHANNEL);
    initFrames();
    startDMAChannel(dmaChannel, frames[0].dmaCB.bus_addr);
}

void stopLED()
{
    setRGB(0, 0, 0);
    usleep(100); // give the DMA some time to write changes from previous setRGB() call to memory
    stopDMAChannel(dmaChannel);
    clearFrames();

    unmapGPIORegister(gpio);
    unmapDMARegister(dma);
}

void setRGB(uint8_t r, uint8_t g, uint8_t b)
{
    for (unsigned int i = 0; i < NUM_FRAMES; i++)
    {
        struct GPIOData* gpioData = frames[i].gpioMem.virtual_addr;
        gpioData->set[0] = 0;
        gpioData->clr[0] = 0;

        if (i <= r)
            gpioData->set[0] = 1 << PIN_RED;
        else
            gpioData->clr[0] = 1 << PIN_RED;

        if (i <= g)
            gpioData->set[0] |= 1 << PIN_GREEN;
        else
            gpioData->clr[0] |= 1 << PIN_GREEN;

        if (i <= b)
            gpioData->set[0] |= 1 << PIN_BLUE;
        else
            gpioData->clr[0] |= 1 << PIN_BLUE;
    }
}