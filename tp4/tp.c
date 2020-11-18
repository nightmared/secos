/* GPLv2 (c) Airbus */
#include <debug.h>
#include <info.h>
#include <gdt.h>
#include <cr.h>

extern info_t *info;

void tp() {
    printf("\nMultiboot memory information:\n");
    print_mbi_memory_headers();

    init_gdt_flat();
    printf("\nGDT description:\n");
    print_gdt();

    // enable interrupts
    asm volatile("sti");

    printf("cr0 = %p\n", get_cr0());
}
