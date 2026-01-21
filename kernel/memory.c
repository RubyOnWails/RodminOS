#include "memory.h"
#include "kernel.h"

static memory_map_entry_t* memory_map;
static uint32_t memory_map_entries;
static uint64_t total_memory;
static uint64_t available_memory;

// Physical memory bitmap
static uint8_t* physical_bitmap;
static uint64_t bitmap_size;

// Virtual memory structures
static page_table_t* kernel_page_table;
static uint64_t next_virtual_address = 0xFFFF800000000000;

// Buddy allocator for physical pages
static buddy_block_t* buddy_blocks[MAX_BUDDY_ORDER];

// Slab allocator for kernel objects
static slab_cache_t slab_caches[MAX_SLAB_CACHES];
static uint32_t slab_cache_count = 0;

void memory_init(void) {
    // Parse memory map from bootloader
    parse_memory_map();
    
    // Initialize physical memory bitmap
    init_physical_bitmap();
    
    // Setup kernel page tables
    setup_kernel_paging();
    
    // Initialize buddy allocator
    init_buddy_allocator();
    
    // Initialize slab allocator
    init_slab_allocator();
    
    kprintf("Memory initialized: %lu MB total, %lu MB available\n", 
            total_memory / (1024*1024), available_memory / (1024*1024));
}

void parse_memory_map(void) {
    memory_map = (memory_map_entry_t*)0x8000;
    memory_map_entries = 0;
    total_memory = 0;
    available_memory = 0;
    
    // Count entries and calculate totals
    for(int i = 0; i < 32; i++) {
        if(memory_map[i].length == 0) break;
        
        memory_map_entries++;
        total_memory += memory_map[i].length;
        
        if(memory_map[i].type == 1) { // Available memory
            available_memory += memory_map[i].length;
        }
    }
}

void init_physical_bitmap(void) {
    bitmap_size = (total_memory / PAGE_SIZE) / 8;
    physical_bitmap = (uint8_t*)0x200000; // Place after kernel
    
    // Mark all memory as used initially
    for(uint64_t i = 0; i < bitmap_size; i++) {
        physical_bitmap[i] = 0xFF;
    }
    
    // Mark available regions as free
    for(uint32_t i = 0; i < memory_map_entries; i++) {
        if(memory_map[i].type == 1) {
            uint64_t start_page = memory_map[i].base / PAGE_SIZE;
            uint64_t end_page = (memory_map[i].base + memory_map[i].length) / PAGE_SIZE;
            
            for(uint64_t page = start_page; page < end_page; page++) {
                clear_bit(physical_bitmap, page);
            }
        }
    }
    
    // Mark kernel and bootloader areas as used
    for(uint64_t page = 0; page < 0x300000 / PAGE_SIZE; page++) {
        set_bit(physical_bitmap, page);
    }
}

uint64_t alloc_physical_page(void) {
    for(uint64_t i = 0; i < bitmap_size * 8; i++) {
        if(!test_bit(physical_bitmap, i)) {
            set_bit(physical_bitmap, i);
            return i * PAGE_SIZE;
        }
    }
    return 0; // Out of memory
}

void free_physical_page(uint64_t address) {
    uint64_t page = address / PAGE_SIZE;
    clear_bit(physical_bitmap, page);
}

void setup_kernel_paging(void) {
    // Allocate page table
    kernel_page_table = (page_table_t*)alloc_physical_page();
    
    // Identity map first 16MB for kernel
    for(uint64_t addr = 0; addr < 0x1000000; addr += PAGE_SIZE) {
        map_page(kernel_page_table, addr, addr, PAGE_PRESENT | PAGE_WRITABLE);
    }
    
    // Map kernel to higher half
    for(uint64_t addr = 0; addr < 0x1000000; addr += PAGE_SIZE) {
        map_page(kernel_page_table, 0xFFFF800000000000 + addr, addr, 
                PAGE_PRESENT | PAGE_WRITABLE);
    }
    
    // Load new page table
    write_cr3((uint64_t)kernel_page_table);
}

void map_page(page_table_t* pml4, uint64_t virtual_addr, uint64_t physical_addr, uint64_t flags) {
    uint64_t pml4_index = (virtual_addr >> 39) & 0x1FF;
    uint64_t pdpt_index = (virtual_addr >> 30) & 0x1FF;
    uint64_t pd_index = (virtual_addr >> 21) & 0x1FF;
    uint64_t pt_index = (virtual_addr >> 12) & 0x1FF;
    
    // Get or create PDPT
    if(!(pml4->entries[pml4_index] & PAGE_PRESENT)) {
        uint64_t pdpt_phys = alloc_physical_page();
        pml4->entries[pml4_index] = pdpt_phys | PAGE_PRESENT | PAGE_WRITABLE;
        
        page_table_t* pdpt = (page_table_t*)pdpt_phys;
        for(int i = 0; i < 512; i++) pdpt->entries[i] = 0;
    }
    
    page_table_t* pdpt = (page_table_t*)(pml4->entries[pml4_index] & ~0xFFF);
    
    // Get or create PD
    if(!(pdpt->entries[pdpt_index] & PAGE_PRESENT)) {
        uint64_t pd_phys = alloc_physical_page();
        pdpt->entries[pdpt_index] = pd_phys | PAGE_PRESENT | PAGE_WRITABLE;
        
        page_table_t* pd = (page_table_t*)pd_phys;
        for(int i = 0; i < 512; i++) pd->entries[i] = 0;
    }
    
    page_table_t* pd = (page_table_t*)(pdpt->entries[pdpt_index] & ~0xFFF);
    
    // Get or create PT
    if(!(pd->entries[pd_index] & PAGE_PRESENT)) {
        uint64_t pt_phys = alloc_physical_page();
        pd->entries[pd_index] = pt_phys | PAGE_PRESENT | PAGE_WRITABLE;
        
        page_table_t* pt = (page_table_t*)pt_phys;
        for(int i = 0; i < 512; i++) pt->entries[i] = 0;
    }
    
    page_table_t* pt = (page_table_t*)(pd->entries[pd_index] & ~0xFFF);
    
    // Map the page
    pt->entries[pt_index] = physical_addr | flags;
}

void init_buddy_allocator(void) {
    // Initialize buddy block lists
    for(int i = 0; i < MAX_BUDDY_ORDER; i++) {
        buddy_blocks[i] = NULL;
    }
    
    // Add available memory regions to buddy allocator
    for(uint32_t i = 0; i < memory_map_entries; i++) {
        if(memory_map[i].type == 1 && memory_map[i].base >= 0x1000000) {
            add_buddy_block(memory_map[i].base, memory_map[i].length);
        }
    }
}

void* buddy_alloc(uint32_t order) {
    if(order >= MAX_BUDDY_ORDER) return NULL;
    
    // Find available block
    for(uint32_t current_order = order; current_order < MAX_BUDDY_ORDER; current_order++) {
        if(buddy_blocks[current_order]) {
            buddy_block_t* block = buddy_blocks[current_order];
            buddy_blocks[current_order] = block->next;
            
            // Split block if necessary
            while(current_order > order) {
                current_order--;
                uint64_t buddy_addr = block->address + (1ULL << (current_order + 12));
                add_buddy_block(buddy_addr, 1ULL << (current_order + 12));
            }
            
            return (void*)block->address;
        }
    }
    
    return NULL; // No memory available
}

void buddy_free(void* ptr, uint32_t order) {
    uint64_t address = (uint64_t)ptr;
    
    // Try to coalesce with buddy
    while(order < MAX_BUDDY_ORDER - 1) {
        uint64_t buddy_addr = address ^ (1ULL << (order + 12));
        
        // Find and remove buddy from free list
        buddy_block_t** current = &buddy_blocks[order];
        bool found_buddy = false;
        
        while(*current) {
            if((*current)->address == buddy_addr) {
                buddy_block_t* buddy = *current;
                *current = buddy->next;
                found_buddy = true;
                break;
            }
            current = &(*current)->next;
        }
        
        if(!found_buddy) break;
        
        // Coalesce
        if(buddy_addr < address) address = buddy_addr;
        order++;
    }
    
    // Add coalesced block to free list
    add_buddy_block(address, 1ULL << (order + 12));
}

void init_slab_allocator(void) {
    slab_cache_count = 0;
    
    // Create common object caches
    create_slab_cache("process", sizeof(process_t), 64);
    create_slab_cache("file_desc", sizeof(file_descriptor_t), 128);
    create_slab_cache("driver", sizeof(driver_t), 32);
}

slab_cache_t* create_slab_cache(const char* name, size_t object_size, uint32_t objects_per_slab) {
    if(slab_cache_count >= MAX_SLAB_CACHES) return NULL;
    
    slab_cache_t* cache = &slab_caches[slab_cache_count++];
    strncpy(cache->name, name, 63);
    cache->object_size = object_size;
    cache->objects_per_slab = objects_per_slab;
    cache->slabs = NULL;
    
    return cache;
}

void* slab_alloc(slab_cache_t* cache) {
    // Find slab with free objects
    slab_t* slab = cache->slabs;
    while(slab && slab->free_objects == 0) {
        slab = slab->next;
    }
    
    // Allocate new slab if needed
    if(!slab) {
        slab = create_slab(cache);
        if(!slab) return NULL;
    }
    
    // Allocate object from slab
    for(uint32_t i = 0; i < cache->objects_per_slab; i++) {
        if(!test_bit(slab->bitmap, i)) {
            set_bit(slab->bitmap, i);
            slab->free_objects--;
            return (void*)((uint64_t)slab->objects + i * cache->object_size);
        }
    }
    
    return NULL;
}

void slab_free(slab_cache_t* cache, void* ptr) {
    // Find which slab contains this object
    slab_t* slab = cache->slabs;
    while(slab) {
        uint64_t slab_start = (uint64_t)slab->objects;
        uint64_t slab_end = slab_start + cache->objects_per_slab * cache->object_size;
        
        if((uint64_t)ptr >= slab_start && (uint64_t)ptr < slab_end) {
            uint32_t index = ((uint64_t)ptr - slab_start) / cache->object_size;
            clear_bit(slab->bitmap, index);
            slab->free_objects++;
            return;
        }
        
        slab = slab->next;
    }
}

// Utility functions
void set_bit(uint8_t* bitmap, uint64_t bit) {
    bitmap[bit / 8] |= (1 << (bit % 8));
}

void clear_bit(uint8_t* bitmap, uint64_t bit) {
    bitmap[bit / 8] &= ~(1 << (bit % 8));
}

bool test_bit(uint8_t* bitmap, uint64_t bit) {
    return bitmap[bit / 8] & (1 << (bit % 8));
}