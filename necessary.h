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
#define PORT_NO 8080
typedef enum {
    ROLE_ADMIN = 1,
    ROLE_USER = 2,
} Roles;
struct client_packet_header{
    Roles rd;
    int operation;
};