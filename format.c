/*
 |	format.c
 |
 |	$Header: /home/hugh/sources/aee/RCS/format.c,v 1.13 2010/07/18 00:59:02 hugh Exp hugh $
 */
 
/*
 |	Copyright (c) 1987, 1988, 1989, 1991, 1994, 1995, 1996, 1998, 2009, 2010 Hugh Mahon.
 */

#include "aee.h"

int 
Blank_Line(test_line)	/* test if line has any non-space characters	*/
struct text *test_line;
{
	char *line;
	int length;

	if (test_line == NULL)
		return(TRUE);

	length = 0;
	line = test_line->line;
	
	/*
	 |	to handle troff/nroff documents, consider a line with a 
	 |	period ('.') in the first column to be blank
	 */

	if ((*line == '.') || (*line == '>'))
		return(TRUE);

	while ((*line != '\0') && ((*line == ' ') || (*line == '\t')) && (length <= curr_buff->curr_line->line_length))
	{
		length++;
		line++;
	}

	if (*line != '\0')
		return(FALSE);
	else
		return(TRUE);
}

void 
Format()	/* format the paragraph according to set margins	*/
{
	struct text *temp_line;
	struct text *start_line;
	int count;
	int orig_count;
	int temp_pos;
	int string_count;
	int offset;
	int temp_case;
	int temp_forward;
	int temp_literal;
	int temp_indent;
	int temp_over;
	int temp_af;
	int counter;
	char *line;
	char *tmp_srchstr;
	char *temp1, *temp2;
	char *temp_dword;
	char temp_d_char = d_char;

/*
 |	if observ_margins is not set, the current line is blank, or text 
 |	marking is on, do not format the current paragraph
 */

	if ((!observ_margins) || (Blank_Line(curr_buff->curr_line)) || (mark_text))
		return;

	clr_cmd_line = TRUE;

/*
 |	save the currently set flags, and clear them
 */

	temp_indent = indent;
	indent = FALSE;
	temp_literal = literal;
	literal = TRUE;
	temp_af = auto_format;
	auto_format = FALSE;

	wmove(com_win, 0, 0);
	wclrtoeol(com_win);
	wprintw(com_win, fmting_par_msg);
	wrefresh(com_win);

/*
 |	get current position in paragraph, so after formatting, the cursor 
 |	will be in the same relative position
 */

	temp_line = curr_buff->curr_line;
	temp_pos = offset = curr_buff->position;
	if (curr_buff->position != 1)
		prev_word();
	temp_dword = d_word;
	d_word = NULL;
	temp_forward = forward;
	forward = TRUE;
	temp_case = case_sen;
	case_sen = TRUE;
	temp_over = overstrike;
	overstrike = FALSE;
	tmp_srchstr = srch_str;
	temp2 = srch_str = (char *) xalloc(curr_buff->curr_line->line_length - curr_buff->position);
	if ((*curr_buff->pointer == ' ') || (*curr_buff->pointer == '\t'))
		adv_word();
	offset -= curr_buff->position;
	line = temp1 = curr_buff->pointer;
	counter = curr_buff->position;
	while ((*temp1 != '\0') && (*temp1 != ' ') && (*temp1 != '\t') && (counter < curr_buff->curr_line->line_length))
	{
		*temp2 = *temp1;
		temp2++;
		temp1++;
		counter++;
	}
	*temp2 = '\0';
	if (curr_buff->position != 1)
		bol();
	while (!Blank_Line(curr_buff->curr_line->prev_line))
		bol();
	start_line = curr_buff->curr_line;
	string_count = 0;
	while (line != curr_buff->pointer)
	{
		search(TRUE, curr_buff->curr_line, curr_buff->position, curr_buff->pointer, 0, FALSE, FALSE);
		string_count++;
	}

	wmove(com_win, 0, 0);
	wclrtoeol(com_win);
	wprintw(com_win, fmting_par_msg);
	wrefresh(com_win);

/*
 |	now get back to the start of the paragraph to start formatting
 */

	if (curr_buff->position != 1)
		bol();
	while (!Blank_Line(curr_buff->curr_line->prev_line))
		bol();
	observ_margins = FALSE;

/*
 |	Start going through lines, putting spaces at end of lines if they do 
 |	not already exist.  Append lines together to get one long line, and 
 |	eliminate spacing at begin of lines.
 */

	while (!Blank_Line(curr_buff->curr_line->next_line))
	{
		eol();
		left(TRUE);
		if (*curr_buff->pointer != ' ')
		{
			right(TRUE);
			insert(' ');
		}
		else
			right(TRUE);
		del_char(FALSE);
		if ((*curr_buff->pointer == ' ') || (*curr_buff->pointer == '\t'))
			del_word(FALSE);
	}

/*
 |	Now there is one long line.  Eliminate extra spaces within the line.
 */

	bol();

/*
 |	At the start of the paragraph allow indendentation, so don't 
 |	automatically delete space(s) or tab(s).
 */
	if ((*curr_buff->pointer == ' ') || (*curr_buff->pointer == '\t'))
		adv_word();

	while (curr_buff->position < curr_buff->curr_line->line_length)
	{
		if ((*curr_buff->pointer == ' ') && (*(curr_buff->pointer + 1) == ' '))
			del_char(FALSE);
		else
			right(TRUE);
	}

/*
 |	Now make sure there are two spaces after a '.'.
 */

	bol();
	while (curr_buff->position < curr_buff->curr_line->line_length)
	{
		if ((*curr_buff->pointer == '.') && (*(curr_buff->pointer + 1) == ' '))
		{
			right(TRUE);
			insert(' ');
			insert(' ');
			while (*curr_buff->pointer == ' ')
				del_char(FALSE);
		}
		right(TRUE);
	}

	observ_margins = TRUE;
	bol();

	wmove(com_win, 0, 0);
	wclrtoeol(com_win);
	wprintw(com_win, fmting_par_msg);
	wrefresh(com_win);

/*
 |	make sure that left margin is properly set for start of paragraph
 */

	if ((curr_buff->position < left_margin) && (*curr_buff->pointer != ' ') && (*curr_buff->pointer != '\t'))
	{
		insert(' ');
		delete(TRUE);
	}
	if (*curr_buff->pointer == ' ')
	{
		adv_word();
		if (curr_buff->scr_pos < left_margin)
		{
			insert(' ');
			delete(TRUE);
		}
	}
	if (*curr_buff->pointer == '\t')
	{
		insert(' ');
		delete(TRUE);
		adv_word();
	}

/*
 |	create lines between margins
 */

	while (curr_buff->position < curr_buff->curr_line->line_length)
	{
		while ((curr_buff->scr_pos < right_margin) && (curr_buff->position < curr_buff->curr_line->line_length))
			right(TRUE);
		if (curr_buff->position < curr_buff->curr_line->line_length)
		{
			prev_word();
			if (curr_buff->position == 1)
				adv_word();
			insert_line(TRUE);
		}
	}

/*
 |	go back to begin of paragraph
 */

	bol();
	while (!Blank_Line(curr_buff->curr_line->prev_line))
		bol();
	while (!Blank_Line(curr_buff->curr_line->next_line))
	{
		eol();
/*
 |	eliminate blanks at end of line
 */
		while (curr_buff->pointer[-1] == ' ')
			delete(TRUE);

/*
 |	justify right margin
 */

		orig_count = count = right_margin - curr_buff->scr_pos;
		while ((right_justify) && (count > 0) && (!Blank_Line(curr_buff->curr_line->next_line)))
		{
			while ((count > 0) && (curr_buff->scr_pos > left_margin))
			{
				prev_word();
				if (curr_buff->scr_pos > left_margin)
				{
					insert(' ');
					count--;
				}
			}
			if ((count >= 0) && (curr_buff->scr_pos <= left_margin))
				eol();

			/*
			 |	if there was no place to insert a ' '
			 */
			if (count == orig_count)
				count = 0;
		}
/*
 |	position cursor at end of line so that cursor will go to the end 
 |	of the next line at the top of the loop when eol() is called
 */
		if (curr_buff->curr_line->line_length != curr_buff->position)
			eol();
	}

/*
 |	put cursor back to original position
 */

	bol();
	while (!Blank_Line(curr_buff->curr_line->prev_line))
		bol();

/*
 |	find word cursor was in
 */

	while (string_count > 0)
	{
		search(TRUE, curr_buff->curr_line, curr_buff->position, curr_buff->pointer, 0, FALSE, FALSE);
		string_count--;
	}

/*
 |	offset the cursor to where it was before from the start of the word
 */

	while (offset > 0)
	{
		offset--;
		right(TRUE);
	}

/*
 |	reset flags and strings to what they were before formatting
 */

	if (d_word != NULL)
		free(d_word);
	d_word = temp_dword;
	forward = temp_forward;
	case_sen = temp_case;
	free(srch_str);
	srch_str = tmp_srchstr;
	indent = temp_indent;
	overstrike = temp_over;
	literal = temp_literal;
	d_char = temp_d_char;
	auto_format = temp_af;

	midscreen(curr_buff->scr_vert, curr_buff->position);
}

int
first_word_len(test_line)
struct text *test_line;
{
	int counter;
	char *pnt;

	if (test_line == NULL)
		return(0);

	pnt = test_line->line;
	if ((pnt == NULL) || (*pnt == '\0') || (*pnt == '.') || 
	    (*pnt == '>'))
		return(0);

	if ((*pnt == ' ') || (*pnt == '\t'))
	{
		pnt = next_word(pnt);
	}

	if (*pnt == '\0')
		return(0);

	counter = 0;
	while ((*pnt != '\0') && ((*pnt != ' ') && (*pnt != '\t')))
	{
		pnt++;
		counter++;
	}
	while ((*pnt != '\0') && ((*pnt == ' ') || (*pnt == '\t')))
	{
		pnt++;
		counter++;
	}
	return(counter);
}

void 
Auto_Format()	/* format the paragraph according to set margins	*/
{
	int string_count;
	int offset;
	int temp_case;
	int word_len;
	int temp_dwl;
	int leave_loop = FALSE;
	int status;
	int counter;
	int tmp_d_line_length;
	char temp_literal;
	char *line;
	char *tmp_srchstr;
	char *temp1, *temp2;
	char *temp_dword;
	char temp_d_char = d_char;
	char *tmp_d_line;
	char temp_forward;
	char not_blank;

/*
 |	if line_wrap is not set, or the current line is blank, 
 |	do not format the current paragraph
 */

	if ((!observ_margins) || (Blank_Line(curr_buff->curr_line)) || (mark_text))
		return;

/*
 |	get current position in paragraph, so after formatting, the cursor 
 |	will be in the same relative position
 */

	temp_literal = literal;
	literal = TRUE;
	tmp_d_line = d_line;
	tmp_d_line_length = dlt_line->line_length;
	d_line = NULL;
	auto_format = FALSE;
	temp_forward = forward;
	forward = TRUE;
	offset = curr_buff->position;
	if ((curr_buff->position != 1) && ((*curr_buff->pointer == ' ') || (*curr_buff->pointer == '\t') || (curr_buff->position == curr_buff->curr_line->line_length) || (*curr_buff->pointer == '\0')))
		prev_word();
	temp_dword = d_word;
	temp_dwl = d_wrd_len;
	d_wrd_len = 0;
	d_word = NULL;
	temp_case = case_sen;
	case_sen = TRUE;
	tmp_srchstr = srch_str;
	temp2 = srch_str = (char *) xalloc(1 + curr_buff->curr_line->line_length - curr_buff->position);
	if ((*curr_buff->pointer == ' ') || (*curr_buff->pointer == '\t'))
		adv_word();
	offset -= curr_buff->position;
	counter = curr_buff->position;
	line = temp1 = curr_buff->pointer;
	while ((*temp1 != '\0') && (*temp1 != ' ') && (*temp1 != '\t') && (counter < curr_buff->curr_line->line_length))
	{
		*temp2 = *temp1;
		temp2++;
		temp1++;
		counter++;
	}
	*temp2 = '\0';
	if (curr_buff->position != 1)
		bol();
	while (!Blank_Line(curr_buff->curr_line->prev_line))
		bol();
	string_count = 0;
	status = TRUE;
	while ((line != curr_buff->pointer) && (status))
	{
		status = search(TRUE, curr_buff->curr_line, curr_buff->position, curr_buff->pointer, 0, FALSE, FALSE);
		string_count++;
	}

/*
 |	now get back to the start of the paragraph to start checking
 */

	if (curr_buff->position != 1)
		bol();
	while (!Blank_Line(curr_buff->curr_line->prev_line))
		bol();

/*
 |	Start going through lines, putting spaces at end of lines if they do 
 |	not already exist.  Check line length, and move words to the next line 
 |	if they cross the margin.  Then get words from the next line if they 
 |	will fit in before the margin.  Also count the number of lines that 
 |	are in the paragraph.
 */

	counter = 0;

	while (!leave_loop)
	{
		/*
		 |	make sure that it fits in the left margin
		 */

		if (left_margin > 0)
		{
			if (curr_buff->position != 1)
			{
				bol();
			}
			if ((*curr_buff->pointer == ' ') || (*curr_buff->pointer == '\t'))
				adv_word();
			if (curr_buff->scr_pos < left_margin)
			{
				insert(' ');
				delete(TRUE);
			}
		}

		if (curr_buff->position != curr_buff->curr_line->line_length)
			eol();
		left(TRUE);
		if (*curr_buff->pointer != ' ')
		{
			right(TRUE);
			insert(' ');
		}
		else
			right(TRUE);

		not_blank = FALSE;

		/*
		 |	fill line if first word on next line will fit 
		 |	in the line without crossing the margin
		 */

		while (((word_len = first_word_len(curr_buff->curr_line->next_line)) > 0) 
		      && ((curr_buff->scr_pos + word_len) < right_margin))
		{
			adv_line();
			if ((*curr_buff->pointer == ' ') || (*curr_buff->pointer == '\t'))
				adv_word();
			del_word(FALSE);
			if (curr_buff->position != 1)
				bol();

			/*
			 |	We know this line was not blank before, so 
			 |	make sure that it doesn't have one of the 
			 |	leading characters that indicate the line 
			 |	should not be modified.
			 |
			 |	We also know that this character should not 
			 |	be left as the first character of this line.
			 */

			if ((Blank_Line(curr_buff->curr_line)) && 
			    (curr_buff->curr_line->line[0] != '.') && 
			    (curr_buff->curr_line->line[0] != '>'))
			{
				del_line(FALSE);
				not_blank = FALSE;
			}
			else
				not_blank = TRUE;

			/*
			 |   go to end of previous line
			 */
			left(TRUE);
			undel_word();
			eol();
			/*
			 |   make sure there's a space at the end of the line
			 */
			left(TRUE);
			if (*curr_buff->pointer != ' ')
			{
				right(TRUE);
				insert(' ');
			}
			else
				right(TRUE);
		}

		/*
		 |	make sure line does not cross right margin
		 */

		while (right_margin <= curr_buff->scr_pos)
		{
			prev_word();
			if (curr_buff->position != 1)
			{
				del_word(FALSE);
				if (Blank_Line(curr_buff->curr_line->next_line))
					insert_line(TRUE);
				else
					adv_line();

				if ((*curr_buff->pointer == ' ') || (*curr_buff->pointer == '\t'))
					adv_word();
				undel_word();
				not_blank = TRUE;
				if (curr_buff->position != 1)
					bol();
				left(TRUE);
			}
		}

		if ((!Blank_Line(curr_buff->curr_line->next_line)) || (not_blank))
		{
			adv_line();
			counter++;
		}
		else
			leave_loop = TRUE;
	}

/*
 |	go back to begin of paragraph, put cursor back to original position
 */

	if (curr_buff->position != 1)
		bol();
	while ((counter-- > 0) || (!Blank_Line(curr_buff->curr_line->prev_line)))
		bol();

/*
 |	find word cursor was in
 */

	status = TRUE;
	while ((status) && (string_count > 0))
	{
		status = search(TRUE, curr_buff->curr_line, curr_buff->position, curr_buff->pointer, 0, FALSE, FALSE);
		string_count--;
	}

/*
 |	offset the cursor to where it was before from the start of the word
 */

	while (offset > 0)
	{
		offset--;
		right(TRUE);
	}

	if ((string_count > 0) && (offset < 0))
	{
		while (offset < 0)
		{
			offset++;
			left(TRUE);
		}
	}

/*
 |	reset flags and strings to what they were before formatting
 */

	if (d_word != NULL)
		free(d_word);
	d_word = temp_dword;
	d_wrd_len = temp_dwl;
	case_sen = temp_case;
	free(srch_str);
	srch_str = tmp_srchstr;
	d_char = temp_d_char;
	auto_format = TRUE;
	dlt_line->line_length = tmp_d_line_length;
	d_line = tmp_d_line;

	forward = temp_forward;
	formatted = TRUE;
	literal = temp_literal;
}

