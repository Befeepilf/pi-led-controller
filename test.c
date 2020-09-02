#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

size_t align_to_block(size_t size, size_t block)
{
    return ((size - 1) / block + 1) * block;
}

uint8_t* create_tag(uint32_t id, size_t value_size, uint8_t* value, size_t* tag_size)
{
    assert(value_size > 0 ? value != NULL : value == NULL);
    assert(tag_size != NULL);

    *tag_size = align_to_block(
        3 * sizeof(uint32_t) + value_size,
        4
    );
    uint8_t* tag = malloc(*tag_size);

    unsigned int i = 0;
    tag[i] = id;
    i += 4;
    tag[i] = value_size;
    i += 4;
    tag[i] = 0x0; // code
    i += 4;
    for (unsigned int j = 0; j < value_size; j++)
    {
        tag[i] = value[j];
        i++;
    }
    if (i < *tag_size)
        tag[i] = 0x0; // padding

    return tag;
}

uint32_t* create_buffer(size_t value_size, uint8_t* value)
{
    assert(value_size > 0 ? value != NULL : value == NULL);

    size_t buffer_size = align_to_block(
        3 * sizeof(uint32_t) + value_size,
        4
    );
    uint8_t* buffer = malloc(buffer_size);

    unsigned int i = 0;
    buffer[i] = buffer_size;
    i += 4;
    buffer[i] = 0x00000000;
    i += 4;
    for (unsigned int j = 0; j < value_size; j++)
    {
        buffer[i] = value[j];
        i += 1;
    }
    buffer[i] = 0x00000000;
    i += 4;
    if (i < buffer_size)
        buffer[i] = 0x0; // padding

    return (uint32_t*) buffer;
}

int main(int argc, char* argv)
{
    uint8_t* value = malloc(2 * sizeof(uint8_t));
    value[0] = 23;
    value[1] = 18;

    size_t tag_size;
    uint8_t* tag = create_tag(12, 2 * sizeof(uint8_t), value, &tag_size);

    uint32_t* buff = create_buffer(tag_size, tag);

    return 0;
}