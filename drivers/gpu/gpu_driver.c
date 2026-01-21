#include "gpu_driver.h"
#include "../kernel/kernel.h"
#include "../kernel/memory.h"

static gpu_context_t gpu_ctx;
static bool gpu_initialized = false;

int init_gpu_driver(void) {
    // Detect GPU hardware
    if(detect_gpu_hardware() != 0) {
        kprintf("No supported GPU found\n");
        return -1;
    }
    
    // Initialize GPU context
    gpu_ctx.vendor_id = read_pci_config(gpu_ctx.pci_bus, gpu_ctx.pci_device, 0, 0x00) & 0xFFFF;
    gpu_ctx.device_id = (read_pci_config(gpu_ctx.pci_bus, gpu_ctx.pci_device, 0, 0x00) >> 16) & 0xFFFF;
    
    // Initialize based on vendor
    switch(gpu_ctx.vendor_id) {
        case PCI_VENDOR_INTEL:
            return init_intel_gpu();
        case PCI_VENDOR_AMD:
            return init_amd_gpu();
        case PCI_VENDOR_NVIDIA:
            return init_nvidia_gpu();
        case PCI_VENDOR_VMWARE:
            return init_vmware_gpu();
        default:
            return init_vesa_gpu();
    }
}

int detect_gpu_hardware(void) {
    // Scan PCI bus for GPU devices
    for(int bus = 0; bus < 256; bus++) {
        for(int device = 0; device < 32; device++) {
            for(int function = 0; function < 8; function++) {
                uint32_t vendor_device = read_pci_config(bus, device, function, 0x00);
                if(vendor_device == 0xFFFFFFFF) continue;
                
                uint32_t class_code = read_pci_config(bus, device, function, 0x08);
                uint8_t base_class = (class_code >> 24) & 0xFF;
                uint8_t sub_class = (class_code >> 16) & 0xFF;
                
                // Check for VGA compatible controller
                if(base_class == 0x03 && sub_class == 0x00) {
                    gpu_ctx.pci_bus = bus;
                    gpu_ctx.pci_device = device;
                    gpu_ctx.pci_function = function;
                    return 0;
                }
            }
        }
    }
    
    return -1;
}

int init_vesa_gpu(void) {
    // Initialize VESA BIOS Extensions
    vbe_info_t* vbe_info = (vbe_info_t*)0x7E00;
    
    // Get VBE controller info
    if(get_vbe_controller_info(vbe_info) != 0) {
        return -1;
    }
    
    // Find best video mode
    uint16_t* modes = (uint16_t*)(vbe_info->video_modes & 0xFFFF);
    uint16_t best_mode = 0;
    uint32_t best_resolution = 0;
    
    for(int i = 0; modes[i] != 0xFFFF; i++) {
        vbe_mode_info_t mode_info;
        if(get_vbe_mode_info(modes[i], &mode_info) == 0) {
            uint32_t resolution = mode_info.width * mode_info.height;
            if(mode_info.bpp >= 24 && resolution > best_resolution) {
                best_mode = modes[i];
                best_resolution = resolution;
                gpu_ctx.width = mode_info.width;
                gpu_ctx.height = mode_info.height;
                gpu_ctx.bpp = mode_info.bpp;
                gpu_ctx.pitch = mode_info.pitch;
                gpu_ctx.framebuffer_phys = mode_info.framebuffer;
            }
        }
    }
    
    if(best_mode == 0) {
        return -1;
    }
    
    // Set video mode
    if(set_vbe_mode(best_mode) != 0) {
        return -1;
    }
    
    // Map framebuffer
    gpu_ctx.framebuffer_size = gpu_ctx.height * gpu_ctx.pitch;
    gpu_ctx.framebuffer_virt = (uint8_t*)map_physical_memory(
        gpu_ctx.framebuffer_phys, gpu_ctx.framebuffer_size);
    
    gpu_ctx.type = GPU_TYPE_VESA;
    gpu_initialized = true;
    
    kprintf("VESA GPU initialized: %dx%d@%dbpp\n", 
            gpu_ctx.width, gpu_ctx.height, gpu_ctx.bpp);
    
    return 0;
}

int init_intel_gpu(void) {
    // Intel integrated graphics initialization
    gpu_ctx.mmio_base = read_pci_config(gpu_ctx.pci_bus, gpu_ctx.pci_device, 0, 0x10) & ~0xF;
    gpu_ctx.mmio_size = 0x200000; // 2MB MMIO space
    
    // Map MMIO registers
    gpu_ctx.mmio_virt = (uint8_t*)map_physical_memory(gpu_ctx.mmio_base, gpu_ctx.mmio_size);
    
    // Initialize Intel GPU specific features
    init_intel_display_engine();
    init_intel_render_engine();
    
    gpu_ctx.type = GPU_TYPE_INTEL;
    gpu_initialized = true;
    
    kprintf("Intel GPU initialized\n");
    return 0;
}

int init_amd_gpu(void) {
    // AMD GPU initialization
    gpu_ctx.mmio_base = read_pci_config(gpu_ctx.pci_bus, gpu_ctx.pci_device, 0, 0x18) & ~0xF;
    gpu_ctx.mmio_size = 0x40000; // 256KB MMIO space
    
    // Map MMIO registers
    gpu_ctx.mmio_virt = (uint8_t*)map_physical_memory(gpu_ctx.mmio_base, gpu_ctx.mmio_size);
    
    // Initialize AMD GPU specific features
    init_amd_display_controller();
    init_amd_graphics_engine();
    
    gpu_ctx.type = GPU_TYPE_AMD;
    gpu_initialized = true;
    
    kprintf("AMD GPU initialized\n");
    return 0;
}

int init_nvidia_gpu(void) {
    // NVIDIA GPU initialization
    gpu_ctx.mmio_base = read_pci_config(gpu_ctx.pci_bus, gpu_ctx.pci_device, 0, 0x14) & ~0xF;
    gpu_ctx.mmio_size = 0x1000000; // 16MB MMIO space
    
    // Map MMIO registers
    gpu_ctx.mmio_virt = (uint8_t*)map_physical_memory(gpu_ctx.mmio_base, gpu_ctx.mmio_size);
    
    // Initialize NVIDIA GPU specific features
    init_nvidia_display_engine();
    init_nvidia_cuda_cores();
    
    gpu_ctx.type = GPU_TYPE_NVIDIA;
    gpu_initialized = true;
    
    kprintf("NVIDIA GPU initialized\n");
    return 0;
}

// Graphics operations

void gpu_clear_screen(uint32_t color) {
    if(!gpu_initialized) return;
    
    switch(gpu_ctx.type) {
        case GPU_TYPE_VESA:
            vesa_clear_screen(color);
            break;
        case GPU_TYPE_INTEL:
            intel_clear_screen(color);
            break;
        case GPU_TYPE_AMD:
            amd_clear_screen(color);
            break;
        case GPU_TYPE_NVIDIA:
            nvidia_clear_screen(color);
            break;
    }
}

void gpu_draw_pixel(int x, int y, uint32_t color) {
    if(!gpu_initialized || x < 0 || y < 0 || 
       x >= gpu_ctx.width || y >= gpu_ctx.height) {
        return;
    }
    
    switch(gpu_ctx.bpp) {
        case 24: {
            uint8_t* pixel = gpu_ctx.framebuffer_virt + y * gpu_ctx.pitch + x * 3;
            pixel[0] = color & 0xFF;         // Blue
            pixel[1] = (color >> 8) & 0xFF;  // Green
            pixel[2] = (color >> 16) & 0xFF; // Red
            break;
        }
        case 32: {
            uint32_t* pixel = (uint32_t*)(gpu_ctx.framebuffer_virt + y * gpu_ctx.pitch + x * 4);
            *pixel = color;
            break;
        }
    }
}

void gpu_draw_rectangle(int x, int y, int width, int height, uint32_t color) {
    if(!gpu_initialized) return;
    
    // Use hardware acceleration if available
    if(gpu_ctx.type != GPU_TYPE_VESA && gpu_ctx.hw_accel_available) {
        switch(gpu_ctx.type) {
            case GPU_TYPE_INTEL:
                intel_hw_draw_rectangle(x, y, width, height, color);
                break;
            case GPU_TYPE_AMD:
                amd_hw_draw_rectangle(x, y, width, height, color);
                break;
            case GPU_TYPE_NVIDIA:
                nvidia_hw_draw_rectangle(x, y, width, height, color);
                break;
        }
    } else {
        // Software fallback
        for(int dy = 0; dy < height; dy++) {
            for(int dx = 0; dx < width; dx++) {
                gpu_draw_pixel(x + dx, y + dy, color);
            }
        }
    }
}

void gpu_draw_line(int x1, int y1, int x2, int y2, uint32_t color) {
    // Bresenham's line algorithm
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = x1 < x2 ? 1 : -1;
    int sy = y1 < y2 ? 1 : -1;
    int err = dx - dy;
    
    while(true) {
        gpu_draw_pixel(x1, y1, color);
        
        if(x1 == x2 && y1 == y2) break;
        
        int e2 = 2 * err;
        if(e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if(e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
}

void gpu_blit_buffer(const uint32_t* buffer, int src_x, int src_y, int src_width,
                     int dest_x, int dest_y, int width, int height) {
    if(!gpu_initialized) return;
    
    for(int y = 0; y < height; y++) {
        for(int x = 0; x < width; x++) {
            int src_offset = (src_y + y) * src_width + (src_x + x);
            uint32_t pixel = buffer[src_offset];
            gpu_draw_pixel(dest_x + x, dest_y + y, pixel);
        }
    }
}

// Hardware acceleration functions

void init_intel_display_engine(void) {
    // Initialize Intel display engine
    volatile uint32_t* regs = (volatile uint32_t*)gpu_ctx.mmio_virt;
    
    // Enable display engine
    regs[INTEL_DISPLAY_CONTROL] |= INTEL_DISPLAY_ENABLE;
    
    // Configure display timing
    regs[INTEL_HTOTAL] = (gpu_ctx.width - 1) | ((gpu_ctx.width + 100 - 1) << 16);
    regs[INTEL_VTOTAL] = (gpu_ctx.height - 1) | ((gpu_ctx.height + 50 - 1) << 16);
    
    gpu_ctx.hw_accel_available = true;
}

void intel_hw_draw_rectangle(int x, int y, int width, int height, uint32_t color) {
    volatile uint32_t* regs = (volatile uint32_t*)gpu_ctx.mmio_virt;
    
    // Setup 2D blit engine
    regs[INTEL_BLT_CMD] = INTEL_BLT_SOLID_FILL | INTEL_BLT_32BPP;
    regs[INTEL_BLT_COLOR] = color;
    regs[INTEL_BLT_DEST_ADDR] = gpu_ctx.framebuffer_phys + y * gpu_ctx.pitch + x * 4;
    regs[INTEL_BLT_SIZE] = (height << 16) | width;
    
    // Start operation
    regs[INTEL_BLT_CONTROL] |= INTEL_BLT_START;
    
    // Wait for completion
    while(regs[INTEL_BLT_STATUS] & INTEL_BLT_BUSY);
}

// 3D Graphics support

int init_3d_engine(void) {
    if(!gpu_initialized) return -1;
    
    switch(gpu_ctx.type) {
        case GPU_TYPE_INTEL:
            return init_intel_3d_engine();
        case GPU_TYPE_AMD:
            return init_amd_3d_engine();
        case GPU_TYPE_NVIDIA:
            return init_nvidia_3d_engine();
        default:
            return -1; // 3D not supported
    }
}

void gpu_draw_triangle_3d(vertex_t* v1, vertex_t* v2, vertex_t* v3, texture_t* texture) {
    if(!gpu_ctx.hw_3d_available) {
        // Software 3D rendering fallback
        software_draw_triangle_3d(v1, v2, v3, texture);
        return;
    }
    
    switch(gpu_ctx.type) {
        case GPU_TYPE_INTEL:
            intel_hw_draw_triangle_3d(v1, v2, v3, texture);
            break;
        case GPU_TYPE_AMD:
            amd_hw_draw_triangle_3d(v1, v2, v3, texture);
            break;
        case GPU_TYPE_NVIDIA:
            nvidia_hw_draw_triangle_3d(v1, v2, v3, texture);
            break;
    }
}

// Shader support

int load_shader(const char* vertex_shader, const char* fragment_shader, shader_program_t* program) {
    if(!gpu_ctx.shader_support) return -1;
    
    switch(gpu_ctx.type) {
        case GPU_TYPE_INTEL:
            return intel_load_shader(vertex_shader, fragment_shader, program);
        case GPU_TYPE_AMD:
            return amd_load_shader(vertex_shader, fragment_shader, program);
        case GPU_TYPE_NVIDIA:
            return nvidia_load_shader(vertex_shader, fragment_shader, program);
        default:
            return -1;
    }
}

// Video memory management

void* gpu_alloc_video_memory(size_t size) {
    if(!gpu_initialized) return NULL;
    
    // Allocate from video memory pool
    for(int i = 0; i < MAX_VIDEO_ALLOCATIONS; i++) {
        if(!gpu_ctx.video_allocations[i].used) {
            gpu_ctx.video_allocations[i].used = true;
            gpu_ctx.video_allocations[i].size = size;
            gpu_ctx.video_allocations[i].virt_addr = allocate_video_memory_block(size);
            return gpu_ctx.video_allocations[i].virt_addr;
        }
    }
    
    return NULL;
}

void gpu_free_video_memory(void* ptr) {
    if(!ptr) return;
    
    for(int i = 0; i < MAX_VIDEO_ALLOCATIONS; i++) {
        if(gpu_ctx.video_allocations[i].virt_addr == ptr) {
            gpu_ctx.video_allocations[i].used = false;
            free_video_memory_block(ptr, gpu_ctx.video_allocations[i].size);
            break;
        }
    }
}

// Display mode management

int gpu_set_display_mode(int width, int height, int bpp) {
    if(!gpu_initialized) return -1;
    
    switch(gpu_ctx.type) {
        case GPU_TYPE_VESA:
            return vesa_set_display_mode(width, height, bpp);
        case GPU_TYPE_INTEL:
            return intel_set_display_mode(width, height, bpp);
        case GPU_TYPE_AMD:
            return amd_set_display_mode(width, height, bpp);
        case GPU_TYPE_NVIDIA:
            return nvidia_set_display_mode(width, height, bpp);
        default:
            return -1;
    }
}

void gpu_get_display_info(display_info_t* info) {
    if(!gpu_initialized || !info) return;
    
    info->width = gpu_ctx.width;
    info->height = gpu_ctx.height;
    info->bpp = gpu_ctx.bpp;
    info->pitch = gpu_ctx.pitch;
    info->framebuffer = gpu_ctx.framebuffer_virt;
    info->hw_accel = gpu_ctx.hw_accel_available;
    info->hw_3d = gpu_ctx.hw_3d_available;
    info->shader_support = gpu_ctx.shader_support;
}

// Utility functions

void vesa_clear_screen(uint32_t color) {
    uint32_t pixels = gpu_ctx.width * gpu_ctx.height;
    
    if(gpu_ctx.bpp == 32) {
        uint32_t* fb = (uint32_t*)gpu_ctx.framebuffer_virt;
        for(uint32_t i = 0; i < pixels; i++) {
            fb[i] = color;
        }
    } else if(gpu_ctx.bpp == 24) {
        uint8_t* fb = gpu_ctx.framebuffer_virt;
        uint8_t r = (color >> 16) & 0xFF;
        uint8_t g = (color >> 8) & 0xFF;
        uint8_t b = color & 0xFF;
        
        for(uint32_t i = 0; i < pixels; i++) {
            fb[i * 3] = b;
            fb[i * 3 + 1] = g;
            fb[i * 3 + 2] = r;
        }
    }
}

uint32_t read_pci_config(int bus, int device, int function, int offset) {
    uint32_t address = 0x80000000 | (bus << 16) | (device << 11) | (function << 8) | offset;
    outl(0xCF8, address);
    return inl(0xCFC);
}

void* map_physical_memory(uint64_t phys_addr, size_t size) {
    // Map physical memory to virtual address space
    uint64_t virt_addr = 0xFFFF800000000000 + phys_addr;
    
    // Add page table entries
    size_t pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    for(size_t i = 0; i < pages; i++) {
        map_page(get_kernel_page_table(), virt_addr + i * PAGE_SIZE, 
                phys_addr + i * PAGE_SIZE, PAGE_PRESENT | PAGE_WRITABLE);
    }
    
    return (void*)virt_addr;
}