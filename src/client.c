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
// Read an exact number of bytes from the socket.
static int read_all(int fd, void *buf, size_t len){
    size_t off = 0;
    char *ptr = (char *)buf;
    while(off < len){
        ssize_t n = read(fd, ptr + off, len - off);
        if(n <= 0){
            return -1;
        }
        off += (size_t)n;
    }
    return 0;
}
void* send_multiple_files(void *arg){
    struct thread_args* args = (struct thread_args*)(arg);
    LOG_DEBUG(args->user_id, "Sending file %s to server.", args->file_name);
    send_data(args->file_name, *(args->sd));    
    // Protocol: server sends compile status before any object data.
    uint32_t status_net = 0;
    if(read_all(*(args->sd), &status_net, sizeof(status_net)) != 0){
        LOG_ERROR(args->user_id, "Failed to read compile status for %s.", args->file_name);
        print_error("Failed to read compile status.");
        close(*(args->sd));
        free(args->sd);
        free(arg);
        return NULL;
    }
    uint32_t status = ntohl(status_net);
    if(status != COMPILE_STATUS_OK){
        // Protocol: read error length + error text and print to stderr.
        uint32_t len_net = 0;
        if(read_all(*(args->sd), &len_net, sizeof(len_net)) == 0){
            uint32_t len = ntohl(len_net);
            if(len > 0){
                char *err_buf = (char *)malloc(len + 1);
                if(err_buf != NULL && read_all(*(args->sd), err_buf, len) == 0){
                    err_buf[len] = '\0';
                    fprintf(stderr, "\n[GCC ERROR]\n%.*s\n", (int)len, err_buf);
                }else{
                    print_error("Compilation failed (error message read failed).");
                }
                free(err_buf);
            }else{
                print_error("Compilation failed (no error message)." );
            }
        }else{
            print_error("Compilation failed (no error length)." );
        }
        close(*(args->sd));
        free(args->sd);
        free(arg);
        return NULL;
    }
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
    int val;
    if(info.rd == ROLE_ADMIN){
        int fd_admin = open("/home/aryan_khadgi/distributed_compiler/data/admin.bin", O_RDONLY, 0677);
        if(fd_admin < 0){
            print_sys_error("Could not open Admin database.");
        }
        while(read(fd_admin,&val, sizeof(int)) > 0){
            if(val == info.id){found = true;break;}
        }
        close(fd_admin);
    }else{
        int fd_user = open("/home/aryan_khadgi/distributed_compiler/data/user.bin", O_RDONLY, 0677);
        if(fd_user < 0){
            print_sys_error("Could not open User database.");
        }
        while(read(fd_user,&val, sizeof(int)) > 0){
            if(val == info.id){found = true;break;}
        }
        close(fd_user);
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
            int *sd = (int *)malloc(sizeof(int));
            char *target_ip = NULL;
            find_server:
            for(int k = 0; k < 2; k++){
                int j = (next_server + k) % 2;
                if(init_connection(sd, server_ip[j]) == false){ continue; }
                struct client_packet_header file_cph = {ROLE_USER, COMPILE_FILE, htonl((uint32_t)info.id)};
                write(*sd, &file_cph, sizeof(struct client_packet_header));
                uint32_t server_response;
                read(*sd, &server_response, sizeof(uint32_t));
                server_response = ntohl(server_response);
                if(server_response == 1){
                    target_ip = server_ip[j];
                    next_server = (j + 1) % 2;
                    break; 
                }else{
                    close(*sd);
                }
            }
            if(target_ip == NULL){
                LOG_WARN(info.id, "All servers full. Waiting 100ms before retrying.");
                usleep(100000); 
                goto find_server;
            }
            LOG_SUCCESS(info.id, "Successfully claimed core on IP %s.", target_ip);
            printf("Successfully claimed core on IP %s.\n", target_ip);
            struct thread_args* args = malloc(sizeof(struct thread_args));
            args->file_name = files[i];
            args->sd = sd;
            args->user_id = info.id;
            if(pthread_create(&threads[i], NULL, send_multiple_files, (void *)args) != 0){
                LOG_ERROR(info.id, "Thread creation failed: %s.", strerror(errno));
                print_sys_error(strerror(errno));
            }
            usleep(15000);
        }
        for(int i = 0; i < argc - 1; i++){
            pthread_join(threads[i], NULL);
        }
    }
    clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    LOG_SUCCESS(info.id, "Client completed compilation in %.15lf seconds.", time_spent);
    printf("Client ran for %.8lf.\n", time_spent);
    return 0;
}