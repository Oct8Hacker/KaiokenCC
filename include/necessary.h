#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <assert.h>
#include <sys/wait.h>
#include <pthread.h>
#include <sched.h>
#include <sys/sysinfo.h>
#include <signal.h>
#include <errno.h>
#include "error_logger.h"
#include <time.h>
#include "log_helper.h"
#include <semaphore.h>
#include <ctype.h>
#define PORT_NO 8080
#define CLIENT 100
#define COMPILE_STATUS_OK 1
#define COMPILE_STATUS_ERR 0
typedef enum {
    ROLE_ADMIN = 1,
    ROLE_USER = 2,
} Roles;
struct client_packet_header{
    Roles rd;
    int operation;
    uint32_t user_id;
};
typedef enum{
    PING_IP = 1,
    COMPILE_FILE = 2,
    PING_IP_CONFIRMED = 3,
    PING_IP_NOT_AVALIABLE = 4
} User_OP;