RarVM Toolchain
===============================================================================

This is a basic toolchain for the RarVM, a virtual machine included with the
popular WinRAR compression suite. Rar includes a VM to support custom data
transformations to improve data redundancy, and thus improve compression
ratios.  However, it also represents a widely deployed machine architecture
about which very little is known...that is just too tempting a target for
exploration to ignore :-)

Currently two basic tools are available for experimentation, a linker and an
assembler. A dissassembler will be available soon, and perhaps eventually a
compiler (in the form of a llvm backend or gcc target).

A related blog post is available here http://blog.cmpxchg8b.com/2012/09/fun-with-constrained-programming.html

Usage
===============================================================================

There are five types of files referenced in this document, they are explained below.

 * .rs     : A RarVM assembly program.
 * .rh     : A RarVM header file.
 * .ro     : A RarVM object file.
 * .ri     : A CPP preprocessed RarVM assembly file, i.e. `cpp < input.rs > input.ri`
 * .rar    : A RarVM program.

Recipes for GNU Make might look like this:

    %.ri: %.rs
        $(CPP) < $< > $@

    %.ro: %.ri
        $(RARAS) -o $@ $<

    %.rar: %.ro
        $(RARLD) $< > $@

You can use C macros and includes if you wish, the assembler understands cpp
directives and ignores them, so simply pipe your program through cpp before
assembling it, producing a .ri file.

Architecture
===============================================================================

Familiarity with x86 (and preferably intel assembly syntax) would be an advantage.

RarVM has 8 named registers, called r0 to r7. r7 is used as a stack pointer for
stack related operations (such as push, call, pop, etc). However, as on x86,
there are no restrictions on setting r7 to whatever you like, although if you
do something stack related it will be masked to fit within the address space
for the duration of that operation.

A RarVM program may execute for at most 250,000,000 instructions, at which
point it will be terminated abnormally. However there are no limits on the
number of programs included in a file, so you must simply split your task into
multiple 250M cycle chunks. If the instruction pointer ever moves
outside the defined code segment, the program is considered to have completed
successfully, and is terminated normally.

Your program will execute with a 0x40000 byte address space, and has access to
3 status flags : ZF (Zero), CF (Carry) and SF (Sign) which can be accessed via
the conditional branch instructions, or via pushf/popf (as on x86).

Operands.
-------------------------------------------------------------------------------

The operand types supported for RarVM are as follows

    * Registers (`r0, r1, ..., r7`)
    * Memory references (`[#0x12345]`)
    * Register indirect references (`[r0]`)
    * Base + Index indirect references (`[r4+#0x1234]`)
    * Immediates (`#0x12312`)

And as a convenience to programmers, the assembler also supports symbolic
references that are resolved to immediates at compile time, simply prefix them
with $.

    * Symbolic references (`jmp $next_loop`)

So a trivial demonstration program might help.

---

    #include <constants.rh>
    #include <crctools.rh>
    #include <math.rh>
    #include <util.rh>
    ; vim: syntax=fasm


    ; Test RAR assembly file that just demonstrates the syntax.
    ;
    ; Usage:
    ;
    ;   $ unrar p -inul helloworld.rar
    ;   Hello, World!
    ;

    _start:
        ; Install our message in the output buffer
        mov     r3, #0x1000                 ; Output buffer.
        mov     [r3+#0], #0x6c6c6548        ; 'lleH'
        mov     [r3+#4], #0x57202c6f        ; 'W ,o'
        mov     [r3+#8], #0x646c726f        ; 'dlro'
        mov     [r3+#12], #0x00000a21       ; '!\n'
        mov     [VMADDR_NEWBLOCKPOS],  r3   ; Pointer
        mov     [VMADDR_NEWBLOCKSIZE], #14  ; Size
        call    $_success

---

Rar programs can either supply their own initial register values, or use the
default set which includes some system information. If you do not specify your
own registers, these will be set:

   * r0: zero
   * r1: zero
   * r2: zero
   * r3: Address of Global Memory Buffer
   * r4: Filter Block Length
   * r5: Filter Exec Count
   * r6: zero
   * r7: End of available memory (and thus the stack grows down on RarVM).

Flags are always initialised to zero.

Currently I havn't invented a syntax for describing your own initial registers,
so you're stuck with these. Maybe a section called .regs?

    section .regs
        dd 0
        dd 0
        dd 0

Then perhaps a .data section for encoding the leading data bytes rar supports.
If you want to produce output, you must write a pointer to your data into the
global memory section, and record the size. See the helloworld.rs in the test
directory for an example.

In general, symbols starting with '`_`' are defined or reserved by the system for
special use in the standard library or internally. Symbols starting with '`__`'
are used internally and should not be called by you.

The magic symbol "`_start`" is important, and is considered the entrypoint for
your program, so you must define it (Rar has no concept of entrypoint, I
emulate it with an implicit jump at startup).

Stdlib
-------------------------------------------------------------------------------

A collection of useful routines is included in the stdlib directory, you can
include these files and call the routines they define. For example, there is no
way to get the remainder of a division operation in rar, so a software
approximation is defined in math.rh:

    #include <math.rh>

    push    #2
    push    #123
    call    $_mod

Calling conventions.

I use the following conventions, feel free to ignore them if you prefer
something hideous like pascal.

    * No registers are saved by callee, except r6 and r7.
    * The return code is placed in r0.
    * Parameters are passed cdecl style (reverse order) on the stack.
    * r6 is used as a frame pointer, and you're required to preserve it.

See the standard library for examples.

Known Bugs
===============================================================================

There are several known bugs in the RarVM.

`[redacted as some have security consequences]`

Status
===============================================================================

This is a list of what currently is believed to work:

    * Basic programs appear to run on the Linux unrar program.
    * I havn't tested Windows yet, because I'm still porting the CRC tools to RarVM.

Needless to say, don't pipe untrusted input into this early alpha build ;-)

FAQ
===============================================================================

Q. Can I produce output from my program?

A. Yes, but the answer is complicated.

   Recall that the RAR format requires a checksum for the output in order for
   it to successfully process your program, so you must pad your output to this
   CRC in order to complete. A template has been included to demonstrate
   this technique at runtime based on CRC tools by Julien Tinnes.

   This requires crc padding to occur at exit, which involves adding a 32bit
   value to the output at a specific location.

   Yes, this is complicated, but you're programming WinRAR, what did you
   expect? The ridiculous constraints are what makes it interesting ;-)

Q. Can I supply input to my program?

A. All input must be included in the archive, there is no way to provide
   external input at runtime.

Q. Can I write self-modifying Code?

A. I don't think so, you can think of the code as occupying a different x86
   segment from your data, so is not addressable. So, in x86 terms, you might
   say ds = ss, but cs != ds.

Q. Can I embed data within code?

A. You could, using opaque predicates, but as you cannot read from the code
   segment, I don't think it would be useful.

Q. How do I force the use of smaller integer encodings to save space?

A. You send me a patch to do that.

Q. How do I set the "Init Registers" option rar supports?

A. You send me a patch to do that.

Q. How do I add data to the "InitData"?

A. You send me a patch to do that.

Q. Whats the deal with the different branch types?

A. I'll add support when I can.

Q. What does "print" do?

A. I have no idea! Does it do something on Windows? (please test and report)

