// Bitstream management routines.
// Copyright (C) 2012 Tavis Ormandy <taviso@cmpxchg8b.com>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <err.h>

#include "bitbuffer.h"

static inline uint32_t __attribute__((const)) max(uint32_t a, uint32_t b)
{
    return a > b ? a : b;
}

static inline uint32_t __attribute__((const)) min(uint32_t a, uint32_t b)
{
    return a > b ? b : a;
}

// Create a new empty bitbuf object, return true on success.
bool bitbuf_create(bitbuf_t **buffer)
{
    if ((*buffer = malloc(sizeof(bitbuf_t)))) {
        (*buffer)->occupancy = 0;
        (*buffer)->max       = 0;
        (*buffer)->bits      = NULL;
        return true;
    }
    return false;
}

// Release an existing bitbuf object.
bool bitbuf_destroy(bitbuf_t *buffer) {
    free(buffer->bits);
    free(buffer);
    return true;
}

// Append nbits from the integer in bits to bitbuf object in buffer.
bool bitbuf_append(bitbuf_t *buffer, uint32_t bits, uint8_t nbits)
{
    // Verify enough space to to handle these bits available in the object
    // already. If not, append enough for a full uint32_t using realloc. No
    // error checking, because this is a @!"£?!"ing rar assembler.
    if (buffer->occupancy + nbits > buffer->max) {
        uint8_t *newbits = realloc(buffer->bits, buffer->max / 8 + 4);

        buffer->bits  = newbits;
        buffer->max  += 32;

        // Clean the new bytes.
        memset(&buffer->bits[buffer->occupancy / 8 + min(1, buffer->occupancy % 8)], 0, 4);
    }

    // Append nbits from the integer bits to the buffer one octet at a time.
    while (nbits) {
        uint8_t *current = &buffer->bits[buffer->occupancy / 8];    // Pointer to the last modified octet.
        uint8_t  avail   = 8 - (buffer->occupancy % 8);             // How many bits in this octet we can modify.

        // There are no spare bits in this byte, move to the next one (value should be zero).
        if (avail == 0) ++current, avail = 8;

        // Get the current value of the final bit (unused bits are zero up to
        // the next octet boundary). The bits are stored from least to most
        // significant, like this:
        //
        // 0b00000000 //             occupancy = 0
        // 0b00000001 // append   1, occupancy = 1
        // 0b00001101 // append 101, occupancy = 4
        //

        // To append these bits, we shift them left however many we need, then
        // or in as many of the new bits as we can, starting with the most
        // significant.
        *current <<= min(nbits, avail);
        *current  |= (bits >> (nbits - min(nbits, avail)));

        // Update records accordingly.
        buffer->occupancy += min(nbits, avail);

        // Remove the saved bits from input by masking them out.
        bits   &= (~0U >> (32 - nbits + min(nbits, avail)));
        nbits  -= min(nbits, avail);
    }

    // Success.
    return true;
}

// Return the number of bits currently recorded.
size_t bitbuf_numbits(bitbuf_t *buffer)
{
    return buffer->occupancy;
}

// Fetch a pointer to the octet aligned and padded bit buffer, only valid until
// you modify it (e.g. append).
bool bitbuf_getbits(bitbuf_t *buffer, const uint8_t **bits, uint32_t *count)
{
    if (bits)   *bits   = buffer->bits;
    if (count)  *count  = buffer->occupancy / 8 + min(1, buffer->occupancy % 8);
    return true;
}
