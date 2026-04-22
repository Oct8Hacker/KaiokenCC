#include "helper.h"
int is_c_file(const char *filename) {
    const char *ext = strrchr(filename, '.');
    if (!ext || ext == filename) return 0;
    return strcmp(ext, ".c") == 0;
}
bool check_files(char* file_name){
    if(access(file_name,F_OK)){
        printf("File %s does not exist!\n", file_name);
        return false;
    }else if(!is_c_file(file_name)){
        printf("File %s is not a C file\n",file_name);
        return false;
    }
    return true;
}