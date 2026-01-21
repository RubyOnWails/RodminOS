#include "visual_debugger.h"
#include "../../gui/gui.h"
#include "../../kernel/kernel.h"

static debugger_t* debugger = NULL;

int main(int argc, char* argv[]) {
    debugger = (debugger_t*)kmalloc(sizeof(debugger_t));
    if(!debugger) return -1;
    
    init_debugger_context();
    create_debugger_window();
    
    if(argc > 1) {
        load_executable(argv[1]);
    }
    
    return run_debugger_loop();
}

void init_debugger_context(void) {
    debugger->state = DEBUG_STATE_IDLE;
    debugger->target_process = NULL;
    debugger->breakpoint_count = 0;
    debugger->watchpoint_count = 0;
    debugger->call_stack_depth = 0;
    debugger->variable_count = 0;
}

void create_debugger_window(void) {
    debugger->window = create_window("Rodmin Visual Debugger", 100, 100, 1400, 900,
                                   WINDOW_RESIZABLE | WINDOW_MINIMIZABLE |
                                   WINDOW_MAXIMIZABLE | WINDOW_CLOSABLE);
    
    create_debugger_menu();
    create_debugger_toolbar();
    create_source_view();
    create_variables_panel();
    create_call_stack_panel();
    create_breakpoints_panel();
    create_memory_view();
    create_registers_panel();
    create_console_panel();
}

int load_executable(const char* path) {
    if(debugger->target_process) {
        detach_from_process();
    }
    
    executable_info_t exe_info;
    if(parse_executable(path, &exe_info) != 0) {
        show_error_dialog("Failed to load executable");
        return -1;
    }
    
    debugger->executable_path = strdup(path);
    debugger->exe_info = exe_info;
    
    load_source_files(&exe_info);
    load_debug_symbols(&exe_info);
    
    return 0;
}

int start_debugging(void) {
    if(!debugger->executable_path) {
        show_error_dialog("No executable loaded");
        return -1;
    }
    
    debug_process_t* proc = launch_debug_process(debugger->executable_path);
    if(!proc) {
        show_error_dialog("Failed to start process");
        return -1;
    }
    
    debugger->target_process = proc;
    debugger->state = DEBUG_STATE_RUNNING;
    
    function_symbol_t* main_func = find_function_symbol("main");
    if(main_func) {
        set_breakpoint_at_address(main_func->address);
    }
    
    update_debugger_display();
    
    return 0;
}

void handle_breakpoint_hit(debug_event_t* event) {
    debugger->state = DEBUG_STATE_PAUSED;
    debugger->current_address = event->address;
    
    source_location_t location;
    if(find_source_location(event->address, &location)) {
        highlight_source_line(location.file, location.line);
    }
    
    update_call_stack();
    update_variable_values();
    update_register_display();
    update_memory_view(event->address);
    
    update_debugger_display();
}

int run_debugger_loop(void) {
    while(true) {
        if(debugger->window->state == WINDOW_CLOSED) break;
        
        handle_debugger_events();
        
        if(debugger->target_process) {
            check_debug_events();
        }
        
        update_debugger_display();
        
        process_yield();
    }
    
    cleanup_debugger();
    return 0;
}