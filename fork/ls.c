#define _GNU_SOURCE
#define _POSIX_C_SOURCE >= 200809L
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <pwd.h>
#include <grp.h>
#include <linux/limits.h>
#include <time.h>

#define error(msg) \
	do { \
		perror(msg); \
		exit(-1); \
	} while (0);

const char chars[] = "rwxrwxrwx";

bool permisos(mode_t modo, char buf[10]);

int
main(int argc, char *argv[])
{
	DIR *dir;
	if (argc == 1)
		dir = opendir(".");
	else if (argc == 2)
		dir = opendir(argv[1]);
	else
		error("El número de arg es inválido");

	if (dir == NULL)
		error("No se pudo abrir el directorio");

	int dir_fd = dirfd(dir);
	if (dir_fd < 0)
		error("Error al obtener el File Descriptor del directorio");

	struct dirent *ent;
	while ((ent = readdir(dir)) != NULL) {
		struct stat statbuf;
		if (fstatat(dir_fd, ent->d_name, &statbuf, 0) < 0)
			error("Error al leer información del archivo");

		// Tipo y permisos
		char tipo = '-';
		if (ent->d_type == DT_DIR)
			tipo = 'd';
		else if (ent->d_type == DT_LNK)
			tipo = 'l';
		char permisos_legible[10];
		bool exec = permisos(statbuf.st_mode, permisos_legible);
		printf("%c%s %lu ", tipo, permisos_legible, statbuf.st_nlink);

		// Usuario y grupo
		printf("%s %s ",
		       getpwuid(statbuf.st_uid)->pw_name,
		       getgrgid(statbuf.st_gid)->gr_name);

		// Tamaño
		printf("%7lu ", statbuf.st_size);

		// Fecha de modificación
		struct tm localtime;
		char modificacion[13];
		localtime_r(&statbuf.st_mtim.tv_sec, &localtime);
		strftime(modificacion,
		         sizeof(modificacion),
		         "%b %d %H:%M",
		         &localtime);
		printf("%s ", modificacion);

		// Nombre
		if (ent->d_type == DT_DIR)
			printf("\e[1m\e[94m");  // Bold blue
		else if (ent->d_type == DT_LNK)
			printf("\e[1m\e[96m");  // Bold cyan
		else if (exec)
			printf("\e[1m\e[92m");  // Bold green

		printf("%s\e[0m", ent->d_name);

		// Link o sufijo
		if (ent->d_type == DT_LNK) {
			char link[PATH_MAX];

			ssize_t link_size = readlinkat(
			        dir_fd, ent->d_name, link, sizeof(link));
			if (link_size < 0)
				error("Error al leer el enlace");

			link[link_size] = '\0';
			printf(" -> %s", link);
		} else if (ent->d_type == DT_DIR) {
			printf("/");
		} else if (exec) {
			printf("*");
		}
		printf("\n");
	}

	closedir(dir);
}

bool
permisos(mode_t modo, char buf[10])
{
	bool exec = false;
	for (int i = 0; i < 9; i++) {
		buf[i] = (modo & (1 << (8 - i))) ? chars[i] : '-';
		exec |= (buf[i] == 'x');
	}
	buf[9] = '\0';

	return exec;
}