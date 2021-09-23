#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define READ_INDEX 0
#define WRITE_INDEX 1

#define error(msg) \
	do { \
		perror(msg); \
		exit(-1); \
	} while (0);

int
main(void)
{
	srand(time(NULL));
	int fdPadreHijo[2];  // Padre --> Hijo
	int fdHijoPadre[2];  // Hijo --> Padre

	printf("Hola, soy PID %d:\n", getpid());

	if (pipe(fdPadreHijo) < 0)
		error("Error al abrir un pipe");

	if (pipe(fdHijoPadre) < 0)
		error("Error al abrir un pipe");

	printf("  - primer pipe me devuelve: [%i, %i]\n",
	       fdPadreHijo[0],
	       fdPadreHijo[1]);
	printf("  - segundo pipe me devuelve: [%i, %i]\n\n",
	       fdHijoPadre[0],
	       fdHijoPadre[1]);

	int i_fork = fork();

	if (i_fork < 0)
		error("Error al crear un nuevo proceso");

	if (i_fork == 0) {  // Hijo
		// Cierro la escritura del pipe Padre-->Hijo y la lecutra del pipe Hijo-->Padre
		close(fdPadreHijo[WRITE_INDEX]);
		close(fdHijoPadre[READ_INDEX]);

		long int lectura;
		if (read(fdPadreHijo[READ_INDEX], &lectura, sizeof(lectura)) < 0)
			error("[Hijo] Error al leer del pipe");

		printf("Donde fork me devuelve 0:\n");
		printf("  - getpid me devuelve: %i\n", getpid());
		printf("  - getppid me devuelve: %i\n", getppid());
		printf("  - recibo valor %li vía fd=%i\n",
		       lectura,
		       fdPadreHijo[READ_INDEX]);
		printf("  - reenvío valor en fd=%i y termino\n\n",
		       fdHijoPadre[WRITE_INDEX]);

		if (write(fdHijoPadre[WRITE_INDEX], &lectura, sizeof(lectura)) < 0)
			error("[Hijo] Error al escribir en el pipe");

		close(fdPadreHijo[READ_INDEX]);
		close(fdHijoPadre[WRITE_INDEX]);
	} else {  // Padre
		// Cierro la lectura del pipe Padre-->Hijo y la escritura del pipe Hijo-->Padre
		close(fdPadreHijo[READ_INDEX]);
		close(fdHijoPadre[WRITE_INDEX]);

		long int randNum = random();
		long int lectura;

		printf("Donde fork me devuelve %i:\n", i_fork);
		printf("  - getpid me devuelve: %i\n", getpid());
		printf("  - getppid me devuelve: %i\n", getppid());
		printf("  - random me devuelve: %li\n", randNum);
		printf("  - envío valor %li a través de fd=%i\n\n",
		       randNum,
		       fdPadreHijo[WRITE_INDEX]);

		if (write(fdPadreHijo[WRITE_INDEX], &randNum, sizeof(randNum)) < 0)
			error("[Padre] Error al escribir en el pipe");

		if (read(fdHijoPadre[READ_INDEX], &lectura, sizeof(lectura)) < 0)
			error("[Padre] Error al leer del pipe");

		printf("Hola, de nuevo PID %i:\n", getpid());
		printf("  - recibí valor %li vía fd=%i\n",
		       lectura,
		       fdHijoPadre[READ_INDEX]);

		close(fdPadreHijo[WRITE_INDEX]);
		close(fdHijoPadre[READ_INDEX]);
	}

	return 0;
}