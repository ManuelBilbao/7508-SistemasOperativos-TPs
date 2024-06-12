#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <string.h>

#ifndef NARGS
#define NARGS 4
#endif

#define error(msg) \
	do { \
		perror(msg); \
		exit(-1); \
	} while (0);

int
main(int argc, char *argv[])
{
	if (argc != 2)
		error("La cantidad de parámetros es inválida");

	char *lineas[NARGS + 2];
	size_t sizes[NARGS + 2];
	bool eof = false;

	for (int i = 0; i < NARGS + 2; i++) {
		lineas[i] = NULL;
		sizes[i] = 0;
	}

	lineas[0] = argv[1];

	while (!eof) {
		for (int i = 1; i < NARGS + 1; i++) {
			ssize_t leidos = getline(&lineas[i], &sizes[i], stdin);

			if (leidos <= 0) {
				free(lineas[i]);
				lineas[i] = NULL;
				eof = true;
				break;
			}

			if (lineas[i][leidos - 1] == '\n')
				lineas[i][leidos - 1] = '\0';

			lineas[i + 1] = NULL;
		}

		int i = fork();
		if (i < 0)
			error("Error al crear un nuevo proceso");

		if (i == 0) {  // Hijo
			if (execvp(argv[1], lineas) < 0)
				error("Error en exec");
		}

		if (wait(NULL) < 0)
			error("Error en el wait");

		for (int i = 1; i < NARGS + 1; i++) {
			free(lineas[i]);
			lineas[i] = NULL;
			sizes[i] = 0;
		}
	}

	return 0;
}