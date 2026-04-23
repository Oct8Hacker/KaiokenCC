#include "necessary.h"
#include "helper.h"
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
    free(arg);
    printf("[Thread %lu] Handling new compilation job...\n", pthread_self());
    char temp_filename[] = "job_XXXXXX.c";
    rcv_data(temp_filename, nsd, 0);
    if(!fork()){
        close(nsd);
        char *arguments[] = {"gcc","-c",temp_filename,NULL};
        execvp("gcc", arguments);
    }else{
        wait(NULL);
        char output_file[strlen(temp_filename) + 1];
        strcpy(output_file, temp_filename);
        output_file[strlen(temp_filename) - 1] = 'o';
        send_data(output_file, nsd);
        close(nsd);
    }
    printf("[Thread %lu] Job finished.\n", pthread_self());
    return NULL;
}
int main(){
    int sd;
    init_connection(&sd);
    while(1){
        int nsd = accept(sd, NULL, NULL);
        pthread_t thread_id;
        //malloc karna padega
        int *arg = (int *)malloc(sizeof(int));
        *arg = nsd;
        pthread_create(&thread_id, NULL, handle_client,(void *)(arg));
        pthread_detach(thread_id);
    }
}