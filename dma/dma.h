#ifndef DMA_H
#define DMA_H

#include <stdint.h>
#include "../peri.h"

#define DMA_BASE_OFFSET     0x007000
#define DMA_PHYSICAL_BASE   (BCM2708_PERI_PHYSICAL_BASE + DMA_BASE_OFFSET)
#define DMA_REGISTER_SIZE   0xff4
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


struct DMAControlBlock {
    uint32_t transferInfo;
    uint32_t srcAddr; // bus address
    uint32_t destAddr; // bus address

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

    uint32_t nextCBAddr; // bus address
    uint32_t reserved[2];
};

// this is not a complete channel register but only the upper 8 bytes
struct DMAChannelHeader {
    uint32_t controlAndStatus;
    uint32_t CBAddr; // bus address
};


uint32_t* mapDMARegister();
struct DMAChannelHeader* getDMAChannel(uint32_t* dma, uint8_t channel);
void startDMAChannel(struct DMAChannelHeader* channel, uint32_t firstFrameBusAddr);
void stopDMAChannel(struct DMAChannelHeader* channel);

#endif