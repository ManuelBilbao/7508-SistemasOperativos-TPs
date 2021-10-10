#include "history.h"

static char history_buffer[MAX_HISTORY_CMDS][BUFLEN];
int history_buffer_size;

FILE* open_history_file(char *mode);

FILE* open_history_file(char *mode) {
	char* hist_file_path = getenv("HISTFILE");
	if (hist_file_path == NULL) {
		hist_file_path = "/home/manu/.fisop_history";
	}

	return fopen(hist_file_path, mode);
}

bool load_history() {
	FILE* hist_file = open_history_file("r");

	if (hist_file == NULL) {
		perror("Error al abrir HISTFILE");
		return false;
	}

	char *s;
	while (history_buffer_size < MAX_HISTORY_CMDS &&
			(s = fgets(history_buffer[history_buffer_size], BUFLEN, hist_file)) != NULL) {
		if (s[strlen(s)-1] == '\n')
			s[strlen(s)-1] = '\0';
		history_buffer_size++;
	}

	fclose(hist_file);

	return true;
}

void record_command_in_history(char *cmd) {
	FILE* hist_file = open_history_file("a");

	if (hist_file == NULL) {
		perror("Error al abrir HISTFILE");
		return;
	}

	strcpy(history_buffer[history_buffer_size++], cmd);
	fprintf(hist_file, "%s\n", cmd);

	fclose(hist_file);
}

void print_history(int n) {
	if (n > history_buffer_size)
		n = history_buffer_size;

	for (int i = history_buffer_size - n; i < history_buffer_size; i++)
		printf("%4d  %s\n", i, history_buffer[i]);
}

char* reverse_history(int n) {
	if (n <= 0)
		return NULL;

	if (n > history_buffer_size)
		return NULL;

	return history_buffer[history_buffer_size-n];
}

int history_size() {
	return history_buffer_size;
}