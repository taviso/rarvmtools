#ifndef __BITBUFFER_H
#define __BITBUFFER_H

typedef struct {
    size_t   occupancy;
    size_t   max;
    uint8_t *bits;
} bitbuf_t;

bool bitbuf_create(bitbuf_t **buffer);
bool bitbuf_destroy(bitbuf_t *buffer);
bool bitbuf_append(bitbuf_t *buffer, uint32_t bits, uint8_t nbits);
bool bitbuf_getbits(bitbuf_t *buffer, const uint8_t **, uint32_t *count);
size_t bitbuf_numbits(bitbuf_t *buffer);
#endif
