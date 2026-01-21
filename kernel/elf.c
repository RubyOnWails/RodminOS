#include "kernel.h"
#include "proc.h"
#include <string.h>

#define ELF_MAGIC 0x464C457F

typedef struct {
    uint32_t magic;
    uint8_t  bits;
    uint8_t  endian;
    uint8_t  version;
    uint8_t  abi;
    uint8_t  abi_version;
    uint8_t  pad[7];
    uint16_t type;
    uint16_t machine;
    uint32_t version2;
    uint64_t entry;
    uint64_t phoff;
    uint64_t shoff;
    uint32_t flags;
    uint16_t ehsize;
    uint16_t phentsize;
    uint16_t phnum;
    uint16_t shentsize;
    uint16_t shnum;
    uint16_t shstrndx;
} elf64_header_t;

typedef struct {
    uint32_t type;
    uint32_t flags;
    uint64_t offset;
    uint64_t vaddr;
    uint64_t paddr;
    uint64_t filesz;
    uint64_t memsz;
    uint64_t align;
} elf64_phdr_t;

int elf_load(process_t* p, void* data) {
    elf64_header_t* header = (elf64_header_t*)data;
    
    if (header->magic != ELF_MAGIC) return -1;
    if (header->bits != 2) return -1; // 64-bit

    elf64_phdr_t* phdr = (elf64_phdr_t*)((uint8_t*)data + header->phoff);
    
    for (int i = 0; i < header->phnum; i++) {
        if (phdr[i].type == 1) { // PT_LOAD
            // Map memory in process address space and copy data
            // vmm_map(p->page_table, phdr[i].vaddr, phdr[i].memsz, phdr[i].flags);
            // memcpy((void*)phdr[i].vaddr, (uint8_t*)data + phdr[i].offset, phdr[i].filesz);
            // memset((void*)(phdr[i].vaddr + phdr[i].filesz), 0, phdr[i].memsz - phdr[i].filesz);
        }
    }
    
    p->context->rip = header->entry;
    return 0;
}
