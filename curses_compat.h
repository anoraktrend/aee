#ifndef CURSES_COMPAT_H
#define CURSES_COMPAT_H

#include <curses.h>

// Map custom attributes to ncurses attributes
#define A_NC_BIG5 A_NORMAL

// Custom extensions needed by aee
void nc_setattrib(int flag);
void nc_clearattrib(int flag); 
bool nc_has_chinese(void);

// Map any custom functions to standard ncurses
#define clear_to_eol wclrtoeol
#define clear_to_end wclrtobot

#endif
