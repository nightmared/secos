#ifndef __PAGING_H__
#define __PAGING_H__

#include <pagemem.h>
#include <mbi.h>

extern int __x86_kernel_structs_end__, __userland_mapped__;

extern pde32_t *pdt;
extern pte32_t *pt;
extern struct mbi_available_memory available_mem_regions[4];
extern uint8_t nb_available_mem_regions;

void enable_paging();
pte32_t *get_pte_for_addr(pde32_t *cr3, uint32_t addr);

// map kernel shared memory, used by the kernel and processes alike
void setup_shared_pde(pte32_t* pt);

void print_pdt(pde32_t* pdt);
void print_pt(pte32_t *pt);


#endif // __PAGING_H__
