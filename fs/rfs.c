#include "fs.h"
#include "memory.h"
#include "kernel.h"

// Global file system state
static rfs_superblock_t* superblock;
static rfs_inode_t* inode_table;
static uint8_t* block_bitmap;
static uint8_t* inode_bitmap;
static rfs_journal_t* journal;

// Open file table
static open_file_t open_files[MAX_OPEN_FILES];
static bool open_file_slots[MAX_OPEN_FILES];

// Mount table
static mount_point_t mount_table[MAX_MOUNT_POINTS];
static uint32_t mount_count = 0;

void fs_init(void) {
    // Initialize open file table
    for(int i = 0; i < MAX_OPEN_FILES; i++) {
        open_file_slots[i] = false;
    }
    
    // Initialize mount table
    mount_count = 0;
    
    // Mount root file system
    mount_root_fs();
    
    // Initialize journal
    init_journal();
    
    kprintf("File system initialized\n");
}

void mount_root_fs(void) {
    // Read superblock from disk
    superblock = (rfs_superblock_t*)kmalloc(sizeof(rfs_superblock_t));
    read_disk_block(0, superblock);
    
    // Verify magic number
    if(superblock->magic != RFS_MAGIC) {
        kprintf("Invalid RFS magic number, formatting...\n");
        format_rfs();
        return;
    }
    
    // Load inode table
    inode_table = (rfs_inode_t*)kmalloc(superblock->inode_count * sizeof(rfs_inode_t));
    read_disk_blocks(superblock->inode_table_start, 
                    (superblock->inode_count * sizeof(rfs_inode_t)) / BLOCK_SIZE + 1,
                    inode_table);
    
    // Load bitmaps
    block_bitmap = (uint8_t*)kmalloc(superblock->block_count / 8);
    read_disk_blocks(superblock->block_bitmap_start,
                    (superblock->block_count / 8) / BLOCK_SIZE + 1,
                    block_bitmap);
    
    inode_bitmap = (uint8_t*)kmalloc(superblock->inode_count / 8);
    read_disk_blocks(superblock->inode_bitmap_start,
                    (superblock->inode_count / 8) / BLOCK_SIZE + 1,
                    inode_bitmap);
    
    // Add root mount point
    mount_table[0].device = 0;
    strcpy(mount_table[0].path, "/");
    mount_table[0].fs_type = FS_TYPE_RFS;
    mount_table[0].superblock = superblock;
    mount_count = 1;
    
    kprintf("Root file system mounted\n");
}

void format_rfs(void) {
    // Create new superblock
    superblock->magic = RFS_MAGIC;
    superblock->version = RFS_VERSION;
    superblock->block_size = BLOCK_SIZE;
    superblock->block_count = get_disk_size() / BLOCK_SIZE;
    superblock->inode_count = superblock->block_count / 4; // 25% for inodes
    
    // Calculate layout
    superblock->inode_table_start = 1;
    superblock->inode_bitmap_start = superblock->inode_table_start + 
                                   (superblock->inode_count * sizeof(rfs_inode_t)) / BLOCK_SIZE + 1;
    superblock->block_bitmap_start = superblock->inode_bitmap_start + 
                                   (superblock->inode_count / 8) / BLOCK_SIZE + 1;
    superblock->data_start = superblock->block_bitmap_start + 
                           (superblock->block_count / 8) / BLOCK_SIZE + 1;
    superblock->journal_start = superblock->data_start;
    superblock->journal_size = 1024; // 1024 blocks for journal
    
    // Write superblock
    write_disk_block(0, superblock);
    
    // Initialize bitmaps
    block_bitmap = (uint8_t*)kcalloc(superblock->block_count / 8, 1);
    inode_bitmap = (uint8_t*)kcalloc(superblock->inode_count / 8, 1);
    
    // Mark system blocks as used
    for(uint32_t i = 0; i < superblock->data_start + superblock->journal_size; i++) {
        set_bit(block_bitmap, i);
    }
    
    // Initialize inode table
    inode_table = (rfs_inode_t*)kcalloc(superblock->inode_count, sizeof(rfs_inode_t));
    
    // Create root directory
    create_root_directory();
    
    // Write everything to disk
    sync_fs();
    
    kprintf("File system formatted\n");
}

void create_root_directory(void) {
    // Allocate root inode
    uint32_t root_inode = alloc_inode();
    if(root_inode != 1) {
        kernel_panic("Failed to allocate root inode");
    }
    
    rfs_inode_t* root = &inode_table[root_inode - 1];
    root->type = INODE_TYPE_DIR;
    root->permissions = 0755;
    root->size = 0;
    root->created = get_system_time();
    root->modified = root->created;
    root->accessed = root->created;
    root->links = 2; // . and parent reference
    
    // Allocate data block for root directory
    uint32_t data_block = alloc_block();
    root->direct_blocks[0] = data_block;
    
    // Create . and .. entries
    rfs_dirent_t* entries = (rfs_dirent_t*)kmalloc(BLOCK_SIZE);
    
    entries[0].inode = root_inode;
    entries[0].type = DIRENT_TYPE_DIR;
    strcpy(entries[0].name, ".");
    
    entries[1].inode = root_inode;
    entries[1].type = DIRENT_TYPE_DIR;
    strcpy(entries[1].name, "..");
    
    write_disk_block(data_block, entries);
    root->size = 2 * sizeof(rfs_dirent_t);
    
    kfree(entries);
}

int fs_open(const char* path, int flags) {
    // Find free slot in open file table
    int slot = -1;
    for(int i = 0; i < MAX_OPEN_FILES; i++) {
        if(!open_file_slots[i]) {
            slot = i;
            open_file_slots[i] = true;
            break;
        }
    }
    
    if(slot == -1) return -1; // No free slots
    
    // Resolve path to inode
    uint32_t inode_num = path_to_inode(path);
    
    if(inode_num == 0 && !(flags & O_CREAT)) {
        open_file_slots[slot] = false;
        return -1; // File not found
    }
    
    // Create file if it doesn't exist and O_CREAT is set
    if(inode_num == 0 && (flags & O_CREAT)) {
        inode_num = create_file(path, INODE_TYPE_FILE, 0644);
        if(inode_num == 0) {
            open_file_slots[slot] = false;
            return -1;
        }
    }
    
    rfs_inode_t* inode = &inode_table[inode_num - 1];
    
    // Check permissions
    if(!check_permissions(inode, flags)) {
        open_file_slots[slot] = false;
        return -1;
    }
    
    // Initialize open file entry
    open_file_t* file = &open_files[slot];
    file->inode_num = inode_num;
    file->flags = flags;
    file->offset = (flags & O_APPEND) ? inode->size : 0;
    file->ref_count = 1;
    
    // Update access time
    inode->accessed = get_system_time();
    
    return slot;
}

int fs_close(int fd) {
    if(fd < 0 || fd >= MAX_OPEN_FILES || !open_file_slots[fd]) {
        return -1;
    }
    
    open_file_t* file = &open_files[fd];
    file->ref_count--;
    
    if(file->ref_count == 0) {
        open_file_slots[fd] = false;
    }
    
    return 0;
}

ssize_t fs_read(int fd, void* buffer, size_t count) {
    if(fd < 0 || fd >= MAX_OPEN_FILES || !open_file_slots[fd]) {
        return -1;
    }
    
    open_file_t* file = &open_files[fd];
    rfs_inode_t* inode = &inode_table[file->inode_num - 1];
    
    // Check if file is readable
    if(!(file->flags & O_RDONLY) && !(file->flags & O_RDWR)) {
        return -1;
    }
    
    // Don't read past end of file
    if(file->offset >= inode->size) {
        return 0;
    }
    
    if(file->offset + count > inode->size) {
        count = inode->size - file->offset;
    }
    
    // Read data
    ssize_t bytes_read = read_inode_data(inode, file->offset, buffer, count);
    
    if(bytes_read > 0) {
        file->offset += bytes_read;
        inode->accessed = get_system_time();
    }
    
    return bytes_read;
}

ssize_t fs_write(int fd, const void* buffer, size_t count) {
    if(fd < 0 || fd >= MAX_OPEN_FILES || !open_file_slots[fd]) {
        return -1;
    }
    
    open_file_t* file = &open_files[fd];
    rfs_inode_t* inode = &inode_table[file->inode_num - 1];
    
    // Check if file is writable
    if(!(file->flags & O_WRONLY) && !(file->flags & O_RDWR)) {
        return -1;
    }
    
    // Start journal transaction
    journal_transaction_t* trans = begin_transaction();
    
    // Write data
    ssize_t bytes_written = write_inode_data(inode, file->offset, buffer, count);
    
    if(bytes_written > 0) {
        file->offset += bytes_written;
        
        // Update inode
        if(file->offset > inode->size) {
            inode->size = file->offset;
        }
        inode->modified = get_system_time();
        
        // Log changes to journal
        log_inode_change(trans, file->inode_num, inode);
    }
    
    // Commit transaction
    commit_transaction(trans);
    
    return bytes_written;
}

off_t fs_lseek(int fd, off_t offset, int whence) {
    if(fd < 0 || fd >= MAX_OPEN_FILES || !open_file_slots[fd]) {
        return -1;
    }
    
    open_file_t* file = &open_files[fd];
    rfs_inode_t* inode = &inode_table[file->inode_num - 1];
    
    off_t new_offset;
    
    switch(whence) {
        case SEEK_SET:
            new_offset = offset;
            break;
        case SEEK_CUR:
            new_offset = file->offset + offset;
            break;
        case SEEK_END:
            new_offset = inode->size + offset;
            break;
        default:
            return -1;
    }
    
    if(new_offset < 0) {
        return -1;
    }
    
    file->offset = new_offset;
    return new_offset;
}

int fs_mkdir(const char* path, mode_t mode) {
    // Check if directory already exists
    if(path_to_inode(path) != 0) {
        return -1; // Already exists
    }
    
    // Create directory
    uint32_t inode_num = create_file(path, INODE_TYPE_DIR, mode);
    if(inode_num == 0) {
        return -1;
    }
    
    // Initialize directory with . and .. entries
    rfs_inode_t* inode = &inode_table[inode_num - 1];
    uint32_t data_block = alloc_block();
    inode->direct_blocks[0] = data_block;
    
    rfs_dirent_t* entries = (rfs_dirent_t*)kmalloc(BLOCK_SIZE);
    
    // Get parent directory inode
    char parent_path[512];
    get_parent_path(path, parent_path);
    uint32_t parent_inode = path_to_inode(parent_path);
    
    entries[0].inode = inode_num;
    entries[0].type = DIRENT_TYPE_DIR;
    strcpy(entries[0].name, ".");
    
    entries[1].inode = parent_inode;
    entries[1].type = DIRENT_TYPE_DIR;
    strcpy(entries[1].name, "..");
    
    write_disk_block(data_block, entries);
    inode->size = 2 * sizeof(rfs_dirent_t);
    
    kfree(entries);
    
    return 0;
}

int fs_rmdir(const char* path) {
    uint32_t inode_num = path_to_inode(path);
    if(inode_num == 0) return -1;
    
    rfs_inode_t* inode = &inode_table[inode_num - 1];
    
    // Check if it's a directory
    if(inode->type != INODE_TYPE_DIR) return -1;
    
    // Check if directory is empty (only . and .. entries)
    if(inode->size > 2 * sizeof(rfs_dirent_t)) {
        return -1; // Directory not empty
    }
    
    // Remove from parent directory
    remove_from_parent_dir(path, inode_num);
    
    // Free inode and blocks
    free_inode_blocks(inode);
    free_inode(inode_num);
    
    return 0;
}

int fs_unlink(const char* path) {
    uint32_t inode_num = path_to_inode(path);
    if(inode_num == 0) return -1;
    
    rfs_inode_t* inode = &inode_table[inode_num - 1];
    
    // Remove from parent directory
    remove_from_parent_dir(path, inode_num);
    
    // Decrease link count
    inode->links--;
    
    // If no more links, free the inode
    if(inode->links == 0) {
        free_inode_blocks(inode);
        free_inode(inode_num);
    }
    
    return 0;
}

// PPM icon support
int fs_set_icon(const char* path, const char* icon_path) {
    uint32_t inode_num = path_to_inode(path);
    if(inode_num == 0) return -1;
    
    rfs_inode_t* inode = &inode_table[inode_num - 1];
    
    // Store icon path in extended attributes
    strncpy(inode->icon_path, icon_path, 255);
    inode->icon_path[255] = '\0';
    
    inode->modified = get_system_time();
    
    return 0;
}

int fs_get_icon(const char* path, char* icon_path, size_t size) {
    uint32_t inode_num = path_to_inode(path);
    if(inode_num == 0) return -1;
    
    rfs_inode_t* inode = &inode_table[inode_num - 1];
    
    strncpy(icon_path, inode->icon_path, size - 1);
    icon_path[size - 1] = '\0';
    
    return 0;
}

// Directory operations
int fs_readdir(const char* path, dirent_info_t* entries, uint32_t* count) {
    uint32_t inode_num = path_to_inode(path);
    if(inode_num == 0) return -1;
    
    rfs_inode_t* inode = &inode_table[inode_num - 1];
    
    if(inode->type != INODE_TYPE_DIR) return -1;
    
    uint32_t max_entries = *count;
    *count = 0;
    
    // Read directory entries
    uint32_t offset = 0;
    while(offset < inode->size && *count < max_entries) {
        rfs_dirent_t dirent;
        read_inode_data(inode, offset, &dirent, sizeof(rfs_dirent_t));
        
        entries[*count].inode = dirent.inode;
        entries[*count].type = dirent.type;
        strcpy(entries[*count].name, dirent.name);
        
        // Get file info
        rfs_inode_t* file_inode = &inode_table[dirent.inode - 1];
        entries[*count].size = file_inode->size;
        entries[*count].created = file_inode->created;
        entries[*count].modified = file_inode->modified;
        entries[*count].permissions = file_inode->permissions;
        strcpy(entries[*count].icon_path, file_inode->icon_path);
        
        (*count)++;
        offset += sizeof(rfs_dirent_t);
    }
    
    return 0;
}

// Utility functions
uint32_t path_to_inode(const char* path) {
    if(strcmp(path, "/") == 0) {
        return 1; // Root inode
    }
    
    uint32_t current_inode = 1;
    char path_copy[512];
    strcpy(path_copy, path);
    
    char* token = strtok(path_copy, "/");
    while(token) {
        current_inode = find_in_directory(current_inode, token);
        if(current_inode == 0) break;
        
        token = strtok(NULL, "/");
    }
    
    return current_inode;
}

uint32_t find_in_directory(uint32_t dir_inode, const char* name) {
    rfs_inode_t* inode = &inode_table[dir_inode - 1];
    
    if(inode->type != INODE_TYPE_DIR) return 0;
    
    uint32_t offset = 0;
    while(offset < inode->size) {
        rfs_dirent_t dirent;
        read_inode_data(inode, offset, &dirent, sizeof(rfs_dirent_t));
        
        if(strcmp(dirent.name, name) == 0) {
            return dirent.inode;
        }
        
        offset += sizeof(rfs_dirent_t);
    }
    
    return 0;
}

void sync_fs(void) {
    // Write superblock
    write_disk_block(0, superblock);
    
    // Write inode table
    write_disk_blocks(superblock->inode_table_start,
                     (superblock->inode_count * sizeof(rfs_inode_t)) / BLOCK_SIZE + 1,
                     inode_table);
    
    // Write bitmaps
    write_disk_blocks(superblock->block_bitmap_start,
                     (superblock->block_count / 8) / BLOCK_SIZE + 1,
                     block_bitmap);
    
    write_disk_blocks(superblock->inode_bitmap_start,
                     (superblock->inode_count / 8) / BLOCK_SIZE + 1,
                     inode_bitmap);
    
    // Flush journal
    flush_journal();
}