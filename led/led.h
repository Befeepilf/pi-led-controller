#ifndef LED_H
#define LED_h

#include <stdint.h>

void startLED();
void stopLED();
void setRGB(uint8_t r, uint8_t g, uint8_t b);

#endif