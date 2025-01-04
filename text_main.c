#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <string.h>

#define MAX_ROWS 51
#define MAX_COLS 100

// Buffer structure
typedef struct {
    char data[MAX_COLS];
    size_t length;
} Line;

typedef struct {
    Line lines[MAX_ROWS];
    size_t num_lines;
} Buffer;

Buffer buffer;

// Function to read bytes from a file and store them in a buffer
void read_bytes(char *buffer, const char *filename, long start_byte, long end_byte) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Error opening the file");
        exit(EXIT_FAILURE);
    }
    if (fseek(file, start_byte, SEEK_SET) != 0) {
        perror("Error seeking in file");
        fclose(file);
        exit(EXIT_FAILURE);
    }
    long bytes_to_read = end_byte - start_byte + 1;

    if (!buffer) {
        perror("Memory allocation failed");
        fclose(file);
        exit(EXIT_FAILURE);
    }
    size_t bytes_read = fread(buffer, 1, bytes_to_read, file);
    buffer[bytes_read] = '\0';
    fclose(file);
}

// Function to load the file content into the buffer and display it on the screen
void load_file_into_buffer(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        return;
    }

    buffer.num_lines = 0;
    while (fgets(buffer.lines[buffer.num_lines].data, MAX_COLS, file) && buffer.num_lines < MAX_ROWS) {
        size_t len = strlen(buffer.lines[buffer.num_lines].data);
        if (buffer.lines[buffer.num_lines].data[len - 1] == '\n') {
            buffer.lines[buffer.num_lines].data[len - 1] = '\0';
        }
        buffer.lines[buffer.num_lines].length = strlen(buffer.lines[buffer.num_lines].data);
        mvprintw(buffer.num_lines, 0, "%s", buffer.lines[buffer.num_lines].data);
        buffer.num_lines++;
    }

    fclose(file);
    refresh();
}

// Function to save the buffer content back to the file
void save_buffer_to_file(const char *filename) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        perror("Error opening file for saving");
        return;
    }

    for (size_t i = 0; i < buffer.num_lines; i++) {
        fprintf(file, "%s\n", buffer.lines[i].data);
    }

    fclose(file);
    mvprintw(MAX_ROWS + 1, 0, "File saved successfully.");
    refresh();
}

// Function to handle command prompt
void handle_command_prompt(int l_row, int l_col, int *cur_curse_y, int *cur_curse_x) {
    while (1) {
        char str[20];
        mvprintw(l_row - 1, 0, "command:");
        clrtoeol();
        echo();
        getstr(str);
        noecho();

        if (!strcmp(str, "q")) {
            endwin();
            exit(0);
        }
        if (!strcmp(str, "b")) {
            move(l_row - 1, 0);
            clrtoeol();
            move(*cur_curse_y, *cur_curse_x);
            break;
        } else {
            move(l_row - 1, 0);
            clrtoeol();
            refresh();
        }
    }
}

// Function to handle keypresses
// Function to handle keypresses
void handle_keypress(int ch, int l_row, int l_col, const char *filename) {
    int cur_curse_y, cur_curse_x;
    getyx(stdscr, cur_curse_y, cur_curse_x);

    switch (ch) {
        case 27: // Escape key
            handle_command_prompt(l_row, l_col, &cur_curse_y, &cur_curse_x);
            break;

        case KEY_BACKSPACE: // Backspace
        case 127:           // Compatibility with some terminals
            if (cur_curse_x > 0) {
                memmove(&buffer.lines[cur_curse_y].data[cur_curse_x - 1],
                        &buffer.lines[cur_curse_y].data[cur_curse_x],
                        buffer.lines[cur_curse_y].length - cur_curse_x + 1);
                buffer.lines[cur_curse_y].length--;
                mvdelch(cur_curse_y, cur_curse_x - 1);
                move(cur_curse_y, cur_curse_x - 1);
            } else if (cur_curse_y > 0) { // Handle deletion at the start of a line
                size_t prev_line_len = buffer.lines[cur_curse_y - 1].length;
                if (prev_line_len + buffer.lines[cur_curse_y].length < MAX_COLS) {
                    strcat(buffer.lines[cur_curse_y - 1].data, buffer.lines[cur_curse_y].data);
                    buffer.lines[cur_curse_y - 1].length += buffer.lines[cur_curse_y].length;
                    for (size_t i = cur_curse_y; i < buffer.num_lines - 1; i++) {
                        buffer.lines[i] = buffer.lines[i + 1];
                    }
                    buffer.num_lines--;
                    move(cur_curse_y - 1, prev_line_len);
                    clrtoeol();
                }
            }
            break;

        case KEY_DOWN: // Arrow Down
            if (cur_curse_y < buffer.num_lines - 1) {
                move(cur_curse_y + 1, cur_curse_x);
                if (cur_curse_x > buffer.lines[cur_curse_y + 1].length) {
                    move(cur_curse_y + 1, buffer.lines[cur_curse_y + 1].length);
                }
            } else if (buffer.num_lines < MAX_ROWS) { // Create a new line if at the bottom
                buffer.num_lines++;
                buffer.lines[buffer.num_lines - 1].data[0] = '\0';
                buffer.lines[buffer.num_lines - 1].length = 0;
                move(cur_curse_y + 1, 0);
            }
            break;

        case KEY_UP: // Arrow Up
            if (cur_curse_y > 0) {
                move(cur_curse_y - 1, cur_curse_x);
                if (cur_curse_x > buffer.lines[cur_curse_y - 1].length) {
                    move(cur_curse_y - 1, buffer.lines[cur_curse_y - 1].length);
                }
            }
            break;

        case KEY_LEFT: // Arrow Left
            if (cur_curse_x > 0) {
                move(cur_curse_y, cur_curse_x - 1);
            } else if (cur_curse_y > 0) {
                move(cur_curse_y - 1, buffer.lines[cur_curse_y - 1].length);
            }
            break;

        case KEY_RIGHT: // Arrow Right
            if (cur_curse_x < buffer.lines[cur_curse_y].length) {
                move(cur_curse_y, cur_curse_x + 1);
            } else if (cur_curse_y < buffer.num_lines - 1) {
                move(cur_curse_y + 1, 0);
            }
            break;

        case 24: // Ctrl-X to save
            save_buffer_to_file(filename);
            break;

        case '\n': // Enter key (create new line)
            if (buffer.num_lines < MAX_ROWS) {
                for (size_t i = buffer.num_lines; i > cur_curse_y + 1; i--) {
                    buffer.lines[i] = buffer.lines[i - 1];
                }
                buffer.lines[cur_curse_y + 1].length = 0;
                buffer.lines[cur_curse_y + 1].data[0] = '\0';
                size_t remaining_len = buffer.lines[cur_curse_y].length - cur_curse_x;
                if (remaining_len > 0) {
                    strncpy(buffer.lines[cur_curse_y + 1].data, &buffer.lines[cur_curse_y].data[cur_curse_x], remaining_len);
                    buffer.lines[cur_curse_y + 1].data[remaining_len] = '\0';
                    buffer.lines[cur_curse_y + 1].length = remaining_len;
                    buffer.lines[cur_curse_y].data[cur_curse_x] = '\0';
                    buffer.lines[cur_curse_y].length = cur_curse_x;
                }
                buffer.num_lines++;
                move(cur_curse_y + 1, 0);
            }
            break;

        default: // Handle character input
            if (cur_curse_x < MAX_COLS - 1 && cur_curse_y < MAX_ROWS) {
                memmove(&buffer.lines[cur_curse_y].data[cur_curse_x + 1],
                        &buffer.lines[cur_curse_y].data[cur_curse_x],
                        buffer.lines[cur_curse_y].length - cur_curse_x + 1);
                buffer.lines[cur_curse_y].data[cur_curse_x] = ch;
                buffer.lines[cur_curse_y].length++;
                mvaddch(cur_curse_y, cur_curse_x, ch);
                move(cur_curse_y, cur_curse_x + 1);
            }
            break;
    }
    refresh();
}


int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *filename = argv[1];

    initscr();
    keypad(stdscr, TRUE);
    noecho();

    int l_row, l_col;
    getmaxyx(stdscr, l_row, l_col);

    load_file_into_buffer(filename);

    while (1) {
        raw();
        int ch_outer = getch();
        handle_keypress(ch_outer, l_row, l_col, filename);
    }

    endwin();
    return 0;
}
