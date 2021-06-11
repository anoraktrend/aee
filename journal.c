/*
 |	journal.c
 |
 |		This file contains the routines for handling the journal file
 |		for ae.
 |
 |	$Header: /home/hugh/sources/aee/RCS/journal.c,v 1.41 2010/07/18 21:53:50 hugh Exp hugh $
 */

/*
 |	This file contains routines for performing journalling for aee and 
 |	xae.  Journalling is writing lines and information about them to a 
 |	file when the cursor moves away from them (if they have been changed 
 |	during the time the cursor was on the line).
 |
 |	The data structure ae_file_info contains the information about 
 |	locations in the file for where to find the information about the 
 |	current line, the information about the previous line, the info 
 |	about the next line, and the location of the actual text.  The 
 |	length of the line (which is important!) is contained in the 'text' 
 |	structure for the line.  
 |
 |	The format that the information is written into the file is as 
 |	follows:
 |
 |	+--+--+--+--+
 |	|  |  |  |  |	previous line info location
 |	+--+--+--+--+
 |
 |	+--+--+--+--+
 |	|  |  |  |  |   next line info location
 |	+--+--+--+--+
 |
 |	+--+--+--+--+
 |	|  |  |  |  |	text location
 |	+--+--+--+--+
 |
 |	+--+--+--+--+
 |	|  |  |  |  |	text length (in bytes, including terminating NULL)
 |	+--+--+--+--+
 |
 */

/*
 |	Also in this file are routines for managing information about what 
 |	journal files have been written, and the relationship between the 
 |	journal file and the file being journalled.  This helps when 
 |	recovering from a disaster, and could be used to determine duplicate 
 |	edit sessions, or a failed previous session.
 |
 |	The name of the file is ~/.aeeinfo, and there is a lock file named 
 |	~/.aeeinfo.L.  The format of ~/.aeeinfo is as follows:
 |
 |	32 30 /home/hugh/sources/aee/journal.c /tmp/hugh/journal/journal.c.rv
 |
 |	where the first number is the length (decimal) if the full file name, 
 |	the second number is the length of the journal file name, followed by 
 |	the file name, and the name of the journal file.  The fields are 
 |	separated by a single space (ASCII 32).
 |
 |
 */

/*
 |
 |	Copyright (c) 1994, 1995, 1996, 1998, 2000, 2001, 2002, 2009, 2010 Hugh Mahon.
 |
 */

char *jrn_vers_str = "@(#)                           journal.c $Revision: 1.41 $";

#include "aee.h"

#include <time.h>

/*
 |	writes the contents of the line, updates the value stored in 
 |	memory of where the start of the line is stored in the file
 */

void 
write_journal(buffer, line)
struct bufr *buffer;
struct text *line;
{
	int counter;
	int ret_val = 0;

	ret_val = line->file_info.line_location = 
				lseek(buffer->journ_fd, 0, SEEK_END);

	for (counter = 0; (counter < line->line_length) && (ret_val != -1); 
			counter += min(1024, (line->line_length - counter)))
	{
		ret_val = write(buffer->journ_fd, &(line->line[counter]), 
			min(1024, (line->line_length - counter)));
	}

	update_journal_entry(buffer, line);

	line->changed = FALSE;
}

/*
 |	update the information for a line that has already been written, or
 |	write a line for the first after its values have been initialised
 |	by journ_info_init
 */

void 
update_journal_entry(buffer, line)
struct bufr *buffer;
struct text *line;
{
	int ret_val;

	if (line->file_info.info_location == NO_FURTHER_LINES)
	{
		journ_info_init(buffer, line);
		return ;
	}


	ret_val = lseek(buffer->journ_fd, line->file_info.info_location, 
								SEEK_SET);
	ret_val |= write(buffer->journ_fd, 
		  (char *)&(line->file_info.prev_info), sizeof(unsigned long));
	ret_val |= write(buffer->journ_fd, 
		  (char *)&(line->file_info.next_info), sizeof(unsigned long));
	ret_val |= write(buffer->journ_fd, 
		  (char *)&(line->file_info.line_location), 
		  	sizeof(unsigned long));
	ret_val |= write(buffer->journ_fd, (char *)&(line->line_length), 
						sizeof(int));
}

/*
 |	change journal info at first line if the first line is deleted 
 |	while editing
 */

void 
remove_journ_line(buffer, line)
struct bufr *buffer;
struct text *line;
{
	if (line->prev_line == NULL)
	{
		/*
		 |	This was the first line, now the next line is.
		 */

		line->next_line->file_info.info_location = 
					line->file_info.info_location;
		update_journal_entry(buffer, line->next_line);
		if (line->next_line->next_line != NULL)
		{
			line->next_line->next_line->file_info.prev_info = 
						line->file_info.info_location;
			update_journal_entry(buffer, line->next_line->next_line);
		}
	}
	else if (line->next_line == NULL)
	{
		/*
		 |	This was the last line, now the prev line is.
		 */

		line->prev_line->file_info.next_info = NO_FURTHER_LINES;
		update_journal_entry(buffer, line->prev_line);

		/*
		 |	no point in writing to file since no way to 
		 |	access current line now
		 */
	}
	else
	{
		/*
		 |	Need to allow previous line to see next line, 
		 |	vice-versa.
		 */
		
		line->prev_line->file_info.next_info = 
				line->next_line->file_info.info_location;
		line->next_line->file_info.prev_info = 
				line->prev_line->file_info.info_location;
		update_journal_entry(buffer, line->next_line);
		update_journal_entry(buffer, line->prev_line);
	}
}

/*
 |	initialize the location of where the information is to reside 
 |	(using lseek), initialize information for this line of where the 
 |	previous line is, set the information where to find this line for 
 |	the previous line, next line then
 |
 |	update current line (write for the first time)
 |	update the previous line (if prev_line != NULL)
 |	update the next line (if next_line != NULL)
 |
 */

void 
journ_info_init(buffer, line)
struct bufr *buffer;
struct text *line;
{
	int ret_val;

	ret_val = line->file_info.info_location = 
					lseek(buffer->journ_fd, 0, SEEK_END);
	if (ret_val == -1)
	{
		wmove(com_win, 0, 0);
		wstandout(com_win);
		wprintf(com_win, journal_err_str);
		wstandend(com_win);
		wrefresh(com_win);
		return;
	}

	if (line->prev_line != NULL)
	{
		line->file_info.prev_info = 
				line->prev_line->file_info.info_location;
		line->prev_line->file_info.next_info = 
				line->file_info.info_location;
	}

	if (line->next_line != NULL)
	{
		line->file_info.next_info = 
				line->next_line->file_info.info_location;
		line->next_line->file_info.prev_info = 
				line->file_info.info_location;
	}
	else
		line->file_info.next_info = NO_FURTHER_LINES;

	update_journal_entry(buffer, line);

	if (line->prev_line != NULL)
		update_journal_entry(buffer, line->prev_line);

	if ((line->next_line != NULL) && 
	    (line->file_info.next_info != NO_FURTHER_LINES))
		update_journal_entry(buffer, line->next_line);
}

/*
 |	read the journal entry from the file
 */

void 
read_journal_entry(buffer, line)
struct bufr *buffer;
struct text *line;
{
	int counter;

	lseek(buffer->journ_fd, line->file_info.info_location, SEEK_SET);
	read(buffer->journ_fd, (char *)&(line->file_info.prev_info), 
						sizeof(unsigned long));
	read(buffer->journ_fd, (char *)&(line->file_info.next_info), 
						sizeof(unsigned long));
	read(buffer->journ_fd, (char *)&(line->file_info.line_location), 
						sizeof(unsigned long));
	read(buffer->journ_fd, (char *)&(line->line_length), 
						sizeof(unsigned int));

	lseek(buffer->journ_fd, line->file_info.line_location, SEEK_SET);

	line->line = malloc(line->line_length);

	for (counter = 0; counter < line->line_length; 
			counter += min(1024, (line->line_length - counter)))
	{
		read(buffer->journ_fd, &(line->line[counter]), 
			min(1024, (line->line_length - counter)));
	}
}

/*
 |	read the contents of the edited file from the journal file
 |	read the file up to the first '\n' (line feed), which is where 
 |	the journal information begins
 |	
 |	This reads the contents of the edited file, plus sets the values 
 |	in the data structures to allow continued editing (it is recreated 
 |	within the editor, so why not?).
 */

int 
recover_from_journal(buffer, file_name)
struct bufr *buffer;
char *file_name;
{
	int counter = 0;
	struct text *line;
	char temp;
	char done = FALSE;
	char name[1024]; /* name of file stored in journal file */
	ssize_t len = 0;

	if ((buffer->journ_fd = open(file_name, O_RDONLY)) == -1)
	{
		/*
		 |	Unable to open journal file.
		 */

		buffer->journ_fd = NULL;
		return(1);
	}

	memset(name, 0, 1024);

	/*
	 |	get to first record in file (skip the file name)
	 */

	do
	{
		len = read(buffer->journ_fd, &temp, 1);
		if (len == 0)
		{
			/*
			 | we've got a zero length file, there's a problem
			 */
			buffer->journalling = FALSE;
			return(1);
		}
		if ((counter < 1024) && (temp != '\n'))
			name[counter] = temp;
		counter++;
	}
	while (temp != '\n');

	line = buffer->first_line;

	/*
	 |	set up first entry
	 */

	line->file_info.info_location = counter;

	do
	{
		read_journal_entry(buffer, line);
		line->vert_len = (scanline(line, line->line_length) / COLS) + 1;
		line->max_length = line->line_length;
		if (line->file_info.next_info != NO_FURTHER_LINES)
		{
			line->next_line = txtalloc();
			buffer->num_of_lines++;
			line->next_line->prev_line = line;
			line->next_line->next_line = NULL;
			line->next_line->line_number = line->line_number + 1;
			line = line->next_line;
			line->file_info.next_info = 0;
			line->file_info.info_location = 
				line->prev_line->file_info.next_info;
		}
		else
			done = TRUE;
	} 
	while (!done);

	curr_buff->pointer = buffer->first_line->line;
	close(buffer->journ_fd);

	change = TRUE;
	buffer->changed = TRUE;

	/*
	 |	open journal for further changes
	 */

	if (buffer->journalling)
	{
		if ((buffer->journ_fd = 
			open(buffer->journal_file, O_WRONLY)) == -1)
		{
			wprintw(com_win, cant_opn_rcvr_fil_msg);
			buffer->journalling = FALSE;
		}
	}


	/*
	 |	Since the user can specify a journal name without specifying 
	 |	the name of the file it journalled, use the name stored in 
	 |	the journal file.
	 */

	if ((buffer->full_name == NULL) || (*buffer->full_name == '\0'))
	{
		buffer->full_name = strdup(name);
		if ((buffer->full_name != NULL) && (*buffer->full_name != '\0'))
		{
			buffer->file_name = ae_basename(buffer->full_name);
			if (strcmp(main_buffer_name, buffer->name))
				buffer->name = strdup(buffer->file_name);
		}
	}

	/*
	 |	Success!
	 */

	return(0);
}

/*
 |	journal database file routines
 */

static char *lock_file_name = NULL;
static char *db_file_name = NULL;

/*
 |	write a lock file so that another process doesn't try to open 
 |	the file
 |
 |	This is an atomic action since the open should fail if the file 
 |	already exists, thus it can't be opened if someone else has a 
 |	lock in place.
 */

int 
lock_journal_fd()
{
	int counter = 0;
	int ret_val;

	if (lock_file_name == NULL)
		lock_file_name = resolve_name("~/.aeeinfo.L");

	/*
	 |	At some point may want to put in a check for an old lock file.
	 */


	while (((ret_val = open(lock_file_name, (O_CREAT | O_EXCL), 0700)) == -1) && (counter < 3))
	{
		sleep(1);
		counter++;
	}
	if (ret_val != -1)
		close(ret_val);
	return(ret_val);
}

/*
 |	remove the lock file
 */

void 
unlock_journal_fd()
{
	unlink(lock_file_name);
}

/*
 |	free the memory used in the list of files being edited and their 
 |	journal files
 */

void 
free_db_list(list)
struct journal_db *list;
{
	if (list->next != NULL)
		free_db_list(list->next);
	free(list->journal_name);
	if (list->file_name != NULL)
		free(list->file_name);
	free(list);
}

/*
 |	read the file ~/.aeeinfo (which contains the name of the file 
 |	being edited and the name of the journal file associated with it)
 */

struct journal_db *
read_journal_db()
{
	char buffer[4092];
	char *tmp;
	int file_name_len;
	int journ_name_len;
	struct journal_db *top_of_list = NULL;
	struct journal_db *list = NULL;
	FILE *db_fp;

	if (lock_journal_fd() == -1)
		return(NULL);

	if (db_file_name == NULL)
		db_file_name = resolve_name("~/.aeeinfo");
	db_fp = fopen(db_file_name, "r");

	if (db_fp == NULL)
	{
		unlock_journal_fd();
		return(NULL);
	}

	while ((tmp = fgets(buffer, 4092, db_fp)) != NULL)
	{
		if (list == NULL)
			top_of_list = list = 
			     (struct journal_db *) malloc(sizeof(struct journal_db));
		else
		{
			list->next = (struct journal_db *) 
					malloc(sizeof(struct journal_db));
			list = list->next;
		}
		list->next = NULL;

		if ((*tmp == ' ') || (*tmp == '\t'))
			tmp = next_word(tmp);
		file_name_len = atoi(tmp);
		tmp = next_word(tmp);
		journ_name_len = atoi(tmp);

		if (file_name_len > 0)
		{
			tmp = next_word(tmp);
			list->file_name = (char *) malloc(file_name_len + 1);
			strncpy(list->file_name, tmp, file_name_len);
			list->file_name[file_name_len] = '\0';
		}
		else
			list->file_name = NULL;

		tmp = next_word(tmp);
		list->journal_name = (char *) malloc(journ_name_len + 1);
		strncpy(list->journal_name, tmp, journ_name_len);
		list->journal_name[journ_name_len] = '\0';
	}
	fclose(db_fp);
	return(top_of_list);
}

/*
 |	Write the file ~/.aeeinfo .
 */

void 
write_db_file(list)
struct journal_db *list;
{
	struct journal_db *tmp;
	FILE *db_fp;

	db_fp = fopen(db_file_name, "w");

	if (db_fp == NULL)
		return;

	for (tmp = list; (db_fp != NULL) && (tmp != NULL); tmp = tmp->next)
	{
		if (tmp->file_name != '\0')
		{
			fprintf(db_fp, "%d %d %s %s\n", 
				(int)strlen(tmp->file_name), 
				(int)strlen(tmp->journal_name), 
				tmp->file_name, 
							tmp->journal_name);
		}
		else
		{
			fprintf(db_fp, "%d %d %s\n", 0, 
				(int)strlen(tmp->journal_name), 
				tmp->journal_name);
		}
	}

	fclose(db_fp);

	unlock_journal_fd();

}

/*
 |	add an entry to the file ~/.aeeinfo (a new file being edited and 
 |	its associated journal file)
 */

void 
add_to_journal_db(buffer)
struct bufr *buffer;
{
	struct journal_db *top_of_list;
	struct journal_db *list;

	top_of_list = list = read_journal_db();
	if (list == (struct journal_db *) -1)	/* could not open lock file */
		return;

	if (list == NULL)
	{
		top_of_list = list = 
			(struct journal_db *) malloc(sizeof(struct journal_db));
	}
	else
	{
		/*
		 |	Could check for duplicate file being edited here.
		 */

		while (list->next != NULL)
			list = list->next;
		list->next = (struct journal_db *) malloc(sizeof(struct journal_db));
		list = list->next;
	}
	list->next = NULL;

	list->journal_name = strdup(buffer->journal_file);
	if (*buffer->full_name != '\0')
		list->file_name = strdup(buffer->full_name);
	else
		list->file_name = NULL;

	write_db_file(top_of_list);
	free_db_list(top_of_list);
}

/*
 |	remove an entry from the file ~/.aeeinfo (the editor is being 
 |	exited and it is time to delete the journal file, so remove the 
 |	reference)
 */

void 
rm_journal_db_entry(buffer)
struct bufr *buffer;
{
	struct journal_db *top_of_list;
	struct journal_db *list;
	struct journal_db *tmp;
	int changed = FALSE;

	top_of_list = list = read_journal_db();
	if (list == (struct journal_db *) NULL)	/* could not open db file */
	{
		return;
	}

	if ((!strcmp(buffer->journal_file, top_of_list->journal_name)) && 
		((top_of_list->file_name == NULL) || 
		   ((top_of_list->file_name != NULL) && 
		    (!strcmp(buffer->full_name, top_of_list->file_name)))))
	{
		top_of_list = list->next;
		tmp = list;
		changed = TRUE;
	}
	else
	{
		while (((list->next != NULL) && (buffer->journal_file != NULL) && 
		      (buffer->full_name != NULL) &&
		      (!((list->next->file_name != NULL) && 
			    (!strcmp(buffer->full_name, list->next->file_name) 
		 && !strcmp(buffer->journal_file, list->next->journal_name)))))
		  && (!((list->next->file_name == NULL) && 
		     !strcmp(buffer->journal_file, list->next->journal_name))))
		{
 			list = list->next;
		}
		if (list->next != NULL)
		{
			tmp = list->next;
			list->next = list->next->next;
			changed = TRUE;
		}
	}
	if (changed == TRUE)
	{
		if (top_of_list == NULL)	/* no entries for file	*/
		{
			unlink(db_file_name);
			unlock_journal_fd();
		}
		else
		{
			write_db_file(top_of_list);
		}
		tmp->next = NULL;
		free_db_list(tmp);
	}
	else
		unlock_journal_fd();

	if (top_of_list != NULL)
		free_db_list(top_of_list);
}

/*
 |	check for the existence of a directory, and if it doesn't exist 
 |	create it (will recursively go through and create the directory 
 |	from the highest location where a directory does exist)
 */

int
create_dir(name)
char *name;
{
	char *path;
	struct stat buf;
	int ret_val;

	if ((name == (char *)NULL) || (*name == '\0'))
		return(-1);

	ret_val = stat(name, &buf);
	if (ret_val == -1)
	{
		path = ae_dirname(name);
		ret_val = create_dir(path);
		if (ret_val != -1)
			ret_val = mkdir(name, 0700);
		free(path);
	}
	return(ret_val);
}

/*
 |	determine a name for a journal file and make sure no other journal 
 |	file of that name already exists (if one does, create a name based
 |	on the current date and time)
 */

void 
journal_name(buffer, file_name)
struct bufr *buffer;
char *file_name;
{
	char *temp, *buff, *name;
	int count;

	if (*file_name == '\0')
	{
		struct tm *time_info;
		time_t t;
		int ret_val;

		/*
		 |	create a file name based on the date, e.g., 
		 |	950913221206.rv
		 |	Not a guarantee of uniqueness, but certainly not 
		 |	likely to be repeated.
		 */
		file_name = (char *)malloc(14);
		t = time(NULL);
		time_info = gmtime(&t);
		ret_val = strftime(file_name, 13, "%y%m%d%H%M%S", time_info);
	}

	if ((journal_dir != NULL) && (*journal_dir != '\0'))
	{
		name = ae_basename(file_name);
		buffer->journal_file = temp = 
			xalloc(strlen(name) + 5 + strlen(journal_dir));
		strcpy(temp, journal_dir);
		strcat(temp, "/");
		strcat(temp, name);
	}

	if (buffer->journal_file == NULL)
	{
		buffer->journal_file = temp = xalloc(strlen(file_name) + 4);
		strcpy(temp, file_name);
	}

	buff = ae_basename(temp);

	temp = buff;

	if (strlen(temp) >= (MAX_NAME_LEN - 3))
	{
		for (count = 1; count < (MAX_NAME_LEN - 3); count++)
			temp++;
	}
	else
	{
		while(*temp != '\0')
			temp++;
	}

	copy_str(".rv", temp);
}

/*
 |	create and initialize the journal file
 */

void 
open_journal_for_write(buffer)
struct bufr *buffer;
{
	int ret_val;
	int length;
	int counter;
	char *temp;
	struct tm *time_info;
	time_t t;

	if (((buffer->journ_fd = 
		open(buffer->journal_file, (O_CREAT | O_EXCL | O_WRONLY), 0600)) == -1) && 
				(errno == EEXIST))
	{
		/*
		 |	file already exists
		 */
		buffer->journal_file = realloc(buffer->journal_file, 
					(strlen(buffer->journal_file) + 17));
		/*
		 |	find the last '/'
		 */
		temp = strrchr(buffer->journal_file, '/');

		if (temp != NULL)
			temp++;
		else
			temp = buffer->journal_file;

		/*
		 |	create a file name based on the date, e.g., 
		 |	950913221206.rv
		 |	Not a guarantee of uniqueness, but certainly not 
		 |	likely to be repeated.
		 */
		t = time(NULL);
		time_info = gmtime(&t);
		ret_val = strftime(temp, 16, "%y%m%d%H%M%S.rv", time_info);

		/*
		 |	For some extra paranoia, check this one, and append 
		 |	a character (a-z) to get uniqueness.
		 */

		counter = 'a';
		length = strlen (buffer->journal_file);
		while (((buffer->journ_fd = 
			open(buffer->journal_file, (O_CREAT | O_EXCL | O_WRONLY), 0600)) == -1) 
				&& (errno == EEXIST) && (counter <= 'z'))
		{
			buffer->journal_file[length] = counter;
			buffer->journal_file[length + 1] = '\0';
			counter++;
		}
	}

	if (buffer->journ_fd == -1)
	{
		wprintw(com_win, cant_opn_rcvr_fil_msg);
		buffer->journalling = FALSE;
		buffer->journ_fd = NULL;
		return;
	}

	add_to_journal_db(buffer);

	if (*buffer->full_name != '\0')
	{
		ret_val = write(buffer->journ_fd, buffer->full_name, 
					strlen(buffer->full_name));
	}
	ret_val |= write(buffer->journ_fd, "\n", 1);

	journ_info_init(buffer, buffer->first_line);
}

/*
 |	delete a journal file and delete its entry from the file ~/.aeeinfo
 */

void 
remove_journal_file(buffer)
struct bufr *buffer;
{
	close(buffer->journ_fd);
	unlink(buffer->journal_file);
	rm_journal_db_entry(buffer);
}

