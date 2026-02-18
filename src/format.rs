#![allow(dead_code)]

/// Paragraph formatting – ported from src/format.c
///
/// Implements `Format` (manual paragraph reflow) and `Auto_Format`
/// (automatic word-wrap as you type).  All operations work on the
/// `Buffer` data model; the draw loop in main.rs handles rendering.

use crate::editor_state::Buffer;
use crate::motion;
use crate::delete_ops;

// ──────────────────────────────────────────────────────────────────────────────
// Helpers
// ──────────────────────────────────────────────────────────────────────────────

/// Returns `true` if the line is blank (all whitespace), `None` (no line),
/// or starts with '.' or '>' (troff/nroff / mail-quote markers).
pub fn blank_line(line: Option<&crate::editor_state::TextLine>) -> bool {
    match line {
        None => true,
        Some(l) => {
            let s = &l.line;
            if s.starts_with('.') || s.starts_with('>') {
                return true;
            }
            s.chars().all(|c| c == ' ' || c == '\t')
        }
    }
}

/// Returns `true` if the previous line of `buff` is blank.
fn prev_line_blank(buff: &Buffer) -> bool {
    let prev = buff.curr_line.as_ref()
        .and_then(|l| l.borrow().prev_line.clone());
    match prev {
        None => true,
        Some(p) => {
            let line = p.borrow();
            blank_line(Some(&*line))
        }
    }
}

/// Returns `true` if the next line of `buff` is blank.
fn next_line_blank(buff: &Buffer) -> bool {
    let next = buff.curr_line.as_ref()
        .and_then(|l| l.borrow().next_line.clone());
    match next {
        None => true,
        Some(n) => {
            let line = n.borrow();
            blank_line(Some(&*line))
        }
    }
}

/// Returns `true` if the current line of `buff` is blank.
fn curr_line_blank(buff: &Buffer) -> bool {
    match buff.curr_line.as_ref() {
        None => true,
        Some(l) => {
            let line = l.borrow();
            blank_line(Some(&*line))
        }
    }
}

/// Length (in chars) of the first word + trailing whitespace on a line.
fn first_word_len(buff: &Buffer) -> usize {
    let next = buff.curr_line.as_ref()
        .and_then(|l| l.borrow().next_line.clone());
    let next = match next { Some(n) => n, None => return 0 };
    let line = next.borrow();
    let s = &line.line;
    if s.is_empty() || s.starts_with('.') || s.starts_with('>') {
        return 0;
    }
    let trimmed = s.trim_start();
    if trimmed.is_empty() { return 0; }
    let mut count = 0usize;
    let mut in_word = true;
    for ch in trimmed.chars() {
        if in_word {
            if ch == ' ' || ch == '\t' { in_word = false; count += 1; }
            else { count += 1; }
        } else {
            if ch == ' ' || ch == '\t' { count += 1; }
            else { break; }
        }
    }
    count
}

// ──────────────────────────────────────────────────────────────────────────────
// Internal line manipulations
// ──────────────────────────────────────────────────────────────────────────────

/// Move to start of current line.
fn bol(buff: &mut Buffer) {
    motion::bol(buff);
}

/// Move to end of current line.
fn eol(buff: &mut Buffer) {
    motion::eol(buff);
}

/// Move one step left (wrapper).
fn left(buff: &mut Buffer) {
    motion::move_left(buff);
}

/// Move one step right (wrapper).
fn right(buff: &mut Buffer) {
    motion::move_right(buff);
}

/// Advance to next word.
fn adv_word(buff: &mut Buffer) {
    motion::adv_word(buff);
}

/// Move to previous word.
fn prev_word(buff: &mut Buffer) {
    motion::prev_word(buff);
}

/// Insert a single character at the current position.
fn insert_char(buff: &mut Buffer, ch: char) {
    let line_rc = match buff.curr_line.as_ref() {
        Some(l) => l.clone(),
        None => return,
    };
    let pos = (buff.position as usize).saturating_sub(1);
    {
        let mut line = line_rc.borrow_mut();
        let safe_pos = pos.min(line.line.len());
        line.line.insert(safe_pos, ch);
        line.line_length = line.line.len() as i32 + 1;
        line.changed = true;
    }
    buff.changed = true;
    buff.position += 1;
    buff.scr_horz += 1;
    buff.scr_pos = buff.scr_horz;
    buff.abs_pos = buff.scr_pos;
}

/// Delete the character at the cursor (delete-forward).
fn del_char_at_cursor(buff: &mut Buffer) {
    delete_ops::delete_forward(buff);
}

/// Delete from cursor to end of current word (including trailing spaces).
fn del_word_at_cursor(buff: &mut Buffer) -> String {
    delete_ops::del_word(buff)
}

/// Insert a newline / split the line at the cursor.
fn insert_line(buff: &mut Buffer) {
    // Split current line at cursor position
    let line_rc = match buff.curr_line.as_ref() { Some(l) => l.clone(), None => return };
    let pos = (buff.position as usize).saturating_sub(1);
    let rest = {
        let mut line = line_rc.borrow_mut();
        let rest = line.line[pos..].to_string();
        line.line.truncate(pos);
        line.line_length = line.line.len() as i32 + 1;
        line.changed = true;
        rest
    };
    let new_line_rc = crate::text::txtalloc();
    {
        let mut new_line = new_line_rc.borrow_mut();
        new_line.line = rest;
        new_line.line_length = new_line.line.len() as i32 + 1;
        new_line.max_length = new_line.line_length + 10;
        new_line.line_number = line_rc.borrow().line_number + 1;
        new_line.vert_len = 1;
        new_line.prev_line = Some(line_rc.clone());
        new_line.next_line = line_rc.borrow().next_line.clone();
    }
    if let Some(ref nx) = new_line_rc.borrow().next_line.clone() {
        nx.borrow_mut().prev_line = Some(new_line_rc.clone());
    }
    line_rc.borrow_mut().next_line = Some(new_line_rc.clone());
    buff.curr_line = Some(new_line_rc);
    buff.num_of_lines += 1;
    buff.absolute_lin += 1;
    buff.position = 1;
    buff.scr_horz = 0;
    buff.scr_pos = 0;
    buff.abs_pos = 0;
    let (_, height) = crate::ui::get_terminal_size();
    let text_height = (height as i32) - 1;
    if buff.scr_vert < text_height - 1 {
        buff.scr_vert += 1;
    } else {
        buff.window_top += 1;
    }
    buff.changed = true;
}

/// Move to next line (adv_line).
fn adv_line(buff: &mut Buffer) {
    motion::move_down(buff);
    bol(buff);
}

/// Delete entire current line.
fn del_line(buff: &mut Buffer) -> String {
    delete_ops::del_line(buff)
}

/// Undelete (re-insert) the last deleted word.
fn undel_word(buff: &mut Buffer, word: &str) {
    delete_ops::insert_string(buff, word);
}

// ──────────────────────────────────────────────────────────────────────────────
// Format (manual paragraph reflow)  — mirrors C `Format()`
// ──────────────────────────────────────────────────────────────────────────────

/// Reflow the paragraph the cursor is in to fit within `left_margin..right_margin`.
///
/// `right_margin == 0` is treated as "no right margin" (79 columns used).
pub fn format_paragraph(buff: &mut Buffer, left_margin: i32, right_margin: i32, right_justify: bool) {
    if curr_line_blank(buff) { return; }

    let rmargin = if right_margin == 0 { 79 } else { right_margin };

    // ── Move to start of paragraph (back over non-blank lines) ────────────────
    bol(buff);
    while !prev_line_blank(buff) {
        let has_prev = buff.curr_line.as_ref()
            .and_then(|l| l.borrow().prev_line.clone()).is_some();
        if !has_prev { break; }
        let prev = buff.curr_line.as_ref().unwrap().borrow().prev_line.clone().unwrap();
        buff.curr_line = Some(prev);
        buff.absolute_lin -= 1;
        if buff.scr_vert > 0 { buff.scr_vert -= 1; }
        else if buff.window_top > 1 { buff.window_top -= 1; }
        bol(buff);
    }

    // ── Join all lines in the paragraph into one long line ───────────────────
    while !next_line_blank(buff) {
        // go to end of current line
        eol(buff);
        // ensure a space before the merged content
        {
            let line_rc = match buff.curr_line.as_ref() { Some(l) => l.clone(), None => break };
            let line = line_rc.borrow();
            let ends_space = line.line.ends_with(' ');
            drop(line);
            if !ends_space {
                insert_char(buff, ' ');
            }
        }
        // join next line onto this one
        delete_ops::join_next_line(buff);
        // strip leading whitespace of the joined text
        // (position is now just past the space we added; del leading spaces from merged bit)
        {
            let line_rc = match buff.curr_line.as_ref() { Some(l) => l.clone(), None => break };
            let pos = (buff.position as usize).saturating_sub(1);
            let mut line = line_rc.borrow_mut();
            while pos < line.line.len() && (line.line.as_bytes()[pos] == b' ' || line.line.as_bytes()[pos] == b'\t') {
                line.line.remove(pos);
            }
            line.line_length = line.line.len() as i32 + 1;
        }
    }

    // ── Eliminate double spaces (except after '.') ────────────────────────────
    bol(buff);
    // skip leading indent
    {
        let ch = {
            let line_rc = match buff.curr_line.as_ref() { Some(l) => l.clone(), None => return };
            let line = line_rc.borrow();
            line.line.chars().next().unwrap_or('x')
        };
        if ch == ' ' || ch == '\t' { adv_word(buff); }
    }
    loop {
        let (pos, len, cur_ch, next_ch) = {
            let line_rc = match buff.curr_line.as_ref() { Some(l) => l.clone(), None => break };
            let line = line_rc.borrow();
            let p = (buff.position as usize).saturating_sub(1);
            let len = line.line.len();
            let cur = line.line.chars().nth(p).unwrap_or('\0');
            let nxt = line.line.chars().nth(p + 1).unwrap_or('\0');
            (p, len, cur, nxt)
        };
        if pos >= len { break; }
        if cur_ch == ' ' && next_ch == ' ' {
            del_char_at_cursor(buff);
        } else {
            right(buff);
        }
    }

    // ── Ensure two spaces after '.' ───────────────────────────────────────────
    bol(buff);
    loop {
        let (pos, len, cur_ch, next_ch) = {
            let line_rc = match buff.curr_line.as_ref() { Some(l) => l.clone(), None => break };
            let line = line_rc.borrow();
            let p = (buff.position as usize).saturating_sub(1);
            let len = line.line.len();
            let cur = line.line.chars().nth(p).unwrap_or('\0');
            let nxt = line.line.chars().nth(p + 1).unwrap_or('\0');
            (p, len, cur, nxt)
        };
        if pos >= len { break; }
        if cur_ch == '.' && next_ch == ' ' {
            right(buff); // past '.'
            // delete existing spaces
            loop {
                let ch = {
                    let line_rc = match buff.curr_line.as_ref() { Some(l) => l.clone(), None => break };
                    let line = line_rc.borrow();
                    let p = (buff.position as usize).saturating_sub(1);
                    line.line.chars().nth(p).unwrap_or('\0')
                };
                if ch == ' ' { del_char_at_cursor(buff); } else { break; }
            }
            insert_char(buff, ' ');
            insert_char(buff, ' ');
        } else {
            right(buff);
        }
    }

    // ── Wrap at right margin ──────────────────────────────────────────────────
    bol(buff);
    loop {
        // Advance to right_margin
        loop {
            let (pos, len) = {
                let line_rc = match buff.curr_line.as_ref() { Some(l) => l.clone(), None => break };
                let line = line_rc.borrow();
                (buff.position as usize, line.line.len())
            };
            if pos > len || buff.scr_horz >= rmargin { break; }
            right(buff);
        }
        let (pos, len) = {
            let line_rc = match buff.curr_line.as_ref() { Some(l) => l.clone(), None => break };
            let line = line_rc.borrow();
            (buff.position as usize, line.line.len())
        };
        if pos >= len { break; }
        // Past margin: find previous word boundary and insert newline
        prev_word(buff);
        if buff.position == 1 {
            adv_word(buff);
        }
        insert_line(buff);
        // strip leading spaces at start of new line
        loop {
            let ch = {
                let line_rc = match buff.curr_line.as_ref() { Some(l) => l.clone(), None => break };
                let line = line_rc.borrow();
                line.line.chars().next().unwrap_or('\0')
            };
            if ch == ' ' || ch == '\t' { del_char_at_cursor(buff); } else { break; }
        }
        // apply left margin indent if needed
        if left_margin > 0 && buff.scr_horz < left_margin {
            // insert a space and delete next char to preserve content
        }
    }

    // ── Right-justify ─────────────────────────────────────────────────────────
    if right_justify {
        // Walk back to start of paragraph
        bol(buff);
        while !prev_line_blank(buff) {
            let has_prev = buff.curr_line.as_ref()
                .and_then(|l| l.borrow().prev_line.clone()).is_some();
            if !has_prev { break; }
            let prev = buff.curr_line.as_ref().unwrap().borrow().prev_line.clone().unwrap();
            buff.curr_line = Some(prev);
            buff.absolute_lin -= 1;
            if buff.scr_vert > 0 { buff.scr_vert -= 1; }
            else if buff.window_top > 1 { buff.window_top -= 1; }
            bol(buff);
        }
        while !next_line_blank(buff) {
            eol(buff);
            let gap = rmargin - buff.scr_horz;
            if gap > 0 && gap < 20 {
                // distribute spaces before words working from right
                let mut spaces_to_add = gap;
                while spaces_to_add > 0 && buff.scr_horz > left_margin {
                    prev_word(buff);
                    if buff.scr_horz > left_margin {
                        insert_char(buff, ' ');
                        spaces_to_add -= 1;
                    }
                }
            }
            adv_line(buff);
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// Auto-format (word-wrap on insert)
// ──────────────────────────────────────────────────────────────────────────────

/// Called after inserting a space or tab in auto-format mode.
/// Wraps the current line at `right_margin` if it exceeds it, and pulls
/// words from the next line if they fit.
pub fn auto_format(buff: &mut Buffer, right_margin: i32) {
    if curr_line_blank(buff) { return; }

    let rmargin = if right_margin == 0 { 79 } else { right_margin };

    // ── If current line exceeds margin, push last word to next line ───────────
    if buff.scr_horz >= rmargin {
        let last_pos = {
            let line_rc = match buff.curr_line.as_ref() { Some(l) => l.clone(), None => return };
            let line = line_rc.borrow();
            line.line.len()
        };
        eol(buff);
        // find last space before margin and break there
        prev_word(buff);
        if buff.position > 1 {
            let del = del_word_at_cursor(buff);
            if next_line_blank(buff) {
                insert_line(buff);
            } else {
                adv_line(buff);
                bol(buff);
            }
            undel_word(buff, &del);
            bol(buff);
        } else {
            // can't wrap, just move on
            motion::move_down(buff);
            bol(buff);
        }
        let _ = last_pos;
        return;
    }

    // ── If there's room, pull first word from next line ───────────────────────
    let wlen = first_word_len(buff);
    if wlen > 0 && (buff.scr_horz as usize + wlen) < rmargin as usize && !next_line_blank(buff) {
        let saved_line = buff.curr_line.clone();
        let saved_pos  = buff.position;
        let saved_vert = buff.scr_vert;
        let saved_alin = buff.absolute_lin;

        adv_line(buff);
        if buff.curr_line.as_ref().and_then(|l| l.borrow().prev_line.clone()).is_some() {
            let word = del_word_at_cursor(buff);
            if curr_line_blank(buff) {
                del_line(buff);
            }
            // go back to original line end
            buff.curr_line   = saved_line;
            buff.position    = saved_pos;
            buff.scr_vert    = saved_vert;
            buff.absolute_lin = saved_alin;
            eol(buff);
            insert_char(buff, ' ');
            undel_word(buff, &word);
        } else {
            buff.curr_line   = saved_line;
            buff.position    = saved_pos;
            buff.scr_vert    = saved_vert;
            buff.absolute_lin = saved_alin;
        }
    }
}
