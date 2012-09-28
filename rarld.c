// Linker for RAR object files.
// Copyright (C) 2012 Tavis Ormandy <taviso@cmpxchg8b.com>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <err.h>
#include <zlib.h>

#include "bitbuffer.h"

#pragma pack(1)

const uint8_t kRarSignature[] = { 0x52, 0x61, 0x72, 0x21, 0x1a, 0x07, 0x00 };

enum {
    TYPE_MARK       = 0x72,
    TYPE_MAIN       = 0x73,
    TYPE_FILE       = 0x74,
    TYPE_COMM       = 0x75,
    TYPE_AV         = 0x76,
    TYPE_SUB        = 0x77,
    TYPE_PROTECT    = 0x78,
    TYPE_SIGN       = 0x79,
    TYPE_NEWSUB     = 0x7A,
    TYPE_ENDARC     = 0x7B,
};

enum {
    HOST_MSDOS      = 0x00,
    HOST_OS2        = 0x01,
    HOST_WIN32      = 0x02,
    HOST_UNIX       = 0x03,
    HOST_MACOS      = 0x04,
    HOST_BEOS       = 0x05,
};

typedef struct {
    uint16_t    crc;
    uint8_t     type;
    uint16_t    flags;
    uint16_t    size;
} hdr_t;

struct mainhdr {
    hdr_t       hdr;
    uint16_t    HighPosAv;
    uint32_t    PosAv;
    uint8_t     EncryptVer;
};

struct filehdr {
    hdr_t       hdr;
    uint32_t    PackSize;
    uint32_t    UnpSize;
    uint8_t     HostOS;
    uint32_t    FileCRC;
    uint32_t    FileTime;
    uint8_t     UnpVer;
    uint8_t     Method;
    uint16_t    NameSize;
    union {
        uint32_t    FileAttr;
        uint32_t    SubFlags;
    };
    uint8_t     FileName[6];
};


int main(int argc, char **argv)
{
    FILE     *input;
    FILE     *output;
    bitbuf_t *tables;

    struct mainhdr mainhdr = {
        .hdr = {
            .crc    = 0x0000,
            .type   = TYPE_MAIN,
            .flags  = 0x0000,
            .size   = sizeof mainhdr,
        },
        .HighPosAv  = 0,
        .PosAv      = 0,
    };

    struct filehdr filehdr = {
        .hdr = {
            .crc    = 0x0000,
            .type   = TYPE_FILE,
            .flags  = 0x0000,
            .size   = sizeof filehdr,
        },
        .UnpSize    = 0,
        .HostOS     = HOST_WIN32,
        .FileCRC    = ~0xDEADBEEF,
        .FileTime   = 0,
        .UnpVer     = 0x1D,
        .Method     = 0,
        .NameSize   = 6,
        {
            .FileAttr   = 0x20000000,
        },
        .FileName   = { 's', 't', 'd', 'o', 'u', 't' },
    };

    // Read the input object file
    if (!(input = fopen(argv[1], "r"))) {
        errx(EXIT_FAILURE, "failed to open specified rar object file, %s", argv[1]);
    }

    bitbuf_create(&tables);

    // Packed Block Header
    bitbuf_append(tables, 0, 1);    // PpmBlock
    bitbuf_append(tables, 0, 1);    // KeepTables

    // We need 20 length codes that are used when building the decode tables. I
    // should simplify this. I don't think I can use just zeroes, but if I can,
    // could just use two runs of 10 zeroes.
    bitbuf_append(tables, 8, 4);
    bitbuf_append(tables, 0, 4);
    bitbuf_append(tables, 8, 4);
    bitbuf_append(tables, 5, 4);
    bitbuf_append(tables, 5, 4);
    bitbuf_append(tables, 4, 4);
    bitbuf_append(tables, 3, 4);
    bitbuf_append(tables, 3, 4);
    bitbuf_append(tables, 3, 4);
    bitbuf_append(tables, 4, 4);
    bitbuf_append(tables, 4, 4);
    bitbuf_append(tables, 3, 4);
    bitbuf_append(tables, 3, 4);
    bitbuf_append(tables, 6, 4);
    bitbuf_append(tables, 6, 4);
    bitbuf_append(tables, 0, 4);
    bitbuf_append(tables, 4, 4);
    bitbuf_append(tables, 6, 4);
    bitbuf_append(tables, 0, 4);
    bitbuf_append(tables, 7, 4);
    bitbuf_append(tables, 2, 4);

    // Flush to an octet boundary.
    bitbuf_append(tables, 0, 2);

    for (int i = 0; i < 69; i++) {
        // TODO: Simplify this, use RLE zeroes.
        const uint8_t huffman_table[] = {
            0x30, 0x1D, 0x6B, 0xE0, 0x17, 0x69, 0x2B, 0xF8, // 8
            0x2A, 0xF4, 0x2B, 0xD4, 0xAF, 0xE8, 0x7A, 0x32, // 16
            0xF4, 0xE6, 0x65, 0xE6, 0x5E, 0x6C, 0x30, 0xE6, // 24
            0xD1, 0x24, 0xF2, 0x00, 0x3D, 0x93, 0xC9, 0xE6, // 32
            0x89, 0xEC, 0x9E, 0xC9, 0xEB, 0x30, 0xB8, 0xE4, // 40
            0x7A, 0x73, 0x1E, 0xBD, 0x6B, 0x61, 0xDD, 0xEB, // 48
            0x34, 0x30, 0xBB, 0x76, 0xA8, 0x30, 0x30, 0xF0, // 56
            0x30, 0x4B, 0x30, 0x00, 0x30, 0x00, 0x30, 0x3E, // 64
            0x0F, 0xF7, 0x7F, 0x30, 0xF9,
        };

        bitbuf_append(tables, huffman_table[i], 8);
    }

    bitbuf_append(tables, 0xDC, 8);
    bitbuf_append(tables, 0x9D, 8);
    bitbuf_append(tables, 0xA1, 8);     // BlockStart?
    bitbuf_append(tables, 0x1,  4);

    enum {
        VM_NEWFILTER    = 0x8, // Defining a new filter
        VM_BLOCKSTARTHI = 0x4, // Add 258 to BlockSize
        VM_BLOCKSIZE    = 0x2, // Change Blocksize
        VM_INITREGS     = 0x1, // Initial Register State Follows
    };

    enum {
        VMTYPE_UINT4    = 0b00,
        VMTYPE_UINT16   = 0b10,
        VMTYPE_UINT32   = 0b11,
    };

    bitbuf_append(tables, 0, 4); // The first byte, contains VM flags.
    bitbuf_append(tables, 7, 4);

    // Now we need to know the size of the input file, so seek to the end.
    fseek(input, 0, SEEK_END);
    bitbuf_append(tables, ftell(input) + 9, 16);    // Literal Size, including vm parameters.
    // XXX: NEED TO ACCOUNT FOR INIT DATA

    // VM DATA STARTS HERE
    bitbuf_append(tables, VMTYPE_UINT32, 2);
    bitbuf_append(tables, 0x00000000, 32);           // Block Start Address
    bitbuf_append(tables, VMTYPE_UINT32, 2);
    bitbuf_append(tables, ftell(input), 32);         // Literal Size of code

    // Because we may not be on an octet boundary, we need to use the bitbuf
    // interface to add this code, so rewind and add each byte.
    fseek(input, 0, SEEK_SET);

    for (int i = fgetc(input); i != EOF; i = fgetc(input)) {
        bitbuf_append(tables, i, 8);
    }

    bitbuf_getbits(tables, NULL, &filehdr.PackSize);

    // Fixup checksums
    mainhdr.hdr.crc = crc32(0, &(mainhdr.hdr.type), sizeof(mainhdr) - offsetof(struct mainhdr, hdr.type));
    filehdr.hdr.crc = crc32(0, &(filehdr.hdr.type), sizeof(filehdr) - offsetof(struct filehdr, hdr.type));

    fwrite(kRarSignature, sizeof kRarSignature, 1, stdout);
    fwrite(&mainhdr, sizeof mainhdr, 1, stdout);
    fwrite(&filehdr, sizeof filehdr, 1, stdout);
    fwrite(tables->bits, filehdr.PackSize, 1, stdout);
    bitbuf_destroy(tables);
}
