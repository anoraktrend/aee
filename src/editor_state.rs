// Editor state and core data structures for aee

use std::rc::Rc;
use std::cell::RefCell;

// Main buffer name
const MAIN_BUFFER_NAME: &str = "main";

// File info for journal
#[derive(Clone)]
pub struct AeFileInfo {
    pub info_location: u64,
    pub prev_info: u64,
    pub next_info: u64,
    pub line_location: u64,
}


// Text line structure
#[derive(Clone)]
pub struct TextLine {
    pub line: String,
    pub line_number: i32,
    pub max_length: i32,
    pub vert_len: i32,
    pub file_info: AeFileInfo,
    pub changed: bool,
    pub prev_line: Option<Rc<RefCell<TextLine>>>,
    pub next_line: Option<Rc<RefCell<TextLine>>>,
    pub line_length: i32,
}

// Buffer structure
pub struct Buffer {
    pub name: String,
    pub first_line: Option<Rc<RefCell<TextLine>>>,
    pub curr_line: Option<Rc<RefCell<TextLine>>>,
    pub scr_vert: i32,
    pub scr_horz: i32,
    pub scr_pos: i32,
    pub position: i32,
    pub abs_pos: i32,
    pub lines: i32,
    pub last_line: i32,
    pub last_col: i32,
    pub num_of_lines: i32,
    pub absolute_lin: i32,
    pub window_top: i32,
    pub file_name: Option<String>,
    pub full_name: Option<String>,
    pub changed: bool,
    pub main_buffer: bool,
    pub edit_buffer: bool,
}

// Undo action
#[derive(Clone)]
pub enum LastAction {
    InsertChar { line: Rc<RefCell<TextLine>>, pos: usize },
    DeleteChar { line: Rc<RefCell<TextLine>>, pos: usize, ch: char },
}

// Main editor state
pub struct EditorState {
    // Text handling
    pub first_line: Option<Rc<RefCell<TextLine>>>,
    pub curr_line: Option<Rc<RefCell<TextLine>>>,
    pub paste_buff: Option<Rc<RefCell<TextLine>>>,
    pub dlt_line: Option<Rc<RefCell<TextLine>>>,
    pub fpste_line: Option<Rc<RefCell<TextLine>>>,
    pub cpste_line: Option<Rc<RefCell<TextLine>>>,
    pub pste_tmp: Option<Rc<RefCell<TextLine>>>,
    pub tmp_line: Option<Rc<RefCell<TextLine>>>,
    pub srch_line: Option<Rc<RefCell<TextLine>>>,

    // Buffers
    pub first_buff: Option<Rc<RefCell<Buffer>>>,
    pub curr_buff: Option<Rc<RefCell<Buffer>>>,

    // Flags
    pub windows: bool,
    pub mark_text: bool,
    pub journ_on: bool,
    pub input_file: bool,
    pub edit: bool,
    pub gold: bool,
    pub recover: bool,
    pub case_sen: bool,
    pub change: bool,
    pub literal: bool,
    pub forward: bool,

    // Additional flags
    pub restricted: bool,
    pub change_dir_allowed: bool,
    pub text_only: bool,
    pub expand: bool,
    pub nohighlight: bool,
    pub echo_flag: bool,
    pub info_window: bool,
    pub recv_file: bool,
    pub win_height: i32,
    pub win_width: i32,

    // Settings
    pub left_margin: i32,
    pub right_margin: i32,
    pub tab_spacing: i32,
    pub info_win_height: i32,

    // Other state
    pub srch_str: Option<String>,
    pub u_srch_str: Option<String>,
    pub old_string: Option<String>,
    pub new_string: Option<String>,

    pub files: Vec<String>,

    // LSP client
    pub lsp_client: Option<crate::lsp::LspClient>,

    // Additional fields from globals
    pub start_at_line: Option<String>,
    pub lines_moved: i32,
    pub d_wrd_len: i32,
    pub value: i32,
    pub tmp_pos: i32,
    pub tmp_vert: i32,
    pub repl_length: i32,
    pub pst_pos: i32,
    pub gold_count: i32,
    pub num_of_bufs: i32,
    pub line_wrap: i32,
    pub info_type: i32,
    pub local_lines: i32,
    pub local_cols: i32,
    pub tab_stops: Vec<i32>, // simplified
    pub in_string: String,
    pub commands: Vec<String>,
    pub init_strings: Vec<String>,
    pub eightbit: bool,

    // Undo
    pub last_action: Option<LastAction>,
}

impl EditorState {
    pub fn new() -> Self {
        EditorState {
            first_line: None,
            curr_line: None,
            paste_buff: None,
            dlt_line: None,
            fpste_line: None,
            cpste_line: None,
            pste_tmp: None,
            tmp_line: None,
            srch_line: None,
            first_buff: None,
            curr_buff: None,
            windows: false,
            mark_text: false,
            journ_on: true,
            input_file: false,
            edit: true,
            gold: false,
            recover: false,
            case_sen: false,
            change: false,
            literal: false,
            forward: true,
            restricted: false,
            change_dir_allowed: true,
            text_only: true,
            expand: false,
            nohighlight: false,
            echo_flag: true,
            info_window: true,
            recv_file: false,
            win_height: 0,
            win_width: 0,
            left_margin: 0,
            right_margin: 0,
            tab_spacing: 8,
            info_win_height: 6,
            srch_str: None,
            u_srch_str: None,
            old_string: None,
            new_string: None,
            files: Vec::new(),
            lsp_client: None,
            start_at_line: None,
            lines_moved: 0,
            d_wrd_len: 0,
            value: 0,
            tmp_pos: 0,
            tmp_vert: 0,
            repl_length: 0,
            pst_pos: 0,
            gold_count: 0,
            num_of_bufs: 0,
            line_wrap: 0,
            info_type: 1,
            local_lines: 0,
            local_cols: 0,
            tab_stops: Vec::new(),
            in_string: String::new(),
            commands: Vec::new(),
            init_strings: Vec::new(),
            eightbit: false,
            last_action: None,
        }
    }

    pub fn parse_options(&mut self, args: &[String]) {
        if crate::file_ops::ae_basename(&args[0]) == "rae" {
            self.restricted = true;
            self.change_dir_allowed = false;
        }

        let mut count = 1;
        while count < args.len() {
            let arg = &args[count];
            if arg.starts_with('-') {
                let buff = &arg[1..];
                if let Some(&b) = buff.as_bytes().get(0) {
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
                if arg == "-text" {
                    self.text_only = true;
                } else if arg == "-binary" {
                    self.text_only = false;
                } else if arg == "-tab" {
                    self.expand = true;
                }
            } else if arg.starts_with('+') {
                self.start_at_line = Some(arg[1..].to_string());
            } else {
                self.files.push(arg.clone());
                self.input_file = true;
                self.recv_file = true;
            }
            count += 1;
        }
    }

    pub async fn initialize(&mut self) {
        // Initialize basic settings
        self.eightbit = false;

        // Initialize strings (placeholder)
        // strings_init();

        // Create first buffer
        self.first_buff = Some(crate::buffer::buf_alloc());
        self.curr_buff = self.first_buff.clone();

        if let Some(ref mut buff) = self.first_buff {
            let mut buff = buff.borrow_mut();
            buff.name = MAIN_BUFFER_NAME.to_string();
            buff.first_line = Some(crate::text::txtalloc());
            buff.curr_line = buff.first_line.clone();
            buff.main_buffer = true;
            buff.edit_buffer = true;

            // Set up the line
            if let Some(ref line) = buff.curr_line {
                let mut line = line.borrow_mut();
                line.line = String::from(""); // initial empty line
                line.line_length = 1;
                line.max_length = 10;
                line.line_number = 1;
                line.vert_len = 1;
            }

            buff.num_of_lines = 1;
            buff.absolute_lin = 1;
            buff.position = 1;
            buff.abs_pos = 0;
            buff.scr_pos = 0;
            buff.scr_vert = 0;
            buff.scr_horz = 0;
        }

        self.num_of_bufs = 1;

        // Set default values
        self.edit = true;
        self.journ_on = true;
        self.input_file = false;
        self.recv_file = false;
        self.forward = true;
        self.literal = false;
        self.case_sen = false;
        self.change = false;
        self.mark_text = false;
        self.gold = false;
        self.recover = false;
        self.restricted = false;
        self.change_dir_allowed = true;
        self.text_only = true;
        self.expand = false;
        self.nohighlight = false;
        self.echo_flag = true;
        self.info_window = true;
        self.win_height = 0;
        self.win_width = 0;
        self.left_margin = 0;
        self.right_margin = 0;
        self.tab_spacing = 8;
        self.info_win_height = 6;

        // Initialize undo buffers (placeholder)
        // undel_init();

        // Initialize keys (placeholder)
        // init_keys();

        // Read init files (placeholder)
        // ae_init();

        // Set up terminal is already done in main.rs

        // Get key assignments (placeholder)
        // get_key_assgn();

        // Create command window
        // self.com_win = Some(crate::ui::make_com_win());

        // Set up buffer window
        if let Some(ref mut buff) = self.curr_buff {
            let _buff = buff.borrow_mut();
            // if self.info_window {
            //     self.info_win = Some(crate::ui::make_info_win(self.info_win_height));
            //     buff.win = Some(newwin(LINES() - self.info_win_height - 1, COLS(), self.info_win_height, 0));
            //     buff.lines = LINES() - self.info_win_height - 1;
            //     buff.last_line = buff.lines - 1;
            // } else {
            //     buff.win = Some(newwin(LINES() - 1, COLS(), 0, 0));
            //     buff.lines = LINES() - 1;
            //     buff.last_line = LINES() - 2;
            // }
            // buff.last_col = COLS() - 1;
            // buff.window_top = if self.info_window { self.info_win_height } else { 0 };
            // if let Some(win) = buff.win {
            //     keypad(win, true);
            //     nodelay(win, false);
            //     idlok(win, true);
            // }
        }

        // If file specified, load it (placeholder)
        if !self.files.is_empty() {
            // TODO: Load file
        }

        // Set right margin
        if self.right_margin == 0 {
            // self.right_margin = COLS() - 1;
        }

        // Display copyright
        // if let Some(ref mut buff) = self.curr_buff {
        //     if let Some(win) = buff.borrow().win {
        //         wmove(win, buff.borrow().last_line, 0);
        //         wstandout(win);
        //         waddstr(win, "aee, copyright notice placeholder");
        //         wstandend(win);
        //         wmove(win, buff.borrow().scr_vert, buff.borrow().scr_horz);
        //         wrefresh(win);
        //     }
        // }
    }

    /// Load `file_name` into the main buffer.
    ///
    /// * If the file exists its contents replace the initial empty line.
    /// * If the file does **not** exist (or is empty) the buffer is left with
    ///   one blank line so the user can start typing immediately – the name is
    ///   still recorded so that Ctrl+S saves to the right path.
    pub fn load_file(&mut self, file_name: &str) {
        // Always record the target file name on the buffer so Ctrl+S knows
        // where to write, even when opening a brand-new (not-yet-existing) file.
        if let Some(ref buff_rc) = self.first_buff {
            let mut buff = buff_rc.borrow_mut();
            buff.file_name = Some(file_name.to_string());
            buff.full_name = Some(crate::file_ops::get_full_path(file_name, ""));
        }

        // Attempt to read the file; on any error (file not found, permission
        // denied, …) we simply keep the already-initialised empty buffer.
        let contents = match crate::file_ops::get_file(file_name) {
            Ok(c) => c,
            Err(_) => return, // new / unreadable file – empty buffer is fine
        };

        let lines: Vec<String> = contents.lines().map(|s| s.to_string()).collect();

        // Empty file on disk → keep the blank buffer line, nothing more to do.
        if lines.is_empty() {
            return;
        }

        if let Some(ref buff_rc) = self.first_buff {
            let mut buff = buff_rc.borrow_mut();

            // Overwrite the initial blank line with the first line of the file.
            if let Some(ref line_rc) = buff.first_line {
                let mut line = line_rc.borrow_mut();
                line.line = lines[0].clone();
                line.line_length = line.line.len() as i32;
                line.max_length = line.line_length + 10;
                line.line_number = 1;
            }

            // Append remaining lines as a linked list.
            let mut prev = buff.first_line.clone();
            for (i, l) in lines.iter().enumerate().skip(1) {
                let new_line = crate::text::txtalloc();
                {
                    let mut nl = new_line.borrow_mut();
                    nl.line = l.clone();
                    nl.line_length = nl.line.len() as i32;
                    nl.max_length = nl.line_length + 10;
                    nl.line_number = (i + 1) as i32;
                    nl.prev_line = prev.clone();
                }
                if let Some(ref p) = prev {
                    p.borrow_mut().next_line = Some(new_line.clone());
                }
                prev = Some(new_line);
            }

            buff.num_of_lines = lines.len() as i32;
            buff.absolute_lin = 1;
            buff.changed = false;
        }
    }

    pub fn use_unused_fields(&self) {
        // Dummy function to suppress unused field warnings
        let _ = &self.first_line;
        let _ = &self.curr_line;
        let _ = &self.paste_buff;
        let _ = &self.dlt_line;
        let _ = &self.fpste_line;
        let _ = &self.cpste_line;
        let _ = &self.pste_tmp;
        let _ = &self.tmp_line;
        let _ = &self.srch_line;
        let _ = &self.first_buff;
        let _ = &self.curr_buff;
        let _ = self.windows;
        let _ = self.mark_text;
        let _ = self.journ_on;
        let _ = self.input_file;
        let _ = self.edit;
        let _ = self.gold;
        let _ = self.recover;
        let _ = self.case_sen;
        let _ = self.change;
        let _ = self.literal;
        let _ = self.forward;
        let _ = self.restricted;
        let _ = self.change_dir_allowed;
        let _ = self.text_only;
        let _ = self.expand;
        let _ = self.nohighlight;
        let _ = self.echo_flag;
        let _ = self.info_window;
        let _ = self.recv_file;
        let _ = self.win_height;
        let _ = self.win_width;
        let _ = self.left_margin;
        let _ = self.right_margin;
        let _ = self.tab_spacing;
        let _ = self.info_win_height;
        let _ = &self.srch_str;
        let _ = &self.u_srch_str;
        let _ = &self.old_string;
        let _ = &self.new_string;
        let _ = &self.files;
        let _ = &self.lsp_client;
        let _ = &self.start_at_line;
        let _ = self.lines_moved;
        let _ = self.d_wrd_len;
        let _ = self.value;
        let _ = self.tmp_pos;
        let _ = self.tmp_vert;
        let _ = self.repl_length;
        let _ = self.pst_pos;
        let _ = self.gold_count;
        let _ = self.num_of_bufs;
        let _ = self.line_wrap;
        let _ = self.info_type;
        let _ = self.local_lines;
        let _ = self.local_cols;
        let _ = &self.tab_stops;
        let _ = &self.in_string;
        let _ = &self.commands;
        let _ = &self.init_strings;
        let _ = self.eightbit;
    }
}
