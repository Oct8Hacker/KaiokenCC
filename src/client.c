#include "../include/client.h"

bool init_connection(int *sd, char* ip){
    struct sockaddr_in serv;
    *sd = socket(AF_INET, SOCK_STREAM, 0);
    if(*sd == -1){
        LOG_ERROR(0,"Socket creation failed.");
        return false;
    }
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = inet_addr(ip);
    serv.sin_port = htons(PORT_NO);
    disable_nagle(*sd);
    LOG_SUCCESS(0,"Nagle disabled.");
    if(connect(*sd, (struct sockaddr *)&serv, sizeof(serv)) == -1){
        LOG_ERROR(0,"Client failed to connect to server.");
        print_sys_error(strerror(errno));
        close(*sd);
        return false;
    }
    return true;
}
void* send_multiple_files(void *arg){
    struct thread_args* args = (struct thread_args*)(arg);
    struct client_packet_header cph = {ROLE_USER, COMPILE_FILE, 0};
    LOG_SUCCESS(args->user_id, "Thread [%lu] has started sending files...", pthread_self());
    if(write(*args->sd, &cph, sizeof(struct client_packet_header)) == -1){
        LOG_ERROR(args->user_id, "Failed to write header: %s.", strerror(errno));
        print_sys_error(strerror(errno));
        return NULL;
    }
    LOG_DEBUG(args->user_id, "Sending file %s to server.", args->file_name);
    send_data(args->file_name, *(args->sd));
    char output_file[strlen(args->file_name) + 1];
    strcpy(output_file, args->file_name);
    output_file[strlen(args->file_name) - 1] = 'o';
    LOG_DEBUG(args->user_id, "File Output File: %s.",output_file);
    if(!rcv_data(output_file, *(args->sd), 1)){
        LOG_ERROR(args->user_id, "Compilation failed for %s.", args->file_name);
        print_error("Compilation failed\n");
        return NULL;
    }
    close(*(args->sd));
    LOG_SUCCESS(args->user_id, "Compilation successful for %s.", args->file_name);
    printf("Compilation successfull for %s\n",args->file_name);
    free(args->sd);
    free(arg);
    return NULL;
}
int main(int argc, char* argv[]){
    generate_log_name();
    if(init_log_file() == -1){
        return -1;
    }
    //temporary users and admin ids assigned
    for(int i = 0;i<10;i++){
        valid_admins[i] = 10 + i;
    }
    for(int i = 0;i<100;i++){
        valid_users[i] = 100 + i;
    }
    LOG_SUCCESS(0,"Log file successfully created.");
    clock_t begin = clock();
    signal(SIGPIPE, SIG_IGN);
    char *files[argc- 1];
    for(int i = 0;i<argc - 1;i++){
        files[i] = argv[i + 1];
        if(!check_files(files[i])){
            LOG_ERROR(0,"File no: %d is invalid.", i + 1);
            return 0;
        }
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
        LOG_ERROR(info.id, "Enter a valid username.");
        print_error("Enter a valid username.");
        exit(1);
    }
    if(info.rd == ROLE_ADMIN){
        printf("1. Remote Server Shutdown\n2. Server Status\n");
        int op;
        scanf("%d",&op);
        int* sd = malloc(sizeof(int));
        while(init_connection(sd,"127.0.0.1") == false) { sched_yield(); }
        LOG_SUCCESS(info.id, "Admin successfully connected to server at IP 127.0.0.1");
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
        char *server_ip[2] = {"127.0.0.1","127.0.0.1"};
        int next_server = 0;
        for (int i = 0; i < argc - 1; i++){
            ping_server:
            char *avaliable_ip = "127.0.0.1";
            int target_index = -1;
            for(int k = 0; k < 2; k++){
                int j = (next_server + k) % 2;
                struct client_packet_header cph = {ROLE_USER, PING_IP, info.id};
                int* packet_sd = malloc(sizeof(int));
                if(init_connection(packet_sd, server_ip[j]) == false){ free(packet_sd); continue; }
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
                LOG_WARN(info.id, "No available servers as of now. Waiting for 0.15ms .");
                print_error("No Avaliable Servers as of now please wait for some time.");
                usleep(1500);
                goto ping_server;
            }
            int *sd = (int *)malloc(sizeof(int));
            if(init_connection(sd, avaliable_ip) == false){ free(sd);continue; }
            LOG_SUCCESS(info.id, "Successfully connected to server at IP %s.", avaliable_ip);
            printf("Successfully connected to server at IP %s.\n", avaliable_ip);
            struct thread_args* args = malloc(sizeof(struct thread_args));
            args->file_name = files[i];
            args->sd = sd;
            args->user_id = info.id;
            pthread_t thread_id;
            thread_creation:
            if(pthread_create(&thread_id, NULL, send_multiple_files, (void *)args) != 0){
                LOG_ERROR(info.id, "Thread creation failed: %s.", strerror(errno));
                print_sys_error(strerror(errno));
                usleep(12000);
                goto thread_creation;
            }
            threads[i] = thread_id;
            if (target_index != -1){
                next_server = (target_index + 1) % 2; 
            }
            usleep(15000);
        }
        for(int i = 0;i<argc - 1;i++){
            pthread_join(threads[i],NULL);
        }
    }
    clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    LOG_SUCCESS(info.id, "Client completed compilation in %.15lf seconds.", time_spent);
    printf("Files compiled in %.8lf.\n", time_spent);
    return 0;
}