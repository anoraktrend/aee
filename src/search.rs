//! Search and replace – ported from src/srch_rep.c
//!
//! Provides forward/reverse literal search with optional case-sensitivity,
//! mirroring the core logic of the C `search()` and `replace()` functions.
//! The regex engine from the C original is not yet ported; only literal
//! search is implemented here.

use crate::editor_state::Buffer;

// ──────────────────────────────────────────────────────────────────────────────
// Search result
// ──────────────────────────────────────────────────────────────────────────────

/// Position of a search match.
pub struct SearchResult {
    /// Absolute line number (1-based) where the match was found.
    pub line_num: i32,
    /// 1-based column of the first character of the match.
    pub col: i32,
    /// Number of lines moved from the starting position.
    pub lines_moved: i32,
}

// ──────────────────────────────────────────────────────────────────────────────
// Forward search  (mirrors C `search()` with forward=TRUE, literal=TRUE)
// ──────────────────────────────────────────────────────────────────────────────

/// Search forward from the current cursor position for `needle`.
/// `case_sensitive` mirrors the `case_sen` global in the C code.
///
/// On success the buffer's `curr_line`, `position`, `scr_horz`, `abs_pos`,
/// `absolute_lin`, `scr_vert`, and `window_top` are updated to point at the
/// first character of the match, exactly as the C `search()` does when
/// `move_cursor` is TRUE.
///
/// Returns `Some(SearchResult)` on success, `None` if not found.
pub fn search_forward(buff: &mut Buffer, needle: &str, case_sensitive: bool) -> Option<SearchResult> {
    if needle.is_empty() { return None; }

    let needle_up: String = needle.to_uppercase();
    let pat: &str = if case_sensitive { needle } else { &needle_up };

    let _start_line_num = buff.absolute_lin;
    let start_pos      = buff.position as usize; // 1-based

    let mut current = buff.curr_line.clone()?;
    let mut line_num = buff.absolute_lin;
    let mut lines_moved = 0i32;
    let mut first_line = true;

    loop {
        let (found_col, line_abs_lin) = {
            let line = current.borrow();
            let text = &line.line;
            // On the first line we start searching *after* the cursor position.
            let search_from = if first_line { start_pos } else { 0 };
            let slice = if search_from < text.len() { &text[search_from..] } else { "" };

            let hit = if case_sensitive {
                slice.find(needle)
            } else {
                // Case-insensitive: compare uppercase
                let slice_up = slice.to_uppercase();
                slice_up.find(pat)
            };

            match hit {
                Some(offset) => {
                    // offset is a byte offset into `slice`; add search_from to get
                    // byte offset into `text`, then convert to 1-based char col.
                    let byte_pos = search_from + offset;
                    let col = text[..byte_pos].chars().count() + 1;
                    (Some(col as i32), line_num)
                }
                None => (None, line_num),
            }
        };

        if let Some(col) = found_col {
            // Update buffer state
            buff.curr_line    = Some(current.clone());
            buff.absolute_lin = line_abs_lin;
            buff.position     = col;
            buff.scr_horz     = (col - 1).max(0);
            buff.scr_pos      = buff.scr_horz;
            buff.abs_pos      = buff.scr_pos;

            // Recalculate scr_vert / window_top so the match line is visible
            let (_, height) = crate::ui::get_terminal_size();
            let text_height = (height as i32) - 1;
            let half = text_height / 2;
            buff.scr_vert   = half.min(buff.absolute_lin - 1);
            buff.window_top = (buff.absolute_lin - buff.scr_vert).max(1);

            return Some(SearchResult { line_num: line_abs_lin, col, lines_moved });
        }

        // Advance to next line
        let next = current.borrow().next_line.clone();
        match next {
            Some(n) => {
                current = n;
                line_num += 1;
                lines_moved += 1;
                first_line = false;
            }
            None => break, // reached EOF without finding the needle
        }
    }

    None
}

// ──────────────────────────────────────────────────────────────────────────────
// Reverse search
// ──────────────────────────────────────────────────────────────────────────────

/// Search backward from the current cursor position for `needle`.
pub fn search_backward(buff: &mut Buffer, needle: &str, case_sensitive: bool) -> Option<SearchResult> {
    if needle.is_empty() { return None; }

    let needle_up: String = needle.to_uppercase();
    let pat: &str = if case_sensitive { needle } else { &needle_up };

    let start_pos = (buff.position as usize).saturating_sub(2); // search *before* cursor

    let mut current = buff.curr_line.clone()?;
    let mut line_num = buff.absolute_lin;
    let mut lines_moved = 0i32;
    let mut first_line = true;

    loop {
        let found_col: Option<i32> = {
            let line = current.borrow();
            let text = &line.line;
            let search_to = if first_line { start_pos.min(text.len()) } else { text.len() };
            let slice = &text[..search_to];

            let hit = if case_sensitive {
                slice.rfind(needle)
            } else {
                let slice_up = slice.to_uppercase();
                slice_up.rfind(pat)
            };

            hit.map(|byte_pos| (text[..byte_pos].chars().count() + 1) as i32)
        };

        if let Some(col) = found_col {
            buff.curr_line    = Some(current.clone());
            buff.absolute_lin = line_num;
            buff.position     = col;
            buff.scr_horz     = (col - 1).max(0);
            buff.scr_pos      = buff.scr_horz;
            buff.abs_pos      = buff.scr_pos;

            let (_, height) = crate::ui::get_terminal_size();
            let text_height = (height as i32) - 1;
            let half = text_height / 2;
            buff.scr_vert   = half.min(buff.absolute_lin - 1);
            buff.window_top = (buff.absolute_lin - buff.scr_vert).max(1);

            return Some(SearchResult { line_num, col, lines_moved });
        }

        let prev = current.borrow().prev_line.clone();
        match prev {
            Some(p) => {
                current = p;
                line_num -= 1;
                lines_moved += 1;
                first_line = false;
            }
            None => break,
        }
    }

    None
}

// ──────────────────────────────────────────────────────────────────────────────
// Replace  (mirrors C `replace()`)
// ──────────────────────────────────────────────────────────────────────────────

/// Replace the next occurrence of `needle` with `replacement`.
/// Returns `true` if a replacement was made.
pub fn replace_next(buff: &mut Buffer, needle: &str, replacement: &str, case_sensitive: bool) -> bool {
    match search_forward(buff, needle, case_sensitive) {
        None => false,
        Some(_res) => {
            // The buffer is now positioned at the start of the match.
            let line_rc = match buff.curr_line.as_ref() {
                Some(l) => l.clone(),
                None => return false,
            };
            let pos = (buff.position as usize).saturating_sub(1); // 0-indexed
            {
                let mut line = line_rc.borrow_mut();
                // Locate byte offset
                let byte_start: usize = line.line.chars().take(pos).map(|c| c.len_utf8()).sum();
                let byte_end   = byte_start + needle.len();
                if byte_end <= line.line.len() {
                    line.line.replace_range(byte_start..byte_end, replacement);
                    line.line_length = line.line.len() as i32 + 1;
                    line.changed = true;
                }
            }
            buff.changed = true;
            // Advance cursor past the replacement
            let n = replacement.chars().count() as i32;
            buff.position += n;
            buff.scr_horz  = (buff.position - 1).max(0);
            buff.scr_pos   = buff.scr_horz;
            buff.abs_pos   = buff.scr_pos;
            true
        }
    }
}

/// Replace **all** occurrences from the current position to EOF.
/// Returns the number of replacements made.
pub fn replace_all(buff: &mut Buffer, needle: &str, replacement: &str, case_sensitive: bool) -> usize {
    let mut count = 0;
    while replace_next(buff, needle, replacement, case_sensitive) {
        count += 1;
    }
    count
}
