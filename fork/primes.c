#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>

#define READ_INDEX 0
#define WRITE_INDEX 1

#define error(msg) \
	do { \
		perror(msg); \
		exit(-1); \
	} while (0);

int
main(int argc, char *argv[])
{
	int max = 10;
	if (argc == 2) {
		max = atoi(argv[1]);
	}

	int fd_izq[2], fd_der[2];
	if (pipe(fd_izq) < 0)
		error("Error al crear un pipe");

	int i_fork = fork();
	if (i_fork < 0)
		error("Error al crear un nuevo proceso");

	if (i_fork > 0) {  // Primero
		// El primero se comunica a su "derecha"
		fd_der[0] = fd_izq[0];
		fd_der[1] = fd_izq[1];
		close(fd_der[READ_INDEX]);

		for (int i = 2; i <= max; i++) {
			// Se podria optimizar y que el primero ya sea un filtro de 2
			if (write(fd_der[WRITE_INDEX], &i, sizeof(i)) < 0)
				error("Error al enviar un numero al segundo "
				      "proceso");
		}

		close(fd_der[WRITE_INDEX]);

		if (wait(NULL) < 0)
			error("Error al hacer wait");
	} else {  // Siguientes
		close(fd_izq[WRITE_INDEX]);

		ssize_t leidos;
		int numero_leido;
		int filtro;
		bool primero = true;

		while ((leidos = read(fd_izq[READ_INDEX],
		                      &numero_leido,
		                      sizeof(numero_leido))) > 0) {
			if (!primero) {
				// Mando los números al hijo
				if (numero_leido % filtro != 0) {
					if (write(fd_der[WRITE_INDEX],
					          &numero_leido,
					          sizeof(numero_leido)) < 0)
						error("Error al enviar un "
						      "número al siguiente "
						      "proceso");
				}
				continue;
			}

			// Solo la primera vez del proceso:

			filtro = numero_leido;
			printf("primo %i\n", numero_leido);

			// Creo un pipe y un proceso a la "derecha"
			if (pipe(fd_der) < 0)
				error("Error al crear un pipe");

			int i = fork();
			if (i < 0)
				error("Error al crear un nuevo proceso");

			if (i == 0) {  // Hijo
				// Cierro la lectura del pipe que hereda con el abuelo
				close(fd_izq[READ_INDEX]);

				// El pipe "derecho" del padre es el "izquierdo" del hijo
				fd_izq[0] = fd_der[0];
				fd_izq[1] = fd_der[1];

				close(fd_izq[WRITE_INDEX]);  // Cierro la escritura del pipe "izquierdo" en el hijo
			} else {
				primero = false;
				close(fd_der[READ_INDEX]);  // Cierro la lectura del pipe "derecho" en el padre
			}
		}

		if (leidos < 0)
			error("Error al leer del pipe");

		// Cierro la lectura del pipe "izquierdo" y la escritura del "derecho"
		close(fd_izq[READ_INDEX]);
		close(fd_der[WRITE_INDEX]);

		// Si fd_izq != fd_der => se creó un proceso hijo al cual hay que esperar
		if (fd_izq[READ_INDEX] != fd_der[READ_INDEX] && wait(NULL) < 0)
			error("Error al hacer wait");
	}

	return 0;
}