#include "../include/client.h"

bool init_connection(int *sd, char* ip){
    struct sockaddr_in serv;
    *sd = socket(AF_INET, SOCK_STREAM, 0);
    if(*sd == -1)return false;
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = inet_addr(ip);
    serv.sin_port = htons(PORT_NO);
    disable_nagle(*sd);
    if(connect(*sd, (struct sockaddr *)&serv, sizeof(serv)) == -1){print_sys_error(strerror(errno));close(*sd);return false;}
    return true;
}
void* send_multiple_files(void *arg){
    struct thread_args* args = (struct thread_args*)(arg);
    struct client_packet_header cph = {ROLE_USER, COMPILE_FILE, 0};
    if(write(*args->sd, &cph, sizeof(struct client_packet_header)) == -1){
        print_sys_error(strerror(errno));
        return NULL;
    }
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
    signal(SIGPIPE, SIG_IGN);
    char *files[argc- 1];
    for(int i = 0;i<argc - 1;i++){
        files[i] = argv[i + 1];
        if(!check_files(files[i]))return 0;
    }
    login_info info = display_login_page(argc);
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
        print_error("Enter a valid username.");
        exit(1);
    }
    if(info.rd == ROLE_ADMIN){
        printf("1. Remote Server Shutdown\n2. Server Status\n");
        int op;
        scanf("%d",&op);
        int* sd = malloc(sizeof(int));
        while(init_connection(sd,"127.0.0.1") == false) { sched_yield(); }
        printf("Successfully connected to server at IP %s\n", "127.0.0.1");
        struct client_packet_header cph = { ROLE_ADMIN,op,info.id };
        write(*sd, &cph,sizeof(struct client_packet_header));
        if(op == 2){
            uint32_t total_jobs;
            uint32_t active_jobs;
            read(*sd, &total_jobs, sizeof(int));
            read(*sd, &active_jobs, sizeof(int));
            total_jobs = ntohl(total_jobs);
            active_jobs = ntohl(active_jobs);
            printf("Number of active jobs: %d\n Number of total jobs: %d\n", active_jobs, total_jobs);
        }
        close(*sd);
        free(sd);
    }else{
        pthread_t threads[argc - 1];
        int *sd_header = (int *)malloc(sizeof(int));
        char *server_ip[1] = {"127.0.0.1"};
        int next_server = 0;
        for (int i = 0; i < argc - 1; i++){
            ping_server:
            char *avaliable_ip = "127.0.0.1";
            int target_index = -1;
            for(int k = 0; k < 1; k++){
                int j = (next_server + k) % 1;
                struct client_packet_header cph = {ROLE_USER, PING_IP, info.id};
                int* packet_sd = malloc(sizeof(int));
                while(init_connection(packet_sd, server_ip[j]) == false){ sched_yield(); }
                write(*packet_sd, &cph,sizeof(struct client_packet_header));
                uint32_t response;
                read(*packet_sd,&response, sizeof(uint32_t));
                response = ntohl(response);
                close(*packet_sd);
                free(packet_sd);
                if(response == 1){
                    avaliable_ip = server_ip[j];
                    target_index = j;
                    break;
                }
            }
            if(avaliable_ip == NULL){
                print_error("No Avaliable Servers as of now please wait for some time.");
                usleep(1500);
                goto ping_server;
            }
            int *sd = (int *)malloc(sizeof(int));
            while(init_connection(sd, avaliable_ip) == false){ sched_yield(); }
            printf("Successfully connected to server at IP %s\n", avaliable_ip);
            struct thread_args* args = malloc(sizeof(struct thread_args));
            args->file_name = files[i];
            args->sd = sd;
            args->user_id = info.id;
            pthread_t thread_id;
            thread_creation:
            if(pthread_create(&thread_id, NULL, send_multiple_files, (void *)args) != 0){
                print_sys_error(strerror(errno));
                usleep(12000);
                goto thread_creation;
            }
            threads[i] = thread_id;
            if (target_index != -1){
                next_server = (target_index + 1) % 1; 
            }
            usleep(1500);
        }
        for(int i = 0;i<argc - 1;i++){
            pthread_join(threads[i],NULL);
        }
    }
    return 0;
}