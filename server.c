#include "server.h"
uint32_t total_jobs = 0;
uint32_t active_jobs = 0;
uint32_t accepted_jobs = 0;
int exit_signal = 0;
int cores;
pthread_mutex_t stats_lock = PTHREAD_MUTEX_INITIALIZER;

void init_connection(int *sd){
    struct sockaddr_in serv;
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = INADDR_ANY;
    serv.sin_port = htons(PORT_NO);
    *sd = socket(AF_INET, SOCK_STREAM, 0);
    disable_nagle(*sd);
    int opt = 1;
    setsockopt(*sd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if(bind(*sd, (struct sockaddr *)&serv, sizeof(serv)) == -1){
        perror("bind");
        close(*sd);
        return;
    }
    if(listen(*sd, 100) == -1){
        perror("listen");
        close(*sd);
        return;
    }
}
void* handle_client(void* arg){
    int nsd = *(int*)arg;
    free(arg);
    printf("[Thread %lu] Handling new compilation job...\n", pthread_self());
    char temp_filename[] = "job_XXXXXX.c";
    rcv_data(temp_filename, nsd, 0);
    pthread_mutex_lock(&stats_lock);
    while (active_jobs >= cores){
        pthread_mutex_unlock(&stats_lock);
        sched_yield();
        pthread_mutex_lock(&stats_lock);
    }
    active_jobs++;
    pthread_mutex_unlock(&stats_lock);
    pid_t pid = fork();
    if(!pid){
        close(nsd);
        char *arguments[] = {"gcc","-c",temp_filename,NULL};
        execvp("gcc", arguments);
        perror("execvp");
        exit(1);
    }else{
        waitpid(pid,NULL,0);
        pthread_mutex_lock(&stats_lock);
        active_jobs--; 
        pthread_mutex_unlock(&stats_lock);
        char output_file[strlen(temp_filename) + 1];
        strcpy(output_file, temp_filename);
        output_file[strlen(temp_filename) - 1] = 'o';
        send_data(output_file, nsd);
        close(nsd);
        unlink(output_file);
        unlink(temp_filename);
    }
    pthread_mutex_lock(&stats_lock);
    total_jobs++;
    accepted_jobs--;
    pthread_mutex_unlock(&stats_lock);
    printf("[Thread %lu] Job finished.\n", pthread_self());
    return NULL;
}
int main(){
    signal(SIGPIPE, SIG_IGN);
    cores = get_nprocs();
    printf("avaliable cores are: %d\n",cores);
    int sd;
    init_connection(&sd);
    while(1){
        int nsd = accept(sd, NULL, NULL);
        if(nsd == -1){
            perror("nsd");
            continue;
        }
        struct client_packet_header cph;
        read(nsd, &cph, sizeof(struct client_packet_header));
        if(cph.rd == ROLE_ADMIN){
            if(cph.operation == 1){
                printf("Shutdown initiated. Waiting for %d active jobs to get compiled.\n", active_jobs);
                while (active_jobs != 0)
                    sched_yield();
                printf("All jobs finished. Shutting down.\n");
                exit(0);
            }else{
                uint32_t net_total = htonl(total_jobs);
                uint32_t active_total = htonl(active_jobs);
                write(nsd,&net_total,sizeof(uint32_t));
                write(nsd,&active_total,sizeof(uint32_t));
            }
            close(nsd);
        }else if(cph.rd == ROLE_USER){
            if(cph.operation == COMPILE_FILE){
                pthread_mutex_lock(&stats_lock);
                accepted_jobs++;
                pthread_mutex_unlock(&stats_lock);
                pthread_t thread_id;
                //malloc karna padega
                int *arg = (int *)malloc(sizeof(int));
                *arg = nsd;
                pthread_create(&thread_id, NULL, handle_client,(void *)(arg));
                pthread_detach(thread_id);
            }else{
                uint32_t threads_avaliable = htonl(cores > accepted_jobs);
                write(nsd, &threads_avaliable, sizeof(uint32_t));
                close(nsd);
            }
        }else{
            close(nsd);
        }
    }
}