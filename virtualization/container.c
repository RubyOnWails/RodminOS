#include "container.h"
#include "../kernel/kernel.h"
#include "../security/security.h"

static container_runtime_t runtime;

int init_container_runtime(void) {
    runtime.container_count = 0;
    runtime.image_count = 0;
    
    init_container_networking();
    init_container_storage();
    setup_container_security();
    
    kprintf("Container runtime initialized\n");
    return 0;
}

int create_container(container_config_t* config) {
    if(runtime.container_count >= MAX_CONTAINERS) return -1;
    
    container_t* container = &runtime.containers[runtime.container_count];
    container->id = generate_container_id();
    strncpy(container->name, config->name, 63);
    strncpy(container->image, config->image, 255);
    
    if(setup_container_namespace(container) != 0) return -1;
    if(setup_container_cgroups(container, config) != 0) return -1;
    if(setup_container_filesystem(container, config) != 0) return -1;
    if(setup_container_network(container, config) != 0) return -1;
    
    container->state = CONTAINER_CREATED;
    runtime.container_count++;
    
    return container->id;
}

int setup_container_namespace(container_t* container) {
    namespace_t* ns = &container->namespace;
    
    ns->pid_ns = create_pid_namespace();
    ns->mount_ns = create_mount_namespace();
    ns->net_ns = create_network_namespace();
    ns->ipc_ns = create_ipc_namespace();
    ns->uts_ns = create_uts_namespace();
    ns->user_ns = create_user_namespace();
    
    return 0;
}

int setup_container_cgroups(container_t* container, container_config_t* config) {
    cgroup_t* cgroup = &container->cgroup;
    
    char cgroup_path[256];
    snprintf(cgroup_path, 256, "/sys/fs/cgroup/rodmin/%s", container->name);
    
    if(create_cgroup(cgroup_path) != 0) return -1;
    
    set_cgroup_limit(cgroup_path, "memory.limit_in_bytes", config->memory_limit);
    set_cgroup_limit(cgroup_path, "cpu.cfs_quota_us", config->cpu_quota);
    set_cgroup_limit(cgroup_path, "blkio.throttle.read_bps_device", config->io_read_bps);
    set_cgroup_limit(cgroup_path, "blkio.throttle.write_bps_device", config->io_write_bps);
    
    strncpy(cgroup->path, cgroup_path, 255);
    
    return 0;
}

int setup_container_filesystem(container_t* container, container_config_t* config) {
    char rootfs_path[512];
    snprintf(rootfs_path, 512, "/var/lib/containers/%s/rootfs", container->name);
    
    if(fs_mkdir(rootfs_path, 0755) != 0) return -1;
    
    if(extract_container_image(config->image, rootfs_path) != 0) return -1;
    
    if(setup_container_mounts(container, rootfs_path, config) != 0) return -1;
    
    strncpy(container->rootfs_path, rootfs_path, 511);
    
    return 0;
}

int extract_container_image(const char* image, const char* dest_path) {
    char image_path[512];
    snprintf(image_path, 512, "/var/lib/containers/images/%s.tar", image);
    
    archive_t archive;
    if(open_archive(&archive, image_path) != 0) return -1;
    
    archive_entry_t entry;
    while(read_archive_entry(&archive, &entry) == 0) {
        char full_path[1024];
        snprintf(full_path, 1024, "%s/%s", dest_path, entry.name);
        
        if(entry.type == ARCHIVE_ENTRY_FILE) {
            extract_file(&archive, &entry, full_path);
        } else if(entry.type == ARCHIVE_ENTRY_DIR) {
            fs_mkdir(full_path, entry.mode);
        } else if(entry.type == ARCHIVE_ENTRY_SYMLINK) {
            create_symlink(entry.link_target, full_path);
        }
    }
    
    close_archive(&archive);
    return 0;
}

int start_container(int container_id) {
    container_t* container = find_container(container_id);
    if(!container || container->state != CONTAINER_CREATED) return -1;
    
    uint32_t pid = process_fork();
    if(pid == 0) {
        // Child process - container init
        enter_container_namespace(container);
        setup_container_environment(container);
        chroot_to_container(container);
        
        // Execute container command
        char* argv[] = {"/bin/sh", NULL};
        char* envp[] = {"PATH=/bin:/usr/bin", NULL};
        process_exec("/bin/sh", argv, envp);
        
        process_exit(-1);
    } else if(pid > 0) {
        // Parent process
        container->init_pid = pid;
        container->state = CONTAINER_RUNNING;
        
        add_pid_to_cgroup(container->cgroup.path, pid);
        
        return 0;
    }
    
    return -1;
}

void enter_container_namespace(container_t* container) {
    namespace_t* ns = &container->namespace;
    
    enter_pid_namespace(ns->pid_ns);
    enter_mount_namespace(ns->mount_ns);
    enter_network_namespace(ns->net_ns);
    enter_ipc_namespace(ns->ipc_ns);
    enter_uts_namespace(ns->uts_ns);
    enter_user_namespace(ns->user_ns);
}

int stop_container(int container_id) {
    container_t* container = find_container(container_id);
    if(!container || container->state != CONTAINER_RUNNING) return -1;
    
    // Send SIGTERM to container init process
    process_kill(container->init_pid, SIGTERM);
    
    // Wait for graceful shutdown
    sleep(10);
    
    // Force kill if still running
    if(is_process_running(container->init_pid)) {
        process_kill(container->init_pid, SIGKILL);
    }
    
    container->state = CONTAINER_STOPPED;
    
    // Cleanup resources
    cleanup_container_cgroups(container);
    cleanup_container_network(container);
    
    return 0;
}

int remove_container(int container_id) {
    container_t* container = find_container(container_id);
    if(!container) return -1;
    
    if(container->state == CONTAINER_RUNNING) {
        stop_container(container_id);
    }
    
    // Remove filesystem
    remove_directory_recursive(container->rootfs_path);
    
    // Remove from runtime
    for(int i = 0; i < runtime.container_count; i++) {
        if(runtime.containers[i].id == container_id) {
            memmove(&runtime.containers[i], &runtime.containers[i + 1],
                   (runtime.container_count - i - 1) * sizeof(container_t));
            runtime.container_count--;
            break;
        }
    }
    
    return 0;
}

int build_container_image(const char* dockerfile_path, const char* image_name) {
    dockerfile_t dockerfile;
    if(parse_dockerfile(dockerfile_path, &dockerfile) != 0) return -1;
    
    char build_context[512];
    get_directory_name(dockerfile_path, build_context);
    
    char temp_rootfs[512];
    snprintf(temp_rootfs, 512, "/tmp/build_%s", image_name);
    fs_mkdir(temp_rootfs, 0755);
    
    // Process Dockerfile instructions
    for(int i = 0; i < dockerfile.instruction_count; i++) {
        dockerfile_instruction_t* inst = &dockerfile.instructions[i];
        
        switch(inst->type) {
            case DOCKERFILE_FROM:
                copy_base_image(inst->args[0], temp_rootfs);
                break;
            case DOCKERFILE_RUN:
                execute_in_container(temp_rootfs, inst->args[0]);
                break;
            case DOCKERFILE_COPY:
                copy_files_to_container(build_context, inst->args[0], 
                                      temp_rootfs, inst->args[1]);
                break;
            case DOCKERFILE_ADD:
                add_files_to_container(build_context, inst->args[0],
                                     temp_rootfs, inst->args[1]);
                break;
            case DOCKERFILE_WORKDIR:
                set_container_workdir(temp_rootfs, inst->args[0]);
                break;
            case DOCKERFILE_ENV:
                set_container_env(temp_rootfs, inst->args[0], inst->args[1]);
                break;
            case DOCKERFILE_EXPOSE:
                add_exposed_port(temp_rootfs, atoi(inst->args[0]));
                break;
        }
    }
    
    // Create image archive
    char image_path[512];
    snprintf(image_path, 512, "/var/lib/containers/images/%s.tar", image_name);
    
    create_image_archive(temp_rootfs, image_path);
    
    // Cleanup
    remove_directory_recursive(temp_rootfs);
    
    return 0;
}

int list_containers(container_info_t* containers, uint32_t* count) {
    uint32_t max_count = *count;
    *count = 0;
    
    for(int i = 0; i < runtime.container_count && *count < max_count; i++) {
        container_t* container = &runtime.containers[i];
        
        containers[*count].id = container->id;
        strncpy(containers[*count].name, container->name, 63);
        strncpy(containers[*count].image, container->image, 255);
        containers[*count].state = container->state;
        containers[*count].pid = container->init_pid;
        
        (*count)++;
    }
    
    return 0;
}