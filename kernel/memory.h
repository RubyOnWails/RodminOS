#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stdbool.h>

// Page flags
#define PAGE_PRESENT    0x001
#define PAGE_WRITABLE   0x002
#define PAGE_USER       0x004
#define PAGE_WRITETHROUGH 0x008
#define PAGE_CACHE_DISABLE 0x010
#define PAGE_ACCESSED   0x020
#define PAGE_DIRTY      0x040
#define PAGE_SIZE_FLAG  0x080
#define PAGE_GLOBAL     0x100
#define PAGE_NO_EXECUTE 0x8000000000000000ULL

// Buddy allocator constants
#define MAX_BUDDY_ORDER 20
#define MIN_BUDDY_SIZE  4096

// Slab allocator constants
#define MAX_SLAB_CACHES 64
#define MAX_SLABS_PER_CACHE 256

// Page table structure
typedef struct {
    uint64_t entries[512];
} page_table_t;

// Buddy allocator block
typedef struct buddy_block {
    uint64_t address;
    uint64_t size;
    struct buddy_block* next;
} buddy_block_t;

// Slab structure
typedef struct slab {
    void* objects;
    uint8_t* bitmap;
    uint32_t free_objects;
    struct slab* next;
} slab_t;

// Slab cache
typedef struct {
    char name[64];
    size_t object_size;
    uint32_t objects_per_slab;
    slab_t* slabs;
} slab_cache_t;

// Memory statistics
typedef struct {
    uint64_t total_memory;
    uint64_t available_memory;
    uint64_t used_memory;
    uint64_t cached_memory;
    uint64_t buffer_memory;
} memory_stats_t;

// Function prototypes
void memory_init(void);
void parse_memory_map(void);
void init_physical_bitmap(void);
void setup_kernel_paging(void);

// Physical memory management
uint64_t alloc_physical_page(void);
void free_physical_page(uint64_t address);
uint64_t alloc_physical_pages(uint32_t count);
void free_physical_pages(uint64_t address, uint32_t count);

// Virtual memory management
void map_page(page_table_t* pml4, uint64_t virtual_addr, uint64_t physical_addr, uint64_t flags);
void unmap_page(page_table_t* pml4, uint64_t virtual_addr);
uint64_t get_physical_address(page_table_t* pml4, uint64_t virtual_addr);
page_table_t* create_page_table(void);
void destroy_page_table(page_table_t* pml4);

// Buddy allocator
void init_buddy_allocator(void);
void* buddy_alloc(uint32_t order);
void buddy_free(void* ptr, uint32_t order);
void add_buddy_block(uint64_t address, uint64_t size);

// Slab allocator
void init_slab_allocator(void);
slab_cache_t* create_slab_cache(const char* name, size_t object_size, uint32_t objects_per_slab);
void* slab_alloc(slab_cache_t* cache);
void slab_free(slab_cache_t* cache, void* ptr);
slab_t* create_slab(slab_cache_t* cache);

// Kernel memory allocation
void* kmalloc(size_t size);
void kfree(void* ptr);
void* krealloc(void* ptr, size_t new_size);
void* kcalloc(size_t count, size_t size);

// Memory utilities
void set_bit(uint8_t* bitmap, uint64_t bit);
void clear_bit(uint8_t* bitmap, uint64_t bit);
bool test_bit(uint8_t* bitmap, uint64_t bit);
void memory_copy(void* dest, const void* src, size_t size);
void memory_set(void* ptr, uint8_t value, size_t size);
int memory_compare(const void* ptr1, const void* ptr2, size_t size);

// Memory statistics
void get_memory_stats(memory_stats_t* stats);
uint64_t get_free_memory(void);
uint64_t get_used_memory(void);

// String functions
size_t strlen(const char* str);
char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, size_t n);
int strcmp(const char* str1, const char* str2);
int strncmp(const char* str1, const char* str2, size_t n);
char* strcat(char* dest, const char* src);

#endif