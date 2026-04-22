#include "necessary.h"
#include <pthread.h>
int disable_nagle(int fd){
    int opt = 1;
    return setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
}
void init_connection(int *sd){
    struct sockaddr_in serv;
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = INADDR_ANY;
    serv.sin_port = htons(PORT_NO);
    *sd = socket(AF_INET, SOCK_STREAM, 0);
    disable_nagle(*sd);
    bind(*sd, (struct sockaddr *)&serv, sizeof(serv));
    listen(*sd, 5);
}
void* handle_client(void* arg){
    int nsd = *(int*)arg;
    char temp_filename[] = "job_XXXXXX.c";
    uint32_t file_size;
    read(nsd, &file_size, sizeof(uint32_t));
    file_size = ntohl(file_size);
    int fd = mkstemps(temp_filename, 2);
    char buffer[4096];
    while(file_size > 0){
        int c = read(nsd, buffer, sizeof(buffer));
        assert(c == write(fd, buffer, c));
        file_size -= c;
    }
    close(nsd);
}
int main(){
    int sd;
    init_connection(&sd);
    while(1){
        int nsd = accept(sd, NULL, NULL);
        pthread_t thread_id;
        pthread_create(&thread_id, NULL, handle_client,(void *)(&nsd));
        pthread_detach(thread_id);
    }
}