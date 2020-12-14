/* GPLv2 (c) Airbus */
#include <intr.h>
#include <pic.h>
#include <gdt.h>
#include <debug.h>
#include <info.h>
#include <io.h>

extern info_t *info;
extern void idt_trampoline();
static int_desc_t IDT[IDT_NR_DESC];
extern int asm_syscall_hdlr;
extern int __userland_mapped__;

void intr_init()
{
    idt_reg_t idtr;
    size_t    i;

    // relocate handlers at 0xc0000000 so that they work in both useerland and kernelland
    uint32_t isr = (0xc0000000-(uint32_t)&__userland_mapped__+(uint32_t)idt_trampoline);
    uint32_t hdlr80 = (0xc0000000-(uint32_t)&__userland_mapped__+(uint32_t)&asm_syscall_hdlr);

    /* re-use default grub GDT code descriptor */
    for(i=0 ; i<IDT_NR_DESC ; i++, isr += IDT_ISR_ALGN) {
        int_desc(&IDT[i], gdt_krn_seg_sel(gdt_code_idx), isr);
    }

    // enable syscalls
    IDT[0x80].offset_1 = hdlr80&((1<<16)-1);
    IDT[0x80].offset_2 = hdlr80>>16;
    //IDT[0x80].type = SEG_DESC_SYS_CALL_GATE_32;
    IDT[0x80].dpl = 3;

    //// Allow userland to raise #BP exceptions
    //IDT[3].dpl = 3;

    idtr.desc  = IDT;
    idtr.limit = sizeof(IDT) - 1;
    set_idtr(idtr);
}

void __regparm__(1) intr_hdlr(int_ctx_t *ctx) {
    //printf("Hola from intr_hdlr\n");
    uint8_t vector = ctx->nr.blow;

    if(vector >= NR_EXCP && vector < 48) {
         return pic_handler(ctx);
    }

    debug("\nIDT event\n"
        " . int    #%d\n"
        " . error  0x%x\n"
        " . cs:eip 0x%x:0x%x\n"
        " . ss:esp 0x%x:0x%x\n"
        " . eflags 0x%x\n"
        "\n- GPR\n"
        "eax     : 0x%x\n"
        "ecx     : 0x%x\n"
        "edx     : 0x%x\n"
        "ebx     : 0x%x\n"
        "esp     : 0x%x\n"
        "ebp     : 0x%x\n"
        "esi     : 0x%x\n"
        "edi     : 0x%x\n"
        ,ctx->nr.raw, ctx->err.raw
        ,ctx->cs.raw, ctx->eip.raw
        ,ctx->ss.raw, ctx->esp.raw
        ,ctx->eflags.raw
        ,ctx->gpr.eax.raw
        ,ctx->gpr.ecx.raw
        ,ctx->gpr.edx.raw
        ,ctx->gpr.ebx.raw
        ,ctx->gpr.esp.raw
        ,ctx->gpr.ebp.raw
        ,ctx->gpr.esi.raw
        ,ctx->gpr.edi.raw);

    excp_hdlr(ctx);
}
