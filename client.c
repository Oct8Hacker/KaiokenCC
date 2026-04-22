#include "necessary.h"
#include "helper.h"

int disable_nagle(int fd){
    int opt = 1;
    return setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
}
void init_connection(int *sd){
    struct sockaddr_in serv;
    *sd = socket(AF_INET, SOCK_STREAM, 0);
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv.sin_port = htons(PORT_NO);
    disable_nagle(*sd);
    connect(*sd, (struct sockaddr *)&serv, sizeof(serv));
}
void send_data(char *file_name, int sd){
    struct stat sb;
    stat(file_name,&sb);
    uint32_t file_size = htonl((uint32_t)sb.st_size);
    write(sd, &file_size, sizeof(uint32_t));
    char buffer[4096];
    int fd = open(file_name,O_RDONLY);
    int sz;
    while((sz = read(fd,buffer,sizeof(buffer))) > 0){
        write(sd,buffer,sz);
    }
    close(fd);
}

int main(int argc, char* argv[]){
    if(argc <= 1){
        printf("Run the program with some files\n");
        return 0;
    }
    // getchar();
    char *files[argc- 1];
    for(int i = 0;i<argc - 1;i++){
        files[i] = argv[i + 1];
        if(!check_files(files[i]))return 0;
    }
    int sd;
    init_connection(&sd);
    for(int i = 0;i<argc - 1;i++){
        send_data(files[i],sd);
    }
    return 0;
}