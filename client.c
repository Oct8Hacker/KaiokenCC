#include "client.h"

bool init_connection(int *sd){
    struct sockaddr_in serv;
    *sd = socket(AF_INET, SOCK_STREAM, 0);
    if(*sd == -1)return false;
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv.sin_port = htons(PORT_NO);
    disable_nagle(*sd);
    if(connect(*sd, (struct sockaddr *)&serv, sizeof(serv)) == -1){perror("connect");close(*sd);return false;}
    return true;
}
void* send_multiple_files(void *arg){
    struct thread_args* args = (struct thread_args*)(arg);
    struct client_packet_header cph = {ROLE_USER, 0};
    write(*args->sd, &cph, sizeof(struct client_packet_header));
    send_data(args->file_name, *(args->sd));
    char output_file[strlen(args->file_name) + 1];
    strcpy(output_file, args->file_name);
    output_file[strlen(args->file_name) - 1] = 'o';
    rcv_data(output_file, *(args->sd), 1);
    close(*(args->sd));
    free(args->sd);
    free(arg);
    return NULL;
}
int main(int argc, char* argv[]){
    if(argc <= 1){
        printf("Run the program with some files\n");
        return 0;
    }
    getchar();
    char *files[argc- 1];
    for(int i = 0;i<argc - 1;i++){
        files[i] = argv[i + 1];
        if(!check_files(files[i]))return 0;
    }
    login_info info = display_login_page();
    bool found = false;
    if(info.rd == ROLE_ADMIN){
        for(int i = 0;i<10;i++){
            found |= (info.id == valid_admins[i]);
        }
    }else{
        for(int i = 0;i<100;i++){
            found |= (info.id == valid_users[i]);
        }
    }
    if(!found){
        printf("\nEnter a valid User/Admin ID\n");
        exit(1);
    }
    if(info.rd == ROLE_ADMIN){
        printf("\n1. Remote Server Shutdown\n2. Server Status\n");
        int op;
        scanf("%d",&op);
        int* sd = malloc(sizeof(int));
        while(init_connection(sd) == false) { sched_yield(); }
        struct client_packet_header cph = { ROLE_ADMIN,op };
        write(*sd, &cph,sizeof(struct client_packet_header));
        if(op == 2){
            uint32_t total_jobs;
            uint32_t active_jobs;
            read(*sd, &total_jobs, sizeof(int));
            read(*sd, &active_jobs, sizeof(int));
            total_jobs = ntohl(total_jobs);
            active_jobs = ntohl(active_jobs);
            printf("Number of active jobs: %d, Number of total jobs: %d\n", active_jobs, total_jobs);
        }
        close(*sd);
        free(sd);
    }else{
        pthread_t threads[argc - 1];
        int *sd_header = (int *)malloc(sizeof(int));
        for(int i = 0;i<argc - 1;i++){
            int *sd = (int *)malloc(sizeof(int));
            while(init_connection(sd) == false){ sched_yield(); }
            struct thread_args* args = malloc(sizeof(struct thread_args));
            args->file_name = files[i];
            args->sd = sd;
            pthread_t thread_id;
            pthread_create(&thread_id, NULL, send_multiple_files, (void *)args);
            threads[i] = thread_id;
        }
        for(int i = 0;i<argc - 1;i++){
            pthread_join(threads[i],NULL);
        }
    }
    return 0;
}