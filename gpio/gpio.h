#ifndef GPIO_H
#define GPIO_H

#define GPIO_PIN_MODE_INP   0
#define GPIO_PIN_MODE_OUT   1

#define GPIO_PIN_OUT_ON     1
#define GPIO_PIN_OUT_OFF    0

uint32_t* mapGPIORegister();
void setPinMode(uint32_t* gpio, uint8_t pin, uint8_t mode);
void setPinOutState(uint32_t* gpio, uint8_t pin, uint8_t state);

#endif