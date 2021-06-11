/*
 |	motion.c
 |
 |	$Header: /home/hugh/sources/aee/RCS/motion.c,v 1.16 1996/04/25 23:44:41 hugh Exp hugh $
 */

/*
 |	Copyright (c) 1986 - 1988, 1991 - 1996 Hugh Mahon.
 */

#include "aee.h"

void 
bottom()			/* go to bottom of file			*/
{
	struct text *temp_line;
	int num_lines;

	num_lines = 0;
	if (curr_buff->position > 1)
		adv_line();
	temp_line = curr_buff->curr_line;
	while (temp_line->next_line != NULL)
	{
		num_lines++;
		temp_line = temp_line->next_line;
	}
	move_rel("d", num_lines);
}

void 
top()				/* go to top of file			*/
{
	struct text *temp_line;
	int num_lines;

	temp_line = curr_buff->curr_line;
	num_lines = 0;
	if (curr_buff->position > 1)
		bol();
	while (temp_line->prev_line != NULL)
	{
		num_lines++;
		temp_line = temp_line->prev_line;
	}
	move_rel("u", num_lines);
}

void 
nextline()			/* move pointers to start of next line	*/
{
	int n;
	int nn;
	int nvert;
	char *npoint;

	n = curr_buff->curr_line->vert_len - (curr_buff->scr_pos / COLS);
	curr_buff->curr_line = curr_buff->curr_line->next_line;
	curr_buff->pointer = curr_buff->curr_line->line;
	curr_buff->position = 1;
	curr_buff->abs_pos = curr_buff->scr_pos = curr_buff->scr_horz = 0;
	if ((curr_buff->scr_vert + n) > curr_buff->last_line)
	{
		n = (curr_buff->scr_vert + n) - curr_buff->last_line;
		if (n < curr_buff->last_line)
		{
			wmove(curr_buff->win, 0, 0);
			for (value = 0; value < n; value++)
				wdeleteln(curr_buff->win);
			nvert = curr_buff->last_line - curr_buff->curr_line->prev_line->vert_len;
			nn = 0;
			npoint = curr_buff->curr_line->prev_line->line;
			curr_buff->scr_vert = curr_buff->last_line;
			if ((!mark_text) && ((n > 1) || (curr_buff->curr_line->prev_line->vert_len > 1)))
				draw_line(nvert, nn, npoint, 1, curr_buff->curr_line->prev_line);
			draw_line(curr_buff->scr_vert, curr_buff->scr_pos, curr_buff->pointer, 1, curr_buff->curr_line);
		}
		else
			midscreen(curr_buff->last_line, curr_buff->position);
	}
	else
		curr_buff->scr_vert += n;
}

void 
prevline()		/* move pointers to end of previous line	*/
{
	int temp_mark;	/* if mark_text must be set to FALSE, store original value here	*/
	int p;
	int pvert;
	int ppos;
	char *pchar;

	p = curr_buff->curr_line->prev_line->vert_len + (curr_buff->scr_pos / COLS);
	curr_buff->curr_line = curr_buff->curr_line->prev_line;
	curr_buff->pointer = curr_buff->curr_line->line;
	curr_buff->position = 1;
	curr_buff->scr_pos = 0;
	if ((curr_buff->scr_vert - p) < 0)
	{
		wmove(curr_buff->win, 0, 0);
		if (p < curr_buff->last_line)
		{
			p = p - curr_buff->scr_vert;
			for (value = 0; value < p; value++)
				winsertln(curr_buff->win);
			draw_line(0, 0, curr_buff->pointer, 1, curr_buff->curr_line);
			curr_buff->scr_vert = 0;
			if ((mark_text) && (cpste_line->prev_line != NULL))
			{
/*				wrefresh(curr_buff->win);*/
				pvert = 0;
				ppos = curr_buff->position;
				p = 0;
				pchar = curr_buff->pointer;
				if (cpste_line->prev_line->line_length != curr_buff->curr_line->line_length)
				{
					ppos += curr_buff->curr_line->line_length - cpste_line->prev_line->line_length;
					p = scanline(curr_buff->curr_line, ppos);
					pvert = p / COLS;
					p %= COLS;
					for (value = 1; value < ppos; value++)
						pchar++;
				}
				temp_mark = mark_text;
				mark_text = FALSE;
				wstandout(curr_buff->win);
				draw_line(pvert, p, pchar, ppos, curr_buff->curr_line);
				mark_text = temp_mark;
			}
			else if (((p - curr_buff->curr_line->vert_len) > 0) && (mark_text))
			{
				curr_buff->pointer = curr_buff->curr_line->next_line->line;
				draw_line(curr_buff->curr_line->vert_len, 0, curr_buff->curr_line->next_line->line, 1, curr_buff->curr_line->next_line);
				curr_buff->pointer = curr_buff->curr_line->line;
			}
			else if ((p - curr_buff->curr_line->vert_len) > 0)
				draw_line(curr_buff->curr_line->vert_len, 0, curr_buff->curr_line->next_line->line, 1, curr_buff->curr_line->next_line);
		}
		else
			midscreen(curr_buff->last_line/2, curr_buff->position);
	}
	else
		curr_buff->scr_vert -= p;
	while (curr_buff->position < curr_buff->curr_line->line_length)
	{
		curr_buff->position++;
		curr_buff->pointer++;
	}
	curr_buff->abs_pos = curr_buff->scr_pos = scanline(curr_buff->curr_line, curr_buff->position);
}

void 
left(disp)		/* move cursor left one character	*/
int disp;
{
	int l;
	int ll;
	char *lpoint;

	if (curr_buff->position != 1)		/* if not at begin of line	*/
	{
		ll = curr_buff->scr_pos;
		curr_buff->pointer--;
		curr_buff->position--;
		if (*curr_buff->pointer == '\t')
			l = scanline(curr_buff->curr_line, curr_buff->position);
		else
			l = curr_buff->scr_pos - len_char(*curr_buff->pointer, curr_buff->scr_pos);
		if ((l / COLS) < (ll / COLS))
		{
			curr_buff->scr_vert--;
			if (curr_buff->scr_vert < 0)
			{
				wmove(curr_buff->win, 0, 0);
				winsertln(curr_buff->win);
				lpoint = curr_buff->curr_line->line;
				curr_buff->scr_vert = 0;
				curr_buff->scr_vert -= l / COLS;
				draw_line(curr_buff->scr_vert, 0, lpoint, 1, curr_buff->curr_line);
				curr_buff->scr_vert = 0;
			}
		}
		curr_buff->abs_pos = curr_buff->scr_pos = l;
		curr_buff->scr_horz = curr_buff->scr_pos % COLS;
		wmove(curr_buff->win, curr_buff->scr_vert, curr_buff->scr_horz);
		if (mark_text)
			slct_left();
	}
	else if (curr_buff->curr_line->prev_line != NULL)
	{
		curr_buff->position = 1;

		if (curr_buff->curr_line->changed && curr_buff->journalling)
			write_journal(curr_buff, curr_buff->curr_line);

		if (disp)
		{
			prevline();
			curr_buff->scr_horz = curr_buff->scr_pos % COLS;
			curr_buff->scr_vert += curr_buff->scr_pos / COLS;
			if (curr_buff->scr_vert > curr_buff->last_line)
			{
				l = curr_buff->scr_vert - curr_buff->last_line;
				if (l < curr_buff->last_line)
				{
					curr_buff->scr_vert -= l;
					wmove(curr_buff->win, 0, 0);
					while (l > 0)
					{
						wdeleteln(curr_buff->win);
						l--;
					}
					l = 1 + curr_buff->scr_vert - curr_buff->curr_line->vert_len;
					draw_line(l, 0, curr_buff->curr_line->line, 1, curr_buff->curr_line);
				}
				else
					midscreen((curr_buff->last_line / 2), curr_buff->position);
			}
			wmove(curr_buff->win, curr_buff->scr_vert, curr_buff->scr_horz);
			if (mark_text)
				slct_line("u");
		}
		else
		{
			curr_buff->curr_line = curr_buff->curr_line->prev_line;
			curr_buff->pointer = curr_buff->curr_line->line;
			while (curr_buff->position < curr_buff->curr_line->line_length)
			{
				curr_buff->pointer++;
				curr_buff->position++;
			}
			curr_buff->abs_pos = curr_buff->scr_pos = scanline(curr_buff->curr_line, curr_buff->position);
		}
		curr_buff->absolute_lin--;
	}
	else if (curr_buff->curr_line->prev_line == NULL)
	{
		wmove(com_win, 0,0);
		werase(com_win);
		wprintw(com_win, left_err_msg);
		wrefresh(com_win);
		clr_cmd_line = TRUE;
	}
}

int 
right(disp)		/* move cursor right one character	*/
int disp;
{
	int r1;
	int r;
	char temp;
	int successful = FALSE;

	temp = *curr_buff->pointer;
	if (curr_buff->position < curr_buff->curr_line->line_length)
	{
		successful = TRUE;
		if (mark_text)
			slct_right();
		r1 = curr_buff->scr_pos;
		r = r1 + len_char(*curr_buff->pointer, curr_buff->scr_pos);
		curr_buff->pointer++;
		curr_buff->position++;
		if ((r / COLS) > (r1 / COLS))
		{
				curr_buff->scr_vert++;
			if (curr_buff->scr_vert > curr_buff->last_line)
			{
				curr_buff->scr_vert--;
				r1 = curr_buff->scr_vert - (r / COLS);
				wmove(curr_buff->win, 0, 0);
				wdeleteln(curr_buff->win);
				wmove(curr_buff->win, (curr_buff->last_line-1), curr_buff->scr_horz);
				if (mark_text)
					wstandout(curr_buff->win);
				if ((temp > 31) && (temp < 127))
					waddch(curr_buff->win, temp);
				else
					value = out_char(curr_buff->win, temp, r, (curr_buff->last_line-1), 0);
				if (mark_text)
					wstandend(curr_buff->win);
				curr_buff->scr_horz = r % COLS;
				draw_line(curr_buff->scr_vert, r, curr_buff->pointer, curr_buff->position, curr_buff->curr_line);
			}
		}
		curr_buff->abs_pos = curr_buff->scr_pos = r;
		curr_buff->scr_horz = curr_buff->scr_pos % COLS;
		wmove(curr_buff->win, curr_buff->scr_vert, curr_buff->scr_horz);
	}
	else if (curr_buff->curr_line->next_line != NULL)
	{
		if (curr_buff->curr_line->changed && curr_buff->journalling)
			write_journal(curr_buff, curr_buff->curr_line);

		successful = TRUE;
		if (disp)
		{
			if (mark_text)
				slct_line("d");
			nextline();
			wmove(curr_buff->win, curr_buff->scr_vert, curr_buff->scr_horz);
		}
		else
		{
			curr_buff->curr_line = curr_buff->curr_line->next_line;
			curr_buff->pointer = curr_buff->curr_line->line;
			curr_buff->abs_pos = curr_buff->scr_pos = 0;
		}
		curr_buff->position = 1;	
		curr_buff->absolute_lin++;
	}
	else if (curr_buff->curr_line->next_line == NULL)
	{
		wmove(com_win, 0, 0);
		werase(com_win);
		wprintw(com_win, right_err_msg);
		wrefresh(com_win);
		clr_cmd_line = TRUE;
	}
	return(successful);
}

void 
find_pos()	/* move to the same column as on line previously occupied */
{
	curr_buff->scr_horz = 0;
	curr_buff->position = 1;
	while ((curr_buff->scr_horz < curr_buff->abs_pos) && (curr_buff->position < curr_buff->curr_line->line_length))
	{
		curr_buff->scr_horz += len_char(*curr_buff->pointer, curr_buff->scr_horz);
		curr_buff->position++;
		curr_buff->pointer++;
	}
}

void 
up()		/* move cursor up one line in the same column	*/
{
	int tscr_pos;

	tscr_pos = curr_buff->abs_pos;
	if (curr_buff->curr_line->prev_line != NULL)
	{
		if (curr_buff->curr_line->changed && curr_buff->journalling)
			write_journal(curr_buff, curr_buff->curr_line);

		if (mark_text)
		{
			while (curr_buff->position > 1)
				left(TRUE);
			left(TRUE);
			while (curr_buff->scr_pos > tscr_pos)
				left(TRUE);
			if ((curr_buff->scr_pos < tscr_pos) && (curr_buff->position < curr_buff->curr_line->line_length))
				right(TRUE);
		}
		else
		{
			prevline();
			curr_buff->pointer = curr_buff->curr_line->line;
			curr_buff->position = 1;
			curr_buff->scr_horz = 0;
			curr_buff->abs_pos = tscr_pos;
			find_pos();
			curr_buff->scr_pos = curr_buff->scr_horz;
			curr_buff->scr_horz = curr_buff->scr_pos % COLS;
			if (((curr_buff->scr_pos / COLS) + curr_buff->scr_vert) > curr_buff->last_line)
				midscreen(curr_buff->last_line/2, curr_buff->position);
			else
				curr_buff->scr_vert += (curr_buff->scr_pos / COLS);
		curr_buff->absolute_lin--;
		}
	}
	else if (curr_buff->curr_line->prev_line == NULL)
	{
		wmove(com_win, 0,0);
		werase(com_win);
		wprintw(com_win, left_err_msg);
		wrefresh(com_win);
		clr_cmd_line = TRUE;
	}
	curr_buff->abs_pos = tscr_pos;
	wmove(curr_buff->win, curr_buff->scr_vert, curr_buff->scr_horz);
}

void 
down()			/* move cursor down one line in the same column	*/
{
	int d;
	int tscr_pos;

	tscr_pos = curr_buff->abs_pos;
	if (curr_buff->curr_line->next_line != NULL)
	{
		if (curr_buff->curr_line->changed && curr_buff->journalling)
			write_journal(curr_buff, curr_buff->curr_line);

		if (mark_text)
		{
			d = curr_buff->scr_vert + (curr_buff->curr_line->vert_len - (curr_buff->scr_pos / COLS));
			while (curr_buff->position < curr_buff->curr_line->line_length)
				right(TRUE);
			right(TRUE);
			while ((curr_buff->scr_pos < tscr_pos) && (curr_buff->position < curr_buff->curr_line->line_length))
				right(TRUE);
		}
		else
		{
			nextline();
			curr_buff->abs_pos = tscr_pos;
			find_pos();
			curr_buff->scr_pos = curr_buff->scr_horz;
			curr_buff->scr_horz = curr_buff->scr_pos % COLS;
			if (((curr_buff->scr_pos / COLS)+curr_buff->scr_vert) > curr_buff->last_line)
			{
				d = (curr_buff->scr_vert + (curr_buff->scr_pos / COLS)) - curr_buff->last_line;
				if (d > curr_buff->last_line)
					midscreen(curr_buff->last_line/2, curr_buff->position);
				else
				{
					curr_buff->scr_vert += (curr_buff->scr_pos / COLS) - d;
					wmove(curr_buff->win, 0,0);
					while (d > 0)
					{
						wdeleteln(curr_buff->win);
						d--;
					}
					draw_line((curr_buff->scr_vert-(curr_buff->scr_pos/COLS)), 0, curr_buff->curr_line->line, 1, curr_buff->curr_line);
				}
			}
			else
				curr_buff->scr_vert += curr_buff->scr_pos / COLS;
			curr_buff->absolute_lin++;
		}
	}
	else if (curr_buff->curr_line->next_line == NULL)
	{
		wmove(com_win, 0,0);
		werase(com_win);
		wprintw(com_win, right_err_msg);
		wrefresh(com_win);
		clr_cmd_line = TRUE;
	}
	curr_buff->abs_pos = tscr_pos;
	wmove(curr_buff->win, curr_buff->scr_vert, curr_buff->scr_horz);
}

void 
adv_word()	/* advance to next word in text		*/
{
	if (curr_buff->position != curr_buff->curr_line->line_length)
	{
		while ((curr_buff->position < curr_buff->curr_line->line_length) && ((*curr_buff->pointer != ' ') && (*curr_buff->pointer != '\t')))
			right(TRUE);
		while ((curr_buff->position < curr_buff->curr_line->line_length) && ((*curr_buff->pointer == ' ') || (*curr_buff->pointer == '\t')))
			right(TRUE);
	}
	else
		right(TRUE);
}

void 
prev_word()	/* move to start of previous word in text	*/
{
	if (curr_buff->position != 1)
	{
		if ((curr_buff->position != 1) && ((curr_buff->pointer[-1] == ' ') || (curr_buff->pointer[-1] == '\t')))
		{	/* if at the start of a word	*/
			while ((curr_buff->position != 1) && ((*curr_buff->pointer != ' ') && (*curr_buff->pointer != '\t')))
				left(TRUE);
		}
		while ((curr_buff->position != 1) && ((*curr_buff->pointer == ' ') || (*curr_buff->pointer == '\t')))
			left(TRUE);
		while ((curr_buff->position != 1) && ((*curr_buff->pointer != ' ') && (*curr_buff->pointer != '\t')))
			left(TRUE);
		if ((curr_buff->position != 1) && ((*curr_buff->pointer == ' ') || (*curr_buff->pointer == '\t')))
			right(TRUE);
	}
	else
		left(TRUE);
}

void 
move_rel(direction, lines)	/* move relative # of lines to current line */
char *direction;
int lines;
{
	struct text *tmp_paste;
	struct text *tmp_line;
	int i;
	int j;
	int m;
	int slct_flag;
	char *tmp;

	if ((lines > curr_buff->last_line) && (mark_text))
	{
		if (*direction == 'u')
		{
			i = 0;
			while ((i<lines) && (curr_buff->curr_line->prev_line != NULL))
			{
				fast_line("u");
				i++;
			}
			while (curr_buff->position > 1)
				fast_left();
		}
		else
		{
			for (i=0; (i < lines) && (curr_buff->curr_line->next_line != NULL); i++)
				fast_line("d");
		}
		curr_buff->pointer = curr_buff->curr_line->line;
		curr_buff->abs_pos = curr_buff->scr_pos = curr_buff->scr_horz = 0;
		curr_buff->position = 1;
		if (curr_buff->curr_line->next_line == NULL)
			midscreen(curr_buff->last_line, curr_buff->position);
		else
			midscreen(curr_buff->last_line/2, curr_buff->position);
	}
	else if ((lines > curr_buff->last_line) && (!mark_text))
	{
		if (*direction == 'u')
		{
			i = 0;

			if (curr_buff->curr_line->changed && curr_buff->journalling)
				write_journal(curr_buff, curr_buff->curr_line);

			while ((i<lines) && (curr_buff->curr_line->prev_line != NULL))
			{
				curr_buff->curr_line = curr_buff->curr_line->prev_line;
				i++;
			}
			curr_buff->absolute_lin -= i;
		}
		else
		{
			i = 0;

			if (curr_buff->curr_line->changed && curr_buff->journalling)
				write_journal(curr_buff, curr_buff->curr_line);

			while ((i<lines) && (curr_buff->curr_line->next_line != NULL))
			{
				curr_buff->curr_line = curr_buff->curr_line->next_line;
				i++;
			}
			curr_buff->absolute_lin += i;
		}
		curr_buff->pointer = curr_buff->curr_line->line;
		curr_buff->abs_pos = curr_buff->scr_pos = curr_buff->scr_horz = 0;
		curr_buff->position = 1;
		if (curr_buff->curr_line->next_line == NULL)
			midscreen(curr_buff->last_line, curr_buff->position);
		else
			midscreen(curr_buff->last_line/2, curr_buff->position);
	}
	else
	{
		if (*direction == 'u')
		{
			while (curr_buff->position != 1)
				left(TRUE);
			curr_buff->abs_pos = curr_buff->scr_pos = curr_buff->scr_horz;
			for (i = 0; (i < lines) && (curr_buff->curr_line->prev_line != NULL); i++)
				up();
			if ((curr_buff->last_line > 5) && ( curr_buff->scr_vert < 4))
			{
				tmp = curr_buff->pointer;
				tmp_line = curr_buff->curr_line;
				slct_flag = mark_text;
				mark_text = FALSE;
				tmp_paste = cpste_line;
				i  = j = 0;
				m = curr_buff->scr_vert;
				while ((i < 5) && (tmp_line->prev_line != NULL))
				{
					tmp_line = tmp_line->prev_line;
					i += tmp_line->vert_len;
					j++;
					m -=tmp_line->vert_len;
				}
				if ((curr_buff->scr_vert - m) > 5)
					m = curr_buff->scr_vert - 5;
				if (i > 5)
					curr_buff->scr_vert = 5;
				else
					curr_buff->scr_vert = i;
				i = -m;
				wmove(curr_buff->win, 0,0);
				for (value=0; value<i; value++)
					winsertln(curr_buff->win);
				i = curr_buff->scr_vert;
				tmp_line = curr_buff->curr_line;
				for (value=0; value<j; value++)
				{
					curr_buff->curr_line = curr_buff->curr_line->prev_line;
					curr_buff->pointer = curr_buff->curr_line->line;
					i -= curr_buff->curr_line->vert_len;
					m += curr_buff->curr_line->vert_len;
					if ((slct_flag) && (cpste_line->prev_line != NULL))
					{
						cpste_line = cpste_line->prev_line;
						wstandout(curr_buff->win);
					}
					draw_line(i, 0, curr_buff->pointer, 1, curr_buff->curr_line);
					wstandend(curr_buff->win);
				}
				curr_buff->curr_line = tmp_line;
				cpste_line = tmp_paste;
				curr_buff->pointer = tmp;
				mark_text = slct_flag;
				curr_buff->scr_horz = scanline(curr_buff->curr_line, curr_buff->position);
			}
		}
		else
		{
			adv_line();
			for (i = 1; (i < lines) && (curr_buff->curr_line->next_line != NULL); i++)
				down();
			if ((curr_buff->last_line > 10) && (curr_buff->scr_vert > (curr_buff->last_line - 5)) && (curr_buff->curr_line->next_line != NULL))
			{
				tmp = curr_buff->pointer;
				tmp_line = curr_buff->curr_line;
				slct_flag = mark_text;
				mark_text = FALSE;
				tmp_paste = cpste_line;
				j = 1;
				i = curr_buff->curr_line->vert_len - 1;
				while ((i < 5) && (tmp_line->next_line != NULL))
				{
					tmp_line = tmp_line->next_line;
					i += tmp_line->vert_len;
					j++;
				}
				if (i > 5)
					i = 5;
				m = i;
				i = (curr_buff->scr_vert + i ) - curr_buff->last_line;
				if (i > 0)
					curr_buff->scr_vert = curr_buff->last_line - m;
				wmove(curr_buff->win, 0, 0);
				for (value=0; value<i; value++)
					wdeleteln(curr_buff->win);
				tmp_line = curr_buff->curr_line;
				i = curr_buff->scr_vert;
				for (value=0; value<j; value++)
				{
					if ((slct_flag) && (cpste_line->next_line != NULL))
					{
						cpste_line = cpste_line->next_line;
						wstandout(curr_buff->win);
					}
					draw_line(i, 0, curr_buff->pointer, 1, curr_buff->curr_line);
					i += curr_buff->curr_line->vert_len;
					wstandend(curr_buff->win);
					curr_buff->curr_line = curr_buff->curr_line->next_line;
					if (curr_buff->curr_line != NULL)
						curr_buff->pointer = curr_buff->curr_line->line;
				}
				curr_buff->curr_line = tmp_line;
				cpste_line = tmp_paste;
				curr_buff->pointer = tmp;
				mark_text = slct_flag;
				curr_buff->scr_horz = 0;
			}
		}
	}
	wmove(curr_buff->win, curr_buff->scr_vert, curr_buff->scr_horz);
}
	
void 
eol()		/* go to end of current line			*/
{
	int e;

	if ((curr_buff->position == curr_buff->curr_line->line_length) && (curr_buff->curr_line->next_line != NULL))
		right(TRUE);

	e = (curr_buff->curr_line->vert_len - (scanline(curr_buff->curr_line, curr_buff->position) / COLS)) - 1;

	if (mark_text)
	{
		while (curr_buff->position != curr_buff->curr_line->line_length)
			fast_right();
	}
	else
	{
		curr_buff->position = curr_buff->curr_line->line_length;
		curr_buff->abs_pos = curr_buff->scr_pos = scanline(curr_buff->curr_line, curr_buff->position);
		curr_buff->scr_horz = curr_buff->scr_pos % COLS;
	}
	curr_buff->scr_vert += e;
	if (curr_buff->scr_vert > curr_buff->last_line)
	{
		curr_buff->scr_vert = curr_buff->last_line;
		midscreen(curr_buff->scr_vert, curr_buff->position);
	}
	else if (mark_text)
		midscreen(curr_buff->scr_vert, curr_buff->position);
	else
	{
		curr_buff->pointer = &(curr_buff->curr_line->line[curr_buff->position - 1]);
		wmove(curr_buff->win, curr_buff->scr_vert, curr_buff->scr_horz);
	}
}

void 
bol()			/* move to beginning of current line	*/
{
	int b;

	b = scanline(curr_buff->curr_line, curr_buff->position) / COLS;
	if (curr_buff->position != 1)
	{
		if (mark_text)
		{
			while (curr_buff->position != 1)
				fast_left();
		}
		else
		{
			curr_buff->position = 1;
			curr_buff->scr_horz = curr_buff->scr_pos = 0;
			curr_buff->pointer = curr_buff->curr_line->line;
		}

		curr_buff->abs_pos = 0;

		if ((curr_buff->scr_vert - b) < 0)
		{
			curr_buff->scr_vert = 0;
			midscreen(0, curr_buff->position);
		}
		else
		{
			curr_buff->scr_vert -= b;
			if (mark_text)
				midscreen(curr_buff->scr_vert, curr_buff->position);
			else
				wmove(curr_buff->win, curr_buff->scr_vert, curr_buff->scr_horz);
		}
	}
	else if (curr_buff->curr_line->prev_line != NULL)
	{
		curr_buff->abs_pos = 0;
		up();
	}
}

void 
adv_line()	/* advance to beginning of next line	*/
{
	curr_buff->abs_pos = 0;
	if (curr_buff->position == 1)
		down();
	else
	{
		if (curr_buff->position < curr_buff->curr_line->line_length)
			eol();
		right(TRUE);
	}
}

/*
 |	move cursor to the next page
 */

void 
next_page()
{
	int counter = 0;
	int vlength;
	int tmp_vert = curr_buff->scr_vert;
	struct text *tmp_line;
	char done = FALSE;

	if (curr_buff->curr_line->changed && curr_buff->journalling)
		write_journal(curr_buff, curr_buff->curr_line);

	vlength = 
	   (curr_buff->curr_line->vert_len - (scanline(curr_buff->curr_line, curr_buff->position) / COLS)) - 1;
	tmp_line = curr_buff->curr_line;
	while ((curr_buff->curr_line->next_line != NULL) && (counter < curr_buff->last_line) && (!done))
	{
		if ((curr_buff->curr_line->vert_len + counter - vlength) <= curr_buff->last_line)
		{
			counter += curr_buff->curr_line->vert_len - vlength;
			adv_line();
		}
		else
			done = TRUE;

		vlength = 0;
	}

	if (counter == 0)
	{
		if (curr_buff->curr_line->next_line != NULL)
			adv_line();
	}

	if (curr_buff->curr_line->next_line != NULL)
		midscreen(tmp_vert, 0);
}

/*
 |	move the cursor to the previous page
 */

void 
prev_page()
{
	int counter = 0;
	int vlength;
	int tmp_vert = curr_buff->scr_vert;
	struct text *tmp_line;
	char done = FALSE;

	if (curr_buff->curr_line->changed && curr_buff->journalling)
		write_journal(curr_buff, curr_buff->curr_line);

	vlength = (scanline(curr_buff->curr_line, curr_buff->position) / COLS);
	tmp_line = curr_buff->curr_line;
	while ((curr_buff->curr_line->prev_line != NULL) && (counter < curr_buff->last_line) && (!done))
	{
		if ((curr_buff->curr_line->vert_len + counter - vlength) <= curr_buff->last_line)
		{
			counter += curr_buff->curr_line->vert_len - vlength;
			if (curr_buff->position != 1)
				bol();
			bol();
		}
		else
			done = TRUE;

		vlength = 0;
	}

	if (counter == 0)
	{
		if (curr_buff->curr_line->prev_line != NULL)
		{
			if (curr_buff->position != 1)
				bol();
			bol();
		}
	}

	midscreen(tmp_vert, 0);
}

