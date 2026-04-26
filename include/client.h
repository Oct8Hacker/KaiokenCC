#pragma once
#include "helper.h"
#include "login.h"
#include "error_logger.h"
struct thread_args{
    char* file_name;
    int* sd;  
    int user_id;  
};
extern int valid_users[100];
extern int valid_admins[10];
bool init_connection(int *sd, char* ip);
void *send_multiple_files(void *arg);
