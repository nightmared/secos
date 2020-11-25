/* GPLv2 (c) Airbus */
#include <debug.h>
#include <info.h>
#include <pagemem.h>
#include <gdt.h>
#include <paging.h>
#include <string.h>
#include <cr.h>

extern info_t *info;
void tp() {
    printf("\nMultiboot memory information:\n");
    print_mbi_memory_headers();

    init_gdt_flat();
    printf("\nGDT description:\n");
    print_gdt();

    // enable interrupts
    asm volatile("sti");

    enable_paging();

    /*
        // map the PD to 0xc0000000
        pte32_t *pte = get_pte_for_addr((uint32_t)0xc0000000);
        pte->addr = (uint32_t)pdt>>12;
        pdt = (pde32_t*)0xc0000000;
        print_pdt();
        printf("%p\n", *pdt);

        // map 0x700000 and 0x7ff000 to 0x2000
        uint32_t addr = 0x700000;
        pte = get_pte_for_addr((uint32_t)addr);
        pte->addr = 0x2000>>12;
        invalidate(addr);
        addr = 0x7ff000;
        pte = get_pte_for_addr((uint32_t)addr);
        pte->addr = 0x2000>>12;
        invalidate(addr);
        memcpy((void*)0x2000, "coucou TLS-SEC !", 17);
        printf("%s\n", (char*)0x700000);
        printf("%s\n", (char*)0x7ff000);

        // trigger a triple-fault
        pdt->p = 0;
        invalidate(pdt);
    */
}
