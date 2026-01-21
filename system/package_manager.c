#include "package_manager.h"
#include "../kernel/kernel.h"
#include "../fs/fs.h"
#include "../net/network.h"

static package_manager_t pkg_mgr;

int init_package_manager(void) {
    pkg_mgr.installed_count = 0;
    pkg_mgr.repository_count = 0;
    
    load_installed_packages();
    load_repositories();
    
    kprintf("Package manager initialized\n");
    return 0;
}

int install_package(const char* package_name) {
    package_info_t* pkg = find_package_in_repos(package_name);
    if(!pkg) return -1;
    
    if(check_dependencies(pkg) != 0) return -1;
    if(download_package(pkg) != 0) return -1;
    if(verify_package(pkg) != 0) return -1;
    if(extract_package(pkg) != 0) return -1;
    if(run_install_scripts(pkg) != 0) return -1;
    
    register_installed_package(pkg);
    return 0;
}

int download_package(package_info_t* pkg) {
    char download_path[512];
    snprintf(download_path, 512, "/tmp/%s-%s.rpkg", pkg->name, pkg->version);
    
    http_request_t request;
    init_http_request(&request);
    request.method = HTTP_GET;
    strncpy(request.url, pkg->download_url, 511);
    
    http_response_t response;
    if(http_send_request(&request, &response) != 0) return -1;
    
    int fd = fs_open(download_path, O_WRONLY | O_CREAT | O_TRUNC);
    if(fd < 0) return -1;
    
    fs_write(fd, response.body, response.body_length);
    fs_close(fd);
    
    strncpy(pkg->local_path, download_path, 511);
    return 0;
}

int update_package_database(void) {
    for(int i = 0; i < pkg_mgr.repository_count; i++) {
        repository_t* repo = &pkg_mgr.repositories[i];
        if(!repo->enabled) continue;
        
        char index_url[512];
        snprintf(index_url, 512, "%s/index.xml", repo->url);
        
        download_repository_index(repo, index_url);
        parse_repository_index(repo);
    }
    
    return 0;
}