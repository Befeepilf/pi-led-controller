#include <unistd.h> // for sleep()
#include "led/led.h"

int main(int argc, char* argv)
{
    startLED();

    for (unsigned int i = 0; i < 10; i++)
    {
        setRGB(255, 0 , 0);
        sleep(1);
        setRGB(0, 255 , 0);
        sleep(1);
        setRGB(0, 0 , 255);
        sleep(1);
    }

    stopLED();

    return 0;
}