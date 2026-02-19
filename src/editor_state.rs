// Editor state and core data structures for aee
// Aligned with include/aee.h struct bufr, struct text, etc.

use std::rc::Rc;
use std::cell::RefCell;

// Main buffer name
pub const MAIN_BUFFER_NAME: &str = "main";

/// NO_FURTHER_LINES sentinel – mirrors C `NO_FURTHER_LINES` (0x0)
pub const NO_FURTHER_LINES: u64 = 0;

// ──────────────────────────────────────────────────────────────────────────────
// struct ae_file_info (mirrors aee.h)
// ──────────────────────────────────────────────────────────────────────────────
#[derive(Clone, Debug)]
pub struct AeFileInfo {
    /// location of this info block in the journal file
    pub info_location: u64,
    /// info location for the previous line
    pub prev_info: u64,
    /// info location for the next line
    pub next_info: u64,
    /// byte offset of actual line text in journal file
    pub line_location: u64,
}

impl Default for AeFileInfo {
    fn default() -> Self {
        AeFileInfo {
            info_location: NO_FURTHER_LINES,
            prev_info: NO_FURTHER_LINES,
            next_info: NO_FURTHER_LINES,
            line_location: 0,
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// struct text (mirrors aee.h)
// ──────────────────────────────────────────────────────────────────────────────
/// One line of text.
///
/// `line_length` follows the C convention: it **includes** the terminating
/// null slot, so an empty line has `line_length == 1` and a line "hello"
/// has `line_length == 6`.
#[derive(Clone)]
pub struct TextLine {
    /// The actual text content (no trailing NUL; length = line_length - 1).
    pub line: String,
    /// 1-based line number in the buffer.
    pub line_number: i32,
    /// Byte capacity of the line buffer (mirrors `max_length`).
    pub max_length: i32,
    /// Number of screen rows this line occupies (for wrapped lines).
    pub vert_len: i32,
    /// Journal file metadata for this line.
    pub file_info: AeFileInfo,
    /// True when the line content has changed since last journal write.
    pub changed: bool,
    /// Number of characters including the null terminator slot.
    pub line_length: i32,
    pub prev_line: Option<Rc<RefCell<TextLine>>>,
    pub next_line: Option<Rc<RefCell<TextLine>>>,
}

// ──────────────────────────────────────────────────────────────────────────────
// struct bufr (mirrors aee.h)
// ──────────────────────────────────────────────────────────────────────────────
/// An edit buffer – roughly equivalent to C `struct bufr`.
/// `next_buff` links buffers in the multi-buffer list; metadata fields are
/// populated by `load_file` and checked on save to detect external changes.
pub struct Buffer {
    /// Buffer name (short file name or generated name like "A", "B", …).
    pub name: String,
    /// First line of text in this buffer.
    pub first_line: Option<Rc<RefCell<TextLine>>>,
    /// Next buffer in the linked list (traversed by `EditorState::buf_count`).
    pub next_buff: Option<Rc<RefCell<Buffer>>>,
    /// Current line the cursor is on.
    pub curr_line: Option<Rc<RefCell<TextLine>>>,

    // ── Cursor / screen position ────────────────────────────────────────────
    /// Vertical position of cursor within the window (0-based).
    pub scr_vert: i32,
    /// Horizontal pixel/column offset within the current screen row.
    pub scr_horz: i32,
    /// Horizontal offset from the start of the logical line.
    pub scr_pos: i32,
    /// 1-based position within the current line (char index + 1).
    pub position: i32,
    /// "Absolute" horizontal position (used by up/down to restore column).
    pub abs_pos: i32,

    // ── Window geometry ─────────────────────────────────────────────────────
    /// Total text rows allocated to this buffer's window.
    pub lines: i32,
    /// Last visible row in the window (lines - 1).
    pub last_line: i32,
    /// Last visible column (COLS - 1).
    pub last_col: i32,
    /// Total number of lines in the buffer.
    pub num_of_lines: i32,
    /// Absolute line number of `curr_line` from the top of the file (1-based).
    pub absolute_lin: i32,
    /// First visible line number (1-based scroll offset).
    pub window_top: i32,

    // ── File metadata ───────────────────────────────────────────────────────
    /// Short file name (basename only).
    pub file_name: Option<String>,
    /// Full canonical path of the file being edited.
    pub full_name: Option<String>,
    /// Original directory when the edit session started.
    pub orig_dir: Option<String>,

    // ── State flags ─────────────────────────────────────────────────────────
    /// True when the buffer content has been modified.
    pub changed: bool,
    /// True if this is the first/main buffer.
    pub main_buffer: bool,
    /// True if this buffer was created via the `edit` command.
    pub edit_buffer: bool,
    /// True if the file uses DOS CR/LF line endings.
    pub dos_file: bool,

    // ── Journal (crash recovery) ─────────────────────────────────────────────
    /// True when journalling is active for this buffer.
    pub journalling: bool,
    /// Path to this buffer's journal (.rv) file.
    pub journal_file: Option<String>,

    // ── File stat cache (mirrors struct stat fields we care about) ───────────
    pub fileinfo_mtime: u64,
    pub fileinfo_size: u64,
}

// ──────────────────────────────────────────────────────────────────────────────
// Tab stops (mirrors struct tab_stops)
// ──────────────────────────────────────────────────────────────────────────────
#[derive(Clone, Debug)]
pub struct TabStop {
    pub column: i32,
    pub next_stop: Option<Box<TabStop>>,
}

// ──────────────────────────────────────────────────────────────────────────────
// Undo action (simplified – the C code has a 128-entry circular buffer)
// ──────────────────────────────────────────────────────────────────────────────
#[derive(Clone)]
pub enum LastAction {
    InsertChar { line: Rc<RefCell<TextLine>>, pos: usize },
    DeleteChar { line: Rc<RefCell<TextLine>>, pos: usize, ch: char },
}

// ──────────────────────────────────────────────────────────────────────────────
// Main editor state
// ──────────────────────────────────────────────────────────────────────────────
/// Main editor state – mirrors the collection of C globals in aee.c.
/// Fields tagged `#[allow(dead_code)]` are C-port placeholders that will be
/// wired up as individual features are ported from C; all others are actively
/// used by the Rust code.
#[allow(dead_code)]
pub struct EditorState {
    // ── Convenience references (also exist inside curr_buff) ─────────────────
    /// Mirrors `first_line` global – synced from `curr_buff` each iteration.
    pub first_line:  Option<Rc<RefCell<TextLine>>>,
    /// Mirrors `curr_line` global – synced from `curr_buff` each iteration.
    pub curr_line:   Option<Rc<RefCell<TextLine>>>,
    /// Cut/paste buffer (mirrors C `paste_buff`).
    #[allow(dead_code)] pub paste_buff:  Option<Rc<RefCell<TextLine>>>,
    /// Deleted-line buffer (mirrors C `dlt_line`).
    #[allow(dead_code)] pub dlt_line:    Option<Rc<RefCell<TextLine>>>,
    /// First paste-line pointer (mirrors C `fpste_line`).
    #[allow(dead_code)] pub fpste_line:  Option<Rc<RefCell<TextLine>>>,
    /// Current paste-line pointer (mirrors C `cpste_line`).
    #[allow(dead_code)] pub cpste_line:  Option<Rc<RefCell<TextLine>>>,
    /// Paste temp pointer (mirrors C `pste_tmp`).
    #[allow(dead_code)] pub pste_tmp:    Option<Rc<RefCell<TextLine>>>,
    /// Temp line pointer (mirrors C `tmp_line`).
    #[allow(dead_code)] pub tmp_line:    Option<Rc<RefCell<TextLine>>>,
    /// Search anchor line (mirrors C `srch_line`).
    #[allow(dead_code)] pub srch_line:   Option<Rc<RefCell<TextLine>>>,

    // ── Buffer list ───────────────────────────────────────────────────────────
    pub first_buff:  Option<Rc<RefCell<Buffer>>>,
    pub curr_buff:   Option<Rc<RefCell<Buffer>>>,

    // ── Boolean flags (mirrors C globals) ────────────────────────────────────
    pub windows:          bool,
    pub mark_text:        bool,
    pub journ_on:         bool,
    pub input_file:       bool,
    /// Main edit-mode flag; the event loop runs while this is `true`.
    pub edit:             bool,
    /// Gold-key (PF1) state – not yet implemented in terminal port.
    #[allow(dead_code)] pub gold:             bool,
    pub recover:          bool,
    pub case_sen:         bool,
    /// True when buffer has been modified this session (mirrors C `change`).
    pub change:           bool,
    /// Literal (non-regex) search mode.
    pub literal:          bool,
    pub forward:          bool,
    pub restricted:       bool,
    pub change_dir_allowed: bool,
    pub text_only:        bool,
    pub expand:           bool,
    pub nohighlight:      bool,
    pub echo_flag:        bool,
    pub info_window:      bool,
    /// Set when a file was given on the command line (used in startup path).
    pub recv_file:        bool,
    pub overstrike:       bool,
    pub indent:           bool,
    pub auto_format:      bool,
    /// Set when the current paragraph has been auto-formatted.
    #[allow(dead_code)] pub formatted:        bool,
    pub observ_margins:   bool,
    pub right_justify:    bool,
    pub status_line:      bool,
    /// Modes-menu visible flag (XAE / not yet ported).
    #[allow(dead_code)] pub ee_mode_menu:     bool,

    // ── XAE window geometry (not used in terminal build) ─────────────────────
    #[allow(dead_code)] pub win_height: i32,
    #[allow(dead_code)] pub win_width:  i32,

    // ── Editing settings ──────────────────────────────────────────────────────
    pub left_margin:     i32,
    pub right_margin:    i32,
    pub tab_spacing:     i32,
    pub info_win_height: i32,
    /// 8-bit (high-byte) input mode (XAE / not yet ported).
    #[allow(dead_code)] pub eightbit:        bool,

    // ── Search / replace strings ──────────────────────────────────────────────
    pub srch_str:    Option<String>,
    /// Previous search string (for undo-search; mirrors C `u_srch_str`).
    #[allow(dead_code)] pub u_srch_str:  Option<String>,
    /// Previous replace target (mirrors C `old_string`).
    #[allow(dead_code)] pub old_string:  Option<String>,
    /// Previous replace target undo (mirrors C `u_old_string`).
    #[allow(dead_code)] pub u_old_string: Option<String>,
    /// Replace-with string (mirrors C `new_string`).
    #[allow(dead_code)] pub new_string:  Option<String>,

    // ── Miscellaneous global state ─────────────────────────────────────────────
    pub files:         Vec<String>,
    pub start_at_line: Option<String>,
    /// Shell command used to print the buffer (mirrors C `print_command`).
    #[allow(dead_code)] pub print_command: String,
    pub journal_dir:   String,

    pub lines_moved: i32,
    /// Length of the last deleted word (mirrors C `d_wrd_len`).
    pub d_wrd_len:   i32,
    /// General-purpose integer (mirrors C `value`).
    #[allow(dead_code)] pub value:       i32,
    /// Temporary cursor position (mirrors C `tmp_pos`).
    #[allow(dead_code)] pub tmp_pos:     i32,
    /// Temporary vertical position (mirrors C `tmp_vert`).
    #[allow(dead_code)] pub tmp_vert:    i32,
    /// Replacement-string length accumulator (mirrors C `repl_length`).
    #[allow(dead_code)] pub repl_length: i32,
    /// Paste position (mirrors C `pst_pos`).
    #[allow(dead_code)] pub pst_pos:     i32,
    /// Gold-key repeat count (mirrors C `gold_count`).
    #[allow(dead_code)] pub gold_count:  i32,
    pub num_of_bufs: i32,
    /// Line-wrap column (mirrors C `line_wrap`).
    #[allow(dead_code)] pub line_wrap:   i32,
    /// Info-window display type (mirrors C `info_type`).
    #[allow(dead_code)] pub info_type:   i32,
    pub local_lines: i32,
    pub local_cols:  i32,

    /// Deleted character (mirrors `d_char`).
    pub d_char: char,
    /// Deleted word (mirrors `d_word`).
    pub d_word: Option<String>,
    /// Deleted line (mirrors `d_line`).
    pub d_line: Option<String>,

    pub tab_stops:     Vec<i32>,
    /// Linked-list of tab-stop columns built from `tab_spacing` during init.
    pub tab_stop_list: Option<Box<TabStop>>,
    /// Current input string being assembled (mirrors C `in_string`).
    #[allow(dead_code)] pub in_string:    String,
    /// Startup command list (mirrors C `commands`).
    #[allow(dead_code)] pub commands:     Vec<String>,
    /// Startup init-string list (mirrors C `init_strings`).
    #[allow(dead_code)] pub init_strings: Vec<String>,

    // ── LSP client ────────────────────────────────────────────────────────────
    pub lsp_client: Option<crate::lsp::LspClient>,

    // ── Simple undo (the C code has a 128-slot circular buffer) ───────────────
    pub last_action: Option<LastAction>,
}

impl EditorState {
    pub fn new() -> Self {
        EditorState {
            first_line:   None,
            curr_line:    None,
            paste_buff:   None,
            dlt_line:     None,
            fpste_line:   None,
            cpste_line:   None,
            pste_tmp:     None,
            tmp_line:     None,
            srch_line:    None,
            first_buff:   None,
            curr_buff:    None,
            windows:      true,
            mark_text:    false,
            journ_on:     true,
            input_file:   false,
            edit:         true,
            gold:         false,
            recover:      false,
            case_sen:     false,
            change:       false,
            literal:      false,
            forward:      true,
            restricted:   false,
            change_dir_allowed: true,
            text_only:    true,
            expand:       false,
            nohighlight:  false,
            echo_flag:    true,
            info_window:  true,
            recv_file:    false,
            overstrike:   false,
            indent:       false,
            auto_format:  false,
            formatted:    false,
            observ_margins: false,
            right_justify:  false,
            status_line:  false,
            ee_mode_menu: false,
            win_height:   0,
            win_width:    0,
            left_margin:  0,
            right_margin: 0,
            tab_spacing:  8,
            info_win_height: 6,  // INFO_WIN_HEIGHT_DEF
            eightbit:     false,
            srch_str:     None,
            u_srch_str:   None,
            old_string:   None,
            u_old_string: None,
            new_string:   None,
            files:        Vec::new(),
            start_at_line: None,
            print_command: "lp".to_string(),
            journal_dir:  String::new(),
            lines_moved:  0,
            d_wrd_len:    0,
            value:        0,
            tmp_pos:      0,
            tmp_vert:     0,
            repl_length:  0,
            pst_pos:      0,
            gold_count:   0,
            num_of_bufs:  0,
            line_wrap:    0,
            info_type:    1, // CONTROL_KEYS
            local_lines:  0,
            local_cols:   0,
            d_char:       '\0',
            d_word:       None,
            d_line:       None,
            tab_stops:     Vec::new(),
            tab_stop_list: None,
            in_string:    String::new(),
            commands:     Vec::new(),
            init_strings: Vec::new(),
            lsp_client:   None,
            last_action:  None,
        }
    }

    /// Parse command-line arguments (mirrors `get_options()` in aee.c).
    pub fn parse_options(&mut self, args: &[String]) {
        if crate::file_ops::ae_basename(&args[0]) == "rae" {
            self.restricted = true;
            self.change_dir_allowed = false;
        }

        let mut count = 1;
        while count < args.len() {
            let arg = &args[count];
            if let Some(buff) = arg.strip_prefix('-') {
                if buff == "text" {
                    self.text_only = true;
                } else if buff == "binary" {
                    self.text_only = false;
                } else if buff == "tab" {
                    self.expand = true;
                } else if let Some(&b) = buff.as_bytes().first() {
                    match b {
                        b'j' => self.journ_on = false,
                        b'r' => self.recover = true,
                        b'e' => self.echo_flag = false,
                        b'i' => self.info_window = false,
                        b'n' => self.nohighlight = true,
                        b'h' => {
                            let num_buff = if buff.len() > 1 { &buff[1..] } else {
                                count += 1;
                                if count < args.len() { &args[count] } else { "" }
                            };
                            self.win_height = num_buff.parse().unwrap_or(0);
                        }
                        b'w' => {
                            let num_buff = if buff.len() > 1 { &buff[1..] } else {
                                count += 1;
                                if count < args.len() { &args[count] } else { "" }
                            };
                            self.win_width = num_buff.parse().unwrap_or(0);
                        }
                        _ => {}
                    }
                }
            } else if let Some(rest) = arg.strip_prefix('+') {
                self.start_at_line = Some(rest.to_string());
            } else {
                self.files.push(arg.clone());
                self.input_file = true;
                self.recv_file = true;
            }
            count += 1;
        }
    }

    /// Initialise the editor state (mirrors the setup section of `main()` in aee.c).
    pub async fn initialize(&mut self) {
        self.eightbit = false;

        // Create first buffer (mirrors buf_alloc + initialisation in aee.c main())
        self.first_buff = Some(crate::buffer::buf_alloc());
        self.curr_buff  = self.first_buff.clone();

        if let Some(ref buff_rc) = self.first_buff {
            let mut buff = buff_rc.borrow_mut();
            buff.name        = MAIN_BUFFER_NAME.to_string();
            buff.first_line  = Some(crate::text::txtalloc());
            buff.curr_line   = buff.first_line.clone();
            buff.main_buffer = true;
            buff.edit_buffer = true;

            if let Some(ref line_rc) = buff.curr_line.clone() {
                let mut line = line_rc.borrow_mut();
                line.line        = String::new();
                line.line_length = 1;   // includes null slot, like C
                line.max_length  = 10;
                line.line_number = 1;
                line.vert_len    = 1;
            }

            buff.num_of_lines = 1;
            buff.absolute_lin = 1;
            buff.position     = 1;
            buff.abs_pos      = 0;
            buff.scr_pos      = 0;
            buff.scr_vert     = 0;
            buff.scr_horz     = 0;
        }

        self.num_of_bufs    = 1;
        self.forward        = true;
        self.windows        = true;

        // Build the tab-stop linked list from tab_spacing (mirrors set_tabs() in aee.c).
        // Columns are multiples of tab_spacing up to column 160.
        let spacing = self.tab_spacing.max(1);
        let mut list: Option<Box<TabStop>> = None;
        let mut col = (160 / spacing) * spacing;
        while col > 0 {
            list = Some(Box::new(TabStop { column: col, next_stop: list }));
            col -= spacing;
        }
        self.tab_stop_list = list;
        // Mirror into the flat Vec<i32> used by other callers.
        self.tab_stops.clear();
        let mut cur = self.tab_stop_list.as_deref();
        while let Some(ts) = cur {
            self.tab_stops.push(ts.column);
            cur = ts.next_stop.as_deref();
        }
    }

    /// Count buffers by walking the `first_buff → next_buff` linked list.
    /// This is the primary reader of `Buffer::next_buff`.
    pub fn buf_count(&self) -> i32 {
        let mut count = 0i32;
        let mut cur = self.first_buff.clone();
        while let Some(b) = cur {
            count += 1;
            cur = b.borrow().next_buff.clone();
        }
        count
    }

    /// Load a file into the main buffer (mirrors `check_fp` + `get_file` in file.c).
    pub fn load_file(&mut self, file_name: &str) {
        // Record the target path on the buffer.
        if let Some(ref buff_rc) = self.first_buff {
            let mut buff = buff_rc.borrow_mut();
            buff.file_name = Some(crate::file_ops::ae_basename(file_name));
            buff.full_name = Some(crate::file_ops::get_full_path(file_name, ""));
        }

        let contents = match crate::file_ops::get_file(file_name) {
            Ok(c)  => c,
            Err(_) => return,
        };

        let lines: Vec<String> = contents.lines().map(|s| s.to_string()).collect();
        if lines.is_empty() { return; }

        if let Some(ref buff_rc) = self.first_buff {
            let mut buff = buff_rc.borrow_mut();

            // Overwrite the initial blank line.
            if let Some(ref line_rc) = buff.first_line {
                let mut line = line_rc.borrow_mut();
                line.line        = lines[0].clone();
                line.line_length = line.line.len() as i32 + 1;
                line.max_length  = line.line_length + 10;
                line.line_number = 1;
            }

            // Append remaining lines as a linked list.
            let mut prev = buff.first_line.clone();
            for (i, l) in lines.iter().enumerate().skip(1) {
                let new_line = crate::text::txtalloc();
                {
                    let mut nl = new_line.borrow_mut();
                    nl.line        = l.clone();
                    nl.line_length = nl.line.len() as i32 + 1;
                    nl.max_length  = nl.line_length + 10;
                    nl.line_number = (i + 1) as i32;
                    nl.prev_line   = prev.clone();
                }
                if let Some(ref p) = prev {
                    p.borrow_mut().next_line = Some(new_line.clone());
                }
                prev = Some(new_line);
            }

            buff.num_of_lines = lines.len() as i32;
            buff.absolute_lin = 1;
            buff.changed      = false;

            // Detect DOS line endings (CR/LF) – mirrors get_line() logic
            if contents.contains("\r\n") {
                buff.dos_file = true;
            }

            // Capture the working directory at load time (mirrors orig_dir in C).
            buff.orig_dir = std::env::current_dir()
                .ok()
                .map(|p| p.to_string_lossy().into_owned());

            // Cache file-stat info so save_file can detect external changes.
            if let Ok(meta) = std::fs::metadata(file_name) {
                buff.fileinfo_mtime = meta.modified()
                    .ok()
                    .and_then(|t| t.duration_since(std::time::UNIX_EPOCH).ok())
                    .map(|d| d.as_secs())
                    .unwrap_or(0);
                buff.fileinfo_size = meta.len();
            }
        }
    }
}
