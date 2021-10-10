#include "builtin.h"
#include <linux/limits.h>

extern int status;

// returns true if the 'exit' call
// should be performed
//
// (It must not be called from here)
int
exit_shell(char *cmd)
{
	return !strcmp(cmd, "exit") || !strcmp(cmd, "exit ");
}

// returns true if "chdir" was performed
//  this means that if 'cmd' contains:
// 	1. $ cd directory (change to 'directory')
// 	2. $ cd (change to $HOME)
//  it has to be executed and then return true
//
//  Remember to update the 'prompt' with the
//  	new directory.
//
// Examples:
//  1. cmd = ['c','d', ' ', '/', 'b', 'i', 'n', '\0']
//  2. cmd = ['c','d', '\0']
int
cd(char *cmd)
{
	if (strncmp(cmd, "cd", 2) != 0)
		return 0;

	if (!strcmp(cmd, "cd") || !strcmp(cmd, "cd ")) {
		char *home = getenv("HOME");

		if (chdir(home) == 0) {
			status = 0;
			snprintf(promt, sizeof promt, "(%s)", home);
		} else {
			status = 1;
		}

		return true;
	} else if (strlen(cmd) > 3 && cmd[2] == ' ') {
		if (chdir(cmd + 3) == 0) {
			char *new_dir = getcwd(new_dir, sizeof(new_dir));
			if (new_dir == NULL) {
				perror("Error al obtener el nuevo directorio");
				exit(-1);
			}
			snprintf(promt, sizeof(promt), "(%s)", new_dir);
			status = 0;
		} else {
			perror("Error al hacer cd");
			status = 1;
		}

		return true;
	}

	return 0;
}

// returns true if 'pwd' was invoked
// in the command line
//
// (It has to be executed here and then
// 	return true)
int
pwd(char *cmd)
{
	if (strcmp(cmd, "pwd") != 0 || strcmp(cmd, "pwd ") != 0)
		return 0;

	char *cwd = getcwd(cwd, sizeof(cwd));
	if (cwd == NULL) {
		status = errno;
		return true;
	}
	fprintf(stdout, "%s\n", cwd);
	status = 0;
	return true;
}
