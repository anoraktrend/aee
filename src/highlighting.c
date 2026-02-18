#include "../include/highlighting.h"
#include <string.h>
#include <ctype.h>

// List of C keywords
const char *c_keywords[] = {
    "auto", "break", "case", "char", "const", "continue", "default", "do",
    "double", "else", "enum", "extern", "float", "for", "goto", "if",
    "int", "long", "register", "return", "short", "signed", "sizeof", "static",
    "struct", "switch", "typedef", "union", "unsigned", "void", "volatile", "while",
    NULL
};

int is_keyword(const char *word) {
    for (int i = 0; c_keywords[i] != NULL; i++) {
        if (strcmp(word, c_keywords[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

void highlight_syntax(WINDOW *win, const char *line, int line_length) {
    int i = 0;
    int in_string = 0;
    int in_char = 0;
    int in_line_comment = 0;
    int in_block_comment = 0;
    int escaped = 0;

    while (i < line_length && line[i] != '\0') {
        if (in_line_comment) {
            // Comments in green
            wattron(win, COLOR_PAIR(2));
            waddch(win, line[i]);
            wattroff(win, COLOR_PAIR(2));
            if (line[i] == '\n') {
                in_line_comment = 0;
            }
            i++;
            continue;
        }

        if (in_block_comment) {
            wattron(win, COLOR_PAIR(2));
            waddch(win, line[i]);
            wattroff(win, COLOR_PAIR(2));
            if (line[i] == '*' && i + 1 < line_length && line[i + 1] == '/') {
                wattron(win, COLOR_PAIR(2));
                waddch(win, line[i]);
                waddch(win, line[i + 1]);
                wattroff(win, COLOR_PAIR(2));
                in_block_comment = 0;
                i += 2;
                continue;
            }
            i++;
            continue;
        }

        if (in_string) {
            wattron(win, COLOR_PAIR(3));
            waddch(win, line[i]);
            wattroff(win, COLOR_PAIR(3));
            if (line[i] == '"' && !escaped) {
                in_string = 0;
            }
            escaped = (line[i] == '\\' && !escaped);
            i++;
            continue;
        }

        if (in_char) {
            wattron(win, COLOR_PAIR(3));
            waddch(win, line[i]);
            wattroff(win, COLOR_PAIR(3));
            if (line[i] == '\'' && !escaped) {
                in_char = 0;
            }
            escaped = (line[i] == '\\' && !escaped);
            i++;
            continue;
        }

        // Check for comments
        if (line[i] == '/' && i + 1 < line_length) {
            if (line[i + 1] == '/') {
                in_line_comment = 1;
                wattron(win, COLOR_PAIR(2));
                waddch(win, line[i]);
                waddch(win, line[i + 1]);
                wattroff(win, COLOR_PAIR(2));
                i += 2;
                continue;
            } else if (line[i + 1] == '*') {
                in_block_comment = 1;
                wattron(win, COLOR_PAIR(2));
                waddch(win, line[i]);
                waddch(win, line[i + 1]);
                wattroff(win, COLOR_PAIR(2));
                i += 2;
                continue;
            }
        }

        // Check for strings
        if (line[i] == '"') {
            in_string = 1;
            wattron(win, COLOR_PAIR(3));
            waddch(win, line[i]);
            wattroff(win, COLOR_PAIR(3));
            i++;
            continue;
        }

        // Check for char literals
        if (line[i] == '\'') {
            in_char = 1;
            wattron(win, COLOR_PAIR(3));
            waddch(win, line[i]);
            wattroff(win, COLOR_PAIR(3));
            i++;
            continue;
        }

        // Check for keywords
        if ((isalnum(line[i]) || line[i] == '_') && !in_string && !in_char) {
            int start = i;
            while (i < line_length && (isalnum(line[i]) || line[i] == '_')) {
                i++;
            }
            int len = i - start;
            char word[256];
            if (len < 256) {
                strncpy(word, &line[start], len);
                word[len] = '\0';
                if (is_keyword(word)) {
                    wattron(win, COLOR_PAIR(1));
                    for (int j = start; j < i; j++) {
                        waddch(win, line[j]);
                    }
                    wattroff(win, COLOR_PAIR(1));
                    continue;
                }
            }
            // Not a keyword, print normally
            for (int j = start; j < i; j++) {
                waddch(win, line[j]);
            }
            continue;
        }

        // Normal character
        waddch(win, line[i]);
        i++;
    }
}
