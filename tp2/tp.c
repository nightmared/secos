/* GPLv2 (c) Airbus */
#include <segmem.h>
#include <debug.h>
#include <intr.h>
#include <mbi.h>
#include <gdt.h>

struct handler_info {
   raw32_t         eip;
   raw32_t         cs;
   eflags_reg_t    eflags;
};

void __attribute__((naked)) bp_handler() {
    struct handler_info *args;
    asm volatile(
            "push %%ebp \n\t"
            "mov %%esp, %%ebp \n\t"
            "pusha \n\t"
            "lea 4(%%ebp), %0" : "=a"(args));

    debug("Breakpoint Handler called!\n");
    printf("args=%p, eip=0x%x, cs=0x%x, eflags=0x%x\n", args, args->eip, args->cs, args->eflags);

    asm volatile(
        "popa \n\t"
        "leave \n\t"
        "iret"
    );
}

void bp_trigger() {
    asm volatile("int3");
}

void tp() {
    printf("\nMultiboot memory information:\n");
    print_mbi_memory_headers();

    init_gdt_flat();
    printf("\nGDT description:\n");
    print_gdt();

    // enable interrupts
    asm volatile("sti");

    // let's spend some time spinning
    for (size_t i = 0; i <1e7; i++) {
        asm volatile("");
    }

    idt_reg_t idtr;
    get_idtr(idtr);
    debug("IDT located at 0x%llx\n", idtr.desc); 

    // overwrite the fourth entry of the IDT (#BP)
    int_desc(idtr.desc+3, gdt_krn_seg_sel(gdt_code_idx), (unsigned int)bp_handler);

    bp_trigger();
}
