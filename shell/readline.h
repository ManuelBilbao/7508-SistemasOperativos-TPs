#ifndef READLINE_H
#define READLINE_H

#include <termios.h>
#include <unistd.h>
#include <stdbool.h>

char *read_line(const char *promt);

#endif  // READLINE_H
