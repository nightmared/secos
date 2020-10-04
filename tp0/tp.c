/* GPLv2 (c) Airbus */
#include "mbi.h"
#include <debug.h>
#include <info.h>

extern info_t   *info;
extern uint32_t __kernel_start__;
extern uint32_t __kernel_end__;

void tp() {
    debug("kernel mem [0x%x - 0x%x]\n", &__kernel_start__, &__kernel_end__);
    debug("MBI flags 0x%x\n", info->mbi->flags);

    if (info->mbi->flags & MBI_FLAG_MMAP) {
        memory_map_t* mmap_info = (memory_map_t*)info->mbi->mmap_addr;
        while ((uint32_t)mmap_info < (uint32_t)(info->mbi->mmap_addr+info->mbi->mmap_length)) {
            char* type;
            switch (mmap_info->type) {
                case MULTIBOOT_MEMORY_AVAILABLE:
                    type = "available";
                    break;
                case MULTIBOOT_MEMORY_RESERVED:
                    type = "system reserved";
                    break;
                case MULTIBOOT_MEMORY_ACPI_RECLAIMABLE:
                    type = "ACPI";
                    break;
                default:
                    type = "unknown";
            }
            debug("0x%-8x - 0x%-9llx (size: 0x%x bytes, usage: %s)\n", (uint32_t)mmap_info->addr, mmap_info->addr+mmap_info->len, (uint32_t)mmap_info->len, type);
            mmap_info = (memory_map_t*)((uint32_t)mmap_info+mmap_info->size+sizeof(mmap_info->size));
        }
    }
}
