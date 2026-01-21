#include "profiler.h"
#include "../../gui/gui.h"
#include "../../kernel/kernel.h"

static profiler_t* profiler = NULL;

int main(int argc, char* argv[]) {
    profiler = (profiler_t*)kmalloc(sizeof(profiler_t));
    if(!profiler) return -1;
    
    init_profiler();
    create_profiler_window();
    
    if(argc > 1) {
        attach_to_process(atoi(argv[1]));
    }
    
    return run_profiler_loop();
}

void init_profiler(void) {
    profiler->state = PROFILER_IDLE;
    profiler->target_pid = 0;
    profiler->sample_count = 0;
    profiler->sample_rate = 1000; // 1000 Hz
    profiler->duration = 10; // 10 seconds
    
    init_symbol_table();
    init_call_graph();
}

void create_profiler_window(void) {
    profiler->window = create_window("Rodmin Performance Profiler", 150, 150, 1200, 800,
                                   WINDOW_RESIZABLE | WINDOW_MINIMIZABLE |
                                   WINDOW_MAXIMIZABLE | WINDOW_CLOSABLE);
    
    create_profiler_toolbar();
    create_flame_graph_view();
    create_function_list();
    create_statistics_panel();
    create_timeline_view();
}

void create_flame_graph_view(void) {
    profiler->flame_graph.x = 0;
    profiler->flame_graph.y = TOOLBAR_HEIGHT;
    profiler->flame_graph.width = 800;
    profiler->flame_graph.height = 400;
    
    profiler->flame_graph.zoom_level = 1.0;
    profiler->flame_graph.offset_x = 0;
    profiler->flame_graph.selected_frame = NULL;
}

int start_profiling(uint32_t pid) {
    if(profiler->state != PROFILER_IDLE) return -1;
    
    profiler->target_pid = pid;
    profiler->state = PROFILER_RUNNING;
    profiler->start_time = get_system_time();
    
    setup_sampling_timer();
    enable_performance_counters();
    
    return 0;
}

void sample_stack_trace(void) {
    if(profiler->sample_count >= MAX_SAMPLES) return;
    
    sample_t* sample = &profiler->samples[profiler->sample_count];
    sample->timestamp = get_system_time();
    sample->pid = profiler->target_pid;
    sample->tid = get_current_thread_id(profiler->target_pid);
    
    capture_stack_trace(profiler->target_pid, sample->stack_trace, &sample->stack_depth);
    capture_cpu_counters(&sample->cpu_counters);
    
    profiler->sample_count++;
}

void capture_stack_trace(uint32_t pid, uint64_t* stack_trace, uint32_t* depth) {
    *depth = 0;
    
    process_t* proc = get_process_by_pid(pid);
    if(!proc) return;
    
    uint64_t rbp = get_process_register(proc, REG_RBP);
    uint64_t rip = get_process_register(proc, REG_RIP);
    
    while(rbp != 0 && *depth < MAX_STACK_DEPTH) {
        stack_trace[*depth] = rip;
        (*depth)++;
        
        uint64_t next_rbp, next_rip;
        if(read_process_memory(proc, rbp, &next_rbp, 8) != 8) break;
        if(read_process_memory(proc, rbp + 8, &next_rip, 8) != 8) break;
        
        rbp = next_rbp;
        rip = next_rip;
    }
}

void build_call_graph(void) {
    clear_call_graph();
    
    for(uint32_t i = 0; i < profiler->sample_count; i++) {
        sample_t* sample = &profiler->samples[i];
        
        for(uint32_t j = 0; j < sample->stack_depth; j++) {
            uint64_t address = sample->stack_trace[j];
            
            call_graph_node_t* node = find_or_create_node(address);
            node->self_time++;
            node->total_time++;
            
            if(j > 0) {
                uint64_t caller_address = sample->stack_trace[j - 1];
                call_graph_node_t* caller = find_or_create_node(caller_address);
                
                add_call_edge(caller, node);
                caller->total_time++;
            }
        }
    }
    
    calculate_percentages();
}

call_graph_node_t* find_or_create_node(uint64_t address) {
    for(uint32_t i = 0; i < profiler->call_graph.node_count; i++) {
        if(profiler->call_graph.nodes[i].address == address) {
            return &profiler->call_graph.nodes[i];
        }
    }
    
    if(profiler->call_graph.node_count >= MAX_CALL_GRAPH_NODES) {
        return NULL;
    }
    
    call_graph_node_t* node = &profiler->call_graph.nodes[profiler->call_graph.node_count];
    node->address = address;
    node->self_time = 0;
    node->total_time = 0;
    node->call_count = 0;
    node->caller_count = 0;
    node->callee_count = 0;
    
    symbol_info_t* symbol = find_symbol_by_address(address);
    if(symbol) {
        strncpy(node->function_name, symbol->name, 127);
    } else {
        snprintf(node->function_name, 128, "0x%lx", address);
    }
    
    profiler->call_graph.node_count++;
    return node;
}

void draw_flame_graph(void) {
    uint32_t* buffer = profiler->window->buffer;
    
    fill_rect(buffer, profiler->flame_graph.x, profiler->flame_graph.y,
              profiler->flame_graph.width, profiler->flame_graph.height, 0xFFFFFFFF);
    
    if(profiler->sample_count == 0) return;
    
    flame_graph_t* fg = build_flame_graph_data();
    
    int y_offset = profiler->flame_graph.y + profiler->flame_graph.height - 20;
    draw_flame_graph_level(buffer, fg->root, 0, profiler->flame_graph.width, 
                          y_offset, 0);
    
    free_flame_graph_data(fg);
}

flame_graph_t* build_flame_graph_data(void) {
    flame_graph_t* fg = (flame_graph_t*)kmalloc(sizeof(flame_graph_t));
    fg->root = (flame_node_t*)kmalloc(sizeof(flame_node_t));
    fg->root->name = strdup("root");
    fg->root->value = profiler->sample_count;
    fg->root->children = NULL;
    fg->root->child_count = 0;
    
    for(uint32_t i = 0; i < profiler->sample_count; i++) {
        sample_t* sample = &profiler->samples[i];
        add_sample_to_flame_graph(fg->root, sample);
    }
    
    return fg;
}

void add_sample_to_flame_graph(flame_node_t* node, sample_t* sample) {
    for(int i = sample->stack_depth - 1; i >= 0; i--) {
        uint64_t address = sample->stack_trace[i];
        
        symbol_info_t* symbol = find_symbol_by_address(address);
        const char* func_name = symbol ? symbol->name : "unknown";
        
        flame_node_t* child = find_flame_child(node, func_name);
        if(!child) {
            child = create_flame_child(node, func_name);
        }
        
        child->value++;
        node = child;
    }
}

void draw_flame_graph_level(uint32_t* buffer, flame_node_t* node, 
                           int x, int width, int y, int level) {
    if(node->child_count == 0) return;
    
    int current_x = x;
    
    for(uint32_t i = 0; i < node->child_count; i++) {
        flame_node_t* child = &node->children[i];
        
        int child_width = (width * child->value) / node->value;
        if(child_width < 1) child_width = 1;
        
        uint32_t color = get_flame_color(child->name, level);
        
        fill_rect(buffer, current_x, y - 20, child_width, 20, color);
        
        draw_rect_border(buffer, current_x, y - 20, child_width, 20, 0xFF000000);
        
        if(child_width > 50) {
            draw_text(buffer, child->name, current_x + 2, y - 15, 
                     0xFF000000, &system_font);
        }
        
        draw_flame_graph_level(buffer, child, current_x, child_width, 
                              y - 20, level + 1);
        
        current_x += child_width;
    }
}

uint32_t get_flame_color(const char* function_name, int level) {
    uint32_t hash = string_hash(function_name);
    
    uint8_t r = 200 + (hash % 56);
    uint8_t g = 100 + ((hash >> 8) % 100);
    uint8_t b = 50 + ((hash >> 16) % 50);
    
    return (0xFF << 24) | (r << 16) | (g << 8) | b;
}

void create_function_list(void) {
    profiler->function_list.x = 800;
    profiler->function_list.y = TOOLBAR_HEIGHT;
    profiler->function_list.width = 400;
    profiler->function_list.height = 400;
    
    profiler->function_table = create_table_view(profiler->function_list.x,
                                               profiler->function_list.y,
                                               profiler->function_list.width,
                                               profiler->function_list.height);
    
    add_table_column(profiler->function_table, "Function", 200);
    add_table_column(profiler->function_table, "Self %", 60);
    add_table_column(profiler->function_table, "Total %", 60);
    add_table_column(profiler->function_table, "Calls", 80);
}

void update_function_list(void) {
    clear_table(profiler->function_table);
    
    function_stats_t* stats = calculate_function_statistics();
    
    qsort(stats, profiler->call_graph.node_count, sizeof(function_stats_t),
          compare_by_self_time);
    
    for(uint32_t i = 0; i < profiler->call_graph.node_count; i++) {
        function_stats_t* stat = &stats[i];
        
        char self_percent[16], total_percent[16], calls[16];
        snprintf(self_percent, 16, "%.2f%%", stat->self_percent);
        snprintf(total_percent, 16, "%.2f%%", stat->total_percent);
        snprintf(calls, 16, "%u", stat->call_count);
        
        const char* row_data[] = {
            stat->function_name,
            self_percent,
            total_percent,
            calls
        };
        
        add_table_row(profiler->function_table, row_data);
    }
    
    kfree(stats);
}

void create_timeline_view(void) {
    profiler->timeline.x = 0;
    profiler->timeline.y = TOOLBAR_HEIGHT + 400;
    profiler->timeline.width = 1200;
    profiler->timeline.height = 200;
}

void draw_timeline(void) {
    uint32_t* buffer = profiler->window->buffer;
    
    fill_rect(buffer, profiler->timeline.x, profiler->timeline.y,
              profiler->timeline.width, profiler->timeline.height, 0xFFFFFFFF);
    
    if(profiler->sample_count == 0) return;
    
    uint64_t start_time = profiler->samples[0].timestamp;
    uint64_t end_time = profiler->samples[profiler->sample_count - 1].timestamp;
    uint64_t duration = end_time - start_time;
    
    for(uint32_t i = 0; i < profiler->sample_count; i++) {
        sample_t* sample = &profiler->samples[i];
        
        int x = profiler->timeline.x + 
                ((sample->timestamp - start_time) * profiler->timeline.width) / duration;
        
        uint32_t color = get_cpu_usage_color(sample->cpu_counters.cpu_usage);
        
        draw_line(buffer, x, profiler->timeline.y, 
                 x, profiler->timeline.y + profiler->timeline.height, color);
    }
    
    draw_timeline_labels(buffer, start_time, end_time);
}

int stop_profiling(void) {
    if(profiler->state != PROFILER_RUNNING) return -1;
    
    disable_sampling_timer();
    disable_performance_counters();
    
    profiler->state = PROFILER_ANALYZING;
    
    build_call_graph();
    update_function_list();
    
    profiler->state = PROFILER_COMPLETE;
    
    return 0;
}

int run_profiler_loop(void) {
    while(true) {
        if(profiler->window->state == WINDOW_CLOSED) break;
        
        handle_profiler_events();
        
        if(profiler->state == PROFILER_RUNNING) {
            check_profiling_timeout();
        }
        
        update_profiler_display();
        
        process_yield();
    }
    
    cleanup_profiler();
    return 0;
}