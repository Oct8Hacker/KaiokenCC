#include "../include/helper.h"
int disable_nagle(int fd){
    int opt = 1;
    return setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
}
int is_c_file(const char *filename) {
    const char *ext = strrchr(filename, '.');
    if (!ext || ext == filename) return 0;
    return strcmp(ext, ".c") == 0;
}
bool check_files(char* file_name){
    if(access(file_name,F_OK)){
        printf("File %s does not exist!\n", file_name);
        return false;
    }else if(!is_c_file(file_name)){
        printf("File %s is not a C file\n",file_name);
        return false;
    }
    return true;
}
void send_data(char *file_name, int sd){
    struct stat sb;
    stat(file_name,&sb);
    uint32_t file_size = htonl((uint32_t)sb.st_size);
    int val = write(sd, &file_size, sizeof(uint32_t));
    if(val < 0){
        print_sys_error("File transfer from host to server failed");
        return;
    }
    char buffer[4096];
    int fd = open(file_name,O_RDONLY);
    if(fd < 0){
        print_sys_error("Open Failed on server side.");
        return;
    }
    int sz;
    while((sz = read(fd,buffer,sizeof(buffer))) > 0){   
        write(sd,buffer,sz);
    }
    close(fd);
}
bool rcv_data(char* file_name, int nsd, bool state){
    uint32_t file_size;
    int val = read(nsd, &file_size, sizeof(uint32_t));
    if(val < 0){
        print_sys_error("Server could not receive data from client.");
        return false;
    }
    file_size = ntohl(file_size);
    int fd;
    if(state == 0)
        fd = mkstemps(file_name, 2);
    else 
        fd = open(file_name, O_CREAT | O_RDWR | O_TRUNC,0777);
    if(fd < 0){
        print_sys_error("Open Failed on Server Side.");
        return false;
    }
    char buffer[4096];
    int read_data;
    while(file_size > 0){
        int c = read(nsd, buffer, sizeof(buffer));
        if(c <= 0){
            print_error("Read the file failed!");
            close(fd);
            return false;
        }
        assert(c == write(fd, buffer, c));
        file_size -= c;
    }
    close(fd);
    return true;
}