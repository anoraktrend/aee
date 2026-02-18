#![allow(dead_code)]

/// Motion functions – ported from src/motion.c
///
/// All functions take the current `Buffer` (via the editor's `curr_buff`)
/// and operate on its fields directly, mirroring the C originals.

use crate::editor_state::Buffer;

// ──────────────────────────────────────────────────────────────────────────────
// Helpers
// ──────────────────────────────────────────────────────────────────────────────

/// Clamp `position` to `[1, line_length]`.
fn clamp_pos(pos: i32, line_len: i32) -> i32 {
    pos.max(1).min(line_len.max(1))
}

// ──────────────────────────────────────────────────────────────────────────────
// Line navigation
// ──────────────────────────────────────────────────────────────────────────────

/// Move to the beginning of the current line (bol).
pub fn bol(buff: &mut Buffer) {
    buff.position = 1;
    buff.abs_pos  = 0;
    buff.scr_pos  = 0;
    buff.scr_horz = 0;
    // Vertical: if position was in the middle of a wrapped line, scr_vert
    // should move back to the first screen row of this logical line.
    // For non-wrapping mode (our default) this is a no-op on scr_vert.
}

/// Move to the end of the current line (eol).
pub fn eol(buff: &mut Buffer) {
    if let Some(ref line_rc) = buff.curr_line.clone() {
        let line = line_rc.borrow();
        let len = (line.line_length - 1).max(0); // line_length includes the null slot
        buff.position = line.line_length;
        buff.abs_pos  = len;
        buff.scr_pos  = len;
        buff.scr_horz = len;
    }
}

/// Move cursor left one character.
/// Returns `true` if the move crossed a line boundary (went to end of prev line).
pub fn move_left(buff: &mut Buffer) -> bool {
    if buff.position > 1 {
        buff.position -= 1;
        buff.scr_horz  = (buff.scr_horz - 1).max(0);
        buff.scr_pos   = buff.scr_horz;
        buff.abs_pos   = buff.scr_pos;
        false
    } else {
        // At start of line – go to end of previous line
        let prev = buff.curr_line.as_ref().and_then(|l| l.borrow().prev_line.clone());
        if let Some(prev_rc) = prev {
            buff.curr_line = Some(prev_rc.clone());
            let prev_len = prev_rc.borrow().line_length;
            buff.position    = prev_len;
            buff.scr_horz    = (prev_len - 1).max(0);
            buff.scr_pos     = buff.scr_horz;
            buff.abs_pos     = buff.scr_pos;
            buff.absolute_lin -= 1;
            if buff.scr_vert > 0 {
                buff.scr_vert -= 1;
            } else if buff.window_top > 1 {
                buff.window_top -= 1;
            }
            true
        } else {
            false // already at very start of file
        }
    }
}

/// Move cursor right one character.
/// Returns `true` if a move was made.
pub fn move_right(buff: &mut Buffer) -> bool {
    let line_len = buff.curr_line.as_ref()
        .map(|l| l.borrow().line_length).unwrap_or(1);
    if buff.position < line_len {
        buff.position += 1;
        buff.scr_horz += 1;
        buff.scr_pos   = buff.scr_horz;
        buff.abs_pos   = buff.scr_pos;
        true
    } else {
        // At end of line – go to start of next line
        let next = buff.curr_line.as_ref().and_then(|l| l.borrow().next_line.clone());
        if let Some(next_rc) = next {
            buff.curr_line   = Some(next_rc);
            buff.position    = 1;
            buff.scr_horz    = 0;
            buff.scr_pos     = 0;
            buff.abs_pos     = 0;
            buff.absolute_lin += 1;
            let (_, height) = crate::ui::get_terminal_size();
            let text_height = (height as i32) - 1; // status bar
            if buff.scr_vert < text_height - 1 {
                buff.scr_vert += 1;
            } else {
                buff.window_top += 1;
            }
            true
        } else {
            false
        }
    }
}

/// Move cursor up one line, keeping the same column (up).
pub fn move_up(buff: &mut Buffer) {
    let saved_abs = buff.abs_pos;
    let prev = buff.curr_line.as_ref().and_then(|l| l.borrow().prev_line.clone());
    if let Some(prev_rc) = prev {
        buff.curr_line = Some(prev_rc.clone());
        buff.absolute_lin -= 1;
        if buff.scr_vert > 0 {
            buff.scr_vert -= 1;
        } else if buff.window_top > 1 {
            buff.window_top -= 1;
        }
        // find_pos: restore column position
        let line_len = prev_rc.borrow().line_length;
        buff.position = clamp_pos(saved_abs + 1, line_len);
        buff.scr_horz = (buff.position - 1).max(0);
        buff.scr_pos  = buff.scr_horz;
        buff.abs_pos  = saved_abs;
    }
}

/// Move cursor down one line, keeping the same column (down).
pub fn move_down(buff: &mut Buffer) {
    let saved_abs = buff.abs_pos;
    let next = buff.curr_line.as_ref().and_then(|l| l.borrow().next_line.clone());
    if let Some(next_rc) = next {
        buff.curr_line = Some(next_rc.clone());
        buff.absolute_lin += 1;
        let (_, height) = crate::ui::get_terminal_size();
        let text_height = (height as i32) - 1;
        if buff.scr_vert < text_height - 1 {
            buff.scr_vert += 1;
        } else {
            buff.window_top += 1;
        }
        let line_len = next_rc.borrow().line_length;
        buff.position = clamp_pos(saved_abs + 1, line_len);
        buff.scr_horz = (buff.position - 1).max(0);
        buff.scr_pos  = buff.scr_horz;
        buff.abs_pos  = saved_abs;
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// Word navigation  (adv_word / prev_word from motion.c)
// ──────────────────────────────────────────────────────────────────────────────

/// Advance to the start of the next word (adv_word).
pub fn adv_word(buff: &mut Buffer) {
    // Skip non-whitespace, then skip whitespace
    loop {
        let (pos, len, at_ws) = {
            let line_rc = match buff.curr_line.as_ref() { Some(l) => l.clone(), None => break };
            let line = line_rc.borrow();
            let pos = buff.position as usize;
            if pos > line.line.len() { break; }
            let ch = line.line.chars().nth(pos.saturating_sub(1)).unwrap_or(' ');
            (buff.position, line.line_length, ch.is_whitespace())
        };
        if pos < len {
            if at_ws {
                // skip whitespace
                loop {
                    let done = {
                        let line_rc = match buff.curr_line.as_ref() { Some(l) => l.clone(), None => break };
                        let line = line_rc.borrow();
                        let p = buff.position as usize;
                        if p >= line.line.len() { true }
                        else {
                            let ch = line.line.chars().nth(p.saturating_sub(1)).unwrap_or('x');
                            !ch.is_whitespace()
                        }
                    };
                    if done { break; }
                    move_right(buff);
                }
                break;
            } else {
                // skip non-whitespace first
                loop {
                    let done = {
                        let line_rc = match buff.curr_line.as_ref() { Some(l) => l.clone(), None => break };
                        let line = line_rc.borrow();
                        let p = buff.position as usize;
                        if p >= line.line.len() { true }
                        else {
                            let ch = line.line.chars().nth(p.saturating_sub(1)).unwrap_or(' ');
                            ch.is_whitespace()
                        }
                    };
                    if done { break; }
                    move_right(buff);
                }
                // then skip whitespace
                loop {
                    let done = {
                        let line_rc = match buff.curr_line.as_ref() { Some(l) => l.clone(), None => break };
                        let line = line_rc.borrow();
                        let p = buff.position as usize;
                        if p >= line.line.len() { true }
                        else {
                            let ch = line.line.chars().nth(p.saturating_sub(1)).unwrap_or('x');
                            !ch.is_whitespace()
                        }
                    };
                    if done { break; }
                    move_right(buff);
                }
                break;
            }
        } else {
            move_right(buff); // go to next line
            break;
        }
    }
}

/// Move to start of previous word (prev_word).
pub fn prev_word(buff: &mut Buffer) {
    if buff.position > 1 {
        // If at start of a word, skip back over whitespace first
        let at_ws = {
            let line_rc = match buff.curr_line.as_ref() { Some(l) => l.clone(), None => return };
            let line = line_rc.borrow();
            let p = (buff.position - 2) as usize; // char before cursor
            line.line.chars().nth(p).map(|c| c.is_whitespace()).unwrap_or(false)
        };
        if at_ws {
            while buff.position > 1 {
                let ws = {
                    let line_rc = match buff.curr_line.as_ref() { Some(l) => l.clone(), None => break };
                    let line = line_rc.borrow();
                    let p = (buff.position - 2) as usize;
                    line.line.chars().nth(p).map(|c| c.is_whitespace()).unwrap_or(false)
                };
                if !ws { break; }
                move_left(buff);
            }
        }
        // skip non-whitespace back
        while buff.position > 1 {
            let ws = {
                let line_rc = match buff.curr_line.as_ref() { Some(l) => l.clone(), None => break };
                let line = line_rc.borrow();
                let p = (buff.position - 2) as usize;
                line.line.chars().nth(p).map(|c| c.is_whitespace()).unwrap_or(true)
            };
            if ws { break; }
            move_left(buff);
        }
    } else {
        move_left(buff);
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// File-level navigation (top / bottom from motion.c)
// ──────────────────────────────────────────────────────────────────────────────

/// Go to the top of the file.
pub fn top(buff: &mut Buffer) {
    // Walk to first line
    while buff.curr_line.as_ref().and_then(|l| l.borrow().prev_line.clone()).is_some() {
        let prev = buff.curr_line.as_ref().unwrap().borrow().prev_line.clone().unwrap();
        buff.curr_line = Some(prev);
    }
    buff.position    = 1;
    buff.abs_pos     = 0;
    buff.scr_pos     = 0;
    buff.scr_horz    = 0;
    buff.scr_vert    = 0;
    buff.window_top  = 1;
    buff.absolute_lin = 1;
}

/// Go to the bottom of the file.
pub fn bottom(buff: &mut Buffer) {
    let (_, height) = crate::ui::get_terminal_size();
    let text_height = (height as i32) - 1;
    while let Some(next) = buff.curr_line.as_ref().and_then(|l| l.borrow().next_line.clone()) {
        buff.curr_line = Some(next);
        buff.absolute_lin += 1;
    }
    // position at end of last line
    let line_len = buff.curr_line.as_ref().map(|l| l.borrow().line_length).unwrap_or(1);
    buff.position = line_len;
    buff.scr_horz = (line_len - 1).max(0);
    buff.scr_pos  = buff.scr_horz;
    buff.abs_pos  = buff.scr_pos;
    // Set scr_vert to last visible row
    buff.scr_vert = (text_height - 1).max(0);
    // Adjust window_top so the last line is visible
    buff.window_top = (buff.absolute_lin - buff.scr_vert).max(1);
}

// ──────────────────────────────────────────────────────────────────────────────
// Page navigation (next_page / prev_page from motion.c)
// ──────────────────────────────────────────────────────────────────────────────

/// Move forward one screen page.
pub fn next_page(buff: &mut Buffer) {
    let (_, height) = crate::ui::get_terminal_size();
    let page = (height as i32) - 2; // text rows per page
    let mut lines = 0;
    while lines < page {
        let has_next = buff.curr_line.as_ref()
            .and_then(|l| l.borrow().next_line.clone()).is_some();
        if !has_next { break; }
        let next = buff.curr_line.as_ref().unwrap().borrow().next_line.clone().unwrap();
        buff.curr_line = Some(next);
        buff.absolute_lin += 1;
        lines += 1;
    }
    buff.position = 1;
    buff.scr_horz = 0;
    buff.scr_pos  = 0;
    buff.abs_pos  = 0;
    buff.window_top = (buff.absolute_lin - buff.scr_vert).max(1);
}

/// Move back one screen page.
pub fn prev_page(buff: &mut Buffer) {
    let (_, height) = crate::ui::get_terminal_size();
    let page = (height as i32) - 2;
    let mut lines = 0;
    while lines < page {
        let has_prev = buff.curr_line.as_ref()
            .and_then(|l| l.borrow().prev_line.clone()).is_some();
        if !has_prev { break; }
        let prev = buff.curr_line.as_ref().unwrap().borrow().prev_line.clone().unwrap();
        buff.curr_line = Some(prev);
        buff.absolute_lin -= 1;
        lines += 1;
    }
    buff.position = 1;
    buff.scr_horz = 0;
    buff.scr_pos  = 0;
    buff.abs_pos  = 0;
    buff.window_top = (buff.absolute_lin - buff.scr_vert).max(1);
}

// ──────────────────────────────────────────────────────────────────────────────
// Goto line (command function from control.c / aee.c)
// ──────────────────────────────────────────────────────────────────────────────

/// Jump to an absolute line number.
pub fn goto_line(buff: &mut Buffer, target: i32) {
    if target <= buff.absolute_lin {
        // Walk backwards
        while buff.absolute_lin > target {
            let has_prev = buff.curr_line.as_ref()
                .and_then(|l| l.borrow().prev_line.clone()).is_some();
            if !has_prev { break; }
            let prev = buff.curr_line.as_ref().unwrap().borrow().prev_line.clone().unwrap();
            buff.curr_line = Some(prev);
            buff.absolute_lin -= 1;
        }
    } else {
        // Walk forwards
        while buff.absolute_lin < target {
            let has_next = buff.curr_line.as_ref()
                .and_then(|l| l.borrow().next_line.clone()).is_some();
            if !has_next { break; }
            let next = buff.curr_line.as_ref().unwrap().borrow().next_line.clone().unwrap();
            buff.curr_line = Some(next);
            buff.absolute_lin += 1;
        }
    }
    buff.position = 1;
    buff.scr_horz = 0;
    buff.scr_pos  = 0;
    buff.abs_pos  = 0;
    // Recalculate scr_vert / window_top
    let (_, height) = crate::ui::get_terminal_size();
    let text_height = (height as i32) - 1;
    let half = text_height / 2;
    buff.scr_vert   = half.min(buff.absolute_lin - 1);
    buff.window_top = (buff.absolute_lin - buff.scr_vert).max(1);
}
