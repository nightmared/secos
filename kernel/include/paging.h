#ifndef __PAGING_H__
#define __PAGING_H__

#include <pagemem.h>

extern pde32_t *pdt;
extern pte32_t *pt;

void enable_paging();
pte32_t *get_pte_for_addr(uint32_t addr);

void print_pdt();
void print_pde(uint16_t idx);


#endif // __PAGING_H__
