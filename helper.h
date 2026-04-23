#pragma once
#include "necessary.h"
int is_c_file(const char *filename);
bool check_files(char *file_name);
void send_data(char *file_name, int sd);
void rcv_data(char *file_name, int nsd, bool state);