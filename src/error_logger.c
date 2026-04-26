#include "../include/error_logger.h"
#include "../include/necessary.h"

void print_error(const char *message) {
    fprintf(stderr, "\033[1;31m[ERROR]\033[0m %s\n", message);
}

void print_sys_error(const char *context) {
    fprintf(stderr, "\033[1;31m[SYSTEM ERROR]\033[0m %s: %s\n", context, strerror(errno));
}

void print_thread_error(const char *context) {
    fprintf(stderr, "\033[1;33m[Thread %lu ERROR]\033[0m %s: %s\n", 
            pthread_self(), context, strerror(errno));
}