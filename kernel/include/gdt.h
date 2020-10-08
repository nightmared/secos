#ifndef __GDT_H__
#define __GDT_H__

#include <segmem.h>
#include <string.h>

void print_gdt();
void reload_segment_selectors();
void update_gdtr();
void init_segment(seg_desc_t *desc, uint32_t base, uint64_t end);
void init_gdt_flat();
void add_segment_to_gdt(seg_desc_t seg);

#endif // __GDT_H__
