/*
 |	control.c
 |
 |	$Header: /home/hugh/sources/aee/RCS/control.c,v 1.50 2010/07/18 00:56:20 hugh Exp hugh $
 */

/*
 |	Copyright (c) 1986 - 1988, 1991 - 2000 - 2002, 2009, 2010 Hugh Mahon.
 */

#include "aee.h"

static char item_alpha[] = "abcdefghijklmnopqrstuvwxyz0123456789 ";

#define max_alpha_char 36

int 
menu_op(menu_list)
struct menu_entries menu_list[];
{
	WINDOW *temp_win;
	int max_width, max_height;
	int x_off, y_off;
	int counter;
	int length;
	int input;
	int temp;
	int list_size;
	int top_offset;		/* offset from top where menu items start */
	int vert_pos;		/* vertical position			  */
	int vert_size;		/* vertical size for menu list item display */
	int off_start = 1;	/* offset from start of menu items to start display */


	/*
	 |	determine number and width of menu items
	 */

	list_size = 1;
	while (menu_list[list_size + 1].item_string != NULL)
		list_size++;
	max_width = 0;
	for (counter = 0; counter <= list_size; counter++)
	{
		if ((length = strlen(menu_list[counter].item_string)) > max_width)
			max_width = length;
	}
	max_width += 3;
	max_width = max(max_width, strlen(cancel_string));
	max_width = max(max_width, max(strlen(more_above_str), strlen(more_below_str)));
	max_width += 6;

	/*
	 |	make sure that window is large enough to handle menu
	 |	if not, print error message and return to calling function
	 */

	if ((max_width > COLS) || ((list_size > LINES) && (LINES < 3)))
	{
		wmove(com_win, 0, 0);
		werase(com_win);
		wprintw(com_win, menu_too_lrg_msg);
		wrefresh(com_win);
		clr_cmd_line = TRUE;
		return(0);
	}

	top_offset = 0;

	if (list_size > LINES)
	{
		max_height = LINES;
		if (max_height > 11)
			vert_size = max_height - 8;
		else
			vert_size = max_height;
	}
	else
	{
		vert_size = list_size;
		max_height = list_size;
	}

	if (LINES >= (vert_size + 8))
	{
		if (menu_list[0].argument != MENU_WARN)
			max_height = vert_size + 8;
		else
			max_height = vert_size + 7;
		top_offset = 4;
	}
	x_off = (COLS - max_width) / 2;
	y_off = (LINES - max_height - 1) / 2;
	temp_win = newwin(max_height, max_width, y_off, x_off);
	keypad(temp_win, TRUE);

	paint_menu(menu_list, max_width, max_height, list_size, top_offset, temp_win, off_start, vert_size);

	counter = 1;
	vert_pos = 0;
	do
	{
		if (off_start > 2)
			wmove(temp_win, (1 + counter + top_offset - off_start), 3);
		else
			wmove(temp_win, (counter + top_offset - off_start), 3);

		wrefresh(temp_win);
		get_input(temp_win);
		input = in;
		if (input == -1)
			exit(0);

		if (((tolower(input) >= 'a') && (tolower(input) <= 'z')) || 
		    ((input >= '0') && (input <= '9')))
		{
			if ((tolower(input) >= 'a') && (tolower(input) <= 'z'))
			{
				temp = 1 + tolower(input) - 'a';
			}
			else if ((input >= '0') && (input <= '9'))
			{
				temp = (2 + 'z' - 'a') + (input - '0');
			}

			if (temp <= list_size)
			{
				input = '\n';
				counter = temp;
			}
		}
		else
		{		
			switch (input)
			{
				case ' ':	/* space	*/
				case '\004':	/* ^d, down	*/
				case KEY_RIGHT:
				case KEY_DOWN:
					counter++;
					if (counter > list_size)
						counter = 1;
					break;
				case '\010':	/* ^h, backspace*/
				case '\025':	/* ^u, up	*/
				case 127:	/* ^?, delete	*/
				case KEY_BACKSPACE:
				case KEY_LEFT:
				case KEY_UP:
					counter--;
					if (counter == 0)
						counter = list_size;
					break;
				case '\033':	/* escape key	*/
					if (menu_list[0].argument != MENU_WARN)
						counter = 0;
					break;
				case '\014':	/* ^l       	*/
				case '\022':	/* ^r, redraw	*/
					paint_menu(menu_list, max_width, max_height, 
						list_size, top_offset, temp_win, 
						off_start, vert_size);
					break;
				default:
					break;
			}
		}
	
		if (((list_size - off_start) >= (vert_size - 1)) && 
			(counter > (off_start + vert_size - 3)) && 
				(off_start > 1))
		{
			if (counter == list_size)
				off_start = (list_size - vert_size) + 2;
			else
				off_start++;

			paint_menu(menu_list, max_width, max_height, 
				   list_size, top_offset, temp_win, off_start, 
				   vert_size);
		}
		else if ((list_size != vert_size) && 
				(counter > (off_start + vert_size - 2)))
		{
			if (counter == list_size)
				off_start = 2 + (list_size - vert_size);
			else if (off_start == 1)
				off_start = 3;
			else
				off_start++;

			paint_menu(menu_list, max_width, max_height, 
				   list_size, top_offset, temp_win, off_start, 
				   vert_size);
		}
		else if (counter < off_start)
		{
			if (counter <= 2)
				off_start = 1;
			else
				off_start = counter;

			paint_menu(menu_list, max_width, max_height, 
				   list_size, top_offset, temp_win, off_start, 
				   vert_size);
		}
	}
	while ((input != '\r') && (input != '\n') && (counter != 0));

	werase(temp_win);
	wrefresh(temp_win);
	delwin(temp_win);

	if ((menu_list[counter].procedure != NULL) || 
	    (menu_list[counter].iprocedure != NULL) || 
	    (menu_list[counter].nprocedure != NULL))
	{
		if (menu_list[counter].argument != -1)
			(*menu_list[counter].iprocedure)(menu_list[counter].argument);
		else if (menu_list[counter].ptr_argument != NULL)
			(*menu_list[counter].procedure)(menu_list[counter].ptr_argument);
		else
			(*menu_list[counter].nprocedure)();
	}

	if (info_window)
		paint_info_win();
	redraw();

	return(counter);
}

void 
paint_menu(menu_list, max_width, max_height, list_size, top_offset, menu_win, 
	   off_start, vert_size)
struct menu_entries menu_list[];
int max_width, max_height, list_size, top_offset;
WINDOW *menu_win;
int off_start, vert_size;
{
	int counter, temp_int;

	werase(menu_win);

	/*
	 |	output top and bottom portions of menu box only if window 
	 |	large enough 
	 */

	if (max_height > vert_size)
	{
		wmove(menu_win, 1, 1);
		if (!nohighlight)
			wstandout(menu_win);
		waddch(menu_win, '+');
		for (counter = 0; counter < (max_width - 4); counter++)
			waddch(menu_win, '-');
		waddch(menu_win, '+');

		wmove(menu_win, (max_height - 2), 1);
		waddch(menu_win, '+');
		for (counter = 0; counter < (max_width - 4); counter++)
			waddch(menu_win, '-');
		waddch(menu_win, '+');
		wstandend(menu_win);
		wmove(menu_win, 2, 3);
		waddstr(menu_win, menu_list[0].item_string);
		wmove(menu_win, (max_height - 3), 3);
		if (menu_list[0].argument != MENU_WARN)
			waddstr(menu_win, cancel_string);
	}
	if (!nohighlight)
		wstandout(menu_win);

	for (counter = 0; counter < (vert_size + top_offset); counter++)
	{
		if (top_offset == 4)
		{
			temp_int = counter + 2;
		}
		else
			temp_int = counter;

		wmove(menu_win, temp_int, 1);
		waddch(menu_win, '|');
		wmove(menu_win, temp_int, (max_width - 2));
		waddch(menu_win, '|');
	}
	wstandend(menu_win);

	if (list_size > vert_size)
	{
		if (off_start >= 3)
		{
			temp_int = 1;
			wmove(menu_win, top_offset, 3);
			waddstr(menu_win, more_above_str);
		}
		else
			temp_int = 0;

		for (counter = off_start; 
			((temp_int + counter - off_start) < (vert_size - 1));
				counter++)
		{
			wmove(menu_win, (top_offset + temp_int + 
						(counter - off_start)), 3);
			if (list_size > 1)
				wprintw(menu_win, "%c) ", item_alpha[min((counter - 1), max_alpha_char)]);
			waddstr(menu_win, menu_list[counter].item_string);
		}

		wmove(menu_win, (top_offset + (vert_size - 1)), 3);

		if (counter == list_size)
		{
			if (list_size > 1)
				wprintw(menu_win, "%c) ", item_alpha[min((counter - 1), max_alpha_char)]);
			wprintw(menu_win, menu_list[counter].item_string);
		}
		else
			wprintw(menu_win, more_below_str);
	}
	else
	{
		for (counter = 1; counter <= list_size; counter++)
		{
			wmove(menu_win, (top_offset + counter - 1), 3);
			if (list_size > 1)
				wprintw(menu_win, "%c) ", item_alpha[min((counter - 1), max_alpha_char)]);
			waddstr(menu_win, menu_list[counter].item_string);
		}
	}
}

void 
shell_op()
{
	char *string;

	if (((string = get_string(shell_cmd_prompt, TRUE)) != NULL) && 
			(*string != '\0'))
	{
		sh_command(string);
		free(string);
	}
}

void 
leave_op()
{
	int index;

	if ((num_of_bufs > 1) && (top_of_stack == NULL))
	{
		index = menu_op(leave_menu_2);
		switch(index)
		{
			case 2:	index = delete_all_buffers();
				if (index == FALSE)
					return;
				break;
			case 1:
			default:
				repaint_screen();
				return;
		}
	}
	if (first_buff->changed)
	{
		index = menu_op(leave_menu);
		repaint_screen();
		switch (index) 
		{
			case 1:	finish(EXIT_str);
				break;
			case 2:	first_buff->changed = FALSE;
				quit(QUIT_str);
				break;
			default: return;
		}
	}
	else
	{
		repaint_screen();
		quit(QUIT_str);
	}
}

void 
spell_op()	/* check spelling of words in the editor	*/
{
	if (restrict_mode())
	{
		return;
	}

	top();			/* go to top of file		*/
	insert_line(FALSE);	/* create two blank lines	*/
	insert_line(FALSE);
	top();
	command(shell_echo_msg);
	adv_line();
	wmove(com_win, 0, 0);
	wprintw(com_win, spell_in_prog_msg);
	wrefresh(com_win);
	command("<>!spell");	/* send contents of buffer to command 'spell' 
				   and read the results back into the editor */
}

void 
ispell_op()
{
	char name[128];
	char string[256];
	int pid;

	if (restrict_mode())
	{
		return;
	}

	pid = getpid();
	sprintf(name, "/tmp/ae.%d", pid);
	if (write_file(name))
	{
		sprintf(string, "ispell %s", name);
		sh_command(string);
		delete_text();
		tmp_file = name;
		recv_file = TRUE;
		check_fp();
		unlink(name);
	}
}

void 
modes_op()
{
	int ret_value;
	int counter;
	char *string;
	int temp_int;

	modes_menu[17].item_string = mode_strings[17];

	do
	{
		sprintf(modes_menu[1].item_string, "%s %s", mode_strings[1], 
					(expand ? ON : OFF));
		sprintf(modes_menu[2].item_string, "%s %s", mode_strings[2], 
					(case_sen ? ON : OFF));
		sprintf(modes_menu[3].item_string, "%s %s", mode_strings[3], 
					( literal ? ON : OFF));
		sprintf(modes_menu[4].item_string, "%s %s", mode_strings[4], 
					(forward ? "forward" : "reverse"));
		sprintf(modes_menu[5].item_string, "%s %s", mode_strings[5], 
					(observ_margins ? ON : OFF));
		sprintf(modes_menu[6].item_string, "%s %s", mode_strings[6], 
					( info_window ? ON : OFF));
		sprintf(modes_menu[7].item_string, "%s %s", mode_strings[7], 
					( status_line ? ON : OFF));
		sprintf(modes_menu[8].item_string, "%s %s", mode_strings[8], 
					( indent ? ON : OFF));
		sprintf(modes_menu[9].item_string, "%s %s", mode_strings[9], 
					( overstrike ? ON : OFF));
		sprintf(modes_menu[10].item_string, "%s %s", mode_strings[10], 
					( auto_format ? ON : OFF));
		sprintf(modes_menu[11].item_string, "%s %s", mode_strings[11], 
					( windows ? ON : OFF));
		sprintf(modes_menu[12].item_string, "%s %3d", mode_strings[12], 
					left_margin);
		sprintf(modes_menu[13].item_string, "%s %3d", mode_strings[13], 
					right_margin);
		sprintf(modes_menu[14].item_string, "%s %3d", mode_strings[14], 
					(info_win_height - 1));
		sprintf(modes_menu[15].item_string, "%s %s", mode_strings[15], 
					(text_only ? text_msg : binary_msg ));
		sprintf(modes_menu[16].item_string, "%s %s", mode_strings[16], 
					(curr_buff->dos_file ? dos_msg : unix_msg ));

		ret_value = menu_op(modes_menu);

		switch (ret_value) 
		{
			case 1:
				expand = !expand;
				break;
			case 2:
				case_sen = !case_sen;
				break;
			case 3:
				literal = !literal;
				break;
			case 4:
				forward = !forward;
				break;
			case 5:
				observ_margins = !observ_margins;
				break;
			case 6:
				info_op();
				break;
			case 7:
				status_line = !status_line;
				break;
			case 8:
				indent = !indent;
				break;
			case 9:
				overstrike = !overstrike;
				break;
			case 10:
				auto_format = !auto_format;
				if (auto_format)
				{
					observ_margins = TRUE;
					indent = FALSE;
				}
				break;
			case 11:
				if (!windows)
					make_win();
				else
					no_windows();
				break;
			case 12:
				string = get_string(left_marg_prompt, TRUE);
				if (string != NULL)
				{
					counter = atoi(string);
					if (counter < right_margin)
						left_margin = counter;
					else
					{
						wmove(com_win, 0, 0);
						werase(com_win);
						wprintw(com_win, left_mrgn_err_msg);
						wrefresh(com_win);
					}
					free(string);
				}
				break;
			case 13:
				string = get_string(right_mrgn_prompt, TRUE);
				if (string != NULL)
				{
					counter = atoi(string);
					if (counter > left_margin)
						right_margin = counter;
					else
					{
						wmove(com_win, 0, 0);
						werase(com_win);
						wprintw(com_win, right_mrgn_err_msg);
						wrefresh(com_win);
					}
					free(string);
				}
				break;
			case 14:
				temp_int = info_win_height;
				string = get_string(info_win_height_prompt, TRUE);
				if (string != NULL)
				{
					counter = atoi(string);
					if ((counter > 0) && (counter <= MAX_HELP_LINES))
						info_win_height = counter + 1;
					else
					{
						wmove(com_win, 0, 0);
						werase(com_win);
						wprintw(com_win, info_win_height_err);
						wrefresh(com_win);
					}
					free(string);
					if ((info_win_height != temp_int) && (info_window))
					{
						redo_win();
						curr_buff->last_line = curr_buff->lines - 1;
						new_screen();
						paint_info_win();
						redraw();
					}
				}
				break;
			case 15:
				text_only = !text_only;
				break;
			case 16:
				if (text_only)
					curr_buff->dos_file = !curr_buff->dos_file;
				break;
			case 17:
				dump_aee_conf();
				break;
			default:
				break;
		}
	}
	while (ret_value != 0);
}

void 
search_op()
{
	search(TRUE, curr_buff->curr_line, curr_buff->position, curr_buff->pointer, 0, FALSE, TRUE);
}


int 
file_op(arg)
int arg;
{
	char *string;
	int flag;

	if (arg == READ_FILE)
	{
		if (restrict_mode())
		{
			return(1);
		}

		string = get_string(file_read_prompt_str, TRUE);
		recv_file = TRUE;
		tmp_file = resolve_name(string);
		check_fp();
		if (tmp_file != string)
			free(tmp_file);
		free(string);
	}
	else if (arg == WRITE_FILE)
	{
		if (restrict_mode())
		{
			return(1);
		}

		string = get_string(file_write_prompt_str, TRUE);
		tmp_file = resolve_name(string);
		write_file(tmp_file);
		if (tmp_file != string)
			free(tmp_file);
		free(string);
	}
	else if (arg == SAVE_FILE)
	{
	/*
	 |	changes made here should be reflected in finish() and 
	 |	in command() where SAVE_str is handled.
	 */

		if (!curr_buff->edit_buffer)
			chng_buf(main_buffer_name);

		if (!curr_buff->changed)
		{
			wmove(com_win, 0, 0);
			wprintw(com_win, no_chng_no_save );
			wclrtoeol(com_win);
			wrefresh(com_win);
			return(0);
		}

		string = curr_buff->full_name;

		if ((string != NULL) && (*string != '\0'))
			flag = TRUE;
		else
			flag = FALSE;

		if ((string == NULL) || (*string == '\0'))
		{
			if (restrict_mode())
			{
				return(1);
			}

			string = get_string(save_file_name_prompt, TRUE);
		}
		if ((string == NULL) || (*string == '\0'))
		{
			wmove(com_win, 0, 0);
			wprintw(com_win, file_not_saved_msg );
			wclrtoeol(com_win);
			wrefresh(com_win);
			clr_cmd_line = TRUE;
			return(0);
		}

		if (!flag)
		{
			tmp_file = resolve_name(string);
			if (tmp_file != string)
			{
				free(string);
				string = tmp_file;
			}
		}

		if (write_file(string))
		{
			if (curr_buff->file_name == NULL)
			{
				curr_buff->full_name = get_full_path(string, 
							NULL);
				curr_buff->file_name = 
					ae_basename(curr_buff->full_name);
			}
			curr_buff->changed = FALSE;
			change = FALSE;
		}
		else 
		{
			if (!flag)
				free(string);
			return(1);
		}
	}
	return(0);
}

void 
info_op()
{
	info_window = !info_window;
	redo_win();
	curr_buff->last_line = curr_buff->lines - 1;
	new_screen();
	if (info_window)
		paint_info_win();
	redraw();
}

int 
macro_assign(keys, macro_string)
char *keys[];
char *macro_string;
{
	int counter;
	char *temp;

	for (counter = 0; counter < 32; counter++)
	{
		temp = keys[counter];
		if (compare(temp, macro_string, FALSE))
		{
			temp = next_word(temp);
			if (*temp == '\0')
				return(counter);
		}
	}
	return(-1);
}

void 
get_key_assgn()
{
	int counter;
	int local_index;

	assignment[gold_key_index].ckey = 0;
	if ((local_index = macro_assign(ctr, assignment[gold_key_index].macro)) != -1)
	{
		assignment[gold_key_index].ckey = local_index;
	}
	for (counter = 1; counter < gold_key_index; counter++)
	{
		assignment[counter].ckey = -1;
		assignment[counter].gckey = -1;
		if ((local_index = macro_assign(ctr, assignment[counter].macro)) != -1)
		{
			assignment[counter].ckey = local_index;
		}
		if (((local_index = macro_assign(g_ctr, assignment[counter].macro)) != -1) && (assignment[gold_key_index].ckey))
		{
			assignment[counter].gckey = local_index;
		}
	}
	paint_information();
}

void 
paint_information()
{
	int counter;
	int local_index;
	int line_index;
	int column, width;
	char buffer[64];

	for (counter = 0; counter < (info_win_height - 1); counter++)
	{
		info_data[counter][0] = '\0';
	}

	width = 0;
	column = 0;
	for (counter = 0, local_index = 1; 
		((width + column) <= min(COLS, MAX_HELP_COLS)) && 
			(local_index < (gold_key_index + 1)) &&
			(assignment[local_index].macro != NULL); 
						counter++, local_index++)
	{
		line_index = counter % (info_win_height - 1);
		if (line_index == 0)
			column += width;
		while ((assignment[local_index].ckey < 0) && 
				(assignment[local_index].gckey < 0) &&
				(assignment[local_index].macro != NULL))
			local_index++;
		if (local_index > gold_key_index)
			break;
		if (assignment[local_index].ckey >= 0)
		{
			if (assignment[local_index].ckey  == 26) 
						/* corresponds with Escape */
				sprintf(buffer, "Esc  %s ", 
					assignment[local_index].description);
			else
				sprintf(buffer, "^%c   %s ", 
					(assignment[local_index].ckey + 'A'), 
					assignment[local_index].description);
		}
		else if (assignment[local_index].gckey > 0)
			sprintf(buffer, "^%c^%c %s ", 
				(assignment[gold_key_index].ckey + 'A'), 
				(assignment[local_index].gckey + 'A'), 
				assignment[local_index].description);

		width = max(width, strlen(buffer));
		if ((width + column) <= min(COLS, MAX_HELP_COLS))
			strcat(info_data[line_index], buffer);
	}
}

void 
paint_info_win()
{
	int counter;
	int width, column;
	int index;

	if ((!info_window) || (info_win == 0))
		return;

        werase(info_win);

	if (info_type == COMMANDS)
	{
		index = 0;
		for (column = 0; column < COLS && (commands[index] != NULL); 
							  column += width + 1)
		{
			width = 0;
			for (counter = 0; 
				counter < (info_win_height - 1) && 
					(commands[index] != NULL); 
								counter++)
			{
				width = max(width, strlen(commands[index]));
				if ((width + column) < COLS)
				{
					wmove(info_win, counter, column);
					waddstr(info_win, commands[index]);
				}
				index++;
			}
		}
	}
	else if (info_type == CONTROL_KEYS)
	{
		for (counter = 0; counter < (info_win_height - 1); counter++)
		{
			wmove(info_win, counter, 0);
			wclrtoeol(info_win);
			waddstr(info_win, info_data[counter]);
		}
	}
	wmove(info_win, (info_win_height - 1), 0);
	if (!nohighlight)
		wstandout(info_win);

	wprintw(info_win, info_help_msg);

	for (counter = strlen(info_help_msg); counter < COLS; counter++)
		waddch(info_win, '=');

	wstandend(info_win);
	wrefresh(info_win);
}

/*
 |	The following routine tests the input string against the list of 
 |	strings, to determine if the string is a unique match with one of the 
 |	valid values.
 */

int 
unique_test(string, list)
char *string;
char *list[];
{
	int counter;
	int num_match;
	int result;

	num_match = 0;
	counter = 0;
	while (list[counter] != NULL)
	{
		result = compare(string, list[counter], FALSE);
		if (result)
			num_match++;
		counter++;
	}
	return(num_match);
}

void 
command_prompt()
{
	char *cmd_str;
	int result;

	info_type = COMMANDS;
	paint_info_win();
	cmd_str = get_string(cmd_prompt, TRUE);
	if ((result = unique_test(cmd_str, commands)) != 1)
	{
		werase(com_win);
		wmove(com_win, 0, 0);
		if (result == 0)
			wprintw(com_win, unkn_cmd_msg, cmd_str);
		else
			wprintw(com_win, non_unique_cmd_msg);

		wrefresh(com_win);

		info_type = CONTROL_KEYS;
		paint_info_win();

		if (cmd_str != NULL)
			free(cmd_str);
		return;
	}
	command(cmd_str);
	wrefresh(com_win);
	info_type = CONTROL_KEYS;
	paint_info_win();
	if (cmd_str != NULL)
		free(cmd_str);
}

/*
 |	after changing the tab spacing, or inserting or deleting tab stops, 
 |	re-calculate the vertical length of lines in buffers to ensure 
 |	proper drawing of lines.
 */

void 
tab_resize()
{
	struct bufr *tt;		/* temporary pointer		*/
	struct text *tmp_tx;		/* temporary text pointer	*/

	tt = first_buff;
	while (tt != NULL)
	{
		for (tmp_tx = tt->first_line; tmp_tx != NULL; 
		        tmp_tx = tmp_tx->next_line)
		{
			tmp_tx->vert_len = (scanline(tmp_tx, 
			     tmp_tx->line_length) / COLS) + 1;
		}
		tt->window_top = first_buff->window_top;
		tt = tt->next_buff;
	}
}

void 
command(cmd_str)	/* process commands from command line	*/
char *cmd_str;
{
	char *cmd_str2;
	char *c_temp;
	char *name;
	char *tmp;
	char dir;
	int alloc_space;	/* have we allocated more space ?	*/
	int c_int;
	int retval;
	int gold_flag;
	int temp_int;
	struct tab_stops *temp_stack;

	clr_cmd_line = TRUE;
	alloc_space = FALSE;
	cmd_str2 = cmd_str;
	if (compare(cmd_str, EXPAND_str, FALSE))
		expand = TRUE;
	else if (compare(cmd_str, NOEXPAND_str, FALSE))
		expand = FALSE;
	else if (compare(cmd_str, NOJUSTIFY_str, FALSE))
		right_justify = FALSE;
	else if (compare(cmd_str, JUSTIFY_str, FALSE))
		right_justify = TRUE;
	else if (compare(cmd_str, EXIT_str, FALSE))
		finish(cmd_str);
	else if (compare(cmd_str, QUIT_str, FALSE))
		quit(cmd_str);
	else if (compare(cmd_str, AUTOFORMAT_str, FALSE))
	{
		auto_format = TRUE;
		observ_margins = TRUE;
		indent = FALSE;
	}
	else if (compare(cmd_str, NOAUTOFORMAT_str, FALSE))
		auto_format = FALSE;
	else if (compare(cmd_str, INFO_str, FALSE))
	{
		if (info_window == FALSE)
			info_op();
	}
	else if (compare(cmd_str, NOINFO_str, FALSE))
	{
		if (info_window == TRUE)
			info_op();
	}
	else if (compare(cmd_str, TABS_str, FALSE))
	{
		cmd_str = next_word(cmd_str);
		if (*cmd_str != '\0')
		{
			tab_set(cmd_str);
			tab_resize();
			new_screen();
		}
		else
		{
			temp_stack = tabs->next_stop;
			werase(com_win);
			wmove(com_win, 0, 0);
			wprintw(com_win, tab_msg);
			while (temp_stack != NULL)
			{
				wprintw(com_win, "%d ", temp_stack->column);
				temp_stack = temp_stack->next_stop;
			}
		}
	}
	else if (compare(cmd_str, UNTABS_str, FALSE))
	{
		unset_tab(cmd_str);
		tab_resize();
		new_screen();
	}
	else if (compare(cmd_str, SPACING_str, FALSE))
	{
		cmd_str2 = next_word(cmd_str);
		if (*cmd_str2 != '\0')
		{
			tab_spacing = atoi(cmd_str2);
			tab_resize();
			midscreen(curr_buff->scr_vert, curr_buff->position);
		}
		else
		{
			werase(com_win);
			wmove(com_win, 0, 0);
			wprintw(com_win, SPACING_msg, tab_spacing);
		}
	}
	else if (compare(cmd_str, WRITE_str, FALSE))
	{
		if (restrict_mode())
		{
			return;
		}
		cmd_str2 = next_word(cmd_str);
		if (*cmd_str2 == '\0')
		{
			alloc_space = TRUE;
			cmd_str2 = get_string(file_write_prompt_str, TRUE);
		}
		tmp = cmd_str2;
		while ((*tmp != ' ') && (*tmp != '\t') && (*tmp != '\0'))
			tmp++;
		*tmp = '\0';
		tmp_file = resolve_name(cmd_str2);
		write_file(tmp_file);
		if (tmp_file != cmd_str2)
			free(tmp_file);
		if (alloc_space)
			free(cmd_str2);
	}
	else if (compare(cmd_str, READ_str, FALSE))
	{
		if (restrict_mode())
		{
			return;
		}
		cmd_str2 = next_word(cmd_str);
		if (*cmd_str2 == '\0')
		{
			alloc_space = TRUE;
			cmd_str2 = get_string(file_read_prompt_str, TRUE);
		}
		tmp = cmd_str2;
		while ((*tmp != ' ') && (*tmp != '\t') && (*tmp != '\0'))
			tmp++;
		*tmp = '\0';
		tmp_file = resolve_name(cmd_str2);
		recv_file = TRUE;
		value = check_fp();
		if (tmp_file != cmd_str2)
			free(tmp_file);
		if (alloc_space)
			free(cmd_str2);
	}
	else if (compare(cmd_str, SAVE_str, FALSE))
	{
		/*
		 |	Should reflect changes made here in file_op() 
		 |	where 'save' command is handled.
		 */

		cmd_str2 = next_word(cmd_str);
		if (!curr_buff->edit_buffer)
			file_op(SAVE_FILE);
		else if ((*cmd_str2 != '\0') && (curr_buff->file_name == NULL))
		{
			tmp_file = resolve_name(cmd_str2);
			if (tmp_file != cmd_str2)
			{
				alloc_space = TRUE;
				cmd_str2 = tmp_file;
			}

			if (write_file(cmd_str2))
			{
				if (curr_buff->file_name == NULL)
				{
					curr_buff->full_name = get_full_path(cmd_str2, 
								NULL);
					curr_buff->file_name = 
						ae_basename(curr_buff->full_name);
				}
				curr_buff->changed = FALSE;
				change = FALSE;
				if (alloc_space)
					free(cmd_str2);
			}
			else 
			{
				if (alloc_space)
					free(cmd_str2);
				return;
			}
		}
		else
		{
			file_op(SAVE_FILE);
		}

	}
	else if (compare(cmd_str, LITERAL_str, FALSE))
		literal = TRUE;
	else if (compare(cmd_str, NOLITERAL_str, FALSE))
		literal = FALSE;
	else if (compare(cmd_str, STATUS_str, FALSE))
		status_line = TRUE;
	else if (compare(cmd_str, NOSTATUS_str, FALSE))
		status_line = FALSE;
	else if (compare(cmd_str, MARGINS_str, FALSE))
		observ_margins = TRUE;
	else if (compare(cmd_str, NOMARGINS_str, FALSE))
		observ_margins = FALSE;
	else if (compare(cmd_str, INDENT_str, FALSE))
		indent = TRUE;
	else if (compare(cmd_str, NOINDENT_str, FALSE))
		indent = FALSE;
	else if (compare(cmd_str, OVERSTRIKE_str, FALSE))
		overstrike = TRUE;
	else if (compare(cmd_str, NOOVERSTRIKE_str, FALSE))
		overstrike = FALSE;
	else if (compare(cmd_str, text_cmd, FALSE))
		text_only = TRUE;
	else if (compare(cmd_str, binary_cmd, FALSE))
		text_only = FALSE;
	else if (compare(cmd_str, info_win_height_cmd_str,  FALSE))
	{
		temp_int = info_win_height;
		tmp = next_word(cmd_str);
		if ((*tmp >= '0') && (*tmp <= '9'))
		{
			c_int = atoi(tmp);
			if ((c_int > 0) && (c_int <= MAX_HELP_LINES))
				info_win_height = c_int + 1;
			else
			{
				wmove(com_win, 0, 0);
				werase(com_win);
				wprintw(com_win, info_win_height_err);
				wrefresh(com_win);
			}
			if ((info_win_height != temp_int) && (info_window))
			{
				redo_win();
				curr_buff->last_line = curr_buff->lines - 1;
				new_screen();
				paint_info_win();
				redraw();
			}
		}
		else
		{
			werase(com_win);
			wmove(com_win, 0, 0);
			wprintw(com_win, "%s %d", info_win_height_msg_str, (info_win_height - 1));
			wrefresh(com_win);
		}
	}
	else if ((compare(cmd_str, LEFTMARGIN_str, FALSE)) || (compare(cmd_str, RIGHTMARGIN_str, FALSE)))
	{
		tmp = next_word(cmd_str);
		if ((*tmp >= '0') && (*tmp <= '9'))
		{
			c_int = atoi(tmp);
			if (compare(cmd_str, LEFTMARGIN_str, FALSE))
			{
				if (c_int > right_margin)
				{
					wmove(com_win, 0, 0);
					wclrtoeol(com_win);
					wprintw(com_win, left_mrg_err_msg);
				}
				else
					left_margin = c_int;
			}
			else if (compare(cmd_str, RIGHTMARGIN_str, FALSE))
			{
				if (c_int < left_margin)
				{
					wmove(com_win, 0, 0);
					wclrtoeol(com_win);
					wprintw(com_win, right_mrg_err_msg);
				}
				else
					right_margin = c_int;
			}
		}
		else
		{
			wmove(com_win, 0, 0);
			wclrtoeol(com_win);
			if (compare(cmd_str, LEFTMARGIN_str, FALSE))
				wprintw(com_win, left_mrg_setting, left_margin);
			else if (compare(cmd_str, RIGHTMARGIN_str, FALSE))
				wprintw(com_win, right_mrg_setting, right_margin);
		}
	}
	else if (compare(cmd_str, LINE_str, FALSE))
	{
		wmove(com_win,0,0);
		wclrtoeol(com_win);
		wprintw(com_win, line_num_str, curr_buff->curr_line->line_number);
		wprintw(com_win, lines_from_top, curr_buff->absolute_lin);
		wprintw(com_win, total_lines_str, curr_buff->num_of_lines);
	}
	else if (compare(cmd_str, FILE_str, FALSE))
	{
		wmove(com_win,0,0);
		wclrtoeol(com_win);
		if (curr_buff->edit_buffer)
		{
			if (curr_buff->file_name != NULL)
			{
				wprintw(com_win, current_file_str, curr_buff->full_name);
			}
			else
			{
				wprintw(com_win, current_file_str, no_file_string );
			}
		}
		else
		{
			if (first_buff->file_name != NULL)
			{
				wprintw(com_win, current_file_str, first_buff->full_name);
			}
			else
			{
				wprintw(com_win, current_file_str, no_file_string );
			}
		}
	}
	else if (compare(cmd_str, COPYRIGHT_str, FALSE))
	{
		wmove(com_win,0,0);
		wclrtoeol(com_win);
		wprintw(com_win, "%s", copyright_notice);
	}
	else if ((*cmd_str >= '0') && (*cmd_str <= '9'))
		goto_line(cmd_str);
	else if ((*cmd_str == '+') || (*cmd_str == '-'))
	{
		if (*cmd_str == '+')
			dir = 'd';
		else
			dir = 'u';
		cmd_str++;
		if ((*cmd_str == ' ') || (*cmd_str == '\t'))
			cmd_str = next_word(cmd_str);
		value = 0;
		while ((*cmd_str >='0') && (*cmd_str <= '9'))
		{
			value = value * 10 + (*cmd_str - '0');
			cmd_str++;
		}
		move_rel(&dir, value);
		werase(com_win);
		wmove(com_win, 0,0);
		wprintw(com_win, line_num_str, curr_buff->curr_line->line_number);
		wrefresh(com_win);
	}
	else if (compare(cmd_str, CHARACTER_str, FALSE))
	{
		wmove(com_win,0,0);
		wclrtoeol(com_win);
		if (*curr_buff->pointer >= 0)
			wprintw(com_win, char_str, *curr_buff->pointer);
		else
			wprintw(com_win, char_str, (*curr_buff->pointer+256));
	}
	else if (compare(cmd_str, REDRAW_str, FALSE))
	{
		clearok(curr_buff->win, TRUE);
		redraw();
	}
	else if (compare(cmd_str, RESEQUENCE_str, FALSE))
	{
		tmp_line = curr_buff->first_line->next_line;
		while (tmp_line != NULL)
		{
			tmp_line->line_number = tmp_line->prev_line->line_number + 1;
			tmp_line = tmp_line->next_line;
		}
	}
	else if (compare(cmd_str, AUTHOR_str, FALSE))
	{
		wmove(com_win,0,0);
		wclrtoeol(com_win);
		wprintw(com_win, "written by Hugh Mahon");
	}
	else if (compare(cmd_str, VERSION_str, FALSE))
	{
		wmove(com_win,0,0);
		wclrtoeol(com_win);
		wprintw(com_win, version_string);
	}
	else if (compare(cmd_str, CASE_str, FALSE))
		case_sen = TRUE;
	else if (compare(cmd_str, NOCASE_str, FALSE))
		case_sen = FALSE;
	else if (compare(cmd_str, EIGHT_str, FALSE))
	{
		eightbit = TRUE;
		new_screen();
	}
	else if (compare(cmd_str, NOEIGHT_str, FALSE))
	{
		eightbit = FALSE;
		new_screen();
	}
	else if (compare(cmd_str, WINDOWS_str, FALSE))
		make_win();
	else if (compare(cmd_str, NOWINDOWS_str, FALSE))
		no_windows();
	else if (compare(cmd_str, DEFINE_str, FALSE))
	{
		cmd_str = next_word(cmd_str);
		def_key(cmd_str);
	}
	else if (compare(cmd_str, SHOW_str, FALSE))
	{
		cmd_str = next_word(cmd_str);
		if (compare(cmd_str, GOLD_str, FALSE))
		{
			cmd_str = next_word(cmd_str);
			gold_flag = TRUE;
		}
		else
			gold_flag = FALSE;
		if (toupper(*cmd_str) == 'F')
		{
			cmd_str++;
			c_int = 0;
			while ((*cmd_str >= '0') && (*cmd_str <= '9'))
			{
				c_int = c_int * 10 + (*cmd_str - '0');
				cmd_str++;
			}
			if (c_int < 65)
			{
				if (gold_flag)
					c_temp = g_f[c_int];
				else
					c_temp = f[c_int];
				werase(com_win);
				wmove(com_win, 0,0);
				wprintw(com_win, key_def_msg, c_temp);
			}
			else
			{
				wmove(com_win,0,0);
				wclrtoeol(com_win);
				wprintw(com_win, unkn_syntax_msg, cmd_str);
			}
		}
		if (toupper(*cmd_str) == 'K')
		{
			cmd_str++;
			c_int = 0;
			while ((*cmd_str >= '0') && (*cmd_str <= '9'))
			{
				c_int = c_int * 10 + (*cmd_str - '0');
				cmd_str++;
			}
			if (c_int < 5)
			{
				if (gold_flag)
					c_temp = g_keypads[c_int];
				else
					c_temp = keypads[c_int];
				werase(com_win);
				wmove(com_win, 0,0);
				wprintw(com_win, key_def_msg, c_temp);
			}
			else
			{
				wmove(com_win,0,0);
				wclrtoeol(com_win);
				wprintw(com_win, unkn_syntax_msg, cmd_str);
			}
		}
		else if (*cmd_str == '^')
		{
			cmd_str++;
			if (*cmd_str == '?')
				c_int = 31;
			else
				c_int = toupper(*cmd_str) - 'A';
			if ((c_int != 8)&& (c_int != 16) && (c_int != 18) && ((c_int >= 0) && (c_int < 32)))
			{
				if (gold_flag)
					c_temp = g_ctr[c_int];
				else
					c_temp = ctr[c_int];
				werase(com_win);
				wmove(com_win, 0,0);
				wprintw(com_win, key_def_msg, c_temp);
			}
			else
			{
				wmove(com_win,0,0);
				wclrtoeol(com_win);
				wprintw(com_win, unkn_syntax_msg, cmd_str);
			}
		}
		wrefresh(com_win);
	}
	else if (compare(cmd_str, HELP_str, FALSE))
		help();
	else if (compare(cmd_str, PRINT_str, FALSE))
	{
		print_buffer();
	}
	else if ((*cmd_str == '<') && (!in_pipe))
	{
		in_pipe = TRUE;
		shell_fork = FALSE;
		cmd_str++;
		if ((*cmd_str == ' ') || (*cmd_str == '\t'))
			cmd_str = next_word(cmd_str);
		c_int = 0;
		in_buff_name[c_int] = '\0';
		while ((*cmd_str != '!') && (*cmd_str != '>') && (*cmd_str != '<') && (*cmd_str != ' ') && (*cmd_str != '\t') && (*cmd_str != '\0'))
		{
			in_buff_name[c_int] = *cmd_str;
			cmd_str++;
			c_int++;
		}
		if (c_int == 0)
		{
			copy_str(curr_buff->name, in_buff_name);
		}
		else
			in_buff_name[c_int] = '\0';
		if ((*cmd_str == ' ') || (*cmd_str == '\t'))
			cmd_str = next_word(cmd_str);
		command(cmd_str);
		in_pipe = FALSE;
		shell_fork = TRUE;
	}
	else if ((*cmd_str == '>') && (!out_pipe))
	{
		out_pipe = TRUE;
		cmd_str++;
		if ((*cmd_str == ' ') || (*cmd_str == '\t'))
			cmd_str = next_word(cmd_str);
		c_int = 0;
		out_buff_name[c_int] = '\0';
		while ((*cmd_str != '!') && (*cmd_str != '>') && (*cmd_str != '<') && (*cmd_str != ' ') && (*cmd_str != '\t') && (*cmd_str != '\0'))
		{
			out_buff_name[c_int] = *cmd_str;
			cmd_str++;
			c_int++;
		}
		if (c_int == 0)
		{
			copy_str(curr_buff->name, out_buff_name);
		}
		else
			out_buff_name[c_int] = '\0';
		if ((*cmd_str == ' ') || (*cmd_str == '\t'))
			cmd_str = next_word(cmd_str);
		command(cmd_str);
		out_pipe = FALSE;
	}
	else if (*cmd_str == '!')
	{
		cmd_str++;
		if ((*cmd_str == ' ') || (*cmd_str == 9))
			cmd_str = next_word(cmd_str);
		sh_command(cmd_str);
	}
	else if (compare(cmd_str, BUFFER_str, FALSE))
	{
		cmd_str = next_word(cmd_str);
		if (*cmd_str == '\0')
		{
			wmove(com_win,0,0);
			wclrtoeol(com_win);
			wprintw(com_win, current_buff_msg, curr_buff->name);
		}
		else
		{
			tmp = cmd_str;
			while ((*tmp != ' ') && (*tmp != '\t') && (*tmp != '\0'))
				tmp++;
			*tmp = '\0';
			chng_buf(cmd_str);
		}
	}
	else if (compare(cmd_str, DELETE_str, FALSE))
	{
		if (!strcmp(curr_buff->name, main_buffer_name))
		{
			wmove(com_win,0,0);
			wclrtoeol(com_win);
			wprintw(com_win, cant_del_buf_msg, main_buffer_name);
		}
		else
			del_buf();
	}
	else if (compare(cmd_str, CD_str, FALSE))
	{
		if (restrict_mode())
		{
			return;
		}

		cmd_str = next_word(cmd_str);
		if (change_dir_allowed != TRUE)
		{
			wmove(com_win,0,0);
			wclrtoeol(com_win);
			wprintw(com_win, no_chng_dir_msg);
		}
		else if ((curr_buff->orig_dir == NULL) && (*cmd_str == '\0'))
		{
			wmove(com_win,0,0);
			wclrtoeol(com_win);
			wprintw(com_win, no_dir_entered_msg);
		}
		else
		{
			if (*cmd_str != '\0')
				c_temp = cmd_str;
			else
				c_temp = curr_buff->orig_dir;
			tmp = c_temp;
			while ((*tmp != ' ') && (*tmp != '\t') && (*tmp != '\0'))
				tmp++;
			*tmp = '\0';
			name = resolve_name(c_temp);
			retval = chdir(name);
			if (name != c_temp)
				free(name);
			if (retval == -1)
			{
				werase(com_win);
				wmove(com_win, 0, 0);
				if (errno == ENOTDIR)
					wprintw(com_win, path_not_dir_msg);
				else if (errno == EACCES)
					wprintw(com_win, path_not_permitted_msg);
				else 
					wprintw(com_win, path_chng_failed_msg);
			}
		}
	}
	else if (compare(cmd_str, edit_cmd_str, FALSE))
	{
		cmd_str2 = next_word(cmd_str);
		if (cmd_str2 != NULL)
		{
			if (*cmd_str != '\0')
				c_temp = cmd_str2;
			tmp = c_temp;
			while ((*tmp != ' ') && (*tmp != '\t') && (*tmp != '\0'))
				tmp++;
			*tmp = '\0';
			if (*c_temp != '\0')
				name = resolve_name(c_temp);
			else
				name = c_temp;
			retval = open_for_edit(name);
			if (name != c_temp)
				free(name);
		}
	}
	else if (compare(cmd_str, pwd_cmd_str, FALSE))
	{
		show_pwd();
	}
	else if (compare(cmd_str, DIFF_str, FALSE))
	{
		diff_file();
	}
	else if (compare(cmd_str, journal_str, FALSE))
	{
		wmove(com_win,0,0);
		wclrtoeol(com_win);
		wprintw(com_win, "journal file is %s", curr_buff->journal_file);
	}
	else
	{
		wmove(com_win,0,0);
		wclrtoeol(com_win);
		wprintw(com_win, unkn_cmd_msg, cmd_str2);
	}
}


void 
init_keys()		/* initialize control keys and function keys	*/
{
	int counter;

	ctr[0] = fn_AC_str;	/* control a */
	ctr[1] = fn_EOT_str;	/* control b */
	ctr[2] = fn_COPY_str;	/* control c */
	ctr[3] = fn_BOL_str; 	/* control d */
	ctr[4] = fn_CMD_str; 	/* control e */
	ctr[5] = fn_SRCH_str;	/* control f */
	ctr[6] = fn_GOLD_str;	/* control g */
	ctr[7] = fn_BCK_str;	/* control h */	/* backspace	*/
	ctr[8] = "";		/* control i */	/* tab		*/
	ctr[9] = fn_CR_str;	/* control j */	/* new-line	*/
	ctr[10] = fn_DC_str; 	/* control k */
	ctr[11] = fn_DL_str; 	/* control l */
	ctr[12] = fn_CR_str;	/* control m */	/* carriage-return	*/
	ctr[13] = fn_NP_str; 	/* control n */
	ctr[14] = fn_EOL_str;	/* control o */
	ctr[15] = fn_PP_str; 	/* control p */
	ctr[16] = "";		/* control q */
	ctr[17] = fn_RD_str; 	/* control r */
	ctr[18] = "";		/* control s */
	ctr[19] = fn_BOT_str;	/* control t */
	ctr[20] = fn_MARK_str;	/* control u */
	ctr[21] = fn_PST_str;	/* control v */
	ctr[22] = fn_DW_str; 	/* control w */
	ctr[23] = fn_CUT_str;	/* control x */
	ctr[24] = fn_AW_str; 	/* control y */
	ctr[25] = fn_RP_str; 	/* control z */
	ctr[26] = fn_MENU_str; 	/* control [ */
	ctr[27] = ""; 		/* control \\ */
	ctr[28] = ""; 		/* control ] */
	ctr[29] = ""; 		/* control ^ */
	ctr[30] = ""; 		/* control _ */
	ctr[31] = fn_BCK_str;	/* control ?*/	/* delete key	*/
	g_ctr[0] = fn_MC_str;		/* gold control a */
	g_ctr[1] = fn_APPEND_str;	/* gold control b */
	g_ctr[2] = fn_CL_str;		/* gold control c */
	g_ctr[3] = fn_PREFIX_str;	/* gold control c */
	g_ctr[4] = "";			/* gold control e */
	g_ctr[5] = fn_PSRCH_str;	/* gold control f */
	g_ctr[6] = fn_GOLD_str;		/* gold control g */
	g_ctr[7] = "";			/* gold control h */
	g_ctr[8] = "";			/* gold control i */
	g_ctr[9] = "";			/* gold control j */
	g_ctr[10] = fn_UDC_str;		/* gold control k */
	g_ctr[11] = fn_UDL_str;		/* gold control l */
	g_ctr[12] = ""; 		/* gold control m */
	g_ctr[13] = fn_NB_str;		/* gold control n */
	g_ctr[14] = ""; 		/* gold control o */
	g_ctr[15] = fn_PB_str;		/* gold control p */
	g_ctr[16] = "";			/* gold control q */
	g_ctr[17] = fn_REV_str;		/* gold control r */
	g_ctr[18] = "";			/* gold control s */
	g_ctr[19] = "";			/* gold control t */
	g_ctr[20] = "";			/* gold control u */
	g_ctr[21] = fn_FWD_str;		/* gold control v */
	g_ctr[22] = fn_UDW_str;		/* gold control w */
	g_ctr[23] = fn_FORMAT_str;	/* gold control x */
	g_ctr[24] = fn_PW_str;		/* gold control y */
	g_ctr[25] = fn_PRP_str;		/* gold control z */
	g_ctr[26] = ""; 		/* control [ */
	g_ctr[27] = ""; 		/* control \\ */
	g_ctr[28] = ""; 		/* control ] */
	g_ctr[29] = ""; 		/* control ^ */
	g_ctr[30] = ""; 		/* control _ */
	g_ctr[31] = ""; 		/* control ? */
	f[0] = "";	
	f[1] = fn_GOLD_str;
	f[2] = fn_UDC_str;
	f[3] = fn_DW_str;
	f[4] = fn_AW_str;
	f[5] = fn_SRCH_str;
	f[6] = fn_MARK_str;
	f[7] = fn_CUT_str;
	f[8] = fn_AL_str;
	g_f[0] = "";
	g_f[1] = fn_GOLD_str;
	g_f[2] = fn_UDL_str;
	g_f[3] = fn_UDW_str;
	g_f[4] = fn_BOL_str;
	g_f[5] = fn_PSRCH_str;
	g_f[6] = fn_COPY_str;
	g_f[7] = fn_PST_str;
	g_f[8] = fn_CMD_str;

	for (counter = 0; counter < 9; counter++)
	{
		g_f_changed[counter] = FALSE;
		f_changed[counter] = FALSE;
	}


	counter = 9;
	while (counter < 64)
	{
		f[counter] = "";
		g_f[counter] = "";
		g_f_changed[counter] = FALSE;
		f_changed[counter] = FALSE;
		counter++;
	}

	keypads[0] = "";
	keypads[1] = "";
	keypads[2] = "";
	keypads[3] = "";
	keypads[4] = "";
	g_keypads[0] = "";
	g_keypads[1] = "";
	g_keypads[2] = "";
	g_keypads[3] = "";
	g_keypads[4] = "";

	for (counter = 0; counter < 32; counter++)
	{
		g_ctr_changed[counter] = FALSE;
		ctr_changed[counter] = FALSE;
	}

}

void 
parse(string)		/* parse commands in string		*/
char *string;
{
	char *temp;
	char dir;
	int valid_flag; /* try to keep GOLD from being executed within multiple definition	*/
	int delim;	/* delimiter for inserting a string	*/

	temp = string;
	if ((*temp == ' ') || (*temp == '\t'))
		temp = next_word(temp);
	valid_flag = FALSE;
	while (*temp != '\0')
	{
		if ((!compare(temp, fn_GOLD_str, FALSE)) && (!valid_flag))
			valid_flag = TRUE;
		if (compare(temp, fn_DL_str, FALSE))
			del_line(TRUE);
		else if (compare(temp, fn_DC_str, FALSE))
			del_char(TRUE);
		else if (compare(temp, fn_CL_str, FALSE))
			Clear_line(TRUE);
		else if (compare(temp, fn_NP_str, FALSE))
		{
			next_page();
		}
		else if (compare(temp, fn_PP_str, FALSE))
		{
			prev_page();
		}
		else if (compare(temp, fn_NB_str, FALSE))
		{
			if (curr_buff->next_buff == NULL)
				t_buff = first_buff;
			else
				t_buff = curr_buff->next_buff;
			chng_buf(t_buff->name);
		}
		else if (compare(temp, fn_PB_str, FALSE))
		{
			t_buff = first_buff;
			while ((t_buff->next_buff != curr_buff) && (t_buff->next_buff != NULL))
				t_buff = t_buff->next_buff;
			chng_buf(t_buff->name);
		}
		else if (compare(temp, fn_UDL_str, FALSE))
			undel_line();
		else if (compare(temp, fn_UDC_str, FALSE))
			undel_char();
		else if (compare(temp, fn_DW_str, FALSE))
			del_word(TRUE);
		else if (compare(temp, fn_UDW_str, FALSE))
			undel_word();
		else if (compare(temp, fn_UND_str, FALSE))
			undel_last();
		else if (compare(temp, fn_EOL_str, FALSE))
			eol();
		else if (compare(temp, fn_BOL_str, FALSE))
			bol();
		else if (compare(temp, fn_BOT_str, FALSE))
			top();
		else if (compare(temp, fn_EOT_str, FALSE))
			bottom();
		else if (compare(temp, fn_FORMAT_str,  FALSE))
			Format();
		else if ((compare(temp, fn_GOLD_str, FALSE)) && (!valid_flag) && (*(next_word(temp)) == '\0'))
			gold_func();
		else if (compare(temp, fn_MARGINS_str, FALSE))
			observ_margins = TRUE;
		else if (compare(temp, fn_NOMARGINS_str, FALSE))
			observ_margins = FALSE;
		else if (compare(temp, fn_IL_str, FALSE))
		{
			insert_line(TRUE);
			if (curr_buff->position != 1)
				bol();
			left(TRUE);
		}
		else if (compare(temp, fn_PRP_str, FALSE))
			repl_prompt(TRUE);
		else if (compare(temp, fn_RP_str, FALSE))
			replace();
		else if (compare(temp, fn_MC_str, FALSE))
			match();
		else if (compare(temp, fn_PSRCH_str, FALSE))
			search_prompt(TRUE);
		else if (compare(temp, fn_SRCH_str, FALSE))
			value = search(TRUE, curr_buff->curr_line, curr_buff->position, curr_buff->pointer, 0, FALSE, TRUE);
		else if (compare(temp, fn_AL_str, FALSE))
			adv_line();
		else if (compare(temp, fn_AW_str, FALSE))
			adv_word();
		else if (compare(temp, fn_AC_str, FALSE))
			ascii();
		else if (compare(temp, fn_PW_str, FALSE))
			prev_word();
		else if (compare(temp, fn_CUT_str, FALSE))
			cut();
		else if (compare(temp, fn_FWD_str, FALSE))
		{
			forward = TRUE;
			wmove(com_win, 0,0);
			wclrtoeol(com_win);
			wprintw(com_win, fwd_mode_str);
			wrefresh(com_win);
			clr_cmd_line = TRUE;
		}
		else if (compare(temp, fn_REV_str, FALSE))
		{
			forward = FALSE;
			wmove(com_win, 0,0);
			wclrtoeol(com_win);
			wprintw(com_win, rev_mode_str);
			wrefresh(com_win);
			clr_cmd_line = TRUE;
		}
		else if (compare(temp, fn_MARK_str, FALSE))
			slct(Mark);
		else if (compare(temp, fn_UNMARK_str, FALSE))
			unmark_text();
		else if (compare(temp, fn_APPEND_str, FALSE))
			slct(Append);
		else if (compare(temp, fn_PREFIX_str, FALSE))
			slct(Prefix);
		else if (compare(temp, fn_COPY_str, FALSE))
		{
			copy();
		}
		else if (compare(temp, fn_CMD_str, FALSE))
		{
			command_prompt();
		}
		else if (compare(temp, fn_PST_str, FALSE))
			paste();
		else if (compare(temp, fn_RD_str, FALSE))
		{
			clearok(curr_buff->win, TRUE);
			redraw();
		}
		else if (compare(temp, fn_UP_str,  FALSE))
			up();
		else if (compare(temp, fn_DOWN_str,  FALSE))
			down();
		else if (compare(temp, fn_LEFT_str,  FALSE))
			left(TRUE);
		else if (compare(temp, fn_RIGHT_str,  FALSE))
			right(TRUE);
		else if (compare(temp, fn_BCK_str,  FALSE))
		{
			if (overstrike)
			{
				if (curr_buff->position > 1)
				{
					if ((!observ_margins) || 
					    ((observ_margins) && 
					    (curr_buff->scr_pos > left_margin)))
					{
						left(TRUE);
						/*
						 |	save the deleted character 
						 */
						if (in == 8)
							d_char = *curr_buff->pointer;
						insert(' ');
						left(TRUE);
						last_deleted(CHAR_BACKSPACE, 1, &d_char);
					}
				}
				/*
				 |	if at begin of line, do nothing
				 */
			}
			else
			{
				if (delete(TRUE))
					last_deleted(CHAR_BACKSPACE, 1, &d_char);
			}
		}
		else if (compare(temp, fn_CR_str,  FALSE))
			insert_line(TRUE);
		else if (compare(temp, fn_EXPAND_str,  FALSE))
			expand = TRUE;
		else if (compare(temp, fn_NOEXPAND_str,  FALSE))
			expand = FALSE;
		else if (compare(temp, fn_EXIT_str,  FALSE))
			finish(temp);
		else if (compare(temp, fn_QUIT_str,  FALSE))
			quit(temp);
		else if (compare(temp, fn_LITERAL_str,  FALSE))
			literal = TRUE;
		else if (compare(temp, fn_NOLITERAL_str,  FALSE))
			literal = FALSE;
		else if (compare(temp, fn_STATUS_str,  FALSE))
			status_line = TRUE;
		else if (compare(temp, fn_NOSTATUS_str,  FALSE))
			status_line = FALSE;
		else if (compare(temp, fn_INDENT_str,  FALSE))
			indent = TRUE;
		else if (compare(temp, fn_NOINDENT_str,  FALSE))
			indent = FALSE;
		else if (compare(temp, fn_OVERSTRIKE_str,  FALSE))
			overstrike = TRUE;
		else if (compare(temp, fn_NOOVERSTRIKE_str,  FALSE))
			overstrike = FALSE;
		else if (compare(temp, fn_CASE_str,  FALSE))
			case_sen = TRUE;
		else if (compare(temp, fn_NOCASE_str,  FALSE))
			case_sen = FALSE;
		else if (compare(temp, fn_WINDOWS_str,  FALSE))
			make_win();
		else if (compare(temp, fn_NOWINDOWS_str,  FALSE))
			no_windows();
		else if (compare(temp, fn_HELP_str,  FALSE))
			help();
		else if ((*temp == '+') || (*temp == '-'))
		{
			if (*temp == '+')
				dir = 'd';
			else
				dir = 'u';
			temp++;
			if ((*temp == ' ') || (*temp == '\t'))
				temp = next_word(temp);
			value = 0;
			while ((*temp >='0') && (*temp <= '9'))
			{
				value = value * 10 + (*temp - '0');
				temp++;
			}
			move_rel(&dir, value);
			if (!status_line)
			{
				clr_cmd_line = TRUE;
				werase(com_win);
				wmove(com_win, 0,0);
				wprintw(com_win, line_num_str, curr_buff->curr_line->line_number);
				wrefresh(com_win);
			}
		}
		else if ( ! (((*temp >= 'a') && (*temp <= 'z')) || 
			     ((*temp >= 'A') && (*temp <= 'Z')) || 
			     ((*temp >= '0') && (*temp <= '9'))) )
		{
			delim = *temp;
			temp++;
			while ((*temp != delim) && (*temp != '\0'))
			{
				insert(*temp);
				temp++;
			}
		}
		else if (compare(temp, fn_MENU_str,  FALSE))
			menu_op(main_menu);
		else
		{
			wmove(com_win,0,0);
			wclrtoeol(com_win);
			wprintw(com_win, unkn_syntax_msg, temp);
			wrefresh(com_win);
			clr_cmd_line = TRUE;
		}
		temp = next_word(temp);
	}
}

int
restrict_mode()
{
	if (!restricted)
		return(FALSE);

	wmove(com_win, 0, 0);
	wprintw(com_win, restricted_msg);
	wclrtoeol(com_win);
	wrefresh(com_win);
	clr_cmd_line = TRUE;
	return(TRUE);
}

/*
 |	Save current configuration to .init.ee file in the current directory.
 */

void 
dump_aee_conf()	
{
	FILE *init_file;
	FILE *old_init_file = NULL;
	char *file_name = ".init.ae";
	char *home_dir =  "~/.init.ae";
	char buffer[512];
	struct stat buf;
	char *string;
	int length;
	int counter;
	int option = 0;
	struct tab_stops *stack_point;

	if (restrict_mode())
	{
		return;
	}

	option = menu_op(config_dump_menu);

	werase(com_win);
	wmove(com_win, 0, 0);

	if (option == 0)
	{
		wprintw(com_win, conf_not_saved_msg);
		wrefresh(com_win);
		return;
	}
	else if (option == 2)
		file_name = resolve_name(home_dir);

	/*
	 |	If a .init.ae file exists, move it to .init.ae.old.
	 */

	if (stat(file_name, &buf) != -1)
	{
		sprintf(buffer, "%s.old", file_name);
		unlink(buffer);
		link(file_name, buffer);
		unlink(file_name);
		old_init_file = fopen(buffer, "r");
	}

	init_file = fopen(file_name, "w");
	if (init_file == NULL)
	{
		wprintw(com_win, conf_dump_err_msg);
		wrefresh(com_win);
		return;
	}

	if (old_init_file != NULL)
	{
		/*
		 |	Copy non-configuration info into new .init.ae file.
		 */
		while ((string = fgets(buffer, 512, old_init_file)) != NULL)
		{
			length = strlen(string);
			string[length - 1] = '\0';

			if (unique_test(string, init_strings) == 1)
			{
				if (compare(string, ECHO_str, FALSE))
				{
					fprintf(init_file, "%s\n", string);
				}
			}
			else
				fprintf(init_file, "%s\n", string);
		}

		fclose(old_init_file);
	}

	fprintf(init_file, "%s\n", case_sen ? CASE_str : NOCASE_str);
	fprintf(init_file, "%s\n", expand ? EXPAND_str : NOEXPAND_str);
	fprintf(init_file, "%s\n", info_window ? INFO_str : NOINFO_str );
	fprintf(init_file, "%s\n", observ_margins ? MARGINS_str : NOMARGINS_str );
	fprintf(init_file, "%s\n", auto_format ? AUTOFORMAT_str : NOAUTOFORMAT_str );
	fprintf(init_file, "%s %s\n", PRINTCOMMAND_str, print_command);
	fprintf(init_file, "%s %d\n", RIGHTMARGIN_str, right_margin);
	fprintf(init_file, "%s %d\n", LEFTMARGIN_str, left_margin);
	fprintf(init_file, "%s\n", nohighlight ? NOHIGHLIGHT_str : HIGHLIGHT_str );
	fprintf(init_file, "%s\n", eightbit ? EIGHT_str : NOEIGHT_str );
	fprintf(init_file, "%s\n", literal ? LITERAL_str : NOLITERAL_str );
	fprintf(init_file, "%s\n", observ_margins ? MARGINS_str : NOMARGINS_str );
	fprintf(init_file, "%s\n", status_line ? STATUS_str : NOSTATUS_str );
	fprintf(init_file, "%s\n", indent ? INDENT_str : NOINDENT_str );
	fprintf(init_file, "%s\n", overstrike ? OVERSTRIKE_str : NOOVERSTRIKE_str );
	fprintf(init_file, "%s\n", windows ? WINDOWS_str : NOWINDOWS_str );
	fprintf(init_file, "%s\n", text_only ? text_cmd : binary_cmd );

	if (info_win_height != INFO_WIN_HEIGHT_DEF)
	{
		fprintf(init_file, "%s %d\n", info_win_height_cmd_str, (info_win_height - 1));
	}

	for (counter = 0; counter < 64; counter++)
	{
		if (f_changed[counter])
			fprintf(init_file, "%s f%d %s\n", DEFINE_str, counter, f[counter]);
	}

	for (counter = 0; counter < 64; counter++)
	{
		if (g_f_changed[counter])
			fprintf(init_file, "%s %s f%d %s\n", DEFINE_str, GOLD_str, counter, g_f[counter]);
	}

	for (counter = 0; counter < 32; counter++)
	{
		if (ctr_changed[counter])
			fprintf(init_file, "%s %s %s\n", DEFINE_str, ctrl_table[counter + 1], ctr[counter]);
	}

	for (counter = 0; counter < 32; counter++)
	{
		if (g_ctr_changed[counter])
			fprintf(init_file, "%s %s %s %s\n", DEFINE_str, GOLD_str, ctrl_table[counter + 1], g_ctr[counter]);
	}

	if (tabs->next_stop != NULL)
	{
		stack_point = tabs->next_stop;
		fprintf(init_file, "%s ", TABS_str);
		while (stack_point != NULL)
		{
			fprintf(init_file, " %d", stack_point->column);
			stack_point = stack_point->next_stop;
		}
		fprintf(init_file, "\n");
	}
	if (tab_spacing != 8)
	{
		fprintf(init_file, "%s %d\n", SPACING_str, tab_spacing);
	}

	fclose(init_file);

	wprintw(com_win, conf_dump_success_msg, file_name);
	wrefresh(com_win);
	clr_cmd_line = TRUE;

	if ((option == 2) && (file_name != home_dir))
	{
		free(file_name);
	}
}

