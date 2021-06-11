/*
 |	keys.c
 |
 |	$Header: /home/hugh/sources/aee/RCS/keys.c,v 1.11 1999/01/01 18:15:50 hugh Exp hugh $
 */

/*
 |
 |	Copyright (c) 1986 - 1988, 1991 - 1999 Hugh Mahon.
 |
 */

#include "aee.h"

void 
control()		/* handles control key operations		*/
{
	char *cstr;	/* command string				*/

	cstr = "";
	if (((in > 0) && (in <= 31)) || (in == 127))
	{
		if (in != 127)
		{
			if (gold)
				cstr = g_ctr[in - 1];
			else
				cstr = ctr[in - 1];
		}
		else
		{
			if (gold)
				cstr = g_ctr[31];
			else
				cstr = ctr[31];
		}
		parse(cstr);
	}
}

void 
function_key()		/* process function key pressed	*/
{
	char *temp;

	if (in == KEY_LEFT)
	{
		temp = fn_LEFT_str;
	}
	else if (in == KEY_RIGHT)
	{
		temp = fn_RIGHT_str;
	}
	else if ( in == KEY_UP)
	{
		temp = fn_UP_str;
	}
	else if (in == KEY_DOWN)
	{
		temp = fn_DOWN_str;
	}
	else if (in == KEY_NPAGE)
	{
		if (gold)
			temp = fn_NB_str;
		else
			temp = fn_NP_str;
	}
	else if (in == KEY_PPAGE)
	{
		if (gold)
			temp = fn_PB_str;
		else
			temp = fn_PP_str;
	}
	else if (in == KEY_DL)
	{
		if (gold)
			temp = fn_UDL_str;
		else
			temp = fn_DL_str;
	}
	else if (in == KEY_DC)
	{
		if (gold)
			temp = fn_UDC_str;
		else
			temp = fn_DC_str;
	}
	else if (in == KEY_IL)
		temp = fn_IL_str;
	else if (in == KEY_IC)
	{
		if (overstrike)
			temp = fn_NOOVERSTRIKE_str;
		else
			temp = fn_OVERSTRIKE_str;
	}
	else if (in == KEY_EOL)
		temp = fn_EOL_str;
	else if (in == KEY_BACKSPACE)
		temp = fn_BCK_str;
	else if (in == KEY_END)
		temp = fn_EOL_str;
	else if (in == KEY_HOME)
		temp = fn_BOL_str;

	else if ((in >= KEY_F0) && (in <= KEY_F(30)))
	{
		if (gold)
			temp = g_f[in - KEY_F0];
		else
			temp = f[in - KEY_F0];
	}

	else if (in == KEY_C1)
	{
		if (gold)
			temp = g_keypads[0];
		else
			temp = keypads[0];
	}
	else if (in == KEY_C3)
	{
		if (gold)
			temp = g_keypads[1];
		else
			temp = keypads[1];
	}
	else if (in == KEY_A1)
	{
		if (gold)
			temp = g_keypads[2];
		else
			temp = keypads[2];
	}
	else if (in == KEY_A3)
	{
		if (gold)
			temp = g_keypads[4];
		else
			temp = keypads[4];
	}
	else if (in == KEY_B2)
	{
		if (gold)
			temp = g_keypads[3];
		else
			temp = keypads[3];
	}

	else
		temp = " ";

	parse(temp);
}

void 
gold_func()
{
	int tmp;

	if ((gold) && (gold_count > 1))
	{
		get_input(curr_buff->win);
		tmp = in;
		do {
			in = tmp;
			gold_count--;
			keyboard();
		} while (gold_count > 0);
		gold = FALSE;
	}
	else if (gold)
		gold = FALSE;
	else
	{
		get_input(com_win);
		if ((in >= '0') && (in <= '9') && (gold_count <= 0))
		{
			gold_count = 0;
			wmove(com_win, 0, 0);
			wclrtoeol(com_win);
			clr_cmd_line = TRUE;
			while ((in >= '0') && (in <= '9'))
			{
				gold_count = (gold_count * 10) + (in - '0');
				waddch(com_win, in);
				wrefresh(com_win);
				get_input(com_win);
			}
		}
		else
		{
			gold = TRUE;
			if (gold_count == 0)
				gold_count = 1;
		}
		tmp = in;
		do {
			in = tmp;
			keyboard();
			gold_count--;
		} while (gold_count > 0);
		gold_count = 0;
		gold = FALSE;
	}
}

void 
keyboard()		/* accept input from keyboard		*/
{
	if (in > 255)
		function_key();
	else if ((in > 31) && (in < 256) && (in != 127))
		insert(in);
	else if (in == 9)
		tab_insert();
	else if (((in > 0) && (in <= 31)) || (in == 127))
		control();
}

void 
def_key(string)		/* define a function or control key to a string */
char *string;
{
	char *temp, *tm1;
	int gold_flag;
	int key;

	temp = string;
	if (compare(temp, fn_GOLD_str, FALSE))
	{
		temp = next_word(temp);
		gold_flag = TRUE;
	}
	else
		gold_flag = FALSE;
	if (toupper(*temp) == 'F')
	{
		temp++;
		key = 0;
		while ((*temp >= '0') && (*temp <= '9'))
		{
			key = key * 10 + (*temp - '0');
			temp++;
		}
		temp = next_word(temp);
		tm1 = xalloc(strlen(temp) + 1);
		copy_str(temp, tm1);
		if (key <= 64)
		{
			if (gold_flag)
			{
				g_f[key] = tm1;
				g_f_changed[key] = (char) TRUE;
			}
			else
			{
				f[key] = tm1;
				f_changed[key] = (char) TRUE;
			}
		}
		else if (com_win_initialized)
		{
			wmove(com_win,0,0);
			wclrtoeol(com_win);
			wprintw(com_win, unkn_syntax_msg, temp);
			wrefresh(com_win);
			clr_cmd_line = TRUE;
		}
	}
	else if (toupper(*temp) == 'K')
	{
		temp++;
		key = 0;
		while ((*temp >= '0') && (*temp <= '9'))
		{
			key = key * 10 + (*temp - '0');
			temp++;
		}
		temp = next_word(temp);
		tm1 = xalloc(strlen(temp) + 1);
		copy_str(temp, tm1);
		if (key <= 4)
		{
			if (gold_flag)
				g_keypads[key] = tm1;
			else
				keypads[key] = tm1;
		}
		else if (com_win_initialized)
		{
			wmove(com_win,0,0);
			wclrtoeol(com_win);
			wprintw(com_win, unkn_syntax_msg, temp);
			wrefresh(com_win);
			clr_cmd_line = TRUE;
		}
	}
	else if (*temp == '^')
	{
		temp++;
		if (*temp == '?')
			key = 31;
		else
			key = toupper(*temp) - 'A';
		temp = next_word(temp);
		tm1 = xalloc(strlen(temp) + 1);
		copy_str(temp, tm1);
		if ((key != 8) && (key != 16) && (key != 18) && ((key >= 0) && (key < 32)))
		{
			if (gold_flag)
			{
				g_ctr[key] = tm1;
				g_ctr_changed[key] = (char) TRUE;
			}
			else
			{
				ctr[key] = tm1;
				ctr_changed[key] = (char) TRUE;
			}
		}
		else if (com_win_initialized)
		{
			wmove(com_win, 0,0);
			wclrtoeol(com_win);
			key += 65;
			wprintw(com_win, cant_redef_msg, key);
			clr_cmd_line = TRUE;
			wrefresh(com_win);
		}
	}
	else if (com_win_initialized)
	{
		wmove(com_win,0,0);
		wclrtoeol(com_win);
		wprintw(com_win, unkn_syntax_msg, temp);
		wrefresh(com_win);
		clr_cmd_line = TRUE;
	}
	get_key_assgn();
	if (info_window)
		paint_info_win();
}


