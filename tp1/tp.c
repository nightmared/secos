/* GPLv2 (c) Airbus */
#include <debug.h>
#include <info.h>
#include <segmem.h>
#include <gdt.h>
#include <string.h>

extern info_t *info;

void tp() {
    debug("\nMultiboot memory information:\n");
    print_mbi_memory_headers();

    debug("\nGDT description:\n");

    // RESULT:
    // Empty segment descriptor !?
    // Empty segment descriptor !?
    // 0x0        - 0xfffff000 (ring: 0, present, kind: code)
    // 0x0        - 0xfffff000 (ring: 0, present, kind: data)
    print_gdt();

    init_gdt_flat();

    debug("\nNew GDT description:\n");
    print_gdt();

    char  src[64];
    char  real_dst[64];
    char *dst = 0;

    memset(src, 0xff, 64);

    src[0] = 0x11;
    src[1] = 0x12;
    src[2] = 0x13;
    src[3] = 0x14;

    seg_desc_t seg;
    init_segment(&seg, (uint32_t)&real_dst[0], ((uint32_t)&real_dst[0])+0x20);
    seg.type = SEG_DESC_DATA_RW;
    add_segment_to_gdt(seg);

    //debug("%x\n", *((uint32_t*)real_dst));

    set_es(gdt_seg_sel(3, 0));
    _memcpy8(dst, src, 32);
    set_es(gdt_seg_sel(2, 0));

    //debug("%x\n", *((uint32_t*)real_dst));
}
