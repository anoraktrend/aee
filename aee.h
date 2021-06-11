/*
 |	aee.h
 |
 |	$Header: /home/hugh/sources/aee/RCS/aee.h,v 1.44 2010/07/18 21:52:43 hugh Exp hugh $
 |
 |	Copyright (c) 1986 - 1988, 1991 - 1996, 1999, 2000, 2001, 2002, 2010 Hugh Mahon.
 |
 */

#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <pwd.h>
#include <limits.h>

#ifdef HAS_SYS_WAIT
#include <sys/wait.h>
#endif

#ifdef HAS_STDLIB
#include <stdlib.h>
#endif

#ifdef HAS_STDARG
#include <stdarg.h>
#endif

#ifdef HAS_UNISTD
#include <unistd.h>
#endif

#ifdef HAS_CTYPE
#include <ctype.h>
#endif


extern int eightbit;		/* eight bit character flag		*/

#ifndef XAE
#ifdef NCURSE
#include "new_curse.h"
#else
#include <curses.h>
#endif
#else	/* ifdef XAE	*/
#ifdef xae11
#include "Xcurse.h"
#else
#include "X10curse.h"
#endif	/* xae11 */
#endif	/* XAE */

extern struct stat buf;

#ifndef XAE
#ifdef SYS5			/* System V specific tty operations	*/
extern struct termio old_term;
extern struct termio new_term;
#include <fcntl.h>
#else
extern struct sgttyb old_term;
extern struct sgttyb new_term;
#endif
#endif	/* XAE */

#ifndef SIGCHLD
#define SIGCHLD SIGCLD
#endif

#define TAB 9

#ifdef NAME_MAX
#define MAX_NAME_LEN NAME_MAX
#else
#define MAX_NAME_LEN 15
#endif	/* NAME_MAX	*/

#ifndef NO_CATGETS
#include <locale.h>
#include <nl_types.h>

extern nl_catd catalog;
#else
#define catgetlocal(a, b) (b)
#endif /* NO_CATGETS */

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif /* max */

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif /* min */


#define Mark 1	/* mark text (don't keep previous paste buffer contents)  */
#define Append 2	/* place new text into paste buffer after contents  */
#define Prefix 3	/* place new text into paste buffer before contents */

extern int errno;

#define NO_FURTHER_LINES 0x0 /* indicates end of data, since offset 
				will never be zero in journal file (file name 
				is written there, not data)      */

struct ae_file_info {			/* info about line in journal file */
	unsigned long info_location;	/* location of this info in file*/
	unsigned long prev_info;	/* info for previous line	*/
	unsigned long next_info;	/* info for next line		*/
	unsigned long line_location;	/* actual data in line		*/
	/* use length of line in bytes from the text data structure below */
	};

struct text {
	char *line;			/* line of characters		*/
	int line_number;		/* line number			*/
	int line_length;  /* actual number of characters in the line 
					   (includes NULL at end of line) */
	int max_length; /* maximum number of characters the line handles */
	int vert_len;	/* number of lines the current line occupies on the screen */
	struct text *next_line;		/* next line of text		*/
	struct text *prev_line;		/* previous line of text	*/
	struct ae_file_info file_info;
	char changed;		/* flag indicating info in line changed */
	};

extern struct text *first_line;	/* first line of current buffer		*/
extern struct text *dlt_line;	/* structure for info on deleted line	*/
extern struct text *curr_line;	/* current line cursor is on		*/
extern struct text *fpste_line;	/* first line of select buffer		*/
extern struct text *cpste_line;	/* current paste/select line		*/
extern struct text *paste_buff;	/* first line of paste buffer		*/
extern struct text *pste_tmp;	/* temporary paste pointer		*/
extern struct text *tmp_line;	/* temporary line pointer		*/
extern struct text *srch_line;	/* temporary pointer for search routine */


struct files {		/* structure to store names of files to be edited*/
	char *name;		/* name of file				*/
	struct files *next_name;
	};

extern struct files *top_of_stack;

struct bufr {			/* structure for names of buffers	*/
	char *name;		/* name of buffer			*/
	struct text *first_line;/* pointer to first line of buffer	*/
	struct bufr *next_buff; /* pointer to next record		*/
	struct text *curr_line;	/* pointer to the current line in buffer*/
	int scr_vert;		/* the last vertical value of buffer	*/
	int scr_horz;		/* the last horizontal value in buffer	*/
	int scr_pos;		/* horizontal offset from start of line	*/
	int position;		/* the last value of position in buffer */
	int abs_pos;		/* 'absolute' horizontal position	*/
	char *pointer;		/* the last value of point in buffer	*/
	WINDOW *win;		/* the window pointer for buffer	*/
	WINDOW *footer;		/* window containing name of window	*/
	int lines;		/* number of lines			*/
	int last_line;		/* last line visible in window		*/
	int last_col;		/* last column visible in the window	*/
	int num_of_lines;	/* number of lines of text total in buffer  */
	int absolute_lin;	/* number of lines from top		*/
	int window_top;	/* # of lines top of window is from top of screen  */
	int journ_fd;		/* file descriptor for journal file	*/
	char journalling;	/* flag indicating if journalling active 
							for this buffer */
	char *journal_file;	/* name of journal file for this buffer	*/
	char *file_name;	/* name of file being edited		*/
	char *full_name;	/* full name of file being edited	*/
	char changed;		/* contents of the buffer have changed	*/
	char *orig_dir;		/* original directory where edit started */
	char main_buffer;	/* true if this is the first buffer 	*/
	char edit_buffer;	/* true if created as a result of edit cmd */
	char dos_file;		/* true if file uses (or is to use) CR/LF 
				   instead of just LF as line terminator   */
	struct stat fileinfo;	/* information about the file obtained 
				   using stat(2)			*/
	};

extern struct bufr *first_buff;	/* first buffer in list			*/
extern struct bufr *curr_buff;	/* pointer to current buffer		*/
extern struct bufr *t_buff;	/* temporary pointer for buffers	*/

struct tab_stops {		/* structure to store tab stops		*/
	int column;		/* column for tab			*/
	struct tab_stops *next_stop;
	};

extern struct tab_stops *tabs;

struct del_buffs {
	int flag;
	char character;
	char *string;
	int length;
	struct del_buffs *prev;
	struct del_buffs *next;
	};

/*
 |	structure for mapping journal files to file names
 */

struct journal_db {
	char *file_name;
	char *journal_name;
	struct journal_db *next;
	};

#define CHNG_SYMBOL(a) (a != 0 ? '+' : ' ')

extern struct del_buffs *undel_first;
extern struct del_buffs *undel_current;

extern WINDOW *com_win;		/* command window			*/
extern WINDOW *help_win;	/* window for help facility		*/
extern int windows;		/* flag for windows or no windows	*/
extern WINDOW *info_win;

extern int lines_moved;		/* number of lines moved in search	*/
extern int d_wrd_len;		/* length of deleted word		*/
extern int value;		/* temporary integer value		*/
extern int tmp_pos;		/* temporary position			*/
extern int tmp_vert;		/* temporary vertical position		*/
extern int tmp_horz;		/* temporary horizontal position	*/
extern int repl_length;		/* length of string to replace		*/
extern int pst_pos;		/* position in cpste_line->line		*/
extern int gold_count;		/* number of times to execute pressed key */
extern int fildes;		/* file descriptor			*/
extern int get_fd;		/* get file descriptor			*/
extern int write_fd;		/* write file descriptor		*/
extern int num_of_bufs;		/* the number of buffers that exist	*/
extern int temp_stdin;		/* temporary storage for stdin		*/
extern int temp_stdout;		/* temp storage for stdout descriptor	*/
extern int temp_stderr;		/* temp storage for stderr descriptor	*/
extern int pipe_out[2];		/* pipe file desc for output		*/
extern int pipe_in[2];		/* pipe file descriptors for input	*/
extern int line_wrap;		/* should line extend forever?		*/
extern int right_margin;	/* right margin	(if observ_margins = TRUE) */
extern int left_margin;		/* left margin				*/
extern int info_type;
extern int info_win_height;
extern int local_LINES;	/* copy of LINES, to detect when win resizes	*/
extern int local_COLS;	/* copy of COLS, to detect when win resizes	*/
extern int bit_bucket;	/* file descriptor to /dev/null			*/
extern int tab_spacing;	/* spacing for tabs				*/

/*
 |	boolean flags
 */

extern char mark_text;		/* flag to indicate if MARK is active	*/
extern char journ_on;		/* flag for journaling			*/
extern char input_file;		/* indicate to read input file		*/
extern char recv_file;		/* indicate reading a file		*/
extern char edit;		/* continue executing while true	*/
extern char gold;		/* 'gold' function key pressed		*/
extern char recover;		/* set true if recover operation to occur */
extern char case_sen;		/* case sensitive search flag		*/
extern char change;		/* indicate changes have been made to file*/
extern char go_on;		/* loop control for help function	*/
extern char clr_cmd_line;	/* flag set to clear command line	*/
extern char literal;		/* is search string to be interpreted literally? */
extern char forward;	/* if TRUE, search after cursor, else search before cursor */
extern char echo_flag;		/* allow/disallow echo in init file	*/
extern char status_line;	/* place status info in the bottom line	*/
extern char overstrike;		/* overstrike / insert mode flag	*/
extern char indent;		/* auto-indent mode flag		*/
extern char expand;		/* expand tabs flag			*/
extern char shell_fork;
extern char out_pipe;		/* flag that info is piped out		*/
extern char in_pipe;		/* flag that info is piped in		*/
extern char observ_margins;	/* should margins be observed		*/
extern char right_justify;	/* justify right margin when formatting */
extern char free_d_line;	/* free d_line (already freed from del list) */
extern char free_d_word;	/* free d_word (already freed from del list) */
extern char info_window;	/* is info window displayed?		*/
extern char auto_format;	/* automatically format paragraph	*/
extern char formatted;		/* has paragraph been formatted?	*/
extern char window_resize;	/* flag to indicate a resize has occurred */
extern char change_dir_allowed;	/* flag to indicate if chdir is allowed */
extern char restricted;		/* flag to indicate restricted mode	*/
extern char com_win_initialized;
extern char text_only;		/* editor is to treat file being read as 
				   text only (not a binary file)	*/
extern char ee_mode_menu; 	/* make main menu look more like ee's	*/

#define CHAR_DELETED 1
#define CHAR_BACKSPACE 2
#define WORD_DELETED 3
#define LINE_DELETED 4

extern long mask;		/* mask for sigblock			*/
#ifdef XAE
extern int win_width;		/* width and height of edit session window */
extern int win_height;
extern char *xsrch_string;
extern char *xold_string;
extern char *xnew_string;
extern Window testwin;
#endif

extern char in_string[513];	/* input buffer string			*/
extern char *pst_line;		/* start of paste line			*/
extern char *pst_pnt;		/* current position within paste line	*/
extern char *pste1;		/* temporary pointers for cut/paste ops */
extern char *pste2;
extern char *pste3;
extern char *old_string;	/* old string to be replaced		*/
extern char *u_old_string;	/* upper case verson of old_string	*/
extern char *new_string;	/* new string to insert instead of old	*/
extern char *srch_str;		/* pointer for search string		*/
extern char *u_srch_str;	/* pointer to non-case sensitive search */
extern char *in_file_name;	/* name of input file and path		*/
extern char *short_file_name;	/* name of input file only		*/
extern char *journ;		/* name of journal file			*/
extern char *journal_dir;	/* name of journal directory		*/
extern char *out_file;		/* name of output file if not input file*/
extern char *tmp_file;		/* temporary file name			*/
extern char d_char;		/* deleted character			*/
extern char *d_word;		/* deleted word				*/
extern char *d_line;		/* deleted line				*/
extern char *term_type;		/* type of terminal being used		*/
extern char *help_line;		/* input line for help facility		*/
extern char *sline;		/* temporary line pointer for help fac	*/
extern char *subject;		/* subject user wants to learn about	*/
extern char *start_of_string;	/* start of search string		*/
extern char match_char;		/* character found by search for match	*/
extern char in_buff_name[128];	/* input buffer name			*/
extern char out_buff_name[128];	/* output buffer name			*/
extern char *start_at_line;	/* move to this line at start of session*/
extern char *print_command;	/* string to hold command for printing file */
extern char nohighlight;

extern char *ctr[32];		/* control key strings			*/
extern char ctr_changed[32];	/* control key strings changed flags	*/
extern char *g_ctr[32];		/* gold control key strings		*/
extern char g_ctr_changed[32];	/* gold control key strings changed 	*/
extern char *f[64];		/* function key strings			*/
extern char f_changed[64];	/* function key strings			*/
extern char *g_f[64];		/* gold function key strings		*/
extern char g_f_changed[64];	/* gold function key strings		*/
extern char *ctrl_table[];
extern char *keypads[5];	/* keypad keys from terminfo		*/
extern char *g_keypads[5];	/* keypad keys from terminfo		*/
extern char *ctrl_table[];

extern int in;			/* input character			*/

extern FILE *fp;		/* file pointer for input		*/
extern FILE *outp;		/* pointer for output file		*/
extern FILE *temp_fp;		/* temporary file pointer		*/
extern FILE *get_fp;		/* read file pointer			*/
extern FILE *write_fp;		/* write file pointer			*/

#define MAX_HELP_LINES	15
#define MAX_HELP_COLS	129

extern char info_data[MAX_HELP_LINES][MAX_HELP_COLS];

struct edit_keys {
	char *macro;
	char *description;
	short fkey, gfkey, ckey, gckey;
	};

extern struct edit_keys assignment[];

#define gold_key_index 65

#define READ_FILE 1
#define WRITE_FILE 2
#define SAVE_FILE 3


/*
 |	The following structure allows menu items to be flexibly declared.
 |	The first item is the string describing the selection, the second 
 |	is the address of the procedure to call when the item is selected,
 |	and the third is the argument for the procedure.
 |
 |	The first menu item will be the title of the menu, with NULL 
 |	parameters for the procedure and argument, followed by the menu items.
 |
 |	If the 'argument' value for the menu title is 1 (MENU_WARN) the 
 |	menu is to be used to display info to the user (the message about 
 |	using Escape is not displayed).
 |
 |	If the procedure value is NULL, the menu item is displayed, but no 
 |	procedure is called when the item is selected.  The number of the 
 |	item will be returned.  If the sixth (argument) parameter is -1, no 
 |	argument is given to the procedure when it is called.
 */

#if defined(__STDC__) || defined(__cplusplus)
#define P_(s) s
#else
#define P_(s) ()
#endif

#define MENU_WARN 1

struct menu_entries {
	char *item_string;
	int (*procedure)P_((struct menu_entries *));
	struct menu_entries *ptr_argument;
	int (*iprocedure)P_((int));
	void (*nprocedure)P_((void));
	unsigned int argument;
	};

#undef P_
/*
 |	allocate space here for the strings that will be in the menu
 */

extern struct menu_entries modes_menu[];
extern int modes_initialized;

#define NUM_MODES_ITEMS 19

extern char *mode_strings[NUM_MODES_ITEMS];

extern struct menu_entries config_dump_menu[];
extern struct menu_entries leave_menu[];
extern struct menu_entries leave_menu_2[];
extern struct menu_entries file_menu[];
extern struct menu_entries search_menu[];
extern struct menu_entries spell_menu[];
extern struct menu_entries misc_menu[];
extern struct menu_entries edit_menu[];
extern struct menu_entries main_menu[];
extern struct menu_entries del_buff_menu[];
extern struct menu_entries rae_err_menu[];
extern struct menu_entries file_being_edited_menu[];
extern struct menu_entries file_modified_menu[];


#define INFO_WIN_HEIGHT_DEF 6

/*
 |	defines for type of data to show in info window
 */

#define CONTROL_KEYS 1
#define COMMANDS     2

extern char *commands[72];
extern char *init_strings[41];

/*
 |	memory debugging macros
 */

#ifdef DEBUG
extern char *dxalloc();
extern char *drealloc();
#define xalloc(a) dxalloc(a, __LINE__)
#define resiz_line(a, b, c) dresiz_line(a, b, c, __LINE__)
extern FILE *error_fp;
#endif /* DEBUG */

/*
 |	declaration for localizable strings
 */

extern char *ae_help_file, *SL_line_str, *SL_col_str, *SL_lit_str, *SL_nolit_str;
extern char *SL_fwd_str, *SL_rev_str, *SL_over_str, *SL_insrt_str, *SL_indent_str;
extern char *SL_noindnt_str, *SL_marg_str, *SL_nomarg_str, *SL_mark_str, *ascii_code_str;
extern char *left_err_msg, *right_err_msg, *help_err_msg, *prompt_for_more, *topic_prompt;
extern char *topic_err_msg, *continue_prompt, *printing_msg, *EXPAND_str, *NOEXPAND_str;
extern char *NOJUSTIFY_str, *JUSTIFY_str, *EXIT_str, *QUIT_str, *AUTOFORMAT_str;
extern char *NOAUTOFORMAT_str, *INFO_str, *NOINFO_str, *TABS_str, *UNTABS_str;
extern char *WRITE_str, *READ_str, *SAVE_str, *LITERAL_str, *NOLITERAL_str;
extern char *STATUS_str, *NOSTATUS_str, *MARGINS_str, *NOMARGINS_str, *INDENT_str;
extern char *NOINDENT_str, *OVERSTRIKE_str, *NOOVERSTRIKE_str, *LEFTMARGIN_str, *RIGHTMARGIN_str;
extern char *LINE_str, *FILE_str, *COPYRIGHT_str, *CHARACTER_str, *REDRAW_str;
extern char *RESEQUENCE_str, *AUTHOR_str, *VERSION_str, *CASE_str, *NOCASE_str;
extern char *EIGHT_str, *NOEIGHT_str, *WINDOWS_str, *NOWINDOWS_str, *DEFINE_str;
extern char *SHOW_str, *HELP_str, *PRINT_str, *BUFFER_str, *DELETE_str;
extern char *GOLD_str, *tab_msg, *file_write_prompt_str, *file_read_prompt_str, *left_mrg_err_msg;
extern char *right_mrg_err_msg, *left_mrg_setting, *right_mrg_setting, *line_num_str, *lines_from_top;
extern char *total_lines_str, *current_file_str, *char_str, *key_def_msg, *unkn_syntax_msg;
extern char *current_buff_msg, *unkn_cmd_msg, *usage_str, *read_only_msg, *no_rcvr_fil_msg;
extern char *cant_opn_rcvr_fil_msg, *fin_read_file_msg, *rcvr_op_comp_msg, *file_is_dir_msg, *path_not_a_dir_msg;
extern char *access_not_allowed_msg, *new_file_msg, *cant_open_msg, *bad_rcvr_msg, *reading_file_msg;
extern char *file_read_lines_msg, *other_buffs_exist_msg, *changes_made_prompt, *no_write_access_msg, *file_exists_prompt;
extern char *cant_creat_fil_msg, *writing_msg, *file_written_msg, *searching_msg, *fwd_srch_msg;
extern char *rev_srch_msg, *str_str, *not_fnd_str, *search_prompt_str, *mark_not_actv_str;
extern char *mark_active_str, *mark_alrdy_actv_str, *no_buff_named_str, *press_ret_to_cont_str, *fn_GOLD_str;
extern char *fn_DL_str, *fn_DC_str, *fn_CL_str, *fn_NP_str, *fn_PP_str;
extern char *fn_NB_str, *fn_PB_str, *fn_UDL_str, *fn_UDC_str, *fn_DW_str;
extern char *fn_UDW_str, *fn_UND_str, *fn_EOL_str, *fn_BOL_str, *fn_BOT_str;
extern char *fn_EOT_str, *fn_FORMAT_str, *fn_MARGINS_str, *fn_NOMARGINS_str, *fn_IL_str;
extern char *fn_PRP_str, *fn_RP_str, *fn_MC_str, *fn_PSRCH_str, *fn_SRCH_str;
extern char *fn_AL_str, *fn_AW_str, *fn_AC_str, *fn_PW_str, *fn_CUT_str;
extern char *fn_FWD_str, *fn_REV_str, *fn_MARK_str, *fn_UNMARK_str, *fn_APPEND_str;
extern char *fn_PREFIX_str, *fn_COPY_str, *fn_CMD_str, *fn_PST_str, *fn_RD_str;
extern char *fn_UP_str, *fn_DOWN_str, *fn_LEFT_str, *fn_RIGHT_str, *fn_BCK_str;
extern char *fn_CR_str, *fn_EXPAND_str, *fn_NOEXPAND_str, *fn_EXIT_str, *fn_QUIT_str;
extern char *fn_LITERAL_str, *fn_NOLITERAL_str, *fn_STATUS_str, *fn_NOSTATUS_str, *fn_INDENT_str;
extern char *fn_NOINDENT_str, *fn_OVERSTRIKE_str, *fn_NOOVERSTRIKE_str, *fn_CASE_str, *fn_NOCASE_str;
extern char *fn_WINDOWS_str, *fn_NOWINDOWS_str, *fn_HELP_str, *fn_MENU_str, *fwd_mode_str;
extern char *rev_mode_str, *ECHO_str, *cmd_prompt, *PRINTCOMMAND_str, *replace_action_prompt;
extern char *replace_prompt_str, *replace_r_char, *replace_skip_char, *replace_all_char, *quit_char;
extern char *yes_char, *no_char, *cant_find_match_str, *fmting_par_msg, *cancel_string;
extern char *menu_too_lrg_msg, *shell_cmd_prompt, *shell_echo_msg, *spell_in_prog_msg, *left_marg_prompt;
extern char *left_mrgn_err_msg, *right_mrgn_prompt, *right_mrgn_err_msg, *cant_redef_msg, *buff_msg;
extern char *cant_chng_while_mark_msg, *too_many_parms_msg, *buff_is_main_msg, *cant_del_while_mark, *main_buffer_name;
extern char *cant_del_buf_msg, *HIGHLIGHT_str, *NOHIGHLIGHT_str;
extern char *non_unique_cmd_msg;
extern char *ON, *OFF;
extern char *save_file_name_prompt, *file_not_saved_msg, *cant_reopen_journ;
extern char *write_err_msg, *jrnl_dir, *CD_str;
extern char 	*no_dir_entered_msg,  *path_not_dir_msg, *path_not_permitted_msg;
extern char *path_chng_failed_msg, *no_chng_dir_msg, *SPACING_str, *SPACING_msg;
extern char *pwd_cmd_str, *pwd_err_msg;

extern char *help_file_str;
extern char *no_file_string, *no_chng_no_save, *changes_made_title;
extern char *save_then_delete, *dont_delete, *delete_anyway_msg;
extern char *edit_cmd_str, *edit_err_msg, *restricted_msg, *rae_no_file_msg;
extern char *more_above_str, *more_below_str, *recover_name_prompt;
extern char *no_file_name_string, *recover_menu_title, *other_recover_file_str;
extern char *info_win_height_prompt, *info_win_height_err;
extern char *info_win_height_cmd_str, *info_win_height_msg_str;
extern char *conf_not_saved_msg;
extern char *conf_dump_err_msg;
extern char *conf_dump_success_msg;
extern char *info_help_msg;
extern char *unix_text_msg, *dos_text_msg;
extern char *text_cmd, *binary_cmd;
extern char *text_msg, *binary_msg, *dos_msg, *unix_msg;
extern char *file_being_edited_msg;
extern char *file_modified_msg;
extern char *DIFF_str;
extern char *ee_mode_str;
extern char *journal_str;
extern char *journal_err_str;

extern char *copyright_notice;
extern char *version_string;

extern char *main_menu_strings[10];
extern char *ee_mode_main_menu_strings[10];

/*
 |	Declare addresses for routines referenced in menus below.
 */

/* --- */

#if defined(__STDC__) || defined(__cplusplus)
#define P_(s) s
#else
#define P_(s) ()
#endif

int main P_((int argc, char *argv[]));
void get_input P_((WINDOW *win_ptr));
void status_display P_((void));
char *xalloc P_((int size));
char *resiz_line P_((int factor, struct text *rline, int rpos));
void dummy P_((int foo));
void insert P_((int character));
int scanline P_((struct text *m_line, int pos));
void tab_insert P_((void));
void unset_tab P_((char *string));
void tab_set P_((char *string));
int tabshift P_((int temp_int));
int out_char P_((WINDOW *window, int character, int abscolumn, int row, int offset));
void draw_line P_((int vertical, int horiz, char *ptr, int t_pos, struct text *dr_l));
void insert_line P_((int disp));
struct text *txtalloc P_((void));
struct files *name_alloc P_((void));
struct bufr *buf_alloc P_((void));
char *next_word P_((char *string));
int scan P_((char *line, int offset, int column));
char *get_string P_((char *prompt, int advance));
int len_char P_((int character, int column));
void ascii P_((void));
void print_buffer P_((void));
int compare P_((char *string1, char *string2, int sensitive));
void goto_line P_((char *cmd_str));
void make_win P_((void));
void no_windows P_((void));
void midscreen P_((int line, int count));
void get_options P_((int numargs, char *arguments[]));
void show_pwd P_((void));
char *get_full_path P_((char *path, char *orig_path));
char *ae_basename P_((char *name));
char *ae_dirname P_((char *path));
char *buff_name_generator P_((void));
int open_for_edit P_((char *string));
void recover_op P_((void));
int check_fp P_((void));
void get_file P_((char *file_name));
void get_line P_((int length, char *in_string, int *append));
void finish P_((char *string));
int delete_all_buffers P_((void));
void quit P_((char *string));
void abort_edit P_((int foo));
int write_file P_((char *file_name));
void sh_command P_((char *string));
void redraw P_((void));
void repaint_screen P_((void));
void copy_str P_((char *str1, char *str2));
void echo_string P_((char *string));
void ae_init P_((void));
char *is_in_string P_((char *string, char *substring));
char *resolve_name P_((char *name));
int file_write_success P_((void));

int menu_op P_((struct menu_entries menu_list[]));
void paint_menu P_((struct menu_entries menu_list[], int max_width, int max_height, int list_size, int top_offset, WINDOW *menu_win, int off_start, int vert_size));
void shell_op P_((void));
void leave_op P_((void));
void spell_op P_((void));
void ispell_op P_((void));
void modes_op P_((void));
void search_op P_((void));
int file_op P_((int arg));
void info_op P_((void));
int macro_assign P_((char *keys[], char *macro_string));
void get_key_assgn P_((void));
void paint_information P_((void));
void paint_info_win P_((void));
int unique_test P_((char *string, char *list[]));
void command_prompt P_((void));
void tab_resize P_((void));
void command P_((char *cmd_str));
void init_keys P_((void));
void parse P_((char *string));
int restrict_mode P_((void));
void dump_aee_conf P_((void));

void del_char P_((int save_flag));
void undel_char P_((void));
char *del_string P_((int length));
void undel_string P_((char *string, int length));
void del_word P_((int save_flag));
void undel_word P_((void));
void Clear_line P_((int save_flag));
void del_line P_((int save_flag));
void undel_line P_((void));
void last_deleted P_((int flag, int length, char *string));
void undel_last P_((void));
void delete_text P_((void));
void undel_init P_((void));
int delete P_((int disp));

int Blank_Line P_((struct text *test_line));
void Format P_((void));
int first_word_len P_((struct text *test_line));
void Auto_Format P_((void));

void help P_((void));
void outfile P_((void));
void ask P_((void));

void write_journal P_((struct bufr *buffer, struct text *line));
void update_journal_entry P_((struct bufr *buffer, struct text *line));
void remove_journ_line P_((struct bufr *buffer, struct text *line));
void journ_info_init P_((struct bufr *buffer, struct text *line));
void read_journal_entry P_((struct bufr *buffer, struct text *line));
int recover_from_journal P_((struct bufr *buffer, char *file_name));
void unlock_journal_fd P_((void));
struct journal_db * read_journal_db P_((void));
int create_dir P_((char *name));
void journal_name P_((struct bufr *buffer, char *file_name));
void open_journal_for_write P_((struct bufr *buffer));
void remove_journal_file P_((struct bufr *buffer));

void control P_((void));
void function_key P_((void));
void gold_func P_((void));
void keyboard P_((void));
void def_key P_((char *string));

void strings_init P_((void));

void copy P_((void));
void paste P_((void));
void unmark_text P_((void));
void cut P_((void));
void cut_up P_((void));
void cut_down P_((void));
void cut_line P_((void));
void fast_left P_((void));
void fast_right P_((void));
void fast_line P_((char *direct));
int slct P_((int flag));
void slct_dlt P_((void));
void slct_right P_((void));
void slct_left P_((void));
void slct_line P_((char *direct));

void bottom P_((void));
void top P_((void));
void nextline P_((void));
void prevline P_((void));
void left P_((int disp));
int right P_((int disp));
void find_pos P_((void));
void up P_((void));
void down P_((void));
void adv_word P_((void));
void prev_word P_((void));
void move_rel P_((char *direction, int lines));
void eol P_((void));
void bol P_((void));
void adv_line P_((void));
void next_page P_((void));
void prev_page P_((void));

int search P_((int move_cursor, struct text *start_line, int offset, char *pointer, int s_str_off, int srch_short, int disp));
int upper P_((int value));
int search_prompt P_((int flag));
void replace P_((void));
int repl_prompt P_((int flag));
void match P_((void));

void new_screen P_((void));
struct bufr *add_buf P_((char *ident));
void chng_buf P_((char *name));
int del_buf P_((void));
void redo_win P_((void));
void resize_check P_((void));
void set_up_term P_((void));
void diff_file P_((void));

#ifdef XAE
void event_init P_((void));
void event_manage P_((void));
void move_to_xy P_((int x, int y));
void eopl P_((void));
void cut_text P_((void));
void raise_window P_((void));
void set_window_name P_((char *name));
#endif /* XAE */

#undef P_


