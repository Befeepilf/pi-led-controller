#include <assert.h> // for assert()
#include <stdint.h> // for uintX_t
#include <stdio.h> // for fprintf(), stderr
#include <stdlib.h> // for exit(), NULL

#include "dma.h"
#include "../mailbox/mailbox.h" // for get_usable_dma_channels()
#include "../util/util.h" // for map_mem

#define DMA_MIN_CHANNEL     0
#define DMA_MAX_CHANNEL     15


/* Prototypes for internal util functions */
void checkDMAChannel(uint8_t channel);


/* Public functions */

uint32_t* mapDMARegister()
{
    return (uint32_t*) map_mem(DMA_PHYSICAL_BASE, DMA_REGISTER_SIZE);
}

void unmapDMARegister(uint32_t* dma)
{
    unmap_mem(dma, DMA_REGISTER_SIZE);
}

struct DMAChannelHeader* getDMAChannel(uint32_t* dma, uint8_t channel)
{
    assert(dma != NULL);
    checkDMAChannel(channel);

    return (struct DMAChannelHeader*) (dma + (channel * DMA_CHANNEL_OFFSET) / 4);
}

void startDMAChannel(struct DMAChannelHeader* channel, uint32_t firstFrameBusAddr)
{
    channel->controlAndStatus |= DMA_CS_END;
    channel->CBAddr = firstFrameBusAddr;
    channel->controlAndStatus |= DMA_CS_ACTIVE;
}

void stopDMAChannel(struct DMAChannelHeader* channel)
{
    channel->controlAndStatus |= DMA_CS_ABORT;
    channel->controlAndStatus |= DMA_CS_RESET;
}


/* Internal util functions */

void checkDMAChannel(uint8_t channel)
{
    if (channel < DMA_MIN_CHANNEL || channel > DMA_MAX_CHANNEL)
    {
        fprintf(stderr, "DMA error: channel id %u is out of range. Must be between %u and %u.\n", channel, DMA_MIN_CHANNEL, DMA_MAX_CHANNEL);
        exit(-1);
    }

    uint16_t channels = get_usable_dma_channels();
    if (~channels & (1 << channel))
    {
        fprintf(stderr, "DMA error: VC says channel %u is not usable.\n", channel);
        exit(-1);
    }
}