#include "../include/server.h"
uint32_t total_jobs = 0;
uint32_t active_jobs = 0;
int exit_signal = 0;
int cores;
pthread_mutex_t stats_lock = PTHREAD_MUTEX_INITIALIZER;
sem_t mutex;
int init_connection(int *sd){
    struct sockaddr_in serv;
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = INADDR_ANY;
    serv.sin_port = htons(PORT_NO);
    *sd = socket(AF_INET, SOCK_STREAM, 0);
    if(*sd == -1){
        LOG_ERROR(0,"Socket connection failed.");
        print_error(strerror(errno));
        return false;
    }
    LOG_SUCCESS(0,"Socket connection established at: %d", *sd);
    disable_nagle(*sd);
    int opt = 1;
    int val = setsockopt(*sd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if(val == -1){
        print_error(strerror(errno));
        close(*sd);
        LOG_ERROR(0,"Server connection failed.");
        return false;
    }
    if(bind(*sd, (struct sockaddr *)&serv, sizeof(serv)) == -1){
        print_error(strerror(errno));
        close(*sd);
        LOG_ERROR(0,"Server bind connection failed.");
        return false;
    }
    if(listen(*sd, CLIENT) == -1){
        print_error(strerror(errno));
        close(*sd);
        LOG_ERROR(0,"Server listen failed");
        return false;
    }
    LOG_SUCCESS(0,"Server successfully listening at port: %d, socket: %d and listening upto %d clients.", PORT_NO, *sd, CLIENT);
    return true;
}
void* handle_client(void* arg){
    int nsd = *(int*)arg;
    free(arg);
    if (sem_trywait(&mutex) != 0) {
        uint32_t response = htonl(0);
        write(nsd, &response, sizeof(uint32_t));
        close(nsd);
        LOG_WARN(0, "Thread %lu rejected client. CPU is full.", pthread_self());
        return NULL;
    }
    pthread_mutex_lock(&stats_lock);
    active_jobs++;
    pthread_mutex_unlock(&stats_lock);
    uint32_t response = htonl(1);
    write(nsd, &response, sizeof(uint32_t));
    LOG_SUCCESS(0, "Thread %lu started compilation.",pthread_self());
    printf("[Thread %lu] Handling new compilation job...\n", pthread_self());
    char temp_filename[] = "job_XXXXXX.c";
    if(!rcv_data(temp_filename, nsd, 0)){
        pthread_mutex_lock(&stats_lock);
        active_jobs--;
        pthread_mutex_unlock(&stats_lock);
        printf("[Thread %lu] job failed...\n",pthread_self());
        print_thread_error(strerror(errno));
        LOG_ERROR(0, "Thread %lu compilation failed in receiving files from client.",pthread_self());
        return NULL;
    }
    LOG_SUCCESS(0, "Thread %lu received file from client. File stored temporarily as %s.",pthread_self(), temp_filename);
    int fd[2];
    pid_t pid = fork();
    pipe(fd);
    if(pid == -1){
        print_thread_error("Fork Failed!");
        LOG_ERROR(0, "Thread %lu terminating as fork failed.",pthread_self());
        sem_post(&mutex);
        return NULL;
    }
    if(!pid){
        close(fd[0]);
        close(nsd);
        close(2);
        dup2(fd[1],2);
        close(fd[1]);
        char *arguments[] = {"gcc","-c",temp_filename,NULL};
        execvp("gcc", arguments);
        print_sys_error("exec failed.");
        LOG_ERROR(0, "Thread %lu terminating as exec failed.",pthread_self());
        sem_post(&mutex);
        return NULL;
    }else{
        waitpid(pid,NULL,0);
        close(fd[1]);
        char buffer[1024];
        int val = read(fd[0], &buffer, sizeof(buffer));
        if(val > 0){
            LOG_ERROR(0,"Thread %lu terminating as compilation failed with error message: %s", pthread_self(),buffer);
            pthread_mutex_lock(&stats_lock);
            active_jobs--;
            pthread_mutex_unlock(&stats_lock);
            close(nsd);
            unlink(temp_filename);
            sem_post(&mutex);
            return NULL;
        }
        char output_file[strlen(temp_filename) + 1];
        strcpy(output_file, temp_filename);
        output_file[strlen(temp_filename) - 1] = 'o';
        if(!send_data(output_file, nsd)){
            LOG_ERROR(0, "Thread %lu terminating as sending object file %s failed.",pthread_self(),output_file);
            print_sys_error("send data failed.");
        }
        pthread_mutex_lock(&stats_lock);
        active_jobs--;
        total_jobs++;
        pthread_mutex_unlock(&stats_lock);
        close(nsd);
        unlink(output_file);
        unlink(temp_filename);
        LOG_SUCCESS(0,"Files %s and object file %s are deleted.",temp_filename, output_file);
    }
    printf("[Thread %lu] Job finished.\n", pthread_self());
    sem_post(&mutex);
    return NULL;
}
void handle_ctrl_c(){
    printf("\n[Server]: Ctrl+C detected! Safely shutting down...\n");
    printf("Number of active jobs: %d\nTotal jobs: %d done before server exiting.\n", active_jobs, total_jobs);
    exit(0);
}
int main(){
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, handle_ctrl_c);
    generate_log_name();
    int status = init_log_file();
    if(status == -1){
        print_sys_error("Log file generation failed.");
        printf("Closing server.");
        exit(0);
    }
    LOG_SUCCESS(0,"Log file successfully created.");
    cores = get_nprocs();
    sem_init(&mutex,0,cores - 1);
    printf("Avaliable cores are: %d\n",cores);
    int sd;
    if(init_connection(&sd) == false){LOG_ERROR(0,"Could not set up server.");return 0;}
    LOG_SUCCESS(0,"Server Successfully hosted at port %d with socket at %d", PORT_NO, sd);
    while(1){
        int nsd = accept(sd, NULL, NULL);
        if(nsd == -1){
            LOG_ERROR(0, "Could not connect to client (accept) failed!");
            print_sys_error(strerror(errno));
            continue;
        }
        LOG_SUCCESS(0,"Successfully connect socket %d to client.", nsd);
        struct client_packet_header cph;
        if(read(nsd, &cph, sizeof(struct client_packet_header)) == -1){
            LOG_ERROR(0, "Could not intital header packet from client.");
            print_sys_error("Read Failed: Could not read Metadata from client to server.");
            continue;
        }
        cph.user_id = ntohl(cph.user_id);
        LOG_SUCCESS(cph.user_id,"Successfully connected to user");
        if(cph.rd == ROLE_ADMIN){
            if(cph.operation == 1){
                printf("Shutdown initiated. Waiting for %d active jobs to get compiled.\n", active_jobs);
                printf("Do you want to wait for other jobs to compile [Y/N]?\n");
                char res;
                scanf("%c", &res);
                res = toupper(res);
                if(res == 'Y'){
                    while(active_jobs != 0)
                        sched_yield();
                    printf("All jobs finished. Shutting down.\n");
                }
                LOG_SUCCESS(cph.user_id, "Server closed by user");
                exit(0);
            }else{
                uint32_t net_total = htonl(total_jobs);
                uint32_t active_total = htonl(active_jobs);
                write(nsd,&net_total,sizeof(uint32_t));
                write(nsd,&active_total,sizeof(uint32_t));
                LOG_SUCCESS(cph.user_id,"Admin asked for active and total jobs.");
            }
            close(nsd);
        }else if(cph.rd == ROLE_USER){
            if(cph.operation == COMPILE_FILE){  
                pthread_t thread_id;
                int *arg = (int *)malloc(sizeof(int));
                *arg = nsd;
                if(pthread_create(&thread_id, NULL, handle_client,(void *)(arg)) != 0){
                    print_sys_error("Thread creation failed at server side.");
                    LOG_ERROR(0,"Thread creation failed.");
                    close(nsd);
                    continue;
                }
                pthread_detach(thread_id);
            }else{
                LOG_ERROR(cph.user_id,"Packet got corrupted mid-way. Resend it.");
                close(nsd);
            }
        }else{
            close(nsd);
        }
    }
}