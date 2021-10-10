#include "exec.h"

// sets "key" with the key part of "arg"
// and null-terminates it
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  key = "KEY"
//
static void
get_environ_key(char *arg, char *key)
{
	int i;
	for (i = 0; arg[i] != '='; i++)
		key[i] = arg[i];

	key[i] = END_STRING;
}

// sets "value" with the value part of "arg"
// and null-terminates it
// "idx" should be the index in "arg" where "=" char
// resides
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  value = "value"
//
static void
get_environ_value(char *arg, char *value, int idx)
{
	size_t i, j;
	for (i = (idx + 1), j = 0; i < strlen(arg); i++, j++)
		value[j] = arg[i];

	value[j] = END_STRING;
}

// sets the environment variables received
// in the command line
//
// Hints:
// - use 'block_contains()' to
// 	get the index where the '=' is
// - 'get_environ_*()' can be useful here
static void
set_environ_vars(char **eargv, int eargc)
{
	for (int i = 0; i < eargc; i++) {
		char key[ARGSIZE];
		char value[ARGSIZE];

		get_environ_key(eargv[i], key);
		get_environ_value(eargv[i], value, block_contains(eargv[i], '='));

		if (setenv(key, value, true) < 0) {
			perror("Error al establecer la variable de entorno");
			_exit(-1);
		}
	}
}

// opens the file in which the stdin/stdout/stderr
// flow will be redirected, and returns
// the file descriptor
//
// Find out what permissions it needs.
// Does it have to be closed after the execve(2) call?
//
// Hints:
// - if O_CREAT is used, add S_IWUSR and S_IRUSR
// 	to make it a readable normal file
static int
open_redir_fd(char *file, int flags)
{
	if (file[0] == '&')
		return atoi(file + 1);

	int file_fd;
	int mode = 0;
	if ((flags | O_CREAT) == flags) {
		flags |= O_TRUNC;  // Para achicar el archivo si ya existia y es mas grande
		mode = S_IWUSR | S_IRUSR;
	}
	file_fd = open(file, flags | O_CLOEXEC, mode);

	return file_fd;
}

// executes a command - does not return
//
// Hint:
// - check how the 'cmd' structs are defined
// 	in types.h
// - casting could be a good option
void
exec_cmd(struct cmd *cmd)
{
	// To be used in the different cases
	struct execcmd *e;
	struct backcmd *b;
	struct execcmd *r;
	struct pipecmd *p;

	switch (cmd->type) {
	case EXEC:
		// spawns a command
		e = (struct execcmd *) cmd;

		set_environ_vars(e->eargv, e->eargc);

		if (execvp(e->argv[0], e->argv) < 0) {
			perror("[EXEC] Error en exec");
			_exit(-1);
		}
		break;

	case BACK: {
		// runs a command in background
		b = (struct backcmd *) cmd;
		exec_cmd(b->c);
		break;
	}

	case REDIR: {
		// changes the input/output/stderr flow
		//
		// To check if a redirection has to be performed
		// verify if file name's length (in the execcmd struct)
		// is greater than zero
		r = (struct execcmd *) cmd;
		if (strlen(r->out_file) > 0) {
			int file_fd =
			        open_redir_fd(r->out_file, O_WRONLY | O_CREAT);
			dup2(file_fd, 1);
		}

		if (strlen(r->in_file) > 0) {
			int file_fd = open_redir_fd(r->in_file, O_RDONLY);
			dup2(file_fd, 0);
		}

		if (strlen(r->err_file) > 0) {
			int file_fd =
			        open_redir_fd(r->err_file, O_WRONLY | O_CREAT);
			dup2(file_fd, 2);
		}

		cmd->type = EXEC;
		exec_cmd(cmd);
		break;
	}

	case PIPE: {
		// pipes two commands
		p = (struct pipecmd *) cmd;

		int fd_pipe[2];
		if (pipe(fd_pipe)) {
			perror("Error al crear el pipe");
			_exit(-1);
		}

		int i1 = fork();
		if (i1 < 0) {
			perror("Error al crear un proceso (L)");
			_exit(-1);
		}

		if (i1 == 0) {  // Left
			close(fd_pipe[READ]);

			dup2(fd_pipe[WRITE], 1);
			close(fd_pipe[WRITE]);

			exec_cmd(p->leftcmd);
		}

		int i2 = fork();
		if (i2 < 0) {
			perror("Error al crear un proceso (R)");
			_exit(-1);
		}

		if (i2 == 0) {  // Right
			close(fd_pipe[WRITE]);

			dup2(fd_pipe[READ], 0);
			close(fd_pipe[READ]);

			exec_cmd(p->rightcmd);
		}

		// Coordinador
		close(fd_pipe[0]);
		close(fd_pipe[1]);

		int exit_status = 0;
		int right_status = 0;
		if (waitpid(i1, NULL, 0) < 0) {
			perror("Error al hacer wait");
			_exit(-1);
		}
		if (waitpid(i2, &right_status, 0) < 0) {
			perror("Error al hacer wait");
			_exit(-1);
		}

		free_command(parsed_pipe);

		if (WIFEXITED(right_status))
			exit_status = WEXITSTATUS(right_status);

		exit(exit_status);
		break;
	}
	}
}