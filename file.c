/*
 |
 |	Handle file operations for aee.
 |	$Header: /home/hugh/sources/aee/RCS/file.c,v 1.34 2010/07/18 21:53:24 hugh Exp hugh $
 |
 |	Copyright (c) 1986 - 1988, 1991 - 1996, 2001, 2009, 2010 Hugh Mahon.
 |
 */

/*
 |	get the full path the the named file
 */

#include "aee.h"

struct menu_entries file_error_menu[] = {
	{"", NULL, NULL, NULL, NULL, MENU_WARN}, 
	{NULL, NULL, NULL, NULL, NULL, -1}, 
	{NULL, NULL, NULL, NULL, NULL, -1}, 
	{NULL, NULL, NULL, NULL, NULL, -1}
	};

char *file_being_edited_msg;

struct menu_entries file_being_edited_menu[] = {
	{"", NULL, NULL, NULL, NULL, MENU_WARN}, 
	{NULL, NULL, NULL, NULL, NULL, -1}, 
	{NULL, NULL, NULL, NULL, NULL, -1}, 
	{NULL, NULL, NULL, NULL, NULL, -1}
	};

char *file_modified_msg;

struct menu_entries file_modified_menu[] = {
	{"", NULL, NULL, NULL, NULL, MENU_WARN}, 
	{NULL, NULL, NULL, NULL, NULL, -1}, 
	{NULL, NULL, NULL, NULL, NULL, -1}, 
	{NULL, NULL, NULL, NULL, NULL, -1}, 
	{NULL, NULL, NULL, NULL, NULL, -1}
	};

int aee_write_status;

void 
show_pwd()
{
	char buffer[1024];
	char *buff;

	wmove(com_win, 0, 0);
	werase(com_win);
	buff = getcwd(buffer, 1024);
	if (buff != NULL)
	{
		wprintw(com_win, "%s", buff);
	}
	else
		wprintw(com_win, "%s", pwd_err_msg);
}

/*
 | get_full_path returns the full directory path for a file
 | If both 'path' and 'orig_path' are NULL then the current directory
 | is returned.
 | If orig_path is not NULL and path does not start with '/', then 
 | orig_path is prefixed to 'path'.
 |
 | In order to get the 'canonical' path, that is, without any '..' in 
 | the path name, once the path is created by concatenating orig_path 
 | and path, a chdir() is made to the path, the current directory 
 | obtained, then a chdir back to the directory in which the editor 
 | was when get_full_path was called.
 */

char *
get_full_path(path, orig_path)
char *path;
char *orig_path;
{
	char *buff = path;
	char long_buffer[1024];
	char curr_path[1024];
	char base_name[256];
	char *tmp;
	char *tmp2;
	char *tmp3;

		if (orig_path != NULL)
		{
			strcpy(long_buffer, orig_path);
			buff = long_buffer;
		}
		else
			buff = getcwd(long_buffer, 1024);
		if (buff != NULL)
		{
			buff = xalloc(strlen(long_buffer) + 
				(path != NULL ? strlen(path) : 0) + 2);
			strcpy(buff, long_buffer);
			if (path != NULL)
			{
				tmp = getcwd(curr_path, 1024);

				if (*path != '/')	/* a '/' indicates an absolute path */
				{
					strcat(buff, "/");
					strcat(buff, path);
				}
				else 
					strcpy(buff, path);
					
				tmp2 = ae_dirname(buff);
				if (tmp2 != (char *)NULL)
				{
					strcpy(base_name, ae_basename(buff));
					chdir(tmp2);
					tmp3 = getcwd(long_buffer, 1024);
					chdir(tmp);
					free(tmp2);
					strcpy(buff, long_buffer);
					strcat(buff, "/");
					strcat(buff, base_name);
				}
			}
		}
		else
			change_dir_allowed = FALSE;

	return(buff);
}

char *
ae_basename(name)
char *name;
{
	char *buff, *base;

	buff = base = name;

	while (*buff != '\0')
	{
		if (*buff == '/')
			base = ++buff;
		else
			buff++;
	}
	return(base);
}

char *
ae_dirname(path)
char *path;
{
	char *tmp = NULL;
	char buffer[1024];

	if (strchr(path, '/') != NULL)
	{
		strcpy(buffer, path);
		tmp = strrchr(buffer, '/');
		*tmp = '\0';
		tmp = (char *) malloc(strlen(buffer) + 1);
		strcpy(tmp, buffer);
	}
	return(tmp);
}

char *
buff_name_generator()
{
	char *buffer = (char *)malloc(4);
	char found;
	struct bufr *tmp_buff;
	int i1, i2;

	found = TRUE;
	/*
	 |	Buffers with the name already existed.
	 */
	buffer[3] = 0;
	buffer[2] = 0;
	i2 = -1;
	do
	{
		if (i2 >= 0)
			buffer[2] = i2 + 'A';
		buffer[1] = 0;
		i1 = -1;

		do 
		{
			if (i1 >= 0)
				buffer[1] = i1 + 'A';
			buffer[0] = 'A'; 
			do 
			{
				found = FALSE;
				tmp_buff = first_buff;
				while (tmp_buff != NULL)
				{
					if (!strcmp(buffer, tmp_buff->name))
						found = TRUE;
					tmp_buff = tmp_buff->next_buff;
				}
				if (found)
					buffer[0]++;

			} while ((buffer[0] <= 'Z') && (found)); 
			i1++;
		} while ((buffer[1] <= 'Z') && (found) );
		i2++; 
	}while ((buffer[2] <= 'Z') && (found));
	return(buffer);
}

int 
open_for_edit(string)
char *string;
{
	struct bufr *tmp_buff;
	char *short_name;
	char buffer[1024];
	char found = FALSE;
	int length = 0;
	int ret_val;

	if (restrict_mode())
	{
		return(FALSE);
	}

	if ((string != NULL) && (*string != '\0'))
	{
		short_name = ae_basename(string);
		do
		{
			tmp_buff = first_buff;
			found = FALSE;
			while (tmp_buff != NULL)
			{
				if (!strcmp(short_name, tmp_buff->name))
					found = TRUE;
				tmp_buff = tmp_buff->next_buff;
			}
			if (found)
			{
				if (length == 0)
				{
					strcpy(buffer, short_name);
					strcat(buffer, "A");
					length = strlen(buffer);
				}
				if (buffer[length-1] <= 'Z')
					buffer[length-1]++;
				short_name = buffer;
			}
		}
		while ((found) && (buffer[length-1] <= 'Z'));
		if ((found) && (buffer[length-1] > 'Z'))
		{
			/*
			 |	Buffers with the name already existed.
			 */
			 return(FALSE);
		}
	}
	else
	{
		short_name = buff_name_generator();
	}


	chng_buf(short_name);
	curr_buff->edit_buffer = TRUE;
	if (*string != '\0')
		tmp_file = strdup(string);
	else
		tmp_file = "";
	recv_file = TRUE;
	input_file = TRUE;
	ret_val = check_fp();

	if (ret_val == FALSE)
	{
		del_buf();
	}
	return ( ret_val );
}

void 
recover_op()
{
	struct journal_db *journal_list;
	struct journal_db *list_tmp;
	struct menu_entries *recover_tmp;
	int list_count;
	int counter;
	int choice;
	char *other_choice;
	struct bufr local_copy;
	char new_buff = FALSE;	/* indicate if a new buffer was created */

	/*
	 |	If restricted mode, there must have been a file specified on 
	 |	the invoking command line.
	 */
	if ((restrict_mode()) && (top_of_stack == NULL))
	{
		wmove(com_win, 0, 0);
		werase(com_win);
		wrefresh(com_win);
		menu_op(rae_err_menu);
		return;
	}

	journal_list = read_journal_db();

	for (list_count = 0, list_tmp = journal_list; list_tmp != NULL; 
			list_count++, list_tmp = list_tmp->next)
		;

	unlock_journal_fd();

	/*
	 | Recover from a failed edit session.  
	 | 1. Read the file ~/.aeeinfo, get the list of journal files.
	 | 2. Create a menu with names of files according to ~/.aeeinfo.
	 | 3. Menu also has entry 'other journal file', which allows the user
	 |    to specify another file.

	 Not yet implemented:
	 | 4. Before creating the menu, the routine checks if the journal file 
	 |    is currently open (someone is currently editing that file!).
	 | 5. After user selection, present a menu allowing the user to copy 
	 |    the journal file, or recover directly from the journal (since 
	 |    starting an edit session from the journal could change it, it 
	 |    might be nice to just allow the user to 'browse' the file).

	 Implemented:
	 | 5a. If there is only one file in the list, automatically open it.
	 | 6. Call recover_from_journal.
	 |
	 |
	 */


	if (list_count == 0)
	{
		/*
		 |	prompt for name of file to recover
		 */
		other_choice = get_string(recover_name_prompt, TRUE);
	}
	/*
	 | If there is just one file in the list, we could just put the user 
	 | into the one file.  I have found that this is not always a good 
	 | idea.

	else if (list_count == 1)
		other_choice = journal_list->journal_name;

	 |
	 */
	else
	{

		/*
		 |	Construct menu.
		 */

		recover_tmp = malloc((3 + list_count) * 
						sizeof(struct menu_entries));
		memset(recover_tmp, 0, ((3 + list_count) * sizeof(struct menu_entries)));

		for (counter = 1, list_tmp = journal_list;
			(counter <= list_count); 
				list_tmp = list_tmp->next, counter++)
		{
			recover_tmp[counter].item_string = (list_tmp->file_name != NULL) ? list_tmp->file_name : ae_basename(list_tmp->journal_name) ;
			recover_tmp[counter].procedure = NULL;
			recover_tmp[counter].ptr_argument = NULL;
			recover_tmp[counter].iprocedure = NULL;
			recover_tmp[counter].nprocedure = NULL;
			recover_tmp[counter].argument = -1;
		}

		/*
		 |	Assign the title of the menu, then add 'other file' to 
		 |	menu.
		 */

		recover_tmp[0].item_string = recover_menu_title;
		recover_tmp[list_count+1].item_string = other_recover_file_str;
		recover_tmp[list_count+2].item_string = NULL;
		
		/*
		 |	Present menu to user.
		 */


		choice = menu_op(recover_tmp);

		free(recover_tmp);

		if (choice == 0)
			return;
		else if (choice == (list_count + 1))
		{
			other_choice = get_string(recover_name_prompt, TRUE);
			in_file_name = NULL;
		}
		else
		{
			for (list_count = 1, list_tmp = journal_list; 
				list_count < choice; 
				list_count++, list_tmp = list_tmp->next)
					;
			other_choice = list_tmp->journal_name;
			in_file_name = list_tmp->file_name;
		}

	}

	/*
	 |	take user's choice, fill in data structures, recover, 
	 |	and return
	 */

	/*
	 |	Change to a new buffer to make sure we aren't messing 
	 |	with the user's edit session.
	 */

	local_copy.journalling = FALSE;

	if ((curr_buff->changed == FALSE) && 
	    (curr_buff->first_line->next_line == NULL) &&
	    (curr_buff->first_line->line_length == 1) &&
	    (*curr_buff->full_name == '\0'))
	{
		local_copy.journalling = curr_buff->journalling;
		local_copy.journ_fd = curr_buff->journ_fd;
		local_copy.journal_file = curr_buff->journal_file;
		local_copy.full_name = curr_buff->full_name;
		local_copy.file_name = curr_buff->file_name;
		local_copy.name = curr_buff->name;
	}
	else
	{
		chng_buf(buff_name_generator());
		new_buff = TRUE;
	}

	curr_buff->journal_file = other_choice;

	if (journ_on)
	{
		curr_buff->journalling = TRUE;
	}

	if (curr_buff->orig_dir == NULL)
	{
		curr_buff->orig_dir = get_full_path(NULL, NULL);
	}

	if (recover_from_journal(curr_buff, curr_buff->journal_file) != 0)
	{
		if (new_buff)
			del_buf();
		else
		{
			curr_buff->journalling = local_copy.journalling;
			curr_buff->journ_fd = local_copy.journ_fd ;
			curr_buff->journal_file = local_copy.journal_file ;
			curr_buff->full_name = local_copy.full_name ;
			curr_buff->file_name = local_copy.file_name ;
			curr_buff->name = local_copy.name ;
		}
		wmove(com_win,0,0);
		wclrtoeol(com_win);
		wprintw(com_win, bad_rcvr_msg);
		wrefresh(com_win);
	}
	else
	{
		/*
		 |	On success, clear flags and continue edit
		 |	session.
		 */

		if ((!new_buff) && (local_copy.journalling))
			remove_journal_file(&local_copy);

		curr_buff->edit_buffer = TRUE;
		curr_buff->curr_line = curr_buff->first_line;
		curr_buff->pointer = curr_buff->curr_line->line;
		new_screen();
		wmove(com_win,0,0);
		wclrtoeol(com_win);
		wprintw(com_win, rcvr_op_comp_msg);
		wrefresh(com_win);
#ifdef XAE
		if ((curr_buff->main_buffer) && (curr_buff->file_name != NULL))
			set_window_name(curr_buff->file_name);
#endif /* XAE */

		recv_file = FALSE;
		input_file = FALSE;
	}

}

int 
check_fp()	/* open or close files according to flags recv_file, 
		   recover, and input_file	*/
{
	char *buff;
	int line_num;
	char buffer[512];
	int value;

	clr_cmd_line = TRUE;

	if (recover)	/* get data from journal file	*/
	{
		recover = FALSE;
		buff = in_file_name = tmp_file;

		if (journ_on)
		{
			curr_buff->journalling = TRUE;
		}

		if (curr_buff->orig_dir == NULL)
		{
			curr_buff->orig_dir = get_full_path(NULL, NULL);
		}

		in_file_name = tmp_file = 
				get_full_path(in_file_name, 
							curr_buff->orig_dir);

		short_file_name = ae_basename(in_file_name);

		curr_buff->file_name = short_file_name;
		curr_buff->full_name = in_file_name;

		journal_name(curr_buff, in_file_name);

		if (recover_from_journal(curr_buff, 
				curr_buff->journal_file) != 0)
		{
			wmove(com_win,0,0);
			wclrtoeol(com_win);
			wprintw(com_win, bad_rcvr_msg);
			wrefresh(com_win);
			journ_on = FALSE;
		}
		else
		{
			/*
			 |	On success, clear flags and continue edit
			 |	session.
			 */

			curr_buff->curr_line = curr_buff->first_line;
			curr_buff->pointer = curr_buff->curr_line->line;
			midscreen(curr_buff->scr_vert, curr_buff->position);
			wmove(com_win,0,0);
			wclrtoeol(com_win);
			wprintw(com_win, rcvr_op_comp_msg);
			wrefresh(com_win);
#ifdef XAE
			if (curr_buff->main_buffer)
				set_window_name(curr_buff->file_name);
#endif /* XAE */

			recv_file = FALSE;
			input_file = FALSE;
		}
	}


	if (recv_file)		/* read in a file	*/
	{
		tmp_vert = curr_buff->scr_vert;
		tmp_horz = curr_buff->scr_horz;
		tmp_line = curr_buff->curr_line;
		if (input_file)		/* get file on command line	*/
		{
			/*
			 |	We want to do a few things here.  First, get 
			 |	the path of the file we're editing. Second, 
			 |	find out if we are already editing the file
			 |	(compare with files listed in ~/.aeeinfo).  
			 |	Third get the journal file name.  Fourth, 
			 |	open the journal file for writing.
			 */
			buff = short_file_name = in_file_name = tmp_file;

			if (curr_buff->orig_dir == NULL)
				curr_buff->orig_dir = 
						get_full_path(NULL, NULL);

			if (*buff != '\0')
			{
				tmp_file = in_file_name = 
				get_full_path(in_file_name, curr_buff->orig_dir);
				short_file_name = ae_basename(in_file_name);
			
				curr_buff->file_name = short_file_name;
				curr_buff->full_name = in_file_name;
			}
			else
			{
				recv_file = FALSE;
				input_file = FALSE;
			}

			if (journ_on)
			{
				struct journal_db *journal_list;
				struct journal_db *list_tmp;
				int match_found = FALSE;

				journal_list = read_journal_db();
				unlock_journal_fd();

				for (list_tmp = journal_list; 
				      (!match_found) && (list_tmp != NULL);
					list_tmp = list_tmp->next)
				{
					/*
					 |	comparing the basename, which 
					 |	may prove to be an erroneous 
					 |	match, but more complete match 
					 |	would be a lot more work
					 */
					
					if (list_tmp->file_name != NULL)
					{
						match_found = !strcmp(in_file_name, list_tmp->file_name);
					}
				}

				/*
				 |	free the memory allocated for the list
				 */
				list_tmp = journal_list;
				while (list_tmp != NULL)
				{
					journal_list = journal_list->next;
					free (list_tmp);
					list_tmp = journal_list;
				}

				if (match_found)
				{
					sprintf(buffer, file_being_edited_msg, short_file_name);
					file_being_edited_menu[0].item_string = buffer;
					match_found = menu_op(file_being_edited_menu);
					if (match_found == 1)
					{
						if (curr_buff->main_buffer)
							quit("");
						else
						{
							recv_file = FALSE;
							input_file = FALSE;
							return(FALSE);
						}
					}
				}

				curr_buff->journalling = TRUE;
				journal_name(curr_buff, in_file_name);
				open_journal_for_write(curr_buff);
			}
#ifdef XAE
			if ((curr_buff->main_buffer) && 
				(curr_buff->file_name != NULL))
				set_window_name(curr_buff->file_name);
#endif /* XAE */
			if (*buff == '\0')
			{
				wmove(com_win,0,0);
				wclrtoeol(com_win);
				wprintw(com_win, no_file_string);
				return(TRUE);
			}

		}
		wmove(com_win,0,0);
		wclrtoeol(com_win);
		value = stat(tmp_file, &buf);
		buf.st_mode &= ~07777;
		if ((value != -1) && (buf.st_mode != 0100000) && (buf.st_mode != 0))
		{

			if (input_file)
			{
				sprintf(buffer, file_is_dir_msg, ae_basename(tmp_file));
				file_error_menu[0].item_string = buffer;
				file_error_menu[1].item_string = continue_prompt;
				menu_op(file_error_menu);
				if (curr_buff->main_buffer)
					quit("");
			}
			else
			{
				wprintw(com_win, file_is_dir_msg, tmp_file);
				wrefresh(com_win);
			}
			recv_file = FALSE;
			input_file = FALSE;
			return(FALSE);
		}
		if ((get_fd = open(tmp_file, O_RDONLY)) == -1)
		{
			wmove(com_win,0,0);
			wclrtoeol(com_win);
			if (input_file)
			{
				if ((get_fp= fopen(tmp_file,"w")) == NULL)
				{
					if ((errno == ENOTDIR) || (errno == ENOENT))
					{
						file_error_menu[0].item_string = path_not_a_dir_msg;
					}
					else if ((errno == EACCES) || (errno == EROFS) || (errno == ETXTBSY) || (errno == EFAULT))
					{
						sprintf(buffer, access_not_allowed_msg, ae_basename(in_file_name));
						file_error_menu[0].item_string = buffer;
					}
					file_error_menu[1].item_string = continue_prompt;
					menu_op(file_error_menu);
					if (curr_buff->main_buffer)
						quit("");
					recv_file = FALSE;
					input_file = FALSE;

					return(FALSE);
				}
				else
				{
					fclose(get_fp);
					unlink(tmp_file);
					wprintw(com_win, new_file_msg, tmp_file);
				}
			}
			else
				wprintw(com_win, cant_open_msg, tmp_file);
			wrefresh(com_win);
			recv_file = FALSE;
			input_file = FALSE;
		}
		else
			get_file(tmp_file);

		if (recv_file)	/* got a file		*/
		{
			close(get_fd);
			recv_file = FALSE;
			line_num = curr_buff->curr_line->line_number;
			curr_buff->scr_vert = tmp_vert;
			curr_buff->scr_horz = tmp_horz;
			if (input_file)
				curr_buff->curr_line = curr_buff->first_line;
			else
				curr_buff->curr_line = tmp_line;
			curr_buff->pointer = curr_buff->curr_line->line;
			midscreen(curr_buff->scr_vert, curr_buff->position);
			if (input_file)	/* get the file on the command line */
			{
				if (access(in_file_name, 2))
				{
					if ((errno == ENOTDIR) || (errno == EACCES) || (errno == EROFS) || (errno == ETXTBSY) || (errno == EFAULT))
						wprintw(com_win, ", %s", read_only_msg);
				}
				if (start_at_line != NULL)
				{
					line_num = atoi(start_at_line) - 1;
					move_rel("d", line_num);
					line_num = 0;
					start_at_line = NULL;
				}
				value = stat(tmp_file, &curr_buff->fileinfo);
				if (value == -1)
				{
					curr_buff->fileinfo.st_mtime = 0;
					curr_buff->fileinfo.st_size = 0;
				}
				wrefresh(com_win);
				input_file = FALSE;
			}
			else
			{
				wmove(com_win, 0, 0);
				werase(com_win);
				wprintw(com_win, fin_read_file_msg, tmp_file);
				wrefresh(com_win);
			}
			in = 0;
		}
	}
	wrefresh(com_win);
	return(TRUE);
}

/*
 |	get_file is called from two places: check_fp and sh_command
 |	When called from check_fp, recv_file is set, and input_file may be 
 |	set, indicating this is a file to be edited.
 |	When called from sh_command, it is reading from a pipe from a forked 
 |	process.
 */

void 
get_file(file_name)	/* read specified file into current buffer	*/
char *file_name;
{
	int can_write;		/* file mode allows write		*/
	int can_read;		/* file has at least one character	*/
	int length;		/* length of line read by read		*/
	int append;		/* should text be appended to current line */
	struct text *temp_line;

	if (recv_file)		/* if reading a file			*/
	{
		wmove(com_win, 0, 0);
		wclrtoeol(com_win);
		wprintw(com_win, reading_file_msg, file_name);
		if ((can_write = access(file_name, 2)))	/* check permission to write */
		{
			if ((errno == ENOTDIR) || (errno == EACCES) || (errno == EROFS) || (errno == ETXTBSY) || (errno == EFAULT))
				wprintw(com_win, read_only_msg);
		}
		wrefresh(com_win);
	}
	if (curr_buff->curr_line->line_length > 1)	/* if current line is not blank	*/
	{
		insert_line(FALSE);
		left(FALSE);
		append = FALSE;
	}
	else
		append = TRUE;
	can_read = FALSE;		/* test if file has any characters  */
	while (((length = read(get_fd, in_string, 512)) != 0) && (length != -1))
	{
		can_read = TRUE;  /* if set file has at least 1 character   */
		get_line(length, in_string, &append);
	}
	if ((can_read) && (curr_buff->curr_line->line_length == 1)) 
	{
		temp_line = curr_buff->curr_line->prev_line;
		temp_line->next_line = curr_buff->curr_line->next_line;
		if (temp_line->next_line != NULL)
			temp_line->next_line->prev_line = temp_line;
		if (curr_buff->journalling)
			remove_journ_line(curr_buff, curr_buff->curr_line);
		free(curr_buff->curr_line);
		curr_buff->curr_line = temp_line;
		curr_buff->num_of_lines--;
	}
	if (input_file)	/* if this is the file to be edited display number of lines	*/
	{
		wmove(com_win, 0, 0);
		wclrtoeol(com_win);
		wprintw(com_win, file_read_lines_msg, in_file_name, curr_buff->curr_line->line_number);
		wrefresh(com_win);
	}
	else
		curr_buff->changed = TRUE;
}

void 
get_line(length, in_string, append)	/* read string and split into lines */
int length;		/* length of string read by read		*/
char *in_string;	/* string read by read				*/
int *append;	/* TRUE if must append more text to end of current line	*/
{
	char *str1;
	char *str2;
	int num;		/* offset from start of string		*/
	int char_count;		/* length of new line (or added portion	*/
	int temp_counter;	/* temporary counter value		*/
	struct text *tline;	/* temporary pointer to new line	*/
	int first_time;		/* if TRUE, the first time through the loop */
	static int last_char_cr = FALSE; /* set if the last character of a 
					buffer was a carriage return	*/

	str2 = in_string;
	num = 0;
	first_time = TRUE;
	while (num < length)
	{
		if (!first_time)
		{
			if (num < length)
			{
				str2++;
				num++;
			}
		}
		else
			first_time = FALSE;
		str1 = str2;
		char_count = 1;
		/* find end of line	*/
		if (!last_char_cr)
		{
			if (text_only)
			{
				while ((*str2 != '\n') && (num < length))
				{
					if (((*str2 == '\r') && (*(str2 + 1) == '\n')) 
					|| ((*str2 == '\r') && ((num + 1) == length)))
					{
						curr_buff->dos_file = TRUE;
						if ((num + 1) == length)
							last_char_cr = TRUE;
					}
					else
						char_count++;
					str2++;
					num++;
				}
			}
			else
			{
				while ((*str2 != '\n') && (num < length))
				{
					char_count++;
					str2++;
					num++;
				}
			}
		}
		else
			last_char_cr = FALSE;
		if (!(*append))	/* if not append to current line, insert new one */
		{
			tline = txtalloc();	/* allocate data structure for next line */
			tline->line_number = curr_buff->curr_line->line_number + 1;
			tline->next_line = curr_buff->curr_line->next_line;
			tline->prev_line = curr_buff->curr_line;
			curr_buff->curr_line->next_line = tline;
			if (tline->next_line != NULL)
				tline->next_line->prev_line = tline;
			curr_buff->curr_line = tline;
			curr_buff->num_of_lines++;
			curr_buff->curr_line->line = curr_buff->pointer = xalloc(char_count);
			curr_buff->curr_line->line_length = char_count;
			curr_buff->curr_line->max_length = char_count;
		}
		else
		{
			curr_buff->pointer = resiz_line(char_count, curr_buff->curr_line, curr_buff->curr_line->line_length); 
			curr_buff->curr_line->line_length += (char_count - 1);
		}
		for (temp_counter = 1; temp_counter < char_count; temp_counter++)
		{
			*curr_buff->pointer = *str1;
			curr_buff->pointer++;
			str1++;
		}
		if (((*str2 == '\n') || ((num == length) && (num < 512))) && 
			(curr_buff->journalling))
			write_journal(curr_buff, curr_buff->curr_line);
		*curr_buff->pointer = '\0';
		*append = FALSE;
		curr_buff->curr_line->vert_len = (scanline(curr_buff->curr_line, curr_buff->curr_line->line_length) / COLS) + 1;
		if ((num == length) && (*str2 != '\n'))
			*append = TRUE;
	}
}

char *
is_in_string(string, substring)	/* a strstr() look-alike for systems without
				   strstr() */
char * string, *substring;
{
	char *full, *sub;

	for (sub = substring; (sub != NULL) && (*sub != '\0'); sub++)
	{
		for (full = string; (full != NULL) && (*full != '\0'); 
				full++)
		{
			if (*sub == *full)
				return(full);
		}
	}
	return(NULL);
}

/*
 |	handle names of the form "~/file", "~user/file", 
 |	"$HOME/foo", "~/$FOO", etc.
 */

char *
resolve_name(name)
char *name;
{
	char long_buffer[1024];
	char short_buffer[128];
	char *buffer;
	char *slash;
	char *tmp;
	char *start_of_var;
	int offset;
	int index;
	int counter;
	struct passwd *user;
	char *name_buffer;

	name_buffer = (char *) malloc(strlen(name) + 1);
	strcpy(name_buffer, name);

	if (name_buffer[0] == '~') 
	{
		if (name_buffer[1] == '/')
		{
			index = getuid();
			user = (struct passwd *) getpwuid(index);
			slash = name_buffer + 1;
		}
		else
		{
			slash = strchr(name_buffer, '/');
			if (slash == NULL) 
				return(name_buffer);
			*slash = '\0';
			user = (struct passwd *) getpwnam((name_buffer + 1));
			*slash = '/';
		}
		if (user == NULL) 
		{
			return(name_buffer);
		}
		buffer = xalloc(strlen(user->pw_dir) + strlen(slash) + 1);
		strcpy(buffer, user->pw_dir);
		strcat(buffer, slash);
	}
	else
		buffer = name_buffer;

	if (is_in_string(buffer, "$"))
	{
		tmp = buffer;
		index = 0;
		
		while ((*tmp != '\0') && (index < 1024))
		{

			while ((*tmp != '\0') && (*tmp != '$') && 
				(index < 1024))
			{
				long_buffer[index] = *tmp;
				tmp++;
				index++;
			}

			if ((*tmp == '$') && (index < 1024))
			{
				counter = 0;
				start_of_var = tmp;
				tmp++;
				if (*tmp == '{') /* } */	/* bracketed variable name */
				{
					tmp++;				/* { */
					while ((*tmp != '\0') && 
						(*tmp != '}') && 
						(counter < 128))
					{
						short_buffer[counter] = *tmp;
						counter++;
						tmp++;
					}			/* { */
					if (*tmp == '}')
						tmp++;
				}
				else
				{
					while ((*tmp != '\0') && 
					       (*tmp != '/') && 
					       (*tmp != '$') && 
					       (counter < 128))
					{
						short_buffer[counter] = *tmp;
						counter++;
						tmp++;
					}
				}
				short_buffer[counter] = '\0';
				if ((slash = getenv(short_buffer)) != NULL)
				{
					offset = strlen(slash);
					if ((offset + index) < 1024)
						strcpy(&long_buffer[index], slash);
					index += offset;
				}
				else
				{
					while ((start_of_var != tmp) && (index < 1024))
					{
						long_buffer[index] = *start_of_var;
						start_of_var++;
						index++;
					}
				}
			}
		}

		if (index == 1024)
			return(buffer);
		else
			long_buffer[index] = '\0';

		if (name_buffer != buffer)
			free(buffer);
		buffer = xalloc(index + 1);
		strcpy(buffer, long_buffer);
	}

	return(buffer);
}

int 
write_file(file_name)	/* write current buffer to specified file	*/
char *file_name;
{
	char cr, lf;
	char *tmp_point;
	struct text *out_line;
	int lines, charac;
	int temp_pos;
	int write_flag;
	int can_write;
	int write_ret = 0;
	int retval = 0;
	int value;
	struct stat filestat;
	char *short_name;
	char buffer[1024];

	clr_cmd_line = TRUE;
	charac = lines = 0;
	aee_write_status = FALSE;
	if ((curr_buff->full_name == NULL) || (strcmp(curr_buff->full_name, file_name)))
	{
		wmove(com_win, 0, 0);
		wclrtoeol(com_win);
		can_write = access(file_name, 2);
		if (can_write)
		{
			write_flag = FALSE;
			if (errno == ENOTDIR)
				wprintw(com_win, path_not_a_dir_msg);
			else if ((errno == EACCES) || (errno == EROFS) || (errno == ETXTBSY) || (errno == EFAULT))
				wprintw(com_win, no_write_access_msg, file_name);
			else if (errno != ENOENT)
			{
				tmp_point = get_string(file_exists_prompt, TRUE);
				if ((toupper(*tmp_point) == toupper(*no_char)) 
					|| (*tmp_point == '\0'))
					write_flag = FALSE;
				else
					write_flag = TRUE;
				free(tmp_point);
			}
			else
				write_flag = TRUE;
		}
		else
		{
			tmp_point = get_string(file_exists_prompt, TRUE);
			if ((toupper(*tmp_point) == toupper(*no_char)) 
				|| (*tmp_point == '\0'))
				write_flag = FALSE;
			else
				write_flag = TRUE;
			free(tmp_point);
		}
		wrefresh(com_win);
	}
	else
	{
		if (!strcmp(curr_buff->full_name, file_name))
		{
			value = stat(file_name, &filestat);
			if ((value == 0) && 
			    ((filestat.st_mtime != curr_buff->fileinfo.st_mtime) || 
			     (filestat.st_size != curr_buff->fileinfo.st_size)))
			{
			/*
			 |	notify user that the file has changed and ask 
			 |	whether to continue file write or not
			 */
				short_name = ae_basename(file_name);
				sprintf(buffer, file_modified_msg, short_name);
				file_modified_menu[0].item_string = buffer;
				value = menu_op(file_modified_menu);
				if (value == 1)
				{
					return(FALSE);
				}
				else if (value == 3)
				{
					sprintf(buffer, "><file-diffs!diff %s -", file_name);
					command(buffer);
					return(FALSE);
				}
			}
		}
		wmove(com_win,0,0);
		wclrtoeol(com_win);
		can_write = access(file_name, 2);
		if (can_write)
		{
			write_flag = FALSE;
			if (errno == ENOTDIR)
				wprintw(com_win, path_not_a_dir_msg);
			else if ((errno == EACCES) || (errno == EROFS) || (errno == ETXTBSY) || (errno == EFAULT))
				wprintw(com_win, no_write_access_msg, file_name);
			else 
				write_flag = TRUE;
		}
		else
		{
			change = FALSE;
			write_flag = TRUE;
		}
		wrefresh(com_win);
	}
	if (write_flag)
	{
		if ((write_fp = fopen(file_name, "w")) == NULL)
		{
			wmove(com_win,0,0);
			wclrtoeol(com_win);
			wprintw(com_win, cant_creat_fil_msg, file_name);
			wrefresh(com_win);
			return(FALSE);
		}
		else
		{
			wmove(com_win,0,0);
			wclrtoeol(com_win);
			wprintw(com_win, writing_msg, file_name);
			wrefresh(com_win);
			cr = '\r';
			lf = '\n';
			out_line = curr_buff->first_line;
			while (out_line != NULL)
			{
				temp_pos = 1;
				tmp_point= out_line->line;
				while ((temp_pos < out_line->line_length) &&
					(write_ret != EOF))
				{
					write_ret = putc(*tmp_point, write_fp);
					tmp_point++;
					temp_pos++;
				}
				charac += out_line->line_length;
				out_line = out_line->next_line;
				if ((text_only) && (curr_buff->dos_file))
					putc(cr, write_fp);
				write_ret = putc(lf, write_fp);
				lines++;
			}
			retval = fclose(write_fp);
			if (retval != 0)
			{
				wstandout(com_win);
				wprintw(com_win, write_err_msg);
				wstandend(com_win);
				wrefresh(com_win);
				return(FALSE);
			}
			wmove(com_win,0,0);
			wclrtoeol(com_win);
			wprintw(com_win, file_written_msg, file_name,lines,charac);
			if (write_ret == EOF)
			{
				wstandout(com_win);
				wprintw(com_win, write_err_msg);
				wstandend(com_win);
				
			}
			wrefresh(com_win);
			value = stat(file_name, &curr_buff->fileinfo);
			if (value == -1)
			{
				curr_buff->fileinfo.st_mtime = 0;
				curr_buff->fileinfo.st_size = 0;
			}
			aee_write_status = TRUE;
			return(TRUE);
		}
	}
	else
		return(FALSE);
}

int 
file_write_success()
{
	return (aee_write_status);
}

void 
diff_file()
{
	char tmp_buff[1024];
	char buff_name[512];
	struct bufr *tmp;
	char *curr_name;

	/* 
	 |  want to do a diff of the current edit buffer with the 
	 |  file it is associated with
	 |
	 |  another way to deal with buffer names is to use the function
	 |  buff_name_generator()
	 */
	if ((curr_buff->file_name != (char *)NULL) && (*curr_buff->file_name != '\0'))
	{
		curr_name = curr_buff->name;
		sprintf(buff_name, "%s-diffs", curr_buff->file_name);
		tmp = first_buff;
		while ((tmp != NULL) && (strcmp(tmp->name, buff_name)))
			tmp = tmp->next_buff;
		
		if (tmp != NULL)
		{
			chng_buf(buff_name);
			del_buf();
			chng_buf(curr_name);
		}

		sprintf(tmp_buff, "><%s!diff %s -", buff_name, curr_buff->full_name);
		command(tmp_buff);
	}
	else
	{
		wmove(com_win, 0, 0);
		werase(com_win);
		wprintw(com_win, no_file_string);
	}
}
