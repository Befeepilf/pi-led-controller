#include <assert.h> // for assert()
#include <stdint.h> // for uintX_t
#include <stdio.h> // for fprintf(), stderr
#include <stdlib.h> // for exit(), NULL

#include "gpio.h"
#include "../util/util.h"
#include "../vc-memory/vc-memory.h"

#define GPIO_MIN_PIN        0
#define GPIO_MAX_PIN        53

/* Prototypes for internal util functions */
void checkPinId(uint8_t pin);


/* Public functions */

uint32_t* mapGPIORegister()
{
    return (uint32_t*) map_mem(GPIO_PHYSICAL_BASE, GPIO_REGISTER_SIZE);
}

void unmapGPIORegister(uint32_t* gpio)
{
    unmap_mem(gpio, GPIO_REGISTER_SIZE);
}

void setPinMode(uint32_t* gpio, uint8_t pin, uint8_t mode)
{
    assert(gpio != NULL);
    checkPinId(pin);

    if (mode == GPIO_PIN_MODE_INP)
        *(gpio + ((pin) / 10)) &= ~(7 << (((pin) % 10) * 3));
    else if (mode == GPIO_PIN_MODE_OUT)
    {
        setPinMode(gpio, pin, GPIO_PIN_MODE_INP); // must first be in input mode before being in output mode
        *(gpio + ((pin) / 10)) |= (1 << (((pin) % 10) * 3));
    }
    else
    {
        fprintf(stderr, "GPIO error: invalid pin mode %u. Must be either %u or %u.\n", mode, GPIO_PIN_MODE_INP, GPIO_PIN_MODE_OUT);
        exit(-1);
    }
}

void setPinOutState(uint32_t* gpio, uint8_t pin, uint8_t state)
{
    assert(gpio != NULL);
    checkPinId(pin);

    if (state == GPIO_PIN_OUT_ON)
        *(gpio + GPIO_SET_OFFSET / 4) = 1 << pin;
    else if (state == GPIO_PIN_OUT_OFF)
        *(gpio + GPIO_CLR_OFFSET / 4) = 1 << pin;
    else
    {
        fprintf(stderr, "GPIO error: invalid pin out state %u. Must be either %u or %u.\n", state, GPIO_PIN_OUT_ON, GPIO_PIN_OUT_OFF);
        exit(-1);
    }
}

struct VCMemory createUncachedGPIOData(uint32_t set1, uint32_t set2, uint32_t clr1, uint32_t clr2)
{
    struct VCMemory gpioDataMem = alloc_vc_uncached(sizeof(struct GPIOData), 1);
    struct GPIOData* gpioData = (struct GPIOData*) gpioDataMem.virtual_addr;

    gpioData->set[0] = set1;
    gpioData->set[1] = set2;
    gpioData->clr[0] = clr1;
    gpioData->clr[1] = clr2;

    return gpioDataMem;
}


/* Internal util functions */

void checkPinId(uint8_t pin)
{
    if (pin < GPIO_MIN_PIN || pin > GPIO_MAX_PIN)
    {
        fprintf(stderr, "GPIO error: pin id %u is out of range. Must be between %u and %u.\n", pin, GPIO_MIN_PIN, GPIO_MAX_PIN);
        exit(-1);
    }
}