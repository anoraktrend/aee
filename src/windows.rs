//! Multi-buffer window management – ported from src/windows.c
//!
//! In the crossterm port there is no ncurses window hierarchy.  Each
//! `Buffer` in the linked list gets a rectangular slice of the terminal
//! screen allocated by `redo_win`.  `new_screen` repaints all of them.
//! A single-buffer build still works identically to before.
//!
//! The functions below are kept for C compatibility and future multi-buffer support.

#![allow(dead_code)]

use crossterm::{
    cursor, execute, queue,
    terminal::{self, Clear, ClearType},
    style::{Print, SetAttribute, Attribute},
};
use std::io::{self, Write};

use crate::editor_state::Buffer;

// ──────────────────────────────────────────────────────────────────────────────
// Screen layout constants
// ──────────────────────────────────────────────────────────────────────────────

/// Minimum number of text rows a buffer window must have.
const MIN_WIN_LINES: usize = 3;

// ──────────────────────────────────────────────────────────────────────────────
// WindowLayout – computed per-buffer geometry
// ──────────────────────────────────────────────────────────────────────────────

/// Geometry for one buffer's on-screen region.
#[derive(Clone, Debug)]
pub struct WindowLayout {
    /// First terminal row of the text area.
    pub top: u16,
    /// Number of text rows (excluding the footer/status bar).
    pub lines: u16,
    /// Terminal columns.
    pub cols: u16,
    /// Whether this buffer currently has focus.
    pub focused: bool,
}

// ──────────────────────────────────────────────────────────────────────────────
// compute_layouts – divide the terminal between all buffers
// ──────────────────────────────────────────────────────────────────────────────

/// Compute on-screen geometry for `num_buffers` buffers.
/// Returns one `WindowLayout` per buffer (in order).
/// `total_rows` is the usable terminal height (excluding the command line).
pub fn compute_layouts(
    num_buffers: usize,
    total_rows: u16,
    total_cols: u16,
    info_win_height: u16,
) -> Vec<WindowLayout> {
    if num_buffers == 0 {
        return vec![];
    }
    let usable = total_rows.saturating_sub(info_win_height) as usize;
    let text_rows_each = (usable / num_buffers).max(MIN_WIN_LINES) as u16;
    let mut layouts = Vec::with_capacity(num_buffers);
    let mut row = info_win_height;
    for i in 0..num_buffers {
        // Last buffer gets any remainder
        let h = if i == num_buffers - 1 {
            total_rows.saturating_sub(row).saturating_sub(1) // -1 for command line
        } else {
            text_rows_each.saturating_sub(1) // -1 for footer line
        };
        layouts.push(WindowLayout {
            top: row,
            lines: h,
            cols: total_cols,
            focused: false,
        });
        row += h + 1; // +1 for footer
    }
    layouts
}

// ──────────────────────────────────────────────────────────────────────────────
// new_screen – repaint all buffers
// ──────────────────────────────────────────────────────────────────────────────

/// Redraw the entire screen.  `buffers` is the list of all Buffer objects in
/// order; `focused_idx` is the index of the currently active buffer.
pub fn new_screen(buffers: &[&mut Buffer], focused_idx: usize, info_win_height: u16) {
    let (cols, rows) = terminal::size().unwrap_or((80, 24));
    // The bottom row is the command line; subtract it.
    let text_rows = rows.saturating_sub(1);
    let layouts = compute_layouts(buffers.len(), text_rows, cols, info_win_height);

    let mut stdout = io::stdout();
    let _ = execute!(stdout, Clear(ClearType::All));

    for (i, (buf, layout)) in buffers.iter().zip(layouts.iter()).enumerate() {
        let is_focused = i == focused_idx;
        paint_buffer_region(buf, layout, is_focused, &mut stdout);
    }

    let _ = stdout.flush();
}

// ──────────────────────────────────────────────────────────────────────────────
// paint_buffer_region – draw one buffer into its layout rectangle
// ──────────────────────────────────────────────────────────────────────────────

fn paint_buffer_region(
    buff: &Buffer,
    layout: &WindowLayout,
    focused: bool,
    stdout: &mut io::Stdout,
) {
    // Paint text lines
    let mut line_rc_opt = buff.first_line.clone();
    // Advance to the scroll offset (window_top line)
    for _ in 0..buff.window_top {
        line_rc_opt = line_rc_opt
            .and_then(|l| l.borrow().next_line.clone());
    }

    for row in 0..layout.lines {
        let _ = execute!(stdout, cursor::MoveTo(layout.top + row, 0));
        let _ = queue!(stdout, Clear(ClearType::CurrentLine));
        if let Some(ref line_rc) = line_rc_opt.clone() {
            let text = line_rc.borrow().line.clone();
            let display: String = text.chars().take(layout.cols as usize).collect();
            let _ = queue!(stdout, Print(&display));
            line_rc_opt = line_rc.borrow().next_line.clone();
        }
    }

    // Paint footer / status bar
    paint_footer(buff, layout, focused, stdout);
}

fn paint_footer(
    buff: &Buffer,
    layout: &WindowLayout,
    focused: bool,
    stdout: &mut io::Stdout,
) {
    let footer_row = layout.top + layout.lines;
    let _ = execute!(
        stdout,
        cursor::MoveTo(footer_row, 0),
        SetAttribute(Attribute::Reverse),
    );

    let change_char = if buff.changed { '*' } else { ' ' };
    let focus_char  = if focused { '*' } else { ' ' };
    let name = buff.file_name.as_deref().unwrap_or(buff.name.as_str());

    let status = format!("{}{} {} ", change_char, focus_char, name);
    let padded = format!("{:<width$}", status, width = layout.cols as usize);
    let truncated: String = padded.chars().take(layout.cols as usize).collect();

    let _ = queue!(stdout, Print(&truncated));
    let _ = execute!(stdout, SetAttribute(Attribute::Reset));
}

// ──────────────────────────────────────────────────────────────────────────────
// add_buf – allocate a new buffer and add it to the list
// ──────────────────────────────────────────────────────────────────────────────

/// Allocate a fresh `Buffer` for editing `ident` (buffer name/identifier).
/// Returns the newly-created buffer.
pub fn add_buf(ident: &str) -> Buffer {
    use crate::text::txtalloc;
    use crate::editor_state::Buffer;

    let first_line = txtalloc();
    {
        let mut l = first_line.borrow_mut();
        l.line = String::new();
        l.line_length = 1;
        l.max_length = 10;
        l.line_number = 1;
        l.vert_len = 1;
    }

    let (cols, rows) = terminal::size().unwrap_or((80, 24));
    Buffer {
        name: ident.to_string(),
        file_name: None,
        full_name: None,
        orig_dir: None,
        next_buff: None,
        first_line: Some(first_line.clone()),
        curr_line: Some(first_line),
        position: 1,
        abs_pos: 0,
        scr_pos: 0,
        scr_horz: 0,
        scr_vert: 0,
        window_top: 0,
        num_of_lines: 1,
        absolute_lin: 1,
        changed: false,
        edit_buffer: true,
        lines: rows as i32 - 1,
        last_line: rows as i32 - 2,
        last_col: cols as i32 - 1,
        main_buffer: false,
        dos_file: false,
        journalling: false,
        journal_file: None,
        fileinfo_mtime: 0,
        fileinfo_size: 0,
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// resize_check – handle terminal resize
// ──────────────────────────────────────────────────────────────────────────────

/// Check whether the terminal has been resized and update `buff` geometry if so.
/// Returns `true` if a resize was detected.
pub fn resize_check(buff: &mut Buffer, last_cols: u16, last_rows: u16) -> bool {
    let (cols, rows) = terminal::size().unwrap_or((80, 24));
    if cols == last_cols && rows == last_rows {
        return false;
    }
    buff.lines    = rows as i32 - 1;
    buff.last_line = buff.lines - 1;
    buff.last_col  = cols as i32 - 1;
    true
}

// ──────────────────────────────────────────────────────────────────────────────
// set_up_term – terminal initialisation (mirrors set_up_term in windows.c)
// ──────────────────────────────────────────────────────────────────────────────

/// Initialise the terminal for raw-mode editing.
pub fn set_up_term() -> io::Result<()> {
    terminal::enable_raw_mode()?;
    execute!(
        io::stdout(),
        terminal::EnterAlternateScreen,
        cursor::Hide,
        Clear(ClearType::All),
    )?;
    Ok(())
}

/// Restore the terminal to its previous state.
pub fn restore_term() -> io::Result<()> {
    execute!(
        io::stdout(),
        cursor::Show,
        terminal::LeaveAlternateScreen,
    )?;
    terminal::disable_raw_mode()?;
    Ok(())
}

// ──────────────────────────────────────────────────────────────────────────────
// paint_info_win – draw the information/status bar at the top
// ──────────────────────────────────────────────────────────────────────────────

/// Draw the optional top info window that shows buffer name, position, etc.
pub fn paint_info_win(buff: &Buffer, height: u16) {
    use crossterm::style::Print;
    let (cols, _) = terminal::size().unwrap_or((80, 24));
    let mut stdout = io::stdout();

    let name = buff.file_name.as_deref().unwrap_or(buff.name.as_str());
    let change = if buff.changed { "modified" } else { "         " };
    let info = format!(
        " aee  {}  {}  ln {}  col {}",
        name, change, buff.absolute_lin, buff.scr_horz + 1
    );
    let padded: String = format!("{:<width$}", info, width = cols as usize)
        .chars()
        .take(cols as usize)
        .collect();

    for row in 0..height {
        let _ = execute!(
            stdout,
            cursor::MoveTo(row, 0),
            SetAttribute(Attribute::Reverse),
            Print(&padded),
            SetAttribute(Attribute::Reset),
        );
    }
    let _ = stdout.flush();
}
