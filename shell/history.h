#ifndef HISTORY_H
#define HISTORY_H

#include "defs.h"
#include "utils.h"

#define MAX_HISTORY_CMDS 1024

void record_command_in_history(char *cmd);

void print_history(int n);

bool load_history(void);

char* reverse_history(int n);

int history_size(void);

#endif