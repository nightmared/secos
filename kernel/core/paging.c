#include <mbi.h>
#include <paging.h>
#include <debug.h>
#include <cr.h>

pde32_t *pdt = (pde32_t*)&__pdt_start__;
pte32_t *pt = (pte32_t*)&__pt_start__;

static struct mbi_available_memory available_mem_regions[4];
static uint8_t nb_available_mem_regions = 4;



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
    // retrieve available memory information
    mbi_get_available_memory_description(&nb_available_mem_regions, available_mem_regions);

    setup_pdt();

    set_cr3(pdt);
    //printf("cr3 = %p\n", get_cr3());

    cr0_reg_t cr0 = (cr0_reg_t)(raw32_t)get_cr0();
    cr0.pg = 1;
    set_cr0(cr0);
}

