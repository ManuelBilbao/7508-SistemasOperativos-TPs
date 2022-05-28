#ifndef BUILTIN_H
#define BUILTIN_H

#include "defs.h"
#include "history.h"

extern char promt[PRMTLEN];

int cd(char *cmd);

int exit_shell(char *cmd);

int pwd(char *cmd);

int history(char *cmd);

#endif  // BUILTIN_H
