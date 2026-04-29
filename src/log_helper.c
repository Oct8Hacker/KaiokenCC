#include "../include/log_helper.h"
#include "../include/error_logger.h"
char log_file_name[100];
char *type[LOG_LEVEL_LEN] = {
    "DEBUG",
    "SUCCESS",
    "WARN",
    "ERR"
};
time_t current_time;
struct tm * m_time; 
int init_log_file(){
    int fd = open(log_file_name, O_CREAT, 0777);
    if(fd == -1){
        print_sys_error("Could not generate log file");
        return -1;
    }
    printf("Log file successfully created.\n");
    close(fd);
    return 1;
}
void generate_log_name(){
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    snprintf(log_file_name, sizeof(log_file_name), "log_%04d%02d%02d_%02d%02d%02d.log", 
         t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
         t->tm_hour, t->tm_min, t->tm_sec);
    return;
}
void ulogger_log(LOG_LEVEL level,int user_id, const char* fmt, ...){
    time(&current_time);
    m_time = localtime(&current_time);
    va_list args;
    va_start(args,fmt);
    char message[1024];
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);
    if (level == LOG_ERR && errno != 0) {
        snprintf(message + strlen(message), sizeof(message) - strlen(message), " (OS Reason: %s)", strerror(errno));
    }
    int fd = open(log_file_name,O_WRONLY | O_APPEND, 0777);
    if(fd != -1){
        struct flock lock;
        lock.l_type = F_WRLCK;
        lock.l_whence = SEEK_SET;
        lock.l_start = 0;
        lock.l_len = 0;
        fcntl(fd,F_SETLKW, &lock);
        char file_entry[1500];
        snprintf(file_entry, sizeof(file_entry), "[%04d-%02d-%02d %02d:%02d:%02d] [%-7s] [Thread %lu] [User %d] %s\n",
                 m_time->tm_year + 1900, m_time->tm_mon + 1, m_time->tm_mday,
                 m_time->tm_hour, m_time->tm_min, m_time->tm_sec,
                 type[level], pthread_self(), user_id, message);
        write(fd, file_entry, strlen(file_entry));
        lock.l_type = F_UNLCK;
        fcntl(fd, F_SETLK, &lock);
        close(fd);
    }else{
        print_sys_error("Open Failed.");
    }
    errno = 0;
}