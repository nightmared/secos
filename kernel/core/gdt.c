#include <gdt.h>
#include <debug.h>

extern int __gdt_start__;
seg_desc_t *mygdt = (seg_desc_t*)&__gdt_start__;

static uint16_t gdt_size = 0;

void print_gdt() {
    gdt_reg_t gdtr;
    get_gdtr(gdtr);

    // skip over the first entry
    seg_desc_t *desc = gdtr.desc;

    while ((uint32_t)desc < gdtr.addr + gdtr.limit) {
        uintptr_t base_addr = (uintptr_t)GET_BASE_FROM_SEG_DESCR(desc);
        uintptr_t limit = (uintptr_t)GET_LIMIT_FROM_SEG_DESCR(desc);
        uint64_t multiplier = 1;
        if (desc->g) {
            multiplier = 1<<12;
        }

        if (desc->raw == 0) {
            debug("Empty segment descriptor\n");
        } else {
            char *presence = "present";
            if (!desc->p) {
                presence = "absent";
            }
            char *kind;
            if (desc->s) {
                if (desc->type < SEG_DESC_CODE_X) { 
                    kind = "data";
                } else {
                    kind = "code";
                }
            } else {
                kind = "system";
            }
            debug("0x%-8x - 0x%-8llx (ring: %d, %s, kind: %s)\n", base_addr, base_addr+multiplier*limit, desc->dpl, presence, kind);
        }

        desc++;
    }
}

void reload_segment_selectors() {
    set_cs(gdt_seg_sel(1, 0));
    set_ss(gdt_seg_sel(2, 0));
    set_ds(gdt_seg_sel(2, 0));
    set_es(gdt_seg_sel(2, 0));
    set_fs(gdt_seg_sel(2, 0));
    set_gs(gdt_seg_sel(2, 0));
}

void update_gdtr() {
    gdt_reg_t new_gdtr;
    new_gdtr.limit = gdt_size*sizeof(seg_desc_t)-1;
    new_gdtr.desc = mygdt;
    set_gdtr(new_gdtr);
    reload_segment_selectors();
}

void init_segment(seg_desc_t *desc, uint32_t base, uint64_t end) {
    desc->raw = 0;

    SET_BASE_FOR_SEG_DESCR(desc, base);

    uint64_t len = end - base;
    if (len > 1<<20) {
        // cannot be encoded with a byte granularity, let's round to the upper page
        uint32_t len_pages = len>>12;
        if (len%(1<<12) != 0) {
            len_pages++;
        }

        SET_LIMIT_FOR_SEG_DESCR(desc, len_pages-1);
        desc->g = 1;
    } else {
        SET_LIMIT_FOR_SEG_DESCR(desc, len-1);
        desc->g = 0;
    }

    // present
    desc->p = 1;
    // code/data
    desc->s = 1;
    // ring 0
    desc->dpl = 0;
    // set length to 32bits
    desc->d = 1;
}

void init_gdt_flat() {
    mygdt = (seg_desc_t*)&__gdt_start__;
    // zero out the table
    memset(mygdt, 0, 1<<13);

    // create flat data & code segments
    init_segment(&mygdt[1], 0, (uint64_t)1<<32);
    mygdt[1].type = SEG_DESC_CODE_XR;
    init_segment(&mygdt[2], 0, (uint64_t)1<<32);
    mygdt[2].type = SEG_DESC_DATA_RW;

    gdt_size = 3;
    update_gdtr();
}

void add_segment_to_gdt(seg_desc_t seg) {
    mygdt[gdt_size++] = seg;
    update_gdtr();
}
