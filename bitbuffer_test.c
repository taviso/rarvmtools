
// Bitstream management routines.
// Copyright (C) 2012 Tavis Ormandy <taviso@cmpxchg8b.com>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include "bitbuffer.h"

int main(int argc, char **argv)
{
    bitbuf_t      *bitbuffer;
    const uint8_t *buf;
    uint32_t       count;

    assert(bitbuf_create(&bitbuffer));
    assert(bitbuf_numbits(bitbuffer) == 0);

    // Append a 1 bit and read it back.
    assert(bitbuf_append(bitbuffer, 0b00000001, 1));
    assert(bitbuf_numbits(bitbuffer) == 1);
    assert(bitbuf_getbits(bitbuffer, &buf, &count));
    assert(count == 1);
    assert(buf[0] == 0b00000001);

    // Append 8 bits and read it back.
    assert(bitbuf_append(bitbuffer, 0b10101010, 8));
    assert(bitbuf_numbits(bitbuffer) == 9);
    assert(bitbuf_getbits(bitbuffer, &buf, &count));
    assert(count == 2);
    assert(buf[0] == 0b11010101);
    assert(buf[1] == 0b00000000);

    // Append 1 more bit.
    assert(bitbuf_append(bitbuffer, 0b00000000, 1));
    assert(bitbuf_numbits(bitbuffer) == 10);
    assert(bitbuf_getbits(bitbuffer, &buf, &count));
    assert(count == 2);
    assert(buf[0] == 0b11010101);
    assert(buf[1] == 0b00000000);

    // Append a big chunk of bits.
    assert(bitbuf_append(bitbuffer, 0b01100001001011011010101010010011, 32));
    assert(bitbuf_numbits(bitbuffer) == 42);
    assert(bitbuf_getbits(bitbuffer, &buf, &count));
    assert(count == 6);
    assert(buf[0] == 0b11010101);
    assert(buf[1] == 0b00011000);
    assert(buf[2] == 0b01001011);
    assert(buf[3] == 0b01101010);
    assert(buf[4] == 0b10100100);
    assert(buf[5] == 0b00000011);

    // Clean up
    assert(bitbuf_destroy(bitbuffer));

    // Start a new one, and create a pattern in odd increments.
    assert(bitbuf_create(&bitbuffer));
    assert(bitbuf_append(bitbuffer, 0b11111111111111111111111111111111, 32));
    assert(bitbuf_append(bitbuffer, 0b11111111111111111111111111111111, 1));
    assert(bitbuf_append(bitbuffer, 0b11111111111111111111111111111111, 2));
    assert(bitbuf_append(bitbuffer, 0b11111111111111111111111111111111, 3));
    assert(bitbuf_append(bitbuffer, 0b11111111111111111111111111111111, 4));
    assert(bitbuf_append(bitbuffer, 0b11111111111111111111111111111111, 5));
    assert(bitbuf_append(bitbuffer, 0b11111111111111111111111111111111, 6));
    assert(bitbuf_append(bitbuffer, 0b11111111111111111111111111111111, 7));
    assert(bitbuf_append(bitbuffer, 0b11111111111111111111111111111111, 4));
    assert(bitbuf_numbits(bitbuffer) == 64);
    assert(bitbuf_getbits(bitbuffer, &buf, &count));
    assert(count == 8);
    assert(memcmp(buf, "\xff\xff\xff\xff\xff\xff\xff\xff", 8) == 0);
    assert(bitbuf_destroy(bitbuffer));
    return 0;
}
