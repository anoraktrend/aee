/*
 |	localize.c
 |
 |	$Header: /home/hugh/sources/aee/RCS/localize.c,v 1.31 2010/07/18 21:54:48 hugh Exp hugh $
 */

/*
 |	Copyright (c) 1994, 1995, 1996, 1998, 1999, 2001, 2002, 2010 Hugh Mahon.
 */

#include "aee.h"

#ifndef NO_CATGETS

#if defined(__STDC__) || defined(__cplusplus)
#define P_(s) s
#else
#define P_(s) ()
#endif

char *catgetlocal P_((int number, char *string));

#undef P_

/*
 |	Get the catalog entry, and if it got it from the catalog, 
 |	make a copy, since the buffer will be overwritten by the 
 |	next call to catgets().
 */

char *
catgetlocal(number, string)
int number;
char *string;
{
	char *temp1;
	char *temp2;

	temp1 = catgets(catalog, 1, number, string);
	if (temp1 != string)
	{
		temp2 = malloc(strlen(temp1) + 1);
		strcpy(temp2, temp1);
		temp1 = temp2;
	}
	return(temp1);
}
#endif /* NO_CATGETS */

/*
 |	The following is to allow for using message catalogs which allow 
 |	the software to be 'localized', that is, to use different languages 
 |	all with the same binary.  For more information, see your system 
 |	documentation, or the X/Open Internationalization Guide.
 */

void 
strings_init()
{
	int counter;

#ifndef NO_CATGETS
	setlocale(LC_ALL, "");
	catalog = catopen("aee", 0);
#endif /* NO_CATGETS */

	ae_help_file = catgetlocal( 1, "/usr/local/lib/help.ae");
	main_buffer_name = catgetlocal( 2, "main");


	/*
	 |	allocate space for strings here for settings menu
	 */

	for (counter = 1; counter < (NUM_MODES_ITEMS - 1); counter++)
	{
		modes_menu[counter].item_string = malloc(80);
	}

	modes_menu[0].item_string = catgetlocal( 3, "modes menu");
	mode_strings[1] = catgetlocal( 4, "tabs to spaces        "); 
	mode_strings[2] = catgetlocal( 5, "case sensitive search ");
	mode_strings[3] = catgetlocal( 6, "literal search        ");
	mode_strings[4] = catgetlocal( 7, "search direction  ");
	mode_strings[5] = catgetlocal( 8, "observe margins       ");
	mode_strings[6] = catgetlocal( 9, "info window           ");
	mode_strings[7] = catgetlocal( 10, "status line           ");
	mode_strings[8] = catgetlocal( 11, "auto indent           ");
	mode_strings[9] = catgetlocal( 12, "overstrike            ");
	mode_strings[10] = catgetlocal( 13, "auto paragraph format ");
	mode_strings[11] = catgetlocal( 14, "multi windows         ");
	mode_strings[12] = catgetlocal( 15, "left margin           ");
	mode_strings[13] = catgetlocal( 16, "right margin          ");
	mode_strings[14] = catgetlocal( 376, "info window height    ");
	mode_strings[15] = catgetlocal( 393, "text/binary mode is   ");
	mode_strings[16] = catgetlocal( 394, "current file type is  ");
	mode_strings[17] = catgetlocal( 385, "save editor config    ");
	leave_menu[0].item_string  = catgetlocal( 17, "leave menu");
	leave_menu[1].item_string  = catgetlocal( 18, "save changes");
	leave_menu[2].item_string  = catgetlocal( 19, "no save");
	leave_menu_2[0].item_string = catgetlocal( 20, "multiple buffers exist");
	leave_menu_2[1].item_string = catgetlocal( 21, "stay in editor");
	leave_menu_2[2].item_string = catgetlocal( 22, "leave anyway");
	file_menu[0].item_string  = catgetlocal( 23, "file menu");
	file_menu[1].item_string  = catgetlocal( 24, "read a file");
	file_menu[2].item_string  = catgetlocal( 25, "write a file");
	file_menu[3].item_string  = catgetlocal( 26, "save file");
	file_menu[4].item_string  = catgetlocal( 27, "print editor contents");
	file_menu[5].item_string  = catgetlocal( 375, "recover from journal");
	search_menu[0].item_string = catgetlocal( 28, "search/replace menu");
	search_menu[1].item_string = catgetlocal( 29, "search for ...");
	search_menu[2].item_string = catgetlocal( 30, "search");
	search_menu[3].item_string = catgetlocal( 31, "replace prompt ...");
	search_menu[4].item_string = catgetlocal( 32, "replace");
	spell_menu[0].item_string = catgetlocal( 33, "spell menu");
	spell_menu[1].item_string = catgetlocal( 34, "use 'spell'");
	spell_menu[2].item_string = catgetlocal( 35, "use 'ispell'");
	misc_menu[0].item_string = catgetlocal( 36, "miscellaneous menu");
	misc_menu[1].item_string = catgetlocal( 37, "format paragraph");
	misc_menu[2].item_string = catgetlocal( 38, "shell command");
	misc_menu[3].item_string = catgetlocal( 39, "check spelling");
	edit_menu[0].item_string = catgetlocal( 40, "edit menu");
	edit_menu[1].item_string = catgetlocal( 41, "mark text");
	edit_menu[2].item_string = catgetlocal( 42, "copy marked text");
	edit_menu[3].item_string = catgetlocal( 43, "cut (delete) marked text");
	edit_menu[4].item_string = catgetlocal( 44, "paste");
	main_menu[0].item_string = main_menu_strings[0] = catgetlocal( 45, "main menu");
	main_menu[1].item_string = main_menu_strings[1] = catgetlocal( 46, "leave editor");
	main_menu[2].item_string = main_menu_strings[2] = catgetlocal( 47, "help");
	main_menu[3].item_string = main_menu_strings[3] = catgetlocal( 48, "edit");
	main_menu[4].item_string = main_menu_strings[4] = catgetlocal( 49, "file operations");
	main_menu[5].item_string = main_menu_strings[5] = catgetlocal( 50, "redraw screen");
	main_menu[6].item_string = main_menu_strings[6] = catgetlocal( 51, "settings");
	main_menu[7].item_string = main_menu_strings[7] = catgetlocal( 52, "search/replace");
	main_menu[8].item_string = main_menu_strings[8] = catgetlocal( 53, "miscellaneous");

	/*
	 |	have main menu entries in same manner as ee's
	 */

	ee_mode_main_menu_strings[0] = main_menu_strings[0]; /* title */
	ee_mode_main_menu_strings[1] = main_menu_strings[1]; /* leave */
	ee_mode_main_menu_strings[2] = main_menu_strings[2]; /* help */
	ee_mode_main_menu_strings[3] = main_menu_strings[4]; /* file */
	ee_mode_main_menu_strings[4] = main_menu_strings[5]; /* redraw screen */
	ee_mode_main_menu_strings[5] = main_menu_strings[6]; /* settings */
	ee_mode_main_menu_strings[6] = main_menu_strings[7]; /* search/replace */
	ee_mode_main_menu_strings[7] = main_menu_strings[8]; /* misc */
	ee_mode_main_menu_strings[8] = main_menu_strings[3]; /* edit */


	SL_line_str = catgetlocal( 54, "line=%d");
	SL_col_str = catgetlocal( 55, "col=%d");
	SL_lit_str = catgetlocal( 56, " lit  ");
	SL_nolit_str = catgetlocal( 57, " nolit");
	SL_fwd_str = catgetlocal( 58, " fwd ");
	SL_rev_str = catgetlocal( 59, " rev ");
	SL_over_str = catgetlocal( 60, "over  ");
	SL_insrt_str = catgetlocal( 61, "insrt ");
	SL_indent_str = catgetlocal( 62, "indent  ");
	SL_noindnt_str = catgetlocal( 63, "noindnt ");
	SL_marg_str = catgetlocal( 64, "  marg ");
	SL_nomarg_str = catgetlocal( 65, "nomarg ");
	SL_mark_str = catgetlocal( 66, "mark");
	ascii_code_str = catgetlocal( 67, "ascii code: ");
	left_err_msg = catgetlocal( 68, "attempted to move cursor before start of file");
	right_err_msg = catgetlocal( 69, "attempted to move past end of file");
	help_err_msg = catgetlocal( 70, "unable to open help file %s");
	prompt_for_more = catgetlocal( 71, "(press RETURN for more): ");
	topic_prompt = catgetlocal( 72, "topic? (TAB then RETURN to go to the main menu. RETURN to exit): ");
	topic_err_msg = catgetlocal( 73, "cannot find topic %s");
	continue_prompt = catgetlocal( 74, "press return to continue ");
	printing_msg = catgetlocal( 75, "sending contents of buffer %s to \"%s\" ");
	EXPAND_str = catgetlocal( 76, "EXPAND");
	NOEXPAND_str = catgetlocal( 77, "NOEXPAND");
	NOJUSTIFY_str = catgetlocal( 78, "NOJUSTIFY");
	JUSTIFY_str = catgetlocal( 79, "JUSTIFY");
	EXIT_str = catgetlocal( 80, "EXIT");
	QUIT_str = catgetlocal( 81, "QUIT");
	AUTOFORMAT_str = catgetlocal( 82, "AUTOFORMAT");
	NOAUTOFORMAT_str = catgetlocal( 83, "NOAUTOFORMAT");
	INFO_str = catgetlocal( 84, "INFO");
	NOINFO_str = catgetlocal( 85, "NOINFO");
	TABS_str = catgetlocal( 86, "TABS");
	UNTABS_str = catgetlocal( 87, "UNTABS");
	WRITE_str = catgetlocal( 88, "WRITE");
	READ_str = catgetlocal( 89, "READ");
	SAVE_str = catgetlocal( 90, "SAVE");
	LITERAL_str = catgetlocal( 91, "LITERAL");
	NOLITERAL_str = catgetlocal( 92, "NOLITERAL");
	STATUS_str = catgetlocal( 93, "STATUS");
	NOSTATUS_str = catgetlocal( 94, "NOSTATUS");
	MARGINS_str = catgetlocal( 95, "MARGINS");
	NOMARGINS_str = catgetlocal( 96, "NOMARGINS");
	INDENT_str = catgetlocal( 97, "INDENT");
	NOINDENT_str = catgetlocal( 98, "NOINDENT");
	OVERSTRIKE_str = catgetlocal( 99, "OVERSTRIKE");
	NOOVERSTRIKE_str = catgetlocal( 100, "NOOVERSTRIKE");
	LEFTMARGIN_str = catgetlocal( 101, "LEFTMARGIN");
	RIGHTMARGIN_str = catgetlocal( 102, "RIGHTMARGIN");
	LINE_str = catgetlocal( 103, "LINE");
	FILE_str = catgetlocal( 104, "FILE");
	COPYRIGHT_str = catgetlocal( 105, "COPYRIGHT");
	CHARACTER_str = catgetlocal( 106, "CHARACTER");
	REDRAW_str = catgetlocal( 107, "REDRAW");
	RESEQUENCE_str = catgetlocal( 108, "RESEQUENCE");
	AUTHOR_str = catgetlocal( 109, "AUTHOR");
	VERSION_str = catgetlocal( 110, "VERSION");
	CASE_str = catgetlocal( 111, "CASE");
	NOCASE_str = catgetlocal( 112, "NOCASE");
	EIGHT_str = catgetlocal( 113, "EIGHT");
	NOEIGHT_str = catgetlocal( 114, "NOEIGHT");
	WINDOWS_str = catgetlocal( 115, "WINDOWS");
	NOWINDOWS_str = catgetlocal( 116, "NOWINDOWS");
	DEFINE_str = catgetlocal( 117, "DEFINE");
	SHOW_str = catgetlocal( 118, "SHOW");
	HELP_str = catgetlocal( 119, "HELP");
	PRINT_str = catgetlocal( 120, "PRINT");
	BUFFER_str = catgetlocal( 121, "BUFFER");
	DELETE_str = catgetlocal( 122, "DELETE");
	DIFF_str = catgetlocal (406, "DIFF");
	GOLD_str = catgetlocal( 123, "GOLD");
	tab_msg = catgetlocal( 124, "set tabs are: ");
	file_write_prompt_str = catgetlocal( 125, "name of file to write: ");
	file_read_prompt_str = catgetlocal( 126, "name of file to read: ");
	left_mrg_err_msg = catgetlocal( 127, "cannot set left margin to value greater than right margin");
	right_mrg_err_msg = catgetlocal( 128, "cannot set right margin to value less than left margin");
	left_mrg_setting = catgetlocal( 129, "left margin is set to %d");
	right_mrg_setting = catgetlocal( 130, "right margin is set to %d");
	line_num_str = catgetlocal( 131, "line %d");
	lines_from_top = catgetlocal( 132, ", lines from top = %d, ");
	total_lines_str = catgetlocal( 133, "total number of lines = %d");
	current_file_str = catgetlocal( 134, "current file is \"%s\" ");
	char_str = catgetlocal( 135, "character = %d");
	key_def_msg = catgetlocal( 136, "key is defined as \"%s\"");
	unkn_syntax_msg = catgetlocal( 137, "unknown syntax beginning at \"%s\"");
	current_buff_msg = catgetlocal( 138, "current buffer is %s");
	unkn_cmd_msg = catgetlocal( 139, "unknown command \"%s\"");
	usage_str = catgetlocal( 140, "usage: %s file [-j] [-r] [-e] [-t] [-n] [i] [+line]\n");
	read_only_msg = catgetlocal( 141, " read only");
	no_rcvr_fil_msg = catgetlocal( 142, "no recover file");
	cant_opn_rcvr_fil_msg = catgetlocal( 143, " can't open new recover file");
	fin_read_file_msg = catgetlocal( 144, "finished reading file %s");
	rcvr_op_comp_msg = catgetlocal( 145, "recover operation complete");
	file_is_dir_msg = catgetlocal( 146, "file %s is a directory");
	path_not_a_dir_msg = catgetlocal( 147, "path is not a directory");
	access_not_allowed_msg = catgetlocal( 148, "mode of file %s does not allow access");
	new_file_msg = catgetlocal( 149, "new file %s");
	cant_open_msg = catgetlocal( 150, "can't open %s");
	bad_rcvr_msg = catgetlocal( 151, " can't open old recover file");
	reading_file_msg = catgetlocal( 152, "reading file %s");
	file_read_lines_msg = catgetlocal( 153, "file %s, %d lines");
	other_buffs_exist_msg = catgetlocal( 154, "other buffers exist, information will be lost, leave anyway? (y/n) ");
	changes_made_prompt = catgetlocal( 155, "changes have been made, are you sure? (y/n [n]) ");
	no_write_access_msg = catgetlocal( 156, "mode of file %s does not allow write access");
	file_exists_prompt = catgetlocal( 157, "file already exists, overwrite? (y/n) [n] ");
	cant_creat_fil_msg = catgetlocal( 158, "unable to create file %s");
	writing_msg = catgetlocal( 159, "writing file %s");
	file_written_msg = catgetlocal( 160, "%s %d lines, %d characters");
	searching_msg = catgetlocal( 161, "	     ...searching");
	fwd_srch_msg = catgetlocal( 162, " forward...");
	rev_srch_msg = catgetlocal( 163, " reverse...");
	str_str = catgetlocal( 164, "string ");
	not_fnd_str = catgetlocal( 165, " not found");
	search_prompt_str = catgetlocal( 166, "search for: ");
	mark_not_actv_str = catgetlocal( 167, "mark not active, must set 'mark' before cut or copy (see 'help' in the menu)");
	mark_active_str = catgetlocal( 168, "mark active");
	mark_alrdy_actv_str = catgetlocal( 169, "mark is already active");
	no_buff_named_str = catgetlocal( 170, "\nno buffer named \"%s\"\n");
	press_ret_to_cont_str = catgetlocal( 171, "press return to continue ");

	fn_GOLD_str = catgetlocal( 172, "GOLD");
	fn_DL_str = catgetlocal( 173, "DL");
	fn_DC_str = catgetlocal( 174, "DC");
	fn_CL_str = catgetlocal( 175, "CL");
	fn_NP_str = catgetlocal( 176, "NP");
	fn_PP_str = catgetlocal( 177, "PP");
	fn_NB_str = catgetlocal( 178, "NB");
	fn_PB_str = catgetlocal( 179, "PB");
	fn_UDL_str = catgetlocal( 180, "UDL");
	fn_UDC_str = catgetlocal( 181, "UDC");
	fn_DW_str = catgetlocal( 182, "DW");
	fn_UDW_str = catgetlocal( 183, "UDW");
	fn_UND_str = catgetlocal( 184, "UND");
	fn_EOL_str = catgetlocal( 185, "EOL");
	fn_BOL_str = catgetlocal( 186, "BOL");
	fn_BOT_str = catgetlocal( 187, "BOT");
	fn_EOT_str = catgetlocal( 189, "EOT");
	fn_FORMAT_str = catgetlocal( 190, "FORMAT");
	fn_MARGINS_str = catgetlocal( 191, "MARGINS");
	fn_NOMARGINS_str = catgetlocal( 192, "NOMARGINS");
	fn_IL_str = catgetlocal( 193, "IL");
	fn_PRP_str = catgetlocal( 194, "PRP");
	fn_RP_str = catgetlocal( 195, "RP");
	fn_MC_str = catgetlocal( 196, "MC");
	fn_PSRCH_str = catgetlocal( 197, "PSRCH");
	fn_SRCH_str = catgetlocal( 198, "SRCH");
	fn_AL_str = catgetlocal( 199, "AL");
	fn_AW_str = catgetlocal( 200, "AW");
	fn_AC_str = catgetlocal( 201, "AC");
	fn_PW_str = catgetlocal( 202, "PW");
	fn_CUT_str = catgetlocal( 203, "CUT");
	fn_FWD_str = catgetlocal( 204, "FWD");
	fn_REV_str = catgetlocal( 205, "REV");
	fn_MARK_str = catgetlocal( 206, "MARK");
	fn_UNMARK_str = catgetlocal( 207, "UNMARK");
	fn_APPEND_str = catgetlocal( 208, "APPEND");
	fn_PREFIX_str = catgetlocal( 209, "PREFIX");
	fn_COPY_str = catgetlocal( 210, "COPY");
	fn_CMD_str = catgetlocal( 211, "CMD");
	fn_PST_str = catgetlocal( 212, "PST");
	fn_RD_str = catgetlocal( 213, "RD");
	fn_UP_str = catgetlocal( 214, "UP");
	fn_DOWN_str = catgetlocal( 215, "DOWN");
	fn_LEFT_str = catgetlocal( 216, "LEFT");
	fn_RIGHT_str = catgetlocal( 217, "RIGHT");
	fn_BCK_str = catgetlocal( 218, "BCK");
	fn_CR_str = catgetlocal( 219, "CR");
	fn_EXPAND_str = catgetlocal( 220, "EXPAND");
	fn_NOEXPAND_str = catgetlocal( 221, "NOEXPAND");
	fn_EXIT_str = catgetlocal( 222, "EXIT");
	fn_QUIT_str = catgetlocal( 223, "QUIT");
	fn_LITERAL_str = catgetlocal( 224, "LITERAL");
	fn_NOLITERAL_str = catgetlocal( 225, "NOLITERAL");
	fn_STATUS_str = catgetlocal( 226, "STATUS");
	fn_NOSTATUS_str = catgetlocal( 227, "NOSTATUS");
	fn_INDENT_str = catgetlocal( 228, "INDENT");
	fn_NOINDENT_str = catgetlocal( 229, "NOINDENT");
	fn_OVERSTRIKE_str = catgetlocal( 230, "OVERSTRIKE");
	fn_NOOVERSTRIKE_str = catgetlocal( 231, "NOOVERSTRIKE");
	fn_CASE_str = catgetlocal( 232, "CASE");
	fn_NOCASE_str = catgetlocal( 233, "NOCASE");
	fn_WINDOWS_str = catgetlocal( 234, "WINDOWS");
	fn_NOWINDOWS_str = catgetlocal( 235, "NOWINDOWS");
	fn_HELP_str = catgetlocal( 236, "HELP");
	fn_MENU_str = catgetlocal( 237, "MENU");
	fwd_mode_str = catgetlocal( 238, "forward mode");
	rev_mode_str = catgetlocal( 239, "reverse mode");
	ECHO_str = catgetlocal( 240, "ECHO");
	cmd_prompt = catgetlocal( 241, "command: ");
	PRINTCOMMAND_str = catgetlocal( 242, "PRINTCOMMAND");
	replace_action_prompt = catgetlocal( 243, "replace (r or <return>), skip (s), all occurrances (a), quit (q): ");
	replace_prompt_str = catgetlocal( 244, "/old_string/new_string/ : ");
	replace_r_char = catgetlocal( 245, "r");
	replace_skip_char = catgetlocal( 246, "s");
	replace_all_char = catgetlocal( 247, "a");
	quit_char = catgetlocal( 248, "q");
	yes_char = catgetlocal( 249, "y");
	no_char = catgetlocal( 250, "n");
	cant_find_match_str = catgetlocal( 251, "unable to find matching character");
	fmting_par_msg = catgetlocal( 252, "...formatting paragraph...");
	cancel_string = catgetlocal( 253, "press Esc to cancel");
	menu_too_lrg_msg = catgetlocal( 254, "menu too large for window");
	shell_cmd_prompt = catgetlocal( 255, "shell command: ");
	shell_echo_msg = catgetlocal( 256, "<!echo 'list of unrecognized words'; echo -=-=-=-=-=-");
	spell_in_prog_msg = catgetlocal( 257, "sending contents of edit buffer to 'spell'");
	left_marg_prompt = catgetlocal( 258, "left margin is: ");
	left_mrgn_err_msg = catgetlocal( 259, "left margin must be less than right margin");
	right_mrgn_prompt = catgetlocal( 260, "right margin is: ");
	right_mrgn_err_msg = catgetlocal( 261, "right margin must be greater than left margin");
	assignment[1].description = catgetlocal( 262, "menu      ");
	assignment[2].description = catgetlocal( 263, "up        ");
	assignment[3].description = catgetlocal( 264, "down      ");
	assignment[4].description = catgetlocal( 265, "left      ");
	assignment[5].description = catgetlocal( 266, "right     ");
	assignment[6].description = catgetlocal( 267, "command   ");
	assignment[7].description = catgetlocal( 268, "top of txt");
	assignment[8].description = catgetlocal( 269, "end of txt");
	assignment[9].description = catgetlocal( 270, "next page ");
	assignment[10].description = catgetlocal( 271, "prev page ");
	assignment[11].description = catgetlocal( 272, "del line  ");
	assignment[12].description = catgetlocal( 273, "und line  ");
	assignment[13].description = catgetlocal( 274, "del word  ");
	assignment[14].description = catgetlocal( 275, "und word  ");
	assignment[15].description = catgetlocal( 276, "del char  ");
	assignment[16].description = catgetlocal( 277, "und char  ");
	assignment[17].description = catgetlocal( 278, "undelete  ");
	assignment[18].description = catgetlocal( 279, "search    ");
	assignment[19].description = catgetlocal( 280, "srch prmpt");
	assignment[20].description = catgetlocal( 281, "beg of lin");
	assignment[21].description = catgetlocal( 282, "end of lin");
	assignment[22].description = catgetlocal( 283, "mark      ");
	assignment[23].description = catgetlocal( 284, "cut       ");
	assignment[24].description = catgetlocal( 285, "copy      ");
	assignment[25].description = catgetlocal( 286, "paste     ");
	assignment[26].description = catgetlocal( 287, "adv word  ");
	assignment[27].description = catgetlocal( 288, "replace   ");
	assignment[28].description = catgetlocal( 289, "repl prmpt");
	assignment[29].description = catgetlocal( 290, "clear line");
	assignment[30].description = catgetlocal( 291, "next buff ");
	assignment[31].description = catgetlocal( 292, "prev buff ");
	assignment[32].description = catgetlocal( 293, "fmt parag ");
	assignment[33].description = catgetlocal( 294, "margins   ");
	assignment[34].description = catgetlocal( 295, "nomargins ");
	assignment[35].description = catgetlocal( 296, "ins line  ");
	assignment[36].description = catgetlocal( 297, "match char");
	assignment[37].description = catgetlocal( 298, "adv line  ");
	assignment[38].description = catgetlocal( 299, "adv char  ");
	assignment[39].description = catgetlocal( 300, "prev word ");
	assignment[40].description = catgetlocal( 301, "forward   ");
	assignment[41].description = catgetlocal( 302, "reverse   ");
	assignment[42].description = catgetlocal( 303, "unmark    ");
	assignment[43].description = catgetlocal( 304, "append    ");
	assignment[44].description = catgetlocal( 305, "prefix    ");
	assignment[45].description = catgetlocal( 306, "redraw    ");
	assignment[46].description = catgetlocal( 307, "expand    ");
	assignment[47].description = catgetlocal( 308, "noexpand  ");
	assignment[48].description = catgetlocal( 309, "exit      ");
	assignment[49].description = catgetlocal( 310, "quit      ");
	assignment[50].description = catgetlocal( 311, "literal   ");
	assignment[51].description = catgetlocal( 312, "noliteral ");
	assignment[52].description = catgetlocal( 313, "status    ");
	assignment[53].description = catgetlocal( 314, "nostatus  ");
	assignment[54].description = catgetlocal( 315, "indent    ");
	assignment[55].description = catgetlocal( 316, "noindent  ");
	assignment[56].description = catgetlocal( 317, "overstrike");
	assignment[57].description = catgetlocal( 318, "noovrstrke");
	assignment[58].description = catgetlocal( 319, "case sens ");
	assignment[59].description = catgetlocal( 320, "nocase sns");
	assignment[60].description = catgetlocal( 321, "windows   ");
	assignment[61].description = catgetlocal( 322, "nowindows ");
	assignment[62].description = catgetlocal( 323, "help      ");
	assignment[63].description = catgetlocal( 324, "carrg rtrn");
	assignment[64].description = catgetlocal( 325, "backspace ");
	assignment[65].description = catgetlocal( 326, "GOLD      ");
	
	cant_redef_msg = catgetlocal( 327, "cannot redefine control-%c");
	buff_msg = catgetlocal( 328, "buffer = %s");
	cant_chng_while_mark_msg = catgetlocal( 329, "you may not change buffers while mark is active");
	too_many_parms_msg = catgetlocal( 330, "too many parameters");
	buff_is_main_msg = catgetlocal( 331, "buffer = main");
	cant_del_while_mark = catgetlocal( 332, "cannot delete buffer while mark active");
	cant_del_buf_msg = catgetlocal( 333, "cannot delete buffer \"%s\" ");
	HIGHLIGHT_str = catgetlocal( 334, "HIGHLIGHT");
	NOHIGHLIGHT_str = catgetlocal( 335, "NOHIGHLIGHT");
	non_unique_cmd_msg = catgetlocal( 336, "entered command is not unique");
	ON = catgetlocal( 337, "ON");
	OFF = catgetlocal( 338, "OFF");
	save_file_name_prompt = catgetlocal( 339, "enter name of file: ");
	file_not_saved_msg = catgetlocal( 340, "no filename entered: file not saved");
	cant_reopen_journ = catgetlocal( 341, "can't reopen journal file");
	write_err_msg = catgetlocal( 342, "WRITE ERROR!");
	jrnl_dir = catgetlocal( 343, "journaldir");
	CD_str = catgetlocal( 344, "CD");
	no_dir_entered_msg = catgetlocal( 344, "ERROR: no directory entered");
	path_not_dir_msg = catgetlocal( 345, "ERROR: path is not a directory");
	path_not_permitted_msg = catgetlocal( 346, "ERROR: no permission for path");
	path_chng_failed_msg = catgetlocal( 347, "ERROR: path changed failed");
	no_chng_dir_msg = catgetlocal( 348, "ERROR: unable to determine file's full path, unable to change directory");
	SPACING_str = catgetlocal( 349, "spacing");
	SPACING_msg = catgetlocal( 350, "tabs are spaced every %d columns");
	help_file_str = catgetlocal( 351, "helpfile");
	pwd_cmd_str = catgetlocal( 352, "pwd");
	pwd_err_msg = catgetlocal( 353, "error getting current working directory");
	no_file_string = catgetlocal( 354, "no file");
	no_chng_no_save = catgetlocal( 355, "no changes have been made, file not saved");
	changes_made_title = catgetlocal( 356, "changes have been made to this buffer");
	save_then_delete  = catgetlocal( 357, "save then delete");
	dont_delete       = catgetlocal( 358, "do not delete");
	delete_anyway_msg = catgetlocal( 359, "delete, do not save");
	edit_cmd_str = catgetlocal( 360, "EDIT");
	edit_err_msg = catgetlocal( 361, "unable to edit file '%s'");
	restricted_msg = catgetlocal( 367, "restricted mode: unable to perform requested operation");
	rae_no_file_msg = catgetlocal( 368, "must specify a file when invoking rae");
	more_above_str = catgetlocal( 369, "^^more^^");
	more_below_str = catgetlocal( 370, "VVmoreVV");
	recover_name_prompt = catgetlocal( 371, "file to recover? ");
	no_file_name_string = catgetlocal( 372, "[no file name]");
	recover_menu_title = catgetlocal( 373, "Files to recover");
	other_recover_file_str = catgetlocal( 374, "Other file...");
	/* 375 is above as file_menu[5].item_string */
	/* 376 is above as mode_strings[14] */	
	info_win_height_prompt = catgetlocal( 377, "height for info window: ");
	info_win_height_err = catgetlocal( 378, "ERROR: height for info window must be greater than zero and less than 16");
	info_win_height_cmd_str = catgetlocal( 379, "height");
	info_win_height_msg_str = catgetlocal( 380, "height of info window is: ");
	config_dump_menu[0].item_string = catgetlocal( 381, "save aee configuration");
	config_dump_menu[1].item_string = catgetlocal( 382, "save in current directory");
	config_dump_menu[2].item_string = catgetlocal( 383, "save in home directory");
	conf_not_saved_msg = catgetlocal( 384, "aee configuration not saved");
	/* 385 is above as mode_strings[17] */	
	conf_dump_err_msg = catgetlocal( 386, "unable to open .init.ae for writing, no configuration saved!");
	conf_dump_success_msg = catgetlocal( 387, "aee configuration saved in file %s");
	info_help_msg = catgetlocal( 388, " ^ = Ctrl key  ---- access HELP through menu ---");
	unix_text_msg = catgetlocal( 389, "uxtxt");
	dos_text_msg = catgetlocal( 390, "dostxt");
	text_cmd = catgetlocal( 391, "text");
	binary_cmd = catgetlocal( 392, "binary");
	/* 393 is above as mode_strings[15]	*/
	/* 394 is above as mode_strings[16]	*/
	text_msg = catgetlocal( 395, "txt");
	binary_msg = catgetlocal( 396, "bin");
	dos_msg = catgetlocal( 397, "DOS");
	unix_msg = catgetlocal( 398, " UX");
	file_being_edited_msg = catgetlocal( 399, "file %s already being edited");
	file_being_edited_menu[1].item_string = catgetlocal( 400, "don't edit");
	file_being_edited_menu[2].item_string = catgetlocal( 401, "edit anyway");
	file_modified_msg = catgetlocal( 402, "file %s has been modified since last read");
	file_modified_menu[1].item_string = catgetlocal( 403, "do not write file");
	file_modified_menu[2].item_string = catgetlocal( 404, "write anyway");
	file_modified_menu[3].item_string = catgetlocal( 405, "do not write file, show diffs");
	/* 406 is above as DIFF_str */
	ee_mode_str = catgetlocal( 407, "ee_mode");
	journal_str = catgetlocal( 408, "journal");
	journal_err_str = catgetlocal( 409, "Error in journal file operation, closing journal.");


	del_buff_menu[0].item_string = changes_made_title;
	del_buff_menu[1].item_string = save_then_delete;
	del_buff_menu[2].item_string = dont_delete;
	del_buff_menu[3].item_string = delete_anyway_msg;

	rae_err_menu[0].item_string = rae_no_file_msg;
	rae_err_menu[1].item_string = continue_prompt;

	/* ----- */

	assignment[1].macro = fn_MENU_str;
	assignment[2].macro = fn_UP_str;
	assignment[3].macro = fn_DOWN_str;
	assignment[4].macro = fn_LEFT_str;
	assignment[5].macro = fn_RIGHT_str;
	assignment[6].macro = fn_CMD_str;
	assignment[7].macro = fn_BOT_str;
	assignment[8].macro = fn_EOT_str;
	assignment[9].macro = fn_NP_str;
	assignment[10].macro = fn_PP_str;
	assignment[11].macro = fn_DL_str;
	assignment[12].macro = fn_UDL_str;
	assignment[13].macro = fn_DW_str;
	assignment[14].macro = fn_UDW_str;
	assignment[15].macro = fn_DC_str;
	assignment[16].macro = fn_UDC_str;
	assignment[17].macro = fn_UND_str;
	assignment[18].macro = fn_SRCH_str;
	assignment[19].macro = fn_PSRCH_str;
	assignment[20].macro = fn_BOL_str;
	assignment[21].macro = fn_EOL_str;
	assignment[22].macro = fn_MARK_str;
	assignment[23].macro = fn_CUT_str;
	assignment[24].macro = fn_COPY_str;
	assignment[25].macro = fn_PST_str;
	assignment[26].macro = fn_AW_str;
	assignment[27].macro = fn_RP_str;
	assignment[28].macro = fn_PRP_str;
	assignment[29].macro = fn_CL_str;
	assignment[30].macro = fn_NB_str;
	assignment[31].macro = fn_PB_str;
	assignment[32].macro = fn_FORMAT_str;
	assignment[33].macro = fn_MARGINS_str;
	assignment[34].macro = fn_NOMARGINS_str;
	assignment[35].macro = fn_IL_str;
	assignment[36].macro = fn_MC_str;
	assignment[37].macro = fn_AL_str;
	assignment[38].macro = fn_AC_str;
	assignment[39].macro = fn_PW_str;
	assignment[40].macro = fn_FWD_str;
	assignment[41].macro = fn_REV_str;
	assignment[42].macro = fn_UNMARK_str;
	assignment[43].macro = fn_APPEND_str;
	assignment[44].macro = fn_PREFIX_str;
	assignment[45].macro = fn_RD_str;
	assignment[46].macro = fn_EXPAND_str;
	assignment[47].macro = fn_NOEXPAND_str;
	assignment[48].macro = fn_EXIT_str;
	assignment[49].macro = fn_QUIT_str;
	assignment[50].macro = fn_LITERAL_str;
	assignment[51].macro = fn_NOLITERAL_str;
	assignment[52].macro = fn_STATUS_str;
	assignment[53].macro = fn_NOSTATUS_str;
	assignment[54].macro = fn_INDENT_str;
	assignment[55].macro = fn_NOINDENT_str;
	assignment[56].macro = fn_OVERSTRIKE_str;
	assignment[57].macro = fn_NOOVERSTRIKE_str;
	assignment[58].macro = fn_CASE_str;
	assignment[59].macro = fn_NOCASE_str;
	assignment[60].macro = fn_WINDOWS_str;
	assignment[61].macro = fn_NOWINDOWS_str;
	assignment[62].macro = fn_HELP_str;
	assignment[63].macro = fn_CR_str;
	assignment[64].macro = fn_BCK_str;
	assignment[65].macro = fn_GOLD_str;
	assignment[66].macro = NULL;
	
	commands[0] = WRITE_str;
	commands[1] = READ_str;
	commands[2] = SAVE_str;
	commands[3] = EXIT_str;
	commands[4] = QUIT_str;
	commands[5] = INFO_str;
	commands[6] = NOINFO_str;
	commands[7] = TABS_str;
	commands[8] = UNTABS_str;
	commands[9] = SPACING_str;
	commands[10] = LITERAL_str;
	commands[11] = NOLITERAL_str;
	commands[12] = STATUS_str;
	commands[13] = NOSTATUS_str;
	commands[14] = MARGINS_str;
	commands[15] = NOMARGINS_str;
	commands[16] = INDENT_str;
	commands[17] = NOINDENT_str;
	commands[18] = OVERSTRIKE_str;
	commands[19] = NOOVERSTRIKE_str;
	commands[20] = LEFTMARGIN_str;
	commands[21] = RIGHTMARGIN_str;
	commands[22] = DEFINE_str;
	commands[23] = SHOW_str;
	commands[24] = HELP_str;
	commands[25] = PRINT_str;
	commands[26] = BUFFER_str;
	commands[27] = DELETE_str;
	commands[28] = LINE_str;
	commands[29] = FILE_str;
	commands[30] = CHARACTER_str;
	commands[31] = REDRAW_str;
	commands[32] = RESEQUENCE_str;
	commands[33] = CASE_str;
	commands[34] = NOCASE_str;
	commands[35] = EIGHT_str;
	commands[36] = NOEIGHT_str;
	commands[37] = WINDOWS_str;
	commands[38] = NOWINDOWS_str;
	commands[39] = NOJUSTIFY_str;
	commands[40] = JUSTIFY_str;
	commands[41] = EXPAND_str;
	commands[42] = NOEXPAND_str;
	commands[43] = AUTOFORMAT_str;
	commands[44] = NOAUTOFORMAT_str;
	commands[45] = VERSION_str;
	commands[46] = AUTHOR_str;
	commands[47] = COPYRIGHT_str;
	commands[48] = CD_str;
	commands[49] = pwd_cmd_str;
	commands[50] = edit_cmd_str;
	commands[51] = info_win_height_cmd_str;
	commands[52] = text_cmd;
	commands[53] = binary_cmd;
	commands[54] = DIFF_str;
	commands[55] = ">";
	commands[56] = "!";
	commands[57] = "0";
	commands[58] = "1";
	commands[59] = "2";
	commands[60] = "3";
	commands[61] = "4";
	commands[62] = "5";
	commands[63] = "6";
	commands[64] = "7";
	commands[65] = "8";
	commands[66] = "9";
	commands[67] = "+";
	commands[68] = "-";
	commands[69] = "<";
	commands[70] = journal_str;
	commands[71] = NULL;

	init_strings[0] = INFO_str;
	init_strings[1] = NOINFO_str;
	init_strings[2] = TABS_str;
	init_strings[3] = UNTABS_str;
	init_strings[4] = LITERAL_str;
	init_strings[5] = NOLITERAL_str;
	init_strings[6] = STATUS_str;
	init_strings[7] = NOSTATUS_str;
	init_strings[8] = MARGINS_str;
	init_strings[9] = NOMARGINS_str;
	init_strings[10] = INDENT_str;
	init_strings[11] = NOINDENT_str;
	init_strings[12] = OVERSTRIKE_str;
	init_strings[13] = NOOVERSTRIKE_str;
	init_strings[14] = LEFTMARGIN_str;
	init_strings[15] = RIGHTMARGIN_str;
	init_strings[16] = DEFINE_str;
	init_strings[17] = CASE_str;
	init_strings[18] = NOCASE_str;
	init_strings[19] = EIGHT_str;
	init_strings[20] = NOEIGHT_str;
	init_strings[21] = WINDOWS_str;
	init_strings[22] = NOWINDOWS_str;
	init_strings[23] = NOJUSTIFY_str;
	init_strings[24] = JUSTIFY_str;
	init_strings[25] = EXPAND_str;
	init_strings[26] = NOEXPAND_str;
	init_strings[27] = AUTOFORMAT_str;
	init_strings[28] = NOAUTOFORMAT_str;
	init_strings[29] = ECHO_str;
	init_strings[30] = PRINTCOMMAND_str;
	init_strings[31] = HIGHLIGHT_str;
	init_strings[32] = NOHIGHLIGHT_str;
	init_strings[33] = jrnl_dir;
	init_strings[34] = SPACING_str;
	init_strings[35] = help_file_str;
	init_strings[36] = info_win_height_cmd_str;
	init_strings[37] = text_cmd;
	init_strings[38] = binary_cmd;
	init_strings[39] = ee_mode_str;
	init_strings[40] = NULL;

#ifndef NO_CATGETS
	catclose(catalog);
#endif /* NO_CATGETS */
}
