/*
* CS252: Systems Programming
* Purdue University
* Example that shows how to read one line with simple editing
* using raw terminal.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "tty-raw-mode.c"
#define MAX_BUFFER_LINE 2048

//extern void tty_raw_mode(void);

// Buffer where line is stored
int line_length;
char line_buffer[MAX_BUFFER_LINE];

// Simple history array
// This history does not change. 
// Yours have to be updated.
int history_index = 0;
char *history[MAX_BUFFER_LINE];
int history_length = 0;

void read_line_print_usage()
{
	char * usage = "\n"
		" ctrl-?       Print usage\n"
		" Backspace    Deletes last character\n"
		" up arrow     See last command in the history\n";

	write(1, usage, strlen(usage));
}

/*
* Input a line with some basic editing.
*/
char * read_line() {

	// Set terminal in raw mode
	tty_raw_mode();

	line_length = 0;
	int csr = line_length;

	// Read one line until enter is typed
	while (1) {

		// Read one character in raw mode.
		char ch;
		read(0, &ch, 1);

		if (ch >= 32) {
			// It is a printable character. 
			if (ch == 127) {//backspace
				if (csr > 0 && line_length > 0) {
					for (int i = csr - 1; i < line_length - 1; ++i) {
						line_buffer[i] = line_buffer[i + 1];
					}
					line_buffer[line_length - 1] = '\0';
					ch = 8;
					for (int i = csr; i > 0; --i) write(1, &ch, 1);
					ch = ' ';
					for (int i = 0; i < line_length; ++i) write(1, &ch, 1);
					ch = 8;
					for (int i = line_length; i > 0; --i) write(1, &ch, 1);
					for (int i = 0; i < line_length - 1; ++i) write(1, &line_buffer[i], 1);
					ch = 8;
					for (int i = 0; i < (line_length - csr); ++i) write(1, &ch, 1);
					--line_length;
					--csr;
				}
				continue;
			}
			// Do echo
			if (csr == line_length) {
				write(1, &ch, 1);
				// If max number of character reached return.
				if (line_length == MAX_BUFFER_LINE - 2)
					break;
				// add char to buffer.
				line_buffer[line_length] = ch;
				++line_length;
				++csr;
			}
			else {
				if (line_length == MAX_BUFFER_LINE - 2)
					break;
				for (int i = line_length; i >= csr; --i) {
					line_buffer[i] = line_buffer[i - 1];
				}
				line_buffer[line_length + 1] = '\0';
				line_buffer[csr] = ch;
				write(1, &ch, 1);
				int tmp = csr;
				for (int i = csr+1; i < line_length+1; ++i) {
					write(1, &line_buffer[i], 1);
					++csr;
				}
				ch = 8;
				for (int i = tmp; i < line_length; ++i) {
					write(1, &ch, 1);
					--csr;
				}
				
				++line_length;
				++csr;
			}

		}
		else if (ch == 10) {
			// <Enter> was typed. Return line

			// Print newline
			write(1, &ch, 1);
			//csr = line_length = 0;
			break;
		}
		else if (ch == 31) {
			// ctrl-?
			read_line_print_usage();
			line_buffer[0] = 0;
			break;
		}
		else if (ch == 8) {
			// <backspace> was typed. Remove previous character read.
			if (csr > 0 && line_length > 0) {
				for (int i = csr - 1; i < line_length - 1; ++i) {
					line_buffer[i] = line_buffer[i + 1];
				}
				line_buffer[line_length - 1] = '\0';
				ch = 8;
				for (int i = csr; i > 0; --i) write(1, &ch, 1);
				ch = ' ';
				for (int i = 0; i < line_length; ++i) write(1, &ch, 1);
				ch = 8;
				for (int i = line_length; i > 0; --i) write(1, &ch, 1);
				for (int i = 0; i < line_length - 1; ++i) write(1, &line_buffer[i], 1);
				ch = 8;
				for (int i = 0; i < (line_length - csr); ++i) write(1, &ch, 1);
				--line_length;
				--csr;
			}
		}
		else if (ch == 4) {//ctrl+d: delete
			if (csr == line_length) continue;
			for (int i = csr; i < line_length - 1; ++i) {
				line_buffer[i] = line_buffer[i + 1];
			}
			line_buffer[line_length - 1] = '\0';
			ch = 8;
			for (int i = csr; i > 0; --i) write(1, &ch, 1);
			ch = ' ';
			for (int i = 0; i < line_length; ++i) write(1, &ch, 1);
			ch = 8;
			for (int i = line_length; i > 0; --i) write(1, &ch, 1);
			for (int i = 0; i < line_length - 1; ++i) write(1, &line_buffer[i], 1);
			ch = 8;
			for (int i = 0; i < line_length - csr - 1; ++i) write(1, &ch, 1);
			--line_length;
		}
		else if (ch == 1) {//home_1
			while (csr > 0) {
				ch = 27;
				write(1, &ch, 1);
				ch = 91;
				write(1, &ch, 1);
				ch = 68;
				write(1, &ch, 1);
				--csr;
			}
		}
		else if (ch == 5) {//end_1
			while (csr < line_length) {
				ch = 27;
				write(1, &ch, 1);
				ch = 91;
				write(1, &ch, 1);
				ch = 67;
				write(1, &ch, 1);
				++csr;
			}
		}
		else if (ch == 27) {
			// Escape sequence. Read two chars more
			//
			// HINT: Use the program "keyboard-example" to
			// see the ascii code for the different chars typed.
			//
			char ch1;
			char ch2;
			read(0, &ch1, 1);
			read(0, &ch2, 1);
			if (ch1 == 91 && ch2 == 65) {//up
										 // Up arrow. Print next line in history.
										 // Erase old line
										 // Print backspaces
				if (history_index < 0) continue;
				int i = 0;
				for (i = 0; i < line_length; i++) {
					ch = 8;
					write(1, &ch, 1);
				}

				// Print spaces on top
				for (i = 0; i < line_length; i++) {
					ch = ' ';
					write(1, &ch, 1);
				}

				// Print backspaces
				for (i = 0; i < line_length; i++) {
					ch = 8;
					write(1, &ch, 1);
				}

				// Copy line from history
				if (history_length > 0 && history_index >= 0) {
					strcpy(line_buffer, history[history_index]);
					--history_index;
					line_length = strlen(line_buffer);
					write(1, line_buffer, line_length);
					csr = line_length;
				}
			}
			else if (ch1 == 91 && ch2 == 66) {//down
				if (history_index == history_length) continue;
				int i = 0;
				for (i = 0; i < line_length; i++) {
					ch = 8;
					write(1, &ch, 1);
				}
				for (i = 0; i < line_length; i++) {
					ch = ' ';
					write(1, &ch, 1);
				}
				for (i = 0; i < line_length; i++) {
					ch = 8;
					write(1, &ch, 1);
				}
				if (history_length >= -1 && history_index < history_length) {
					if (history_index == history_length - 1) {
						strcpy(line_buffer, "");
						line_length = strlen(line_buffer);
						write(1, line_buffer, line_length);
						csr = line_length;
					}
					else {
						strcpy(line_buffer, history[history_index + 1]);
						++history_index;
						line_length = strlen(line_buffer);
						write(1, line_buffer, line_length);
						csr = line_length;
					}
				}
			}
			else if (ch1 == 91 && ch2 == 68) {//left
				if (csr > 0) {
					ch = 8;
					write(1, &ch, 1);
					--csr;
				}
			}
			else if (ch1 == 91 && ch2 == 67) {//right
				if (csr < line_length) {
					ch = 27;
					write(1, &ch, 1);
					ch = 91;
					write(1, &ch, 1);
					ch = 67;
					write(1, &ch, 1);
					++csr;
				}
			}
			else if (ch1 == 91 && ch2 == 72) {//home_2
				while (csr > 0) {
					ch = 27;
					write(1, &ch, 1);
					ch = 91;
					write(1, &ch, 1);
					ch = 68;
					write(1, &ch, 1);
					--csr;
				}
			}
			else if (ch1 == 91 && ch2 == 70) {//end_2
				while (csr != line_length) {
					ch = 27;
					write(1, &ch, 1);
					ch = 91;
					write(1, &ch, 1);
					ch = 67;
					write(1, &ch, 1);
					++csr;
				}
			}
		}
	}

	// Add eol and null char at the end of string
	line_buffer[line_length] = 10;
	line_length++;
	line_buffer[line_length] = 0;

	history[history_length] = (char *)malloc(strlen(line_buffer) + 1);
	strcpy(history[history_length], line_buffer);
	history[history_length][strlen(line_buffer) - 1] = '\0';
	history_index = history_length;
	history_length++;
	return line_buffer;
}
