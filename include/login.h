#pragma once
#include "necessary.h"
extern int valid_users[100];
extern int valid_admins[10];
typedef enum {
    NOT_OK,
    OK
} Status;
typedef struct {
    Roles rd;
    int id;
} login_info;
login_info display_login_page(int argc);