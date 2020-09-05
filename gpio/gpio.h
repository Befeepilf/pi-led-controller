#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>

#include "../peri.h"
#include "../vc-memory/vc-memory.h"

#define GPIO_BASE_OFFSET    0x200000
#define GPIO_PHYSICAL_BASE  (BCM2708_PERI_PHYSICAL_BASE + GPIO_BASE_OFFSET)
#define GPIO_BUS_BASE       (BCM2708_PERI_BUS_BASE + GPIO_BASE_OFFSET)
#define GPIO_REGISTER_SIZE  0xb4
#define GPIO_SET_OFFSET     0x1c
#define GPIO_CLR_OFFSET     0x28

#define GPIO_PIN_MODE_INP   0
#define GPIO_PIN_MODE_OUT   1

#define GPIO_PIN_OUT_ON     1
#define GPIO_PIN_OUT_OFF    0

struct GPIOData {
    uint32_t set[2];
    uint32_t clr[2];
};

uint32_t* mapGPIORegister();
void unmapGPIORegister(uint32_t* gpio);
void setPinMode(uint32_t* gpio, uint8_t pin, uint8_t mode);
void setPinOutState(uint32_t* gpio, uint8_t pin, uint8_t state);
struct VCMemory createUncachedGPIOData(uint32_t set1, uint32_t set2, uint32_t clr1, uint32_t clr2);

#endif