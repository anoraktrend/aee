/*
 |	windows.c
 |
 |	$Header: /home/hugh/sources/aee/RCS/windows.c,v 1.20 2010/07/18 01:01:22 hugh Exp hugh $
 */

/*
 |
 |	Copyright (c) 1986 - 1988, 1991 - 1999 Hugh Mahon.
 |
 */

#include "aee.h"

void 
new_screen()		/* draw all of the new information on the screen */
{
	int i;
	char *nchar;		/* temporary character pointer		*/
	struct bufr *tb;	/* temp buffer for storage of current stats */
	struct bufr *buff_holder;	/* tmp storage of curr_buff	*/
	int temp_mark;		/* temporary mark flag			*/
	char *name;

	if ((windows) && (num_of_bufs > 1))
	{
		buff_holder = curr_buff;
		temp_mark = mark_text;
		mark_text = FALSE;
		tb = buf_alloc();
		for (t_buff = first_buff; t_buff != NULL; t_buff = t_buff->next_buff)
		{
			werase(t_buff->footer);
			wrefresh(t_buff->footer);
			wmove(t_buff->footer, 0,0);
			wstandout(t_buff->footer);
			wprintw(t_buff->footer, "%c  ", CHNG_SYMBOL(t_buff->changed) );
			name = (t_buff->file_name != NULL ? t_buff->file_name : t_buff->name);
			for (i = 2, nchar = name; (i < COLS) && (*nchar != '\0'); i+=len_char(*nchar, i), nchar++)
				waddch(t_buff->footer, *nchar);
			for (; i < COLS; i++)
				waddch(t_buff->footer, '-');
			wstandend(t_buff->footer);
			wrefresh(t_buff->footer);
			werase(t_buff->win);
			wrefresh(t_buff->win);
			curr_buff = t_buff;
			midscreen(curr_buff->scr_vert, curr_buff->position);
			wrefresh(t_buff->win);
		}
		curr_buff = buff_holder;
		wmove(curr_buff->footer, 0, 0);
		wstandout(curr_buff->footer);
		wprintw(curr_buff->footer, "%c*", CHNG_SYMBOL(curr_buff->changed));
		wstandend(curr_buff->footer);
		wrefresh(curr_buff->footer);
		free(tb);
		mark_text = temp_mark;
		if (mark_text)
		{
			midscreen(curr_buff->scr_vert, curr_buff->position);
		}
	}
	else
	{
		werase(curr_buff->win);
		wrefresh(curr_buff->win);
		midscreen(curr_buff->scr_vert, curr_buff->position);
	}
	wrefresh(curr_buff->win);
}

struct bufr *
add_buf(ident)		/* add a new buffer to the list */
char *ident;
{
	struct bufr *t;			/* temporary pointer		*/

	t = NULL;
	if (windows && (num_of_bufs >= ((LINES - 1)/2)))
	{
		no_windows();
	}
	if ((windows) && (num_of_bufs > 1))
	{
		wmove(curr_buff->footer, 0, 0);
		wstandout(curr_buff->footer);
		wprintw(curr_buff->footer, "%c ", CHNG_SYMBOL(curr_buff->changed) );
		wstandend(curr_buff->footer);
		wrefresh(curr_buff->footer);
	}
	t = first_buff;
	while (t->next_buff != NULL)
		t = t->next_buff;
	curr_buff = t->next_buff = buf_alloc();
	curr_buff->num_of_lines = 1;
	curr_buff->absolute_lin = 1;
	t = t->next_buff;
	t->name = xalloc(strlen(ident) + 1);
	copy_str(ident, t->name);
	curr_buff->curr_line = curr_buff->first_line = txtalloc();
	t->next_buff = NULL;
	curr_buff->pointer = curr_buff->curr_line->line = curr_buff->first_line->line = xalloc(10);
	*t->first_line->line = '\0';
	t->first_line->vert_len = t->first_line->line_number = t->first_line->line_length = 1;
	t->first_line->max_length = 10;
	t->first_line->prev_line = NULL;
	t->first_line->next_line = NULL;
	t->footer = NULL;
	t->position = 1;
	curr_buff->scr_pos = curr_buff->abs_pos = curr_buff->scr_vert = curr_buff->scr_horz = 0;
	t->win = NULL;
	num_of_bufs++;
	if (!windows)
	{
		t->window_top = first_buff->window_top;
		t->lines = first_buff->lines;
		t->last_line = first_buff->last_line;
		t->last_col = first_buff->last_col;
		t->win = first_buff->win;
	}
	else
		redo_win();
	new_screen();
	return(t);
}

void 
chng_buf(name)	/* move to the named buffer, if it doesn't exist, create it */
char *name;
{
	struct bufr *tmp;
	char *test;

	clr_cmd_line = TRUE;
	test = next_word(name);
	if ((*test == '\0') && (!mark_text))
	{
		wrefresh(curr_buff->win);

		if (curr_buff->curr_line->changed && curr_buff->journalling)
			write_journal(curr_buff, curr_buff->curr_line);

		tmp = first_buff;
		while ((tmp != NULL) && (strcmp(name, tmp->name)))
			tmp = tmp->next_buff;
		if (tmp == NULL)
			tmp = add_buf(name);
		else
		{
			if ((windows) && (num_of_bufs > 1))
			{
				wmove(curr_buff->footer, 0, 0);
				wstandout(curr_buff->footer);
				wprintw(curr_buff->footer, "%c ", CHNG_SYMBOL(curr_buff->changed));
				wstandend(curr_buff->footer);
				wrefresh(curr_buff->footer);
			}
			curr_buff = tmp;
			if (!windows)
				new_screen();
			else if (num_of_bufs > 1)
			{
				wmove(curr_buff->footer, 0, 0);
				wstandout(curr_buff->footer);
				wprintw(curr_buff->footer, "%c*", CHNG_SYMBOL(curr_buff->changed));
				wstandend(curr_buff->footer);
				wrefresh(curr_buff->footer);
			}
			wmove(com_win,0,0);
			wclrtoeol(com_win);
			wprintw(com_win, buff_msg, curr_buff->file_name != NULL ? 
			           curr_buff->file_name : curr_buff->name);
			wrefresh(com_win);
			wmove(curr_buff->win, curr_buff->scr_vert, curr_buff->scr_horz);
		}
	}
	else if (mark_text)
	{
		wmove(com_win,0,0);
		wclrtoeol(com_win);
		wprintw(com_win, cant_chng_while_mark_msg);
		wrefresh(com_win);
		wmove(curr_buff->win, curr_buff->scr_vert, curr_buff->scr_horz);
	}
	else
	{
		wmove(com_win,0,0);
		wclrtoeol(com_win);
		wprintw(com_win, too_many_parms_msg);
		wrefresh(com_win);
		wmove(curr_buff->win, curr_buff->scr_vert, curr_buff->scr_horz);
	}
}

int 
del_buf()			/* delete current buffer		*/
{
	struct bufr *garb;
	int choice;

	if (!mark_text)
	{
		if ((curr_buff->edit_buffer) && (curr_buff->changed))
		{
			choice = menu_op(del_buff_menu);
			if ((choice == 0) || (choice == 2))
				return(FALSE);

			if ((choice == 1) && (!file_write_success()))
				return(FALSE);
		}
		if (curr_buff->journalling)
			/* if writing to recover file, delete it */
		{
			remove_journal_file(curr_buff);
		}

		curr_buff->curr_line = curr_buff->first_line->next_line;
		while (curr_buff->curr_line != NULL)
		{
			free(curr_buff->curr_line->prev_line->line);
			free(curr_buff->curr_line->prev_line);
			curr_buff->curr_line = curr_buff->curr_line->next_line;
		}
		garb = first_buff;
		while (garb->next_buff != curr_buff)
			garb = garb->next_buff;
		garb->next_buff = curr_buff->next_buff;
		if (windows)
		{
			werase(curr_buff->win);
			wrefresh(curr_buff->win);
			delwin(curr_buff->win);
			werase(curr_buff->footer);
			wrefresh(curr_buff->footer);
			delwin(curr_buff->footer);
			curr_buff->footer = NULL;
		}
		free(curr_buff);
		num_of_bufs--;
		if (windows)
			redo_win();
		curr_buff = first_buff;
		new_screen();
		wmove(com_win,0,0);
		wclrtoeol(com_win);
		wprintw(com_win, buff_is_main_msg);
		wrefresh(com_win);
		wmove(curr_buff->win, curr_buff->scr_vert, curr_buff->scr_horz);
		clr_cmd_line = TRUE;
	}
	else
	{
		wmove(com_win, 0, 0);
		werase(com_win);
		wprintw(com_win, cant_del_while_mark);
		wrefresh(com_win);
		return(FALSE);
	}
	return(TRUE);
}

void 
redo_win()	/* adjust windows to allow for new/deleted windows	*/
{
	int total_lines;
	int num_of_lines;		/* number of lines in windows	*/
	int counter;			/* temporary counter		*/
	int iter;			/* iteration counter		*/
	int remainder;			/* remainder of lines		*/
	int temp_mark;			/* temporary mark flag		*/
	int offset;			/* offset from top		*/
	struct bufr *tt;		/* temporary pointer		*/
	struct text *tmp_tx;		/* temporary text pointer	*/

	if (window_resize)
	{
		werase(com_win);
		delwin(com_win);
		com_win = newwin(1, COLS, LINES-1, 0);
		curr_buff->last_col = COLS - 1;
	}

	if (!windows)
	{
		if ((info_window) && (info_win == 0))
		{
			delwin(curr_buff->win);
			info_win = newwin(info_win_height, COLS, 0, 0);
			paint_information();
			first_buff->win = curr_buff->win = newwin(LINES - info_win_height - 1, COLS, info_win_height, 0);
			first_buff->last_line = LINES - info_win_height - 2;
			first_buff->lines = LINES - info_win_height - 1;
			first_buff->window_top = info_win_height;
		}
		else if ((!info_window) && (info_win != 0))
		{
			delwin(curr_buff->win);
			delwin(info_win);
			first_buff->last_line = LINES - 2;
			first_buff->lines = LINES - 1;
			first_buff->win = curr_buff->win = newwin(LINES - 1, COLS, 0, 0);
			first_buff->window_top = 0;
			info_win = FALSE;
		}
		else if ((info_window) && (window_resize))
		{
			delwin(info_win);
			delwin(curr_buff->win);
			info_win = newwin(info_win_height, COLS, 0, 0);
			paint_information();
			first_buff->win = curr_buff->win = newwin(LINES - info_win_height - 1, COLS, info_win_height, 0);
			first_buff->last_line = LINES - info_win_height - 2;
			first_buff->lines = LINES - info_win_height - 1;
		}
		else
		{
			offset = 0;
			if (info_window)
			{
				delwin(info_win);
				info_win = newwin(info_win_height, COLS, 0, 0);
				paint_information();
				offset = info_win_height;
			}
			delwin(curr_buff->win);
			first_buff->win = curr_buff->win = newwin(LINES - 1, COLS, offset, 0);
			first_buff->last_line = LINES - 2;
			first_buff->lines = LINES - 1;
		}

		first_buff->last_col = COLS - 1;

		if (num_of_bufs > 1)
		{
			tt = first_buff->next_buff;
			while (tt != NULL)
			{
				tt->lines = first_buff->lines;
				tt->last_line = first_buff->last_line;
				tt->last_col = first_buff->last_col;
				tt->win = first_buff->win;
				tt = tt->next_buff;
			}
		}
		tt = first_buff;
		while (tt != NULL)
		{
			/*
			 |	if a resize, re-calculate the 
			 |	vertical length lines of text
			 */
			if (local_COLS != COLS)
			{
				for (tmp_tx = tt->first_line; tmp_tx != NULL; 
				        tmp_tx = tmp_tx->next_line)
				{
					tmp_tx->vert_len = (scanline(tmp_tx, 
					     tmp_tx->line_length) / COLS) + 1;
				}
			}
			tt->window_top = first_buff->window_top;
			tt = tt->next_buff;
		}
		window_resize = FALSE;
		return;
	}

	temp_mark = mark_text;
	mark_text = FALSE;

	{
		if ((!info_window) && (info_win != 0))
		{
			delwin(info_win);
			info_win = 0;
		}
		else if ((info_window) && (info_win == 0))
		{
			info_win = newwin(info_win_height, COLS, 0, 0);
			paint_information();
		}
		else if ((info_window) && (window_resize))
		{
			delwin(info_win);
			info_win = newwin(info_win_height, COLS, 0, 0);
			paint_information();
		}
		else if (info_window)
		{
			delwin(info_win);
			info_win = newwin(info_win_height, COLS, 0, 0);
			paint_information();
		}

		if (info_window)
			total_lines = (LINES - 1) - info_win_height;
		else
			total_lines = LINES - 1;

		num_of_lines = total_lines/num_of_bufs;
		remainder = total_lines % num_of_bufs;
		if (info_window)
			counter = info_win_height;
		else
			counter = 0;

		for (iter=0, tt=first_buff; (tt != NULL) ; iter++)
		{
			tt->last_col = COLS - 1;
			tt->lines = num_of_lines;
			if (remainder > 0)
			{
				tt->lines++;
				remainder--;
			}
			if (tt->win != NULL)
			{
				werase(tt->win);
				wrefresh(tt->win);
				delwin(tt->win);
				tt->win = NULL;
			}
			if (num_of_bufs > 1)
				tt->lines--;
			if (tt->scr_vert > (tt->lines - 1))
				tt->scr_vert = tt->lines - 1;
			/*
			 |	if a resize, re-calculate the vertical length 
			 |	lines of text
			 */
			if (local_COLS != COLS)
			{
				for (tmp_tx = tt->first_line; tmp_tx != NULL; 
				     tmp_tx = tmp_tx->next_line)
				{
					tmp_tx->vert_len = (scanline(tmp_tx, 
					     tmp_tx->line_length) / COLS) + 1;
					
				}
			}
			tt->last_line = tt->lines - 1;
			tt->win = newwin(tt->lines,COLS,counter,0);
			tt->window_top = counter;
			counter += tt->lines;
			if (tt->footer != NULL)
			{
				werase(tt->footer);
				wrefresh(tt->footer);
				delwin(tt->footer);
				tt->footer = NULL;
			}
			if (num_of_bufs > 1)
			{
				tt->footer = newwin(1, COLS, counter, 0);
				counter++;
			}
			keypad(tt->win, TRUE);
			nodelay(tt->win, FALSE);
			idlok(tt->win, TRUE);
			tt = tt->next_buff;
		}
	}
	mark_text = temp_mark;
	window_resize = FALSE;
}

void 
resize_check()
{
	/*
	 |	if screen size has not changed, return immediately
	 */

	if ((LINES == local_LINES) && (COLS == local_COLS))
		return;

	/*
	 |	if the screen size has changed, save buffer information, 
	 |	resize windows accordingly, save new size, then repaint screen
	 */

	window_resize = TRUE;
	clearok(stdscr, TRUE);
	redo_win();
	local_LINES = LINES;
	local_COLS = COLS;
	curr_buff->last_line = curr_buff->lines - 1;

	/*
	 |	repaint the screen with all buffers (if applicable)
	 */
	new_screen();

	if (info_win != NULL)
		paint_info_win();
}

void 
set_up_term()		/* set up the terminal for operating with ae	*/
{
#ifndef XAE
	initscr();
#else
	initscr(win_width, win_height);
	event_init();
#endif
	savetty();
	saveterm();
	noecho();
	raw();
	nonl();
	local_LINES = LINES;
	local_COLS = COLS;
}
