#include "printer_driver.h"
#include "../kernel/kernel.h"

static printer_context_t printers[MAX_PRINTERS];
static uint32_t printer_count = 0;

int init_printer_driver(void) {
    detect_printers();
    init_print_spooler();
    register_print_formats();
    kprintf("Printer driver initialized\n");
    return 0;
}

void detect_printers(void) {
    detect_usb_printers();
    detect_network_printers();
    detect_parallel_printers();
}

int add_printer(uint16_t vendor_id, uint16_t product_id, 
                printer_type_t type, void* device_data) {
    if(printer_count >= MAX_PRINTERS) return -1;
    
    printer_context_t* printer = &printers[printer_count];
    printer->id = printer_count;
    printer->vendor_id = vendor_id;
    printer->product_id = product_id;
    printer->type = type;
    printer->device_data = device_data;
    printer->status = PRINTER_STATUS_READY;
    
    init_printer_capabilities(printer);
    printer_count++;
    
    return printer->id;
}

int print_document(int printer_id, print_job_t* job) {
    if(printer_id >= printer_count) return -1;
    
    printer_context_t* printer = &printers[printer_id];
    
    switch(job->format) {
        case PRINT_FORMAT_POSTSCRIPT:
            return print_postscript(printer, job);
        case PRINT_FORMAT_PCL:
            return print_pcl(printer, job);
        case PRINT_FORMAT_TEXT:
            return print_text(printer, job);
        case PRINT_FORMAT_IMAGE:
            return print_image(printer, job);
        default:
            return -1;
    }
}

int print_postscript(printer_context_t* printer, print_job_t* job) {
    ps_interpreter_t interpreter;
    init_ps_interpreter(&interpreter);
    
    uint8_t* ps_data = (uint8_t*)job->data;
    size_t data_size = job->data_size;
    
    render_context_t render_ctx;
    init_render_context(&render_ctx, job->page_width, job->page_height);
    
    if(parse_postscript(&interpreter, ps_data, data_size, &render_ctx) != 0) {
        return -1;
    }
    
    return send_to_printer(printer, render_ctx.bitmap, render_ctx.bitmap_size);
}

int send_to_printer(printer_context_t* printer, uint8_t* data, size_t size) {
    switch(printer->type) {
        case PRINTER_TYPE_USB:
            return send_usb_print_data(printer, data, size);
        case PRINTER_TYPE_NETWORK:
            return send_network_print_data(printer, data, size);
        case PRINTER_TYPE_PARALLEL:
            return send_parallel_print_data(printer, data, size);
        default:
            return -1;
    }
}