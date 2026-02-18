//! Delete operations – ported from src/delete.c
//!
//! Functions operate on the Buffer directly, mirroring the C originals.
//! The C code made heavy use of ncurses for redraw; here we only update
//! the data model – the draw loop in main.rs handles all rendering.

use crate::editor_state::Buffer;

// ──────────────────────────────────────────────────────────────────────────────
// Delete character (backspace / delete) — mirrors C `delete()`
// ──────────────────────────────────────────────────────────────────────────────

/// Backspace: delete the character *before* the cursor.
/// Returns the deleted character, or `None` if nothing was deleted.
pub fn backspace(buff: &mut Buffer) -> Option<char> {
    let pos = buff.position as usize;
    if pos > 1 {
        // Delete char at (pos-2) in the string (0-indexed, pos-1 is the
        // character the cursor sits ON, pos-2 is the one before it).
        let ch = {
            let line_rc = buff.curr_line.as_ref()?.clone();
            let mut line = line_rc.borrow_mut();
            if pos - 2 < line.line.len() {
                let removed = line.line.remove(pos - 2);
                line.line_length -= 1;
                line.changed = true;
                buff.changed = true;
                removed
            } else {
                return None;
            }
        };
        buff.position -= 1;
        buff.scr_horz  = (buff.scr_horz - 1).max(0);
        buff.scr_pos   = buff.scr_horz;
        buff.abs_pos   = buff.scr_pos;
        Some(ch)
    } else if buff.curr_line.as_ref().and_then(|l| l.borrow().prev_line.clone()).is_some() {
        // At the beginning of the line: merge with previous line (the C
        // `delete` function when position == 1 joins curr_line onto prev).
        join_with_prev_line(buff)
    } else {
        None
    }
}

/// Delete-forward: delete the character *at* the cursor position.
/// Returns the deleted character, or `None` if nothing was deleted.
pub fn delete_forward(buff: &mut Buffer) -> Option<char> {
    let pos = buff.position as usize;
    let line_len = buff.curr_line.as_ref()?.borrow().line_length;
    if pos <= line_len as usize {
        let ch = {
            let line_rc = buff.curr_line.as_ref()?.clone();
            let mut line = line_rc.borrow_mut();
            if pos - 1 < line.line.len() {
                let removed = line.line.remove(pos - 1);
                line.line_length -= 1;
                line.changed = true;
                buff.changed = true;
                removed
            } else {
                return None;
            }
        };
        // position stays the same (or clamps to new line length)
        let new_len = buff.curr_line.as_ref()?.borrow().line_length;
        if buff.position > new_len {
            buff.position = new_len.max(1);
            buff.scr_horz = (buff.position - 1).max(0);
            buff.scr_pos  = buff.scr_horz;
            buff.abs_pos  = buff.scr_pos;
        }
        Some(ch)
    } else if buff.curr_line.as_ref().and_then(|l| l.borrow().next_line.clone()).is_some() {
        // At the end of the line: merge next line into this one
        join_with_next_line(buff)
    } else {
        None
    }
}

/// Public version of join-forward, used by format.rs.
pub fn join_next_line(buff: &mut Buffer) {
    join_with_next_line(buff);
}

/// Merge `curr_line->next_line` into `curr_line` (join forward).
fn join_with_next_line(buff: &mut Buffer) -> Option<char> {
    let next_rc = buff.curr_line.as_ref()?.borrow().next_line.clone()?;
    let next_text = next_rc.borrow().line.clone();
    {
        let line_rc = buff.curr_line.as_ref()?.clone();
        let mut line = line_rc.borrow_mut();
        line.line.push_str(&next_text);
        line.line_length = line.line.len() as i32 + 1;
        line.changed = true;
        // Relink: skip over next_rc
        line.next_line = next_rc.borrow().next_line.clone();
        if let Some(ref nn) = line.next_line.clone() {
            nn.borrow_mut().prev_line = buff.curr_line.clone();
        }
    }
    buff.changed = true;
    buff.num_of_lines -= 1;
    Some('\n')
}

/// Merge `curr_line` into `curr_line->prev_line` (join backward, used by backspace at bol).
fn join_with_prev_line(buff: &mut Buffer) -> Option<char> {
    let prev_rc = buff.curr_line.as_ref()?.borrow().prev_line.clone()?;
    let curr_text = buff.curr_line.as_ref()?.borrow().line.clone();
    let prev_len = prev_rc.borrow().line_length;
    {
        let mut prev = prev_rc.borrow_mut();
        let _old_len = prev.line.len();
        prev.line.push_str(&curr_text);
        prev.line_length = prev.line.len() as i32 + 1;
        prev.changed = true;
        // Relink
        prev.next_line = buff.curr_line.as_ref()?.borrow().next_line.clone();
        if let Some(ref nn) = prev.next_line.clone() {
            nn.borrow_mut().prev_line = Some(prev_rc.clone());
        }
    }
    buff.curr_line = Some(prev_rc);
    buff.absolute_lin -= 1;
    buff.changed = true;
    buff.num_of_lines -= 1;
    // Cursor lands at the old end of the previous line
    buff.position  = prev_len;
    buff.scr_horz  = (prev_len - 1).max(0);
    buff.scr_pos   = buff.scr_horz;
    buff.abs_pos   = buff.scr_pos;
    if buff.scr_vert > 0 {
        buff.scr_vert -= 1;
    } else if buff.window_top > 1 {
        buff.window_top -= 1;
    }
    Some('\n')
}

// ──────────────────────────────────────────────────────────────────────────────
// Delete word (del_word from delete.c)
// ──────────────────────────────────────────────────────────────────────────────

/// Delete from cursor to end of the current word (including trailing
/// whitespace), mirroring `del_word()` in the C source.
/// Returns the deleted string.
pub fn del_word(buff: &mut Buffer) -> String {
    let line_rc = match buff.curr_line.as_ref() {
        Some(l) => l.clone(),
        None => return String::new(),
    };
    let mut line = line_rc.borrow_mut();
    let pos = (buff.position as usize).saturating_sub(1); // 0-indexed start
    let chars: Vec<char> = line.line.chars().collect();
    let len = chars.len();
    if pos >= len { return String::new(); }

    // Scan forward: skip non-whitespace, then skip whitespace
    let mut end = pos;
    while end < len && chars[end] != ' ' && chars[end] != '\t' { end += 1; }
    while end < len && (chars[end] == ' ' || chars[end] == '\t') { end += 1; }

    // Calculate byte offsets from char offsets
    let byte_start: usize = chars[..pos].iter().collect::<String>().len();
    let byte_end:   usize = chars[..end].iter().collect::<String>().len();
    let deleted: String = line.line[byte_start..byte_end].to_string();
    line.line.replace_range(byte_start..byte_end, "");
    line.line_length = line.line.len() as i32 + 1;
    line.changed = true;
    buff.changed = true;
    deleted
}

// ──────────────────────────────────────────────────────────────────────────────
// Delete to end of line (Clear_line from delete.c)
// ──────────────────────────────────────────────────────────────────────────────

/// Delete from cursor to end of line, leaving the line truncated at the cursor.
/// Returns the deleted text.
pub fn del_to_eol(buff: &mut Buffer) -> String {
    let line_rc = match buff.curr_line.as_ref() {
        Some(l) => l.clone(),
        None => return String::new(),
    };
    let mut line = line_rc.borrow_mut();
    let pos = (buff.position as usize).saturating_sub(1);
    if pos >= line.line.len() { return String::new(); }
    let deleted = line.line[pos..].to_string();
    line.line.truncate(pos);
    line.line_length = line.line.len() as i32 + 1;
    line.changed = true;
    buff.changed = true;
    deleted
}

// ──────────────────────────────────────────────────────────────────────────────
// Delete entire line (del_line from delete.c)
// ──────────────────────────────────────────────────────────────────────────────

/// Delete the current line, joining it out of the linked list.
/// The cursor moves to the start of the next line (or the previous if at EOF).
/// Returns the deleted line text.
pub fn del_line(buff: &mut Buffer) -> String {
    let line_rc = match buff.curr_line.as_ref() {
        Some(l) => l.clone(),
        None => return String::new(),
    };
    let deleted = line_rc.borrow().line.clone();
    let prev = line_rc.borrow().prev_line.clone();
    let next = line_rc.borrow().next_line.clone();

    match (prev.clone(), next.clone()) {
        (Some(ref p), Some(ref n)) => {
            p.borrow_mut().next_line = Some(n.clone());
            n.borrow_mut().prev_line = Some(p.clone());
            buff.curr_line = Some(n.clone());
        }
        (None, Some(ref n)) => {
            n.borrow_mut().prev_line = None;
            buff.curr_line = Some(n.clone());
        }
        (Some(ref p), None) => {
            p.borrow_mut().next_line = None;
            buff.curr_line = Some(p.clone());
            buff.absolute_lin -= 1;
            if buff.scr_vert > 0 {
                buff.scr_vert -= 1;
            } else if buff.window_top > 1 {
                buff.window_top -= 1;
            }
        }
        (None, None) => {
            // Only line in the buffer: clear it instead of removing
            line_rc.borrow_mut().line.clear();
            line_rc.borrow_mut().line_length = 1;
        }
    }
    buff.position  = 1;
    buff.scr_horz  = 0;
    buff.scr_pos   = 0;
    buff.abs_pos   = 0;
    buff.changed   = true;
    buff.num_of_lines -= 1;
    deleted
}

// ──────────────────────────────────────────────────────────────────────────────
// Undelete (insert saved text back)
// ──────────────────────────────────────────────────────────────────────────────

/// Re-insert a previously-deleted string at the current cursor position.
/// Mirrors `undel_string()` / `undel_word()` in the C source.
pub fn insert_string(buff: &mut Buffer, text: &str) {
    let line_rc = match buff.curr_line.as_ref() {
        Some(l) => l.clone(),
        None => return,
    };
    let pos = (buff.position as usize).saturating_sub(1);
    {
        let mut line = line_rc.borrow_mut();
        // Ensure pos ≤ line.len()
        let safe_pos = pos.min(line.line.len());
        line.line.insert_str(safe_pos, text);
        line.line_length = line.line.len() as i32 + 1;
        line.changed = true;
    }
    buff.changed = true;
    // Advance cursor past inserted text
    let n = text.chars().count() as i32;
    buff.position += n;
    buff.scr_horz += n;
    buff.scr_pos   = buff.scr_horz;
    buff.abs_pos   = buff.scr_pos;
}

/// Re-insert a single character (used by undel_char).
pub fn insert_char_at_cursor(buff: &mut Buffer, ch: char) {
    let s = ch.to_string();
    insert_string(buff, &s);
}
