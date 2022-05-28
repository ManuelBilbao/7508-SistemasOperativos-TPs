#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

#define error(msg) \
	do { \
		perror(msg); \
		exit(-1); \
	} while (0);

int
main(int argc, char const *argv[])
{
	if (argc != 3)
		error("La cantidad de parámetros es inválida");

	int src_fd = open(argv[1], O_RDONLY);
	if (src_fd < 0)
		error("Error al intentar abrir el archivo de origen");

	int dst_fd = open(argv[2],
	                  O_RDWR | O_CREAT,
	                  S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
	if (dst_fd < 0)
		error("Error al intentar abrir el archivo de destino");

	struct stat statbuf;
	if (fstat(src_fd, &statbuf) < 0)
		error("Error al obtener los datos del archivo");

	size_t file_size = statbuf.st_size;

	// Lleno el archivo nuevo para poder cargarlo con memcpy
	if (ftruncate(dst_fd, file_size) < 0)
		error("Error al redimensionar el archivo");

	char *src_addr;
	src_addr = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, src_fd, 0);
	if (src_addr == MAP_FAILED)
		error("Error en mmap src");

	char *dst_addr;
	dst_addr = mmap(NULL, file_size, PROT_WRITE, MAP_SHARED, dst_fd, 0);
	if (dst_addr == MAP_FAILED)
		error("Error en mmap dst");

	memcpy(dst_addr, src_addr, file_size);

	close(src_fd);
	close(dst_fd);

	return 0;
}