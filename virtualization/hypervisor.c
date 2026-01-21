#include "hypervisor.h"
#include "../kernel/kernel.h"
#include "../kernel/memory.h"

static hypervisor_t hypervisor;

int init_hypervisor(void) {
    if(!check_virtualization_support()) return -1;
    
    hypervisor.vm_count = 0;
    hypervisor.state = HV_STATE_READY;
    
    init_vmx();
    setup_ept();
    init_vm_scheduler();
    
    kprintf("Hypervisor initialized\n");
    return 0;
}

bool check_virtualization_support(void) {
    uint32_t eax, ebx, ecx, edx;
    
    cpuid(1, &eax, &ebx, &ecx, &edx);
    if(!(ecx & (1 << 5))) return false; // VMX support
    
    uint64_t msr = read_msr(MSR_IA32_FEATURE_CONTROL);
    if(!(msr & 1)) return false; // Feature control locked
    
    return true;
}

int create_vm(vm_config_t* config) {
    if(hypervisor.vm_count >= MAX_VMS) return -1;
    
    vm_t* vm = &hypervisor.vms[hypervisor.vm_count];
    vm->id = hypervisor.vm_count;
    vm->state = VM_STATE_STOPPED;
    
    vm->memory_size = config->memory_size;
    vm->vcpu_count = config->vcpu_count;
    
    if(allocate_vm_memory(vm) != 0) return -1;
    if(create_vcpus(vm) != 0) return -1;
    if(setup_vm_devices(vm, config) != 0) return -1;
    
    hypervisor.vm_count++;
    return vm->id;
}

int allocate_vm_memory(vm_t* vm) {
    vm->guest_memory = (uint8_t*)buddy_alloc(get_order(vm->memory_size));
    if(!vm->guest_memory) return -1;
    
    vm->ept_root = create_ept_tables(vm);
    if(!vm->ept_root) return -1;
    
    return 0;
}

int create_vcpus(vm_t* vm) {
    for(int i = 0; i < vm->vcpu_count; i++) {
        vcpu_t* vcpu = &vm->vcpus[i];
        vcpu->id = i;
        vcpu->vm = vm;
        vcpu->state = VCPU_STATE_STOPPED;
        
        if(init_vmcs(vcpu) != 0) return -1;
        if(setup_vcpu_state(vcpu) != 0) return -1;
    }
    
    return 0;
}

int start_vm(int vm_id) {
    if(vm_id >= hypervisor.vm_count) return -1;
    
    vm_t* vm = &hypervisor.vms[vm_id];
    if(vm->state != VM_STATE_STOPPED) return -1;
    
    for(int i = 0; i < vm->vcpu_count; i++) {
        if(start_vcpu(&vm->vcpus[i]) != 0) return -1;
    }
    
    vm->state = VM_STATE_RUNNING;
    return 0;
}

int start_vcpu(vcpu_t* vcpu) {
    vcpu->state = VCPU_STATE_RUNNING;
    
    uint32_t cpu = allocate_physical_cpu();
    vcpu->physical_cpu = cpu;
    
    schedule_vcpu_on_cpu(vcpu, cpu);
    
    return 0;
}

void vm_exit_handler(vcpu_t* vcpu) {
    uint32_t exit_reason = vmread(VM_EXIT_REASON);
    
    switch(exit_reason) {
        case EXIT_REASON_CPUID:
            handle_cpuid_exit(vcpu);
            break;
        case EXIT_REASON_IO_INSTRUCTION:
            handle_io_exit(vcpu);
            break;
        case EXIT_REASON_MSR_READ:
            handle_msr_read_exit(vcpu);
            break;
        case EXIT_REASON_MSR_WRITE:
            handle_msr_write_exit(vcpu);
            break;
        case EXIT_REASON_EPT_VIOLATION:
            handle_ept_violation(vcpu);
            break;
        case EXIT_REASON_INTERRUPT_WINDOW:
            handle_interrupt_window(vcpu);
            break;
        default:
            handle_unknown_exit(vcpu, exit_reason);
            break;
    }
}

void handle_io_exit(vcpu_t* vcpu) {
    uint64_t exit_qualification = vmread(EXIT_QUALIFICATION);
    
    uint16_t port = (exit_qualification >> 16) & 0xFFFF;
    bool is_write = (exit_qualification & 8) != 0;
    uint8_t size = (exit_qualification & 7) + 1;
    
    if(is_write) {
        uint32_t value = get_guest_register(vcpu, REG_RAX);
        handle_vm_io_write(vcpu->vm, port, value, size);
    } else {
        uint32_t value = handle_vm_io_read(vcpu->vm, port, size);
        set_guest_register(vcpu, REG_RAX, value);
    }
    
    advance_guest_rip(vcpu);
}

uint32_t handle_vm_io_read(vm_t* vm, uint16_t port, uint8_t size) {
    for(int i = 0; i < vm->device_count; i++) {
        vm_device_t* device = &vm->devices[i];
        if(port >= device->io_base && port < device->io_base + device->io_size) {
            return device->io_read(device, port - device->io_base, size);
        }
    }
    
    return 0xFFFFFFFF;
}

void handle_vm_io_write(vm_t* vm, uint16_t port, uint32_t value, uint8_t size) {
    for(int i = 0; i < vm->device_count; i++) {
        vm_device_t* device = &vm->devices[i];
        if(port >= device->io_base && port < device->io_base + device->io_size) {
            device->io_write(device, port - device->io_base, value, size);
            return;
        }
    }
}

int setup_vm_devices(vm_t* vm, vm_config_t* config) {
    vm->device_count = 0;
    
    add_vm_device(vm, create_virtual_uart());
    add_vm_device(vm, create_virtual_keyboard());
    add_vm_device(vm, create_virtual_mouse());
    add_vm_device(vm, create_virtual_disk(config->disk_image));
    add_vm_device(vm, create_virtual_network());
    
    if(config->gpu_passthrough) {
        add_vm_device(vm, create_gpu_passthrough_device());
    } else {
        add_vm_device(vm, create_virtual_gpu());
    }
    
    return 0;
}

vm_device_t* create_virtual_disk(const char* disk_image) {
    vm_device_t* device = (vm_device_t*)kmalloc(sizeof(vm_device_t));
    
    device->type = VM_DEVICE_DISK;
    device->io_base = 0x1F0;
    device->io_size = 8;
    device->io_read = virtual_disk_read;
    device->io_write = virtual_disk_write;
    
    virtual_disk_t* disk = (virtual_disk_t*)kmalloc(sizeof(virtual_disk_t));
    disk->image_fd = fs_open(disk_image, O_RDWR);
    disk->sector_count = get_file_size(disk_image) / 512;
    
    device->private_data = disk;
    
    return device;
}

uint32_t virtual_disk_read(vm_device_t* device, uint16_t offset, uint8_t size) {
    virtual_disk_t* disk = (virtual_disk_t*)device->private_data;
    
    switch(offset) {
        case 0: // Data register
            return disk->data_buffer[disk->buffer_pos++];
        case 1: // Error/Features
            return 0;
        case 2: // Sector count
            return disk->sector_count & 0xFF;
        case 7: // Status register
            return 0x50; // Drive ready, seek complete
        default:
            return 0;
    }
}

void virtual_disk_write(vm_device_t* device, uint16_t offset, uint32_t value, uint8_t size) {
    virtual_disk_t* disk = (virtual_disk_t*)device->private_data;
    
    switch(offset) {
        case 0: // Data register
            disk->data_buffer[disk->buffer_pos++] = value & 0xFF;
            break;
        case 2: // Sector count
            disk->sectors_to_transfer = value & 0xFF;
            break;
        case 3: // LBA low
            disk->lba = (disk->lba & 0xFFFFFF00) | (value & 0xFF);
            break;
        case 4: // LBA mid
            disk->lba = (disk->lba & 0xFFFF00FF) | ((value & 0xFF) << 8);
            break;
        case 5: // LBA high
            disk->lba = (disk->lba & 0xFF00FFFF) | ((value & 0xFF) << 16);
            break;
        case 7: // Command register
            handle_disk_command(disk, value & 0xFF);
            break;
    }
}

void handle_disk_command(virtual_disk_t* disk, uint8_t command) {
    switch(command) {
        case 0x20: // Read sectors
            read_disk_sectors(disk);
            break;
        case 0x30: // Write sectors
            write_disk_sectors(disk);
            break;
        case 0xEC: // Identify drive
            identify_drive(disk);
            break;
    }
}