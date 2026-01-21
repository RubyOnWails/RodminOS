#include "shell.h"
#include "../kernel/kernel.h"
#include "../fs/fs.h"
#include "../kernel/process.h"

static shell_context_t shell_ctx;

void init_shell(void) {
    // Initialize shell context
    strcpy(shell_ctx.cwd, "/");
    shell_ctx.last_exit_code = 0;
    
    // Initialize environment variables
    init_environment();
    
    // Initialize built-in commands
    init_builtin_commands();
    
    kprintf("Shell initialized\n");
}

void init_environment(void) {
    shell_ctx.env_count = 0;
    
    // Set default environment variables
    set_env_var("PATH", "/bin:/usr/bin:/system/bin");
    set_env_var("HOME", "/home/user");
    set_env_var("USER", "user");
    set_env_var("SHELL", "/bin/rsh");
    set_env_var("TERM", "rodmin-terminal");
    set_env_var("PS1", "\\u@\\h:\\w$ ");
}

void init_builtin_commands(void) {
    shell_ctx.builtin_count = 0;
    
    // Register built-in commands
    register_builtin("cd", cmd_cd);
    register_builtin("pwd", cmd_pwd);
    register_builtin("ls", cmd_ls);
    register_builtin("cat", cmd_cat);
    register_builtin("echo", cmd_echo);
    register_builtin("mkdir", cmd_mkdir);
    register_builtin("rmdir", cmd_rmdir);
    register_builtin("rm", cmd_rm);
    register_builtin("cp", cmd_cp);
    register_builtin("mv", cmd_mv);
    register_builtin("chmod", cmd_chmod);
    register_builtin("chown", cmd_chown);
    register_builtin("ps", cmd_ps);
    register_builtin("kill", cmd_kill);
    register_builtin("top", cmd_top);
    register_builtin("mount", cmd_mount);
    register_builtin("umount", cmd_umount);
    register_builtin("df", cmd_df);
    register_builtin("free", cmd_free);
    register_builtin("uname", cmd_uname);
    register_builtin("date", cmd_date);
    register_builtin("whoami", cmd_whoami);
    register_builtin("env", cmd_env);
    register_builtin("export", cmd_export);
    register_builtin("unset", cmd_unset);
    register_builtin("history", cmd_history);
    register_builtin("alias", cmd_alias);
    register_builtin("which", cmd_which);
    register_builtin("find", cmd_find);
    register_builtin("grep", cmd_grep);
    register_builtin("head", cmd_head);
    register_builtin("tail", cmd_tail);
    register_builtin("wc", cmd_wc);
    register_builtin("sort", cmd_sort);
    register_builtin("uniq", cmd_uniq);
    register_builtin("cut", cmd_cut);
    register_builtin("sed", cmd_sed);
    register_builtin("awk", cmd_awk);
    register_builtin("tar", cmd_tar);
    register_builtin("gzip", cmd_gzip);
    register_builtin("gunzip", cmd_gunzip);
    register_builtin("wget", cmd_wget);
    register_builtin("curl", cmd_curl);
    register_builtin("ssh", cmd_ssh);
    register_builtin("scp", cmd_scp);
    register_builtin("ping", cmd_ping);
    register_builtin("netstat", cmd_netstat);
    register_builtin("ifconfig", cmd_ifconfig);
    register_builtin("ppmview", cmd_ppmview);
    register_builtin("diskutil", cmd_diskutil);
}

int execute_command_line(const char* cmdline) {
    if(!cmdline || strlen(cmdline) == 0) {
        return 0;
    }
    
    // Parse command line
    command_t cmd;
    if(parse_command_line(cmdline, &cmd) != 0) {
        return -1;
    }
    
    // Handle pipes and redirections
    if(cmd.pipe_count > 0) {
        return execute_pipeline(&cmd);
    }
    
    // Execute single command
    return execute_single_command(&cmd);
}

int parse_command_line(const char* cmdline, command_t* cmd) {
    // Initialize command structure
    cmd->argc = 0;
    cmd->pipe_count = 0;
    cmd->input_redirect = NULL;
    cmd->output_redirect = NULL;
    cmd->append_output = false;
    cmd->background = false;
    
    char* line = strdup(cmdline);
    char* token;
    char* saveptr;
    
    // Check for background execution
    int len = strlen(line);
    if(len > 0 && line[len - 1] == '&') {
        cmd->background = true;
        line[len - 1] = '\0';
    }
    
    // Parse tokens
    token = strtok_r(line, " \t", &saveptr);
    while(token && cmd->argc < MAX_ARGS - 1) {
        if(strcmp(token, "|") == 0) {
            // Pipe
            cmd->pipe_positions[cmd->pipe_count++] = cmd->argc;
        } else if(strcmp(token, "<") == 0) {
            // Input redirection
            token = strtok_r(NULL, " \t", &saveptr);
            if(token) {
                cmd->input_redirect = strdup(token);
            }
        } else if(strcmp(token, ">") == 0) {
            // Output redirection
            token = strtok_r(NULL, " \t", &saveptr);
            if(token) {
                cmd->output_redirect = strdup(token);
                cmd->append_output = false;
            }
        } else if(strcmp(token, ">>") == 0) {
            // Append output redirection
            token = strtok_r(NULL, " \t", &saveptr);
            if(token) {
                cmd->output_redirect = strdup(token);
                cmd->append_output = true;
            }
        } else {
            // Regular argument
            cmd->argv[cmd->argc++] = strdup(token);
        }
        
        token = strtok_r(NULL, " \t", &saveptr);
    }
    
    cmd->argv[cmd->argc] = NULL;
    kfree(line);
    
    return 0;
}

int execute_single_command(command_t* cmd) {
    if(cmd->argc == 0) return 0;
    
    // Check for built-in command
    for(int i = 0; i < shell_ctx.builtin_count; i++) {
        if(strcmp(cmd->argv[0], shell_ctx.builtins[i].name) == 0) {
            return shell_ctx.builtins[i].func(cmd->argc, cmd->argv);
        }
    }
    
    // Execute external command
    return execute_external_command(cmd);
}

int execute_external_command(command_t* cmd) {
    // Find executable in PATH
    char executable_path[512];
    if(!find_executable(cmd->argv[0], executable_path)) {
        printf("Command not found: %s\n", cmd->argv[0]);
        return -1;
    }
    
    // Create new process
    uint32_t pid = process_fork();
    if(pid == 0) {
        // Child process
        
        // Handle redirections
        if(cmd->input_redirect) {
            int fd = fs_open(cmd->input_redirect, O_RDONLY);
            if(fd >= 0) {
                dup2(fd, STDIN_FILENO);
                fs_close(fd);
            }
        }
        
        if(cmd->output_redirect) {
            int flags = cmd->append_output ? (O_WRONLY | O_CREAT | O_APPEND) : 
                                           (O_WRONLY | O_CREAT | O_TRUNC);
            int fd = fs_open(cmd->output_redirect, flags);
            if(fd >= 0) {
                dup2(fd, STDOUT_FILENO);
                fs_close(fd);
            }
        }
        
        // Execute program
        process_exec(executable_path, cmd->argv, shell_ctx.environ);
        process_exit(-1); // Should not reach here
    } else if(pid > 0) {
        // Parent process
        if(!cmd->background) {
            // Wait for child to complete
            int status;
            process_wait(pid, &status);
            shell_ctx.last_exit_code = status;
            return status;
        } else {
            printf("[%d] %s\n", pid, cmd->argv[0]);
            return 0;
        }
    } else {
        printf("Failed to create process\n");
        return -1;
    }
}

// Built-in command implementations

int cmd_cd(int argc, char* argv[]) {
    const char* path = (argc > 1) ? argv[1] : get_env_var("HOME");
    
    if(!path) path = "/";
    
    // Resolve path
    char resolved_path[512];
    if(resolve_path(path, resolved_path) != 0) {
        printf("cd: %s: No such file or directory\n", path);
        return 1;
    }
    
    // Check if it's a directory
    struct stat st;
    if(fs_stat(resolved_path, &st) != 0 || !S_ISDIR(st.st_mode)) {
        printf("cd: %s: Not a directory\n", path);
        return 1;
    }
    
    // Change directory
    strcpy(shell_ctx.cwd, resolved_path);
    set_env_var("PWD", shell_ctx.cwd);
    
    return 0;
}

int cmd_pwd(int argc, char* argv[]) {
    printf("%s\n", shell_ctx.cwd);
    return 0;
}

int cmd_ls(int argc, char* argv[]) {
    const char* path = (argc > 1) ? argv[1] : shell_ctx.cwd;
    bool long_format = false;
    bool show_hidden = false;
    bool show_all = false;
    
    // Parse options
    for(int i = 1; i < argc; i++) {
        if(argv[i][0] == '-') {
            for(char* opt = &argv[i][1]; *opt; opt++) {
                switch(*opt) {
                    case 'l': long_format = true; break;
                    case 'a': show_all = true; break;
                    case 'A': show_hidden = true; break;
                }
            }
        } else {
            path = argv[i];
        }
    }
    
    // Read directory
    dirent_info_t entries[MAX_DIR_ENTRIES];
    uint32_t count = MAX_DIR_ENTRIES;
    
    if(fs_readdir(path, entries, &count) != 0) {
        printf("ls: cannot access '%s': No such file or directory\n", path);
        return 1;
    }
    
    // Display entries
    for(uint32_t i = 0; i < count; i++) {
        // Skip hidden files unless requested
        if(!show_all && !show_hidden && entries[i].name[0] == '.') {
            continue;
        }
        
        if(long_format) {
            // Long format: permissions size date name
            char perms[11];
            format_permissions(entries[i].permissions, perms);
            
            char size_str[32];
            format_file_size(entries[i].size, size_str);
            
            char date_str[64];
            format_date(entries[i].modified, date_str);
            
            printf("%s %8s %s %s\n", perms, size_str, date_str, entries[i].name);
        } else {
            // Simple format
            printf("%s  ", entries[i].name);
        }
    }
    
    if(!long_format) printf("\n");
    
    return 0;
}

int cmd_cat(int argc, char* argv[]) {
    if(argc < 2) {
        printf("Usage: cat <file>\n");
        return 1;
    }
    
    for(int i = 1; i < argc; i++) {
        int fd = fs_open(argv[i], O_RDONLY);
        if(fd < 0) {
            printf("cat: %s: No such file or directory\n", argv[i]);
            continue;
        }
        
        char buffer[1024];
        ssize_t bytes_read;
        
        while((bytes_read = fs_read(fd, buffer, sizeof(buffer))) > 0) {
            for(ssize_t j = 0; j < bytes_read; j++) {
                putchar(buffer[j]);
            }
        }
        
        fs_close(fd);
    }
    
    return 0;
}

int cmd_echo(int argc, char* argv[]) {
    bool newline = true;
    int start = 1;
    
    // Check for -n option
    if(argc > 1 && strcmp(argv[1], "-n") == 0) {
        newline = false;
        start = 2;
    }
    
    for(int i = start; i < argc; i++) {
        if(i > start) printf(" ");
        printf("%s", argv[i]);
    }
    
    if(newline) printf("\n");
    
    return 0;
}

int cmd_ps(int argc, char* argv[]) {
    process_info_t processes[MAX_PROCESSES];
    uint32_t count = MAX_PROCESSES;
    
    get_process_list(processes, &count);
    
    printf("  PID  PPID STATE PRI     TIME COMMAND\n");
    
    for(uint32_t i = 0; i < count; i++) {
        const char* state_str;
        switch(processes[i].state) {
            case PROCESS_READY: state_str = "R"; break;
            case PROCESS_RUNNING: state_str = "S"; break;
            case PROCESS_BLOCKED: state_str = "D"; break;
            case PROCESS_ZOMBIE: state_str = "Z"; break;
            default: state_str = "?"; break;
        }
        
        printf("%5d %5d %5s %3d %8lu %s\n",
               processes[i].pid, processes[i].ppid, state_str,
               processes[i].priority, processes[i].cpu_time,
               processes[i].name);
    }
    
    return 0;
}

int cmd_ppmview(int argc, char* argv[]) {
    if(argc < 2) {
        printf("Usage: ppmview <file.ppm>\n");
        return 1;
    }
    
    // Load and display PPM image
    ppm_image_t image;
    if(!load_ppm_image(argv[1], &image)) {
        printf("ppmview: Failed to load %s\n", argv[1]);
        return 1;
    }
    
    printf("PPM Image: %s\n", argv[1]);
    printf("Dimensions: %dx%d\n", image.width, image.height);
    printf("Size: %lu bytes\n", image.width * image.height * 4);
    
    // Create simple image viewer window
    window_t* viewer = create_window("PPM Viewer", 100, 100, 
                                   image.width + 20, image.height + 50,
                                   WINDOW_CLOSABLE);
    
    if(viewer) {
        // Draw image to window
        draw_ppm_image(viewer->buffer, &image, 10, 40);
        blit_window_to_screen(viewer);
        
        printf("Image displayed in window. Press any key to close.\n");
        getchar();
        
        destroy_window(viewer);
    }
    
    free_ppm_image(&image);
    
    return 0;
}

int cmd_diskutil(int argc, char* argv[]) {
    if(argc < 2) {
        printf("Usage: diskutil <command>\n");
        printf("Commands:\n");
        printf("  info     - Show disk information\n");
        printf("  list     - List all disks\n");
        printf("  format   - Format disk\n");
        return 1;
    }
    
    if(strcmp(argv[1], "info") == 0) {
        printf("Disk Information:\n");
        printf("Total Space: %lu MB\n", get_disk_size() / (1024 * 1024));
        printf("Free Space:  %lu MB\n", get_free_space() / (1024 * 1024));
        printf("Used Space:  %lu MB\n", get_used_space() / (1024 * 1024));
    } else if(strcmp(argv[1], "list") == 0) {
        printf("Available Disks:\n");
        printf("  /dev/sda1 - System Drive (RFS)\n");
    } else if(strcmp(argv[1], "format") == 0) {
        printf("WARNING: This will erase all data!\n");
        printf("Type 'YES' to confirm: ");
        
        char confirm[10];
        if(fgets(confirm, sizeof(confirm), stdin) && 
           strcmp(confirm, "YES\n") == 0) {
            printf("Formatting disk...\n");
            format_rfs();
            printf("Format complete.\n");
        } else {
            printf("Format cancelled.\n");
        }
    }
    
    return 0;
}

// Utility functions

bool find_executable(const char* name, char* path) {
    // If name contains '/', treat as absolute/relative path
    if(strchr(name, '/')) {
        strcpy(path, name);
        return fs_stat(path, NULL) == 0;
    }
    
    // Search in PATH
    const char* path_env = get_env_var("PATH");
    if(!path_env) return false;
    
    char* path_copy = strdup(path_env);
    char* dir = strtok(path_copy, ":");
    
    while(dir) {
        snprintf(path, 512, "%s/%s", dir, name);
        if(fs_stat(path, NULL) == 0) {
            kfree(path_copy);
            return true;
        }
        dir = strtok(NULL, ":");
    }
    
    kfree(path_copy);
    return false;
}

void set_env_var(const char* name, const char* value) {
    // Find existing variable
    for(int i = 0; i < shell_ctx.env_count; i++) {
        if(strcmp(shell_ctx.env_vars[i].name, name) == 0) {
            strncpy(shell_ctx.env_vars[i].value, value, 255);
            return;
        }
    }
    
    // Add new variable
    if(shell_ctx.env_count < MAX_ENV_VARS) {
        strncpy(shell_ctx.env_vars[shell_ctx.env_count].name, name, 63);
        strncpy(shell_ctx.env_vars[shell_ctx.env_count].value, value, 255);
        shell_ctx.env_count++;
    }
}

const char* get_env_var(const char* name) {
    for(int i = 0; i < shell_ctx.env_count; i++) {
        if(strcmp(shell_ctx.env_vars[i].name, name) == 0) {
            return shell_ctx.env_vars[i].value;
        }
    }
    return NULL;
}

void register_builtin(const char* name, builtin_func_t func) {
    if(shell_ctx.builtin_count < MAX_BUILTINS) {
        strncpy(shell_ctx.builtins[shell_ctx.builtin_count].name, name, 63);
        shell_ctx.builtins[shell_ctx.builtin_count].func = func;
        shell_ctx.builtin_count++;
    }
}

int resolve_path(const char* path, char* resolved) {
    if(path[0] == '/') {
        // Absolute path
        strcpy(resolved, path);
    } else {
        // Relative path
        snprintf(resolved, 512, "%s/%s", shell_ctx.cwd, path);
    }
    
    // Normalize path (remove . and ..)
    normalize_path(resolved);
    
    return 0;
}

void normalize_path(char* path) {
    char* components[64];
    int count = 0;
    
    // Split path into components
    char* token = strtok(path, "/");
    while(token && count < 64) {
        if(strcmp(token, ".") == 0) {
            // Skip current directory
        } else if(strcmp(token, "..") == 0) {
            // Go up one directory
            if(count > 0) count--;
        } else {
            components[count++] = token;
        }
        token = strtok(NULL, "/");
    }
    
    // Rebuild path
    strcpy(path, "/");
    for(int i = 0; i < count; i++) {
        if(i > 0 || strcmp(path, "/") != 0) {
            strcat(path, "/");
        }
        strcat(path, components[i]);
    }
}