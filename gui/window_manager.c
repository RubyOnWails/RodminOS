#include "gui.h"
#include "memory.h"
#include "kernel.h"
#include "driver.h"

// Global GUI state
static gui_context_t* gui_context;
static window_t* window_list = NULL;
static window_t* active_window = NULL;
static desktop_t desktop;

// Graphics buffer
static uint32_t* framebuffer;
static uint32_t screen_width;
static uint32_t screen_height;
static uint32_t screen_bpp;

// Font system
static font_t system_font;
static font_t title_font;

// Theme system
static theme_t current_theme;

void gui_init(void) {
    // Initialize graphics driver
    init_graphics_driver();
    
    // Get screen information
    get_screen_info(&screen_width, &screen_height, &screen_bpp);
    
    // Allocate framebuffer
    framebuffer = (uint32_t*)kmalloc(screen_width * screen_height * 4);
    
    // Initialize GUI context
    gui_context = (gui_context_t*)kmalloc(sizeof(gui_context_t));
    gui_context->screen_width = screen_width;
    gui_context->screen_height = screen_height;
    gui_context->framebuffer = framebuffer;
    gui_context->window_count = 0;
    
    // Load system fonts
    load_system_fonts();
    
    // Initialize theme
    init_default_theme();
    
    // Initialize desktop
    init_desktop();
    
    // Start window manager
    start_window_manager();
    
    kprintf("GUI system initialized: %dx%d\n", screen_width, screen_height);
}

void init_desktop(void) {
    desktop.width = screen_width;
    desktop.height = screen_height;
    desktop.background_color = current_theme.desktop_bg;
    
    // Load desktop wallpaper
    load_ppm_image("/system/wallpaper.ppm", &desktop.wallpaper);
    
    // Initialize taskbar
    desktop.taskbar.x = 0;
    desktop.taskbar.y = screen_height - TASKBAR_HEIGHT;
    desktop.taskbar.width = screen_width;
    desktop.taskbar.height = TASKBAR_HEIGHT;
    desktop.taskbar.background_color = current_theme.taskbar_bg;
    
    // Initialize system tray
    desktop.system_tray.x = screen_width - SYSTEM_TRAY_WIDTH;
    desktop.system_tray.y = screen_height - TASKBAR_HEIGHT;
    desktop.system_tray.width = SYSTEM_TRAY_WIDTH;
    desktop.system_tray.height = TASKBAR_HEIGHT;
    
    // Load desktop icons
    load_desktop_icons();
    
    // Draw initial desktop
    draw_desktop();
}

window_t* create_window(const char* title, int x, int y, int width, int height, uint32_t flags) {
    // Allocate window structure
    window_t* window = (window_t*)slab_alloc(get_window_cache());
    if(!window) return NULL;
    
    // Initialize window
    window->id = generate_window_id();
    strncpy(window->title, title, 255);
    window->x = x;
    window->y = y;
    window->width = width;
    window->height = height;
    window->flags = flags;
    window->state = WINDOW_NORMAL;
    window->visible = true;
    window->focused = false;
    
    // Allocate window buffer
    window->buffer = (uint32_t*)kmalloc(width * height * 4);
    
    // Initialize window decorations
    window->title_bar_height = TITLE_BAR_HEIGHT;
    window->border_width = BORDER_WIDTH;
    
    // Load window icons
    load_ppm_image("/system/icons/window_close.ppm", &window->close_icon);
    load_ppm_image("/system/icons/window_minimize.ppm", &window->minimize_icon);
    load_ppm_image("/system/icons/window_maximize.ppm", &window->maximize_icon);
    
    // Add to window list
    window->next = window_list;
    window_list = window;
    gui_context->window_count++;
    
    // Draw window
    draw_window(window);
    
    return window;
}

void destroy_window(window_t* window) {
    if(!window) return;
    
    // Remove from window list
    if(window_list == window) {
        window_list = window->next;
    } else {
        window_t* current = window_list;
        while(current && current->next != window) {
            current = current->next;
        }
        if(current) {
            current->next = window->next;
        }
    }
    
    // Free window buffer
    kfree(window->buffer);
    
    // Free window structure
    slab_free(get_window_cache(), window);
    
    gui_context->window_count--;
    
    // Redraw desktop
    draw_desktop();
}

void draw_window(window_t* window) {
    if(!window || !window->visible) return;
    
    // Draw window background
    fill_rect(window->buffer, 0, 0, window->width, window->height, 
              current_theme.window_bg);
    
    // Draw title bar
    draw_title_bar(window);
    
    // Draw border
    draw_window_border(window);
    
    // Draw window controls
    draw_window_controls(window);
    
    // Blit window to screen
    blit_window_to_screen(window);
}

void draw_title_bar(window_t* window) {
    uint32_t title_color = window->focused ? 
                          current_theme.title_bar_active : 
                          current_theme.title_bar_inactive;
    
    // Fill title bar background
    fill_rect(window->buffer, 0, 0, window->width, window->title_bar_height, title_color);
    
    // Draw title text
    draw_text(window->buffer, window->title, 10, 5, 
              current_theme.title_text, &title_font);
    
    // Draw window icon if available
    if(window->icon.data) {
        draw_ppm_image(window->buffer, &window->icon, 5, 5);
    }
}

void draw_window_controls(window_t* window) {
    int control_x = window->width - 20;
    int control_y = 5;
    
    // Close button
    draw_ppm_image(window->buffer, &window->close_icon, control_x, control_y);
    control_x -= 25;
    
    // Maximize button
    if(window->flags & WINDOW_RESIZABLE) {
        draw_ppm_image(window->buffer, &window->maximize_icon, control_x, control_y);
        control_x -= 25;
    }
    
    // Minimize button
    if(window->flags & WINDOW_MINIMIZABLE) {
        draw_ppm_image(window->buffer, &window->minimize_icon, control_x, control_y);
    }
}

void gui_handle_mouse(mouse_event_t* event) {
    // Find window under cursor
    window_t* target_window = find_window_at_position(event->x, event->y);
    
    if(target_window) {
        // Convert to window coordinates
        int window_x = event->x - target_window->x;
        int window_y = event->y - target_window->y;
        
        // Handle title bar clicks
        if(window_y < target_window->title_bar_height) {
            handle_title_bar_click(target_window, window_x, window_y, event);
        } else {
            // Send event to window
            send_window_event(target_window, EVENT_MOUSE, event);
        }
        
        // Focus window
        focus_window(target_window);
    } else {
        // Click on desktop
        handle_desktop_click(event);
    }
}

void gui_handle_keyboard(keyboard_event_t* event) {
    if(active_window) {
        send_window_event(active_window, EVENT_KEYBOARD, event);
    }
}

void handle_title_bar_click(window_t* window, int x, int y, mouse_event_t* event) {
    // Check window controls
    int control_x = window->width - 20;
    
    // Close button
    if(x >= control_x && x < control_x + 15 && y >= 5 && y < 20) {
        if(event->type == MOUSE_BUTTON_DOWN && event->button == MOUSE_LEFT) {
            send_window_event(window, EVENT_CLOSE, NULL);
        }
        return;
    }
    control_x -= 25;
    
    // Maximize button
    if(window->flags & WINDOW_RESIZABLE) {
        if(x >= control_x && x < control_x + 15 && y >= 5 && y < 20) {
            if(event->type == MOUSE_BUTTON_DOWN && event->button == MOUSE_LEFT) {
                toggle_window_maximize(window);
            }
            return;
        }
        control_x -= 25;
    }
    
    // Minimize button
    if(window->flags & WINDOW_MINIMIZABLE) {
        if(x >= control_x && x < control_x + 15 && y >= 5 && y < 20) {
            if(event->type == MOUSE_BUTTON_DOWN && event->button == MOUSE_LEFT) {
                minimize_window(window);
            }
            return;
        }
    }
    
    // Window dragging
    if(event->type == MOUSE_BUTTON_DOWN && event->button == MOUSE_LEFT) {
        start_window_drag(window, event->x, event->y);
    }
}

void focus_window(window_t* window) {
    if(active_window == window) return;
    
    // Unfocus previous window
    if(active_window) {
        active_window->focused = false;
        draw_title_bar(active_window);
    }
    
    // Focus new window
    active_window = window;
    window->focused = true;
    
    // Move to front of window list
    move_window_to_front(window);
    
    // Redraw window
    draw_title_bar(window);
    
    // Send focus event
    send_window_event(window, EVENT_FOCUS, NULL);
}

void minimize_window(window_t* window) {
    window->state = WINDOW_MINIMIZED;
    window->visible = false;
    
    // Add to taskbar
    add_to_taskbar(window);
    
    // Redraw desktop
    draw_desktop();
    
    // Focus next window
    focus_next_window();
}

void maximize_window(window_t* window) {
    if(window->state == WINDOW_MAXIMIZED) {
        // Restore window
        window->state = WINDOW_NORMAL;
        window->x = window->restore_x;
        window->y = window->restore_y;
        window->width = window->restore_width;
        window->height = window->restore_height;
    } else {
        // Save current position
        window->restore_x = window->x;
        window->restore_y = window->y;
        window->restore_width = window->width;
        window->restore_height = window->height;
        
        // Maximize
        window->state = WINDOW_MAXIMIZED;
        window->x = 0;
        window->y = 0;
        window->width = screen_width;
        window->height = screen_height - TASKBAR_HEIGHT;
        
        // Reallocate buffer
        kfree(window->buffer);
        window->buffer = (uint32_t*)kmalloc(window->width * window->height * 4);
    }
    
    draw_window(window);
}

// PPM image support
bool load_ppm_image(const char* path, ppm_image_t* image) {
    int fd = fs_open(path, O_RDONLY);
    if(fd < 0) return false;
    
    // Read PPM header
    char header[256];
    ssize_t bytes_read = fs_read(fd, header, 255);
    if(bytes_read < 0) {
        fs_close(fd);
        return false;
    }
    header[bytes_read] = '\0';
    
    // Parse header
    char* line = strtok(header, "\n");
    if(!line || strcmp(line, "P6") != 0) {
        fs_close(fd);
        return false;
    }
    
    // Skip comments
    do {
        line = strtok(NULL, "\n");
    } while(line && line[0] == '#');
    
    // Get dimensions
    sscanf(line, "%d %d", &image->width, &image->height);
    
    // Get max color value
    line = strtok(NULL, "\n");
    int max_val;
    sscanf(line, "%d", &max_val);
    
    // Allocate image data
    image->data = (uint32_t*)kmalloc(image->width * image->height * 4);
    
    // Read pixel data
    uint8_t* rgb_data = (uint8_t*)kmalloc(image->width * image->height * 3);
    fs_read(fd, rgb_data, image->width * image->height * 3);
    
    // Convert RGB to RGBA
    for(int i = 0; i < image->width * image->height; i++) {
        uint8_t r = rgb_data[i * 3];
        uint8_t g = rgb_data[i * 3 + 1];
        uint8_t b = rgb_data[i * 3 + 2];
        image->data[i] = (0xFF << 24) | (r << 16) | (g << 8) | b;
    }
    
    kfree(rgb_data);
    fs_close(fd);
    
    return true;
}

void draw_ppm_image(uint32_t* buffer, ppm_image_t* image, int x, int y) {
    if(!image || !image->data) return;
    
    for(int dy = 0; dy < image->height; dy++) {
        for(int dx = 0; dx < image->width; dx++) {
            int screen_x = x + dx;
            int screen_y = y + dy;
            
            if(screen_x >= 0 && screen_x < screen_width && 
               screen_y >= 0 && screen_y < screen_height) {
                uint32_t pixel = image->data[dy * image->width + dx];
                buffer[screen_y * screen_width + screen_x] = pixel;
            }
        }
    }
}

void draw_desktop(void) {
    // Clear screen
    fill_rect(framebuffer, 0, 0, screen_width, screen_height, desktop.background_color);
    
    // Draw wallpaper
    if(desktop.wallpaper.data) {
        draw_ppm_image(framebuffer, &desktop.wallpaper, 0, 0);
    }
    
    // Draw desktop icons
    draw_desktop_icons();
    
    // Draw all visible windows
    window_t* window = window_list;
    while(window) {
        if(window->visible) {
            blit_window_to_screen(window);
        }
        window = window->next;
    }
    
    // Draw taskbar
    draw_taskbar();
    
    // Draw system tray
    draw_system_tray();
    
    // Update screen
    update_screen();
}

void draw_taskbar(void) {
    // Fill taskbar background
    fill_rect(framebuffer, desktop.taskbar.x, desktop.taskbar.y,
              desktop.taskbar.width, desktop.taskbar.height,
              desktop.taskbar.background_color);
    
    // Draw start button
    draw_start_button();
    
    // Draw window buttons
    draw_window_buttons();
}

void draw_system_tray(void) {
    // Fill system tray background
    fill_rect(framebuffer, desktop.system_tray.x, desktop.system_tray.y,
              desktop.system_tray.width, desktop.system_tray.height,
              current_theme.system_tray_bg);
    
    // Draw clock
    draw_system_clock();
    
    // Draw system icons
    draw_system_icons();
}

// Graphics primitives
void fill_rect(uint32_t* buffer, int x, int y, int width, int height, uint32_t color) {
    for(int dy = 0; dy < height; dy++) {
        for(int dx = 0; dx < width; dx++) {
            int px = x + dx;
            int py = y + dy;
            
            if(px >= 0 && px < screen_width && py >= 0 && py < screen_height) {
                buffer[py * screen_width + px] = color;
            }
        }
    }
}

void draw_line(uint32_t* buffer, int x1, int y1, int x2, int y2, uint32_t color) {
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = x1 < x2 ? 1 : -1;
    int sy = y1 < y2 ? 1 : -1;
    int err = dx - dy;
    
    while(true) {
        if(x1 >= 0 && x1 < screen_width && y1 >= 0 && y1 < screen_height) {
            buffer[y1 * screen_width + x1] = color;
        }
        
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

void draw_text(uint32_t* buffer, const char* text, int x, int y, uint32_t color, font_t* font) {
    if(!font || !font->data) return;
    
    int cursor_x = x;
    int cursor_y = y;
    
    while(*text) {
        char c = *text++;
        
        if(c == '\n') {
            cursor_x = x;
            cursor_y += font->height;
            continue;
        }
        
        // Draw character
        draw_character(buffer, c, cursor_x, cursor_y, color, font);
        cursor_x += font->width;
    }
}

void init_default_theme(void) {
    current_theme.desktop_bg = 0xFF2E3440;
    current_theme.window_bg = 0xFFECEFF4;
    current_theme.title_bar_active = 0xFF5E81AC;
    current_theme.title_bar_inactive = 0xFF4C566A;
    current_theme.title_text = 0xFFECEFF4;
    current_theme.taskbar_bg = 0xFF3B4252;
    current_theme.system_tray_bg = 0xFF434C5E;
    current_theme.button_bg = 0xFFD8DEE9;
    current_theme.button_hover = 0xFFE5E9F0;
    current_theme.button_pressed = 0xFFBCC5D1;
    current_theme.text_color = 0xFF2E3440;
    current_theme.border_color = 0xFF4C566A;
}

void gui_emergency_mode(void) {
    // Switch to simple text mode for kernel panic
    // This would interface with VGA text mode driver
    init_text_mode();
}