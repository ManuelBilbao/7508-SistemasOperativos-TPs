/*
Linkear con -lrt (i.e. gcc timeout.c -o timeout -lrt)
*/
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

#define error(msg) \
	do { \
		perror(msg); \
		exit(-1); \
	} while (0);

void empezar_timer(int segundos);

int
main(int argc, char *argv[])
{
	if (argc < 3)
		error("La cantidad de parámetros es inválida");

	int i_fork = fork();
	if (i_fork < 0)
		error("Error al crear un nuevo proceso");

	if (i_fork == 0) {  // Hijo
		if (execvp(argv[2], argv + 2) < 0)
			error("Error al lanzar exec");
	}

	siginfo_t info;
	sigset_t signal_set;
	sigemptyset(&signal_set);
	sigaddset(&signal_set, SIGCHLD);
	sigaddset(&signal_set, SIGALRM);
	sigprocmask(SIG_BLOCK, &signal_set, NULL);

	empezar_timer(atoi(argv[1]));
	if (sigwaitinfo(&signal_set, &info) < 0)
		error("Error al hacer wait");

	/*
	        Existe una syscall muy interesante, sigtimedwait(2), que trabaja
	        de la misma forma que sigwaitinfo(2) pero añade un parámetro timeout
	        el cual establece un intervalo de tiempo máximo para esperar una
	        señal. Con esta syscall se podría omitir la creación y seteo de un
	        timer manualmente, bastaría con enviarle un sigseg_t que contenga
	        únicamente SIGCHLD y el timespec requerido: sigtimedwait(&signal_set,
	        &info, (struct timespec) {atoi(argv[1]), 0});
	*/

	kill(i_fork, SIGTERM);

	return 0;
}

void
empezar_timer(int segundos)
{
	timer_t timerid;

	struct itimerspec duracion;
	duracion.it_value.tv_sec = segundos;
	duracion.it_value.tv_nsec = 0;
	duracion.it_interval.tv_sec = 0;
	duracion.it_interval.tv_nsec = 0;

	timer_create(CLOCK_REALTIME, NULL, &timerid);
	timer_settime(timerid, 0, &duracion, NULL);
}