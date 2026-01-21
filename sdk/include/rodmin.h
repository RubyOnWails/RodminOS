#ifndef RODMIN_SDK_H
#define RODMIN_SDK_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Rodmin System API
#define ROD_VERSION "1.0.0-SDK"

// System Calls
extern int rod_syscall(int num, uint64_t arg1, uint64_t arg2, uint64_t arg3);

// File I/O
int rod_open(const char* path, int flags);
int rod_close(int fd);
size_t rod_read(int fd, void* buf, size_t count);
size_t rod_write(int fd, const void* buf, size_t count);

// Process Management
int rod_exec(const char* path, const char** argv);
void rod_exit(int status);
int rod_getpid(void);

// GUI API
typedef void* rod_window_t;
rod_window_t rod_create_window(const char* title, int x, int y, int w, int h);
void rod_window_draw_pixel(rod_window_t win, int x, int y, uint32_t color);
void rod_window_blit(rod_window_t win, const uint32_t* buffer);

// Memory API
void* rod_malloc(size_t size);
void rod_free(void* ptr);

#endif
