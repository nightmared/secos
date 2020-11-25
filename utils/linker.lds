/* GPLv2 (c) Airbus */
OUTPUT_FORMAT("elf32-i386","elf32-i386","elf32-i386");
OUTPUT_ARCH("i386")

ENTRY(entry)

PHDRS
{
   phboot  PT_LOAD FLAGS (7);
   phstack PT_LOAD FLAGS (6);
   phrw    PT_LOAD FLAGS (6);
   phsetup PT_LOAD FLAGS (7);
}

SECTIONS
{
   . = 0x100000;
   .mbh      : { KEEP(*(.mbh)) . = ALIGN(4);     } : phboot
   .stack    : { KEEP(*(.stack))                 } : phstack

   __kernel_start__ = .;

   .idt_jmp  : { KEEP(*(.idt_jmp))               } : phsetup
   .text     : { *(.text)                        } : phsetup
   .rodata   : { *(.rodata)                      } : phsetup
   .data     : { *(.data)                        } : phsetup
   .bss      : { *(.bss COMMON)                  } : phsetup
   /DISCARD/ : { *(.note* .indent .comment)      } : phsetup
   
   __kernel_end__ = .;

   . = 0x200000;
   __x86_kernel_structs_start__ = .;
   __pdt_start__ = .;
   .pdt      : { KEEP(*(.pdt))                   } : phrw
   __pt_start__ = .;
   .pt       : { KEEP(*(.pt))                    } : phrw
   __gdt_start__ = .;
   .gdt      : { KEEP(*(.gdt))                   } : phrw
   __tss_start__ = .;
   .tss      : { KEEP(*(.tss))                   } : phrw
   __x86_kernel_structs_end__ = .;
}
