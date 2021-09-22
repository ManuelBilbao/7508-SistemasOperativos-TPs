#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <string.h>
#include <linux/limits.h>

#define error(msg) \
	do { \
		perror(msg); \
		exit(-1); \
	} while (0);

void read_dir(DIR *dir, char path[PATH_MAX], char *busqueda, char *(*cmp_func)(char *, char *));

int
main(int argc, char *argv[])
{
	DIR *dir_actual = opendir(".");
	if (dir_actual == NULL)
		error("No se pudo abrir el directorio");

	char *busqueda = "";
	char *(*cmp_func)(char *, char *) = &strstr;
	if (argc == 2) {
		busqueda = argv[1];
	} else if (argc == 3 && !strcmp(argv[1], "-i")) {
		cmp_func = &strcasestr;
		busqueda = argv[2];
	} else {
		error("Error en el formato o cantidad de los argumentos");
	}

	read_dir(dir_actual, "", busqueda, cmp_func);

	return 0;
}

void read_dir(DIR *dir, char path[PATH_MAX], char *busqueda, char *(*cmp_func)(char *, char *)) {
	struct dirent *ent;
	while ((ent = readdir(dir)) != NULL) {
		// Ignoro los alias . y ..
		if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, ".."))
			continue;

		if ((*cmp_func)(ent->d_name, busqueda) != NULL)
			printf("%s%s\n", path, ent->d_name);
		
		if (ent->d_type == DT_DIR) {
			char path_nuevo[PATH_MAX];
			strcpy(path_nuevo, path);
			strcat(path_nuevo, ent->d_name);
			strcat(path_nuevo, "/");

			int dir_fd = dirfd(dir);
			if (dir_fd < 0)
				error("Error al obtener el File Descriptor del directorio");

			int subdir_fd = openat(dir_fd, ent->d_name, O_DIRECTORY);
			if (subdir_fd < 0)
				error("Error al abrir un directorio");

			DIR *subdir = fdopendir(subdir_fd);
			if (subdir == NULL)
				error("Error al obtener la estructura del directorio");

			read_dir(subdir, path_nuevo, busqueda, cmp_func);
		}
	}
	closedir(dir);
}