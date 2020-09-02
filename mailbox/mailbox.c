#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "mailbox.h"


int open_vcio()
{
    int vcio_fd;
    if ((vcio_fd = open("/dev/vcio", O_RDWR|O_SYNC)) < 0)
    {
        printf("Cannot open /dev/vcio: %s\n", strerror(errno));
        exit(-1);
    }

    return vcio_fd;
}

size_t align_to_block(size_t size, size_t block)
{
    return ((size - 1) / block + 1) * block;
}

uint32_t* create_tag(uint32_t id, size_t value_size, uint32_t* value, size_t* tag_size)
{
    assert(value_size > 0 ? value != NULL : value == NULL);
    assert(tag_size != NULL);

    *tag_size = 3 * sizeof(uint32_t) + value_size;
    uint32_t* tag = malloc(*tag_size);

    unsigned int i = 0;
    tag[i++] = id;
    tag[i++] = value_size;
    tag[i++] = value_size; // 0x0 indicates request
    for (unsigned int j = 0; j < value_size / 4; j++)
    {
        tag[i++] = value[j];
    }

    return tag;
}

uint32_t* create_buffer(size_t value_size, uint32_t* value)
{
    assert(value_size > 0 ? value != NULL : value == NULL);

    size_t buffer_size = 3 * sizeof(uint32_t) + value_size;
    uint32_t* buffer = malloc(buffer_size);

    unsigned int i = 0;
    buffer[i++] = buffer_size;
    buffer[i++] = 0x00000000;
    for (unsigned int j = 0; j < value_size / 4; j++)
    {
        buffer[i++] = value[j];
    }
    buffer[i] = 0x00000000;


    return buffer;
}

size_t make_request(uint32_t* buffer)
{
    int vcio_fd = open_vcio();


    if (ioctl(vcio_fd, _IOWR(100, 0, char*), buffer) < 0)
    {
        printf("ioctl: request failed: %s\n", strerror(errno));
        exit(-1);
    }

    close(vcio_fd);

    uint32_t res_code = buffer[1];
    if (res_code == 0x80000000)
        printf("ioctl: request was successful\n");
    else if (res_code == 0x80000001)
    {
        printf("ioctl: error parsing request buffer\n");
        exit(-1);
    }
    else
    {
        printf("ioctl: unknown response code\n");
        exit(-1);
    }

    uint32_t tag_res_code = buffer[4];

    if (tag_res_code >> 31 == 0x0)
    {
        printf("mailbox: unknown request tag\n");
        exit(-1);
    }

    uint32_t tag_res_len = tag_res_code ^ 0x80000000; // ignore most significant bit
    printf("Response size: %u bytes\n", tag_res_len);

    return (size_t) tag_res_len;
}

uint32_t get_board_rev()
{
    size_t res_size = 4;
    uint32_t* res_buff = malloc(res_size);

    size_t tag_size;

    uint32_t* tag = create_tag(
        MB_TAG_BOARD_REV,
        res_size,
        res_buff,
        &tag_size
    );

    uint32_t* buffer = create_buffer(tag_size, tag);
    make_request(buffer);

    uint32_t rev = buffer[5];
    printf("Revision: %u\n", rev);

    return rev;
}


int main(int argc, char* argv)
{
    get_board_rev();

    return 0;
}