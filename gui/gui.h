#ifndef GUI_H
#define GUI_H

#include <stdint.h>
#include <stdbool.h>

// GUI constants
#define MAX_WINDOWS 256
#define TITLE_BAR_HEIGHT 30
#define BORDER_WIDTH 2
#define TASKBAR_HEIGHT 40
#define SYSTEM_TRAY_WIDTH 200

// Window flags
#define WINDOW_RESIZABLE    0x01
#define WINDOW_MINIMIZABLE  0x02
#define WINDOW_MAXIMIZABLE  0x04
#define WINDOW_CLOSABLE     0x08
#define WINDOW_MODAL        0x10
#define WINDOW_TOPMOST      0x20

// Window states
#define WINDOW_NORMAL     0
#define WINDOW_MINIMIZED  1
#define WINDOW_MAXIMIZED  2
#define WINDOW_FULLSCREEN 3

// Event types
#define EVENT_MOUSE     1
#define EVENT_KEYBOARD  2
#define EVENT_PAINT     3
#define EVENT_RESIZE    4
#define EVENT_CLOSE     5
#define EVENT_FOCUS     6
#define EVENT_UNFOCUS   7

// Mouse events
#define MOUSE_MOVE        1
#define MOUSE_BUTTON_DOWN 2
#define MOUSE_BUTTON_UP   3
#define MOUSE_WHEEL       4

// Mouse buttons
#define MOUSE_LEFT   1
#define MOUSE_RIGHT  2
#define MOUSE_MIDDLE 3

// Keyboard events
#define KEY_DOWN 1
#define KEY_UP   2

// PPM image structure
typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t* data;
} ppm_image_t;

// Font structure
typedef struct {
    uint32_t width;
    uint32_t height;
    uint8_t* data;
    char name[64];
} font_t;

// Theme structure
typedef struct {
    uint32_t desktop_bg;
    uint32_t window_bg;
    uint32_t title_bar_active;
    uint32_t title_bar_inactive;
    uint32_t title_text;
    uint32_t taskbar_bg;
    uint32_t system_tray_bg;
    uint32_t button_bg;
    uint32_t button_hover;
    uint32_t button_pressed;
    uint32_t text_color;
    uint32_t border_color;
} theme_t;

// Mouse event structure
typedef struct {
    uint32_t type;
    int x, y;
    int delta_x, delta_y;
    uint32_t button;
    uint32_t modifiers;
} mouse_event_t;

// Keyboard event structure
typedef struct {
    uint32_t type;
    uint32_t keycode;
    uint32_t scancode;
    uint32_t modifiers;
    char character;
} keyboard_event_t;

// Window structure
typedef struct window {
    uint32_t id;
    char title[256];
    int x, y;
    int width, height;
    uint32_t flags;
    uint32_t state;
    bool visible;
    bool focused;
    
    // Restoration data for maximize/minimize
    int restore_x, restore_y;
    int restore_width, restore_height;
    
    // Graphics
    uint32_t* buffer;
    uint32_t title_bar_height;
    uint32_t border_width;
    
    // Icons
    ppm_image_t icon;
    ppm_image_t close_icon;
    ppm_image_t minimize_icon;
    ppm_image_t maximize_icon;
    
    // Process info
    uint32_t process_id;
    
    // Linked list
    struct window* next;
} window_t;

// Desktop structure
typedef struct {
    uint32_t width, height;
    uint32_t background_color;
    ppm_image_t wallpaper;
    
    struct {
        int x, y;
        uint32_t width, height;
        uint32_t background_color;
    } taskbar;
    
    struct {
        int x, y;
        uint32_t width, height;
    } system_tray;
    
    // Desktop icons
    struct desktop_icon {
        char name[256];
        char path[512];
        ppm_image_t icon;
        int x, y;
        bool selected;
        struct desktop_icon* next;
    }* icons;
} desktop_t;

// GUI context
typedef struct {
    uint32_t screen_width;
    uint32_t screen_height;
    uint32_t* framebuffer;
    uint32_t window_count;
} gui_context_t;

// Widget structures
typedef struct widget {
    uint32_t type;
    int x, y;
    uint32_t width, height;
    bool visible;
    bool enabled;
    
    // Event handlers
    void (*on_click)(struct widget* widget, mouse_event_t* event);
    void (*on_paint)(struct widget* widget, uint32_t* buffer);
    
    // Widget-specific data
    void* data;
    
    struct widget* next;
} widget_t;

typedef struct {
    widget_t base;
    char text[256];
    uint32_t text_color;
    uint32_t background_color;
    font_t* font;
} label_t;

typedef struct {
    widget_t base;
    char text[256];
    uint32_t text_color;
    uint32_t background_color;
    uint32_t border_color;
    bool pressed;
    bool hovered;
} button_t;

typedef struct {
    widget_t base;
    char text[1024];
    uint32_t cursor_pos;
    uint32_t selection_start;
    uint32_t selection_end;
    bool focused;
} textbox_t;

// Function prototypes

// GUI initialization
void gui_init(void);
void init_desktop(void);
void init_graphics_driver(void);
void get_screen_info(uint32_t* width, uint32_t* height, uint32_t* bpp);

// Window management
window_t* create_window(const char* title, int x, int y, int width, int height, uint32_t flags);
void destroy_window(window_t* window);
void show_window(window_t* window);
void hide_window(window_t* window);
void move_window(window_t* window, int x, int y);
void resize_window(window_t* window, int width, int height);
void focus_window(window_t* window);
void minimize_window(window_t* window);
void maximize_window(window_t* window);
void toggle_window_maximize(window_t* window);

// Window drawing
void draw_window(window_t* window);
void draw_title_bar(window_t* window);
void draw_window_border(window_t* window);
void draw_window_controls(window_t* window);
void blit_window_to_screen(window_t* window);

// Event handling
void gui_handle_mouse(mouse_event_t* event);
void gui_handle_keyboard(keyboard_event_t* event);
void handle_title_bar_click(window_t* window, int x, int y, mouse_event_t* event);
void handle_desktop_click(mouse_event_t* event);
void send_window_event(window_t* window, uint32_t type, void* data);

// Window utilities
window_t* find_window_at_position(int x, int y);
window_t* find_window_by_id(uint32_t id);
void move_window_to_front(window_t* window);
void focus_next_window(void);
uint32_t generate_window_id(void);

// Desktop management
void draw_desktop(void);
void draw_desktop_icons(void);
void load_desktop_icons(void);
void add_desktop_icon(const char* name, const char* path, const char* icon_path, int x, int y);

// Taskbar
void draw_taskbar(void);
void draw_start_button(void);
void draw_window_buttons(void);
void add_to_taskbar(window_t* window);
void remove_from_taskbar(window_t* window);

// System tray
void draw_system_tray(void);
void draw_system_clock(void);
void draw_system_icons(void);
void add_system_tray_icon(const char* name, ppm_image_t* icon);

// PPM image support
bool load_ppm_image(const char* path, ppm_image_t* image);
void free_ppm_image(ppm_image_t* image);
void draw_ppm_image(uint32_t* buffer, ppm_image_t* image, int x, int y);
void draw_ppm_image_scaled(uint32_t* buffer, ppm_image_t* image, int x, int y, int width, int height);

// Graphics primitives
void fill_rect(uint32_t* buffer, int x, int y, int width, int height, uint32_t color);
void draw_line(uint32_t* buffer, int x1, int y1, int x2, int y2, uint32_t color);
void draw_circle(uint32_t* buffer, int cx, int cy, int radius, uint32_t color);
void draw_rounded_rect(uint32_t* buffer, int x, int y, int width, int height, int radius, uint32_t color);

// Text rendering
void draw_text(uint32_t* buffer, const char* text, int x, int y, uint32_t color, font_t* font);
void draw_character(uint32_t* buffer, char c, int x, int y, uint32_t color, font_t* font);
int get_text_width(const char* text, font_t* font);
int get_text_height(font_t* font);

// Font management
void load_system_fonts(void);
font_t* load_font(const char* path);
void free_font(font_t* font);

// Theme management
void init_default_theme(void);
void load_theme(const char* path);
void apply_theme(theme_t* theme);

// Widget system
widget_t* create_widget(uint32_t type, int x, int y, uint32_t width, uint32_t height);
void destroy_widget(widget_t* widget);
label_t* create_label(const char* text, int x, int y);
button_t* create_button(const char* text, int x, int y, uint32_t width, uint32_t height);
textbox_t* create_textbox(int x, int y, uint32_t width, uint32_t height);

// Screen management
void update_screen(void);
void clear_screen(uint32_t color);
void set_pixel(int x, int y, uint32_t color);
uint32_t get_pixel(int x, int y);

// Window dragging
void start_window_drag(window_t* window, int start_x, int start_y);
void update_window_drag(int current_x, int current_y);
void end_window_drag(void);

// Memory management
void* get_window_cache(void);

// Emergency mode
void gui_emergency_mode(void);
void init_text_mode(void);

// Utility functions
uint32_t rgb_to_color(uint8_t r, uint8_t g, uint8_t b);
void color_to_rgb(uint32_t color, uint8_t* r, uint8_t* g, uint8_t* b);
uint32_t blend_colors(uint32_t color1, uint32_t color2, uint8_t alpha);

// Standard functions
int abs(int x);
int sscanf(const char* str, const char* format, ...);

#endif