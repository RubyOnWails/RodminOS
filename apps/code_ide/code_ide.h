#ifndef CODE_IDE_H
#define CODE_IDE_H

#include "sdk/include/rodmin.h"

// Professional IDE structures
typedef struct {
    char* filename;
    char* buffer;
    size_t size;
    size_t cursor;
    bool modified;
} ide_document_t;

typedef struct {
    rod_window_t main_window;
    ide_document_t* active_doc;
    void* font;
    uint32_t theme_bg;
    uint32_t theme_fg;
} ide_context_t;

// IDE Functions
void ide_init(ide_context_t* ctx);
void ide_open_file(ide_context_t* ctx, const char* path);
void ide_save_file(ide_context_t* ctx);
void ide_render(ide_context_t* ctx);
void ide_handle_input(ide_context_t* ctx, uint32_t key);

#endif
