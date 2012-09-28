#ifndef __RAR_H
#define __RAR_H


typedef struct {
    char        symbol[128];
    uint32_t    flags;
    uint32_t    address;
} label_t;


bool rar_assemble_line(const char *line, bitbuf_t *output, label_t *symtab, size_t numsyms);


#endif
