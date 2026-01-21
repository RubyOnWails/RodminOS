#include "terminal.h"
#include "../../gui/gui.h"
#include "../../kernel/kernel.h"
#include "../../cli/shell.h"

static terminal_t* terminal = NULL;

int main(int argc, char* argv[]) {
    // Initialize terminal
    terminal = (terminal_t*)kmalloc(sizeof(terminal_t));
    if(!terminal) return -1;
    
    // Initialize terminal state
    terminal->cursor_x = 0;
    terminal->cursor_y = 0;
    terminal->scroll_offset = 0;
    terminal->input_pos = 0;
    terminal->history_pos = 0;
    terminal->history_count = 0;
    
    // Create main window
    terminal->window = create_window("Rodmin Terminal", 200, 150, 800, 500,
                                   WINDOW_RESIZABLE | WINDOW_MINIMIZABLE |
                                   WINDOW_MAXIMIZABLE | WINDOW_CLOSABLE);
    
    if(!terminal->window) {
        kfree(terminal);
        return -1;
    }
    
    // Initialize terminal buffer
    init_terminal_buffer();
    
    // Load terminal font
    load_terminal_font();
    
    // Initialize shell
    init_shell();
    
    // Display welcome message
    display_welcome();
    
    // Display prompt
    display_prompt();
    
    // Main event loop
    return run_terminal_loop();
}

void init_terminal_buffer(void) {
    // Allocate terminal buffer
    terminal->buffer = (terminal_char_t*)kcalloc(TERMINAL_ROWS * TERMINAL_COLS, 
                                               sizeof(terminal_char_t));
    
    // Initialize with spaces
    for(int i = 0; i < TERMINAL_ROWS * TERMINAL_COLS; i++) {
        terminal->buffer[i].character = ' ';
        terminal->buffer[i].foreground = TERMINAL_DEFAULT_FG;
        terminal->buffer[i].background = TERMINAL_DEFAULT_BG;
        terminal->buffer[i].attributes = 0;
    }
    
    // Initialize input buffer
    terminal->input_buffer[0] = '\0';
    
    // Initialize command history
    for(int i = 0; i < MAX_HISTORY; i++) {
        terminal->history[i][0] = '\0';
    }
}

void load_terminal_font(void) {
    // Load monospace font for terminal
    terminal->font = load_font("/system/fonts/mono.font");
    if(!terminal->font) {
        // Fallback to system font
        terminal->font = &system_font;
    }
    
    // Calculate character dimensions
    terminal->char_width = terminal->font->width;
    terminal->char_height = terminal->font->height;
    
    // Calculate terminal dimensions in characters
    terminal->cols = (terminal->window->width - 20) / terminal->char_width;
    terminal->rows = (terminal->window->height - TITLE_BAR_HEIGHT - 20) / terminal->char_height;
}

void display_welcome(void) {
    print_line("Rodmin OS Terminal v1.0");
    print_line("Type 'help' for available commands.");
    print_line("");
}

void display_prompt(void) {
    char prompt[256];
    char cwd[512];
    
    // Get current working directory
    get_current_directory(cwd, sizeof(cwd));
    
    // Format prompt: user@hostname:path$
    snprintf(prompt, sizeof(prompt), "user@rodmin:%s$ ", cwd);
    
    print_text(prompt, TERMINAL_PROMPT_COLOR, TERMINAL_DEFAULT_BG);
    terminal->prompt_length = strlen(prompt);
}

void print_line(const char* text) {
    print_text(text, TERMINAL_DEFAULT_FG, TERMINAL_DEFAULT_BG);
    newline();
}

void print_text(const char* text, uint32_t fg_color, uint32_t bg_color) {
    while(*text) {
        if(*text == '\n') {
            newline();
        } else if(*text == '\t') {
            // Tab to next 8-character boundary
            int spaces = 8 - (terminal->cursor_x % 8);
            for(int i = 0; i < spaces; i++) {
                put_char(' ', fg_color, bg_color);
            }
        } else {
            put_char(*text, fg_color, bg_color);
        }
        text++;
    }
    
    draw_terminal();
}

void put_char(char c, uint32_t fg_color, uint32_t bg_color) {
    if(terminal->cursor_x >= terminal->cols) {
        newline();
    }
    
    int index = terminal->cursor_y * terminal->cols + terminal->cursor_x;
    
    if(index < TERMINAL_ROWS * TERMINAL_COLS) {
        terminal->buffer[index].character = c;
        terminal->buffer[index].foreground = fg_color;
        terminal->buffer[index].background = bg_color;
        terminal->buffer[index].attributes = 0;
    }
    
    terminal->cursor_x++;
}

void newline(void) {
    terminal->cursor_x = 0;
    terminal->cursor_y++;
    
    // Scroll if necessary
    if(terminal->cursor_y >= terminal->rows) {
        scroll_up();
        terminal->cursor_y = terminal->rows - 1;
    }
}

void scroll_up(void) {
    // Move all lines up by one
    for(int y = 0; y < terminal->rows - 1; y++) {
        for(int x = 0; x < terminal->cols; x++) {
            int src_index = (y + 1) * terminal->cols + x;
            int dst_index = y * terminal->cols + x;
            terminal->buffer[dst_index] = terminal->buffer[src_index];
        }
    }
    
    // Clear bottom line
    for(int x = 0; x < terminal->cols; x++) {
        int index = (terminal->rows - 1) * terminal->cols + x;
        terminal->buffer[index].character = ' ';
        terminal->buffer[index].foreground = TERMINAL_DEFAULT_FG;
        terminal->buffer[index].background = TERMINAL_DEFAULT_BG;
        terminal->buffer[index].attributes = 0;
    }
}

void draw_terminal(void) {
    uint32_t* buffer = terminal->window->buffer;
    
    // Clear window background
    fill_rect(buffer, 0, TITLE_BAR_HEIGHT, terminal->window->width,
              terminal->window->height - TITLE_BAR_HEIGHT, TERMINAL_DEFAULT_BG);
    
    // Draw terminal text
    for(int y = 0; y < terminal->rows; y++) {
        for(int x = 0; x < terminal->cols; x++) {
            int index = y * terminal->cols + x;
            terminal_char_t* tc = &terminal->buffer[index];
            
            int pixel_x = 10 + x * terminal->char_width;
            int pixel_y = TITLE_BAR_HEIGHT + 10 + y * terminal->char_height;
            
            // Draw character background
            if(tc->background != TERMINAL_DEFAULT_BG) {
                fill_rect(buffer, pixel_x, pixel_y, terminal->char_width, 
                         terminal->char_height, tc->background);
            }
            
            // Draw character
            if(tc->character != ' ') {
                draw_character(buffer, tc->character, pixel_x, pixel_y, 
                             tc->foreground, terminal->font);
            }
        }
    }
    
    // Draw cursor
    draw_cursor();
    
    // Update screen
    blit_window_to_screen(terminal->window);
}

void draw_cursor(void) {
    uint32_t* buffer = terminal->window->buffer;
    
    int pixel_x = 10 + terminal->cursor_x * terminal->char_width;
    int pixel_y = TITLE_BAR_HEIGHT + 10 + terminal->cursor_y * terminal->char_height;
    
    // Draw blinking cursor
    static bool cursor_visible = true;
    static uint64_t last_blink = 0;
    uint64_t current_time = get_system_time();
    
    if(current_time - last_blink > 500) { // 500ms blink rate
        cursor_visible = !cursor_visible;
        last_blink = current_time;
    }
    
    if(cursor_visible) {
        fill_rect(buffer, pixel_x, pixel_y + terminal->char_height - 2,
                 terminal->char_width, 2, TERMINAL_CURSOR_COLOR);
    }
}

void handle_terminal_keyboard_event(keyboard_event_t* event) {
    if(event->type != KEY_DOWN) return;
    
    switch(event->keycode) {
        case KEY_ENTER:
            handle_enter();
            break;
            
        case KEY_BACKSPACE:
            handle_backspace();
            break;
            
        case KEY_DELETE:
            handle_delete();
            break;
            
        case KEY_LEFT:
            handle_left_arrow();
            break;
            
        case KEY_RIGHT:
            handle_right_arrow();
            break;
            
        case KEY_UP:
            handle_up_arrow();
            break;
            
        case KEY_DOWN:
            handle_down_arrow();
            break;
            
        case KEY_HOME:
            handle_home();
            break;
            
        case KEY_END:
            handle_end();
            break;
            
        case KEY_TAB:
            handle_tab();
            break;
            
        default:
            if(event->character >= 32 && event->character < 127) {
                handle_character_input(event->character);
            }
            break;
    }
    
    draw_terminal();
}

void handle_enter(void) {
    // Echo the input
    newline();
    
    // Add to history if not empty
    if(strlen(terminal->input_buffer) > 0) {
        add_to_history(terminal->input_buffer);
    }
    
    // Execute command
    execute_command(terminal->input_buffer);
    
    // Clear input buffer
    terminal->input_buffer[0] = '\0';
    terminal->input_pos = 0;
    
    // Display new prompt
    display_prompt();
}

void handle_character_input(char c) {
    if(terminal->input_pos < MAX_INPUT_LENGTH - 1) {
        // Insert character at cursor position
        memmove(&terminal->input_buffer[terminal->input_pos + 1],
                &terminal->input_buffer[terminal->input_pos],
                strlen(&terminal->input_buffer[terminal->input_pos]) + 1);
        
        terminal->input_buffer[terminal->input_pos] = c;
        terminal->input_pos++;
        
        // Redraw input line
        redraw_input_line();
    }
}

void handle_backspace(void) {
    if(terminal->input_pos > 0) {
        terminal->input_pos--;
        
        // Remove character
        memmove(&terminal->input_buffer[terminal->input_pos],
                &terminal->input_buffer[terminal->input_pos + 1],
                strlen(&terminal->input_buffer[terminal->input_pos + 1]) + 1);
        
        // Redraw input line
        redraw_input_line();
    }
}

void handle_up_arrow(void) {
    // Navigate command history backwards
    if(terminal->history_pos > 0) {
        terminal->history_pos--;
        strcpy(terminal->input_buffer, terminal->history[terminal->history_pos]);
        terminal->input_pos = strlen(terminal->input_buffer);
        redraw_input_line();
    }
}

void handle_down_arrow(void) {
    // Navigate command history forwards
    if(terminal->history_pos < terminal->history_count - 1) {
        terminal->history_pos++;
        strcpy(terminal->input_buffer, terminal->history[terminal->history_pos]);
        terminal->input_pos = strlen(terminal->input_buffer);
        redraw_input_line();
    } else if(terminal->history_pos == terminal->history_count - 1) {
        terminal->history_pos = terminal->history_count;
        terminal->input_buffer[0] = '\0';
        terminal->input_pos = 0;
        redraw_input_line();
    }
}

void handle_tab(void) {
    // Tab completion
    char completions[MAX_COMPLETIONS][256];
    int completion_count = get_completions(terminal->input_buffer, completions, MAX_COMPLETIONS);
    
    if(completion_count == 1) {
        // Single completion - auto-complete
        strcpy(terminal->input_buffer, completions[0]);
        terminal->input_pos = strlen(terminal->input_buffer);
        redraw_input_line();
    } else if(completion_count > 1) {
        // Multiple completions - show list
        newline();
        for(int i = 0; i < completion_count; i++) {
            print_line(completions[i]);
        }
        display_prompt();
        redraw_input_line();
    }
}

void redraw_input_line(void) {
    // Clear current input line
    int start_x = terminal->prompt_length;
    for(int x = start_x; x < terminal->cols; x++) {
        int index = terminal->cursor_y * terminal->cols + x;
        terminal->buffer[index].character = ' ';
        terminal->buffer[index].foreground = TERMINAL_DEFAULT_FG;
        terminal->buffer[index].background = TERMINAL_DEFAULT_BG;
    }
    
    // Redraw input text
    terminal->cursor_x = terminal->prompt_length;
    print_text(terminal->input_buffer, TERMINAL_INPUT_COLOR, TERMINAL_DEFAULT_BG);
    
    // Position cursor
    terminal->cursor_x = terminal->prompt_length + terminal->input_pos;
}

void execute_command(const char* command) {
    if(strlen(command) == 0) return;
    
    // Parse command and arguments
    char* args[MAX_ARGS];
    int argc = parse_command(command, args);
    
    if(argc == 0) return;
    
    // Execute built-in commands
    if(strcmp(args[0], "clear") == 0) {
        clear_terminal();
    } else if(strcmp(args[0], "exit") == 0) {
        exit_terminal();
    } else if(strcmp(args[0], "help") == 0) {
        show_help();
    } else {
        // Execute external command
        execute_external_command(argc, args);
    }
}

void clear_terminal(void) {
    // Clear terminal buffer
    for(int i = 0; i < TERMINAL_ROWS * TERMINAL_COLS; i++) {
        terminal->buffer[i].character = ' ';
        terminal->buffer[i].foreground = TERMINAL_DEFAULT_FG;
        terminal->buffer[i].background = TERMINAL_DEFAULT_BG;
    }
    
    terminal->cursor_x = 0;
    terminal->cursor_y = 0;
}

void show_help(void) {
    print_line("Available commands:");
    print_line("  clear    - Clear the terminal");
    print_line("  exit     - Exit the terminal");
    print_line("  help     - Show this help message");
    print_line("  ls       - List directory contents");
    print_line("  cd       - Change directory");
    print_line("  cat      - Display file contents");
    print_line("  mkdir    - Create directory");
    print_line("  rm       - Remove file");
    print_line("  cp       - Copy file");
    print_line("  mv       - Move/rename file");
    print_line("  ps       - List processes");
    print_line("  top      - System monitor");
    print_line("");
}

void add_to_history(const char* command) {
    // Don't add duplicate consecutive commands
    if(terminal->history_count > 0 && 
       strcmp(terminal->history[terminal->history_count - 1], command) == 0) {
        return;
    }
    
    // Add to history
    if(terminal->history_count < MAX_HISTORY) {
        strcpy(terminal->history[terminal->history_count], command);
        terminal->history_count++;
    } else {
        // Shift history up
        for(int i = 0; i < MAX_HISTORY - 1; i++) {
            strcpy(terminal->history[i], terminal->history[i + 1]);
        }
        strcpy(terminal->history[MAX_HISTORY - 1], command);
    }
    
    terminal->history_pos = terminal->history_count;
}

int run_terminal_loop(void) {
    // Main event loop
    while(true) {
        // Handle window events
        // This would be integrated with the main GUI event system
        
        // Check for window close
        if(terminal->window->state == WINDOW_CLOSED) {
            break;
        }
        
        // Update cursor blink
        draw_cursor();
        
        // Yield to other processes
        process_yield();
    }
    
    // Cleanup
    destroy_window(terminal->window);
    kfree(terminal->buffer);
    kfree(terminal);
    
    return 0;
}