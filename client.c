#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
int is_c_file(const char *filename) {
    const char *ext = strrchr(filename, '.');
    if (!ext || ext == filename) return 0;
    return strcmp(ext, ".c") == 0;
}
int main(int argc, char* argv[]){
    if(argc <= 1){
        printf("Run the program with some files\n");
        return 0;
    }
    char *files[argc- 1];
    for(int i = 0;i<argc - 1;i++){
        files[i] = argv[i + 1];
        if(access(files[i],F_OK)){
            printf("File %s does not exist!\n", files[i]);
            return 0;
        }else if(!is_c_file(files[i])){
            printf("File %s is not a C file\n",files[i]);
            return 0;
        }
    }

}