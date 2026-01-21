#include "../../sdk/include/rodmin.h"
#include "code_ide.h"
#include <stdlib.h>
#include <string.h>

static ide_context_t ide_ctx;

void ide_init(ide_context_t* ctx) {
    ctx->main_window = rod_create_window("Rodmin IDE v1.0", 100, 100, 800, 600);
    ctx->active_doc = NULL;
    ctx->theme_bg = 0x1E1E1E; // Dark theme
    ctx->theme_fg = 0xD4D4D4;
    
    kprintf("IDE Initialized.\n");
}

void ide_open_file(ide_context_t* ctx, const char* path) {
    int fd = rod_open(path, 0);
    if (fd >= 0) {
        ide_document_t* doc = (ide_document_t*)rod_malloc(sizeof(ide_document_t));
        doc->filename = strdup(path);
        doc->size = 4096; // Placeholder
        doc->buffer = (char*)rod_malloc(doc->size);
        rod_read(fd, doc->buffer, doc->size);
        ctx->active_doc = doc;
        rod_close(fd);
    }
}

void ide_render(ide_context_t* ctx) {
    // Render code with syntax highlighting logic
    if (ctx->active_doc) {
        // ... draw_text_shaded(...)
    }
}

int main(int argc, char** argv) {
    ide_init(&ide_ctx);
    if (argc > 1) {
        ide_open_file(&ide_ctx, argv[1]);
    }
    
    while(1) {
        ide_render(&ide_ctx);
        // rod_wait_event(...);
    }
    return 0;
}