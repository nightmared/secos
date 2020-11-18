/* GPLv2 (c) Airbus */
#include <debug.h>
#include <info.h>
#include <pagemem.h>
#include <gdt.h>
#include <string.h>
#include <cr.h>

extern info_t *info;

pde32_t *pdt = (pde32_t*)&__pdt_start__;
pte32_t *pt = (pte32_t*)&__pt_start__;

void print_pde(uint16_t idx) {
    if (idx >= 1024) {
        printf("invalid PDT query: %d index too high\n", idx);
    }

    pde32_t *pde = pdt+idx;
    printf("%d: %p (user: %d, present: %d, writable: %d)\n", idx, pde->addr<<20, pde->lvl, pde->p, pde->rw);
}

void print_pdt() {
    printf("PDT description:\n");
    for (int i = 0; i < 1024; i++) {
        print_pde(i);
    }
}

void setup_pdt() {
    for (int i = 0; i < 1024; i++) {
        pde32_t *cur_pde = pdt + i;

        pte32_t *matching_pt = pt+1024*i;

        // map a PT per PDE
        cur_pde->raw = (uint32_t)matching_pt;
        cur_pde->p = 1;
        cur_pde->rw = 1;

        for (int j = 0; j < 1024; j++) {
            pte32_t *cur_pte = matching_pt+j;

            // linear memory mapping
            cur_pte->raw = (uint32_t)((i<<PG_4M_SHIFT)|(j<<PG_4K_SHIFT));
            cur_pte->p = 1;
            cur_pte->rw = 1;
        }
    }
}

pte32_t *get_pte_for_addr(uint32_t addr) {
    return pt+pd32_idx(addr)*1024+pt32_idx(addr);
}

void enable_paging() {
    setup_pdt();

    set_cr3(pdt);
    //printf("cr3 = %p\n", get_cr3());

    cr0_reg_t cr0 = (cr0_reg_t)(raw32_t)get_cr0();
    cr0.pg = 1;
    set_cr0(cr0);
}

void tp() {
    printf("\nMultiboot memory information:\n");
    print_mbi_memory_headers();

    init_gdt_flat();
    printf("\nGDT description:\n");
    print_gdt();

    // enable interrupts
    asm volatile("sti");

    enable_paging();



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
}
