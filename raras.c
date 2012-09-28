// Simple two-pass assembler for RarVM.
// Copyright (C) 2012 Tavis Ormandy <taviso@cmpxchg8b.com>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//

#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <getopt.h>
#include <err.h>

#include "bitbuffer.h"
#include "rar.h"

int main(int argc, char **argv)
{
    FILE     *input       = NULL;
    FILE     *output      = NULL;
    char     *line        = NULL;
    size_t    address     = 1;
    size_t    size        = 0;
    label_t  *symtab      = NULL;
    size_t    symtab_size = 0;
    uint8_t   checkbyte   = 0;
    int       opt;
    uint32_t  codesize;
    bitbuf_t *text;
    const uint8_t *code;

    // Parse commandline arguments.
    while ((opt = getopt(argc, argv, "o:")) != -1) {
        switch (opt) {
            case 'o':
                if (!(output = fopen(optarg, "w"))) {
                    errx(EXIT_FAILURE, "failed to open output file %s", optarg);
                }
        }
    }

    // Open the input file, the last argument specified.
    input = fopen(argv[optind], "r");

    // Verify we have enough input to continue.
    if (!input) {
        err(EXIT_FAILURE, "failed to open input file %s", argv[optind]);
    }

    if (!output) {
        errx(EXIT_FAILURE, "no output file specified");
    }

    // First pass, locate all labels and record their address.
    while (getline(&line, &size, input) > 0) {
        char *comment   = strchrnul(line, ';');
        char *label     = strchrnul(line, ':');
        char *newline   = strchrnul(line, '\n');

        // Remove any comment, label symbol, or newline.
        *comment = '\0';
        *label   = '\0';
        *newline = '\0';

        // Skip if this is a cpp or blank line.
        if (*line == '#' || *line == '\0') {
            continue;
        }

        // If there is a ':' character before any comment, then this is a label.
        if (label < comment) {
            // Now line is the nul terminated label.
            symtab = realloc(symtab, sizeof(label_t) * ++symtab_size);

            symtab[symtab_size - 1].flags   = 0;
            symtab[symtab_size - 1].address = address;

            strncpy(symtab[symtab_size - 1].symbol,
                    line,
                    sizeof(symtab[symtab_size - 1].symbol) - 1);

            // Labels do not occupy space, so no need to assemble.
            continue;
        }

        // We dont actually use or save the result, because we havn't built our
        // symtab yet, that will wait until pass 2.
        if (rar_assemble_line(line, NULL, NULL, 0)) {
            // The instruction seems valid, so we need to increment address.
            address++;
        }
    }

    // Rewind input file.
    rewind(input);

    // Reset address.
    address = 0;

    // Allocate a bitbuffer for program text.
    bitbuf_create(&text);

    // No data present (XXX: SUPPORT DB/DD/DW ETC)
    bitbuf_append(text, 0, 1);  // DataFlag

    // Inject a jmp to entrypoint.
    rar_assemble_line("jmp $_start", text, symtab, symtab_size);

    // Second pass, now assemble and generate output.
    while (getline(&line, &size, input) > 0) {
        char *comment   = strchrnul(line, ';');
        char *label     = strchrnul(line, ':');
        char *newline   = strchrnul(line, '\n');

        // Remove any comment or label symbol.
        *comment = '\0';
        *label   = '\0';
        *newline = '\0';

        // Skip if this is a CPP or blank line
        if (*line == '#' || *line == '\0') {
            continue;
        }

        // If there is a ':' character before any comment, then this is a label.
        if (label < comment) {
            continue;
        }

        // Now we assemble the line and append to out bitbuffer.
        rar_assemble_line(line, text, symtab, symtab_size);
    }

    // Fetch the results.
    bitbuf_getbits(text, &code, &codesize);

    // Calculate the check byte.
    for (int i = 0; i < codesize; i++)
        checkbyte ^= code[i];

    fwrite(&checkbyte, 1, 1, output);
    fwrite(code, 1, codesize, output);
    free(line);

    fclose(output);
    fclose(input);
    return 0;
}
