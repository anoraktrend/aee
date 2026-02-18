#ifndef HIGHLIGHTING_H
#define HIGHLIGHTING_H

#include "../include/aee.h"

// Function to highlight syntax in a line
void highlight_syntax(WINDOW *win, const char *line, int line_length);

// Function to check if a word is a keyword
int is_keyword(const char *word);

#endif // HIGHLIGHTING_H
