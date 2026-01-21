#ifndef FILE_EXPLORER_H
#define FILE_EXPLORER_H

#include "../../gui/gui.h"
#include "../../fs/fs.h"

#define MAX_DIR_ENTRIES 1024
#define TOOLBAR_HEIGHT 40
#define SIDEBAR_WIDTH 200
#define STATUS_BAR_HEIGHT 25
#define ICON_SIZE 48

#define VIEW_MODE_ICONS 0
#define VIEW_MODE_LIST 1

typedef struct {
    char* name;
    uint64_t size;
    uint32_t type;
    uint32_t permissions;
    uint64_t created;
    uint64_t modified;
    ppm_image_t icon;
    bool selected;
    int x, y; // Display position
} file_item_t;

typedef struct {
    window_t* window;
    char current_path[512];
    
    // UI elements
    struct {
        int x, y;
        uint32_t width, height;
    } toolbar, sidebar, main_view, status_bar;
    
    button_t* back_button;
    button_t* forward_button;
    button_t* up_button;
    textbox_t* address_bar;
    button_t* icon_view_button;
    button_t* list_view_button;
    
    // File items
    file_item_t* items;
    uint32_t item_count;
    int selected_item;
    
    // View settings
    uint32_t view_mode;
    bool show_hidden;
    
    // Icons
    ppm_image_t file_icon;
    ppm_image_t folder_icon;
    ppm_image_t image_icon;
    ppm_image_t text_icon;
    ppm_image_t executable_icon;
    ppm_image_t home_icon;
    ppm_image_t desktop_icon;
    ppm_image_t documents_icon;
    ppm_image_t downloads_icon;
    ppm_image_t drive_icon;
} file_explorer_t;

// Function prototypes
int main(int argc, char* argv[]);
void create_explorer_ui(void);
void load_directory(const char* path);
void draw_explorer_window(void);
void draw_toolbar(void);
void draw_sidebar(void);
void draw_main_view(void);
void draw_icon_view(void);
void draw_list_view(void);
void draw_status_bar(void);

void handle_explorer_mouse_event(mouse_event_t* event);
void handle_main_view_click(int x, int y, mouse_event_t* event);
void handle_toolbar_click(int x, int y, mouse_event_t* event);
void handle_sidebar_click(int x, int y, mouse_event_t* event);

void open_selected_item(void);
void navigate_up(void);
void clear_selection(void);
void show_context_menu(int x, int y);
void create_context_menu(void);

void load_file_icons(void);
void load_default_icon(file_item_t* item);
void calculate_item_position(file_item_t* item, uint32_t index);

void update_status_bar(void);
void show_error_dialog(const char* message);
void open_file_with_default_app(const char* filename);

const char* get_file_type_string(uint32_t type);
void format_file_size(uint64_t size, char* buffer);
void format_date(uint64_t timestamp, char* buffer);
bool is_double_click(mouse_event_t* event);

int run_explorer_loop(void);

// Standard functions
int snprintf(char* buffer, size_t size, const char* format, ...);
char* strrchr(const char* str, int c);
void process_yield(void);

#endif