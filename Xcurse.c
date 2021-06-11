/*
	$Header: /home/hugh/sources/aee/RCS/Xcurse.c,v 1.41 2010/07/18 00:52:24 hugh Exp hugh $
*/

char *XCURSE_copyright_notice = "Copyright (c) 1987, 1988, 1991, 1992, 1994, 1995, 1996, 1998, 1999, 2001, 2004, 2009, 2010 Hugh Mahon.";

char *XCURSE_version_string = "@(#) Xcurse.c $Revision: 1.41 $";

#include "Xcurse.h"
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>

#include "aee.h"

#ifdef HAS_STDLIB
#include <stdlib.h>
#endif

#if defined(__STDC__)
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#ifdef HAS_UNISTD
#include <unistd.h>
#endif

#ifdef HAS_SYS_IOCTL
#include <sys/ioctl.h>
#endif

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif /* min */

extern int eightbit;

static WINDOW *virtual_scr;
WINDOW *curscr;
WINDOW *stdscr;
WINDOW *last_window_refreshed;

WINDOW *clear_win;

char *new_curse = "February 1988";

int STAND = FALSE;	/* is standout mode activated?			*/
int Curr_x;		/* current x position on screen			*/
int Curr_y;		/* current y position on the screen		*/
unsigned int LINES;
unsigned int COLS;
int initialized = FALSE;	/* tells whether new_curse is initialized	*/
int Repaint_screen;	/* if an operation to change screen impossible, repaint screen	*/
int Num_bits;	/* number of bits per character	*/

int XKey_vals[] = {
	84, 83, 82, 81, 89, 90, 91, 
	92, 0, 0, 0, 0, 134, 133, 132, 135, 127, 119, 
	111, 103, 102, 110, 94, 
	0, 0, 0, 0, 255
};

/*
 |	names of keys used below obtained through XKeysymToString()
 */

char *Key_strings[] = {
	"F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", 
	"F11", "F12", "Up", "Down", "Left", "Right", "Next", "Prior", 
	"Delete", "hpDeleteLine", "hpInsertLine", "Insert", "End", 
	"hpInsertChar", "hpDeleteChar", "BackSpace", "Home", NULL
	};

int Key_vals[] = {
	KEY_F(1), KEY_F(2), KEY_F(3), KEY_F(4), KEY_F(5), KEY_F(6), KEY_F(7),
	KEY_F(8), KEY_F(9), KEY_F(10), KEY_F(11), KEY_F(12), 
	KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_NPAGE, KEY_PPAGE,
	KEY_DC, KEY_DL, KEY_IL, KEY_IC, KEY_EOL, KEY_IC, KEY_DC, KEY_BACKSPACE,
	KEY_HOME, 255
};

/* the following data should go into a data structure in a future version */

Window wid;		/* window id				*/
Display *dp;		/* display pointer			*/
int Xscreen;		/* screen 				*/
char *font_name;	/* name of font to use			*/
XFontStruct *xaefont;	/* font information			*/
int fontwidth, fontheight;	/* font information		*/
XSetWindowAttributes win_attrib;	/* window attributes	*/
XEvent event;		/* event				*/
XKeyEvent *keyevent;	/* keyboard event structure		*/
char *background;	/* name of background color		*/
char *foreground;	/* name of foreground color		*/
char *geometry;		/* height, width, xoffset, yoffset	*/
char reverse_video;	/* reverse fore- and back-ground	*/
int fore_pixel;		/* the pixel value of the foreground color */
int back_pixel;		/* the pixel value of the background color */
int temp_pixel;		/* temporary storage 			*/
XColor Fore_color;	/* struct holding foreground color values */
XColor Back_color;	/* struct holding background color values */
XColor exact_def_return;
Pixmap Fore_pixmap;	/* foreground pixmap			*/
Pixmap Back_pixmap;	/* background pixmap			*/
short bits;
Colormap colormap;
GC gc, revgc, cursor_gc, cursor_revgc;
XGCValues gcvalues;
/* end of information to be put into a data structure at a future date	*/

Pixmap Temp_pixmap;	/* temporary storage, used when reversing colors */

int xtemp, ytemp;

#define icon_width 48
#define icon_height 48
static unsigned char icon_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x80, 0x01, 0x00, 0x00, 0x00,
   0x00, 0x80, 0x02, 0x00, 0x00, 0x00, 0x00, 0x40, 0x02, 0x00, 0x00, 0x00,
   0x00, 0x40, 0x04, 0x00, 0x00, 0x00, 0x00, 0x20, 0x04, 0x00, 0x00, 0x00,
   0x00, 0x20, 0x08, 0x00, 0x00, 0x00, 0x00, 0x10, 0x08, 0x00, 0x00, 0x00,
   0x00, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x08, 0x10, 0x00, 0xf8, 0x03,
   0x00, 0x04, 0x20, 0x00, 0x06, 0x0c, 0x00, 0x04, 0x20, 0x00, 0x01, 0x10,
   0x00, 0x02, 0x40, 0x80, 0x00, 0x20, 0x00, 0x02, 0x40, 0x40, 0x00, 0x40,
   0x00, 0x03, 0x80, 0x40, 0x00, 0x40, 0x00, 0xff, 0xff, 0x20, 0x00, 0x80,
   0x80, 0x00, 0x00, 0x21, 0x00, 0x80, 0x80, 0x00, 0x00, 0xe1, 0xff, 0xff,
   0x40, 0x00, 0x00, 0x22, 0x00, 0x80, 0x20, 0x00, 0x00, 0x26, 0x00, 0x00,
   0x20, 0x00, 0x00, 0x24, 0x00, 0x00, 0x10, 0x00, 0x00, 0x24, 0x00, 0x00,
   0x10, 0x00, 0x00, 0x48, 0x00, 0x00, 0x08, 0x00, 0x00, 0x48, 0x00, 0x00,
   0x08, 0x00, 0x00, 0x90, 0x00, 0x10, 0x04, 0x00, 0x00, 0x10, 0x01, 0x08,
   0x04, 0x00, 0x00, 0x20, 0x06, 0x04, 0x02, 0x00, 0x00, 0x20, 0xf8, 0x03};

Pixmap pixmap;

int *virtual_lines;


/*
 |	Copy the contents of one window to another.
 */

void 
copy_window(origin, destination)
WINDOW *origin, *destination;
{
	int row, column;
	struct _line *orig, *dest;

	orig = origin->first_line;
	dest = destination->first_line;

	for (row = 0; 
		row < (min(origin->Num_lines, destination->Num_lines)); 
			row++)
	{
		for (column = 0; 
		    column < (min(origin->Num_cols, destination->Num_cols)); 
			column++)
		{
			dest->row[column] = orig->row[column];
			dest->attributes[column] = orig->attributes[column];
		}
		dest->changed = orig->changed;
		dest->scroll = orig->scroll;
		dest->last_char = min(orig->last_char, destination->Num_cols);
		orig = orig->next_screen;
		dest = dest->next_screen;
	}
	destination->LX = min((destination->Num_cols - 1), origin->LX);
	destination->LY = min((destination->Num_lines - 1), origin->LY);
	destination->Attrib = origin->Attrib;
	destination->scroll_up = origin->scroll_up;
	destination->scroll_down = origin->scroll_down;
	destination->SCROLL_CLEAR = origin->SCROLL_CLEAR;
}

void 
reinitscr() 
{
	WINDOW *local_virt;
	WINDOW *local_std;
	WINDOW *local_cur;

	local_virt = newwin(LINES, COLS, 0, 0);
	local_std = newwin(LINES, COLS, 0, 0);
	local_cur = newwin(LINES, COLS, 0, 0);
	copy_window(virtual_scr, local_virt);
	copy_window(stdscr, local_std);
	copy_window(curscr, local_cur);
	delwin(virtual_scr);
	delwin(stdscr);
	delwin(curscr);
	virtual_scr = local_virt;
	stdscr = local_std;
	curscr = local_cur;
	free(virtual_lines);
	virtual_lines = (int *) malloc(LINES * (sizeof(int)));
}

int argc1;
char **argv1;


void 
get_options(argc, argv)	/* get arguments from original command line	*/
int argc;
char *argv[];
{
	char *buff;
	char *extens;
	int count;
	int ret_val;
	struct files *temp_names;
	int *x_off, *y_off;

	buff = ae_basename(argv[0]);
	if (!strcmp(buff, "rxae"))
	{
		restricted = TRUE;
		change_dir_allowed = FALSE;
	}

	argc1 = argc;
	argv1 = argv;
	x_off = (int *) &xtemp;
	y_off = (int *) &ytemp;
	if ((dp = XOpenDisplay(0)) == NULL)
	{
		printf("could not open display\n");
		exit(-1);
	}
	Xscreen = DefaultScreen(dp);
	geometry = NULL;
	reverse_video = FALSE;
	foreground = NULL;
	background = NULL;
	font_name = NULL;
	get_defaults();
	top_of_stack = NULL;
	echo_flag = TRUE;
	journ_on = TRUE;
	input_file = FALSE;
	recv_file = FALSE;
	recover = FALSE;
	count = 1;
	while (count < argc)
	{
		buff = argv[count];
		if (*buff == '-')	/* options	*/
		{
			++buff;
			if (*buff == 'j')	/* turn off recover file */
				journ_on = FALSE;
			else if (*buff == 'r')	/* recover from bad exit */
				recover = TRUE;
			else if (*buff == 'e')	/* turn off echo in init file */
				echo_flag = FALSE;
			else if (*buff == 'R')
				reverse_video = TRUE;
			else if (*buff == 'i')	/* turn off info window	*/
				info_window = FALSE;
			else if (!strcmp(argv[count], "-text"))
			{
				text_only = TRUE;
			}
			else if (!strcmp(argv[count], "-binary"))
			{
				text_only = FALSE;
			}
			else if (*buff == 't')	/* expand tabs to spaces */
				expand = TRUE;
			else if (*buff == 'n')	
					/* turn off highlighting on menus*/
				nohighlight = TRUE;
			else if (!strcmp(argv[count], "-fg"))
			{
				count++;
				foreground = argv[count];
			}
			else if (!strcmp(argv[count], "-bg"))
			{
				count++;
				background = argv[count];
			}
			else if (!strcmp(argv[count], "-fn"))
			{
				count++;
				font_name = argv[count];
			}
			else if (!strncmp(buff, "geometry", strlen(buff)))
			{
				count++;
				geometry = argv[count];
			}
		}
		else if (*buff == '+')
		{
			buff++;
			start_at_line = buff;
		}
		else	/* if the argument wasn't an option, must be a file name, right?	*/
		{
			if (top_of_stack == NULL)
				temp_names = top_of_stack = name_alloc();
			else
			{
				temp_names->next_name = name_alloc();
				temp_names = temp_names->next_name;
			}
			temp_names->next_name = NULL;
			extens = temp_names->name = xalloc(strlen(buff) + 1);

			while (*buff != '\0')
			{
				*extens = *buff;
				buff++;
				extens++;
			}
			*extens = '\0';
			input_file = TRUE;
			recv_file = TRUE;
		}
		count++;
	}
	if (geometry != NULL)
		ret_val = XParseGeometry(geometry, x_off, y_off, &COLS, &LINES);
	if (COLS == 0)
		COLS = 80;
	if (LINES == 0)
		LINES = 24;
	if (xtemp == 0)
		xtemp = 10;
	if (ytemp == 0)
		ytemp = 15;
	win_height = LINES;
	win_width = COLS;
/*	if (top_of_stack == NULL)
	{
		fprintf(stderr, usage_str, argv[0]);
		exit(0);
	}
*/
}

void 
get_defaults()
{
	char *temp;

	geometry = XGetDefault(dp, "xae", "geometry");
	foreground = XGetDefault(dp, "xae", "foreground");
	background = XGetDefault(dp, "xae", "background");
	temp = getenv("XAEBACKGROUND");
	if ((temp != NULL) && (*temp != '\0'))
		background = temp;
	temp = getenv("XAEFOREGROUND");
	if ((temp != NULL) && (*temp != '\0'))
		foreground = temp;
	temp = getenv("XAEGEOMETRY");
	if ((temp != NULL) && (*temp != '\0'))
		geometry = temp;
	font_name = XGetDefault(dp, "xae", "font");
	temp = XGetDefault(dp, "xae", "ReverseVideo");
	if ((temp != NULL) && (!strcmp(temp, "on")))
		reverse_video = TRUE;
}

void 
initscr(cols, lines)		/* initialize terminal for operations	*/
int cols, lines;
{
	int temp_val;
	XSizeHints  xsh;
	XWMHints wmHints;

#ifdef DEBUG
{
/*
 |	The following code will start an hpterm which will in turn execute 
 |	a debugger and "adopt" the process, so it can be debugged.
 */

	int child_id;
	char command_line[256];

	child_id = getpid();
	if (!fork())
	{
	    sprintf(command_line, 
	      "hpterm -fg wheat -bg DarkSlateGrey -n %s -e xdb %s -P %d", 
			"hello",  "xae", child_id);
		execl("/bin/sh", "sh", "-c", command_line, NULL);
		fprintf(stderr, "could not exec new window\n");
		exit(1);
	}

/*
 |	When in debugger, set child_id to zero (p child_id=0) to continue 
 |	executing the software.
 */

	while (child_id != 0)
		;
}
#endif

	LINES = lines;
	COLS = cols;
	keyevent = (XKeyEvent *) &event;
	back_pixel = XWhitePixel(dp, Xscreen);
	fore_pixel = XBlackPixel(dp, Xscreen);
	colormap = XDefaultColormap(dp, Xscreen);
	if (DisplayPlanes(dp, Xscreen) > 1)
	{
		if (foreground == NULL)
			foreground = "white";
		if (XParseColor(dp, colormap, foreground, &Fore_color) != 0)
		{
			XAllocColor(dp, colormap, &Fore_color);
			fore_pixel = Fore_color.pixel;
		}
		if (background == NULL)
			background = "#6B002F003100";
		if (XParseColor(dp, colormap, background, &Back_color) != 0)
		{
			XAllocColor(dp, colormap, &Back_color);
			back_pixel = Back_color.pixel;
		}
	}
	if (reverse_video)
	{
		temp_pixel = fore_pixel;
		fore_pixel = back_pixel;
		back_pixel = temp_pixel;
		bits = 0x99;
	}
	if (font_name == NULL)
	{
		font_name = "6x10";
		if ((XDisplayHeight(dp, Xscreen) > (LINES * 15)) &&
		    (XDisplayWidth(dp, Xscreen) > (COLS * 9)))
			font_name = "9x15";
	}
	if ((xaefont = XLoadQueryFont(dp, font_name)) == NULL)
	{
		printf("could not open font \"%s\" \n", font_name);
		exit(-1);
	}
	fontwidth = xaefont->max_bounds.rbearing - xaefont->min_bounds.lbearing;
	fontheight = xaefont->max_bounds.ascent + xaefont->max_bounds.descent;

	if (xtemp < 0)
	{
		temp_val = XDisplayWidth(dp, Xscreen);
		xtemp = temp_val - ((COLS*fontwidth) - xtemp);
	}
	if (ytemp < 0)
	{
		temp_val = XDisplayHeight(dp, Xscreen);
		ytemp = (temp_val + ytemp) - (LINES*fontheight);
	}
	if ((wid = XCreateWindow(dp, RootWindow(dp, Xscreen), xtemp, ytemp, 
	     (COLS*fontwidth), (LINES*fontheight), 2, 
	     DefaultDepth(dp, Xscreen), InputOutput, 
	     DefaultVisual(dp, Xscreen), (CWBackPixel | CWColormap), 
	     &win_attrib)) == (Window) NULL)
	{
		printf("could not create window \n");
		exit(-1);
	}
/*
 |	Try to set up some default values and icon information
 */

	pixmap = XCreateBitmapFromData(dp, wid, (char *)icon_bits, icon_width, icon_height);
	xsh.height = fontheight * LINES;
	xsh.width = fontwidth * COLS;
	xsh.base_height = xsh.height;
	xsh.base_width = xsh.width;
	xsh.max_width = XDisplayWidth(dp, DefaultScreen(dp));
	xsh.max_height = XDisplayHeight(dp, DefaultScreen(dp));
	xsh.min_height = fontheight * 24;
	xsh.min_width = fontwidth * 40;
	xsh.x = xtemp;
	xsh.y = ytemp;
	xsh.width_inc = fontwidth;
	xsh.height_inc = fontheight;
	xsh.flags = (USPosition | PSize | PResizeInc );
	XSetStandardProperties(dp, wid, "xae", "xae", pixmap, argv1, argc1, &xsh);

	win_attrib.override_redirect = FALSE;
	XSetWindowBackground(dp, wid, back_pixel);
	XChangeWindowAttributes (dp, wid, CWOverrideRedirect, 
		&win_attrib);
	gcvalues.font = xaefont->fid;
	gcvalues.foreground = fore_pixel;
	gcvalues.background = back_pixel;
	gcvalues.function   = GXcopy;
	gc = XCreateGC(dp, wid, (GCFont | GCForeground | 
			GCBackground | GCFunction), &gcvalues);
	gcvalues.background = fore_pixel;
	gcvalues.foreground = back_pixel;
	revgc = XCreateGC(dp, wid, (GCFont | GCForeground | 
			GCBackground | GCFunction), &gcvalues);

	gcvalues.foreground = fore_pixel;
	gcvalues.background = back_pixel;
	gcvalues.function   = GXcopy;
	cursor_gc = XCreateGC(dp, wid, (GCFont | GCForeground | 
			GCBackground | GCFunction), &gcvalues);
	gcvalues.background = fore_pixel;
	gcvalues.foreground = back_pixel;
	cursor_revgc = XCreateGC(dp, wid, (GCFont | GCForeground | 
			GCBackground | GCFunction), &gcvalues);

	XSetLineAttributes(dp, gc,    1, LineSolid, CapButt, JoinMiter);
	XSetLineAttributes(dp, revgc, 1, LineSolid, CapButt, JoinMiter);
	XSetLineAttributes(dp, cursor_gc,    1, LineSolid, CapButt, JoinMiter);
	XSetLineAttributes(dp, cursor_revgc, 1, LineSolid, CapButt, JoinMiter);

	XMapWindow(dp, wid);
	XStoreName(dp, wid, "xae");

	wmHints.initial_state = NormalState;
	wmHints.icon_pixmap = pixmap;
	wmHints.flags = IconPixmapHint | StateHint;
	XSetWMHints(dp, wid, &wmHints);

	XSelectInput(dp, wid, (KeyPress | ButtonPress | ButtonRelease | Expose));
	XFlush(dp);
	Key_Get();
	virtual_lines = (int *) malloc(LINES * (sizeof(int)));
	virtual_scr = newwin(LINES, COLS, 0, 0);
	stdscr = newwin(LINES, COLS, 0, 0);
	curscr = newwin(LINES, COLS, 0, 0);
	wmove(stdscr, 0, 0);
	werase(stdscr);
	Repaint_screen = TRUE;
	initialized = TRUE;
}

void 
Key_Get()
{
	int counter;
	
	for (counter = 0; Key_strings[counter] != NULL; counter++)
		XKey_vals[counter] = XStringToKeysym(Key_strings[counter]);
}

struct _line *
Screenalloc(columns)
int columns;
{
	int i;
	struct _line *tmp;

	tmp = (struct _line *) malloc(sizeof (struct _line));
	tmp->row = malloc(columns + 1);
	tmp->attributes = malloc(columns + 1);
	tmp->prev_screen = NULL;
	tmp->next_screen = NULL;
	for (i = 0; i < columns; i++)
	{
		tmp->row[i] = ' ';
		tmp->attributes[i] = '\0';
	}
	tmp->scroll = tmp->changed = FALSE;
	tmp->row[0] = '\0';
	tmp->attributes[0] = '\0';
	tmp->row[columns] = '\0';
	tmp->attributes[columns] = '\0';
	tmp->last_char = 0;
	return(tmp);
}

WINDOW *
newwin(lines, cols, start_l, start_c)
int lines, cols;	/* number of lines and columns to be in window	*/
int start_l, start_c;	/* starting line and column to be inwindow	*/
{
	WINDOW *Ntemp;
	struct _line *temp_screen;
	int i;

	Ntemp = (WINDOW *) malloc(sizeof(WINDOW));
	Ntemp->SR = start_l;
	Ntemp->SC = start_c;
	Ntemp->Num_lines = lines;
	Ntemp->Num_cols = cols;
	Ntemp->LX = 0;
	Ntemp->LY = 0;
	Ntemp->scroll_down = Ntemp->scroll_up = 0;
	Ntemp->SCROLL_CLEAR = FALSE;
	Ntemp->Attrib = FALSE;
	Ntemp->first_line = temp_screen = Screenalloc(cols);
	Ntemp->first_line->number = 0;
	Ntemp->line_array = (struct _line **) malloc(LINES * sizeof(struct _line *));

	Ntemp->line_array[0] = Ntemp->first_line;

	for (i = 1; i < lines; i++)
	{
		temp_screen->next_screen = Screenalloc(cols);
		temp_screen->next_screen->number = i;
		temp_screen->next_screen->prev_screen = temp_screen;
		temp_screen = temp_screen->next_screen;
		Ntemp->line_array[i] = temp_screen;
	}
	Ntemp->first_line->prev_screen = NULL;
	temp_screen->next_screen = NULL;
	return(Ntemp);
}

void 
wmove(window, row, column)	/* move cursor to indicated position in window */
WINDOW *window;
int row, column;
{
	if ((row < window->Num_lines) && (column < window->Num_cols))
	{
		window->LX = column;
		window->LY = row;
	}
}

void 
clear_line(line, column, cols)
struct _line *line;
int column;
int cols;
{
	int j;

	if (column > line->last_char)
	{
		for (j = line->last_char; j < column; j++)
		{
			line->row[j] = ' ';
			line->attributes[j] = '\0';
		}
	}
	line->last_char = column;
	line->row[column] = '\0';
	line->attributes[column] = '\0';
	line->changed = TRUE;
}

void 
werase(window)			/* clear the specified window		*/
WINDOW *window;
{
	int i;
	struct _line *tmp;

	window->SCROLL_CLEAR = CLEAR;
	window->scroll_up = window->scroll_down = 0;
	for (i = 0, tmp = window->first_line; i < window->Num_lines; i++, tmp = tmp->next_screen)
		clear_line(tmp, 0, window->Num_cols);
}

void 
wclrtoeol(window)	/* erase from current cursor position to end of line */
WINDOW *window;
{
	int column, row;
	struct _line *tmp;

	window->SCROLL_CLEAR = CHANGE;
	column = window->LX;
	row = window->LY;
	for (row = 0, tmp = window->first_line; row < window->LY; row++)
		tmp = tmp->next_screen;
	clear_line(tmp, column, window->Num_cols);
}

void 
wrefresh(window)		/* flush all previous output		*/
WINDOW *window;
{
	wnoutrefresh(window);
	doupdate();
	virtual_scr->SCROLL_CLEAR = FALSE;
	virtual_scr->scroll_down = virtual_scr->scroll_up = 0;
	XFlush(dp);
}

void 
touchwin(window)
WINDOW *window;
{
	struct _line *user_line;
	int line_counter = 0;

	for (line_counter = 0, user_line = window->first_line; 
		line_counter < window->Num_lines; line_counter++)
	{
		user_line->changed = TRUE;
	}
	window->SCROLL_CLEAR = TRUE;
}

void 
wnoutrefresh(window)
WINDOW *window;
{
	struct _line *user_line;
	struct _line *virtual_line;
	int line_counter = 0;
	int user_col = 0;
	int virt_col = 0;

	cursor(virtual_scr, virtual_scr->LY, virtual_scr->LX, TRUE, FALSE);

	if (window->SR >= virtual_scr->Num_lines)
		return;
	user_line = window->first_line;
	virtual_line = virtual_scr->first_line;
	virtual_scr->SCROLL_CLEAR = window->SCROLL_CLEAR;
	virtual_scr->LX = window->LX + window->SC;
	virtual_scr->LY = window->LY + window->SR;
	virtual_scr->scroll_up = window->scroll_up;
	virtual_scr->scroll_down = window->scroll_down;
	if ((last_window_refreshed == window) && (!window->SCROLL_CLEAR))
		return;
	for (line_counter = 0; line_counter < window->SR; line_counter++)
	{
		virtual_line = virtual_line->next_screen;
	}
	for (line_counter = 0; (line_counter < window->Num_lines)
		&& ((line_counter + window->SR) < virtual_scr->Num_lines); 
			line_counter++)
	{
		if ((last_window_refreshed != window) || (user_line->changed) || ((SCROLL | CLEAR) & window->SCROLL_CLEAR))
		{
			for (user_col = 0, virt_col = window->SC; 
				(virt_col < virtual_scr->Num_cols) 
				  && (user_col < user_line->last_char); 
				  	virt_col++, user_col++)
			{
				virtual_line->row[virt_col] = user_line->row[user_col];
				virtual_line->attributes[virt_col] = user_line->attributes[user_col];
			}
			for (user_col = user_line->last_char, 
			     virt_col = window->SC + user_line->last_char; 
				(virt_col < virtual_scr->Num_cols) 
				  && (user_col < window->Num_cols); 
				  	virt_col++, user_col++)
			{
				virtual_line->row[virt_col] = ' ';
				virtual_line->attributes[virt_col] = '\0';
			}
		}
		if (virtual_scr->Num_cols != window->Num_cols)
		{
			if (virtual_line->last_char < (user_line->last_char + window->SC))
			{
				if (virtual_line->row[virtual_line->last_char] == '\0')
					virtual_line->row[virtual_line->last_char] = ' ';
				virtual_line->last_char = 
					min(virtual_scr->Num_cols, 
					  (user_line->last_char + window->SC));
			}
		}
		else
			virtual_line->last_char = user_line->last_char;
		virtual_line->row[virtual_line->last_char] = '\0';
		virtual_line->changed = user_line->changed;
		virtual_line = virtual_line->next_screen;
		user_line = user_line->next_screen;
	}
	window->SCROLL_CLEAR = FALSE;
	window->scroll_up = window->scroll_down = 0;
	last_window_refreshed = window;
}

void 
flushinp()			/* flush input				*/
{
}

int 
wgetch(window)			/* get character from specified window	*/
WINDOW *window;
{
	int status;
	int in_value;
	char temp;
	KeySym keysym_return;

	if ((status = XLookupString(keyevent, &temp, 1, &keysym_return, NULL)))
		in_value = temp;
	else if ((IsModifierKey(keysym_return)) || (IsMiscFunctionKey(keysym_return)))
	{
		in_value = 0;
	}
	else
	{
/*		key_code = XKeysymToKeycode(dp, keysym_return); */
		in_value = look_up_key(keysym_return);
	}
	return(in_value);
}

int
look_up_key(key_code)		/* decode mapping of key		*/
int key_code;
{
	int count;

	count = 0;
	while ((Key_vals[count] != 255) && (key_code != XKey_vals[count]))
		count++;
	if (XKey_vals[count] == key_code)
		return(Key_vals[count]);
	else
	{
#ifdef TEST
		wmove(stdscr, 0, 0);
		wclrtoeol(stdscr);
		wprintw(stdscr, "unknown keycode = %o", key_code);
		wrefresh(stdscr);
#endif
		return(0);
	}
}

void 
waddch(window, c)	/* output the character in the specified window	*/
WINDOW *window;
int c;
{
	int column, j;
	int shift;	/* number of spaces to shift if a tab		*/
	struct _line *tmpline;

	column = window->LX;
	if (c == '\t')
	{
		shift = (column + 1) % 8;
		if (shift == 0)
			shift++;
		else
			shift = 9 - shift;
		while (shift > 0)
		{
			shift--;
			waddch(window, ' ');
		}
	}
	else if ((!((window->LY >= (window->Num_lines - 1)) && (column >= (window->Num_cols - 1)))) && (column < window->Num_cols) && (window->LY < window->Num_lines))
	{
		if (( c != '\b') && (c != '\n') && (c != '\r'))
		{
			tmpline = window->first_line; 
			tmpline = window->line_array[window->LY];
			tmpline->changed = TRUE;
			tmpline->row[column] = c;
			tmpline->attributes[column] = window->Attrib;
			if (column >= tmpline->last_char)
			{
				if (column > tmpline->last_char)
					for (j = tmpline->last_char; j < column; j++)
					{
						tmpline->row[j] = ' ';
						tmpline->attributes[j] = '\0';
					}
				tmpline->row[column + 1] = '\0';
				tmpline->attributes[column + 1] = '\0';
				tmpline->last_char = column + 1;
			}
		}
		if (c == '\n')
		{
			wclrtoeol(window);
			window->LX = window->Num_cols;
		}
		else if (c == '\r')
			window->LX = 0;
		else if (c == '\b')
			window->LX--;
		else
			window->LX++;
	}
	if (window->LX >= window->Num_cols)
	{
		window->LX = 0;
		window->LY++;
		if (window->LY >= window->Num_lines)
		{
			window->LY = window->Num_lines - 1;
			wmove(window, 0, 0);
			wdeleteln(window);
			wmove(window, window->LY, 0);
		}
	}
	window->SCROLL_CLEAR = CHANGE;
}

void 
winsertln(window)	/* insert a blank line into the specified window */
WINDOW *window;
{
	int row, column;
	struct _line *tmp;
	struct _line *tmp1;

	window->scroll_down += 1;
	window->SCROLL_CLEAR = SCROLL;
	column = window->LX;
	row = window->LY;
	for (row = 0, tmp = window->first_line; (row < window->Num_lines) && (tmp->next_screen != NULL); row++)
		tmp = tmp->next_screen;
	if (tmp->prev_screen != NULL)
		tmp->prev_screen->next_screen = NULL;
	tmp1 = tmp;
	clear_line(tmp1, 0, window->Num_cols);
	tmp1->number = -1;
	for (row = 0, tmp = window->first_line; (row < window->LY) && (tmp->next_screen != NULL); row++)
		tmp = tmp->next_screen;
	if ((window->LY == (window->Num_lines - 1)) && (window->Num_lines > 1))
	{
		tmp1->next_screen = tmp->next_screen;
		tmp->next_screen = tmp1;
		tmp->next_screen->prev_screen = tmp;
	}
	else if (window->Num_lines > 1)
	{
		if (tmp->prev_screen != NULL)
			tmp->prev_screen->next_screen = tmp1;
		tmp1->prev_screen = tmp->prev_screen;
		tmp->prev_screen = tmp1;
		tmp1->next_screen = tmp;
		tmp->scroll = DOWN;
	}
	if (window->LY == 0)
		window->first_line = tmp1;

	for (row = 0, tmp1 = window->first_line; 
		row < window->Num_lines; row++)
	{
		window->line_array[row] = tmp1;
		tmp1 = tmp1->next_screen;
	}
}

void 
wdeleteln(window)	/* delete a line in the specified window */
WINDOW *window;
{
	int row, column;
	struct _line *tmp;
	struct _line  *tmpline;

	if (window->Num_lines > 1)
	{
		window->scroll_up += 1;
		window->SCROLL_CLEAR = SCROLL;
		column = window->LX;
		row = window->LY;
		for (row = 0, tmp = window->first_line; row < window->LY; row++)
			tmp = tmp->next_screen;
		if (window->LY == 0)
			window->first_line = tmp->next_screen;
		if (tmp->prev_screen != NULL)
			tmp->prev_screen->next_screen = tmp->next_screen;
		if (tmp->next_screen != NULL)
		{
			tmp->next_screen->scroll = UP;
			tmp->next_screen->prev_screen = tmp->prev_screen;
		}
		tmpline = tmp;
		clear_line(tmpline, 0, window->Num_cols);
		tmpline->number = -1;
		for (row = 0, tmp = window->first_line; tmp->next_screen != NULL; row++)
		{
			tmp = tmp->next_screen;
		}
		if (tmp != NULL)
		{
			tmp->next_screen = tmpline;
			tmp->next_screen->prev_screen = tmp;
			tmp = tmp->next_screen;
		}
		else
			tmp = tmpline;
		tmp->next_screen = NULL;

		for (row = 0, tmp = window->first_line; row < window->Num_lines; row++)
		{
			window->line_array[row] = tmp;
			tmp = tmp->next_screen;
		}
	}
	else
	{
		clear_line(window->first_line, 0, window->Num_cols);
	}
}

void 
wclrtobot(window)	/* delete from current position to end of the window */
WINDOW *window;
{
	int row, column;
	struct _line *tmp;

	window->SCROLL_CLEAR |= CLEAR;
	column = window->LX;
	row = window->LY;
	for (row = 0, tmp = window->first_line; row < window->LY; row++)
		tmp = tmp->next_screen;
	clear_line(tmp, column, window->Num_cols);
	for (row = (window->LY + 1); row < window->Num_lines; row++)
	{
		tmp = tmp->next_screen;
		clear_line(tmp, 0, window->Num_cols);
	}
	wmove(window, row, column);
}

void 
wstandout(window)	/* begin standout mode in window	*/
WINDOW *window;
{
	window->Attrib |= A_STANDOUT;
}

void 
wstandend(window)	/* end standout mode in window	*/
WINDOW *window;
{
	window->Attrib &= ~A_STANDOUT;
}

void 
waddstr(window, string)	/* write 'string' in window	*/
WINDOW *window;
char *string;
{
	char *wstring;

	for (wstring = string; *wstring != '\0'; wstring++)
		waddch(window, *wstring);
}

void 
clearok(window, flag)	/* erase screen and redraw at next refresh	*/
WINDOW *window;
int flag;
{
	Repaint_screen = TRUE;
	clear_win = window;
}

void 
echo()			/* turn on echoing				*/
{

}

void 
noecho()		/* turn off echoing				*/
{

}

void 
raw()			/* set to read characters immediately		*/
{
}

void 
noraw()			/* set to normal character read mode		*/
{

}

void 
nl()
{
}

void 
nonl()
{

}

void 
saveterm()
{
}

void 
fixterm()
{
}

void 
resetterm()
{
}

void 
nodelay(window, flag)
WINDOW *window;
int flag;
{
}

void 
idlok(window, flag)
WINDOW *window;
int flag;
{
}

void 
keypad(window, flag)
WINDOW *window;
int flag;
{
}

void 
savetty()		/* save current tty stats			*/
{

}

void 
resetty()		/* restore previous tty stats			*/
{

}

void 
endwin()		/* end windows					*/
{
	keypad(stdscr, FALSE);
	free(stdscr);
	initialized = FALSE;
}

void 
delwin(window)		/* delete the window structure			*/
WINDOW *window;
{
	int i;

	for (i = 1; (i < window->Num_lines) && (window->first_line->next_screen != NULL); i++)
	{
		window->first_line = window->first_line->next_screen;
		free(window->first_line->prev_screen->row);
		free(window->first_line->prev_screen->attributes);
		free(window->first_line->prev_screen);
	}
	if (window->first_line != NULL)
	{
		free(window->first_line->row);
		free(window->first_line->attributes);
		free(window->first_line);
		free(window);
	}
}

#ifndef __STDC__
void 
wprintw(va_alist)
va_dcl
#else /* __STDC__ */
void 
wprintw(WINDOW *window, const char *format, ...)
#endif /* __STDC__ */
{
#ifndef __STDC__
	WINDOW *window;
	char *format;
	va_list ap;
#else
	va_list ap;
#endif
	int value;
	char *fpoint;
	char *wtemp;

#ifndef __STDC__
	va_start(ap);
	window = va_arg(ap, WINDOW *);
	format = va_arg(ap, char *);
#else /* __STDC__ */
	va_start(ap, format);
#endif /* __STDC__ */

	fpoint = (char *) format;
	while (*fpoint != '\0')
	{
		if (*fpoint == '%')
		{
			fpoint++;
			if (*fpoint == 'd')
			{
				value = va_arg(ap, int);
				iout(window, value, 10);
			}
			else if (*fpoint == 'c')
			{
				value = va_arg(ap, int);
				waddch(window, value);
			}
			else if (*fpoint == 's')
			{
				wtemp = va_arg(ap, char *);
					waddstr(window, wtemp);
			}
			fpoint++;
		}
		else if (*fpoint == '\\')
		{
			fpoint++;
			if (*fpoint == 'n')
				waddch(window, '\n');
			else if ((*fpoint >= '0') && (*fpoint <= '9'))
			{
				value = 0;
				while ((*fpoint >= '0') && (*fpoint <= '9'))
				{
					value = (value * 8) + (*fpoint - '0');
					fpoint++;
				}
				waddch(window, value);
			}
			fpoint++;
		}
		else
			waddch(window, *fpoint++);
	}
#ifdef __STDC__
	va_end(ap);
#endif /* __STDC__ */
}

void 
iout(window, value, base)	/* output characters		*/
WINDOW *window;
int value;
int base;	/* base of number to be printed			*/
{
	int i;

	if ((i = value / base) != 0)
		iout(window, i, base);
	waddch(window, ((value % base) + '0'));
}

int 
Comp_line(line1, line2)		/* compare lines	*/
struct _line *line1;
struct _line *line2;
{
	int count1;
	int i;
	char *att1, *att2;
	char *c1, *c2;

	if (line1->last_char != line2->last_char)
		return(2);

	c1 = line1->row;
	c2 = line2->row;
	att1 = line1->attributes;
	att2 = line2->attributes;
	i = 0;
	while ((c1[i] != '\0') && (c2[i] != '\0') && (c1[i] == c2[i]) && (att1[i] == att2[i]))
		i++;
	count1 = i + 1;
	if ((count1 == 1) && (c1[i] == '\0') && (c2[i] == '\0'))
		count1 = 0;			/* both lines blank	*/
	else if ((c1[i] == '\0') && (c2[i] == '\0'))
		count1 = -1;			/* equal		*/
	else
		count1 = 1;			/* lines unequal	*/
	return(count1);
}

struct _line *
Insert_line(row, end_row, window)	/* insert line into screen */
int row;
int end_row;
WINDOW *window;
{
	int i;
	struct _line *tmp;
	struct _line *tmp1;
	int wind_y_offset;

	for (i = 0, tmp = curscr->first_line; i < window->SR; i++)
		tmp = tmp->next_screen;
	if ((end_row + window->SR) == 0)
		curscr->first_line = curscr->first_line->next_screen;
	top_of_win = tmp;
	for (i = 0, tmp = top_of_win; (tmp->next_screen != NULL) && (i < end_row); i++)
		tmp = tmp->next_screen;
	if (tmp->prev_screen != NULL)
		tmp->prev_screen->next_screen = tmp->next_screen;
	if (tmp->next_screen != NULL)
		tmp->next_screen->prev_screen = tmp->prev_screen;
	tmp1 = tmp;
	clear_line(tmp, 0, window->Num_cols);
	tmp1->number = -1;
	for (i = 0, tmp = curscr->first_line; (tmp->next_screen != NULL) && (i < window->SR); i++)
		tmp = tmp->next_screen;
	top_of_win = tmp;
	for (i = 0, tmp = top_of_win; i < row; i++)
		tmp = tmp->next_screen;

	if ((tmp->prev_screen != NULL) && (window->Num_lines > 0))
		tmp->prev_screen->next_screen = tmp1;

	tmp1->prev_screen = tmp->prev_screen;
	tmp->prev_screen = tmp1;
	tmp1->next_screen = tmp;
	if ((row + window->SR) == 0)
		curscr->first_line = tmp1;

	if (tmp1->next_screen != NULL)
		tmp1 = tmp1->next_screen;
	wind_y_offset = window->SR * fontheight;
	XCopyArea(dp, wid, wid, gc, 0, (wind_y_offset + (row * fontheight)), (COLS * fontwidth), ((end_row - row) * fontheight), 0, (wind_y_offset + ((1 + row) * fontheight)));
	XClearArea(dp, wid, 0, (wind_y_offset + (row * fontheight)), (COLS * fontwidth), fontheight, FALSE);
	XFlush(dp);

	for (i = 0, top_of_win = curscr->first_line; (top_of_win->next_screen != NULL) && (i < window->SR); i++)
		top_of_win = top_of_win->next_screen;

	return(tmp1);
}

struct _line *
Delete_line(row, end_row, window)	/* delete a line on screen */
int row;
int end_row;
WINDOW *window;
{
	int i;
	struct _line *tmp;
	struct _line *tmp1;
	struct _line *tmp2;
	int wind_y_offset;

	i = 0;
	tmp = curscr->first_line;
	while (i < window->SR)
	{
		i++;
		tmp = tmp->next_screen;
	}
	top_of_win = tmp;
	if ((row + window->SR) == 0)
		curscr->first_line = top_of_win->next_screen;
	for (i = 0, tmp = top_of_win; i < row; i++)
		tmp = tmp->next_screen;
	if (tmp->prev_screen != NULL)
		tmp->prev_screen->next_screen = tmp->next_screen;
	if (tmp->next_screen != NULL)
		tmp->next_screen->prev_screen = tmp->prev_screen;
	tmp2 = tmp->next_screen;
	tmp1 = tmp;
	clear_line(tmp1, 0, window->Num_cols);
	tmp1->number = -1;
	for (i = 0, tmp = curscr->first_line; (tmp->next_screen != NULL) && (i < window->SR); i++)
		tmp = tmp->next_screen;
	top_of_win = tmp;
	for (i = 0, tmp = top_of_win; (i < end_row) && (tmp->next_screen != NULL); i++)
		tmp = tmp->next_screen;
	tmp1->next_screen = tmp;
	tmp1->prev_screen = tmp->prev_screen;
	if (tmp1->prev_screen != NULL)
		tmp1->prev_screen->next_screen = tmp1;
	tmp->prev_screen = tmp1;
	wind_y_offset = window->SR * fontheight;
	XCopyArea(dp, wid, wid, gc, 0, (wind_y_offset + ((row + 1) * fontheight)), (COLS * fontwidth), ((end_row - row) * fontheight), 0, (wind_y_offset + (row * fontheight)));
	XClearArea(dp, wid, 0, (wind_y_offset + (end_row * fontheight)), (COLS * fontwidth), fontheight, FALSE);
	XFlush(dp);
/*	if (row == (window->Num_lines-1))
		tmp2 = tmp1;
	if ((row + window->SR) == 0)
		curscr->first_line = top_of_win = tmp2;*/

	return(tmp2);
}

void 
CLEAR_TO_EOL(window, row, column)
WINDOW *window;
int row, column;
{
	int x, y, width, height;
	struct _line *tmp1;

	for (y = 0, tmp1 = curscr->first_line; (y < (window->SR+row)) && (tmp1->next_screen != NULL); y++)
		tmp1 = tmp1->next_screen;
	for (x = column; x<window->Num_cols; x++)
	{
		tmp1->row[x] = ' ';
		tmp1->attributes[x] = '\0';
	}
	tmp1->row[column] = '\0';
	tmp1->last_char = column;
	if (column <= tmp1->last_char)
	{
		x = column * fontwidth;
		y = (row + window->SR) * fontheight;
		width = (COLS - column) * fontwidth;
		height = fontheight;
		XClearArea(dp, wid, x, y, width, height, FALSE);
	}
}


void 
doupdate()
{
	WINDOW *window;
	int similar;
	int diff;
	int begin_old, begin_new;
	int count1, j;
	int i;
	int from_top, tmp_ft, offset;
	int changed;
	int first_same;		/* first line from bottom where diffs occur */
	int last_same;		/* first line from top which is same 	    */
	int bottom;

	struct _line *curr;
	struct _line *virt;
	struct _line *old;

	struct _line *new;

	struct _line *old1, *new1;

	char *cur_lin;
	char *vrt_lin;
	char *cur_att;
	char *vrt_att;

	window = virtual_scr;

	if (Repaint_screen)
	{
		for (i = 0, curr = curscr->first_line; i < curscr->Num_lines; i++, curr = curr->next_screen)
		{
			Position(curscr, i, 0);
			for (j = 0; (curr->row[j] != '\0') && (j < curscr->Num_cols); j++)
			{
				Char_out(curr->row[j], curr->attributes[j], curr->row, curr->attributes, j);
			}
			if (STAND)
				STAND = FALSE;
		}
		Repaint_screen = FALSE;
	}

	similar = 0;
	diff = FALSE;
	top_of_win = curscr->first_line;

	for (from_top = 0, curr = top_of_win, virt = window->first_line; 
			from_top < window->Num_lines; from_top++)
	{
		virtual_lines[from_top] = TRUE;
		if ((similar = Comp_line(curr, virt)) > 0)
		{
			virtual_lines[from_top] = FALSE;
			diff = TRUE;
		}
		curr = curr->next_screen;
		virt = virt->next_screen;
	}

	from_top = 0;
	virt = window->first_line;
	curr = top_of_win;
	similar = 0;
	/*
	 |  if the window has lines that are different
	 */
	if (diff)
	{
		last_same = -1;
		changed = FALSE;
		for (first_same = window->Num_lines; 
		    (first_same > from_top) && (virtual_lines[first_same - 1]);
		     first_same--)
			;
		for (last_same = 0; 
		    (last_same < window->Num_lines) && (virtual_lines[last_same] == FALSE);
		     last_same++)
			;

		while (from_top < first_same)	
					/* check entire lines for diffs	*/
		{


			if (from_top >= last_same)
			{
				for (last_same = from_top; 
				     (last_same < window->Num_lines) && 
				     (virtual_lines[last_same] == FALSE);
				      last_same++)
					;
			}
			if (!virtual_lines[from_top])
			{
				diff = TRUE;
				/*
				 |	check for lines deleted (scroll up)
				 */
				for (tmp_ft = from_top+1, old = curr->next_screen; 
					((window->scroll_up) && (diff) && 
					(tmp_ft < last_same) && 
					(!virtual_lines[tmp_ft]));
						tmp_ft++)
				{
					if ((Comp_line(old, virt) == -1) && (!virtual_lines[from_top]))
					{
						/*
						 |	Find the bottom of the 
						 |	area that should be 
						 |	scrolled.
						 */
						for (bottom = tmp_ft, 
						     old1 = old, new1 = virt,
						     count1 = 0;
							(bottom < window->Num_lines) && 
								(Comp_line(old1, new1) <= 0);
								bottom++, old1 = old1->next_screen, 
								new1 = new1->next_screen, count1++)
							;
						if (count1 > 3)
						{
							for (offset = (tmp_ft - from_top); (offset > 0); offset--)
							{
								old = Delete_line(from_top, min((bottom - 1), (window->Num_lines - 1)), window);
								diff = FALSE;
							}
							top_of_win = curscr->first_line;
							curr = top_of_win;
							for (offset = 0; offset < from_top; offset++)
								curr = curr->next_screen;
							for (offset = from_top, old=curr, new=virt; 
							   offset < window->Num_lines; 
							   old=old->next_screen, new=new->next_screen,
							   offset++)
							{
								similar = Comp_line(old, new);
								virtual_lines[offset] = (similar > 0 ? FALSE : TRUE);
							}
						}
					}
					else
						old = old->next_screen;
				}
				/*
				 |	check for lines inserted (scroll down)
				 */
				for (tmp_ft = from_top-1, old = curr->prev_screen; 
					((window->scroll_down) && (tmp_ft >= 0) && 
					(diff) && 
					(!virtual_lines[tmp_ft])); 
					  tmp_ft--)
				{
					if (Comp_line(old, virt) == -1)
					{
						/*
						 |	Find the bottom of the 
						 |	area that should be 
						 |	scrolled.
						 */
						for (bottom = from_top, old1 = old, 
						     new1 = virt, count1 = 0;
							(bottom < window->Num_lines) && 
								(Comp_line(old1, new1) <= 0);
								bottom++, old1 = old1->next_screen, 
								new1 = new1->next_screen, 
								count1++)
							;
						if (count1 > 3)
						{
							for (offset = (from_top - tmp_ft); (offset > 0); offset--)
							{
								old = Insert_line(tmp_ft, min((bottom - 1), (window->Num_lines -1)), window);
								diff = FALSE;
							}
							top_of_win = curscr->first_line;
							curr = top_of_win;
							for (offset = 0; offset < from_top; offset++)
								curr = curr->next_screen;
							for (offset = from_top, old=curr, new=virt; 
							   offset < window->Num_lines; 
							   old=old->next_screen, new=new->next_screen,
							   offset++)
							{
								similar = Comp_line(old, new);
								virtual_lines[offset] = (similar > 0 ? FALSE : TRUE);
							}
						}
					}
					else
						old = old->prev_screen;
				}
			}
			from_top++;
			curr = curr->next_screen;
			virt = virt->next_screen;
		}
	}
	for (from_top = 0, curr = curscr->first_line; from_top < window->SR; from_top++)
		curr = curr->next_screen;
	top_of_win = curr;
	for (from_top = 0, curr = top_of_win, virt = window->first_line; from_top < window->Num_lines; from_top++, curr = curr->next_screen, virt = virt->next_screen)
	{
		if (Comp_line(curr, virt) > 0)
		{
			j = 0;
			cur_lin = curr->row;
			cur_att = curr->attributes;
			vrt_lin = virt->row;
			vrt_att = virt->attributes;
			while ((j < window->Num_cols) && (vrt_lin[j] != '\0'))
			{
				while ((cur_lin[j] == vrt_lin[j]) && (cur_att[j] == vrt_att[j]) && (j < window->Num_cols) && (vrt_lin[j] != '\0'))
					j++;
				begin_old = j;
				begin_new = j;
				if ((j < window->Num_cols) && (vrt_lin[j] != '\0'))
				{
					Position(window, from_top, begin_old);
					for (j = begin_old; (vrt_lin[j] != '\0') && (j < window->Num_cols); j++)
						Char_out(vrt_lin[j], vrt_att[j], cur_lin, cur_att, j);
				}
			}
			if ((vrt_lin[j] == '\0') && (cur_lin[j] != '\0'))
/*			if (j < curr->last_char) */
			{
				Position(window, from_top, j);
				CLEAR_TO_EOL(window, from_top, j);
			}
			if (STAND)
			{
				STAND = FALSE;
			}
			curr->last_char = virt->last_char;
			virt->number = from_top;
		}
	}

	for (count1 = 0, virt = window->first_line; count1 < window->Num_lines; count1++)
	{
		window->line_array[count1] = virt;
		virt = virt->next_screen;
	}

	cursor(window, window->LY, window->LX, FALSE, FALSE);
}

void 
Position(window, row, col)	/* position the cursor for output on the screen	*/
WINDOW *window;
int row;
int col;
{
	int pos_row;
	int pos_column;

	pos_row = row + window->SR;
	pos_column = col + window->SC;
	Curr_x = pos_column;
	Curr_y = pos_row;
}

/*
 |	the following routines are responsible for actually handling window
 |	output
 */

void 
Char_out(newc, newatt, line, attrib, offset)	/* output character with proper attribute	*/
char newc;
char newatt;
char *line;
char *attrib;
int offset;
{
	int x, y;

	if ((newatt) && (!STAND))
		STAND = TRUE;
	else if ((STAND) && (!newatt))
		STAND = FALSE;
	if (!((Curr_y >= (LINES - 1)) && (Curr_x >= (COLS - 1))))
	{
		x = Curr_x * fontwidth;
		y = Curr_y * fontheight + xaefont->ascent;
		if (!STAND)
			XDrawImageString(dp, wid, gc, x, y, &newc, 1);
		else
			XDrawImageString(dp, wid, revgc, x, y, &newc, 1);
	Curr_x++;
	line[offset] = newc;
	attrib[offset] = newatt;
	}
}

void 
draw_cursor(visible)
int visible;		/* specify if cursor is to be visible */
{
	cursor(virtual_scr, virtual_scr->LY, virtual_scr->LX, FALSE, visible);
}

void 
cursor(window, row, col, erase, visible)	/* draw the cursor at the designated position */
WINDOW *window;
int row;
int col;
int erase;	/* erase the cursor on the screen */
int visible;
{
	int x, y, y1;
	int reverse;
	int pos_row;
	int pos_column;
	static int last_row = -1;
	static int last_col = -1;
	static int last_x = -1;
	static int last_y = -1;
	struct _line *tmp1;
	char output_char;
	static int cursor_state = TRUE;

	if ((erase) && (last_row == -1))
		return;
	tmp1 = curscr->first_line;
	if (erase)
	{
		Curr_x = last_col;
		Curr_y = last_row;
		/*
		 |	make sure the previous cursor position is still within
		 |	the window, since a resize could have made the window
		 |	smaller
		 */
		if ((Curr_y >= window->Num_lines) || (Curr_x >= window->Num_cols))
			return;

		tmp1 = window->line_array[last_row];
	}
	else
	{
		pos_row = row + window->SR;
		pos_column = col + window->SC;
		last_col = Curr_x = pos_column;
		last_row = Curr_y = pos_row;

		/*
		 |	make sure the previous cursor position is still within
		 |	the window, since a resize could have made the window
		 |	smaller
		 */
		if ((Curr_y >= window->Num_lines) || (Curr_x >= window->Num_cols))
			return;

		tmp1 = window->line_array[pos_row];
	}
	output_char = tmp1->row[Curr_x];
	reverse = (tmp1->attributes[last_col] & A_STANDOUT);
	if (tmp1->row[Curr_x] == '\0')
	{
		output_char = ' ';
		reverse = 0;
	}
	x = Curr_x * fontwidth;
	y = Curr_y * fontheight + xaefont->ascent;
	y1 = (1 + Curr_y) * fontheight ;

	if (erase)
	{
		if (!reverse)
		{
			XDrawImageString(dp, wid, gc, x, y, &output_char, 1);
		}
		else
		{
			XDrawImageString(dp, wid, revgc, x, y, &output_char, 1);
		}
	}
	else
	{

		if ((last_y == last_row) && (last_x == last_col))
		{
			if ((cursor_state) && (!visible))
			{
				if (reverse)
					XDrawImageString(dp, wid, revgc, x, y, &output_char, 1);
				else
					XDrawImageString(dp, wid, gc, x, y, &output_char, 1);
			}
			else
			{
				if (reverse)
					XDrawLine(dp, wid, cursor_revgc, x, y1, x, (y1 - fontheight));
				else
					XDrawLine(dp, wid, cursor_gc,    x, y1, x, (y1 - fontheight));
			}

			cursor_state = !cursor_state;
		}
		else
		{
				XDrawLine(dp, wid, cursor_gc,    x, y1, x, (y1 - fontheight));
				cursor_state = TRUE;
		}

		if (visible)
			cursor_state = TRUE;

		last_y = last_row;
		last_x = last_col;
	}
}

