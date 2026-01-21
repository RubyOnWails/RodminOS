#ifndef GPU_DRIVER_H
#define GPU_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

// PCI Vendor IDs
#define PCI_VENDOR_INTEL  0x8086
#define PCI_VENDOR_AMD    0x1002
#define PCI_VENDOR_NVIDIA 0x10DE
#define PCI_VENDOR_VMWARE 0x15AD

// GPU Types
#define GPU_TYPE_VESA   0
#define GPU_TYPE_INTEL  1
#define GPU_TYPE_AMD    2
#define GPU_TYPE_NVIDIA 3

// Intel GPU registers
#define INTEL_DISPLAY_CONTROL 0x70008
#define INTEL_DISPLAY_ENABLE  0x80000000
#define INTEL_HTOTAL         0x60000
#define INTEL_VTOTAL         0x6000C
#define INTEL_BLT_CMD        0x22000
#define INTEL_BLT_COLOR      0x22004
#define INTEL_BLT_DEST_ADDR  0x22008
#define INTEL_BLT_SIZE       0x2200C
#define INTEL_BLT_CONTROL    0x22010
#define INTEL_BLT_STATUS     0x22014
#define INTEL_BLT_START      0x00000001
#define INTEL_BLT_BUSY       0x00000001
#define INTEL_BLT_SOLID_FILL 0x40000000
#define INTEL_BLT_32BPP      0x00000003

#define MAX_VIDEO_ALLOCATIONS 256

// VBE structures
typedef struct {
    char signature[4];
    uint16_t version;
    uint32_t oem_string;
    uint32_t capabilities;
    uint32_t video_modes;
    uint16_t total_memory;
    uint16_t oem_software_rev;
    uint32_t oem_vendor_name;
    uint32_t oem_product_name;
    uint32_t oem_product_rev;
    uint8_t reserved[222];
    uint8_t oem_data[256];
} __attribute__((packed)) vbe_info_t;

typedef struct {
    uint16_t attributes;
    uint8_t window_a;
    uint8_t window_b;
    uint16_t granularity;
    uint16_t window_size;
    uint16_t segment_a;
    uint16_t segment_b;
    uint32_t win_func_ptr;
    uint16_t pitch;
    uint16_t width;
    uint16_t height;
    uint8_t w_char;
    uint8_t y_char;
    uint8_t planes;
    uint8_t bpp;
    uint8_t banks;
    uint8_t memory_model;
    uint8_t bank_size;
    uint8_t image_pages;
    uint8_t reserved0;
    uint8_t red_mask;
    uint8_t red_position;
    uint8_t green_mask;
    uint8_t green_position;
    uint8_t blue_mask;
    uint8_t blue_position;
    uint8_t reserved_mask;
    uint8_t reserved_position;
    uint8_t direct_color_attributes;
    uint32_t framebuffer;
    uint32_t off_screen_mem_off;
    uint16_t off_screen_mem_size;
    uint8_t reserved1[206];
} __attribute__((packed)) vbe_mode_info_t;

// 3D Graphics structures
typedef struct {
    float x, y, z;
    float u, v;
    uint32_t color;
} vertex_t;

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t format;
    void* data;
} texture_t;

typedef struct {
    uint32_t vertex_shader_id;
    uint32_t fragment_shader_id;
    uint32_t program_id;
} shader_program_t;

// Video memory allocation
typedef struct {
    bool used;
    size_t size;
    void* virt_addr;
    uint64_t phys_addr;
} video_allocation_t;

// GPU context
typedef struct {
    uint32_t type;
    uint16_t vendor_id;
    uint16_t device_id;
    
    // PCI information
    int pci_bus;
    int pci_device;
    int pci_function;
    
    // Display information
    uint32_t width;
    uint32_t height;
    uint32_t bpp;
    uint32_t pitch;
    
    // Memory mapping
    uint64_t framebuffer_phys;
    uint8_t* framebuffer_virt;
    size_t framebuffer_size;
    
    uint64_t mmio_base;
    uint8_t* mmio_virt;
    size_t mmio_size;
    
    // Capabilities
    bool hw_accel_available;
    bool hw_3d_available;
    bool shader_support;
    
    // Video memory management
    video_allocation_t video_allocations[MAX_VIDEO_ALLOCATIONS];
} gpu_context_t;

// Display information
typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t bpp;
    uint32_t pitch;
    void* framebuffer;
    bool hw_accel;
    bool hw_3d;
    bool shader_support;
} display_info_t;

// Function prototypes

// Driver initialization
int init_gpu_driver(void);
int detect_gpu_hardware(void);
int init_vesa_gpu(void);
int init_intel_gpu(void);
int init_amd_gpu(void);
int init_nvidia_gpu(void);
int init_vmware_gpu(void);

// Basic graphics operations
void gpu_clear_screen(uint32_t color);
void gpu_draw_pixel(int x, int y, uint32_t color);
void gpu_draw_rectangle(int x, int y, int width, int height, uint32_t color);
void gpu_draw_line(int x1, int y1, int x2, int y2, uint32_t color);
void gpu_draw_circle(int cx, int cy, int radius, uint32_t color);
void gpu_blit_buffer(const uint32_t* buffer, int src_x, int src_y, int src_width,
                     int dest_x, int dest_y, int width, int height);

// Hardware acceleration
void init_intel_display_engine(void);
void init_intel_render_engine(void);
void intel_hw_draw_rectangle(int x, int y, int width, int height, uint32_t color);
void init_amd_display_controller(void);
void init_amd_graphics_engine(void);
void amd_hw_draw_rectangle(int x, int y, int width, int height, uint32_t color);
void init_nvidia_display_engine(void);
void init_nvidia_cuda_cores(void);
void nvidia_hw_draw_rectangle(int x, int y, int width, int height, uint32_t color);

// 3D Graphics
int init_3d_engine(void);
int init_intel_3d_engine(void);
int init_amd_3d_engine(void);
int init_nvidia_3d_engine(void);
void gpu_draw_triangle_3d(vertex_t* v1, vertex_t* v2, vertex_t* v3, texture_t* texture);
void software_draw_triangle_3d(vertex_t* v1, vertex_t* v2, vertex_t* v3, texture_t* texture);
void intel_hw_draw_triangle_3d(vertex_t* v1, vertex_t* v2, vertex_t* v3, texture_t* texture);
void amd_hw_draw_triangle_3d(vertex_t* v1, vertex_t* v2, vertex_t* v3, texture_t* texture);
void nvidia_hw_draw_triangle_3d(vertex_t* v1, vertex_t* v2, vertex_t* v3, texture_t* texture);

// Shader support
int load_shader(const char* vertex_shader, const char* fragment_shader, shader_program_t* program);
int intel_load_shader(const char* vertex_shader, const char* fragment_shader, shader_program_t* program);
int amd_load_shader(const char* vertex_shader, const char* fragment_shader, shader_program_t* program);
int nvidia_load_shader(const char* vertex_shader, const char* fragment_shader, shader_program_t* program);
void use_shader_program(shader_program_t* program);
void set_shader_uniform(shader_program_t* program, const char* name, const void* value);

// Video memory management
void* gpu_alloc_video_memory(size_t size);
void gpu_free_video_memory(void* ptr);
void* allocate_video_memory_block(size_t size);
void free_video_memory_block(void* ptr, size_t size);

// Display mode management
int gpu_set_display_mode(int width, int height, int bpp);
void gpu_get_display_info(display_info_t* info);
int vesa_set_display_mode(int width, int height, int bpp);
int intel_set_display_mode(int width, int height, int bpp);
int amd_set_display_mode(int width, int height, int bpp);
int nvidia_set_display_mode(int width, int height, int bpp);

// VBE functions
int get_vbe_controller_info(vbe_info_t* info);
int get_vbe_mode_info(uint16_t mode, vbe_mode_info_t* info);
int set_vbe_mode(uint16_t mode);

// Utility functions
void vesa_clear_screen(uint32_t color);
void intel_clear_screen(uint32_t color);
void amd_clear_screen(uint32_t color);
void nvidia_clear_screen(uint32_t color);
uint32_t read_pci_config(int bus, int device, int function, int offset);
void write_pci_config(int bus, int device, int function, int offset, uint32_t value);
void* map_physical_memory(uint64_t phys_addr, size_t size);

// Hardware I/O
void outl(uint16_t port, uint32_t value);
uint32_t inl(uint16_t port);

// Memory management
void* get_kernel_page_table(void);
void map_page(void* page_table, uint64_t virt_addr, uint64_t phys_addr, uint64_t flags);

// Standard functions
int abs(int x);

#endif