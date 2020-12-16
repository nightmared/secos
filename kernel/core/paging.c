#include <paging.h>
#include <debug.h>
#include <cr.h>

pde32_t *pdt = (pde32_t*)&__pdt_start__;
pte32_t *pt = (pte32_t*)&__pt_start__;

struct mbi_available_memory available_mem_regions[4];
uint8_t nb_available_mem_regions = 4;

void print_pt(pte32_t *pt) {
    printf("\nPT description:\n");
    for (int i = 0; i < 1024; i++) {
        if ((pt+i)->p) {
            printf("%d: %08p (user: %d, writable: %d)\n", i, (pt+i)->addr<<12, (pt+i)->lvl, (pt+i)->rw);
        }
    }
}

void print_pdt(pde32_t *pdt) {
    printf("\nPDT description:\n");
    for (int i = 0; i < 1024; i++) {
        pde32_t* pde = pdt+i;
        if (pde->p) {
            printf("%d: %08p (user: %d, writable: %d)\n", i, pde->addr<<12, pde->lvl, pde->rw);
        }
    }
}

void setup_identity_pdt() {
    memset(pdt, 0, PG_4K_SIZE);
    memset(pt, 0, PG_4M_SIZE);

    for (int i = 0; i < 1024; i++) {
        pde32_t *cur_pde = pdt + i;

        pte32_t *matching_pt = pt+1024*i;

        // map a PT per PDE
        cur_pde->raw = (uint32_t)matching_pt;
        cur_pde->p = 0;
        cur_pde->rw = 1;

        for (int j = 0; j < 1024; j++) {
            pte32_t *cur_pte = matching_pt+j;

            // identity memory mapping
            cur_pte->raw = (uint32_t)((i<<PG_4M_SHIFT)|(j<<PG_4K_SHIFT));
            cur_pte->p = 0;
            cur_pte->rw = 1;
        }
    }

    uint32_t end = (uint32_t)&__x86_kernel_structs_end__;

    // map all pages between 0x100000 & &__x86_kernel_structs_end__ (we could be more precise and map between 0x100000 & &__kernel_end__, and between &__x86_kernel_structs_start__ & &__x86_kernel_structs_end__, but we don't really care for a few pages)
    for (uint32_t i = 0; i <= pd32_idx(end); i++) {
        for (int j = 0; j < 1024; j++) {
            uint32_t base_addr = (uint32_t)((i<<PG_4M_SHIFT)|(j<<PG_4K_SHIFT));
            if (base_addr >= 0x100000 && base_addr <= end) {
                (pdt+i)->p = 1;
                (pt+1024*i+j)->p = 1;
            }
        }
    }

    // setup the memory mapping shared with userspace
    (pdt+pd32_idx(0xc0000000))->p = 1;
    setup_shared_pde((pte32_t*)((pdt+pd32_idx(0xc0000000))->addr<<PG_4K_SHIFT));
}

pte32_t *get_pte_for_addr(pde32_t *cr3, uint32_t addr) {
    pde32_t *pde = cr3+pd32_idx(addr);
    if (pde->p == 0) {
        return NULL;
    }
    return (pte32_t*)(pde->addr<<PG_4K_SHIFT)+pt32_idx(addr);
}

void setup_shared_pde(pte32_t* pt) {
    uint32_t base_addr = (uint32_t)&__userland_mapped__;
    uint32_t size = (uint32_t)&__x86_kernel_structs_end__-base_addr;
    uint32_t nb_pages = size>>PG_4K_SHIFT;
    if (size & ((1<<PG_4K_SHIFT)-1)) {
        nb_pages++;
    }

    // we assume the shared data does not exced the size of a PDE (4MB)
    for (uint32_t i = 0; i < nb_pages; i++) {
        (pt+i)->addr = (base_addr>>PG_4K_SHIFT)+i;
        (pt+i)->rw = 0;
        (pt+i)->p = 1;
        (pt+i)->lvl = 1;
    }
}

void enable_paging() {
    // retrieve available memory information
    mbi_get_available_memory_description(&nb_available_mem_regions, available_mem_regions);

    setup_identity_pdt();

    set_cr3(pdt);

    cr0_reg_t cr0 = (cr0_reg_t)(raw32_t)get_cr0();
    cr0.pg = 1;
    set_cr0(cr0);
}

