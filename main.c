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


#define PIN_RED 18
#define PIN_GREEN 23
#define PIN_BLUE 24


int main(int argc, char* argv)
{
    uint32_t* gpio = mapGPIORegister();
    uint32_t* dma = mapDMARegister();

    getDMAChannel(dma, 5);

    setPinMode(gpio, PIN_RED, GPIO_PIN_MODE_OUT);
    setPinMode(gpio, PIN_GREEN, GPIO_PIN_MODE_OUT);
    setPinMode(gpio, PIN_BLUE, GPIO_PIN_MODE_OUT);

    setPinOutState(gpio, PIN_RED, GPIO_PIN_OUT_ON);
    setPinOutState(gpio, PIN_GREEN, GPIO_PIN_OUT_OFF);
    setPinOutState(gpio, PIN_BLUE, GPIO_PIN_OUT_OFF);

    //startDMA();

    // setPinOutState(gpio, PIN_RED, GPIO_PIN_OUT_OFF);
    // setPinOutState(gpio, PIN_GREEN, GPIO_PIN_OUT_OFF);
    // setPinOutState(gpio, PIN_BLUE, GPIO_PIN_OUT_OFF);

    return 0;
}


// void startDMA()
// {   
//     struct GPIOData {
//         uint32_t set[2];
//         uint32_t clr[2];
//     };

//     // set buffer
//     struct VCMemory gpioSetMem = alloc_vc_uncached(sizeof(struct GPIOData), 1);
//     struct GPIOData* gpioSet = (struct GPIOData*) gpioSetMem.virtual_addr;
//     gpioSet->set[0] = (1 << PIN_RED);
//     gpioSet->clr[0] = (1 << PIN_GREEN) | (1 << PIN_BLUE);

//     // clear buffer
//     struct VCMemory gpioClrMem = alloc_vc_uncached(sizeof(struct GPIOData), 1);
//     struct GPIOData* gpioClr = (struct GPIOData*) gpioClrMem.virtual_addr;
//     gpioClr->set[0] = 0;
//     gpioClr->clr[0] = (1 << PIN_RED) | (1 << PIN_GREEN) | (1 << PIN_BLUE);


//     unsigned int num_frames = 100;
//     unsigned int brightness = 50;
//     unsigned int split = num_frames - brightness;
//     struct VCMemory frames[num_frames];

//     for (unsigned int i = 0; i < split; i++)
//     {
//         frames[num_frames - i - 1] = alloc_vc_uncached(sizeof(struct ControlBlock), 32);
//         struct ControlBlock* cb = (struct ControlBlock*) frames[num_frames - i - 1].virtual_addr;
//         cb->transferInfo = DMA_CB_TI_ENABLE_2DMODE | DMA_CB_TI_ENABLE_SRC_INC | DMA_CB_TI_ENABLE_DEST_INC;
//         cb->srcAddr = gpioClrMem.bus_addr;
//         cb->destAddr = GPIO_BUS_BASE + GPIO_SET_OFFSET;
//         cb->transferLen = DMA_CB_TXFR_YLEN(2) | DMA_CB_TXFR_XLEN(8);
//         cb->TDModeStride = DMA_CB_STRIDE_DEST(GPIO_CLR_OFFSET - GPIO_SET_OFFSET - 8) | DMA_CB_STRIDE_SRC(0);
//         if (i > 0)
//             cb->nextCBAddr = frames[num_frames - i].bus_addr;
//     }

//     for (unsigned int i = 0; i < num_frames - split; i++)
//     {
//         frames[num_frames - split - i - 1] = alloc_vc_uncached(sizeof(struct ControlBlock), 32);
//         struct ControlBlock* cb = (struct ControlBlock*) frames[num_frames - split - i - 1].virtual_addr;
//         cb->transferInfo = DMA_CB_TI_ENABLE_2DMODE | DMA_CB_TI_ENABLE_SRC_INC | DMA_CB_TI_ENABLE_DEST_INC;
//         cb->srcAddr = gpioSetMem.bus_addr;
//         cb->destAddr = GPIO_BUS_BASE + GPIO_SET_OFFSET;
//         cb->transferLen = DMA_CB_TXFR_YLEN(2) | DMA_CB_TXFR_XLEN(8);
//         cb->TDModeStride = DMA_CB_STRIDE_DEST(GPIO_CLR_OFFSET - GPIO_SET_OFFSET - 8) | DMA_CB_STRIDE_SRC(0);
//         cb->nextCBAddr = frames[num_frames - split - i].bus_addr;
//     }

//     // loop back
//     ((struct ControlBlock*) frames[num_frames - 1].virtual_addr)->nextCBAddr = frames[0].bus_addr;


//     // struct VCMemory cbMem = alloc_vc_uncached(sizeof(struct ControlBlock), 32);
//     // struct ControlBlock* cb = (struct ControlBlock*) cbMem.virtual_addr;
//     // cb->transferInfo = DMA_CB_TI_ENABLE_2DMODE | DMA_CB_TI_ENABLE_SRC_INC | DMA_CB_TI_ENABLE_DEST_INC;
//     // cb->srcAddr = gpioDataMem.bus_addr;
//     // cb->destAddr = GPIO_BUS_BASE + GPIO_SET_OFFSET;
//     // // transfer 8 bytes two times - two set registers and two clear registers respectively (see GPIOData)
//     // cb->transferLen = DMA_CB_TXFR_YLEN(2) | DMA_CB_TXFR_XLEN(8);
//     // // write gpioData.set to GPIO_SET registers, then increment destAddr in order to write gpioData.clr to GPIO_CLR registers
//     // cb->TDModeStride = DMA_CB_STRIDE_DEST(GPIO_CLR_OFFSET - GPIO_SET_OFFSET - 8) | DMA_CB_STRIDE_SRC(0);
//     // cb->nextCBAddr = cbMem.bus_addr;

//     struct DmaChannelHeader* dmaChannel = (struct DmaChannelHeader*) (dma + (DMA_CHANNEL * DMA_CHANNEL_OFFSET) / 4);
//     dmaChannel->controlAndStatus |= DMA_CS_END;
//     dmaChannel->CBAddr = frames[0].bus_addr;
//     dmaChannel->controlAndStatus |= DMA_CS_ACTIVE;

//     for (unsigned int i = 0; i < 20; i++)
//     {
//         printf("%u\n", i);
//         sleep(1);
//     }

//     dmaChannel->controlAndStatus |= DMA_CS_ABORT;
//     dmaChannel->controlAndStatus |= DMA_CS_RESET;

//     free_vc_uncached(&gpioSetMem);
//     free_vc_uncached(&gpioClrMem);
//     for (unsigned int i = 0; i < num_frames; i++)
//     {
//         free_vc_uncached(&frames[i]);
//     }
// }