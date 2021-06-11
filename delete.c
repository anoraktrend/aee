/*
 |	delete.c
 |
 |	$Header: /home/hugh/sources/aee/RCS/delete.c,v 1.10 2009/03/12 02:52:13 hugh Exp hugh $
 */

/*
 |	Copyright (c) 1986 - 1988, 1991 - 1996, 2009, 2010 Hugh Mahon.
 */

#include "aee.h"

void 
del_char(save_flag)	/* delete character at current cursor position	*/
int save_flag;
{
	int dselect;

	dselect = mark_text;
	if ((mark_text) && (curr_buff->position == curr_buff->curr_line->line_length))
		mark_text = FALSE;
	in = 8;
	if (right(TRUE))
	{
		if ((delete(TRUE)) && (save_flag))
			last_deleted(CHAR_DELETED, 1, &d_char);
	}
	mark_text = dselect;
}

void 
undel_char()			/* undelete last deleted character	*/
{
	int temp_flag;

	temp_flag = overstrike;
	overstrike = FALSE;
	if (d_char == '\n')	/* insert line if last del_char deleted eol */
		insert_line(TRUE);
	else
	{
		in = d_char;
		insert(in);
	}
	overstrike = temp_flag;
}

char *
del_string(length)		/* delete string of specified length at current cursor position	*/
int length;
{
	int tposit;
	int i, d, count;
	char *d_word2, *d_word3;
	char *temp_point;

	change = TRUE;
	d = curr_buff->curr_line->vert_len;
	temp_point = xalloc(1 + length);
	d_word3 = curr_buff->pointer;
	d_word2 = temp_point;
	tposit = curr_buff->position;
	count = 0;
	while ((tposit < curr_buff->curr_line->line_length) && (count < length))
	{
		count++;
		tposit++;
		*d_word2 = *d_word3;
		d_word2++;
		d_word3++;
	}
	*d_word2 = '\0';
	d_word2 = curr_buff->pointer;
	if ((mark_text) && (pst_pos != cpste_line->line_length))
	{
		for (i = 1; (i <= count) ; i++)
			slct_dlt();
	}
	while (tposit < curr_buff->curr_line->line_length)
	{
		tposit++;
		*d_word2 = *d_word3;
		d_word2++;
		d_word3++;
	}
	curr_buff->curr_line->line_length -= length;
	curr_buff->curr_line->vert_len = (scanline(curr_buff->curr_line, curr_buff->curr_line->line_length) / COLS) + 1;
	curr_buff->curr_line->changed = TRUE;
	curr_buff->changed = TRUE;
	if ((curr_buff->curr_line->vert_len < d) && (curr_buff->scr_vert < curr_buff->last_line))
	{
		i = d -= curr_buff->curr_line->vert_len;
		i = curr_buff->last_line - i;
		wmove(curr_buff->win, curr_buff->scr_vert+1, 0);
		while (d > 0)
		{
			d--;
			wdeleteln(curr_buff->win);
		}
		d = curr_buff->scr_vert;
		tmp_line = curr_buff->curr_line;
		d += curr_buff->curr_line->vert_len - (curr_buff->scr_pos / COLS);
		while ((d <= i) && (tmp_line->next_line != NULL))
		{
			tmp_line = tmp_line->next_line;
			d += tmp_line->vert_len;
		}
		if (tmp_line != NULL)
		{
			d -= tmp_line->vert_len;
			wmove(curr_buff->win, d, 0);
			wclrtobot(curr_buff->win);
			for (; (d <= curr_buff->last_line) && (tmp_line != NULL); d += tmp_line->vert_len, tmp_line=tmp_line->next_line)
				draw_line(d, 0, tmp_line->line, 1, tmp_line);
			wmove(curr_buff->win, curr_buff->scr_vert, curr_buff->scr_horz);
		}
	}
	*d_word2 = '\0';
	draw_line(curr_buff->scr_vert, curr_buff->scr_pos, curr_buff->pointer, curr_buff->position, curr_buff->curr_line);
	return(temp_point);
}

void 
undel_string(string, length)	/* insert string at current position	*/
char *string;
int length;
{
	int temp;
	int tposit;
	char *d_word2;
	char *d_word3;
	char *d_word4;
	char *d_word5;
	int u;
	int temp_flag;
	int temp_mark;	/* if mark_text must be set FALSE, store original value here	*/
	int temp_d_char;

	formatted = FALSE;
	temp_flag = overstrike;
	overstrike = FALSE;
	if ((observ_margins) && (curr_buff->scr_pos < left_margin))
	{
		temp_d_char = d_char;
		insert(' ');
		delete(TRUE);
		d_char = temp_d_char;
	}
	u = curr_buff->curr_line->vert_len;
	if ((curr_buff->curr_line->max_length - (curr_buff->curr_line->line_length + length)) < 5)
		curr_buff->pointer = resiz_line(length, curr_buff->curr_line, curr_buff->position);
	d_word5 = d_word4 = xalloc(curr_buff->curr_line->line_length + length);
	d_word3 = string;
	temp = 1;
	while (temp <= length)
	{
		temp++;
		*d_word5 = *d_word3;
		d_word5++;
		d_word3++;
	}
	d_word2 = curr_buff->pointer;
	tposit = curr_buff->position;
	while (tposit < curr_buff->curr_line->line_length)
	{
		temp++;
		tposit++;
		*d_word5 = *d_word2;
		d_word5++;
		d_word2++;
	}
	curr_buff->curr_line->line_length += length;
	curr_buff->curr_line->changed = TRUE;
	curr_buff->changed = TRUE;
	d_word2 = curr_buff->pointer;
	*d_word5 = '\0';
	d_word3 = d_word4;
	tposit = 1;
	while (tposit < temp)
	{
		tposit++;
		*d_word2 = *d_word3;
		d_word3++;
		d_word2++;
	}
	*d_word2 = '\0';
	if ((mark_text) && (pst_pos == 1) && ((cpste_line->line_length > 1) || (cpste_line->next_line != NULL)))
	{
		temp_mark = mark_text;
		mark_text = FALSE;
		for (temp = 1; temp <= length; temp++)
			right(FALSE);
		mark_text = temp_mark;
		for (temp = length; temp >= 1; temp--)
			left(TRUE);
	}
	free(d_word4);
	curr_buff->curr_line->vert_len = (scanline(curr_buff->curr_line, curr_buff->curr_line->line_length) / COLS) + 1;
	if ((curr_buff->curr_line->vert_len > u) && (curr_buff->scr_vert < curr_buff->last_line))
	{
		u = curr_buff->curr_line->vert_len - u;
		wmove(curr_buff->win, curr_buff->scr_vert+1, 0);
		while (u > 0)
		{
			u--;
			winsertln(curr_buff->win);
		}
	}
	draw_line(curr_buff->scr_vert, curr_buff->scr_pos, curr_buff->pointer, curr_buff->position, curr_buff->curr_line);
	overstrike = temp_flag;
}

void 
del_word(save_flag)		/* delete word in front of cursor	*/
int save_flag;
{
	int tposit;
	int difference;
	char *d_word3;
	char tmp_char;

	if (free_d_word)
	{
		free_d_word = FALSE;
		free(d_word);
	}
	change = TRUE;
	curr_buff->curr_line->changed = TRUE;
	curr_buff->changed = TRUE;
	tmp_char = d_char;
	tposit = curr_buff->position;
	d_word3 = curr_buff->pointer;
	while ((tposit < curr_buff->curr_line->line_length) && ((*d_word3 != ' ') && (*d_word3 != '\t')))
	{
		tposit++;
		d_word3++;
	}
	while ((tposit < curr_buff->curr_line->line_length) && ((*d_word3 == ' ') || (*d_word3 == '\t')))
	{
		tposit++;
		d_word3++;
	}
	d_wrd_len = difference = tposit - curr_buff->position;
	d_word = del_string(difference);
	d_char = tmp_char;
	if (save_flag)
		last_deleted(WORD_DELETED, d_wrd_len, d_word);
	formatted = FALSE;
}

void 
undel_word()		/* undelete last deleted word		*/
{
	undel_string(d_word, d_wrd_len);
}

void 
Clear_line(save_flag)		/* delete from cursor to end of line	*/
int save_flag;
{
	int difference;

	if (free_d_line && (d_line != NULL))
	{
		free_d_line = FALSE;
		free(d_line);
	}
	difference = dlt_line->line_length = 1 + curr_buff->curr_line->line_length - curr_buff->position;
	d_line = del_string(difference);
	if ((dlt_line->line_length > 1) && (save_flag))
		last_deleted(LINE_DELETED, dlt_line->line_length, d_line);
	*curr_buff->pointer = '\0';
	curr_buff->curr_line->line_length = curr_buff->position;
	formatted = FALSE;
}

void 
del_line(save_flag)	/* delete line forward of cursor to end of line */
int save_flag;
{
	int dselect;

	Clear_line(save_flag);
	dselect = mark_text;
	mark_text = FALSE;
	if ((dselect) && (pst_pos != cpste_line->line_length))
	{
		if (cpste_line->next_line != NULL)
		{
			cpste_line = fpste_line = cpste_line->next_line;
			free(cpste_line->prev_line->line);
			free(cpste_line->prev_line);
			cpste_line->prev_line = NULL;
			pst_pnt = pst_line = cpste_line->line;
			pst_pos = 1;
		}
		else
		{
			pst_pos = cpste_line->line_length = 1;
			pst_pnt = pst_line = cpste_line->line;
			*pst_pnt = '\0';
		}
	}
	if (curr_buff->curr_line->next_line != NULL)
	{
		right(TRUE);
		delete(TRUE);
		if ((dlt_line->line_length <= 1) && (save_flag))
			last_deleted(LINE_DELETED, dlt_line->line_length, d_line);
	}
	mark_text = dselect;
}

void 
undel_line()			/* undelete last deleted line		*/
{
	char *ud1;
	char *ud2;
	int tposit;
	int u;
	int temp_flag;
	int temp_left;	/* storage for left_margin	*/
	int temp_mark;	/* if mark_text must be set FALSE, store original value here	*/

	temp_left = left_margin;
	left_margin = 0;
	temp_flag = indent;
	indent = FALSE;
	insert_line(TRUE);
	left(TRUE);
	u = curr_buff->scr_pos / COLS;
	curr_buff->pointer = resiz_line(dlt_line->line_length, curr_buff->curr_line, curr_buff->position);
	curr_buff->curr_line->line_length += dlt_line->line_length - 1;
	ud1 = curr_buff->pointer;
	ud2 = d_line;
	tposit = 1;
	while (tposit < dlt_line->line_length)
	{
		tposit++;
		*ud1 = *ud2;
		ud1++;
		ud2++;
	}
	*ud1 = '\0';
	if ((mark_text) && (pst_pos != cpste_line->line_length))
	{
		temp_mark = mark_text;
		mark_text = FALSE;
		for (tposit = 1; tposit < dlt_line->line_length; tposit++)
			right(FALSE);
		mark_text = temp_mark;
		for (tposit = 1; tposit < dlt_line->line_length; tposit++)
			fast_left();
	}
	curr_buff->curr_line->vert_len = (scanline(curr_buff->curr_line, curr_buff->curr_line->line_length) / COLS) + 1;
	curr_buff->curr_line->changed = TRUE;
	curr_buff->changed = TRUE;
	if (curr_buff->curr_line->vert_len > (u+1))
	{
		u = curr_buff->curr_line->vert_len - (u+1);
		wmove(curr_buff->win, curr_buff->scr_vert+1, 0);
		while (u > 0)
		{
			u--;
			winsertln(curr_buff->win);
		}
	}
	draw_line(curr_buff->scr_vert, curr_buff->scr_pos, curr_buff->pointer, curr_buff->position, curr_buff->curr_line);
	indent = temp_flag;
	left_margin = temp_left;
	formatted = FALSE;
}

void 
last_deleted(flag, length, string)
int flag;
int length;
char *string;
{
	if ((flag == WORD_DELETED) && (length == 0))
		return;

	undel_current = undel_current->next;
	if (undel_current->string != NULL)
	{
		if (undel_current->flag == WORD_DELETED)
		{
			if (undel_current->string != d_word)
				free(undel_current->string);
			else
				free_d_word = TRUE;
		}
		else if (undel_current->flag == LINE_DELETED)
		{
			if (undel_current->string != d_line)
				free(undel_current->string);
			else
				free_d_line = TRUE;
		}
	}

	if (undel_current->next == undel_first)
		undel_first = undel_first->next;

	undel_current->flag = flag;
	undel_current->length = length;

	if ((flag == CHAR_DELETED) || (flag == CHAR_BACKSPACE))
	{
		undel_current->character = *string;
	}
	else if ((flag == WORD_DELETED) || (flag == LINE_DELETED))
	{
		undel_current->string = string;
	}
}

void 
undel_last()
{
	char *temp_ptr;
	char temp_char;
	int temp_length;
	int auto_indent_temp = indent;

	if (undel_current->flag != 0)
	{
		indent = FALSE;
		if ((undel_current->flag == CHAR_DELETED) || 
			(undel_current->flag == CHAR_BACKSPACE))
		{
			temp_char = d_char;
			d_char = undel_current->character;
			undel_char();
			d_char = temp_char;
			if (undel_current->flag == CHAR_DELETED)
				left(TRUE);
		}
		else if (undel_current->flag == WORD_DELETED)
		{
			temp_ptr = d_word;
			temp_length = d_wrd_len;
			d_word = undel_current->string;
			d_wrd_len = undel_current->length;
			undel_word();
			d_word = temp_ptr;
			d_wrd_len = temp_length;
		}
		else if (undel_current->flag == LINE_DELETED)
		{
			temp_ptr = d_line;
			temp_length = dlt_line->line_length;
			d_line = undel_current->string;
			dlt_line->line_length = undel_current->length;
			undel_line();
			d_line = temp_ptr;
			dlt_line->line_length = temp_length;
		}
		undel_current->flag = 0;
		if (undel_current != undel_first)
			undel_current = undel_current->prev;
		indent = auto_indent_temp;
	}
}

void 
delete_text()
{
	while (curr_buff->curr_line->next_line != NULL)
		curr_buff->curr_line = curr_buff->curr_line->next_line;
	while (curr_buff->curr_line != curr_buff->first_line)
	{
		if (curr_buff->journalling)
			remove_journ_line(curr_buff, curr_buff->curr_line);
		free(curr_buff->curr_line->line);
		curr_buff->curr_line = curr_buff->curr_line->prev_line;
		free(curr_buff->curr_line->next_line);
	}
	curr_buff->curr_line->next_line = NULL;
	*curr_buff->curr_line->line = '\0';
	curr_buff->curr_line->line_length = 1;
	curr_buff->curr_line->line_number = 1;
	curr_buff->pointer = curr_buff->curr_line->line;
	curr_buff->scr_pos = curr_buff->scr_vert = curr_buff->scr_horz = 0;
	curr_buff->position = 1;
}

void 
undel_init()
{
	int counter;
	struct del_buffs *temp;

	for (counter = 0; counter < 128; counter++)
	{
		if (undel_first == NULL)
			temp = undel_current = undel_first = 
			(struct del_buffs *) xalloc(sizeof(struct del_buffs));
		else
		{
			temp = 
			(struct del_buffs *) xalloc(sizeof(struct del_buffs));
		}
		undel_current->next = temp;
		temp->prev = undel_current;
		undel_current = temp;
		undel_current->flag = 0;
		undel_current->string = NULL;
	}
	undel_first->prev = temp;
	temp->next = undel_first;
	undel_current = undel_first;
}

int 
delete(disp)			/* delete character within the line	*/
int disp;
{
	char *tp;
	char *temp2;
	struct text *temp_buff;
	int d;
	int temp_vert;
	int temp_pos;
	int dselect;
	int success = FALSE;

	if (curr_buff->position != 1)	/* if not at beginning of line	*/
	{
		formatted = FALSE;
		curr_buff->curr_line->changed = TRUE;
		curr_buff->changed = TRUE;
		success = TRUE;
		if ((mark_text) && (pst_pos != 1))
			slct_dlt();
		change = TRUE;
		temp_pos = --curr_buff->position;
		temp2 = tp = curr_buff->pointer;
		tp--;
		curr_buff->pointer--;
		if (*curr_buff->pointer == '\t')
			curr_buff->abs_pos = curr_buff->scr_pos = scanline(curr_buff->curr_line, temp_pos);
		else
			curr_buff->abs_pos = curr_buff->scr_pos -= len_char(*curr_buff->pointer, curr_buff->scr_pos);
		d = curr_buff->scr_pos % COLS;
		if ((curr_buff->scr_horz - d) < 0)
		{
			curr_buff->scr_vert--;
			if (curr_buff->scr_vert < 0)
			{
				curr_buff->scr_vert++;
				left(TRUE);
				right(TRUE);
			}
		}
		curr_buff->scr_horz = d;
		if ((in == 8) || (in == KEY_BACKSPACE) || (in == 127))
			d_char = *curr_buff->pointer;	/* save deleted character */
		curr_buff->curr_line->line_length--;
		while (temp_pos <= curr_buff->curr_line->line_length)
		{
			temp_pos++;
			*tp= *temp2;
			tp++;
			temp2++;
		}
		if ((curr_buff->position != curr_buff->curr_line->line_length) || (*curr_buff->pointer == '\t'))
			value = (scanline(curr_buff->curr_line, curr_buff->curr_line->line_length) / COLS) + 1;
		else
			value = (curr_buff->scr_pos / COLS) + 1;
		if (value < curr_buff->curr_line->vert_len)
		{
			d = curr_buff->curr_line->vert_len - value;
			curr_buff->curr_line->vert_len = value;
			if ((curr_buff->scr_vert < curr_buff->last_line) && (disp))
			{
				wmove(curr_buff->win, curr_buff->scr_vert+1, 0);
				while (d > 0)
				{
					d--;
					wdeleteln(curr_buff->win);
				}
				wmove(curr_buff->win, curr_buff->scr_vert, curr_buff->scr_horz);
				d = curr_buff->scr_vert + (curr_buff->curr_line->vert_len - (curr_buff->scr_pos / COLS));
				temp_buff = curr_buff->curr_line;
				while ((d <= curr_buff->last_line) && (temp_buff->next_line != NULL))
				{
					temp_buff = temp_buff->next_line;
					d += temp_buff->vert_len;
				}
				d -= temp_buff->vert_len;
				draw_line(d, 0, temp_buff->line, 1, temp_buff);
			}
		}
	}
	else if (curr_buff->curr_line->prev_line != NULL)
	{
		formatted = FALSE;
		success = TRUE;
		change = TRUE;
		d = curr_buff->curr_line->vert_len + curr_buff->curr_line->prev_line->vert_len;
		dselect = mark_text;
		mark_text = FALSE;
		if ((dselect) && (cpste_line->prev_line != NULL))
		{
			cpste_line = cpste_line->prev_line;
			free(cpste_line->next_line->line);
			free(cpste_line->next_line);
			cpste_line->next_line = NULL;
		}
		left(disp);		/* go to previous line	*/
		mark_text = dselect;
		temp_buff = curr_buff->curr_line->next_line;
		curr_buff->pointer = resiz_line(temp_buff->line_length, curr_buff->curr_line, curr_buff->position);
		if (temp_buff->next_line != NULL)
			temp_buff->next_line->prev_line = curr_buff->curr_line;
		if (curr_buff->journalling)
			remove_journ_line(curr_buff, curr_buff->curr_line->next_line);
		curr_buff->curr_line->next_line = temp_buff->next_line;
		temp2 = temp_buff->line;
		if (in == 8)
			d_char = '\n';
		tp = curr_buff->pointer;
		temp_pos = 1;
		while (temp_pos < temp_buff->line_length)
		{
			curr_buff->curr_line->line_length++;
			temp_pos++;
			*tp = *temp2;
			tp++;
			temp2++;
		}
		*tp = '\0';
		curr_buff->curr_line->changed = TRUE;
		curr_buff->changed = TRUE;
		free(temp_buff->line);
		free(temp_buff);
		temp_buff = curr_buff->curr_line;
		temp_vert = curr_buff->scr_vert;
		curr_buff->curr_line->vert_len = (scanline(curr_buff->curr_line, curr_buff->curr_line->line_length) / COLS) + 1;
/*		curr_buff->abs_pos = curr_buff->scr_pos = scanline(curr_buff->curr_line, curr_buff->position);*/
		curr_buff->num_of_lines--;
		if (disp)
		{
			if (d > curr_buff->curr_line->vert_len)
			{
				wmove(curr_buff->win, (curr_buff->scr_vert + 1), 0);
				d = d - curr_buff->curr_line->vert_len;
				for (value = 0; value < d; value++)
					wdeleteln(curr_buff->win);
				temp_vert += curr_buff->curr_line->vert_len - (curr_buff->scr_pos / COLS);
				d = curr_buff->last_line - d;
				while ((temp_buff->next_line != NULL) && (temp_vert <= d))
				{
					temp_buff = temp_buff->next_line;
					temp_vert += temp_buff->vert_len;
				}
				if (temp_buff != NULL)
				{
					temp_vert -= temp_buff->vert_len;
					wmove(curr_buff->win, curr_buff->last_line, 0);
					wclrtobot(curr_buff->win);
					for (; (temp_vert <= curr_buff->last_line) && (temp_buff != NULL); temp_vert+= temp_buff->vert_len, temp_buff=temp_buff->next_line)
						draw_line(temp_vert, 0, temp_buff->line, 1, temp_buff);
					wmove(curr_buff->win, curr_buff->scr_vert, curr_buff->scr_horz);
				}
			}
		}
	}
	if (disp)
		draw_line(curr_buff->scr_vert, curr_buff->scr_pos, curr_buff->pointer, curr_buff->position, curr_buff->curr_line);

	return(success);
}

