#include "login.h"
int valid_users[100] = {0};
int valid_admins[10] = {0};
login_info display_login_page(){
    printf("\n====================\n");
    printf("Select Your Role: \n");
    printf("1. ADMIN\n");
    printf("2. USER\n");
    printf("====================\n");
    int response;
    printf("Enter your choice: ");
    scanf("%d", &response);
    Roles role = (Roles)response;
    bool found = 0;
    if(role == ROLE_ADMIN){
        printf("\nEnter your adminID: ");
    }else{
        printf("\nEnter your userID: ");
    }
    int id;
    scanf("%d",&id);
    login_info info = {role, id};
    return info;
}