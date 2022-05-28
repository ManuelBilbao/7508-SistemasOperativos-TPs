#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#define error(msg) \
	do { \
		perror(msg); \
		exit(-1); \
	} while (0);

bool es_numero(char *nombre);
void mostrar_datos(int fd);

int
main()
{
	DIR *proc_dir = opendir("/proc");
	if (proc_dir == NULL)
		error("No se pudo abrir el directorio /proc");

	int proc_dir_fd = dirfd(proc_dir);
	if (proc_dir_fd < 0)
		perror("Error al obtener el File Descriptor del directorio");

	printf("  PID          VSZ STAT COMMAND\n");

	struct dirent *ent;
	while ((ent = readdir(proc_dir)) != NULL) {
		if (!es_numero(ent->d_name))
			continue;

		int proc_fd = openat(proc_dir_fd, ent->d_name, O_DIRECTORY);
		if (proc_fd < 0)
			perror("Error al abrir un directorio");

		mostrar_datos(proc_fd);
	}

	closedir(proc_dir);
}

bool
es_numero(char *nombre)
{
	bool all_digits = true;
	for (size_t i = 0; i < strlen(nombre); i++) {
		if (!isdigit(nombre[i]))
			all_digits = false;
	}

	return all_digits;
}

void
mostrar_datos(int fd)
{
	int pid;
	unsigned long vsize;
	char comm[256];
	char status;

	DIR *dir = fdopendir(fd);
	if (dir == NULL)
		error("Error al obtener el directorio");

	int stat_fd = openat(fd, "stat", O_RDONLY);
	FILE *stat_file = fdopen(stat_fd, "r");
	if (stat_file == NULL)
		perror("Error al abrir el archivo stat");

	if (fscanf(stat_file,
	       "%d (%[^)]) %c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %*u %*u "
	       "%*d %*d %*d %*d %*d %*d %*u %lu",
	       &pid,
	       comm,
	       &status,
	       &vsize) == EOF)
		error("Error al leer el archivo stat");

	printf("%5d %12lu  %c   %s\n", pid, vsize, status, comm);

	closedir(dir);
	fclose(stat_file);
}