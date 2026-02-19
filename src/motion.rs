// motion.rs – cursor-movement routines
// Mirrors src/motion.c exactly (bottom, top, nextline, prevline, left, right,
// find_pos, up, down, adv_word, prev_word, move_rel, eol, bol, adv_line,
// next_page, prev_page, goto_line).
//
// All public functions take `&mut Buffer` to match the calling convention used
// in main.rs and format.rs.

use crate::editor_state::Buffer;
use crate::ui::{COLS, scanline};

// ────────────────────────────────────────────────────────────────────────────
// Internal character-width helper (mirrors C `len_char`)
// ────────────────────────────────────────────────────────────────────────────

fn len_char(scr_pos: i32, ch: char) -> i32 {
    if ch == '\t' {
        8 - (scr_pos % 8)
    } else if (ch as u32) < 32 || ch == '\x7f' {
        2
    } else {
        1
    }
}

// ────────────────────────────────────────────────────────────────────────────
// bottom / top
// ────────────────────────────────────────────────────────────────────────────

/// Move cursor to the bottom of the file (mirrors C `bottom()`).
pub fn bottom(buff: &mut Buffer) {
    let pos = buff.position;
    if pos > 1 { adv_line(buff); }
    let num_lines = {
        let mut count = 0i32;
        let mut tmp = buff.curr_line.clone();
        while let Some(line_rc) = tmp {
            if line_rc.borrow().next_line.is_none() { break; }
            count += 1;
            let next = line_rc.borrow().next_line.clone();
            tmp = next;
        }
        count
    };
    move_rel(buff, "d", num_lines);
}

/// Move cursor to the top of the file (mirrors C `top()`).
pub fn top(buff: &mut Buffer) {
    let num_lines = {
        let mut count = 0i32;
        let mut tmp = buff.curr_line.clone();
        while let Some(line_rc) = tmp {
            if line_rc.borrow().prev_line.is_none() { break; }
            count += 1;
            let prev = line_rc.borrow().prev_line.clone();
            tmp = prev;
        }
        count
    };
    move_rel(buff, "u", num_lines);
}

// ────────────────────────────────────────────────────────────────────────────
// nextline / prevline
// ────────────────────────────────────────────────────────────────────────────

/// Move to the first character of the next line (mirrors C `nextline()`).
pub fn nextline(buff: &mut Buffer) {
    let next = buff.curr_line.as_ref().and_then(|l| l.borrow().next_line.clone());
    if let Some(next_line) = next {
        let cols      = COLS();
        let vert_len  = buff.curr_line.as_ref()
            .map(|l| l.borrow().vert_len)
            .unwrap_or(1)
            - (buff.scr_pos / cols);

        buff.curr_line = Some(next_line);
        buff.position  = 1;
        buff.abs_pos   = 0;
        buff.scr_pos   = 0;
        buff.scr_horz  = 0;

        let new_vert = buff.scr_vert.saturating_add(vert_len);
        buff.scr_vert = new_vert.min(buff.last_line);
    }
}

/// Move to the last character of the previous line (mirrors C `prevline()`).
pub fn prevline(buff: &mut Buffer) {
    let prev = buff.curr_line.as_ref().and_then(|l| l.borrow().prev_line.clone());

    if let Some(prev_line) = prev {
        let cols     = COLS();
        let prev_vert = prev_line.borrow().vert_len;
        let p        = prev_vert + (buff.scr_pos / cols);

        buff.curr_line = Some(prev_line.clone());
        buff.position  = 1;
        buff.scr_pos   = 0;
        buff.scr_vert  = buff.scr_vert.saturating_sub(p);

        // Advance to end of the (now-current) previous line.
        let ll = buff.curr_line.as_ref()
            .map(|l| l.borrow().line_length)
            .unwrap_or(1);
        while buff.position < ll {
            buff.position = buff.position.saturating_add(1);
        }
        let scr_p = buff.curr_line.clone()
            .map(|l| scanline(&l.borrow(), buff.position))
            .unwrap_or(0);
        buff.abs_pos  = scr_p;
        buff.scr_pos  = scr_p;
    }
}

// ────────────────────────────────────────────────────────────────────────────
// find_pos (mirrors C `find_pos()`)
// ────────────────────────────────────────────────────────────────────────────

/// Restore the cursor to the same column on the new current line.
/// Called after up()/down() to preserve horizontal position.
pub fn find_pos(buff: &mut Buffer) {
    let target = buff.abs_pos;
    let mut scr_horz = 0i32;
    let mut pos      = 1i32;

    let (ll, line_s) = {
        let line = buff.curr_line.as_ref().map(|l| l.borrow());
        match line {
            Some(ref l) => (l.line_length, l.line.clone()),
            None        => return,
        }
    };
    let chars: Vec<char> = line_s.chars().collect();

    while scr_horz < target && pos < ll {
        let ch = chars.get((pos - 1) as usize).copied().unwrap_or('\0');
        scr_horz = scr_horz.saturating_add(len_char(scr_horz, ch));
        pos = pos.saturating_add(1);
    }

    buff.scr_horz = scr_horz % COLS();
    buff.scr_pos  = scr_horz;
    buff.position = pos;
}

// ────────────────────────────────────────────────────────────────────────────
// left / right
// ────────────────────────────────────────────────────────────────────────────

/// Move cursor one character left (mirrors C `left()`).
/// Returns `true` if the cursor moved.
pub fn left(buff: &mut Buffer, _disp: bool) -> bool {
    let pos     = buff.position;
    let scr_pos = buff.scr_pos;

    if pos != 1 {
        let ch = {
            let line = buff.curr_line.as_ref().map(|l| l.borrow()).unwrap();
            let chars: Vec<char> = line.line.chars().collect();
            chars.get((pos - 2) as usize).copied().unwrap_or('\0')
        };

        buff.position -= 1;
        let new_scr = if ch == '\t' {
            buff.curr_line.clone()
                .map(|l| scanline(&l.borrow(), buff.position))
                .unwrap_or(0)
        } else {
            scr_pos - len_char(scr_pos, ch)
        };
        buff.abs_pos  = new_scr;
        buff.scr_pos  = new_scr;
        buff.scr_horz = new_scr % COLS();
        true
    } else {
        let has_prev = buff.curr_line.as_ref()
            .and_then(|l| l.borrow().prev_line.clone())
            .is_some();
        if has_prev {
            prevline(buff);
            buff.scr_horz    = buff.scr_pos % COLS();
            buff.absolute_lin -= 1;
            true
        } else {
            false
        }
    }
}

/// Move cursor one character right (mirrors C `right()`).
/// Returns `true` if the cursor moved.
pub fn right(buff: &mut Buffer, _disp: bool) -> bool {
    let pos = buff.position;
    let ll  = buff.curr_line.as_ref()
        .map(|l| l.borrow().line_length)
        .unwrap_or(1);

    if pos < ll {
        let ch = {
            let line = buff.curr_line.as_ref().map(|l| l.borrow()).unwrap();
            let chars: Vec<char> = line.line.chars().collect();
            chars.get((pos - 1) as usize).copied().unwrap_or('\0')
        };

        let r = buff.scr_pos.saturating_add(len_char(buff.scr_pos, ch));
        buff.position = buff.position.saturating_add(1);
        buff.abs_pos   = r;
        buff.scr_pos   = r;
        buff.scr_horz  = r % COLS();
        true
    } else {
        let has_next = buff.curr_line.as_ref()
            .and_then(|l| l.borrow().next_line.clone())
            .is_some();
        if has_next {
            nextline(buff);
            buff.position  = 1;
            buff.abs_pos   = 0;
            buff.scr_pos   = 0;
            buff.absolute_lin += 1;
            true
        } else {
            false
        }
    }
}

// ────────────────────────────────────────────────────────────────────────────
// up / down
// ────────────────────────────────────────────────────────────────────────────

/// Move cursor up one line (mirrors C `up()`).
pub fn up(buff: &mut Buffer) {
    if buff.curr_line.as_ref()
        .and_then(|l| l.borrow().prev_line.clone())
        .is_none()
    { return; }

    let tscr_pos = buff.abs_pos;
    prevline(buff);
    buff.position  = 1;
    buff.scr_horz  = 0;
    buff.abs_pos   = tscr_pos;
    find_pos(buff);
    buff.scr_pos   = buff.scr_horz;
    buff.scr_horz  = buff.scr_pos % COLS();
    buff.scr_vert = buff.scr_vert.saturating_add(buff.scr_pos / COLS());
    buff.absolute_lin = buff.absolute_lin.saturating_sub(1);
    buff.abs_pos   = tscr_pos;
}

/// Move cursor down one line (mirrors C `down()`).
pub fn down(buff: &mut Buffer) {
    if buff.curr_line.as_ref()
        .and_then(|l| l.borrow().next_line.clone())
        .is_none()
    { return; }

    let tscr_pos = buff.abs_pos;
    nextline(buff);
    buff.abs_pos   = tscr_pos;
    find_pos(buff);
    buff.scr_pos   = buff.scr_horz;
    buff.scr_horz  = buff.scr_pos % COLS();
    buff.scr_vert = buff.scr_vert.saturating_add(buff.scr_pos / COLS());
    buff.absolute_lin = buff.absolute_lin.saturating_add(1);
    buff.abs_pos   = tscr_pos;
}

// ────────────────────────────────────────────────────────────────────────────
// Public aliases used by main.rs and format.rs
// ────────────────────────────────────────────────────────────────────────────

/// Alias for `left(buff, true)` – used by main.rs key handling.
pub fn move_left(buff: &mut Buffer) { left(buff, true); }

/// Alias for `right(buff, true)` – used by main.rs key handling.
pub fn move_right(buff: &mut Buffer) { right(buff, true); }

/// Alias for `up(buff)` – used by main.rs key handling.
pub fn move_up(buff: &mut Buffer) { up(buff); }

/// Alias for `down(buff)` – used by main.rs key handling.
pub fn move_down(buff: &mut Buffer) { down(buff); }

// ────────────────────────────────────────────────────────────────────────────
// goto_line (mirrors C `goto_line()`)
// ────────────────────────────────────────────────────────────────────────────

/// Move cursor to absolute line number `n` (1-based).
pub fn goto_line(buff: &mut Buffer, n: i32) {
    // Go to top first.
    top(buff);
    // Then advance n-1 lines.
    for _ in 1..n {
        if buff.curr_line.as_ref()
            .and_then(|l| l.borrow().next_line.clone())
            .is_none()
        { break; }
        adv_line(buff);
    }
}

// ────────────────────────────────────────────────────────────────────────────
// adv_word / prev_word
// ────────────────────────────────────────────────────────────────────────────

fn curr_char(buff: &Buffer) -> char {
    let pos = buff.position;
    let line = buff.curr_line.as_ref().map(|l| l.borrow()).unwrap();
    let chars: Vec<char> = line.line.chars().collect();
    chars.get((pos - 1) as usize).copied().unwrap_or('\0')
}

fn prev_char(buff: &Buffer) -> char {
    let pos = buff.position;
    if pos < 2 { return '\0'; }
    let line = buff.curr_line.as_ref().map(|l| l.borrow()).unwrap();
    let chars: Vec<char> = line.line.chars().collect();
    chars.get((pos - 2) as usize).copied().unwrap_or('\0')
}

fn at_end(buff: &Buffer) -> bool {
    buff.position >= buff.curr_line.as_ref()
        .map(|l| l.borrow().line_length)
        .unwrap_or(1)
}

fn at_start(buff: &Buffer) -> bool {
    buff.position == 1
}

fn is_word_sep(ch: char) -> bool { ch == ' ' || ch == '\t' }

/// Advance to the next word (mirrors C `adv_word()`).
pub fn adv_word(buff: &mut Buffer) {
    if at_end(buff) { right(buff, true); return; }
    while !at_end(buff) && !is_word_sep(curr_char(buff)) { right(buff, true); }
    while !at_end(buff) &&  is_word_sep(curr_char(buff)) { right(buff, true); }
}

/// Move to the start of the previous word (mirrors C `prev_word()`).
pub fn prev_word(buff: &mut Buffer) {
    if at_start(buff) { left(buff, true); return; }
    if !at_start(buff) && is_word_sep(prev_char(buff)) {
        while !at_start(buff) && !is_word_sep(curr_char(buff)) { left(buff, true); }
    }
    while !at_start(buff) &&  is_word_sep(curr_char(buff)) { left(buff, true); }
    while !at_start(buff) && !is_word_sep(curr_char(buff)) { left(buff, true); }
    if !at_start(buff) && is_word_sep(curr_char(buff)) { right(buff, true); }
}

// ────────────────────────────────────────────────────────────────────────────
// eol / bol / adv_line
// ────────────────────────────────────────────────────────────────────────────

/// Move to end of current line (mirrors C `eol()`).
pub fn eol(buff: &mut Buffer) {
    let (pos, ll) = (buff.position,
        buff.curr_line.as_ref().map(|l| l.borrow().line_length).unwrap_or(1));

    if pos == ll {
        let has_next = buff.curr_line.as_ref()
            .and_then(|l| l.borrow().next_line.clone())
            .is_some();
        if has_next { right(buff, true); }
    }

    let ll2 = buff.curr_line.as_ref()
        .map(|l| l.borrow().line_length)
        .unwrap_or(1);
    buff.position = ll2;
    let scr_p = buff.curr_line.clone()
        .map(|l| scanline(&l.borrow(), buff.position))
        .unwrap_or(0);
    buff.abs_pos  = scr_p;
    buff.scr_pos  = scr_p;
    buff.scr_horz = scr_p % COLS();
    let delta = (scr_p / COLS()).min(buff.last_line - buff.scr_vert);
    buff.scr_vert = buff.scr_vert.saturating_add(delta);
}

/// Move to beginning of current line (mirrors C `bol()`).
pub fn bol(buff: &mut Buffer) {
    let pos      = buff.position;
    let has_prev = buff.curr_line.as_ref()
        .and_then(|l| l.borrow().prev_line.clone())
        .is_some();

    if pos != 1 {
        let delta = buff.scr_pos / COLS();
        buff.position  = 1;
        buff.scr_horz  = 0;
        buff.scr_pos   = 0;
        buff.abs_pos   = 0;
        buff.scr_vert  = buff.scr_vert.saturating_sub(delta);
    } else if has_prev {
        buff.abs_pos = 0;
        up(buff);
    }
}

/// Advance to the beginning of the next line (mirrors C `adv_line()`).
pub fn adv_line(buff: &mut Buffer) {
    buff.abs_pos = 0;
    let pos = buff.position;
    if pos == 1 {
        down(buff);
    } else {
        let ll = buff.curr_line.as_ref()
            .map(|l| l.borrow().line_length)
            .unwrap_or(1);
        if pos < ll { eol(buff); }
        right(buff, true);
    }
}

// ────────────────────────────────────────────────────────────────────────────
// move_rel (mirrors C `move_rel(direction, lines)`)
// ────────────────────────────────────────────────────────────────────────────

/// Move `lines` lines in `direction` ("u" = up, "d" = down).
pub fn move_rel(buff: &mut Buffer, direction: &str, lines: i32) {
    if direction.starts_with('u') {
        // Go to BOL first, then move up.
        if buff.position != 1 {
            while left(buff, true) && buff.position != 1 {}
        }
        for _ in 0..lines {
            let has_prev = buff.curr_line.as_ref()
                .and_then(|l| l.borrow().prev_line.clone())
                .is_some();
            if !has_prev { break; }
            up(buff);
        }
    } else {
        adv_line(buff);
        for i in 1..lines {
            let has_next = buff.curr_line.as_ref()
                .and_then(|l| l.borrow().next_line.clone())
                .is_some();
            if !has_next { break; }
            if i < lines { down(buff); }
        }
    }
}

// ────────────────────────────────────────────────────────────────────────────
// next_page / prev_page
// ────────────────────────────────────────────────────────────────────────────

/// Move the cursor forward one page (mirrors C `next_page()`).
pub fn next_page(buff: &mut Buffer) {
    let last_line = buff.last_line;
    let mut counter = 0i32;
    while counter < last_line {
        if buff.curr_line.as_ref()
            .and_then(|l| l.borrow().next_line.clone())
            .is_none()
        { break; }
        adv_line(buff);
        counter += 1;
    }
}

/// Move the cursor backward one page (mirrors C `prev_page()`).
pub fn prev_page(buff: &mut Buffer) {
    let last_line = buff.last_line;
    let mut counter = 0i32;
    while counter < last_line {
        if buff.curr_line.as_ref()
            .and_then(|l| l.borrow().prev_line.clone())
            .is_none()
        { break; }
        if buff.position != 1 { bol(buff); }
        bol(buff);
        counter += 1;
    }
}
