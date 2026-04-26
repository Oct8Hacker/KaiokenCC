#include "../include/login.h"
int valid_users[100] = {0};
int valid_admins[10] = {0};
login_info display_login_page(int argc){
    printf("======================================\n");
    printf("        Select Your Role: \n");
    printf("            1. ADMIN\n");
    printf("            2. USER\n");
    printf("======================================\n");
    int response;
    printf("Enter your choice: ");
    scanf("%d", &response);
    Roles role = (Roles)response;
    if(argc == 1){
        assert((int)role == argc);
    }
    if(role == ROLE_ADMIN){
        printf("\nEnter your adminID: ");
    }else{
        printf("\nEnter your userID: ");
    }
    int id;
    scanf("%d",&id);
    LOG_DEBUG(id, "Login attempt recorded with role: %s", role == ROLE_ADMIN ? "ADMIN" : "USER");
    login_info info = {role, id};
    return info;
}