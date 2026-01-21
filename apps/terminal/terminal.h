#ifndef TERMINAL_H
#define TERMINAL_H

#include "../../gui/gui.h"

#define TERMINAL_ROWS 50
#define TERMINAL_COLS 120
#define MAX_INPUT_LENGTH 1024
#define MAX_HISTORY 100
#define MAX_ARGS 64
#define MAX_COMPLETIONS 50

// Terminal colors
#define TERMINAL_DEFAULT_FG 0xFFE0E0E0
#define TERMINAL_DEFAULT_BG 0xFF1E1E1E
#define TERMINAL_CURSOR_COLOR 0xFFFFFFFF
#define TERMINAL_PROMPT_COLOR 0xFF00FF00
#define TERMINAL_INPUT_COLOR 0xFFFFFFFF
#define TERMINAL_ERROR_COLOR 0xFFFF0000
#define TERMINAL_SUCCESS_COLOR 0xFF00FF00

// Key codes
#define KEY_ENTER 13
#define KEY_BACKSPACE 8
#define KEY_DELETE 127
#define KEY_TAB 9
#define KEY_LEFT 37
#define KEY_RIGHT 39
#define KEY_UP 38
#define KEY_DOWN 40
#define KEY_HOME 36
#define KEY_END 35

typedef struct {
    char character;
    uint32_t foreground;
    uint32_t background;
    uint8_t attributes;
} terminal_char_t;

typedef struct {
    window_t* window;
    terminal_char_t* buffer;
    
    // Terminal dimensions
    int rows, cols;
    int char_width, char_height;
    
    // Cursor position
    int cursor_x, cursor_y;
    int scroll_offset;
    
    // Input handling
    char input_buffer[MAX_INPUT_LENGTH];
    int input_pos;
    int prompt_length;
    
    // Command history
    char history[MAX_HISTORY][MAX_INPUT_LENGTH];
    int history_count;
    int history_pos;
    
    // Font
    font_t* font;
} terminal_t;

// Function prototypes
int main(int argc, char* argv[]);
void init_terminal_buffer(void);
void load_terminal_font(void);

// Display functions
void display_welcome(void);
void display_prompt(void);
void print_line(const char* text);
void print_text(const char* text, uint32_t fg_color, uint32_t bg_color);
void put_char(char c, uint32_t fg_color, uint32_t bg_color);
void newline(void);
void scroll_up(void);

// Drawing functions
void draw_terminal(void);
void draw_cursor(void);
void redraw_input_line(void);

// Input handling
void handle_terminal_keyboard_event(keyboard_event_t* event);
void handle_enter(void);
void handle_character_input(char c);
void handle_backspace(void);
void handle_delete(void);
void handle_left_arrow(void);
void handle_right_arrow(void);
void handle_up_arrow(void);
void handle_down_arrow(void);
void handle_home(void);
void handle_end(void);
void handle_tab(void);

// Command execution
void execute_command(const char* command);
void execute_external_command(int argc, char* args[]);
int parse_command(const char* command, char* args[]);

// Built-in commands
void clear_terminal(void);
void exit_terminal(void);
void show_help(void);

// History management
void add_to_history(const char* command);

// Tab completion
int get_completions(const char* partial, char completions[][256], int max_completions);

// Shell integration
void init_shell(void);
void get_current_directory(char* buffer, size_t size);

// Main loop
int run_terminal_loop(void);

// Standard functions
void memmove(void* dest, const void* src, size_t n);

#endif