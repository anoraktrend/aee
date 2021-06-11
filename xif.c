/*
 | xif.c
 | $Revision $
 */

/*
 |	Copyright (c) 1987, 1988, 1991 - 1996, 2001, 2009, 2010 Hugh Mahon.
 */

#include "Xcurse.h"
#include <X11/Xutil.h>
#include <X11/cursorfont.h>

#include "aee.h"

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

/*
 |	definitions for xif.c
 */

char num_string[10];
int in_char;

Cursor ae_cursor;
Cursor blank_cursor;

Pixmap blank_pix;

int cursor_on;

char *aeMenuTitle = "ae menu";
char *aemenuitems[] = {
			"commands", 
			"functions", 
			"help", 
			"settings", 
			"file operations", 
			"search/replace", 
			"mark/paste text" 
			};

char *file_menutitle = "file operations";
char *file_menuitems[] = {
			"read file", 
			"write file" 
			};

char *sr_menutitle = "search/replace";
char *sr_menuitems[] = {
			"search for ...", 
			"search", 
			"replace" 
			};

char *mp_menutitle = "mark/paste text";
char *mp_menuitems[] = {
			"mark text", 
			"mark append", 
			"mark prefix", 
			"copy", 
			"cut", 
			"paste" 
			};

char *com_menutitle = "commands";
char *com_menuitems[] = {
			"buffer (create/move to)", 
			"define key", 
			"character", 
			"delete current buffer", 
			"current line number is", 
			"resequence line numbers", 
			"exit (save changes)", 
			"quit (no changes)" 
			};

char *com_strings[] = {
	"buffer ", "define ", "character", "delete", 
	"line", "resequence", "exit", "quit"
	};

char *func_menutitle = "functions";
char *func_menuitems[] = {
			"next page", 
			"previous page", 
			"next buffer", 
			"previous buffer", 
			"delete line", 
			"delete word", 
			"delete char", 
			"undelete line", 
			"undelete word", 
			"undelete char", 
			"begin of line", 
			"end of line", 
			"begin of text", 
			"end of text", 
			"insert line", 
			"next line", 
			"next word", 
			"previous word", 
			"match () [] <> {}", 
			"redraw screen" 
			};

XFontStruct *panelfont;

/*
 |	definitions from Xcurse.c
 */

extern int fontheight, fontwidth;
extern Display *dp;		/* display pointer			*/
extern int Xscreen;		/* screen 				*/
extern XEvent event;		/* event				*/
extern Colormap colormap;	/* colormap of display			*/
extern XColor Back_color;	/* struct holding background color values */
extern WINDOW *last_window_refreshed;
/* end of definitions from Xcurse.c	*/

extern Window wid;
extern XFontStruct *xaefont;
XButtonEvent *eventbutton;
XMotionEvent *eventmotion;
XKeyEvent *eventkey;

/* ----- radio button declaration section ----- */

short case_rb_button = 1;
char *case_rblabels[] = {
			"not case sensitive ", 
			"case sensitive " 
			};

short eight_rb_button = 0;
char *eight_rblabels[] = {
			"don't display eight bit characters", 
			"display eight bit characters" 
			};

short expand_rb_button = 0;
char *expand_rblabels[] = {
			"do not expand tabs", 
			"expand tabs to spaces"
			};

short indent_rb_button = 0;
char *indent_rblabels[] = {
			"do not indent ", 
			"automatically indent "
			};

short liter_rb_button = 0;
char *liter_rblabels[] = {
			"metacharacters allowed", 
			"literal search"
			};

short over_rb_button = 0;
char *over_rblabels[] = {
			"insert characters ", 
			"replace characters "
			};

short stat_rb_button = 0;
char *stat_rblabels[] = {
			"no status line", 
			"display status line"
			};

short window_rb_button = 1;
char *window_rblabels[] = {
			"display only one buffer", 
			"display multiple buffers"
			};

short wrap_rb_button = 1;
char *wrap_rblabels[] = {
			"cut off line at right margin", 
			"allow line of any length"
			};

short forward_rb_button = 0;
char *forward_rblabels[] = {
			"search reverse", 
			"search forward"
			};

char *pblabel[] = {
			"settings OK", 
			"cancel"
			};

short srch_fwd_rb_button = 0;
char *srch_fwd_rblabels[] = {
			"search reverse", 
			"search forward"
			};

short srch_lit_rb_button = 0;
char *srch_lit_rblabels[] = {
			"allow metacharacters",
			"literal search"
			};

short srch_cas_rb_button = 0;
char *srch_cas_rblabels[] = {
			"not case sensitive search", 
			"case sensitive search"
			};

char *srch_pblabel[] = {
			"settings OK", 
			"cancel"
			};


char *xsrch_string;

/* ----- replace panel declarations ----- */
short rpl_fwd_rb_button = 0;
char *rpl_fwd_rblabels[] = {
			"search reverse", 
			"search forward"
			};

short rpl_lit_rb_button = 0;
char *rpl_lit_rblabels[] = {
			"allow metacharacters",
			"literal search"
			};

short rpl_cas_rb_button = 0;
char *rpl_cas_rblabels[] = {
			"not case sensitive search", 
			"case sensitive search"
			};

char *rpl_pblabel[] = {
			"settings OK", 
			"cancel"
			};

char *xold_string;
char *xnew_string;

int tempx, tempy;
int wheight, wwidth;

/* ----- read panel declarations ----- */
char *read_pblabel[] = {
			"OK", 
			"cancel"
			};

char *xread_string;

/* -----write panel declarations ----- */
char *write_pblabel[] = {
			"OK", 
			"cancel"
			};

char *xwrite_string;

/* ----- buffer panel declarations ----- */
char *buff_pblabel[] = {
			"OK", 
			"cancel"
			};

char *xbuff_string;

/* ----- define key panel declarations ----- */
char *defkey_pblabel[] = {
			"OK", 
			"cancel"
			};

char *xdefkey_string;
int menu_active;

char *menubackground;	/* name of background color	*/
char *menuforeground;	/* name of foreground color	*/
int menufore_pixel;	/* the pixel value of the foreground color	*/
int menuback_pixel;	/* the pixel value of the background color	*/
XColor menuFore_color;	/* structure which holds foreground color values */
XColor menuBack_color;	/* structure which holds background color values */

static unsigned int xae_window_height, xae_window_width;

#define icon_width 48
#define icon_height 48

static int xfd;
static int number_of_events;
static fd_set ttymask;
static struct timeval timeout;

static char xae_in_focus = TRUE;

void 
event_init()
{
	XSelectInput(dp, wid, (KeyPressMask | ButtonPressMask | ButtonReleaseMask | 
	    ExposureMask | ButtonMotionMask | PointerMotionMask | StructureNotifyMask | 
	    FocusChangeMask));
	in_char = 0;
	xae_window_height = LINES * fontheight;
	xae_window_width = COLS * fontwidth;
	eventbutton = (XButtonEvent *) &event;
	eventmotion = (XMotionEvent *) &event;
	eventkey= (XKeyEvent *) &event;
#ifdef hide_cursor
	ae_cursor = XCreateFontCursor(dp, XC_arrow);
	XDefineCursor(dp, wid, ae_cursor);
	cursor_on = TRUE;
	blank_pix = XCreateBitmapFromData(dp, wid, blank_bits, 1, 1, Back_color, Back_color, XDefaultDepth(dp, Xscreen));
	blank_cursor = XCreatePixmapCursor(dp, blank_pix, None, 0, 0, 0, 0);
#endif
	panelfont = xaefont;
	xfd = ConnectionNumber(dp);
}

static void 
paste_it()
{
	int temp_indent;  /* if indent must be set FALSE, store original value here */
	char *temp_buff;
	char *temp_point;
	int counter;
	int buff_counter;

	temp_indent = indent;
	if (indent)
	{
		indent = FALSE;
	}
	temp_buff = XFetchBytes(dp, &buff_counter);
	temp_point = temp_buff;
	counter = 0;
	while ((counter < buff_counter) && (temp_buff != NULL))
	{
		in = *temp_point;
		if ((in == '\n') || (in == '\r'))
			insert_line(TRUE);
		else
			insert(in);
		if (counter < buff_counter)
			temp_point++;
		counter++;
	}
	indent = temp_indent;
	if (temp_buff != NULL)
		free(temp_buff);
	wmove(curr_buff->win, curr_buff->scr_vert, curr_buff->scr_horz);
	wrefresh(curr_buff->win);
}

void 
event_manage()		/* manage X-windows events for xae	*/
{
	int valid;
	int tempy;
	int tempx;
	int cut_paste;	/* mark text to put in X buffer	*/
	int counter;
	int buff_counter;
	int temp_indent;  /* if indent must be set FALSE, store original value here */
	char *temp_point;
	char *temp_buff;
	int temp;

	valid = FALSE;
	cut_paste = FALSE;

	do {
#ifndef XIF_DEBUG
		number_of_events = XPending(dp);
		if ((xae_in_focus) && (number_of_events == 0))
		{
			do 
			{

				timeout.tv_sec = 0;
				timeout.tv_usec = 500000;
				FD_ZERO(&ttymask);
				FD_SET(xfd, &ttymask);
				temp = select((xfd+1), &ttymask, NULL, NULL, &timeout);

				if (temp == 0)
				{
					draw_cursor(FALSE);
					XFlush(dp);
				}
				else
				{
					draw_cursor(TRUE);
					XFlush(dp);
				}
			} while (temp == 0);
		}
#endif /* ifndef XIF_DEBUG */

		XNextEvent(dp, &event);


		if (event.type == KeyPress)
		{
			KeySym key;
#ifdef hide_cursor
			if (cursor_on)
			{
				XDefineCursor(dp, wid, blank_cursor);
				XFlush(dp);
				cursor_on = FALSE;
			}
#endif
			char text[10];
			XLookupString(eventkey, text, 10, &key, NULL);
			char *keystring = XKeysymToString(key);

			if ((keystring != NULL) && (strcmp("Insert", keystring) == 0))
			{
				if ((eventkey->state & ShiftMask) != 0)
					paste_it();
				else
				{
					overstrike = !overstrike;
					status_display();
				}
			}
			else
				valid = wgetch(stdscr);
		}
		else if ((event.type == MotionNotify) && ((eventmotion->state == (ShiftMask | Button2MotionMask)) || (eventmotion->state == Button1MotionMask)))
		{
#ifdef hide_cursor
			if (!cursor_on)
			{
				cursor_on = TRUE;
				XDefineCursor(dp, wid, ae_cursor);
			}
#endif
			tempx = eventbutton->x / fontwidth;
			tempy = eventbutton->y / fontheight;
			tempy -= curr_buff->window_top;
			if (tempy < 0)
				tempy = 0;
			if (tempy > curr_buff->last_line)
				tempy = curr_buff->last_line;
			if (tempx < 0)
				tempx = 0;
			if (tempx > curr_buff->last_col)
				tempx = curr_buff->last_col;
			move_to_xy(tempx, tempy);
			if ((!cut_paste) && (!mark_text))
			{
				cut_paste = TRUE;
				slct(1);
			}
			wrefresh(curr_buff->win);
		}
#ifdef hide_cursor
		else if ((event.type == MotionNotify) && (!cursor_on))
		{
			XDefineCursor(dp, wid, ae_cursor);
			XFlush(dp);
			cursor_on = TRUE;
		}
#endif
		else if ((event.type == ButtonRelease) && (((eventbutton->button == Button2) && (eventbutton->button == ShiftMask)) || (eventbutton->button == Button1)) && (cut_paste))
		{
			cut_text();
			cut_paste = FALSE;
		}
		else if ((event.type == ButtonPress) && (((eventbutton->button == Button3) && (eventbutton->state == ShiftMask)) || ((eventbutton->button == Button2) && (!(eventbutton->state & ShiftMask)))))
		{
			paste_it();
		}
		else if ((event.type == ButtonPress) && (eventbutton->button == Button1))
		{
			tempy = (eventbutton->y / fontheight);
			/* make sure cursor is in window	*/
			if ((!mark_text) && (tempy < curr_buff->window_top) && 
					(curr_buff->window_top > 0))
			{
				if (!((info_win != 0) && 
						(tempy < info_win_height)))
				{
					while ((tempy < curr_buff->window_top) 
					   && (curr_buff->window_top > 0))
					     parse(fn_PB_str); /* prev buffer */
				}
			}
			else if ((!mark_text) && (tempy > 
			     (curr_buff->window_top + curr_buff->lines)) && 
			     (curr_buff->next_buff != NULL))
			{
				while ((curr_buff->next_buff != NULL) && 
				   (tempy >= curr_buff->next_buff->window_top))
					parse(fn_NB_str);  /* next buffer  */
			}
			else
			{
				tempy -= curr_buff->window_top;
				tempx = eventbutton->x / fontwidth;

				if (tempy < 0)
					tempy = 0;
				if (tempy > curr_buff->last_line)
					tempy = curr_buff->last_line;
				if (tempx < 0)
					tempx = 0;
				if (tempx > curr_buff->last_col)
					tempx = curr_buff->last_col;
				move_to_xy(tempx, tempy);
			}
			status_display();
			wrefresh(curr_buff->win);
		}
		else if (event.type == Expose)
		{
			while (XCheckTypedEvent(dp, Expose, &event))
				;
			/*
			 |	time to cheat, we know which is the last 
			 |	window that was refreshed
			 */

			clearok(curr_buff->win, TRUE);
			touchwin(last_window_refreshed);
			touchwin(last_window_refreshed);
			wrefresh(last_window_refreshed);
			XFlush(dp);
		}
		else if (event.type == GraphicsExpose)
		{
			while (XCheckTypedEvent(dp, GraphicsExpose, &event))
				;
			/*
			 |	A real hack to handle situation when 
			 |	window is partially obscured by another 
			 |	window.  There are better ways to handle 
			 |	this that completely redrawing the 
			 |	window.
			 */

			clearok(curr_buff->win, TRUE);
			touchwin(last_window_refreshed);
			wrefresh(last_window_refreshed);
			XFlush(dp);
		}
		else if (event.type == ConfigureNotify)	/* resize of window */
		{
			if ((xae_window_height != event.xconfigure.height ) || 
			    (xae_window_width != event.xconfigure.width ))
			{
				LINES = event.xconfigure.height / fontheight;
				COLS = event.xconfigure.width / fontwidth;
				XClearWindow(dp, wid);
				XFlush(dp);
				reinitscr();
				resize_check();
				wrefresh(curr_buff->win);
				XFlush(dp);
				xae_window_height = event.xconfigure.height;
				xae_window_width = event.xconfigure.width;
			}
		}
		else if (event.type == FocusOut)	/* loss of focus */
		{
			xae_in_focus = FALSE;
			draw_cursor(TRUE);
			if (curr_buff->curr_line->changed && curr_buff->journalling)
				write_journal(curr_buff, curr_buff->curr_line);
		}
		else if (event.type == FocusIn)	/* acquisition of focus */
		{
			xae_in_focus = TRUE;
		}
		else if (cut_paste)
		{
			cut_text();
			cut_paste = FALSE;
		}
	} while (!valid);
}


void 
move_to_xy(x, y)	/* move text cursor to mouse cursor position	*/
int x, y;	/* horizontal and vertical position of the mouse	*/
{
	if ((curr_buff->scr_vert > y) || ((curr_buff->scr_vert == y) && (curr_buff->scr_horz > x)))
	{
		if ((curr_buff->position != 1) && ((curr_buff->scr_vert - (curr_buff->scr_pos / COLS)) > y))
			bol();			/* bol	*/
		while ((curr_buff->position == 1) && (curr_buff->curr_line->prev_line != NULL) && 
			((curr_buff->scr_vert - curr_buff->curr_line->prev_line->vert_len) > y))
			bol();			/* bol	*/
		if ((curr_buff->position == 1) && ((curr_buff->scr_vert - 
		      curr_buff->curr_line->prev_line->vert_len) <= y) && (curr_buff->scr_vert > y))
			eopl();			/* end of prev line	*/
		while ((curr_buff->scr_vert != y) && (curr_buff->position > 1))
			left(TRUE);		/* left	*/
		while ((curr_buff->scr_vert == y) && (curr_buff->scr_horz > x) && (curr_buff->position > 1))
			left(TRUE);	/* left	*/
		if (curr_buff->scr_vert < y)
			adv_line();	/* adv line	*/
	}
	else
	{
		if (((curr_buff->curr_line->vert_len - (curr_buff->scr_pos / COLS)) + curr_buff->scr_vert) <= y)
			adv_line();	/* adv line	*/
		while (((curr_buff->scr_vert + curr_buff->curr_line->vert_len) <= y) && 
						(curr_buff->curr_line->next_line != NULL))
			adv_line();	/* adv line	*/
		while ((curr_buff->scr_vert < y) && (curr_buff->position < curr_buff->curr_line->line_length))
			right(TRUE);	/* right	*/
		while ((curr_buff->scr_horz < x) && (curr_buff->scr_vert == y) && 
					(curr_buff->position < curr_buff->curr_line->line_length))
			right(TRUE);	/* right	*/
		if (curr_buff->scr_vert > y)
			left(TRUE);
	}
}

void 
eopl()		/* move to end of previous line without shifting screen	*/
{
	if (curr_buff->position != 1)
		bol();
	if ((curr_buff->curr_line->prev_line != NULL) && (curr_buff->scr_vert - curr_buff->curr_line->prev_line->vert_len) < 0)
	{
		curr_buff->curr_line = curr_buff->curr_line->prev_line;
		curr_buff->pointer = curr_buff->curr_line->line;
		curr_buff->position = 1;
		curr_buff->scr_pos = 0;
		curr_buff->scr_vert -= curr_buff->curr_line->vert_len;
		while (curr_buff->position < curr_buff->curr_line->line_length)
		{
			curr_buff->pointer++;
			curr_buff->position++;
		}
		curr_buff->scr_pos = scanline(curr_buff->curr_line, curr_buff->position);
		curr_buff->abs_pos = curr_buff->scr_pos;
		curr_buff->scr_vert += curr_buff->scr_pos / COLS;
		curr_buff->scr_horz = curr_buff->scr_pos % COLS;
		if (mark_text)
			slct_line("u");
	}
	else
		left(TRUE);
}

void 
cut_text()
{
	int counter;
	int buff_counter;
	char *out_buff;
	char *temp_point;
	char *temp_buff;
	struct text *temp_paste;
	struct text *temp_text;

	temp_paste = paste_buff;
	paste_buff = NULL;
	copy();
	temp_text = paste_buff;
	counter = 0;
	while (temp_text != NULL)
	{
		counter += temp_text->line_length;
		temp_text = temp_text->next_line;
	}
	out_buff = malloc(counter + 1);
	temp_point = out_buff;
	temp_text = paste_buff;
	while (temp_text != NULL)
	{
		buff_counter = 1;
		temp_buff = temp_text->line;
		while (buff_counter < temp_text->line_length)
		{
			*temp_point = *temp_buff;
			temp_point++;
			temp_buff++;
			buff_counter++;
		}
		if (temp_text->next_line != NULL)
		{
			*temp_point = '\n';
			temp_point++;
			temp_text = temp_text->next_line;
			free(temp_text->prev_line->line);
			free(temp_text->prev_line);
		}
		else
		{
			counter -= 1;
			*temp_point = '\0';
			free(temp_text->line);
			free(temp_text);
			temp_text = NULL;
		}
	}
	if (counter > 0)
		XStoreBytes(dp, out_buff, counter);
	free(out_buff);
	paste_buff = temp_paste;
	midscreen(curr_buff->scr_vert, curr_buff->position);
	wrefresh(curr_buff->win);
}


void 
raise_window()
{
	XRaiseWindow(dp, wid);
}

void 
set_window_name(name)
char *name;
{
	static char *window_name = NULL;

	if (window_name != NULL)
		free(window_name);

	window_name = xalloc(strlen("xae:") + strlen(name) + 1);
	strcpy(window_name, "xae:");
	strcat(window_name, name);
	XStoreName(dp, wid, window_name);
	XSetIconName(dp, wid, window_name);
}
