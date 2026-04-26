#pragma once
#include "necessary.h"
#include <stdarg.h>
int init_log_file();
void generate_log_name();
// https://medium.com/@m0_/writing-a-simple-logging-system-in-c-f8fd90a166db
extern char log_file_name[100];
extern time_t current_time;
extern struct tm *m_time; 
typedef enum {
    LOG_DBG,
    LOG_SUCCESS,
    LOG_WARN,
    LOG_ERR,
    LOG_LEVEL_LEN
} LOG_LEVEL;
extern char *type[LOG_LEVEL_LEN];

// char *colors[LOG_LEVEL_LEN] = {
//     "\x1b[0m",      // Reset/White for Debug
//     "\x1b[32m",     // Green for Success
//     "\x1b[1;33m",   // Yellow for Warn
//     "\x1b[31m"      // Red for Err
// };
void ulogger_log(LOG_LEVEL level,int user_id, const char* fmt, ...);
#define LOG_DEBUG(id, ...)   ulogger_log(LOG_DBG, id, __VA_ARGS__)
#define LOG_SUCCESS(id, ...) ulogger_log(LOG_SUCCESS, id, __VA_ARGS__)
#define LOG_WARN(id, ...)    ulogger_log(LOG_WARN, id, __VA_ARGS__)
#define LOG_ERROR(id, ...)   ulogger_log(LOG_ERR, id, __VA_ARGS__)
