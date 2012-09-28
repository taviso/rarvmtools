
//
// RAR assembly parsing routines.
// Copyright (C) 2012 Tavis Ormandy <taviso@cmpxchg8b.com>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <iconv.h>
#include <string.h>
#include <assert.h>
#include <err.h>

#include "bitbuffer.h"
#include "rar.h"

typedef enum {
    VM_MOV,     VM_CMP,     VM_ADD,     VM_SUB,     VM_JZ,
    VM_JNZ,     VM_INC,     VM_DEC,     VM_JMP,     VM_XOR,
    VM_AND,     VM_OR,      VM_TEST,    VM_JS,      VM_JNS,
    VM_JB,      VM_JBE,     VM_JA,      VM_JAE,     VM_PUSH,
    VM_POP,     VM_CALL,    VM_RET,     VM_NOT,     VM_SHL,
    VM_SHR,     VM_SAR,     VM_NEG,     VM_PUSHA,   VM_POPA,
    VM_PUSHF,   VM_POPF,    VM_MOVZX,   VM_MOVSX,   VM_XCHG,
    VM_MUL,     VM_DIV,     VM_ADC,     VM_SBB,     VM_PRINT,
    VM_MOVB,    VM_MOVD,    VM_CMPB,    VM_CMPD,    VM_ADDB,
    VM_ADDD,    VM_SUBB,    VM_SUBD,    VM_INCB,    VM_INCD,
    VM_DECB,    VM_DECD,    VM_NEGB,    VM_NEGD,
    VM_STANDARD,
} vm_opcode_t;

typedef enum {
    VMCF_OP1        = 1 << 0,
    VMCF_OP2        = 1 << 1,
    VMCF_BYTEMODE   = 1 << 2,
    VMCF_JUMP       = 1 << 3,
    VMCF_PROC       = 1 << 4,
} vm_opflags_t;

static const uint8_t vm_opcode_flags_table[UINT8_MAX] = {
    [VM_ADC]    = VMCF_OP2 | VMCF_BYTEMODE,
    [VM_ADD]    = VMCF_OP2 | VMCF_BYTEMODE,
    [VM_AND]    = VMCF_OP2 | VMCF_BYTEMODE,
    [VM_CALL]   = VMCF_OP1 | VMCF_PROC,
    [VM_CMP]    = VMCF_OP2 | VMCF_BYTEMODE,
    [VM_DEC]    = VMCF_OP1 | VMCF_BYTEMODE,
    [VM_DIV]    = VMCF_OP2 | VMCF_BYTEMODE,
    [VM_INC]    = VMCF_OP1 | VMCF_BYTEMODE,
    [VM_JAE]    = VMCF_OP1 | VMCF_JUMP,
    [VM_JA]     = VMCF_OP1 | VMCF_JUMP,
    [VM_JBE]    = VMCF_OP1 | VMCF_JUMP,
    [VM_JB]     = VMCF_OP1 | VMCF_JUMP,
    [VM_JMP]    = VMCF_OP1 | VMCF_JUMP,
    [VM_JNS]    = VMCF_OP1 | VMCF_JUMP,
    [VM_JNZ]    = VMCF_OP1 | VMCF_JUMP,
    [VM_JS]     = VMCF_OP1 | VMCF_JUMP,
    [VM_JZ]     = VMCF_OP1 | VMCF_JUMP,
    [VM_MOVSX]  = VMCF_OP2,
    [VM_MOV]    = VMCF_OP2 | VMCF_BYTEMODE,
    [VM_MOVZX]  = VMCF_OP2,
    [VM_MUL]    = VMCF_OP2 | VMCF_BYTEMODE,
    [VM_NEG]    = VMCF_OP1 | VMCF_BYTEMODE,
    [VM_NOT]    = VMCF_OP1 | VMCF_BYTEMODE,
    [VM_OR]     = VMCF_OP2 | VMCF_BYTEMODE,
    [VM_POP]    = VMCF_OP1,
    [VM_PUSH]   = VMCF_OP1,
    [VM_RET]    = VMCF_PROC,
    [VM_SAR]    = VMCF_OP2 | VMCF_BYTEMODE,
    [VM_SBB]    = VMCF_OP2 | VMCF_BYTEMODE,
    [VM_SHL]    = VMCF_OP2 | VMCF_BYTEMODE,
    [VM_SHR]    = VMCF_OP2 | VMCF_BYTEMODE,
    [VM_SUB]    = VMCF_OP2 | VMCF_BYTEMODE,
    [VM_TEST]   = VMCF_OP2 | VMCF_BYTEMODE,
    [VM_XCHG]   = VMCF_OP2 | VMCF_BYTEMODE,
    [VM_XOR]    = VMCF_OP2 | VMCF_BYTEMODE,
};

typedef enum {
    REG0,       REG1,       REG2,       REG3,       REG4,
    REG5,       REG6,       REG7,
} vm_reg_t;

static const char * vm_opcode_to_string(uint8_t opcode)
{
    static const char * mnemonics[UINT8_MAX] = {
        [VM_MOV]   = "mov",   [VM_CMP]   = "cmp",   [VM_ADD]   = "add",   [VM_SUB]   = "sub",
        [VM_JZ]    = "jz",    [VM_JNZ]   = "jnz",   [VM_INC]   = "inc",   [VM_DEC]   = "dec",
        [VM_JMP]   = "jmp",   [VM_XOR]   = "xor",   [VM_AND]   = "and",   [VM_OR]    = "or",
        [VM_TEST]  = "test",  [VM_JS]    = "js",    [VM_JNS]   = "jns",   [VM_JB]    = "jb",
        [VM_JBE]   = "jbe",   [VM_JA]    = "ja",    [VM_JAE]   = "jae",   [VM_PUSH]  = "push",
        [VM_POP]   = "pop",   [VM_CALL]  = "call",  [VM_RET]   = "ret",   [VM_NOT]   = "not",
        [VM_SHL]   = "shl",   [VM_SHR]   = "shr",   [VM_SAR]   = "sar",   [VM_NEG]   = "neg",
        [VM_PUSHA] = "pusha", [VM_POPA]  = "popa",  [VM_PUSHF] = "pushf", [VM_POPF]  = "popf",
        [VM_MOVZX] = "movzx", [VM_MOVSX] = "movsx", [VM_XCHG]  = "xchg",  [VM_MUL]   = "mul",
        [VM_DIV]   = "div",   [VM_ADC]   = "adc",   [VM_SBB]   = "sbb",   [VM_PRINT] = "print",
        [VM_MOVB]  = "movb",  [VM_MOVD]  = "movd",  [VM_CMPB]  = "cmpb",  [VM_CMPD]  = "cmpd",
        [VM_ADDB]  = "addb",  [VM_ADDD]  = "addd",  [VM_SUBB]  = "subb",  [VM_SUBD]  = "subd",
        [VM_INCB]  = "incb",  [VM_INCD]  = "incd",  [VM_DECB]  = "decb",  [VM_DECD]  = "decd",
        [VM_NEGB]  = "negb",  [VM_NEGD]  = "negd",
    };

    return mnemonics[opcode] ? mnemonics[opcode] : "bad";
}

static const char * vm_reg_to_string(uint8_t reg)
{
    static const char * names[UINT8_MAX] = {
        [REG0] = "r0",
        [REG1] = "r1",
        [REG2] = "r2",
        [REG3] = "r3",
        [REG4] = "r4",
        [REG5] = "r5",
        [REG6] = "r6",
        [REG7] = "r7",
    };

    return names[reg] ? names[reg] : "bad";
}


static uint8_t vm_string_to_opcode(const char *mnemonic)
{
    unsigned i;

    for (i = 0; i < UINT8_MAX; i++) {
        if (strcmp(vm_opcode_to_string(i), mnemonic) == 0) {
            return i;
        }
    }

    errx(EXIT_FAILURE, "unrecognised mnemonic '%s' encountered", mnemonic);
}

static uint8_t vm_string_to_register(const char *reg)
{
    unsigned i;

    for (i = 0; i < UINT8_MAX; i++) {
        if (strcmp(vm_reg_to_string(i), reg) == 0) {
            return i;
        }
    }

    errx(EXIT_FAILURE, "unrecognised register '%s' encountered", reg);
}

static bool vm_assemble_memory(const char *operand, bitbuf_t *output, label_t *symtab, size_t numsyms)
{
    char index[128]     = {0};
    char base[128]      = {0};
    char modifier[2]    = {0};

    // Parse the memory reference provided.
    if (sscanf(operand, "[%127[^]+-]%1[+-]%127[^]]]", base, modifier, index) >= 1) {

        // Operand type reg/mem
        bitbuf_append(output, 0b01, 2);

        // These are the possibilities RarVM supports:
        //
        //      [#123]         ; base only
        //      [r0]           ; register only
        //      [r0+#123]      ; register base, literal index
        //      [r0-#123]      ; (syntactic sugar, i will just negate it).

        if (*index == '\0') {
            switch (*base) {
                case 'r':   bitbuf_append(output, 0b0, 1);  // Zero Base
                            bitbuf_append(output, vm_string_to_register(base), 3);
                            break;
                case '#':   bitbuf_append(output, 0b1, 1);  // Non-zero base
                            bitbuf_append(output, 0b1, 1);  // Base address only
                            bitbuf_append(output, 0b11, 2);
                            bitbuf_append(output, strtoul(base + 1, NULL, 0), 32);
                            break;
                case '$':   // XXX FIXME
                default:
                    goto error;
                    break;
            }
        } else {
            // I currently only support base register and literal index.
            // FIXME: support symbolic references, this is not trivial as
            //        branch targets are handled differently.
            if (*base != 'r' && *index != '#') {
                goto error;
            }

            bitbuf_append(output, 0b1, 1);                          // Non-zero base
            bitbuf_append(output, 0b0, 1);                          // Base address and index
            bitbuf_append(output, vm_string_to_register(base), 3);  // Encode register
            bitbuf_append(output, 0b11, 2);                         // 32bit literal follows.

            // Handle negative offsets, which makes managing stack frames much
            // friendlier for programmers.
            switch (*modifier) {
                case '+': bitbuf_append(output, +strtoul(index + 1, NULL, 0), 32);
                          break;
                case '-': bitbuf_append(output, -strtoul(index + 1, NULL, 0), 32);
                          break;
                default:
                    goto error;
                    break;
            }
        }
    } else {
        goto error;
    }

    return true;

error:
    errx(EXIT_FAILURE, "unable to parse memory reference %s", operand);
}

static bool vm_assemble_immediate(const char *operand, bitbuf_t *output, bool bytemode)
{
    // Output immediate bits.
    bitbuf_append(output, 0b00, 2);
    //  0b00: 4 bit integer
    //  0b01: 8 or 12 bit integer (??)
    //  0b10: 16 bit integer.
    //  0b11: 32 bit integer.
    bitbuf_append(output, 0b11, 2);
    // Output integer.
    bitbuf_append(output, strtoul(operand, NULL, 0), 32);
    return true;
}

static bool vm_assemble_register(const char *operand, bitbuf_t *output)
{
    // Output register bit.
    bitbuf_append(output, 0b1, 1);

    // Output register number.
    bitbuf_append(output, vm_string_to_register(operand), 3);
    return true;
}

static bool vm_assemble_symbol(const char *operand, bitbuf_t *output, label_t *symtab, size_t numsyms)
{
    for (int i = 0; i < numsyms; i++) {
        if (strcmp(symtab[i].symbol, operand) == 0) {
            // Immediate flag.
            bitbuf_append(output, 0b00, 2);
            // 32bit size.
            bitbuf_append(output, 0b11, 2);
            // Output address.
            // XXX: RarVM uses an unusual encoding format to encode branch targets.
            bitbuf_append(output, symtab[i].address + 256, 32);
            return true;
        }
    }

    // Cannot locate the symbol specified.
    errx(EXIT_FAILURE, "unresolved symbol %s encountered", operand);

    return false;
}

bool rar_assemble_line(const char *line, bitbuf_t *output, label_t *symtab, size_t numsyms)
{
    char opcode[128] = {0};
    char op1[128]    = {0};
    char op2[128]    = {0};

    // Parse out the opcode and the operands.
    if (sscanf(line, "%127s %127[^, \t\n] , %127[^ \t\n]", opcode, op1, op2) >= 1) {
        uint8_t opnum = vm_string_to_opcode(opcode);

        // If this is a test pass, just verify we know opcode.
        if (output == NULL) return true;

        // This is a real pass, we need to encode it. RarVM has two possible
        // opcode encodings, one for 3 bit and one for 6 bit instructions.
        switch (opnum) {
            case 0b000 ... 0b111:
                bitbuf_append(output, 0, 1);          // 1 bit flag
                bitbuf_append(output, opnum, 3);      // 3 bit opcode
                break;
            case 0b1000 ... 0b100111:
                bitbuf_append(output, 1, 1);          // 1 bit flag
                bitbuf_append(output, opnum + 24, 5); // 5 bit opcode
                break;
            default:
                // Opcodes greater than 39 are probably bytemode encodings.
                errx(EXIT_FAILURE, "XXX: %s must be a special instruction, requires more work\n", opcode);
                break;
        }

        if (vm_opcode_flags_table[opnum] & VMCF_BYTEMODE) {
            // FIXME: This is not currently supported.
            bitbuf_append(output, 0, 1);
        }

        // Check if there are operands required.
        if (vm_opcode_flags_table[opnum] & (VMCF_OP1 | VMCF_OP2)) {
            // This instruction requires operands.
            switch (*op1) {
                case '[': // This is a memory location, [r8+1231].
                          vm_assemble_memory(op1, output, symtab, numsyms);
                          break;
                case '#': // This is an immediate, #0x123123, skip over the '#' and assemble.
                          vm_assemble_immediate(op1 + 1, output, false);
                          break;
                case 'r': // This is a register, r4.
                          vm_assemble_register(op1, output);
                          break;
                case '$': // This is a symbol, $_start, skip over the '$' and assemble.
                          vm_assemble_symbol(op1 + 1, output, symtab, numsyms);
                          break;
                default:
                    errx(EXIT_FAILURE, "failed to parse operand 1 to %s instruction, %s", opcode, op1);
            }

            // Certain opcodes require a second operand.
            if (vm_opcode_flags_table[opnum] & VMCF_OP2) {
                switch (*op2) {
                    case '[': // This is a memory location, [r8+1231].
                              vm_assemble_memory(op2, output, symtab, numsyms);
                              break;
                    case '#': // This is an immediate, #0x123123.
                              vm_assemble_immediate(op2 + 1, output, false);
                              break;
                    case 'r': // This is a register, r4.
                              vm_assemble_register(op2, output);
                              break;
                    case '$': // This is a symbol, $_start.
                              vm_assemble_symbol(op2 + 1, output, symtab, numsyms);
                              break;
                    default:
                        errx(EXIT_FAILURE, "failed to parse operand 2 to %s instruction, %s", opcode, op2);
                }
            }
        }
        return true;
    }
    return false;
}
