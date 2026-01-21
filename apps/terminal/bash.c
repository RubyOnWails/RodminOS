#include "../../sdk/include/rodmin.h"
#include <stdio.h>
#include <string.h>

// Bash-like Shell Implementation for Rodmin OS
void shell_main(void) {
    char cmd[1024];
    printf("Rodmin Bash (MSYS/WSL Compatible Layer)\n");
    printf("rodmin@localhost:~$ ");
    
    while(1) {
        // Read input (simulated)
        // ...
        printf("rodmin@localhost:~$ ");
    }
}

// WSL/Unix Compatibility Wrappers
int unix_fork(void) {
    return rod_syscall(SYS_FORK, 0, 0, 0);
}

int unix_execve(const char* path, char* const argv[], char* const envp[]) {
    // Translate unix paths to rodmin paths if necessary
    return rod_syscall(SYS_EXEC, (uint64_t)path, (uint64_t)argv, 0);
}
