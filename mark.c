/*
 |	mark.c
 |
 |	$Header: /home/hugh/sources/aee/RCS/mark.c,v 1.12 2010/07/18 01:00:56 hugh Exp hugh $
 */

/*
 |	Copyright (c) 1986 - 1988, 1991 - 1996, 2009, 2010 Hugh Mahon.
 */

#include "aee.h"

/* Add missing external declarations */
extern struct bufr *curr_buff;
extern WINDOW *com_win;
extern char mark_text;
extern struct text *paste_buff;
extern struct text *fpste_line;
extern struct text *cpste_line;
extern struct text *tmp_line;
extern struct text *pste_tmp;
extern char *pst_line;
extern char *pst_pnt;
extern int pst_pos;
extern char *pste1;
extern char *pste2;

/* Constants for mark_text values */
#define Mark 1
#define Append 2  
#define Prefix 3

void 
copy()			/* copy selected (marked) text into paste buffer */
{
	struct text *tmp_pste;

	if (mark_text)
	{
		if ((paste_buff != NULL) && (mark_text == Mark))
		{
			tmp_pste = paste_buff;
			while (tmp_pste->next_line != NULL)
			{
				free(tmp_pste->line);
				tmp_pste = tmp_pste->next_line;
				free(tmp_pste->prev_line);
			}
			free(tmp_pste->line);
			free(tmp_pste);
		}
		if ((mark_text == Mark) || (paste_buff == NULL))
			paste_buff = fpste_line;
		else if (mark_text == Append)
		{
			tmp_pste = paste_buff;
			while (tmp_pste->next_line != NULL)
				tmp_pste = tmp_pste->next_line;
			fpste_line->prev_line = tmp_pste;
			tmp_pste->next_line = fpste_line;
		}
		else if (mark_text == Prefix)
		{
			tmp_pste = fpste_line;
			while (tmp_pste->next_line != NULL)
				tmp_pste = tmp_pste->next_line;
			tmp_pste->next_line = paste_buff;
			paste_buff->prev_line = tmp_pste;
			paste_buff = fpste_line;
		}
		mark_text = FALSE;
		midscreen(curr_buff->scr_vert, curr_buff->position);
	}
	else
	{
		wmove(com_win, 0, 0);
		werase(com_win);
		wprintw(com_win, mark_not_actv_str);
		wrefresh(com_win);
		clr_cmd_line = TRUE;
	}
}

void 
paste()		/* insert text from paste buffer into current buffer	*/
{
	int vert;
	int tposit;
	int ppos;
	int temp_flag1;
	int temp_flag2;
	int temp_abs_line;
	int temp_left_margin;

	temp_abs_line = curr_buff->absolute_lin;
	temp_flag1 = overstrike;
	temp_flag2 = indent;
	temp_left_margin = left_margin;
	overstrike = FALSE;
	indent = FALSE;
	left_margin = 0;
	if ((!mark_text) && (paste_buff != NULL))
	{
		vert = curr_buff->scr_vert;
		tmp_line = curr_buff->curr_line;
		ppos = curr_buff->scr_pos;
		cpste_line = paste_buff;
		tposit = curr_buff->position;
		while (cpste_line != NULL)
		{
			pst_pos = 1;
			pst_pnt = pst_line = cpste_line->line;
			insert_line(FALSE);
			left(FALSE);
			curr_buff->pointer = resiz_line(cpste_line->line_length,curr_buff->curr_line,curr_buff->position);
			while (pst_pos < cpste_line->line_length)
			{
				*curr_buff->pointer = *pst_pnt;
				curr_buff->pointer++;
				pst_pnt++;
				curr_buff->position++;
				curr_buff->curr_line->line_length++;
				pst_pos++;
			}
			curr_buff->curr_line->changed = TRUE;
			curr_buff->changed = TRUE;
			*curr_buff->pointer = '\0';
			curr_buff->curr_line->vert_len = (scanline(curr_buff->curr_line, curr_buff->curr_line->line_length) / COLS) + 1;
			cpste_line = cpste_line->next_line;
			right(FALSE);
		}
		delete(FALSE);
		if (curr_buff->journalling)
			write_journal(curr_buff, curr_buff->curr_line);
		curr_buff->abs_pos = curr_buff->scr_pos = ppos;
		curr_buff->scr_vert = vert;
		curr_buff->absolute_lin = temp_abs_line;
		curr_buff->curr_line = tmp_line;
		curr_buff->pointer = curr_buff->curr_line->line;
		curr_buff->position = 1;
		while (curr_buff->position < tposit)
		{
			curr_buff->pointer++;
			curr_buff->position++;
		}
/*		curr_buff->scr_pos = curr_buff->abs_pos = scanline(curr_buff->curr_line, curr_buff->curr_line->line_length);*/
		midscreen(curr_buff->scr_vert, curr_buff->position);
	}
	else if (mark_text)
	{
		wmove(com_win, 0, 0);
		wclrtoeol(com_win);
		wprintw(com_win, mark_active_str);
		wrefresh(com_win);
		clr_cmd_line = TRUE;
		wmove(curr_buff->win, curr_buff->scr_vert, curr_buff->scr_horz);
	}
	overstrike = temp_flag1;
	indent = temp_flag2;
	left_margin = temp_left_margin;
}

void 
unmark_text()	/* unmark text and do not change contents of paste buffer */
{
    if (mark_text)
    {
        mark_text = FALSE;
        if (cpste_line != NULL) {
            // Clean up paste buffer
            while (cpste_line->prev_line != NULL)
                cpste_line = cpste_line->prev_line;
            while (cpste_line->next_line != NULL)
            {
                if (cpste_line->line != NULL)
                    free(cpste_line->line);
                cpste_line = cpste_line->next_line;
                free(cpste_line->prev_line);
            }
            if (cpste_line->line != NULL)
                free(cpste_line->line);
            free(cpste_line);
            cpste_line = NULL;
        }
        // Update display
        wmove(com_win, 0, 0);
        wclrtoeol(com_win);
        wprintw(com_win, "Mark mode disabled");
        wrefresh(com_win);
        midscreen(curr_buff->scr_vert, curr_buff->position);
    }
    else
    {
        wmove(com_win,0,0);
        wclrtoeol(com_win);
        wprintw(com_win, mark_not_actv_str);
        wrefresh(com_win);
        wmove(curr_buff->win, curr_buff->scr_vert, curr_buff->scr_horz);
        clr_cmd_line = TRUE;
    }
}

void 
cut()	/* cut selected (marked) text out of current buffer	*/
{
	int vert;
	int temp_slct;
	char tmp_char;

	if (mark_text)
	{
		temp_slct = mark_text;
		mark_text = FALSE;
		tmp_char = d_char;
		vert = curr_buff->scr_vert;
		if (cpste_line->next_line != NULL)
			cut_down();
		else if (cpste_line->prev_line != NULL)
			cut_up();
		else 
			cut_line();
		cpste_line = fpste_line;
		while (cpste_line != NULL)
		{
			pst_pos = 1;
			pst_pnt = cpste_line->line;
			while (pst_pos < cpste_line->line_length)
			{
				pst_pos++;
				pst_pnt++;
			}
			if (*pst_pnt == -1)
				*pst_pnt = '\0';
			cpste_line = cpste_line->next_line;
		}
		curr_buff->scr_vert = vert;
		midscreen(curr_buff->scr_vert, curr_buff->position);
		d_char = tmp_char;
		mark_text = temp_slct;
		copy();
	}
	else
	{
		wmove(com_win,0,0);
		wclrtoeol(com_win);
		wprintw(com_win, mark_not_actv_str);
		wrefresh(com_win);
		wmove(curr_buff->win, curr_buff->scr_vert, curr_buff->scr_horz);
		clr_cmd_line = TRUE;
	}
}
	

void 
cut_up()	/* cut text above current cursor position	*/
{
	cut_line();
	left(FALSE);
	cpste_line = cpste_line->prev_line;
	while (cpste_line->prev_line != NULL)
	{
		cpste_line = cpste_line->prev_line;
		curr_buff->pointer = curr_buff->curr_line->line;
		*curr_buff->pointer = '\0';
		curr_buff->position = curr_buff->curr_line->line_length = 1;
		delete(FALSE);
	}
	pst_pnt = pst_line = cpste_line->line;
	pst_pos = 1;
	while (pst_pos < cpste_line->line_length)
	{
		pst_pos++;
		pst_pnt++;
	}
	cut_line();
	right(FALSE);
	delete(FALSE);
}

void 
cut_down()		/* cut text below current cursor position	*/
{
	if (((pst_pos == 1) && (pst_pos < cpste_line->line_length)) ||
	 ((pst_pos != 1) && (pst_pos == cpste_line->line_length)))
	{
		cut_line();
		right(FALSE);
	}
	else
		curr_buff->curr_line = curr_buff->curr_line->next_line;
	cpste_line = cpste_line->next_line;
	while (cpste_line->next_line != NULL)
	{
		cpste_line = cpste_line->next_line;
		curr_buff->pointer = curr_buff->curr_line->line;
		*curr_buff->pointer = '\0';
		curr_buff->position = curr_buff->curr_line->line_length = 1;
		delete(FALSE);
		right(FALSE);
	}
	pst_pnt = pst_line = cpste_line->line;
	pst_pos = 1;
	cut_line();
	delete(FALSE);
}

void 
cut_line()		/* cut text in current line			*/
{
	if (((pst_pos == 1) && (pst_pos < cpste_line->line_length)) || ((pst_pos != 1) && (pst_pos == cpste_line->line_length)))
	{
		if (pst_pos == cpste_line->line_length)
		{
			while (pst_pos > 1)
			{
				pst_pos--;
				delete(FALSE);
			}
		}
		else
		{
			while (pst_pos < cpste_line->line_length)
			{
				right(FALSE);
				delete(FALSE);
				pst_pos++;
			}
		}
	}
}

void 
fast_left()	/* move left one character while selecting (marking) text but not displaying on screen	*/
{
	int tpst_pos;

	if ((curr_buff->position == 1) && (curr_buff->curr_line->prev_line != NULL))
	{
		if (curr_buff->curr_line->changed && curr_buff->journalling)
			write_journal(curr_buff, curr_buff->curr_line);
		curr_buff->curr_line = curr_buff->curr_line->prev_line;
		curr_buff->pointer = curr_buff->curr_line->line;
		curr_buff->position = 1;
		while (curr_buff->position < curr_buff->curr_line->line_length)
		{
			curr_buff->position++;
			curr_buff->pointer++;
		}
		curr_buff->scr_pos = scanline(curr_buff->curr_line, curr_buff->position);
	}
	else if (curr_buff->position > 1)
	{
		curr_buff->pointer--;
		curr_buff->position--;
		if (*curr_buff->pointer == '\t')
			curr_buff->scr_pos = scanline(curr_buff->curr_line, curr_buff->position);
		else
			curr_buff->scr_pos -= len_char(*curr_buff->pointer, curr_buff->scr_pos);
	}
	if (pst_pos != 1)
		slct_dlt();
	else if (curr_buff->position < curr_buff->curr_line->line_length)
	{
		if ((cpste_line->max_length - cpste_line->line_length) < 5)
			pst_pnt = resiz_line(10, cpste_line, pst_pos);
		pste1 = pst_pnt;
		tpst_pos = pst_pos;
		while (tpst_pos < cpste_line->line_length)
		{
			tpst_pos++;
			pste1++;
		}
		pste1++;
		cpste_line->line_length++;
		while (pst_pnt < pste1)
		{
			pste2= pste1 - 1;
			*pste1= *pste2;
			pste1--;
		}
		*pst_pnt = *curr_buff->pointer;
	}
}

void 
fast_right()	/* move right one character and select (mark) but do not display	*/
{
	if (curr_buff->position < curr_buff->curr_line->line_length)
	{
		if ((pst_pos == cpste_line->line_length) && (*pst_pnt == '\0') && (curr_buff->position < curr_buff->curr_line->line_length))
		{
			if ((cpste_line->max_length - cpste_line->line_length) < 5)
				pst_pnt = resiz_line(10, cpste_line, pst_pos);
			*pst_pnt = *curr_buff->pointer;
			pst_pnt++;
			pst_pos++;
			cpste_line->line_length++;
			*pst_pnt = '\0';
		}
		else if (curr_buff->position < curr_buff->curr_line->line_length)
			slct_dlt();
		curr_buff->scr_pos += len_char(*curr_buff->pointer, curr_buff->scr_pos);
		curr_buff->pointer++;
		curr_buff->position++;
	}
	else if (curr_buff->curr_line->next_line != NULL)
	{
		if (curr_buff->curr_line->changed && curr_buff->journalling)
			write_journal(curr_buff, curr_buff->curr_line);
		curr_buff->curr_line = curr_buff->curr_line->next_line;
		curr_buff->pointer = curr_buff->curr_line->line;
		curr_buff->position = 1;	
		curr_buff->scr_pos = 0;
	}
}

void 
fast_line(char *direct)	/* move to the previous or next line while selecting text but do not display */
{
	if (*direct == 'u')
	{
		curr_buff->absolute_lin--;
		while (curr_buff->position > 1)
			fast_left();
		fast_left();
		if (cpste_line->prev_line == NULL)
		{
			pste_tmp = txtalloc();
			pste_tmp->prev_line = NULL;
			pste_tmp->next_line = cpste_line;
			cpste_line->prev_line = pste_tmp;
			pste_tmp->line = pst_pnt= pst_line = xalloc(curr_buff->curr_line->max_length);
			fpste_line = cpste_line = pste_tmp;
			pste_tmp->line_length = 1;
			pste_tmp->max_length = curr_buff->curr_line->max_length;
			*pst_pnt = '\0';
			pst_pos = 1;
		}
		else
		{
			free(cpste_line->line);
			cpste_line = cpste_line->prev_line;
			free(cpste_line->next_line);
			cpste_line->next_line = NULL;
			pst_pnt = pst_line = cpste_line->line;
			pst_pos = 1;
			while (pst_pos < cpste_line->line_length)
			{
				pst_pos++;
				pst_pnt++;
			}
		}
	}
	else
	{
		curr_buff->absolute_lin++;
		while (curr_buff->position < curr_buff->curr_line->line_length)
			fast_right();
		fast_right();
		if (cpste_line->next_line == NULL)
		{
			pste_tmp = txtalloc();
			pst_pnt = pst_line = pste_tmp->line = xalloc(curr_buff->curr_line->max_length);
			*pst_pnt = '\0';
			pst_pos = 1;
			pste_tmp->line_length = 1;
			pste_tmp->max_length = curr_buff->curr_line->max_length;
			cpste_line->next_line = pste_tmp;
			pste_tmp->next_line = NULL;
			pste_tmp->prev_line = cpste_line;
			cpste_line = pste_tmp;
		}
		else
		{
			free(pst_line);
			fpste_line = cpste_line = cpste_line->next_line;
			pst_pnt = pst_line = cpste_line->line;
			pst_pos = 1;
			free(cpste_line->prev_line);
			cpste_line->prev_line = NULL;
		}
	}
}

int 
slct(int flag)	/* initiate process for selecting (marking) text	*/
{
    // If mark mode is already active, disable it
    if (mark_text) {
        unmark_text();
        return 0;
    }

    // Otherwise enable mark mode
    mark_text = flag;
    
    // Initialize paste buffer structures
    cpste_line = fpste_line = txtalloc();
    if (cpste_line == NULL) {
        wmove(com_win, 0, 0);
        wclrtoeol(com_win);
        wprintw(com_win, "Error: Could not allocate memory");
        wrefresh(com_win);
        mark_text = FALSE;
        return(-1);
    }

    // Initialize line buffer with proper size
    cpste_line->line = pst_line = pst_pnt = xalloc(curr_buff->curr_line->max_length);
    if (pst_line == NULL) {
        free(cpste_line);
        wmove(com_win, 0, 0);
        wclrtoeol(com_win);
        wprintw(com_win, "Error: Could not allocate memory");
        wrefresh(com_win);
        mark_text = FALSE;
        return(-1);
    }

    // Initialize buffer state
    pst_pos = cpste_line->line_length = 1;
    *pst_pnt = '\0';
    cpste_line->max_length = curr_buff->curr_line->max_length;
    cpste_line->prev_line = NULL;
    cpste_line->next_line = NULL;
    cpste_line->line_number = 1;

    // Update display
    wmove(com_win, 0, 0);
    wclrtoeol(com_win);
    wprintw(com_win, "Mark mode enabled");
    wrefresh(com_win);
    return(0);
}

void 
slct_dlt()	/* delete character in buffer	*/
{
    int tmpi;  // Declare tmpi variable
    
    // Add bounds checking at the start
    if (!mark_text || cpste_line == NULL || pst_pnt == NULL) {
        wmove(com_win, 0, 0);
        wclrtoeol(com_win);
        wprintw(com_win, "Error: Invalid mark state");
        wrefresh(com_win);
        return;
    }

    if (mark_text)
    {
        if ((pst_pos == cpste_line->line_length) && 
            (curr_buff->position != curr_buff->curr_line->line_length))
        {
            if ((cpste_line->max_length - cpste_line->line_length) < 5)
                pst_pnt = resiz_line(10, cpste_line, pst_pos);
            *pst_pnt = *curr_buff->pointer;
            pst_pnt++;
            *pst_pnt = '\0';
            pst_pos++;
            cpste_line->line_length++;
            wmove(curr_buff->win, curr_buff->scr_vert, curr_buff->scr_horz);
            wstandout(curr_buff->win);
            if ((*curr_buff->pointer < 32) || (*curr_buff->pointer > 126))
                tmpi = out_char(curr_buff->win, *curr_buff->pointer, curr_buff->scr_pos, curr_buff->scr_vert, 0);
            else
                waddch(curr_buff->win, *curr_buff->pointer);
            wstandend(curr_buff->win);
        }
        else if (curr_buff->position < curr_buff->curr_line->line_length)
        {
            slct_dlt();
            pste1 = curr_buff->pointer;
            wmove(curr_buff->win, curr_buff->scr_vert, curr_buff->scr_horz);
            if ((*pste1 < 32) || (*pste1 > 126))
                tmpi = out_char(curr_buff->win, *pste1, curr_buff->scr_pos, curr_buff->scr_vert, 0);
            else
                waddch(curr_buff->win, *pste1);
        }
        wmove(curr_buff->win, curr_buff->scr_vert, curr_buff->scr_horz);
    }
}

void 
slct_left()	/* select text while moving left and display	*/
{
	int tmpi;

	// Add null check and bounds check at start
	if (cpste_line == NULL || curr_buff == NULL) {
		wmove(com_win, 0, 0);
		wclrtoeol(com_win);
		wprintw(com_win, "Error: Invalid paste buffer");
		wrefresh(com_win);
		return;
	}

	if (pst_pos != 1)
	{
		if (pst_pos <= cpste_line->line_length) { // Add bounds check
			slct_dlt();
			pste1 = curr_buff->pointer;
			wmove(curr_buff->win, curr_buff->scr_vert, curr_buff->scr_horz);
			if ((*pste1 < 32) || (*pste1 > 126))
				tmpi = out_char(curr_buff->win, *pste1, curr_buff->scr_pos, curr_buff->scr_vert, 0);
			else
				waddch(curr_buff->win, *pste1);
		}
	}
	else if (curr_buff->position < curr_buff->curr_line->line_length)
	{
		if ((cpste_line->max_length - cpste_line->line_length) < 5)
			pst_pnt = resiz_line(10, cpste_line, pst_pos);
		if (pst_pnt != NULL) { // Add null check after resize
			pste1 = pst_pnt;
			tmpi = pst_pos;
			while (tmpi < cpste_line->line_length)
			{
				tmpi++;
				pste1++;
			}
			pste1++;
			while (pst_pnt < pste1)
			{
				pste2 = pste1 - 1;
				*pste1 = *pste2;
				pste1--;
			}
			*pst_pnt = *curr_buff->pointer;
			cpste_line->line_length++;
			wstandout(curr_buff->win);
			if ((*curr_buff->pointer < 32) || (*curr_buff->pointer > 126))
				tmpi += out_char(curr_buff->win, *curr_buff->pointer, curr_buff->scr_pos, curr_buff->scr_vert, 0);
			else
				waddch(curr_buff->win, *pste1);
			wstandend(curr_buff->win);
		}
	}
	wmove(curr_buff->win, curr_buff->scr_vert, curr_buff->scr_horz);
}

void 
slct_right()    /* select text while moving right and display */
{
    int tmpi;

    // Add null check and bounds check at start
    if (cpste_line == NULL || curr_buff == NULL) {
        wmove(com_win, 0, 0);
        wclrtoeol(com_win);
        wprintw(com_win, "Error: Invalid paste buffer");
        wrefresh(com_win);
        return;
    }

    if (curr_buff->position < curr_buff->curr_line->line_length)
    {
        if ((pst_pos == cpste_line->line_length) && (*pst_pnt == '\0'))
        {
            if ((cpste_line->max_length - cpste_line->line_length) < 5)
                pst_pnt = resiz_line(10, cpste_line, pst_pos);
            if (pst_pnt != NULL) {
                *pst_pnt = *curr_buff->pointer;
                pst_pnt++;
                pst_pos++;
                cpste_line->line_length++;
                *pst_pnt = '\0';
                wmove(curr_buff->win, curr_buff->scr_vert, curr_buff->scr_horz);
                wstandout(curr_buff->win);
                if ((*curr_buff->pointer < 32) || (*curr_buff->pointer > 126))
                    tmpi = out_char(curr_buff->win, *curr_buff->pointer, curr_buff->scr_pos, curr_buff->scr_vert, 0);
                else
                    waddch(curr_buff->win, *curr_buff->pointer);
                wstandend(curr_buff->win);
            }
        }
        else if (curr_buff->position < curr_buff->curr_line->line_length)
        {
            slct_dlt();
            pste1 = curr_buff->pointer;
            wmove(curr_buff->win, curr_buff->scr_vert, curr_buff->scr_horz);
            if ((*pste1 < 32) || (*pste1 > 126))
                tmpi = out_char(curr_buff->win, *pste1, curr_buff->scr_pos, curr_buff->scr_vert, 0);
            else
                waddch(curr_buff->win, *pste1);
        }
        wmove(curr_buff->win, curr_buff->scr_vert, curr_buff->scr_horz);
    }
}

void 
slct_line(char *direct)	/* move to the previous or next line	*/
{
	if (*direct == 'u')
	{
		if (cpste_line->prev_line == NULL)
		{
			pste_tmp = txtalloc();
			pste_tmp->prev_line = NULL;
			pste_tmp->next_line = cpste_line;
			cpste_line->prev_line = pste_tmp;
			pste_tmp->line = pst_pnt= pst_line = xalloc(curr_buff->curr_line->max_length);
			pste_tmp->line_length = 1;
			pste_tmp->max_length = curr_buff->curr_line->max_length;
			fpste_line = cpste_line = pste_tmp;
			*pst_pnt = '\0';
			pst_pos = 1;
			slct_left();
		}
		else
		{
			free(cpste_line->line);
			cpste_line = cpste_line->prev_line;
			free(cpste_line->next_line);
			cpste_line->next_line = NULL;
			pst_pnt = pst_line = cpste_line->line;
			pst_pos = 1;
			while (pst_pos < cpste_line->line_length)
			{
				pst_pos++;
				pst_pnt ++;
			}
		}
	}
	else
	{
		if (cpste_line->next_line == NULL)
		{
			pste_tmp = txtalloc();
			pst_pnt = pst_line = pste_tmp->line = xalloc(curr_buff->curr_line->max_length);
			*pst_pnt = '\0';
			pst_pos = 1;
			pste_tmp->line_length = 1;
			pste_tmp->max_length = curr_buff->curr_line->max_length;
			cpste_line->next_line = pste_tmp;
			pste_tmp->next_line = NULL;
			pste_tmp->prev_line = cpste_line;
			cpste_line = pste_tmp;
		}
		else
		{
			free(pst_line);
			fpste_line = cpste_line = cpste_line->next_line;
			pst_pnt = pst_line = cpste_line->line;
			pst_pos = 1;
			free(cpste_line->prev_line);
			cpste_line->prev_line = NULL;
			wmove(curr_buff->win, curr_buff->scr_vert,curr_buff->scr_horz);
		}
	}
}

