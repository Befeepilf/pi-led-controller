# GPIO LED Controller
This library is meant for controlling an RGB LED by switching the red, green and blue channel each with a seperate GPIO pin.


## Usage

### GPIO pins
Currently, the library assumes the following mapping between GPIO pins and RGB channels:
| GPIO pin | Corresponding RGB channel |
| -------- | ------------------------- |
| 18       | Red                       |
| 23       | Green                     |
| 24       | Blue                      |

You can change the mapping by altering the `PIN_RED`, `PIN_GREEN` & `PIN_BLUE` macros inside `led/led.c`.

### DMA channel
The library also assumes that the 5th DMA channel is usable and enabled. To change the DMA channel alter the `DMA_CHANNEL` macro inside `led/led.c`.

### API
All high-level functions are declared in led/led.h, which are:

| Function name     | Description   |
| ----------------- | ------------- |
| `startLED()`      | Must be called *once* in the program before using `setRGB`. Maps DMA & GPIO into memory and initializes the DMA. |
| `stopLED()`       | Unmaps DMA & GPIO and frees allocated memory. Must be called before program exits.  |
| `setRGB(uint8_t red, uint8_t green, uint8_t blue)` | Sets the current color. |

### Example
Here is a simple example to get you started:
```c
#include <unistd.h> // for sleep()
#include "led/led.h"

int main(int argc, char* argv)
{
    startLED();

    for (unsigned int i = 0; i < 100; i++)
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
```

### Compiling
This is *not* a header-only library. You have to compile and link all the source files in this repo except for `/example.c`.