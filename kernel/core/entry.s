/* GPLv2 (c) Airbus */
.section .stack, "aw", @nobits
.align 16
.space 0x2000

.section .gdt, "aw", @nobits
.align 8
.space 0x10000

.section .tss, "aw", @nobits
.align 8
.space 0x2500

.section .pdt, "aw", @nobits
.align 0x1000
.space 0x1000

.section .pt, "aw", @nobits
.align 0x1000
.space 0x400000

.text
.globl entry
.type  entry,"function"

entry:
        cli
        movl    $__kernel_start__, %esp
        pushl   $0
        popf
        movl    %ebx, %eax
        call    start

halt:
        hlt
        jmp     halt
