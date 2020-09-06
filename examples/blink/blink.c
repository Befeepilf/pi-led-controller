#include <unistd.h> // for sleep()
#include "../../src/led/led.h"

int main(int argc, char* argv)
{
    startLED();

    for (unsigned int i = 0; i < 25; i++)
    {
        setRGB(255, 0 , 0);
        usleep(50000);
        setRGB(0, 255 , 0);
        usleep(50000);
        setRGB(0, 0 , 255);
        usleep(50000);
    }

    stopLED();

    return 0;
}