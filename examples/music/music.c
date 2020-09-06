#include <assert.h>
#include <pulse/simple.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "../../src/led/led.h"

#define BUFFER_SIZE 1024
#define MAX_VOLUME  12000.0 // shouldn't be bigger than 32767 (int16_t), and should be float

uint16_t average(int16_t* buffer, size_t length)
{
    uint64_t avg = 0;
    for (unsigned int i = 0; i < length; i++)
    {
        avg += abs(buffer[i]);
    }
    avg /= length;

    return (avg > MAX_VOLUME ? MAX_VOLUME : avg);
}

int main(int argc, char* argv)
{
    pa_sample_spec sample_spec;
    sample_spec.format = PA_SAMPLE_S16NE;
    sample_spec.channels = 2;
    sample_spec.rate = 44100;

    int stream_error;

    pa_simple* s = pa_simple_new(
        NULL,               // use default server
        "LED Controller",   // name of this application
        PA_STREAM_RECORD,   // stream direction
        NULL,               // use default sink
        "Record audio",     // name of stream
        &sample_spec,
        NULL,               // use default channel map
        NULL,               // use default buffering attributes
        &stream_error
    );

    assert(s != NULL);

    startLED();

    int16_t buffer[BUFFER_SIZE];
    uint8_t last_brightness = 0;
    while(1)
    {
        pa_simple_read(s, buffer, sizeof(buffer), &stream_error);
        uint16_t volume = average(buffer, BUFFER_SIZE);

        uint8_t brightness = (volume / MAX_VOLUME) * 255;

        setRGB(
            0.9 * last_brightness + 0.1 * brightness,
            brightness,
            0.5 * last_brightness + 0.5 * brightness
        );

        last_brightness = 0.8 * last_brightness + 0.2 * brightness;
    }

    // although never reached
    stopLED();
    pa_simple_free(s);
    return 0;
}