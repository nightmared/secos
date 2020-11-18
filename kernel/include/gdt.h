#ifndef __GDT_H__
#define __GDT_H__

#include <segmem.h>
#include <string.h>

// offset wrt. gdt_code_idx and gdt_data_idx to get the corresponding userland (ring3) entrries
#define GDT_RING3_OFFSET 2

// theses objects are (they do not point !) at the start of the respective gdt and tss
extern int __gdt_start__, __tss_start__;
extern uint16_t gdt_size, gdt_code_idx, gdt_data_idx, gdt_tss_idx;

void print_gdt();
void reload_segment_selectors();
void update_gdtr();
void init_segment(seg_desc_t *desc, uint32_t base, uint64_t end);
void init_gdt_flat();
void add_segment_to_gdt(seg_desc_t seg);

#endif // __GDT_H__
