/*
 |	help.c
 |
 |	$Header: /home/hugh/sources/aee/RCS/help.c,v 1.10 2009/03/12 02:55:15 hugh Exp hugh $
 */

/*
 |	Copyright (c) 1986 - 1988, 1991 - 1995, 1999 Hugh Mahon.
 */

#include "aee.h"

char *help_file_list[4] = {
	"/usr/local/aee/help.ae", 
	"/usr/local/lib/help.ae", 
	"~/.help.ae", 
	"help.ae" 
	};

void 
help()		/* help function	*/
{
	int counter;
	char *string;

	go_on = TRUE;
	if ((fp = fopen(ae_help_file, "r")) == NULL)
	{
		for (counter = 0; (counter < 4) && (fp == NULL); counter++)
		{
			string = resolve_name(help_file_list[counter]);
			fp = fopen(string, "r");
		}
		if (fp == NULL)
		{
			wmove(com_win, 0, 0);
			werase(com_win);
			wprintw(com_win, help_err_msg, ae_help_file);
			wrefresh(com_win);
			clr_cmd_line = TRUE;
			go_on = FALSE;
		}
	}
	if (go_on)
	{
		help_win = newwin(LINES-1, COLS, 0, 0);
		if ((windows) && (num_of_bufs > 1))
		{
			t_buff = first_buff;
			while (t_buff != NULL)
			{
				werase(t_buff->win);
				wrefresh(t_buff->win);
				werase(t_buff->footer);
				wrefresh(t_buff->footer);
				t_buff = t_buff->next_buff;
			}
		}
		else
		{
			werase(curr_buff->win);
			wrefresh(curr_buff->win);
		}
		help_line = xalloc(512);
		do 
		{
			outfile();
			ask();
		}
		while (go_on);
		free(help_line);
		werase(help_win);
		wrefresh(help_win);
		delwin(help_win);
		paint_info_win();
		new_screen();
		clr_cmd_line = TRUE;
	}
}

void 
outfile()	/* output data from help file until a form-feed (separator of subjects)	*/
{
	char *prompt;
	int counter;
	int scroll_flag = FALSE;

	counter = 0;
	werase(help_win);
	wmove(help_win,0,0);
	while (((sline = fgets(help_line, 512, fp)) != NULL) && (*sline != 12))
	{
		counter++;
		if (counter == LINES)
		{
			wrefresh(help_win);
			prompt = get_string(prompt_for_more, FALSE);
			if (prompt != NULL)
				free(prompt);
			scroll_flag = TRUE;
			counter = 0;
		}
		if (scroll_flag)
		{
			wmove(help_win, 0, 0);
			wdeleteln(help_win);
			wmove(help_win, (LINES - 2), 0);
		}
		wprintw(help_win, "%s", sline);
	}
	wrefresh(help_win);
}

void 
ask()	/* prompt user what help topic do they wish to learn about	*/
{
	char *topic;
	char *tline;

	topic = get_string(topic_prompt, FALSE);
	if (*topic == '\0')
		go_on = FALSE;
	else if (*topic != 9)
	{
		rewind(fp);
		do
		{
			tline = sline = fgets(help_line, 512, fp);
			if (tline != NULL)
			{
				while ((*tline != '\n') && (*tline != '\0'))
					tline++;
				*tline = '\0';
			}
		}
		while ((sline != NULL) && (strncmp(sline, topic, strlen(topic))));
		if (sline == NULL)
		{
			wmove(help_win, LINES -2, 0);
			wclrtoeol(help_win);
			wstandout(help_win);
			wprintw(help_win, topic_err_msg, topic);
			wstandend(help_win);
			wrefresh(help_win);
			rewind(fp);
			free(topic);
			topic = get_string(continue_prompt, TRUE);
		}
		else
			tline = sline = fgets(help_line, 512, fp);
	}
	else if (*topic == '\t')
		rewind(fp);
	free(topic);
}


