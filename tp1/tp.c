/* GPLv2 (c) Airbus */
#include <debug.h>
#include <info.h>
#include <segmem.h>

extern info_t *info;

seg_desc_t __attribute__((section(".gdt"),aligned(8))) mygdt[1<<13];

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

void set_gdt_flat() {
    // zero out the table
    for (size_t i = 0; i < sizeof(mygdt)/sizeof(mygdt[0]); i++) {
        mygdt[i].raw = 0;
    }
    for (int i = 1; i < 3; i++) {
        // start at 0x0
        SET_BASE_FOR_SEG_DESCR(mygdt+i, 0x0);
        // limit at 4GiB
        SET_LIMIT_FOR_SEG_DESCR(mygdt+i, (1<<20)-1);
        // present
        mygdt[i].p = 1;
        // code/data
        mygdt[i].s = 1;
        // use pages instead of bytes
        mygdt[i].g = 1;
        // ring 0
        mygdt[i].dpl = 0;
        // set length to 32bits
        mygdt[i].d = 1;
    }
    mygdt[1].type = SEG_DESC_CODE_XRA;
    mygdt[2].type = SEG_DESC_DATA_RWA;

    gdt_reg_t new_gdtr;
    new_gdtr.limit = 3*sizeof(seg_desc_t)-1;
    new_gdtr.desc = &mygdt[0];
    set_gdtr(new_gdtr);
    reload_segment_selectors();
}

void tp() {
    // RESULT:
    // Empty segment descriptor !?
    // 0x0        - 0xfffff000 (ring: 0, present, kind: code)
    // 0x0        - 0xfffff000 (ring: 0, present, kind: data)
    debug("Before:\n");
    print_gdt();

    set_gdt_flat();

    debug("After:\n");
    print_gdt();
}
