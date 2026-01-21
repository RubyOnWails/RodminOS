#ifndef FS_H
#define FS_H

#include <stdint.h>
#include <stdbool.h>

// File system constants
#define RFS_MAGIC 0x52464653 // "RFS\0"
#define RFS_VERSION 1
#define BLOCK_SIZE 4096
#define MAX_FILENAME 255
#define MAX_PATH 512
#define MAX_OPEN_FILES 1024
#define MAX_MOUNT_POINTS 16

// File types
#define INODE_TYPE_FILE 1
#define INODE_TYPE_DIR  2
#define INODE_TYPE_LINK 3
#define INODE_TYPE_DEVICE 4

// Directory entry types
#define DIRENT_TYPE_FILE 1
#define DIRENT_TYPE_DIR  2
#define DIRENT_TYPE_LINK 3

// File flags
#define O_RDONLY 0x0000
#define O_WRONLY 0x0001
#define O_RDWR   0x0002
#define O_CREAT  0x0040
#define O_EXCL   0x0080
#define O_TRUNC  0x0200
#define O_APPEND 0x0400

// Seek constants
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

// File system types
#define FS_TYPE_RFS 1
#define FS_TYPE_NFS 2
#define FS_TYPE_SMB 3

// RFS Superblock
typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t block_size;
    uint32_t block_count;
    uint32_t inode_count;
    uint32_t free_blocks;
    uint32_t free_inodes;
    
    // Layout information
    uint32_t inode_table_start;
    uint32_t inode_bitmap_start;
    uint32_t block_bitmap_start;
    uint32_t data_start;
    uint32_t journal_start;
    uint32_t journal_size;
    
    // Timestamps
    uint64_t created;
    uint64_t mounted;
    uint64_t last_check;
    
    // Features
    uint32_t features;
    uint32_t readonly_features;
    uint32_t incompatible_features;
    
    char label[64];
    uint8_t uuid[16];
} rfs_superblock_t;

// RFS Inode
typedef struct {
    uint32_t type;
    uint32_t permissions;
    uint32_t uid;
    uint32_t gid;
    uint64_t size;
    uint32_t links;
    
    // Timestamps
    uint64_t created;
    uint64_t modified;
    uint64_t accessed;
    
    // Block pointers
    uint32_t direct_blocks[12];
    uint32_t indirect_block;
    uint32_t double_indirect_block;
    uint32_t triple_indirect_block;
    
    // Extended attributes
    char icon_path[256];
    uint32_t extended_attrs;
    
    // Encryption
    uint32_t encryption_key_id;
    uint8_t encryption_iv[16];
} rfs_inode_t;

// Directory entry
typedef struct {
    uint32_t inode;
    uint16_t type;
    uint16_t name_len;
    char name[MAX_FILENAME + 1];
} rfs_dirent_t;

// Open file structure
typedef struct {
    uint32_t inode_num;
    uint32_t flags;
    uint64_t offset;
    uint32_t ref_count;
} open_file_t;

// Mount point
typedef struct {
    uint32_t device;
    char path[MAX_PATH];
    uint32_t fs_type;
    void* superblock;
    uint32_t flags;
} mount_point_t;

// Directory entry info for readdir
typedef struct {
    uint32_t inode;
    uint32_t type;
    char name[MAX_FILENAME + 1];
    uint64_t size;
    uint64_t created;
    uint64_t modified;
    uint32_t permissions;
    char icon_path[256];
} dirent_info_t;

// Journal structures
typedef struct {
    uint32_t magic;
    uint32_t sequence;
    uint32_t type;
    uint32_t length;
    uint64_t timestamp;
} journal_header_t;

typedef struct {
    journal_header_t header;
    uint32_t inode_num;
    rfs_inode_t inode_data;
} journal_inode_entry_t;

typedef struct {
    journal_header_t header;
    uint32_t block_num;
    uint8_t block_data[BLOCK_SIZE];
} journal_block_entry_t;

typedef struct journal_transaction {
    uint32_t id;
    uint32_t entry_count;
    journal_header_t* entries[256];
    struct journal_transaction* next;
} journal_transaction_t;

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t start_block;
    uint32_t size;
    uint32_t head;
    uint32_t tail;
    uint32_t sequence;
} rfs_journal_t;

// File system operations
void fs_init(void);
void mount_root_fs(void);
void format_rfs(void);

// File operations
int fs_open(const char* path, int flags);
int fs_close(int fd);
ssize_t fs_read(int fd, void* buffer, size_t count);
ssize_t fs_write(int fd, const void* buffer, size_t count);
off_t fs_lseek(int fd, off_t offset, int whence);
int fs_stat(const char* path, struct stat* buf);
int fs_fstat(int fd, struct stat* buf);

// Directory operations
int fs_mkdir(const char* path, mode_t mode);
int fs_rmdir(const char* path);
int fs_readdir(const char* path, dirent_info_t* entries, uint32_t* count);

// File management
int fs_unlink(const char* path);
int fs_rename(const char* oldpath, const char* newpath);
int fs_link(const char* oldpath, const char* newpath);
int fs_symlink(const char* target, const char* linkpath);

// PPM icon support
int fs_set_icon(const char* path, const char* icon_path);
int fs_get_icon(const char* path, char* icon_path, size_t size);

// Extended attributes
int fs_setxattr(const char* path, const char* name, const void* value, size_t size);
int fs_getxattr(const char* path, const char* name, void* value, size_t size);
int fs_listxattr(const char* path, char* list, size_t size);

// Encryption
int fs_encrypt_file(const char* path, const char* key);
int fs_decrypt_file(const char* path, const char* key);

// Mount operations
int fs_mount(const char* device, const char* mountpoint, const char* fstype);
int fs_umount(const char* mountpoint);

// Utility functions
uint32_t path_to_inode(const char* path);
uint32_t find_in_directory(uint32_t dir_inode, const char* name);
void get_parent_path(const char* path, char* parent);
void get_filename(const char* path, char* filename);

// Block allocation
uint32_t alloc_block(void);
void free_block(uint32_t block);
uint32_t alloc_inode(void);
void free_inode(uint32_t inode);

// I/O operations
ssize_t read_inode_data(rfs_inode_t* inode, uint64_t offset, void* buffer, size_t count);
ssize_t write_inode_data(rfs_inode_t* inode, uint64_t offset, const void* buffer, size_t count);
void read_disk_block(uint32_t block, void* buffer);
void write_disk_block(uint32_t block, const void* buffer);
void read_disk_blocks(uint32_t start_block, uint32_t count, void* buffer);
void write_disk_blocks(uint32_t start_block, uint32_t count, const void* buffer);

// Journal operations
void init_journal(void);
journal_transaction_t* begin_transaction(void);
void commit_transaction(journal_transaction_t* trans);
void abort_transaction(journal_transaction_t* trans);
void log_inode_change(journal_transaction_t* trans, uint32_t inode_num, rfs_inode_t* inode);
void log_block_change(journal_transaction_t* trans, uint32_t block_num, const void* data);
void flush_journal(void);
void recover_journal(void);

// File system maintenance
void sync_fs(void);
int fsck_rfs(void);
void defrag_rfs(void);

// Permissions and security
bool check_permissions(rfs_inode_t* inode, int flags);
int change_permissions(const char* path, mode_t mode);
int change_owner(const char* path, uid_t uid, gid_t gid);

// File system information
uint64_t get_disk_size(void);
uint64_t get_free_space(void);
uint64_t get_used_space(void);

// Network file systems
int nfs_mount(const char* server, const char* path, const char* mountpoint);
int smb_mount(const char* server, const char* share, const char* mountpoint);

// Standard types
typedef uint32_t mode_t;
typedef uint32_t uid_t;
typedef uint32_t gid_t;
typedef int64_t off_t;
typedef int64_t ssize_t;

struct stat {
    uint32_t st_ino;
    uint32_t st_mode;
    uint32_t st_nlink;
    uint32_t st_uid;
    uint32_t st_gid;
    uint64_t st_size;
    uint64_t st_atime;
    uint64_t st_mtime;
    uint64_t st_ctime;
};

// String functions
char* strtok(char* str, const char* delim);

#endif