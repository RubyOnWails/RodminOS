#ifndef SHELL_H
#define SHELL_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_ARGS 64
#define MAX_PIPES 8
#define MAX_ENV_VARS 256
#define MAX_BUILTINS 64
#define MAX_DIR_ENTRIES 1024

// File descriptors
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

// File type macros
#define S_ISDIR(m) (((m) & 0170000) == 0040000)
#define S_ISREG(m) (((m) & 0170000) == 0100000)

typedef struct {
    int argc;
    char* argv[MAX_ARGS];
    
    // Pipes
    int pipe_count;
    int pipe_positions[MAX_PIPES];
    
    // Redirections
    char* input_redirect;
    char* output_redirect;
    bool append_output;
    
    // Background execution
    bool background;
} command_t;

typedef struct {
    char name[64];
    char value[256];
} env_var_t;

typedef int (*builtin_func_t)(int argc, char* argv[]);

typedef struct {
    char name[64];
    builtin_func_t func;
} builtin_command_t;

typedef struct {
    char cwd[512];
    int last_exit_code;
    
    // Environment variables
    env_var_t env_vars[MAX_ENV_VARS];
    int env_count;
    char** environ;
    
    // Built-in commands
    builtin_command_t builtins[MAX_BUILTINS];
    int builtin_count;
} shell_context_t;

// Shell initialization
void init_shell(void);
void init_environment(void);
void init_builtin_commands(void);

// Command execution
int execute_command_line(const char* cmdline);
int parse_command_line(const char* cmdline, command_t* cmd);
int execute_single_command(command_t* cmd);
int execute_external_command(command_t* cmd);
int execute_pipeline(command_t* cmd);

// Built-in commands
int cmd_cd(int argc, char* argv[]);
int cmd_pwd(int argc, char* argv[]);
int cmd_ls(int argc, char* argv[]);
int cmd_cat(int argc, char* argv[]);
int cmd_echo(int argc, char* argv[]);
int cmd_mkdir(int argc, char* argv[]);
int cmd_rmdir(int argc, char* argv[]);
int cmd_rm(int argc, char* argv[]);
int cmd_cp(int argc, char* argv[]);
int cmd_mv(int argc, char* argv[]);
int cmd_chmod(int argc, char* argv[]);
int cmd_chown(int argc, char* argv[]);
int cmd_ps(int argc, char* argv[]);
int cmd_kill(int argc, char* argv[]);
int cmd_top(int argc, char* argv[]);
int cmd_mount(int argc, char* argv[]);
int cmd_umount(int argc, char* argv[]);
int cmd_df(int argc, char* argv[]);
int cmd_free(int argc, char* argv[]);
int cmd_uname(int argc, char* argv[]);
int cmd_date(int argc, char* argv[]);
int cmd_whoami(int argc, char* argv[]);
int cmd_env(int argc, char* argv[]);
int cmd_export(int argc, char* argv[]);
int cmd_unset(int argc, char* argv[]);
int cmd_history(int argc, char* argv[]);
int cmd_alias(int argc, char* argv[]);
int cmd_which(int argc, char* argv[]);
int cmd_find(int argc, char* argv[]);
int cmd_grep(int argc, char* argv[]);
int cmd_head(int argc, char* argv[]);
int cmd_tail(int argc, char* argv[]);
int cmd_wc(int argc, char* argv[]);
int cmd_sort(int argc, char* argv[]);
int cmd_uniq(int argc, char* argv[]);
int cmd_cut(int argc, char* argv[]);
int cmd_sed(int argc, char* argv[]);
int cmd_awk(int argc, char* argv[]);
int cmd_tar(int argc, char* argv[]);
int cmd_gzip(int argc, char* argv[]);
int cmd_gunzip(int argc, char* argv[]);
int cmd_wget(int argc, char* argv[]);
int cmd_curl(int argc, char* argv[]);
int cmd_ssh(int argc, char* argv[]);
int cmd_scp(int argc, char* argv[]);
int cmd_ping(int argc, char* argv[]);
int cmd_netstat(int argc, char* argv[]);
int cmd_ifconfig(int argc, char* argv[]);
int cmd_ppmview(int argc, char* argv[]);
int cmd_diskutil(int argc, char* argv[]);

// Environment management
void set_env_var(const char* name, const char* value);
const char* get_env_var(const char* name);
void unset_env_var(const char* name);

// Built-in command management
void register_builtin(const char* name, builtin_func_t func);

// Path utilities
bool find_executable(const char* name, char* path);
int resolve_path(const char* path, char* resolved);
void normalize_path(char* path);

// File utilities
void format_permissions(uint32_t mode, char* buffer);
void format_file_size(uint64_t size, char* buffer);
void format_date(uint64_t timestamp, char* buffer);

// I/O utilities
int dup2(int oldfd, int newfd);
char* fgets(char* str, int size, FILE* stream);

// Standard functions
char* strdup(const char* str);
char* strtok_r(char* str, const char* delim, char** saveptr);
char* strchr(const char* str, int c);
int printf(const char* format, ...);
int putchar(int c);
int getchar(void);
FILE* stdin;

#endif