#include <gdt.h>
#include <debug.h>

seg_desc_t *mygdt = (seg_desc_t*)&__gdt_start__;

uint16_t gdt_size = 0;
uint16_t gdt_code_idx = 1;
uint16_t gdt_data_idx = 3;
uint16_t gdt_tss_idx = 5;


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

        printf("%02p: ", (uint32_t)desc-(uint32_t)gdtr.desc);
        if (desc->raw == 0) {
            printf("Empty segment descriptor\n");
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
            printf("0x%-8x - 0x%-8llx (ring: %d, %s, kind: %s)\n", base_addr, base_addr+multiplier*(limit+1)-1, desc->dpl, presence, kind);
        }

        desc++;
    }
}

void reload_segment_selectors() {
    set_cs_with_iret(gdt_seg_sel(gdt_code_idx, 0));
    set_ss(gdt_seg_sel(gdt_data_idx, 0));
    set_ds(gdt_seg_sel(gdt_data_idx, 0));
    set_es(gdt_seg_sel(gdt_data_idx, 0));
    set_fs(gdt_seg_sel(gdt_data_idx, 0));
    set_gs(gdt_seg_sel(gdt_data_idx, 0));
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

    // kernel entries
    init_segment(&mygdt[gdt_code_idx], 0, (uint64_t)1<<32);
    mygdt[gdt_code_idx].type = SEG_DESC_CODE_XR;
    init_segment(&mygdt[gdt_data_idx], 0, (uint64_t)1<<32);
    mygdt[gdt_data_idx].type = SEG_DESC_DATA_RW;

    // userland entries
    init_segment(&mygdt[gdt_code_idx+GDT_RING3_OFFSET], 0, (uint64_t)1<<32);
    mygdt[gdt_code_idx+GDT_RING3_OFFSET].type = SEG_DESC_CODE_XR;
    mygdt[gdt_code_idx+GDT_RING3_OFFSET].dpl = 3; // ring3
    init_segment(&mygdt[gdt_data_idx+GDT_RING3_OFFSET], 0, (uint64_t)1<<32);
    mygdt[gdt_data_idx+GDT_RING3_OFFSET].type = SEG_DESC_DATA_RW;
    mygdt[gdt_data_idx+GDT_RING3_OFFSET].dpl = 3; // ring3
    init_segment(&mygdt[gdt_tss_idx], (uint32_t)&__tss_start__, (uint32_t)&__tss_start__+sizeof(tss_t));
    mygdt[gdt_tss_idx].d = 0;
    mygdt[gdt_tss_idx].g = 0;
    mygdt[gdt_tss_idx].s = 0; // system kind
    mygdt[gdt_tss_idx].type = SEG_DESC_SYS_TSS_AVL_32;
    mygdt[gdt_tss_idx].dpl = 3;

    gdt_size = gdt_tss_idx+1;
    update_gdtr();
}

void add_segment_to_gdt(seg_desc_t seg) {
    mygdt[gdt_size++] = seg;
    update_gdtr();
}
