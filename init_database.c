#include "include/necessary.h"
int main(void){
    int fd;
    fd = open("/home/aryan_khadgi/distributed_compiler/data/admin.bin",O_WRONLY | O_CREAT,0777);
    int dummy_id = 100;
    while(dummy_id < 115){
        write(fd,&dummy_id,sizeof(int));
        dummy_id++;
    }
    close(fd);
    dummy_id = 0;
    fd = open("/home/aryan_khadgi/distributed_compiler/data/user.bin",O_WRONLY | O_CREAT, 0777);
    while(dummy_id < 100){
        write(fd,&dummy_id,sizeof(int));
        dummy_id++;
    }
    close(fd);
}