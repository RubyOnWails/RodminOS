#include "icon_generator.h"
#include "../kernel/kernel.h"

static icon_cache_t icon_cache[MAX_CACHED_ICONS];
static uint32_t cache_count = 0;

int generate_icon_set(void) {
    create_folder_icons();
    create_file_icons();
    create_app_icons();
    create_system_icons();
    return 0;
}

void create_folder_icons(void) {
    create_folder_icon("folder.ppm", ICON_STATE_NORMAL);
    create_folder_icon("folder_hover.ppm", ICON_STATE_HOVER);
    create_folder_icon("folder_active.ppm", ICON_STATE_ACTIVE);
    create_folder_icon("folder_inactive.ppm", ICON_STATE_INACTIVE);
}

void create_folder_icon(const char* filename, icon_state_t state) {
    ppm_image_t icon;
    icon.width = 48;
    icon.height = 48;
    icon.data = (uint32_t*)kmalloc(48 * 48 * 4);
    
    uint32_t base_color = get_folder_color(state);
    
    for(int y = 0; y < 48; y++) {
        for(int x = 0; x < 48; x++) {
            uint32_t color = calculate_folder_pixel(x, y, base_color);
            icon.data[y * 48 + x] = color;
        }
    }
    
    save_ppm_icon(&icon, filename);
    kfree(icon.data);
}

uint32_t get_folder_color(icon_state_t state) {
    switch(state) {
        case ICON_STATE_NORMAL: return 0xFFD700;
        case ICON_STATE_HOVER: return 0xFFE55A;
        case ICON_STATE_ACTIVE: return 0xFFB000;
        case ICON_STATE_INACTIVE: return 0xCCAA00;
        default: return 0xFFD700;
    }
}

uint32_t calculate_folder_pixel(int x, int y, uint32_t base_color) {
    if(y < 15 && x >= 5 && x < 35) return base_color;
    if(y >= 15 && y < 40 && x >= 2 && x < 45) return base_color;
    if(y == 14 && x >= 2 && x < 45) return darken_color(base_color, 0.3);
    if(x == 1 || x == 45 || y == 40) return darken_color(base_color, 0.5);
    return 0x00000000;
}

void create_app_icons(void) {
    create_terminal_icon();
    create_browser_icon();
    create_editor_icon();
    create_media_icons();
}

void create_terminal_icon(void) {
    ppm_image_t icon;
    icon.width = 48;
    icon.height = 48;
    icon.data = (uint32_t*)kmalloc(48 * 48 * 4);
    
    for(int y = 0; y < 48; y++) {
        for(int x = 0; x < 48; x++) {
            uint32_t color = 0x000000FF;
            if(x >= 4 && x < 44 && y >= 4 && y < 44) {
                color = 0x1E1E1EFF;
                if((x >= 8 && x < 12 && y >= 12 && y < 16) ||
                   (x >= 16 && x < 32 && y >= 12 && y < 16)) {
                    color = 0x00FF00FF;
                }
            }
            icon.data[y * 48 + x] = color;
        }
    }
    
    save_ppm_icon(&icon, "terminal.ppm");
    kfree(icon.data);
}

int load_icon_with_state(const char* base_name, icon_state_t state, ppm_image_t* icon) {
    char filename[256];
    const char* suffix = get_state_suffix(state);
    snprintf(filename, 256, "%s%s.ppm", base_name, suffix);
    
    icon_cache_entry_t* cached = find_cached_icon(filename);
    if(cached) {
        *icon = cached->icon;
        return 0;
    }
    
    if(load_ppm_image(filename, icon) == 0) {
        cache_icon(filename, icon);
        return 0;
    }
    
    return -1;
}

const char* get_state_suffix(icon_state_t state) {
    switch(state) {
        case ICON_STATE_HOVER: return "_hover";
        case ICON_STATE_ACTIVE: return "_active";
        case ICON_STATE_INACTIVE: return "_inactive";
        default: return "";
    }
}

void cache_icon(const char* filename, ppm_image_t* icon) {
    if(cache_count >= MAX_CACHED_ICONS) return;
    
    icon_cache_entry_t* entry = &icon_cache[cache_count];
    strncpy(entry->filename, filename, 255);
    entry->icon = *icon;
    entry->last_used = get_system_time();
    cache_count++;
}

uint32_t darken_color(uint32_t color, float factor) {
    uint8_t r = ((color >> 16) & 0xFF) * (1.0 - factor);
    uint8_t g = ((color >> 8) & 0xFF) * (1.0 - factor);
    uint8_t b = (color & 0xFF) * (1.0 - factor);
    return (0xFF << 24) | (r << 16) | (g << 8) | b;
}