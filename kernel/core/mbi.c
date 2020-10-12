#include <mbi.h>
#include <info.h>
#include <debug.h>

extern info_t   *info;

void print_mbi_memory_headers() {
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
