/*
	+---------------------------------------------------------------+
	|								|
	|	Hugh F. Mahon						|
	|	begun Nov. 22, 1985					|
	|								|
	|	ae (another editor)					|
	+---------------------------------------------------------------+

	$Header: /home/hugh/sources/aee/RCS/aee.c,v 1.82 2010/07/18 21:52:26 hugh Exp hugh $

*/

/*
 |	An easy to use, simple screen oriented editor.
 |
 |	THIS MATERIAL IS PROVIDED "AS IS".  THERE ARE
 |	NO WARRANTIES OF ANY KIND WITH REGARD TO THIS
 |	MATERIAL, INCLUDING, BUT NOT LIMITED TO, THE
 |	IMPLIED WARRANTIES OF MERCHANTABILITY AND
 |	FITNESS FOR A PARTICULAR PURPOSE.  Neither
 |	Hewlett-Packard nor Hugh Mahon shall be liable
 |	for errors contained herein, nor for
 |	incidental or consequential damages in
 |	connection with the furnishing, performance or
 |	use of this material.  Neither Hewlett-Packard
 |	nor Hugh Mahon assumes any responsibility for
 |	the use or reliability of this software or
 |	documentation.  This software and
 |	documentation is totally UNSUPPORTED.  There
 |	is no support contract available.  Hewlett-
 |	Packard has done NO Quality Assurance on ANY
 |	of the program or documentation.  You may find
 |	the quality of the materials inferior to
 |	supported materials.
 |
 |	This software is not a product of Hewlett-Packard, Co., or any 
 |	other company.  No support is implied or offered with this software.
 |	You've got the source, and you're on your own.
 |
 |	This software may be distributed under the terms of Larry Wall's 
 |	Artistic license, a copy of which is included in this distribution. 
 |
 |	This notice must be included with this software and any derivatives.
 |
 |	This software and documentation contains
 |	proprietary information which is protected by
 |	copyright.  All rights are reserved.
 |
 */
char *copyright_notice = "Copyright (c) 1986 - 1988, 1991 - 2002, 2009, 2010 Hugh Mahon.";

char *long_notice[] = {
	"This software and documentation contains", 
	"proprietary information which is protected by", 
	"copyright.  All rights are reserved."
	};

#include "aee_version.h"

char *version_string = "@(#) aee (another easy editor) version "  AEE_VERSION  " " DATE_STRING " $Revision: 1.82 $";

#include "aee.h"

int eightbit;			/* eight bit character flag		*/

struct stat buf;

#ifndef NO_CATGETS
nl_catd catalog;
#endif /* NO_CATGETS */

extern int errno;

struct text *first_line;	/* first line of current buffer		*/
struct text *dlt_line;		/* structure for info on deleted line	*/
struct text *curr_line;		/* current line cursor is on		*/
struct text *fpste_line;	/* first line of select buffer		*/
struct text *cpste_line;	/* current paste/select line		*/
struct text *paste_buff;	/* first line of paste buffer		*/
struct text *pste_tmp;		/* temporary paste pointer		*/
struct text *tmp_line;		/* temporary line pointer		*/
struct text *srch_line;		/* temporary pointer for search routine */

struct files *top_of_stack;

struct bufr *first_buff;	/* first buffer in list			*/
struct bufr *curr_buff;		/* pointer to current buffer		*/
struct bufr *t_buff;		/* temporary pointer for buffers	*/

struct tab_stops *tabs;

struct del_buffs *undel_first;
struct del_buffs *undel_current;

WINDOW *com_win;		/* command window			*/
WINDOW *help_win;		/* window for help facility		*/
int windows;			/* flag for windows or no windows	*/
WINDOW *info_win;

int lines_moved;		/* number of lines moved in search	*/
int d_wrd_len;			/* length of deleted word		*/
int value;			/* temporary integer value		*/
int tmp_pos;			/* temporary position			*/
int tmp_vert;			/* temporary vertical position		*/
int tmp_horz;			/* temporary horizontal position	*/
int repl_length;		/* length of string to replace		*/
int pst_pos;			/* position in cpste_line->line		*/
int gold_count;			/* number of times to execute pressed key */
int fildes;			/* file descriptor			*/
int get_fd;			/* get file descriptor			*/
int write_fd;			/* write file descriptor		*/
int num_of_bufs;		/* the number of buffers that exist	*/
int temp_stdin;			/* temporary storage for stdin		*/
int temp_stdout;		/* temp storage for stdout descriptor	*/
int temp_stderr;		/* temp storage for stderr descriptor	*/
int pipe_out[2];		/* pipe file desc for output		*/
int pipe_in[2];			/* pipe file descriptors for input	*/
int line_wrap;			/* should line extend forever?		*/
int right_margin;		/* right margin	(if observ_margins = TRUE) */
int left_margin;		/* left margin				*/
int info_type;
int info_win_height = INFO_WIN_HEIGHT_DEF;
int local_LINES = 0;		/* copy of LINES, to detect when win resizes */
int local_COLS = 0;		/* copy of COLS, to detect when win resizes  */
int bit_bucket;			/* file descriptor to /dev/null		*/
int tab_spacing = 8;		/* spacing for tabs			*/

/*
 |	boolean flags
 */

char mark_text;			/* flag to indicate if MARK is active	*/
char journ_on;			/* flag for journaling			*/
char input_file;		/* indicate to read input file		*/
char recv_file;			/* indicate reading a file		*/
char edit;			/* continue executing while true	*/
char gold;			/* 'gold' function key pressed		*/
char recover;			/* set true if recover operation to occur */
char case_sen;			/* case sensitive search flag		*/
char change;			/* indicate changes have been made to file*/
char go_on;			/* loop control for help function	*/
char clr_cmd_line;		/* flag set to clear command line	*/
char literal;		/* is search string to be interpreted literally? */
char forward;	/* if TRUE, search after cursor, else search before cursor */
char echo_flag;			/* allow/disallow echo in init file	*/
char status_line;		/* place status info in the bottom line	*/
char overstrike;		/* overstrike / insert mode flag	*/
char indent;			/* auto-indent mode flag		*/
char expand;			/* expand tabs flag			*/
char shell_fork;
char out_pipe = FALSE;		/* flag that info is piped out		*/
char in_pipe = FALSE;		/* flag that info is piped in		*/
char observ_margins = FALSE;	/* should margins be observed		*/
char right_justify = FALSE;	/* justify right margin when formatting */
char free_d_line = FALSE;	/* free d_line (already freed from del list) */
char free_d_word = FALSE;	/* free d_word (already freed from del list) */
char info_window = TRUE;	/* is info window displayed?		*/
char auto_format = FALSE;	/* automatically format paragraph	*/
char formatted = FALSE;		/* has paragraph been formatted?	*/
char window_resize = FALSE;	/* flag to indicate a resize has occurred */
char change_dir_allowed = TRUE;	/* flag to indicate if chdir is allowed */
char restricted = FALSE;	/* flag to indicate restricted mode	*/
char com_win_initialized = FALSE;
char text_only = TRUE;		/* editor is to treat file being read as 
				   text only (not a binary file)	*/
char ee_mode_menu = FALSE;	/* make main menu look more like ee's	*/

long mask;			/* mask for sigblock			*/
#ifdef XAE
int win_width;			/* width and height of edit session window */
int win_height;
extern char *xsrch_string;
extern char *xold_string;
extern char *xnew_string;
extern Window testwin;
#endif

char in_string[513];		/* input buffer string			*/
char *pst_line;			/* start of paste line			*/
char *pst_pnt;			/* current position within paste line	*/
char *pste1;			/* temporary pointers for cut/paste ops */
char *pste2;
char *pste3;
char *old_string;		/* old string to be replaced		*/
char *u_old_string;		/* upper case verson of old_string	*/
char *new_string;		/* new string to insert instead of old	*/
char *srch_str;			/* pointer for search string		*/
char *u_srch_str;		/* pointer to non-case sensitive search */
char *in_file_name;		/* name of input file and path		*/
char *short_file_name;		/* name of input file only		*/
char *journ;			/* name of journal file			*/
char *journal_dir = "";		/* name of journal directory		*/
char *out_file;			/* name of output file if not input file*/
char *tmp_file;			/* temporary file name			*/
char d_char;			/* deleted character			*/
char *d_word;			/* deleted word				*/
char *d_line;			/* deleted line				*/
char *term_type;		/* type of terminal being used		*/
char *help_line;			/* input line for help facility		*/
char *sline;			/* temporary line pointer for help fac	*/
char *subject;			/* subject user wants to learn about	*/
char *start_of_string;		/* start of search string		*/
char match_char;		/* character found by search for match	*/
char in_buff_name[128];		/* input buffer name			*/
char out_buff_name[128];	/* output buffer name			*/
char *start_at_line;		/* move to this line at start of session*/
char *print_command = "lp";	/* string to hold command for printing file */
char nohighlight = FALSE;

char *ctr[32];			/* control key strings			*/
char ctr_changed[32];		/* control key strings changed flags	*/
char *g_ctr[32];		/* gold control key strings		*/
char g_ctr_changed[32];	/* gold control key strings changed 	*/
char *f[64];			/* function key strings			*/
char f_changed[64];		/* function key strings			*/
char *g_f[64];			/* gold function key strings		*/
char g_f_changed[64];		/* gold function key strings		*/
char *ctrl_table[] = { 
	"^@", "^A", "^B", "^C", "^D", "^E", "^F", "^G", 
	"^H", "\t", "^J", "^K", "^L", "^M", "^N", "^O", "^P", "^Q", "^R", 
	"^S", "^T", "^U", "^V", "^W", "^X", "^Y", "^Z", "^[", "^\\", "^]", 
	"^^", "^_", "^?"
	};
char *keypads[5];		/* keypad keys from terminfo		*/
char *g_keypads[5];		/* keypad keys from terminfo		*/

int in;				/* input character			*/

FILE *fp;			/* file pointer for input		*/
FILE *outp;			/* pointer for output file		*/
FILE *temp_fp;			/* temporary file pointer		*/
FILE *get_fp;			/* read file pointer			*/
FILE *write_fp;			/* write file pointer			*/

char info_data[MAX_HELP_LINES][MAX_HELP_COLS];

struct edit_keys assignment[] = {
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 }, 
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 },
{ "", "",  0, 0, 0, 0 }
};


/*
 |	allocate space here for the strings that will be in the menu
 */

struct menu_entries modes_menu[] = {
	{"", NULL, NULL, NULL, NULL, 0}, 	/* 0 title		*/
	{"", NULL, NULL, NULL, NULL, -1}, 	/* 1      		*/
	{"", NULL, NULL, NULL, NULL, -1}, 	/* 2      		*/
	{"", NULL, NULL, NULL, NULL, -1}, 	/* 3      		*/
	{"", NULL, NULL, NULL, NULL, -1}, 	/* 4      		*/
	{"", NULL, NULL, NULL, NULL, -1}, 	/* 5      		*/
	{"", NULL, NULL, NULL, NULL, -1}, 	/* 6      		*/
	{"", NULL, NULL, NULL, NULL, -1}, 	/* 7      		*/
	{"", NULL, NULL, NULL, NULL, -1}, 	/* 8      		*/
	{"", NULL, NULL, NULL, NULL, -1}, 	/* 9      		*/
	{"", NULL, NULL, NULL, NULL, -1}, 	/* 10     		*/
	{"", NULL, NULL, NULL, NULL, -1}, 	/* 11     		*/
	{"", NULL, NULL, NULL, NULL, -1}, 	/* 12     		*/
	{"", NULL, NULL, NULL, NULL, -1}, 	/* 13     		*/
	{"", NULL, NULL, NULL, NULL, -1}, 	/* 14     		*/
	{"", NULL, NULL, NULL, NULL, -1}, 	/* 15 text/binary mode  */
	{"", NULL, NULL, NULL, NULL, -1}, 	/* 16 dos/ux text file  */
	{"", NULL, NULL, NULL, NULL, -1}, 	/* 17 save configuration */
	{NULL, NULL, NULL, NULL, NULL, -1}
	};

int modes_initialized = FALSE;

char *mode_strings[NUM_MODES_ITEMS];

struct menu_entries config_dump_menu[] = {
	{"", NULL, NULL, NULL, NULL, 0}, 
	{"", NULL, NULL, NULL, NULL, -1},
	{"", NULL, NULL, NULL, NULL, -1},
	{NULL, NULL, NULL, NULL, NULL, -1}
	};

struct menu_entries leave_menu[] = {
	{"", NULL, NULL, NULL, NULL, 0}, 
	{"", NULL, NULL, NULL, NULL, -1}, 
	{"", NULL, NULL, NULL, NULL, -1}, 
	{NULL, NULL, NULL, NULL, NULL, -1}
	};

struct menu_entries leave_menu_2[] = {
	{"", NULL, NULL, NULL, NULL, 0}, 
	{"", NULL, NULL, NULL, NULL, -1}, 
	{"", NULL, NULL, NULL, NULL, -1}, 
	{NULL, NULL, NULL, NULL, NULL, -1}
	};

struct menu_entries file_menu[] = {
	{"", NULL, NULL, NULL, NULL, 0},
	{"", NULL, NULL, file_op, NULL, READ_FILE},
	{"", NULL, NULL, file_op, NULL, WRITE_FILE},
	{"", NULL, NULL, file_op, NULL, SAVE_FILE},
	{"", NULL, NULL, NULL, print_buffer, -1},
	{"", NULL, NULL, NULL, recover_op, -1},
	{NULL, NULL, NULL, NULL, NULL, -1}
	};

struct menu_entries search_menu[] = {
	{"", NULL, NULL, NULL, NULL, 0}, 
	{"", NULL, NULL, search_prompt, NULL, TRUE},
	{"", NULL, NULL, NULL, search_op, -1},
	{"", NULL, NULL, repl_prompt, NULL, TRUE},
	{"", NULL, NULL, NULL, replace, -1},
	{NULL, NULL, NULL, NULL, NULL, -1}
	};

struct menu_entries spell_menu[] = {
	{"", NULL, NULL, NULL, NULL, 0}, 
	{"", NULL, NULL, NULL, spell_op, -1},
	{"", NULL, NULL, NULL, ispell_op, -1},
	{NULL, NULL, NULL, NULL, NULL, -1}
	};

struct menu_entries misc_menu[] = {
	{"", NULL, NULL, NULL, NULL, 0}, 
	{"", NULL, NULL, NULL, Format, -1},
	{"", NULL, NULL, NULL, shell_op, -1}, 
	{"", menu_op, spell_menu, NULL, NULL, -1}, 
	{NULL, NULL, NULL, NULL, NULL, -1}
	};

struct menu_entries edit_menu[] = {
	{"", NULL, NULL, NULL, NULL, 0}, 
	{"", NULL, NULL, slct, NULL, Mark}, 
	{"", NULL, NULL, NULL, copy, -1}, 
	{"", NULL, NULL, NULL, cut, -1}, 
	{"", NULL, NULL, NULL, paste, -1}, 
	{NULL, NULL, NULL, NULL, NULL, -1}
	};

char *main_menu_strings[10];
char *ee_mode_main_menu_strings[10];

struct menu_entries main_menu[] = {
	{"", NULL, NULL, NULL, NULL, 0}, 
	{"", NULL, NULL, NULL, leave_op, -1}, 
	{"", NULL, NULL, NULL, help, -1}, 
	{"", menu_op, edit_menu, NULL, NULL, -1}, 
	{"", menu_op, file_menu, NULL, NULL, -1}, 
	{"", NULL, NULL, NULL, redraw, -1}, 
	{"", NULL, NULL, NULL, modes_op, -1}, 
	{"", menu_op, search_menu, NULL, NULL, -1}, 
	{"", menu_op, misc_menu, NULL, NULL, -1}, 
	{NULL, NULL, NULL, NULL, NULL, -1}
	};

struct menu_entries del_buff_menu[] = {
	{"", NULL, NULL, NULL, NULL, MENU_WARN}, 
	{"", NULL, NULL, file_op, NULL, SAVE_FILE}, 
	{"", NULL, NULL, NULL, NULL, -1}, 
	{"", NULL, NULL, NULL, NULL, -1}, 
	{NULL, NULL, NULL, NULL, NULL, -1}
	};

struct menu_entries rae_err_menu[] = {
	{"", NULL, NULL, NULL, NULL, MENU_WARN}, 
	{"", NULL, NULL, NULL, NULL, -1}, 
	{NULL, NULL, NULL, NULL, NULL, -1}
	};


char *commands[72];
char *init_strings[41];

/*
 |	memory debugging macros
 */

#ifdef DEBUG
FILE *error_fp;
#endif /* DEBUG */

/*
 |	declaration for localizable strings
 */

char *ae_help_file, *SL_line_str, *SL_col_str, *SL_lit_str, *SL_nolit_str;
char *SL_fwd_str, *SL_rev_str, *SL_over_str, *SL_insrt_str, *SL_indent_str;
char *SL_noindnt_str, *SL_marg_str, *SL_nomarg_str, *SL_mark_str, *ascii_code_str;
char *left_err_msg, *right_err_msg, *help_err_msg, *prompt_for_more, *topic_prompt;
char *topic_err_msg, *continue_prompt, *printing_msg, *EXPAND_str, *NOEXPAND_str;
char *NOJUSTIFY_str, *JUSTIFY_str, *EXIT_str, *QUIT_str, *AUTOFORMAT_str;
char *NOAUTOFORMAT_str, *INFO_str, *NOINFO_str, *TABS_str, *UNTABS_str;
char *WRITE_str, *READ_str, *SAVE_str, *LITERAL_str, *NOLITERAL_str;
char *STATUS_str, *NOSTATUS_str, *MARGINS_str, *NOMARGINS_str, *INDENT_str;
char *NOINDENT_str, *OVERSTRIKE_str, *NOOVERSTRIKE_str, *LEFTMARGIN_str, *RIGHTMARGIN_str;
char *LINE_str, *FILE_str, *COPYRIGHT_str, *CHARACTER_str, *REDRAW_str;
char *RESEQUENCE_str, *AUTHOR_str, *VERSION_str, *CASE_str, *NOCASE_str;
char *EIGHT_str, *NOEIGHT_str, *WINDOWS_str, *NOWINDOWS_str, *DEFINE_str;
char *SHOW_str, *HELP_str, *PRINT_str, *BUFFER_str, *DELETE_str;
char *GOLD_str, *tab_msg, *file_write_prompt_str, *file_read_prompt_str, *left_mrg_err_msg;
char *right_mrg_err_msg, *left_mrg_setting, *right_mrg_setting, *line_num_str, *lines_from_top;
char *total_lines_str, *current_file_str, *char_str, *key_def_msg, *unkn_syntax_msg;
char *current_buff_msg, *unkn_cmd_msg, *usage_str, *read_only_msg, *no_rcvr_fil_msg;
char *cant_opn_rcvr_fil_msg, *fin_read_file_msg, *rcvr_op_comp_msg, *file_is_dir_msg, *path_not_a_dir_msg;
char *access_not_allowed_msg, *new_file_msg, *cant_open_msg, *bad_rcvr_msg, *reading_file_msg;
char *file_read_lines_msg, *other_buffs_exist_msg, *changes_made_prompt, *no_write_access_msg, *file_exists_prompt;
char *cant_creat_fil_msg, *writing_msg, *file_written_msg, *searching_msg, *fwd_srch_msg;
char *rev_srch_msg, *str_str, *not_fnd_str, *search_prompt_str, *mark_not_actv_str;
char *mark_active_str, *mark_alrdy_actv_str, *no_buff_named_str, *press_ret_to_cont_str, *fn_GOLD_str;
char *fn_DL_str, *fn_DC_str, *fn_CL_str, *fn_NP_str, *fn_PP_str;
char *fn_NB_str, *fn_PB_str, *fn_UDL_str, *fn_UDC_str, *fn_DW_str;
char *fn_UDW_str, *fn_UND_str, *fn_EOL_str, *fn_BOL_str, *fn_BOT_str;
char *fn_EOT_str, *fn_FORMAT_str, *fn_MARGINS_str, *fn_NOMARGINS_str, *fn_IL_str;
char *fn_PRP_str, *fn_RP_str, *fn_MC_str, *fn_PSRCH_str, *fn_SRCH_str;
char *fn_AL_str, *fn_AW_str, *fn_AC_str, *fn_PW_str, *fn_CUT_str;
char *fn_FWD_str, *fn_REV_str, *fn_MARK_str, *fn_UNMARK_str, *fn_APPEND_str;
char *fn_PREFIX_str, *fn_COPY_str, *fn_CMD_str, *fn_PST_str, *fn_RD_str;
char *fn_UP_str, *fn_DOWN_str, *fn_LEFT_str, *fn_RIGHT_str, *fn_BCK_str;
char *fn_CR_str, *fn_EXPAND_str, *fn_NOEXPAND_str, *fn_EXIT_str, *fn_QUIT_str;
char *fn_LITERAL_str, *fn_NOLITERAL_str, *fn_STATUS_str, *fn_NOSTATUS_str, *fn_INDENT_str;
char *fn_NOINDENT_str, *fn_OVERSTRIKE_str, *fn_NOOVERSTRIKE_str, *fn_CASE_str, *fn_NOCASE_str;
char *fn_WINDOWS_str, *fn_NOWINDOWS_str, *fn_HELP_str, *fn_MENU_str, *fwd_mode_str;
char *rev_mode_str, *ECHO_str, *cmd_prompt, *PRINTCOMMAND_str, *replace_action_prompt;
char *replace_prompt_str, *replace_r_char, *replace_skip_char, *replace_all_char, *quit_char;
char *yes_char, *no_char, *cant_find_match_str, *fmting_par_msg, *cancel_string;
char *menu_too_lrg_msg, *shell_cmd_prompt, *shell_echo_msg, *spell_in_prog_msg, *left_marg_prompt;
char *left_mrgn_err_msg, *right_mrgn_prompt, *right_mrgn_err_msg, *cant_redef_msg, *buff_msg;
char *cant_chng_while_mark_msg, *too_many_parms_msg, *buff_is_main_msg, *cant_del_while_mark, *main_buffer_name;
char *cant_del_buf_msg, *HIGHLIGHT_str, *NOHIGHLIGHT_str;
char *non_unique_cmd_msg;
char *ON, *OFF;
char *save_file_name_prompt, *file_not_saved_msg, *cant_reopen_journ;
char *write_err_msg, *jrnl_dir, *CD_str ;
char *no_dir_entered_msg,  *path_not_dir_msg, *path_not_permitted_msg;
char *path_chng_failed_msg, *no_chng_dir_msg, *SPACING_str, *SPACING_msg;
char *help_file_str;
char *pwd_cmd_str, *pwd_err_msg;
char *no_file_string, *no_chng_no_save, *changes_made_title;
char *save_then_delete, *dont_delete, *delete_anyway_msg;
char *edit_cmd_str, *edit_err_msg, *restricted_msg, *rae_no_file_msg;
char *more_above_str, *more_below_str, *recover_name_prompt;
char *no_file_name_string, *recover_menu_title, *other_recover_file_str;
char *info_win_height_prompt, *info_win_height_err, *info_win_height_cmd_str;
char *info_win_height_msg_str;
char *conf_not_saved_msg;	/* message if unable to save configuration */
char *conf_dump_err_msg;
char *conf_dump_success_msg;
char *info_help_msg;
char *unix_text_msg, *dos_text_msg;
char *text_cmd, *binary_cmd;
char *text_msg, *binary_msg, *dos_msg, *unix_msg;
char *DIFF_str;
char *ee_mode_str;
char *journal_str, *journal_err_str;

int 
main(argc, argv)		/* beginning of main program		*/
int argc;
char *argv[];
{
#ifdef DEBUG
	error_fp = fopen("/tmp/ae.mem_debug", "w");
#endif /* DEBUG */

#ifdef DIAG_DBG
		if (!strcmp(argv[argc-1], "-DEBUG"))
		{
/*
 |	The following code will start an xterm which will in turn execute 
 |	a debugger and "adopt" the process, so it can be debugged.
 |
 |	To compile to allow debug, use the command:
 |
 |		CFLAGS='-DDIAG_DBG -g' make
 |
 */

			int child_id;
			char command_line[256];

			child_id = getpid();
			if (!fork())
			{
				sprintf(command_line, 
	      			"xterm -geom =80x40 -fg wheat -bg DarkSlateGrey -n %s -e gdb %s %d", 
					"hello",  argv[0], child_id);
				printf("command_line=%s\n", command_line);
				execl("/bin/sh", "sh", "-c", command_line, NULL);
				fprintf(stderr, "could not exec new window\n");
				exit(1);
			}

/*
 |	When in debugger, set child_id to zero (p child_id=0) to continue 
 |	executing the software.
 */

			while (child_id != 0)
				;
			argc--;
		}
#endif

	eightbit = FALSE;
#ifndef DIAG_DBG
	mask = 077777777 & (~020000) & (~02);
	for (value = 1; value < 24; value++)
	{
		if ((value != SIGCHLD) && (value != SIGALRM) && (value != SIGSEGV)
#ifdef XAE
		   && (value != SIGTSTP)
#endif /* XAE */

		)
			signal(value, SIG_IGN);
		else
			signal(value, SIG_DFL);
	}
#endif	/* ifndef DIAG	*/
	signal(SIGINT, abort_edit);
#ifdef XAE
	win_width = 0;
	win_height = 0;
#endif



	start_at_line = NULL;
	d_word = NULL;
	d_wrd_len = 0;
	d_line = xalloc(1);
	*d_line = '\0';
	tabs = (struct tab_stops *) xalloc(sizeof( struct tab_stops));
	tabs->next_stop = NULL;
	tabs->column = 0;
	dlt_line = txtalloc();
	dlt_line->line = d_line;
	dlt_line->line_length = 1;
	paste_buff = NULL;
	status_line = FALSE;
	srch_str = u_srch_str = NULL;
	u_old_string = new_string = old_string = NULL;

	/*
	 |	initialize literal strings
	 */

	strings_init();

	first_buff = curr_buff = buf_alloc();
	first_buff->name = main_buffer_name;
	first_buff->first_line = txtalloc();
	curr_buff->curr_line = curr_buff->first_line;
	first_buff->next_buff = NULL;
	first_buff->footer = NULL;
	first_buff->main_buffer = TRUE;
	first_buff->edit_buffer = TRUE;
	curr_buff->curr_line->line = curr_buff->pointer = xalloc(10);
	curr_buff->curr_line->line_length = 1;
	curr_buff->curr_line->max_length = 10;
	curr_buff->curr_line->prev_line = NULL;
	curr_buff->curr_line->next_line = NULL;
	curr_buff->curr_line->line_number = 1;
	curr_buff->curr_line->vert_len = 1;
	curr_buff->num_of_lines = 1;
	curr_buff->absolute_lin = 1;
	num_of_bufs = 1;
	curr_buff->position = 1;
	curr_buff->abs_pos = curr_buff->scr_pos = 0;
	curr_buff->scr_vert = 0;
	curr_buff->scr_horz = 0;
	bit_bucket = open("/dev/null", O_WRONLY);
	forward = windows = edit = TRUE;
	observ_margins = literal = FALSE;
	expand = indent = overstrike = FALSE;
	clr_cmd_line = change = mark_text = gold = case_sen = FALSE;
	in_pipe = out_pipe = FALSE;
	shell_fork = TRUE;
	left_margin = 0;
	right_margin = 0;
	gold_count = 0;
	info_type = CONTROL_KEYS;
	top_of_stack = NULL;
	echo_flag = TRUE;
	journ_on = TRUE;
	input_file = FALSE;
	recv_file = FALSE;
	recover = FALSE;
	undel_init();
	init_keys();
	ae_init();
	get_options(argc, argv);
	set_up_term();		/* initialize terminal		*/
	get_key_assgn();
	com_win = newwin(1, COLS, LINES-1, 0);
	com_win_initialized = TRUE;
	curr_buff->last_col = COLS - 1;
	curr_buff->last_line = LINES - 2;
	first_buff->lines = LINES - 1;
	curr_buff->window_top = 0;
	if (info_window)
	{
		info_win = newwin(info_win_height, COLS, 0, 0);
		paint_info_win();
		curr_buff->window_top = info_win_height;
		first_buff->win = newwin(LINES-info_win_height-1, COLS, info_win_height, 0);
		first_buff->lines = LINES - info_win_height - 1;
		curr_buff->last_line = first_buff->lines - 1;
	}
	else
		first_buff->win = newwin(LINES-1, COLS, 0, 0);
	keypad(curr_buff->win, TRUE);
	nodelay(curr_buff->win, FALSE);
	idlok(curr_buff->win, TRUE);

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
		abort_edit(0);
	}


	if (top_of_stack != NULL)
		tmp_file = top_of_stack->name;
	else
		tmp_file = "";
	input_file = TRUE;
	recv_file = TRUE;

	if (top_of_stack != NULL)
		top_of_stack = top_of_stack->next_name;

	value = check_fp();

	input_file = FALSE;
	recv_file = FALSE;

	if (right_margin == 0)
		right_margin = COLS - 1;

	wmove(curr_buff->win, curr_buff->last_line, 0);
	wstandout(curr_buff->win);
	wprintw(curr_buff->win, "aee, %s", copyright_notice);
	wstandend(curr_buff->win);
	wmove(curr_buff->win, curr_buff->scr_vert, curr_buff->scr_horz);
	wrefresh(curr_buff->win);
	midscreen(curr_buff->scr_vert, curr_buff->position);

	while (edit) 
	{
		get_input(curr_buff->win);
		if (clr_cmd_line)
		{
			clr_cmd_line = FALSE;
			werase(com_win);
			wrefresh(com_win);
		}
		keyboard();
		status_display();
		wmove(curr_buff->win, curr_buff->scr_vert, curr_buff->scr_horz);
		wrefresh(curr_buff->win);
	}
	exit(0);
}

void 
get_input(win_ptr)
WINDOW *win_ptr;
{
	int value;
#ifndef XAE
	static int counter = 0;
	pid_t parent_pid;
#endif /* XAE */

	resize_check();

#ifdef XAE
	event_manage();
#endif /* XAE */

	in = wgetch(win_ptr);
	value = alarm(0);
	signal(SIGALRM, dummy);

	if (in == EOF)		/* somehow lost contact with input	*/
		abort_edit(0);

#ifndef XAE
	/*
	 |	The above check used to work to detect if the parent 
	 |	process died, but now it seems we need a more 
	 |	sophisticated check.
	 |
	 |	Note that this check is only required for the terminal 
	 |	version, and may be undesireable for xae, so is ifdef'd 
	 |	accordingly.
	 */
	if (counter > 50)
	{
		parent_pid = getppid();
		if (parent_pid == 1)
			abort_edit(0);
		else
			counter = 0;
	}
	else
		counter++;
#endif /* !XAE */
}

void 
status_display()	/* display status line			*/
{
	if ((status_line) && (!clr_cmd_line))
	{
		werase(com_win);
		wmove(com_win, 0, 0);

		if (curr_buff->edit_buffer)
		{
			if (curr_buff->file_name != NULL)
				wprintw(com_win, "%c%s  ", CHNG_SYMBOL(curr_buff->changed), curr_buff->file_name);
			else
				wprintw(com_win, "%c[%s]  ", CHNG_SYMBOL(curr_buff->changed), no_file_string);
		}
		else
		{
			if (curr_buff->name != NULL)
				wprintw(com_win, "%c->%s  ", CHNG_SYMBOL(curr_buff->changed), curr_buff->name);
			else
				wprintw(com_win, "%c[%s]  ", CHNG_SYMBOL(curr_buff->changed), no_file_string);
		}

		wmove(com_win, 0, 16);
		wclrtoeol(com_win);
		wmove(com_win, 0, 18);
		wprintw(com_win, SL_line_str, curr_buff->curr_line->line_number);
		wmove(com_win, 0, 30);
		wprintw(com_win, SL_col_str, curr_buff->scr_pos);
		wmove(com_win, 0, 40);
		if (literal)
			wprintw(com_win, SL_lit_str);
		else
			wprintw(com_win, SL_nolit_str);
		if (forward)
			wprintw(com_win, SL_fwd_str);
		else
			wprintw(com_win, SL_rev_str);
		if (overstrike)
			wprintw(com_win, SL_over_str);
		else
			wprintw(com_win, SL_insrt_str);
		if (indent)
			wprintw(com_win, SL_indent_str);
		else
			wprintw(com_win, SL_noindnt_str);
		if (observ_margins)
			wprintw(com_win, SL_marg_str);
		else
			wprintw(com_win, SL_nomarg_str);
		if (mark_text)
		{
			wstandout(com_win);
			wprintw(com_win, SL_mark_str);
			wstandend(com_win);
		}
		else if (text_only)
		{
			wprintw(com_win, "%s", 
			  curr_buff->dos_file ? dos_text_msg : unix_text_msg);
		}

		if (curr_buff->footer != NULL)
		{
			wmove(curr_buff->footer, 0,0);
			wstandout(curr_buff->footer);
			wprintw(curr_buff->footer, "%c", CHNG_SYMBOL(curr_buff->changed) );
			wstandend(curr_buff->footer);
			wrefresh(curr_buff->footer);
		}
		wrefresh(com_win);
	}
}

char *
#ifndef DEBUG
xalloc(size)	/* allocate memory, report if errors	*/
int size;	/* size of memory to allocate		*/
#else
dxalloc(size, line_num)
int size;
int line_num;
#endif /* DEBUG */
{
	int counter;
	char *memory;

#ifdef DEBUG
	fprintf(error_fp, "m s=%d, l#=%d\n", size, line_num);
	fflush(error_fp);
#endif /*	DEBUG */

	if (size == 0)
		size++;
	if ((memory = malloc(size)) == NULL)
	{
		counter = 0;
		werase(com_win);
		wmove(com_win, 0,0);
		wstandout(com_win);
		wprintw(com_win, "memory allocation errors, please wait");
		wstandend(com_win);
		wrefresh(com_win);
		clr_cmd_line = TRUE;
		while (((memory = malloc(size)) == NULL) && (counter < 10000))
			counter++;
		if (memory == NULL)
			abort_edit(0);
		werase(com_win);
	}
	return(memory);
}

char *
#ifndef DEBUG
resiz_line(factor, rline, rpos)	/* resize the line to length + factor*/
int factor;		/* resize factor				*/
struct text *rline;	/* pointer to line structure			*/
int rpos;		/* position in line				*/
#else
dresiz_line(factor, rline, rpos, line_num) 
int factor;		/* resize factor				*/
struct text *rline;	/* pointer to line structure			*/
int rpos;		/* position in line				*/
int line_num;
#endif
{
	char *rpoint;
	int resiz_var;
 
#ifdef DEBUG
	fprintf(error_fp, "r s=%d, l#=%d\n", factor, line_num);
	fflush(error_fp);
#endif /*	DEBUG */

	rline->max_length += factor;
	while ((rpoint = realloc(rline->line, rline->max_length )) == NULL)
		;
	rline->line = rpoint;
	for (resiz_var = 1 ; (resiz_var < rpos) ; resiz_var++)
		rpoint++;
	return(rpoint);
}

void 
dummy(foo)
int foo;
{/*this is a dummy function to eliminate any unresolved signals		*/
}

void 
insert(character)		/* insert character into line		*/
int character;			/* new character			*/
{
	int counter;
	char *temp;		/* temporary pointer			*/
	char *temp2;		/* temporary pointer			*/
	int temp_overstrike;

	if ((observ_margins) && (curr_buff->scr_pos < left_margin))
	{
		temp_overstrike = overstrike;
		overstrike = FALSE;
		counter = left_margin;
		left_margin = 0;
		while (curr_buff->scr_pos < counter)
			insert(' ');
		left_margin = counter;
		overstrike = temp_overstrike;
	}
	change = TRUE;
	curr_buff->curr_line->changed = TRUE;
	curr_buff->changed = TRUE;
	if ((!overstrike) || (curr_buff->position == curr_buff->curr_line->line_length))
	{
		if ((curr_buff->curr_line->max_length - curr_buff->curr_line->line_length) < 5)
			curr_buff->pointer = resiz_line(10, curr_buff->curr_line, curr_buff->position);
		curr_buff->curr_line->line_length++;
		temp = curr_buff->pointer;
		counter = curr_buff->position;
		while (counter < curr_buff->curr_line->line_length)	/* find end of line */
		{
			counter++;
			temp++;
		}
		temp++;			/* increase length of line by one	*/
		while (curr_buff->pointer < temp)
		{
			temp2=temp - 1;
			*temp= *temp2;	/* shift characters over by one		*/
			temp--;
		}
	}
	*curr_buff->pointer = character;	/* insert new character			*/
	if (curr_buff->position != (curr_buff->curr_line->line_length - 1))
		counter = (scanline(curr_buff->curr_line, curr_buff->curr_line->line_length) / COLS) + 1;
	else
		counter = ((curr_buff->scr_pos + len_char(*curr_buff->pointer, curr_buff->scr_pos)) / COLS) + 1;
	if (counter > curr_buff->curr_line->vert_len)
	{
		curr_buff->curr_line->vert_len = counter;
		if (curr_buff->scr_vert < curr_buff->last_line)
		{
			wmove(curr_buff->win, (curr_buff->scr_vert+1), 0);
			winsertln(curr_buff->win);
		}
	}
	wmove(curr_buff->win, curr_buff->scr_vert, curr_buff->scr_horz);
	wclrtoeol(curr_buff->win);
	if ((mark_text) && (((pst_pos == cpste_line->line_length) && (cpste_line->next_line == NULL)) || (overstrike)))
		right(TRUE);
	else
	{
		counter = curr_buff->scr_horz + len_char(character, curr_buff->scr_pos);
		if (counter > curr_buff->last_col)
		{
			counter = curr_buff->scr_vert + 1;
			if (counter > curr_buff->last_line)
			{
				wmove(curr_buff->win, 0, 0);
				wdeleteln(curr_buff->win);
				wmove(curr_buff->win, curr_buff->scr_vert-1, curr_buff->scr_horz);
			}
		}
		if ((character < 32) || (character > 126))
			curr_buff->scr_horz += out_char(curr_buff->win, character, curr_buff->scr_pos, curr_buff->scr_vert, 0);
		else
		{
			waddch(curr_buff->win, character);
			curr_buff->scr_horz++;
		}
		curr_buff->abs_pos = curr_buff->scr_pos += len_char(*curr_buff->pointer, curr_buff->scr_pos);
		curr_buff->pointer++;
		curr_buff->position++;
		if (curr_buff->scr_horz > curr_buff->last_col)
		{
			curr_buff->scr_horz = curr_buff->scr_pos % COLS;
			if (curr_buff->scr_vert < curr_buff->last_line)
				curr_buff->scr_vert++;
		}
	}
	draw_line(curr_buff->scr_vert, curr_buff->scr_pos, curr_buff->pointer, curr_buff->position, curr_buff->curr_line);
	if ((observ_margins) && (right_margin < curr_buff->scr_pos))
	{
		counter = curr_buff->position;
		while (curr_buff->scr_pos > right_margin)
			prev_word();
		if (curr_buff->scr_pos == 0)
		{
			while (curr_buff->position < counter)
				right(TRUE);
		}
		else
		{
			counter -= curr_buff->position;
			insert_line(TRUE);
			for (value = 0; value < counter; value++)
				right(TRUE);
		}
	}

	if ((auto_format) && ((character == '\t') || ((character == ' ') && (!formatted))))
		Auto_Format();
	else if ((character != ' ') && (character != '\t'))
		formatted = FALSE;
}

int 
scanline(m_line, pos)	/* find the proper horizontal position for the current position in the line */
struct text *m_line;
int pos;
{
	int temp;
	int count;
	char *ptr;

	ptr = m_line->line;
	count = 1;
	temp = 0;
	while (count < pos)
	{
		if ((*ptr >= 0) && (*ptr <= 8))
			temp += 2;
		else if (*ptr == 9)
			temp += tabshift(temp);
		else if ((*ptr >= 10) && (*ptr <= 31))
			temp += 2;
		else if ((*ptr >= 32) && (*ptr < 127))
			temp++;
		else if (*ptr == 127)
			temp += 2;
		else if (!eightbit)
			temp += 5;
		else
			temp++;
		ptr++;
		count++;
	}
	return(temp);
}

void 
tab_insert()	/* insert a tab or spaces according to value of 'expand' */
{
	int temp;
	int counter;

	if (expand)
	{
		counter = tabshift(curr_buff->scr_pos);
		for (temp = 0; temp < counter; temp++)
			insert(' ');
		if (auto_format)
			Auto_Format();
	}
	else
		insert('\t');
}

void 
unset_tab(string)	/* remove tab setting			*/
char *string;
{
	char *pointer;
	int column;
	int loop_on;
	struct tab_stops *temp_stack_point;
	struct tab_stops *stack_point;

	pointer = string;
	while (*pointer != '\0')
	{
		while (((*pointer < '0') || (*pointer > '9')) && (*pointer != '\0'))
			pointer++;
		column = atoi(pointer);
		temp_stack_point = tabs;
		if (column > 0)
		{
			loop_on = TRUE;
			while ((temp_stack_point->next_stop != NULL) && (loop_on))
			{
				if (temp_stack_point->next_stop->column != column)
					temp_stack_point = temp_stack_point->next_stop;
				else
					loop_on = FALSE;
			}
			if (temp_stack_point->next_stop != NULL)
			{
				stack_point = temp_stack_point->next_stop;
				temp_stack_point->next_stop = stack_point->next_stop;
				free(stack_point);
			}
		}
		while (((*pointer >= '0') && (*pointer <= '9')) && (*pointer != '\0'))
			pointer++;
	}
}

void 
tab_set(string)	/* get tab settings from the string (columns)	*/
char *string;
{
	char *pointer;
	int column;
	int loop_on;
	struct tab_stops *temp_stack_point;
	struct tab_stops *stack_point;

	pointer = string;
	while (*pointer != '\0')
	{
		while (((*pointer < '0') || (*pointer > '9')) && (*pointer != '\0'))
			pointer++;
		column = atoi(pointer);
		temp_stack_point = tabs;
		if (column > 0)
		{
			loop_on = TRUE;
			while ((temp_stack_point->next_stop != NULL) && (loop_on))
			{
				if (temp_stack_point->next_stop->column <= column)
					temp_stack_point = temp_stack_point->next_stop;
				else
					loop_on = FALSE;
			}
			if ((column > temp_stack_point->column) && (column != temp_stack_point->column))
			{
				stack_point = (struct tab_stops *) xalloc(sizeof( struct tab_stops));
				stack_point->column = column;
				stack_point->next_stop = temp_stack_point->next_stop;
				temp_stack_point->next_stop = stack_point;
			}
		}
		while (((*pointer >= '0') && (*pointer <= '9')) && (*pointer != '\0'))
			pointer++;
	}
}

int 
tabshift(temp_int)	/* give the number of spaces to shift to the next 
			   tab stop					*/
int temp_int;	/* current column	*/
{
	int leftover;
	struct tab_stops *stack_point;

	stack_point = tabs;
	while ((stack_point != NULL) && (stack_point->column <= temp_int))
		stack_point = stack_point->next_stop;
	if (stack_point != NULL)
	{
		leftover = (stack_point->column - temp_int);
		return(leftover);
	}
	leftover = ((temp_int + 1) % tab_spacing);
	if (leftover == 0)
		return (1);
	else
		return ((tab_spacing + 1) - leftover);
}

int 
out_char(window, character, abscolumn, row, offset)	/* output non-printing character */
WINDOW *window;
char character;
int abscolumn;
int row;
int offset;
{
	int i1, i2, iter;
	int column;
	int space;
	char *string;
	char *tmp;

	column = abscolumn % COLS;
	space = FALSE;
	if ((character >= 0) && (character < 32))
	{
		if (character == '\t')
		{
			i1 = tabshift(abscolumn);
			for (i2=1; (i2 <= i1)&&((((i2+column)<=curr_buff->last_col)&&(row==curr_buff->last_line))||(row<curr_buff->last_line)); i2++)
				waddch(window, ' ');
			return(i1);
		}
		else
			string = ctrl_table[(int)character];
	}
	else if ((character < 0) || (character >= 127))
	{
		if (character == 127)
			string = "^?";
		else if (!eightbit)
		{
			space = TRUE;
			tmp = string = xalloc(6);
			*tmp = '<';
			tmp++;
			if ((i1=character) < 0)
				i1 = 256 + character;
			i2 = i1 / 100;
			i1 -=i2 * 100;
			*tmp = i2 + '0';
			tmp++;
			i2 = i1 / 10;
			i1 -= i2 * 10;
			*tmp = i2 + '0';
			tmp++;
			i2 = i1;
			*tmp = i2 + '0';
			tmp++;
			*tmp = '>';
			tmp++;
			*tmp = '\0';
		}
		else
		{
			waddch(window, (unsigned char) character);
			return(1);
		}
	}
	tmp = string;
	for (iter=0; (iter < offset) && (tmp != NULL) && (character != '\t'); iter++)
		tmp++;
	for (iter = column;
	     (((iter <= curr_buff->last_col) && (row == curr_buff->last_line)) || 
	     (row < curr_buff->last_line)) && (*tmp != '\0'); iter++, tmp++)
		waddch(window, *tmp);
	iter -= column;
	if (space)
		free(string);
	return(iter);
}

void 
draw_line(vertical, horiz, ptr, t_pos, dr_l)	/* redraw line from current position */
int vertical;
int horiz;
char *ptr;
int t_pos;
struct text *dr_l;
{
	int d;
	char *temp;
	int abs_column;
	int column;
	int row;
	int tp_pos;
	int posit;
	int highlight;
	int offset;

	highlight = FALSE;
	abs_column = horiz;
	column = abs_column % COLS;
	row = vertical;
	temp = ptr;
	d = 0;
	posit = t_pos;
	if (row < 0)
	{
		while (row < 0)
		{
			abs_column += d =  len_char(*temp, abs_column);
			column += d;
			if (column > curr_buff->last_col)
			{
				row++;
				if ((d > 1) && (row == 0))
				{
					d = d - (column - COLS);
					abs_column -= column - COLS;
				}
				else
				{
					posit++;
					temp++;
					d = 0;
				}
				column %= COLS;
			}
			else
			{
				posit++;
				temp++;
			}
		}
		column = 0;
	}
	wmove(curr_buff->win, row, column);
	wclrtoeol(curr_buff->win);
	if ((mark_text) && (pst_pos == 1) && (curr_buff->pointer == ptr) && 
			(cpste_line->line_length > 1))
	{
		highlight = TRUE;
		tp_pos = pst_pos;
	}
	else if (mark_text)
		wstandend(curr_buff->win);
	while ((posit < dr_l->line_length) && (((column <= curr_buff->last_col) && 
			(row == curr_buff->last_line)) || (row < curr_buff->last_line)))
	{
		if ((mark_text) && (pst_pos == 1) && (curr_buff->pointer == ptr) && (highlight))
		{
			if (tp_pos < cpste_line->line_length)
				wstandout(curr_buff->win);
			else
				wstandend(curr_buff->win);
			tp_pos++;
		}
		if ((*temp < 32) || (*temp > 126))
		{
			offset = out_char(curr_buff->win, *temp, abs_column, row, d);
			column += offset;
			abs_column += offset;
			d = 0;
		}
		else
		{
			abs_column++;
			column++;
			waddch(curr_buff->win, *temp);
		}
		if (column > curr_buff->last_col)
		{
			column = column % COLS;
			row++;
		}
		posit++;
		temp++;
	}
	if (((column < curr_buff->last_col) && (row == curr_buff->last_line)) || (row < curr_buff->last_line))
		wclrtoeol(curr_buff->win);
	wstandend(curr_buff->win);
	wmove(curr_buff->win, vertical, horiz);
}

void 
insert_line(disp)	/* insert new line into file and if disp=TRUE, 
			reorganize screen accordingly	*/
int disp;
{
	int temp_pos;
	int temp_pos2;
	int i;
	char *temp;
	char *extra;
	struct text *temp_nod;	/* temporary pointer to new line	*/
	int temp_overstrike;

	i = curr_buff->curr_line->vert_len;
	wmove(curr_buff->win, curr_buff->scr_vert, curr_buff->scr_horz);
	wclrtoeol(curr_buff->win);
	change = TRUE;
	temp_nod= txtalloc();
	temp_nod->line = extra = xalloc(6 + curr_buff->curr_line->line_length - curr_buff->position);
	temp_nod->line_length = 1 + curr_buff->curr_line->line_length - curr_buff->position;
	temp_nod->vert_len = 1;
	temp_nod->max_length = temp_nod->line_length + 5;
	if (curr_buff->position == 1)
		temp_nod->line_number = curr_buff->curr_line->line_number;
	else
		temp_nod->line_number = curr_buff->curr_line->line_number + 1;
	temp_nod->next_line = curr_buff->curr_line->next_line;
	if (temp_nod->next_line != NULL)
		temp_nod->next_line->prev_line = temp_nod;
	temp_nod->prev_line = curr_buff->curr_line;
	curr_buff->curr_line->next_line = temp_nod;
	temp_pos2 = curr_buff->position;
	temp = curr_buff->pointer;
	if (temp_pos2 != curr_buff->curr_line->line_length)
	{
		temp_pos = 1;
		while (temp_pos2 < curr_buff->curr_line->line_length)
		{
			temp_pos++;
			temp_pos2++;
			*extra= *temp;
			extra++;
			temp++;
		}
		temp = curr_buff->pointer;
		*temp = '\0';
		temp = resiz_line((1 - temp_nod->line_length), curr_buff->curr_line, curr_buff->position);
		curr_buff->curr_line->line_length = curr_buff->position;
		curr_buff->curr_line->vert_len = (scanline(curr_buff->curr_line, curr_buff->curr_line->line_length) / COLS) + 1;
		curr_buff->curr_line->changed = TRUE;
	}
	curr_buff->curr_line->line_length = curr_buff->position;
	curr_buff->curr_line = temp_nod;
	curr_buff->curr_line->vert_len = (scanline(curr_buff->curr_line, curr_buff->curr_line->line_length) / COLS) + 1;
	*extra = '\0';
	curr_buff->position = 1;
	curr_buff->pointer = curr_buff->curr_line->line;
	curr_buff->num_of_lines++;
	curr_buff->absolute_lin++;
	curr_buff->curr_line->changed = TRUE;
	curr_buff->changed = TRUE;

	if (curr_buff->journalling)
	{
		write_journal(curr_buff, curr_buff->curr_line);
		if (curr_buff->curr_line->prev_line->changed)
			write_journal(curr_buff, curr_buff->curr_line->prev_line);
	}

	if (disp)
	{
		i = (curr_buff->curr_line->vert_len + curr_buff->curr_line->prev_line->vert_len) - i;
		if (curr_buff->scr_vert < curr_buff->last_line)
		{
			curr_buff->scr_vert++;
			wclrtoeol(curr_buff->win);
			wmove(curr_buff->win, curr_buff->scr_vert, 0);
			if (i > 0)
				winsertln(curr_buff->win);
		}
		else 
		{
			wmove(curr_buff->win, 0, 0);
			wdeleteln(curr_buff->win);
			wmove(curr_buff->win, curr_buff->last_line, 0);
			wclrtobot(curr_buff->win);
		}
		curr_buff->abs_pos = curr_buff->scr_pos = curr_buff->scr_horz = 0;
		draw_line(curr_buff->scr_vert, curr_buff->scr_pos, curr_buff->pointer, curr_buff->position, curr_buff->curr_line);
	}
	curr_buff->abs_pos = curr_buff->scr_pos = curr_buff->scr_horz = 0;
	if ((mark_text) && (cpste_line->line_length == pst_pos))
	{
		i = mark_text;
		mark_text = FALSE;
		left(TRUE);
		mark_text = i;
		right(TRUE);
	}
	if (indent)
	{
		int temp_lm = left_margin;

		left_margin = 0;
		if (curr_buff->curr_line->prev_line != NULL)
		{
			temp = curr_buff->curr_line->prev_line->line;
			i = overstrike;
			overstrike = FALSE;
			while ((*temp == ' ') || (*temp == '\t'))
			{
				insert(*temp);
				temp++;
			}
			overstrike = i;
		}
		left_margin = temp_lm;
	}
	if ((observ_margins) && (left_margin != 0) && (curr_buff->scr_pos < left_margin))
	{
		temp_overstrike = overstrike;
		overstrike = FALSE;
		i = left_margin;
		left_margin = 0;
		while (curr_buff->scr_pos < i)
			insert(' ');
		left_margin = i;
		overstrike = temp_overstrike;
	}
}

struct text *txtalloc()		/* allocate space for line structure	*/
{
	struct text *txt_tmp;

	txt_tmp = (struct text *) xalloc(sizeof( struct text));
	txt_tmp->changed = FALSE;
	txt_tmp->file_info.info_location = NO_FURTHER_LINES;
	txt_tmp->file_info.prev_info = NO_FURTHER_LINES;
	txt_tmp->file_info.next_info = NO_FURTHER_LINES;
	return(txt_tmp);
}

struct files *name_alloc()	/* allocate space for file name list node */
{
	struct files *temp_name;

	temp_name = (struct files *) xalloc(sizeof( struct files));
	return(temp_name);
}

struct bufr *buf_alloc()	/* allocate space for buffers		*/
{
	struct bufr *temp_buf;

	temp_buf = (struct bufr *) xalloc(sizeof(struct bufr));
	temp_buf->journal_file = NULL;
	temp_buf->file_name = NULL;
	temp_buf->full_name = "";
	temp_buf->orig_dir = NULL;
	temp_buf->journalling = FALSE;
	temp_buf->changed = FALSE;
	temp_buf->main_buffer = FALSE;
	temp_buf->edit_buffer = FALSE;
	temp_buf->dos_file = FALSE;
	temp_buf->journ_fd = NULL;
	return (temp_buf);
}

char *next_word(string)		/* move to next word in string		*/
char *string;
{
	while ((*string != '\0') && ((*string != 32) && (*string != 9)))
		string++;
	while ((*string != '\0') && ((*string == 32) || (*string == 9)))
		string++;
	return(string);
}

int 
scan(line, offset, column)	/* determine horizontal position for get_string	*/
char *line;
int offset;
int column;
{
	char *stemp;
	int i;
	int j;

	stemp = line;
	i = 0;
	j = column;
	while (i < offset)
	{
		i++;
		j += len_char(*stemp, j);
		stemp++;
	}
	return(j);
}

char *get_string(prompt, advance)	/* read string from input on command line */
char *prompt;		/* string containing user prompt message	*/
int advance;		/* if true, skip leading spaces and tabs	*/
{
	char *string;
	char *tmp_string;
	char *nam_str;
	char *g_point;
	int tmp_int;
	int g_horz, g_position, g_pos;
	int esc_flag;

	g_point = tmp_string = xalloc(512);
	wmove(com_win,0,0);
	wclrtoeol(com_win);
	waddstr(com_win, prompt);
	wrefresh(com_win);
	nam_str = tmp_string;
	clr_cmd_line = TRUE;
	g_horz = g_position = scan(prompt, strlen(prompt), 0);
	g_pos = 0;
	do
	{
		esc_flag = FALSE;
		get_input(com_win);
		if (((in == 8) || (in == 127) || (in == KEY_BACKSPACE)) && (g_pos > 0))
		{
			tmp_int = g_horz;
			g_pos--;
			g_horz = scan(g_point, g_pos, g_position);
				tmp_int = tmp_int - g_horz;
			for (; 0 < tmp_int; tmp_int--)
			{
				if ((g_horz+tmp_int) < (curr_buff->last_col - 1))
				{
					waddch(com_win, '\010');
					waddch(com_win, ' ');
					waddch(com_win, '\010');
				}
			}
			nam_str--;
		}
		else if ((in != 8) && (in != 127) && (in != '\n') && (in != '\r') && (in < 256))
		{
			if (in == '\026')	/* control-v, accept next character verbatim	*/
			{			/* allows entry of ^m, ^j, and ^h	*/
				esc_flag = TRUE;
				get_input(com_win);
			}
			*nam_str = in;
			g_pos++;
			if (((in < 32) || (in > 126)) && (g_horz < (curr_buff->last_col - 1)))
				g_horz += out_char(com_win, in, g_horz, curr_buff->last_line, 0);
			else
			{
				g_horz++;
				if (g_horz < (curr_buff->last_col - 1))
					waddch(com_win, in);
			}
			nam_str++;
		}
		wrefresh(com_win);
		if (esc_flag)
			in = '\0';
	} while ((in != '\n') && (in != '\r'));
	*nam_str = '\0';
	nam_str = tmp_string;
	if (((*nam_str == ' ') || (*nam_str == 9)) && (advance))
		nam_str = next_word(nam_str);
	string = xalloc(strlen(nam_str) + 1);
	copy_str(nam_str, string);
	free(tmp_string);
	wrefresh(com_win);
	return(string);
}

int 
len_char(character, column)	/* return the length of the character	*/
char character;
int column;	/* the column must be known to provide spacing for tabs	*/
{
	int length;

	if (character == '\t')
		length = tabshift(column);
	else if ((character >= 0) && (character < 32))
		length = 2;
	else if ((character >= 32) && (character <= 126))
		length = 1;
	else if (character == 127)
		length = 2;
	else if (((character > 126) || (character < 0)) && (!eightbit))
		length = 5;
	else
		length = 1;

	return(length);
}

void 
ascii()		/* get ascii code from user and insert into file	*/
{
	int i;
	char *string, *t;

	i = 0;
	t = string = get_string(ascii_code_str, TRUE);
	if (*t != '\0')
	{
		while ((*string >= '0') && (*string <= '9'))
		{
			i= i * 10 + (*string - '0');
			string++;
		}
		in = i;
		wmove(curr_buff->win, curr_buff->scr_vert, curr_buff->scr_horz);
		insert(in);
	}
	free(t);
}

void 
print_buffer()
{
	char buffer[256];

	sprintf(buffer, ">!%s", print_command);
	wmove(com_win, 0, 0);
	wclrtoeol(com_win);
	wprintw(com_win, printing_msg, curr_buff->name, print_command);
	wrefresh(com_win);
	command(buffer);
}

int 
compare(string1, string2, sensitive)	/* compare two strings	*/
char *string1;
char *string2;
int sensitive;
{
	char *strng1;
	char *strng2;
	int tmp;
	int equal;

	strng1 = string1;
	strng2 = string2;
	tmp = 0;
	if ((strng1 == NULL) || (strng2 == NULL) || (*strng1 == '\0') || (*strng2 == '\0'))
		return(FALSE);
	equal = TRUE;
	while (equal)
	{
		if (sensitive)
		{
			if (*strng1 != *strng2)
				equal = FALSE;
		}
		else
		{
			if (toupper(*strng1) != toupper(*strng2))
				equal = FALSE;
		}
		strng1++;
		strng2++;
		if ((*strng1 == '\0') || (*strng2 == '\0') || (*strng1 == ' ') || (*strng2 == ' '))
			break;
		tmp++;
	}
	return(equal);
}

void 
goto_line(cmd_str)	/* goto first line with the given number	*/
char *cmd_str;
{
	int number;
	int i;
	char *ptr;
	char *direction;
	struct text *t_line;

	ptr = cmd_str;
	i= 0;
	while ((*ptr >='0') && (*ptr <= '9'))
	{
		i= i * 10 + (*ptr - '0');
		ptr++;
	}
	number = i;
	i = 0;
	t_line = curr_buff->curr_line;
	while ((t_line->line_number > number) && (t_line->prev_line != NULL))
	{
		i++;
		t_line = t_line->prev_line;
		direction = "u";
	}
	if (i == 0)
	{
		while ((t_line->line_number < number) && (t_line->next_line != NULL))
		{
			i++;
			direction = "d";
			t_line = t_line->next_line;
		}
	}
	if (i != 0)
		move_rel(direction, i);
	wmove(com_win,0,0);
	wclrtoeol(com_win);
	wprintw(com_win, line_num_str, curr_buff->curr_line->line_number);
	wrefresh(com_win);
	clr_cmd_line = TRUE;
	wmove(curr_buff->win, curr_buff->scr_vert, curr_buff->scr_horz);
}

void 
make_win()		/* allow windows		*/
{
	if (((!windows) && (!mark_text)) && (num_of_bufs <= ((LINES - 1)/2)))
	{
		windows = TRUE;
		if (num_of_bufs > 1)
		{
			werase(first_buff->win);
			wrefresh(first_buff->win);
			delwin(first_buff->win);
			t_buff = first_buff;
			while (t_buff != NULL)
			{
				t_buff->win = NULL;
				t_buff = t_buff->next_buff;
			}
			redo_win();
			new_screen();
			curr_buff->win = curr_buff->win;
		}
	}
}

void 
no_windows()		/* disallow windows		*/
{
	int temp;

	if ((windows) && (!mark_text))
	{
		windows = FALSE;
		if (num_of_bufs > 1)
		{
			t_buff = first_buff;
			werase(t_buff->win);
			wrefresh(t_buff->win);
			delwin(t_buff->win);
			werase(t_buff->footer);
			wrefresh(t_buff->footer);
			delwin(t_buff->footer);
			t_buff->footer = NULL;
			if (info_window)
			{
				temp = info_win_height;
				if (info_win == NULL)
				{
					info_win = newwin(info_win_height, COLS, 0, 0);
					paint_info_win();
				}
			}
			else
				temp = 0;
			t_buff->win = newwin(LINES - temp - 1, COLS, temp, 0);
			t_buff->window_top = temp;
			keypad(t_buff->win, TRUE);
			idlok(t_buff->win, TRUE);
			t_buff->lines = LINES - temp - 1;
			t_buff->last_col = COLS - 1;
			t_buff->last_line = t_buff->lines - 1;
			t_buff = t_buff->next_buff;
			while (t_buff != NULL)
			{
				werase(t_buff->win);
				wrefresh(t_buff->win);
				werase(t_buff->footer);
				wrefresh(t_buff->footer);
				delwin(t_buff->footer);
				delwin(t_buff->win);
				t_buff->footer = NULL;
				t_buff->win = first_buff->win;
				t_buff->window_top = first_buff->window_top;
				t_buff->lines = first_buff->lines;
				t_buff->last_line = first_buff->last_line;
				t_buff->last_col = first_buff->last_col;
				t_buff = t_buff->next_buff;
			}
			new_screen();
		}
	}
}

void 
midscreen(line, count)	/* repaint window with current line at the specified line	*/
int line;
int count;
{
	struct text *mid_line;
	struct text *a_line;
	struct text *b_line;
	char *tp;		/* temporary pointer		*/
	int abs_column;	/* the number of columns from start of line	*/
	int ti;			/* temporary integer		*/
	int ti1;		/* another temporary integer	*/
	int tppos;		/* temporary paste position	*/
	int m;
	int i;

	mid_line = curr_buff->curr_line;

	/*
	 |	find the first line visible on the screen (from the top)
	 */

	i = (curr_buff->scr_pos / COLS);
	while ((i < line) && (curr_buff->curr_line->prev_line != NULL) && (i < curr_buff->last_line))
	{
		curr_buff->curr_line = curr_buff->curr_line->prev_line;
		i += curr_buff->curr_line->vert_len;
	}

	/*
	 |	set up temporary pointers to store the current information
	 */

	tp = curr_buff->curr_line->line;
	tppos = 1;
	ti1 = ti = 0;
	curr_buff->scr_vert = curr_buff->scr_horz = 0;
	wmove(curr_buff->win, 0, 0);
	a_line = curr_buff->curr_line;
	if (i > line)
		ti = line - i;
	else
		ti = 0;

	/*
	 |	erase the window and paint the information
	 */

	werase(curr_buff->win);
	while ((a_line != NULL) && (ti <= curr_buff->last_line))
	{
		draw_line(ti, 0, tp, 1, a_line);
		ti += a_line->vert_len;
		a_line = a_line->next_line;
		if (a_line != NULL)
			tp = a_line->line;
	}
	if (i > line)
		curr_buff->scr_vert = line;
	else
		curr_buff->scr_vert = i;
	curr_buff->curr_line = mid_line;

	/*
	 |	Now highlight areas that have been marked.
	 */

	if (mark_text)		/* paint selected text	*/
	{
		b_line = cpste_line;
		a_line = curr_buff->curr_line;
		tppos = pst_pos;

		/*
		 |	if the marked areas are before the cursor...
		 */

		if ((pst_pos == cpste_line->line_length) && (cpste_line->next_line == NULL))
		{

			/*
			 |	if there are multiple marked lines, find out 
			 |	how much vertical space they use
			 */

			if (cpste_line->prev_line != NULL)
			{
				i = scanline(curr_buff->curr_line, count) / COLS;
				while ((i <= curr_buff->scr_vert) && (cpste_line->prev_line!= NULL))
				{
					curr_buff->curr_line = curr_buff->curr_line->prev_line;
					cpste_line = cpste_line->prev_line;
					i += curr_buff->curr_line->vert_len;
				}
			}
			else
				i = 0;

			tp = curr_buff->curr_line->line;
			ti = 1;

			/*
			 |	set up to highlight a partial line
			 */

			if (curr_buff->curr_line->line_length != cpste_line->line_length)
			{
				if (i != 0)
				{
					ti = curr_buff->curr_line->line_length - 
						cpste_line->line_length;
					ti++;
					for (ti1=1; ti1<ti; ti1++)
						tp++;
				}
				else 
				{
					for (ti = 1; 
					      ti <= (curr_buff->position - 
						cpste_line->line_length); ti++)
					    tp++;
				}
			}

			abs_column = curr_buff->scr_horz = scanline(curr_buff->curr_line, ti);
			m = curr_buff->scr_horz / COLS;
			curr_buff->scr_horz = curr_buff->scr_horz % COLS;
			wmove(curr_buff->win, (m + curr_buff->scr_vert - 1), curr_buff->scr_horz);
			wstandout(curr_buff->win);
			curr_buff->pointer = tp;
			pst_pos = 1;

			if (i == 0)
				i = scanline(curr_buff->curr_line, curr_buff->position) / COLS;

			draw_line(m + curr_buff->scr_vert - i, abs_column, curr_buff->pointer, ti, 
						curr_buff->curr_line);

			for (ti = i - curr_buff->curr_line->vert_len; ti >= 0; 
					ti -= curr_buff->curr_line->vert_len)
			{
				cpste_line = cpste_line->next_line;
				curr_buff->curr_line = curr_buff->curr_line->next_line;
				pst_pos = 1;
				curr_buff->pointer = curr_buff->curr_line->line;
				draw_line(curr_buff->scr_vert-ti, 0, curr_buff->curr_line->line, 1, 
						curr_buff->curr_line);
			}
		}
		/*
		 |	or the marked text is after the cursor...
		 */
		else
		{
			pst_pos = 1;
			i = curr_buff->scr_vert;
			draw_line(curr_buff->scr_vert, curr_buff->scr_pos, curr_buff->pointer, curr_buff->position, curr_buff->curr_line);
			i += curr_buff->curr_line->vert_len - (scanline(curr_buff->curr_line, curr_buff->position) / COLS);
			if (curr_buff->curr_line->next_line != NULL)
			{
				curr_buff->curr_line = curr_buff->curr_line->next_line;
				cpste_line = cpste_line->next_line;
				curr_buff->pointer = curr_buff->curr_line->line;
				pst_pos = 1;
				while ((i <= curr_buff->last_line) && (cpste_line != NULL))
				{
					draw_line(i, 0, curr_buff->pointer, 1, curr_buff->curr_line);
					i += curr_buff->curr_line->vert_len;
					if (curr_buff->curr_line->next_line != NULL)
					{
						curr_buff->curr_line = curr_buff->curr_line->next_line;
						cpste_line = cpste_line->next_line;
						curr_buff->pointer = curr_buff->curr_line->line;
					}
				}
			}
		}
		pst_pos = tppos;
		curr_buff->curr_line = a_line;
		cpste_line = b_line;
	}
	curr_buff->pointer = curr_buff->curr_line->line;
	for (value = 1; value < count; value++)
		curr_buff->pointer ++;
	curr_buff->abs_pos = curr_buff->scr_pos = scanline(curr_buff->curr_line, count);
	curr_buff->scr_horz = curr_buff->scr_pos % COLS;
	wmove(curr_buff->win, curr_buff->scr_vert, curr_buff->scr_horz);
}

#ifndef xae11
void 
get_options(numargs, arguments)	/* get arguments from original command line	*/
int numargs;
char *arguments[];
{
	char *buff;
	char *extens;
	int count;
	struct files *temp_names;

	buff = ae_basename(arguments[0]);
	if (!strcmp(buff, "rae"))
	{
		restricted = TRUE;
		change_dir_allowed = FALSE;
	}

	count = 1;
	while (count < numargs)
	{
		buff = arguments[count];
		if (*buff == '-')	/* options	*/
		{
			++buff;
			if (*buff == 'j')	/* turn off recover file */
				journ_on = FALSE;
			else if (*buff == 'r')	/* recover from bad exit */
				recover = TRUE;
			else if (*buff == 'e')	/* turn off echo in init file */
				echo_flag = FALSE;
			else if (*buff == 'i')	/* turn off info window	*/
				info_window = FALSE;
			else if (!strcmp(arguments[count], "-text"))
			{
				text_only = TRUE;
			}
			else if (!strcmp(arguments[count], "-binary"))
			{
				text_only = FALSE;
			}
			else if (!strcmp(arguments[count], "-tab"))
						/* expand tabs to spaces */
				expand = TRUE;
			else if (*buff == 'n')	/* turn off highlighting on menus*/
				nohighlight = TRUE;
#ifdef XAE
			else if (*buff == 'h')	/* get height of window	*/
			{
				buff++;
				if (*buff == '\0')
				{
					count++;
					buff = arguments[count];
				}
				win_height = atoi(buff);
			}
			else if (*buff == 'w')	/* get width of window	*/
			{
				buff++;
				if (*buff == '\0')
				{
					count++;
					buff = arguments[count];
				}
				win_width = atoi(buff);
			}
#endif
		}
		else if (*buff == '+')
		{
			buff++;
			start_at_line = buff;
		}
		else	/* if the argument wasn't an option, must be a file name, right?	*/
		{
			if (top_of_stack == NULL)
				temp_names = top_of_stack = name_alloc();
			else
			{
				temp_names->next_name = name_alloc();
				temp_names = temp_names->next_name;
			}
			temp_names->next_name = NULL;
			extens = temp_names->name = xalloc(strlen(buff) + 1);
			while (*buff != '\0')
			{
				*extens = *buff;
				buff++;
				extens++;
			}
			*extens = '\0';
			input_file = TRUE;
			recv_file = TRUE;
		}
		count++;
	}
}
#endif /* xae11	*/

void 
finish(string)	/* prepare to EXIT edit session, write out file	*/
char *string;
{
	char *query;
	int leave;

	leave = TRUE;
	if (mark_text)
	{
		copy();
	}

	if ((num_of_bufs > 1) && (top_of_stack == NULL))
	{
		do
			query = get_string(other_buffs_exist_msg, TRUE);
		while ((toupper(*query) != toupper(*yes_char)) && 
			(toupper(*query) != toupper(*no_char)));
		if (toupper(*query) != toupper(*yes_char))
			leave = FALSE;
		else
		{
			leave = delete_all_buffers();
		}
	}

	if (leave)
	{
		if (!curr_buff->main_buffer)
			chng_buf(main_buffer_name);

		if (file_op(SAVE_FILE) == 0)
		{
			change = FALSE;
			quit(string);
		}
	}
}

int 
delete_all_buffers()
{
	int success = TRUE;

	t_buff = first_buff->next_buff;
	while ((t_buff != NULL) && (success))
	{
		chng_buf(t_buff->name);
		success = del_buf();
		t_buff = first_buff->next_buff;
	}
	return(success);
}

void 
quit(string)		/* leave editor, or if other files specified in invoking command line, edit next in sequence	*/
char *string;
{
	char *query;
	char *ans;
	int editable_buffs = 0;

	if (num_of_bufs > 1)
	{
		t_buff = first_buff;
		while (t_buff != NULL)
		{
			if (t_buff->edit_buffer)
				editable_buffs++;
			t_buff = t_buff->next_buff;
		}
	}

	if ((num_of_bufs > 1) && (top_of_stack == NULL))
	{
		do
			query = get_string(other_buffs_exist_msg, TRUE);
		while ((toupper(*query) != toupper(*yes_char)) && 
			(toupper(*query) != toupper(*no_char)));
		if (toupper(*query) != toupper(*yes_char))
			return;
		else
		{
			if (mark_text)
				copy();
			chng_buf(main_buffer_name);
		}

		if (!delete_all_buffers())
			return;
	}
	if (first_buff->changed) /* if changes have been made in the file */
	{
		ans = get_string(changes_made_prompt, TRUE);
       		wmove(com_win, 0, 0);
        	werase(com_win);
	        wrefresh(com_win);
		if (toupper(*ans) != toupper(*yes_char))
			return;
	}
	if (mark_text)
	{
		copy();
	}
	chng_buf(main_buffer_name);
	if (curr_buff->journalling)
			/* if recover file in use, delete it */
	{
		remove_journal_file(curr_buff);
	}
	while ((string != NULL) && (*string != '\0') && (*string != '!'))
		string++;
	if ((string != NULL) && (*string == '!'))
		top_of_stack = NULL;
	if (top_of_stack == NULL)	/* no other files to edit */
	{
		edit = FALSE;
		wrefresh(com_win);
		echo();
		nl();
		resetty();
		resetterm();
		noraw();
		endwin();
#ifndef XAE
		putchar('\n');
#endif	/* XAE */
		exit(0);
	}
	else	/* prepare to edit next file	*/
	{
		change = FALSE;
		curr_buff->curr_line = curr_buff->first_line;
		while (curr_buff->curr_line->next_line != NULL)
		{
			free(curr_buff->curr_line->line);
			curr_buff->curr_line = curr_buff->curr_line->next_line;
			free(curr_buff->curr_line->prev_line);
		}
		curr_buff->num_of_lines = 1;
		curr_buff->absolute_lin = 1;
		free(curr_buff->curr_line->line);
		free(curr_buff->curr_line);
		curr_buff->scr_vert = curr_buff->scr_horz = 0;
		curr_buff->curr_line = curr_buff->first_line = txtalloc();
		curr_buff->curr_line->line = curr_buff->pointer = xalloc(10);
		curr_buff->curr_line->line_length = 1;
		curr_buff->curr_line->max_length = 10;
		curr_buff->curr_line->prev_line = NULL;
		curr_buff->curr_line->next_line = NULL;
		curr_buff->curr_line->line_number = 1;
		curr_buff->position = 1;
		curr_buff->scr_pos = curr_buff->abs_pos = curr_buff->scr_horz = curr_buff->scr_vert = 0;
		free(curr_buff->journal_file);
		free(curr_buff->full_name);
		curr_buff->journal_file = NULL;
		curr_buff->file_name = NULL;
		curr_buff->full_name = "";
		werase(curr_buff->win);
		recv_file = TRUE;
		input_file = TRUE;
		tmp_file = top_of_stack->name;
		top_of_stack = top_of_stack->next_name;
		value = check_fp();
	}
}

void 
abort_edit(foo)	/* handle interrupt signal (break key) gracefully	*/
int foo;
{
	wrefresh(com_win);
	echo();
	nl();
	resetty();
	resetterm();
	noraw();
	endwin();
	putchar('\n');
	exit(0);
}

void 
sh_command(string)	/* execute shell command			*/
char *string;		/* string containing user command		*/
{
	char *temp_point;
	char *last_slash;
	char *path;		/* directory path to executable		*/
	int parent;		/* zero if child, child's pid if parent	*/
	struct text *line_holder;
	struct bufr *tmp;	/* pointer to buffer structure		*/

	if (restrict_mode())
	{
		return;
	}

	if (!(path = getenv("SHELL")))
		path = "/bin/sh";
	last_slash = temp_point = path;
	while (*temp_point != '\0')
	{
		if (*temp_point == '/')
			last_slash = ++temp_point;
		else
			temp_point++;
	}
	keypad(curr_buff->win, FALSE);
	echo();
	nl();
	noraw();
	resetty();
	if (in_pipe)
	{
		pipe(pipe_in);		/* create a pipe	*/
		parent = fork();
		if (!parent)		/* if the child		*/
		{
/*
 |  child process which will fork and exec shell command (if shell output is
 |  to be read by editor)
 */
			in_pipe = FALSE;
/*
 |  redirect stdout to pipe
 */
			temp_stdout = dup(1);
			close(1);
			dup(pipe_in[1]);
/*
 |  redirect stderr to pipe
 */
			temp_stderr = dup(2);
			close(2);
			dup(pipe_in[1]);
			close(pipe_in[1]);
		}
		else  /* if the parent	*/
		{
/*
 |  prepare editor to read from the pipe
 */
			signal(SIGCHLD, SIG_IGN);
			chng_buf(in_buff_name);
			line_holder = curr_buff->curr_line;
			tmp_vert = curr_buff->scr_vert;
			close(pipe_in[1]);
			get_fd = pipe_in[0];
			get_file("");
			close(pipe_in[0]);
			curr_buff->scr_vert = tmp_vert;
			curr_buff->scr_horz = curr_buff->scr_pos = curr_buff->abs_pos = 0;
			curr_buff->position = 1;
			curr_buff->curr_line = line_holder;
			curr_buff->pointer = curr_buff->curr_line->line;
			midscreen(curr_buff->scr_vert, 1);
			out_pipe = FALSE;
			signal(SIGCHLD, SIG_DFL);
/*
 |  since flag "in_pipe" is still TRUE, do not even wait for child to complete
 |  (the pipe is closed, no more output can be expected)
 */
		}
	}
	if (!in_pipe)
	{
		signal(SIGINT, SIG_IGN);
		if (out_pipe)
		{
			pipe(pipe_out);
		}
/*
 |  fork process which will exec command
 */
		parent = fork();   
		if (!parent)		/* if the child	*/
		{
			if (shell_fork)
				putchar('\n');
			if (out_pipe)
			{
/*
 |  prepare the child process (soon to exec a shell command) to read from the 
 |  pipe (which will be output from the editor's buffer)
 */
				close(0);
				dup(pipe_out[0]);
				close(pipe_out[0]);
				close(pipe_out[1]);
			}
			for (value = 1; value < 24; value++)
				signal(value, SIG_DFL);
			execl(path, last_slash, "-c", string, NULL);
			printf("could not exec %s\n", path);
			exit(-1);
		}
		else	/* if the parent	*/
		{
			if (out_pipe)
			{
/*
 |  output the contents of the buffer to the pipe (to be read by the 
 |  process forked and exec'd above as stdin)
 */
				close(pipe_out[0]);
				tmp = first_buff;
				while ((tmp != NULL) && (strcmp(out_buff_name, tmp->name)))
					tmp = tmp->next_buff;
				if (tmp != NULL)
				{
					line_holder = tmp->first_line;
					while (line_holder != NULL)
					{
						write(pipe_out[1], line_holder->line, (line_holder->line_length-1));
                                                if (tmp->dos_file)
                                                   write(pipe_out[1], "\r", 1);
						write(pipe_out[1], "\n", 1);
						line_holder = line_holder->next_line;
					}
				}
				else
					printf(no_buff_named_str, out_buff_name);
				close(pipe_out[1]);
				out_pipe = FALSE;
			}
			wait(&parent);
			signal(SIGINT, abort_edit);
/*
 |  if this process is actually the child of the editor, exit
 |  The editor forks a process.  If output must be sent to the command to be 
 |  exec'd another process is forked, and that process (the child's child) 
 |  will exec the command.  In this case, "shell_fork" will be FALSE.  If no 
 |  output is to be performed to the shell command, "shell_fork" will be TRUE.
 */
			if (!shell_fork)
				exit(0);
		}
	}
	if (shell_fork)
	{
		printf(press_ret_to_cont_str);
		fflush(stdout);
		while (((in = getchar()) != '\n') && (in != '\r'))
			;
#ifdef XAE
		raise_window();
#endif
	}
	fixterm();
	noecho();
	nonl();
	raw();
	keypad(curr_buff->win, TRUE);
	curr_buff->win = curr_buff->win;
	clearok(curr_buff->win, TRUE);
	redraw();
}

void 
redraw()		/* redraw screen			*/
{
	repaint_screen();
}

void 
repaint_screen()
{
	struct bufr *t_buff;	/* temporary buffer	*/

	for (t_buff = first_buff; t_buff != NULL; t_buff = t_buff->next_buff)
	{
		touchwin(t_buff->win);
		wnoutrefresh(t_buff->win);
		if (t_buff->footer != NULL)
		{
			touchwin(t_buff->footer);
			wnoutrefresh(t_buff->footer);
		}
	}
	if (info_window)
	{
		touchwin(info_win);
		wnoutrefresh(info_win);
	}
}

void 
copy_str(str1, str2)	/* copy string1 to string2			*/
char *str1, *str2;
{
	char *t1, *t2;

	t1 = str1;
	t2 = str2;
	while (*t1 != '\0')
	{
		*t2 = *t1;
		t2++;
		t1++;
	}
	*t2 = *t1;
}

void 
echo_string(string)	/* echo the given string	*/
char *string;
{
	char *temp;
	int Counter;

	if (echo_flag)
	{
		temp = string;
		while (*temp != '\0')
		{
			if (*temp == '\\')
			{
				temp++;
				if (*temp == 'n')
					putchar('\n');
				else if (*temp == 't')
					putchar('\t');
				else if (*temp == 'b')
					putchar('\b');
				else if (*temp == 'r')
					putchar('\r');
				else if (*temp == 'f')
					putchar('\f');
				else if ((*temp == 'e') || (*temp == 'E'))
					putchar('\033');		/* escape */
				else if (*temp == '\\')
					putchar('\\');
				else if (*temp == '\'')
					putchar('\'');
				else if ((*temp >= '0') && (*temp <= '9'))
				{
					Counter = 0;
					while ((*temp >= '0') && (*temp <= '9'))
					{
						Counter = (8 * Counter) + (*temp - '0');
						temp++;
					}
					putchar(Counter);
					temp--;
				}
				temp++;
			}
			else
			{
				putchar(*temp);
				temp++;
			}
		}
	}
	fflush(stdout);
}

char *init_name[6] = {
	"/usr/local/aee/init.ae", 
	"/usr/local/lib/init.ae", 
	NULL,                           /* to be ~/.init.ae */
	".init.ae",
	NULL,                           /* to be ~/.aeerc */
	".aeerc"
	};

void 
ae_init()	/* check for init file and read it if it exists	*/
{
	FILE *init_file;
	char *string;
	char *str1;
	char *str2;
	char *home;
	char *tmp;
	int c_int;
	int lines;
	int counter;

	home = xalloc(11);
	strcpy(home, "~/.init.ae");
	init_name[2] = resolve_name(home);
	free(home);

	home = xalloc(11);
	strcpy(home, "~/.aeerc");
	init_name[4] = resolve_name(home);
	free(home);
	
	

	string = xalloc(512);
	for (counter = 0; counter < 6; counter++)
	{
		lines = 1;
		if (!(access(init_name[counter], 4)))
		{
			init_file = fopen(init_name[counter], "r");
			while ((str2 = fgets(string, 512, init_file)) != NULL)
			{
				str1 = str2 = string;
				while (*str2 != '\n')
					str2++;
				*str2 = '\0';

				if (unique_test(string, init_strings) != 1)
					continue;

				if (compare(str1, DEFINE_str, FALSE))
				{
					str1 = next_word(str1);
					def_key(str1);
				}
				else if (compare(str1, WINDOWS_str,  FALSE))
					windows = TRUE;
				else if (compare(str1, NOWINDOWS_str,  FALSE))
					windows = FALSE;
				else if (compare(str1, CASE_str,  FALSE))
					case_sen = TRUE;
				else if (compare(str1, NOCASE_str,  FALSE))
					case_sen = FALSE;
				else if (compare(str1, TABS_str,  FALSE))
					tab_set(str1);
				else if (compare(str1, UNTABS_str,  FALSE))
					unset_tab(str1);
				else if (compare(str1, EXPAND_str,  FALSE))
					expand = TRUE;
				else if (compare(str1, SPACING_str, FALSE))
				{
					str1 = next_word(str1);
					if (*str1 != '\0')
						tab_spacing = atoi(str1);
				}
				else if (compare(str1, NOEXPAND_str,  FALSE))
					expand = FALSE;
				else if (compare(str1, NOJUSTIFY_str,  FALSE))
					right_justify = FALSE;
				else if (compare(str1, JUSTIFY_str,  FALSE))
					right_justify = TRUE;
				else if (compare(str1, LITERAL_str,  FALSE))
					literal = TRUE;
				else if (compare(str1, NOLITERAL_str,  FALSE))
					literal = FALSE;
				else if (compare(str1, STATUS_str,  FALSE))
					status_line = TRUE;
				else if (compare(str1, NOSTATUS_str,  FALSE))
					status_line = FALSE;
				else if (compare(str1, INDENT_str,  FALSE))
					indent = TRUE;
				else if (compare(str1, NOINDENT_str,  FALSE))
					indent = FALSE;
				else if (compare(str1, OVERSTRIKE_str,  FALSE))
					overstrike = TRUE;
				else if (compare(str1, NOOVERSTRIKE_str,  FALSE))
					overstrike = FALSE;
				else if (compare(str1, AUTOFORMAT_str,  FALSE))
				{
					auto_format = TRUE;
					observ_margins = TRUE;
					indent = FALSE;
				}
				else if (compare(str1, NOAUTOFORMAT_str,  FALSE))
					auto_format = FALSE;
				else if (compare(str1, EIGHT_str,  FALSE))
					eightbit = TRUE;
				else if (compare(str1, NOEIGHT_str,  FALSE))
					eightbit = FALSE;
				else if (compare(str1, MARGINS_str,  FALSE))
					observ_margins = TRUE;
				else if (compare(str1, NOMARGINS_str,  FALSE))
					observ_margins = FALSE;
				else if (compare(str1, ee_mode_str,  FALSE))
				{
					ee_mode_menu = TRUE;
					main_menu[0].item_string = ee_mode_main_menu_strings[0];
					main_menu[1].item_string = ee_mode_main_menu_strings[1];
					main_menu[2].item_string = ee_mode_main_menu_strings[2];
					main_menu[3].item_string = ee_mode_main_menu_strings[3];
					main_menu[4].item_string = ee_mode_main_menu_strings[4];
					main_menu[5].item_string = ee_mode_main_menu_strings[5];
					main_menu[6].item_string = ee_mode_main_menu_strings[6];
					main_menu[7].item_string = ee_mode_main_menu_strings[7];
					main_menu[8].item_string = ee_mode_main_menu_strings[8];
					main_menu[3].procedure = menu_op;
					main_menu[3].ptr_argument = file_menu;
					main_menu[4].procedure = NULL;
					main_menu[4].ptr_argument = NULL;
					main_menu[4].iprocedure = NULL;
					main_menu[4].nprocedure = redraw;
					main_menu[5].procedure = NULL;
					main_menu[5].ptr_argument = NULL;
					main_menu[5].iprocedure = NULL;
					main_menu[5].nprocedure = modes_op;
					main_menu[6].procedure = menu_op;
					main_menu[6].ptr_argument = search_menu;
					main_menu[6].iprocedure = NULL;
					main_menu[6].nprocedure = NULL;
					main_menu[7].procedure = menu_op;
					main_menu[7].ptr_argument = misc_menu;
					main_menu[7].iprocedure = NULL;
					main_menu[7].nprocedure = NULL;
					main_menu[8].procedure = menu_op;
					main_menu[8].ptr_argument = edit_menu;
					main_menu[8].iprocedure = NULL;
					main_menu[8].nprocedure = NULL;
				}
				else if ((compare(str1, LEFTMARGIN_str,  FALSE)) || (compare(str1, RIGHTMARGIN_str,  FALSE)))
				{
					tmp = next_word(str1);
					if ((*tmp >= '0') && (*tmp <= '9'))
					{
						c_int = atoi(tmp);
						if (compare(str1, LEFTMARGIN_str,  FALSE))
							left_margin = c_int;
						else if (compare(str1, RIGHTMARGIN_str,  FALSE))
							right_margin = c_int;
					}
				}
				else if (compare(str1, info_win_height_cmd_str,  FALSE))
				{
					tmp = next_word(str1);
					if ((*tmp >= '0') && (*tmp <= '9'))
					{
						c_int = atoi(tmp);
						if ((c_int > 0) && (c_int <= MAX_HELP_LINES))
							info_win_height = c_int + 1;
					}
				}
				else if (compare(str1, ECHO_str,  FALSE))
				{
#ifndef XAE
					str1 = next_word(str1);
					if (*str1 != '\0')
						echo_string(str1);
#endif
				}
				else if (compare(str1, PRINTCOMMAND_str, FALSE))
				{
					str1 = next_word(str1);
					print_command = xalloc(strlen(str1)+1);
					copy_str(str1, print_command);
				}
				else if (compare(str1, INFO_str,  FALSE))
					info_window = TRUE;
				else if (compare(str1, NOINFO_str,  FALSE))
					info_window = FALSE;   
				else if (compare(str1, HIGHLIGHT_str,  FALSE))
					nohighlight = FALSE;
				else if (compare(str1, NOHIGHLIGHT_str,  FALSE))
					nohighlight = TRUE;
				else if (compare(str1, jrnl_dir,  FALSE))
				{
					str1 = next_word(str1);
					tmp = str1;
					while ((*tmp != ' ') && 
					       (*tmp != '\t') && 
					       (*tmp != '\0'))
						tmp++;
					*tmp = '\0';
					journal_dir = resolve_name(str1);
					if (str1 == journal_dir)
					{
						journal_dir = 
						   xalloc(strlen(str1) + 1);
						strcpy(journal_dir, str1);
					}
					value = create_dir(journal_dir);
					if (value == -1)
					{
						/*
						 |  journal dir doesn't exist
						 */
						free(journal_dir);
						journal_dir = NULL;
					}
				}
				else if (compare(str1, text_cmd,  FALSE))
					text_only = TRUE;
				else if (compare(str1, binary_cmd,  FALSE))
					text_only = FALSE;
				else if (compare(str1, help_file_str,  FALSE))
				{
					str1 = next_word(str1);
					tmp = str1;
					while ((*tmp != ' ') && 
					       (*tmp != '\t') && 
					       (*tmp != '\0'))
						tmp++;
					*tmp = '\0';
					ae_help_file = resolve_name(str1);
					if (str1 == ae_help_file)
					{
						ae_help_file = 
						   xalloc(strlen(str1) + 1);
						strcpy(ae_help_file, str1);
					}
				}
				lines++;
			}
			fclose(init_file);
			if (!compare(curr_buff->name, main_buffer_name, TRUE))
				chng_buf(main_buffer_name);
		}
	}
	free(string);
}

