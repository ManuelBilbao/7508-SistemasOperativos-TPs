#include "defs.h"
#include "types.h"
#include "readline.h"
#include "runcmd.h"
#include "history.h"

char promt[PRMTLEN] = { 0 };

// runs a shell command
static void
run_shell()
{
	char *cmd;

	while ((cmd = read_line(promt)) != NULL)
		if (run_cmd(cmd) == EXIT_SHELL)
			return;
}

// initializes the shell
// with the "HOME" directory
static void
init_shell()
{
	char buf[BUFLEN] = { 0 };
	char *home = getenv("HOME");

	if (chdir(home) < 0) {
		snprintf(buf, sizeof buf, "cannot cd to %s ", home);
		perror(buf);
	} else {
		snprintf(promt, sizeof promt, "(%s)", home);
	}

	if (!load_history())
		fprintf(stderr, "No se pudo cargar el historial de comandos\n");
}

int
main(void)
{
	init_shell();

	run_shell();

	return 0;
}
