/*
 |	srch_rep.c
 |
 |	$Header: /home/hugh/sources/aee/RCS/srch_rep.c,v 1.9 2009/03/12 02:57:14 hugh Exp hugh $
 */

/*
 |
 |	Copyright (c) 1986 - 1988, 1991 - 1996 Hugh Mahon.
 |
 */

#include "aee.h"

int 
search(move_cursor, start_line, offset, pointer, s_str_off, srch_short, disp)	/* search for srch_str in file after cursor	*/
int move_cursor;/* should the cursor be moved by search to the found string */
struct text *start_line;/* these parameters allow for better meta handling */
int offset;
char *pointer;
int s_str_off;
int srch_short;
int disp;		/* boolean for whether or not to display message */
{
	int end_of_line;
	int iter;	/* position in line of start of suspect string	*/
	int found;
	int found2;
	int iter2;	/* position in line of end of suspect string	*/
	int increment;
	int moved_lines;
	char x;
	char x1, x2;
	char *direction;
	char *srch_1;		/* pointer to start of suspect string	*/
	char *srch_2;		/* pointer to next character of string	*/
	char *srch_3;

	repl_length = 0;
	if (forward)
		direction = "d";
	else
		direction = "u";
	end_of_line = FALSE;
	if ((srch_str == NULL) || (*srch_str == '\0'))
		return(FALSE);
	if ((move_cursor) && (disp))
	{
		wmove(com_win,0,0);
		wclrtoeol(com_win);
		waddstr(com_win, searching_msg);
		if (forward)
			waddstr(com_win, fwd_srch_msg);
		else
			waddstr(com_win, rev_srch_msg);
		wrefresh(com_win);
		clr_cmd_line = TRUE;
	}
	moved_lines = 0;
	found = FALSE;
	srch_line = start_line;
	srch_1 = pointer;
	iter = offset;
	if (!srch_short)
	{
		if ((offset < start_line->line_length) && (forward))
		{
			srch_1++;
			iter = offset + 1;
		}
		else if ((offset > 1) && (!forward))
		{
			srch_1--;
			iter = offset - 1;
		}
		else
			end_of_line = TRUE;
	}
	while ((!found) && (srch_line != NULL))
	{
		while ((!end_of_line) && (!found))
		{
			srch_2 = srch_1;
			iter2 = iter;
			if (literal)
			{
				if (case_sen)	/* if case sensitive	*/
				{
					srch_3 = srch_str;
					while ((*srch_2 == *srch_3) && (*srch_3 != '\0') && (iter2 < srch_line->line_length))
					{
						found = TRUE;
						srch_2++;
						srch_3++;
						iter2++;
					}	/* end while	*/
				}
				else		/* if not case sensitive */
				{
					srch_3 = u_srch_str;
					while ((toupper(*srch_2) == *srch_3) && (*srch_3 != '\0') && (iter2 < srch_line->line_length))
					{
						found = TRUE;
						srch_2++;
						srch_3++;
						iter2++;
					}
				}	/* end else	*/
			}
			else
			{
				if (case_sen)
					srch_3 = srch_str;
				else
					srch_3 = u_srch_str;
				do
				{
					found2 = FALSE;
					if (srch_3[s_str_off] == '$')
					{
						if (iter2 == srch_line->line_length)
						{
							s_str_off++;
							found2 = TRUE;
						}
					}
					else if (srch_3[s_str_off] == '.')
					{
						if (iter2 < srch_line->line_length)
						{
							s_str_off++;
							iter2++;
							if (iter2 <= srch_line->line_length)
								srch_2++;
							found2 = TRUE;
						}
					}
					else if (srch_3[s_str_off] == '*')
					{
						if (iter2 < srch_line->line_length)
						{
							increment = 1;
							if ((srch_3[s_str_off+increment] != '$') && (srch_3[s_str_off+increment] != '['))
							{
								if (search(FALSE, srch_line, iter2, srch_2, (s_str_off+increment), TRUE, disp))
								{
									while (srch_3[s_str_off] != '\0')
										s_str_off++;
									found2 = TRUE;
								}
							}
							found2 = TRUE;
							iter2++;
							if (iter2 <= srch_line->line_length)
								srch_2++;
							else	/* if at end of line, increment srch_3 to exit do...while loop	*/
								s_str_off++;
						}
						else if ((iter2 == srch_line->line_length) && ((srch_3[s_str_off+1] == '$') || (srch_3[s_str_off+1] == '\0')))
						{
							s_str_off++;
							found2 = TRUE;
						}
					}
					else if (srch_3[s_str_off] == '^')
					{
						if (iter == 1)
						{
							found2 = TRUE;
							s_str_off++;
						}
					}
					else if (srch_3[s_str_off] == '[')
					{
						s_str_off++;
						x = '\0';
						if (srch_3[s_str_off] == '^')
						{
							x = '^';
							s_str_off++;
						}
						else if (srch_3[s_str_off] == '\\')
							s_str_off++;
						found2 = FALSE;
						while ((srch_3[s_str_off] != '\0') && (srch_3[s_str_off] != ']') && (!found2))
						{
							if (srch_3[s_str_off] == '\\')
								s_str_off++;
							x1 = srch_3[s_str_off];
							s_str_off++;
							if (srch_3[s_str_off] == '-')
							{
								s_str_off++;
								if (srch_3[s_str_off] == '\\')
									s_str_off++;
								x2 = srch_3[s_str_off];
								s_str_off++;
								if (case_sen)
								{
									if ((*srch_2 >= x1) && (*srch_2 <= x2))
										found2 = TRUE;
								}
								else
								{
									if ((toupper(*srch_2) >= x1) && (toupper(*srch_2) <= x2))
										found2 = TRUE;
								}
							}
							else if (case_sen)
							{
								if (x1 == *srch_2)
									found2 = TRUE;
							}
							else
							{
								if (x1 == toupper(*srch_2))
									found2 = TRUE;
							}
						}
						if (x == '^')
							found2 = !found2;
						if (found2)
						{
							iter2++;
							if (iter2 <= srch_line->line_length)
								srch_2++;
						}
						while ((srch_3[s_str_off] != ']') && (srch_3[s_str_off] != '\0'))
						{
							if (srch_3[s_str_off] == '\\')
								s_str_off++;
							s_str_off++;
						}
						if (srch_3[s_str_off] != '\0')
							s_str_off++;
					}
					else
					{
						if (srch_3[s_str_off] == '\\')
							s_str_off++;
						if (case_sen)
						{
							if (srch_3[s_str_off] == *srch_2)
								found2 = TRUE;
						}
						else
						{
							if (srch_3[s_str_off] == toupper(*srch_2))
								found2 = TRUE;
						}
						s_str_off++;
						iter2++;
						if (iter2 <= srch_line->line_length)
							srch_2++;
					}
				} while ((srch_3[s_str_off] != '\0') && (found2));
				found = found2;
			}
			if (srch_short)
			{
				if (found)
					repl_length = (iter2 - iter) - 1;
				return(found);
			}
			if (!((srch_3[s_str_off] == '\0') && (found)))
			{
				s_str_off = 0;
				found = FALSE;
				if (forward)
				{
					if (iter < srch_line->line_length)
						srch_1++;
					else
						end_of_line = TRUE;
					iter++;
				}
				else
				{
					if (iter > 1)
						srch_1--;
					else
						end_of_line = TRUE;
					iter--;
				}
			}
		}
		if (!found)
		{
			if (forward)
			{
				srch_line = srch_line->next_line;
				if (srch_line != NULL)
					srch_1 = srch_line->line;
				iter = 1;
			}
			else
			{
				srch_line = srch_line->prev_line;
				if (srch_line != NULL)
				{
					srch_1 = srch_line->line;
					iter = 1;
					while (iter < srch_line->line_length)
					{
						iter++;
						srch_1++;
					}
				}
			}
			end_of_line = FALSE;
			moved_lines++;
		}
	}
	start_of_string = srch_1;
	lines_moved = moved_lines;
	if (found)
	{
		repl_length += 1 + (iter2 - iter);
		if (move_cursor)
		{
			if (disp)
			{
				wmove(com_win, 0, 0);
				wclrtoeol(com_win);
				clr_cmd_line = FALSE;
				status_display();
				wrefresh(com_win);
			}
			if (lines_moved != 0)
				move_rel(direction, lines_moved);
			if ((lines_moved != 0) || (forward))
			{
				while (curr_buff->position < iter)
					right(TRUE);
			}
			else if ((lines_moved == 0) && (!forward))
			{
				while (curr_buff->position > iter)
					left(TRUE);
			}
		}
		else
			tmp_pos = iter;
	}
	else
	{
		if (disp)
		{
			wmove(com_win,0,0);
			wclrtoeol(com_win);
			wprintw(com_win, str_str);
			iter2 = curr_buff->scr_horz;
	 		for (srch_3 = srch_str; *srch_3 != '\0'; srch_3++)
			{
				if ((*srch_3 >= 32) && (*srch_3 < 127))
				{
					iter2++;
					waddch(com_win, *srch_3);
				}
				else
					iter2 += out_char(com_win, *srch_3, iter2, 0, 0);
			}
			wprintw(com_win, not_fnd_str);
			wrefresh(com_win);
			wmove(curr_buff->win, curr_buff->scr_vert,curr_buff->scr_horz);
		}
	}
	return(found);
}

int 
upper(value)		/* convert character to upper case		*/
char value;
{
	char value2;

	if ((value >= 'a') && (value <= 'z'))
		value2 = value - ('a' - 'A');
	else 
		value2 = value;
	return(value2);
}

int 
search_prompt(flag)	/* prompt and read search string (srch_str)	*/
int flag;
{
	char *tmp_srch;
	char *srch_1;
	char *srch_3;

#ifndef XAE
	tmp_srch = get_string(search_prompt_str, FALSE);
#else
	if (flag)
		tmp_srch = get_string(search_prompt_str, FALSE);
	else
	{
		tmp_srch = xalloc(strlen(xsrch_string) + 1);
		copy_str(xsrch_string, tmp_srch);
	}
#endif	/* ifndef XAE	*/
	if (*tmp_srch != '\0')
	{
		if (srch_str != NULL) 
			free(srch_str);
		srch_3 = srch_str = tmp_srch;
		if (u_srch_str != NULL) 
			free(u_srch_str);
		srch_1 = u_srch_str = xalloc(strlen(srch_str) + 1);
		while (*srch_3 != '\0')		/* make upper case version of string */
		{
			*srch_1 = toupper(*srch_3);
			srch_1++;
			srch_3++;
		}
		*srch_1 = '\0';
		if (strlen(srch_str) >= 1)
			value = search(TRUE, curr_buff->curr_line, curr_buff->position, curr_buff->pointer, 0, FALSE, TRUE);
	}
	return(0);
}

void 
replace()		/* replace the given one string with another	*/
{
	int counter;
	int i, j;
	int chng_all;		/* change all occurrences		*/
	int go_on;		/* continue to replace			*/
	int t_vert;		/* temporary vertical position		*/
	int t_horz;		/* temporary horizontal position	*/
	int s_flag;		/* temporary select flag		*/
	int t_posit;		/* temporary position			*/
	int t_pos;		/* temporary abs_pos			*/
	int temp_flag;		/* temp storage for value of overstrike	*/
	int temp_abs_line;	/* keep track of original absolute line	*/
	char *pnt;		/* temporary pointer			*/
	char *t_srch_str;	/* temp search string			*/
	char *ut_srch_str;	/* upper case search string		*/
	char *response;		/* response from user			*/
	char *del_strg;		/* string deleted from line		*/
	char res;		/* value to hold response		*/
	struct text *t_line;	/* temp pointer to hold curr_buff->curr_line	*/

/*	continue to look for occurrences while go_on is true,
	if chng_all is true, do not prompt user whether or not to change
*/

	temp_abs_line = curr_buff->absolute_lin;
	temp_flag = overstrike;
	overstrike = FALSE;
	s_flag = mark_text;
	mark_text = FALSE;
	t_pos = curr_buff->scr_pos;
	t_srch_str = srch_str;
	ut_srch_str = u_srch_str;
	t_posit = curr_buff->position;
	t_line = curr_buff->curr_line;
	t_vert = curr_buff->scr_vert;
	t_horz = curr_buff->scr_horz;
	go_on = TRUE;
	u_srch_str = u_old_string;
	srch_str = old_string;
	chng_all = FALSE;
	while (go_on)
	{
		if (search(TRUE, curr_buff->curr_line, curr_buff->position, curr_buff->pointer, 0, FALSE, TRUE))
		{
			wstandout(curr_buff->win);
			if (curr_buff->position == curr_buff->curr_line->line_length)
				waddch(curr_buff->win, ' ');
			else if ((*curr_buff->pointer >= ' ') && (*curr_buff->pointer < 127))
				waddch(curr_buff->win, *curr_buff->pointer);
			else
				value = out_char(curr_buff->win, *curr_buff->pointer, curr_buff->scr_pos, curr_buff->scr_vert, 0);
			wstandend(curr_buff->win);
			wrefresh(curr_buff->win);
			if (!chng_all)
			{
				response = get_string(replace_action_prompt, TRUE);
				if (toupper(*response) == toupper(*replace_all_char))
					chng_all = TRUE;
				else if (toupper(*response) == toupper(*quit_char))
					go_on = FALSE;
				res = *response;
				free(response);
			}
			wmove(curr_buff->win, curr_buff->scr_vert, curr_buff->scr_horz);
			if ((*curr_buff->pointer >= ' ') && (*curr_buff->pointer < 127))
				waddch(curr_buff->win, *curr_buff->pointer);
			else
				value = out_char(curr_buff->win, *curr_buff->pointer, curr_buff->scr_pos, curr_buff->scr_vert, 0);
			if (((toupper(res) == toupper(*replace_r_char)) || 
			      (go_on) || (res == '\0')) && 
			      (toupper(res) != toupper(*replace_skip_char)))
			{
				del_strg = del_string(repl_length - 1);
				counter = strlen(new_string) + 1;
				pnt = new_string;
				for (i = 1; i < counter; i++)
				{
					if ((!literal) && (*pnt == '&'))
					{
						undel_string(del_strg, (repl_length - 1));
						for (j = 1; j < repl_length; j++)
							right(TRUE);
					}
					else if ((!literal) && ((*pnt == '\\') && (pnt[1] == '&')))
					{
						i++;
						pnt++;
						insert(*pnt);
					}
					else
						insert(*pnt);
					pnt++;
				}
				if (!forward)
				{
					for (i = 1; i < counter; i++)
						left(TRUE);
				}
				free(del_strg);
			}
		}
		else
			go_on = FALSE;
	}
	u_srch_str = ut_srch_str;
	srch_str = t_srch_str;
	curr_buff->curr_line = t_line;
	curr_buff->pointer = curr_buff->curr_line->line;
	for (value = 1; value < t_posit; value++)
		curr_buff->pointer++;
	curr_buff->position = t_posit;
	curr_buff->abs_pos = curr_buff->scr_pos = t_pos;
	curr_buff->scr_vert = t_vert;
	curr_buff->scr_horz = t_horz;
	mark_text = s_flag;
	midscreen(curr_buff->scr_vert, curr_buff->position);
	werase(com_win);
	wrefresh(com_win);
	overstrike = temp_flag;
	curr_buff->absolute_lin = temp_abs_line;
}

int 
repl_prompt(flag)	/* prompt for replace parameters		*/
int flag;
{
	char *rparam;
	char *rp;
	char *start;
	char *upt;
	char delimiter;
	int last_char;
	int counter;

#ifndef XAE
	rp = rparam = get_string(replace_prompt_str, TRUE);
#else
	if (flag)
		rp = rparam = get_string(replace_prompt_str, TRUE);
	else
	{
		counter = strlen(xnew_string) + strlen(xold_string) + 2;
		if (counter > 2)
		{
			rp = rparam = xalloc(counter + 3);
			*rp = '\33';
			rp++;
			copy_str(xold_string, rp);
			while (*rp != '\0')
				rp++;
			*rp = '\33';
			rp++;
			copy_str(xnew_string, rp);
			while (*rp != '\0')
				rp++;
			*rp = '\33';
			rp++;
			*rp = '\0';
			rp = rparam;
		}
		else
			rp = "\000";
	}
#endif	/* ifndef XAE	*/
	delimiter = *rp;
	if (*rp != '\0')
	{
			++rp;
		start = rp;
		counter = 1;
		while ((*rp != delimiter) && (*rp != '\0'))
		{
			counter++;
			rp++;
		}
		if (*rp == '\0')
			last_char = TRUE;
		else
		{
			last_char = FALSE;
			*rp = '\0';
		}
		if (old_string != NULL)
		{
			free(old_string);
			free(new_string);
			free(u_old_string);
		}
		old_string = xalloc(counter);
		u_old_string = xalloc(counter);
		copy_str(start, old_string);
		upt = u_old_string;
		while (*start != '\0')
		{
			*upt = toupper(*start);
			upt++;
			start++;
		}
		*upt = '\0';
		if (!last_char)
			start = ++rp;
		counter = 1;
		while ((*rp != delimiter) && (*rp != '\0'))
		{
			counter++;
			rp++;
		}
		*rp = '\0';
		new_string = xalloc(counter);
		copy_str(start, new_string);
		free(rparam);
		if (strlen(old_string) >= 1)
			replace();
	}
	return(0);
}

void 
match()		/* find the matching (), {}, or []	*/
{
	int level;		/* level of nesting			*/
	int t_vert;		/* temporary vertical position		*/
	int t_horz;		/* temporary horizontal position	*/
	int s_flag;		/* temporary select flag		*/
	int t_posit;		/* temporary position			*/
	int t_pos;		/* temporary abs_pos			*/
	int mforward;		/* temporary storage of true forward	*/
	int mliteral;		/* temp storage of true literal value	*/
	int total_lines;	/* sum of lines moved			*/
	int temp_abs_line;	/* keep track of original line		*/
	char curr_char;		/* the current character 		*/
	char *t_srch_str;	/* temp search string			*/
	char *ut_srch_str;	/* upper case search string		*/
	char *direction;	/* direction in which to move		*/
	struct text *t_line;	/* temp pointer to hold curr_buff->curr_line	*/

	if ((*curr_buff->pointer == '{') || (*curr_buff->pointer == '}') || (*curr_buff->pointer == '(') || (*curr_buff->pointer == ')') || (*curr_buff->pointer == '[') || (*curr_buff->pointer == ']') || (*curr_buff->pointer == '<') || (*curr_buff->pointer == '>'))
	{
		temp_abs_line = curr_buff->absolute_lin;
		curr_char = *curr_buff->pointer;
		mliteral = literal;
		mforward = forward;
		literal = FALSE;
		if ((*curr_buff->pointer == '{') || (*curr_buff->pointer == '(') || (*curr_buff->pointer == '[') || (*curr_buff->pointer == '<'))  /* provide matches for quoted characters - > ] ) } 	*/
		{
			direction = "d";
			forward = TRUE;
		}
		else
		{
			direction = "u";
			forward = FALSE;
		}
		s_flag = mark_text;
		mark_text = FALSE;
		t_pos = curr_buff->scr_pos;
		t_srch_str = srch_str;
		ut_srch_str = u_srch_str;
		t_posit = curr_buff->position;
		t_line = curr_buff->curr_line;
		t_vert = curr_buff->scr_vert;
		t_horz = curr_buff->scr_horz;
		if ((curr_char == '{') || (curr_char == '}'))
			srch_str = "[{}]";
		if ((curr_char == '(') || (curr_char == ')'))
			srch_str = "[()]";
		if ((curr_char == '<') || (curr_char == '>'))
			srch_str = "[<>]";
		if ((curr_char == '[') || (curr_char == ']'))
			srch_str = "[\\[\\]]";
		u_srch_str = srch_str;
		level = 1;
		total_lines = 0;
		while ((level > 0) && (search(FALSE, curr_buff->curr_line, curr_buff->position, curr_buff->pointer, 0, FALSE, TRUE)))
		{
			total_lines += lines_moved;
			curr_buff->curr_line = srch_line;
			curr_buff->pointer = start_of_string;
			curr_buff->position = tmp_pos;
			if (*start_of_string == curr_char)
				level++;
			else
				level--;
		}
		lines_moved = total_lines;
		curr_buff->curr_line = t_line;
		curr_buff->pointer = curr_buff->curr_line->line;
		for (value = 1; value < t_posit; value++)
			curr_buff->pointer++;
		curr_buff->position = t_posit;
		curr_buff->abs_pos = curr_buff->scr_pos = t_pos;
		curr_buff->scr_vert = t_vert;
		curr_buff->scr_horz = t_horz;
		mark_text = s_flag;
		if (level == 0)
		{
			if (lines_moved != 0)
				move_rel(direction, lines_moved);
			if ((lines_moved != 0) || (forward))
			{
				while (curr_buff->position < tmp_pos)
					right(TRUE);
			}
			else if ((lines_moved == 0) && (!forward))
			{
				while (curr_buff->position > tmp_pos)
					left(TRUE);
			}
		}
		else
		{
			clr_cmd_line = TRUE;
			wmove(com_win, 0, 0);
			wclrtoeol(com_win);
			wprintw(com_win, cant_find_match_str);
			wrefresh(com_win);
			curr_buff->absolute_lin = temp_abs_line;
		}
		u_srch_str = ut_srch_str;
		srch_str = t_srch_str;
		literal = mliteral;
		forward = mforward;
	}
}

