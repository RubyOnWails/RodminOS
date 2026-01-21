#include "file_explorer.h"
#include "../../gui/gui.h"
#include "../../fs/fs.h"
#include "../../kernel/kernel.h"

static file_explorer_t* explorer = NULL;

int main(int argc, char* argv[]) {
    // Initialize file explorer
    explorer = (file_explorer_t*)kmalloc(sizeof(file_explorer_t));
    if(!explorer) return -1;
    
    // Initialize explorer state
    strcpy(explorer->current_path, "/");
    explorer->selected_item = -1;
    explorer->view_mode = VIEW_MODE_ICONS;
    explorer->show_hidden = false;
    
    // Create main window
    explorer->window = create_window("Rodmin File Explorer", 100, 100, 800, 600, 
                                   WINDOW_RESIZABLE | WINDOW_MINIMIZABLE | 
                                   WINDOW_MAXIMIZABLE | WINDOW_CLOSABLE);
    
    if(!explorer->window) {
        kfree(explorer);
        return -1;
    }
    
    // Load icons
    load_file_icons();
    
    // Create UI elements
    create_explorer_ui();
    
    // Load initial directory
    load_directory(explorer->current_path);
    
    // Main event loop
    return run_explorer_loop();
}

void create_explorer_ui(void) {
    // Create toolbar
    explorer->toolbar.x = 0;
    explorer->toolbar.y = TITLE_BAR_HEIGHT;
    explorer->toolbar.width = explorer->window->width;
    explorer->toolbar.height = TOOLBAR_HEIGHT;
    
    // Create navigation buttons
    explorer->back_button = create_button("Back", 10, TITLE_BAR_HEIGHT + 5, 60, 30);
    explorer->forward_button = create_button("Forward", 80, TITLE_BAR_HEIGHT + 5, 60, 30);
    explorer->up_button = create_button("Up", 150, TITLE_BAR_HEIGHT + 5, 60, 30);
    
    // Create address bar
    explorer->address_bar = create_textbox(220, TITLE_BAR_HEIGHT + 5, 400, 30);
    strcpy(explorer->address_bar->text, explorer->current_path);
    
    // Create view mode buttons
    explorer->icon_view_button = create_button("Icons", 630, TITLE_BAR_HEIGHT + 5, 50, 30);
    explorer->list_view_button = create_button("List", 690, TITLE_BAR_HEIGHT + 5, 50, 30);
    
    // Create sidebar
    explorer->sidebar.x = 0;
    explorer->sidebar.y = TITLE_BAR_HEIGHT + TOOLBAR_HEIGHT;
    explorer->sidebar.width = SIDEBAR_WIDTH;
    explorer->sidebar.height = explorer->window->height - TITLE_BAR_HEIGHT - TOOLBAR_HEIGHT - STATUS_BAR_HEIGHT;
    
    // Create main view area
    explorer->main_view.x = SIDEBAR_WIDTH;
    explorer->main_view.y = TITLE_BAR_HEIGHT + TOOLBAR_HEIGHT;
    explorer->main_view.width = explorer->window->width - SIDEBAR_WIDTH;
    explorer->main_view.height = explorer->window->height - TITLE_BAR_HEIGHT - TOOLBAR_HEIGHT - STATUS_BAR_HEIGHT;
    
    // Create status bar
    explorer->status_bar.x = 0;
    explorer->status_bar.y = explorer->window->height - STATUS_BAR_HEIGHT;
    explorer->status_bar.width = explorer->window->width;
    explorer->status_bar.height = STATUS_BAR_HEIGHT;
    
    // Create context menu
    create_context_menu();
}

void load_directory(const char* path) {
    // Clear current items
    if(explorer->items) {
        for(uint32_t i = 0; i < explorer->item_count; i++) {
            kfree(explorer->items[i].name);
        }
        kfree(explorer->items);
    }
    
    // Read directory contents
    dirent_info_t entries[MAX_DIR_ENTRIES];
    uint32_t count = MAX_DIR_ENTRIES;
    
    if(fs_readdir(path, entries, &count) != 0) {
        show_error_dialog("Failed to read directory");
        return;
    }
    
    // Allocate items array
    explorer->items = (file_item_t*)kmalloc(count * sizeof(file_item_t));
    explorer->item_count = 0;
    
    // Process entries
    for(uint32_t i = 0; i < count; i++) {
        // Skip hidden files if not showing them
        if(!explorer->show_hidden && entries[i].name[0] == '.') {
            continue;
        }
        
        file_item_t* item = &explorer->items[explorer->item_count];
        
        // Copy basic info
        item->name = (char*)kmalloc(strlen(entries[i].name) + 1);
        strcpy(item->name, entries[i].name);
        item->size = entries[i].size;
        item->type = entries[i].type;
        item->permissions = entries[i].permissions;
        item->created = entries[i].created;
        item->modified = entries[i].modified;
        item->selected = false;
        
        // Load icon
        if(strlen(entries[i].icon_path) > 0) {
            load_ppm_image(entries[i].icon_path, &item->icon);
        } else {
            // Use default icon based on type
            load_default_icon(item);
        }
        
        // Calculate display position
        calculate_item_position(item, explorer->item_count);
        
        explorer->item_count++;
    }
    
    // Update address bar
    strcpy(explorer->address_bar->text, path);
    strcpy(explorer->current_path, path);
    
    // Update status bar
    update_status_bar();
    
    // Redraw window
    draw_explorer_window();
}

void draw_explorer_window(void) {
    uint32_t* buffer = explorer->window->buffer;
    
    // Clear window
    fill_rect(buffer, 0, TITLE_BAR_HEIGHT, explorer->window->width, 
              explorer->window->height - TITLE_BAR_HEIGHT, 0xFFFFFFFF);
    
    // Draw toolbar
    draw_toolbar();
    
    // Draw sidebar
    draw_sidebar();
    
    // Draw main view
    draw_main_view();
    
    // Draw status bar
    draw_status_bar();
    
    // Update screen
    blit_window_to_screen(explorer->window);
}

void open_selected_item(void) {
    if(explorer->selected_item == -1) return;
    
    file_item_t* item = &explorer->items[explorer->selected_item];
    
    if(item->type == DIRENT_TYPE_DIR) {
        // Navigate to directory
        char new_path[512];
        if(strcmp(explorer->current_path, "/") == 0) {
            snprintf(new_path, 512, "/%s", item->name);
        } else {
            snprintf(new_path, 512, "%s/%s", explorer->current_path, item->name);
        }
        
        load_directory(new_path);
    } else {
        // Open file with default application
        open_file_with_default_app(item->name);
    }
}

int run_explorer_loop(void) {
    // Main event loop
    while(true) {
        // Handle window events
        // This would be integrated with the main GUI event system
        
        // Check for window close
        if(explorer->window->state == WINDOW_CLOSED) {
            break;
        }
        
        // Yield to other processes
        process_yield();
    }
    
    // Cleanup
    destroy_window(explorer->window);
    kfree(explorer);
    
    return 0;
}