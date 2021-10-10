#include "defs.h"
#include "readline.h"
#include "history.h"

#define BACKSPACE 127
#define ESCAPE '\033'
#define HOME_ESCAPE 'H'
#define END_ESCAPE 'F'
#define UP_ARROW_ESCAPE 'A'
#define DOWN_ARROW_ESCAPE 'B'
#define RIGHT_ARROW_ESCAPE 'C'
#define LEFT_ARROW_ESCAPE 'D'

#define error(msg)                                                             \
	do {                                                                   \
		perror(msg);                                                   \
		exit(-1);                                                      \
	} while (0);

const char CTRL_D = '\004';
const char PROMPT_SYMBOL[] = "$ ";
const char DELETE_STRING[] = "\b \b";
const char MOVE_LEFT_STRING[] = "\b";
const char MOVE_RIGHT_STRING[] = "\033[C";

static char buffer[BUFLEN];
struct termios saved_attributes;

void print_prompt_symbol(void);
void print_prompt(const char *prompt);

void reset_input_mode(void);
void set_input_mode(void);

void reset_buffer(int *current, int *last);
void clear_stdout_line(int current, int last);
void print_buffer(int size, int current);

void move_cursor_horizontal(char dir, int n);
void shift_left_buffer(int from);
void shift_right_buffer(int from);
void delete_characters(int n);

bool handle_escape(char c, int *current, int *last, int *hist);

void
print_prompt_symbol()
{
#ifndef SHELL_NO_INTERACTIVE
	if (write(STDOUT_FILENO, PROMPT_SYMBOL, strlen(PROMPT_SYMBOL)) < 0)
		error("Error al imprimir el prompt");
#endif
}

void
print_prompt(const char *prompt)
{
#ifndef SHELL_NO_INTERACTIVE
	fprintf(stdout, "%s %s %s\n", COLOR_RED, prompt, COLOR_RESET);
#endif
	print_prompt_symbol();
}

void
reset_input_mode()
{
	tcsetattr(STDIN_FILENO, TCSANOW, &saved_attributes);
}

void
set_input_mode()
{
	struct termios tattr;

	/* Make sure stdin is a terminal. */
	if (!isatty(STDIN_FILENO))
		error("No es una terminal");

	/* Save the terminal attributes so we can restore them later. */
	tcgetattr(STDIN_FILENO, &saved_attributes);
	atexit(reset_input_mode);

	/* Set the funny terminal modes. */
	tcgetattr(STDIN_FILENO, &tattr);
	tattr.c_lflag &= ~(ICANON | ECHO); /* Clear ICANON and ECHO. */
	tattr.c_cc[VMIN] = 1;
	tattr.c_cc[VTIME] = 0;
	tcsetattr(STDIN_FILENO, TCSANOW, &tattr);
}

void
reset_buffer(int *current, int *last)
{
	memset(buffer, 0, BUFLEN);
	clear_stdout_line(*current, *last);

	*current = 0;
	*last = 0;
}

void
clear_stdout_line(int current, int last)
{
	move_cursor_horizontal('r', last - current);
	delete_characters(last + strlen(PROMPT_SYMBOL));
}

void
print_buffer(int size, int current)
{
	print_prompt_symbol();
	for (int i = 0; i < size; i++)
		if (write(STDOUT_FILENO, &buffer[i], sizeof(buffer[i])) < 0)
			error("Error al escribir el buffer en la terminal");

	move_cursor_horizontal('l', size - current);
}

void
move_cursor_horizontal(char dir, int n)
{
	const char *code = (dir == 'l') ? MOVE_LEFT_STRING : MOVE_RIGHT_STRING;

	for (int i = 0; i < n; i++)
		if (write(STDOUT_FILENO, code, strlen(code)) < 0)
			error("Error al mover el cursor");
}

void
delete_characters(int n)
{
	for (int i = 0; i < n; i++)
		if (write(STDOUT_FILENO, DELETE_STRING, strlen(DELETE_STRING)) < 0)
			error("Error al borrar un caracter");
}

void
shift_left_buffer(int from)
{
	strcpy(&buffer[from], &buffer[from + 1]);
}

void
shift_right_buffer(int from)
{
	strcpy(&buffer[from + 1], &buffer[from]);
}

bool
handle_escape(char c, int *current, int *last, int *hist)
{
	switch (c) {
	case '[':
		return true;
	case HOME_ESCAPE:
		move_cursor_horizontal('l', *current);
		*current = 0;

		break;
	case END_ESCAPE:
		move_cursor_horizontal('r', *last - *current);
		*current = *last;

		break;
	case LEFT_ARROW_ESCAPE:
		if (*current <= 0)
			break;

		move_cursor_horizontal('l', 1);
		(*current)--;

		break;
	case RIGHT_ARROW_ESCAPE:
		if (*current >= *last)
			break;

		move_cursor_horizontal('r', 1);
		(*current)++;

		break;
	case UP_ARROW_ESCAPE:
	case DOWN_ARROW_ESCAPE:
		reset_buffer(current, last);

		if (c == UP_ARROW_ESCAPE)
			*hist += (*hist == history_size()) ? 0 : 1;
		else
			*hist -= (*hist < 1) ? 0 : 1;

		char *hist_line = reverse_history(*hist);
		if (hist_line != NULL) {
			strcpy(buffer, hist_line);
			*last = strlen(buffer);
			*current = strlen(buffer);
		}

		print_buffer(*last, *current);
	}

	return false;
}

// reads a line from the standard input
// and prints the prompt
char *
read_line(const char *promt)
{
	char c;
	int last = 0, current = 0, hist_i = 0;
	bool escape = false;

	print_prompt(promt);

	memset(buffer, 0, BUFLEN);

	set_input_mode();

	c = getchar();

	while (c != END_LINE && c != EOF && c != CTRL_D) {
		switch (c) {
		case ESCAPE:  // Escape
			escape = true;
			break;
		case BACKSPACE:  // Backspace (borrar)
			if (current <= 0)
				break;

			clear_stdout_line(current, last);

			// Shift all characters 1 place left
			strcpy(&buffer[current], &buffer[current + 1]);
			buffer[last] = END_STRING;

			current--;
			last--;

			print_buffer(last, current);

			break;
		default:
			if (escape) {
				escape = handle_escape(c, &current, &last, &hist_i);
				break;
			}

			// Shift all characters 1 place right
			strcpy(&buffer[current + 1], &buffer[current]);
			buffer[current++] = c;
			last++;

			clear_stdout_line(current, last);
			print_buffer(last, current);

			break;
		}
		c = getchar();
	}

	reset_input_mode();

	// if the user press ctrl+D
	// just exit normally
	if (c == EOF || c == CTRL_D)
		return NULL;

	if (write(STDOUT_FILENO, &c, sizeof c) < 0)
		error("Error al intentar escribir en la terminal");

	buffer[last] = END_STRING;

	return buffer;
}
