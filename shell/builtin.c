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
	if (!strncmp(cmd, "cd", 2)) {
		if (!strcmp(cmd, "cd") || !strcmp(cmd, "cd ")) {
			char *home = getenv("HOME");

			if (chdir(getenv("HOME")) < 0) {
				status = 1;
			} else {
				snprintf(promt, sizeof promt, "(%s)", home);
				status = 0;
			}

			return true;
		} else if (strlen(cmd) > 3 && cmd[2] == ' ') {
			if (chdir(cmd + 3) == 0) {
				char *new_prompt =
				        getcwd(new_prompt, sizeof(new_prompt));
				if (new_prompt == NULL) {
					status = 1;
					return true;
				}
				snprintf(promt, sizeof(promt), "(%s)", new_prompt);
				status = 0;
			} else {
				perror("Error al hacer cd");
				status = 1;
			}

			return true;
		}
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
	if (!strncmp(cmd, "pwd", 3)) {
		char cwd[PATH_MAX];
		if (getcwd(cwd, sizeof(cwd)) == NULL) {
			status = errno;
			return true;
		}
		fprintf(stdout, "%s\n", cwd);
		status = 0;
		return true;
	}
	return 0;
}