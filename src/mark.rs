#![allow(dead_code)]

/// Mark / cut / copy / paste – ported from src/mark.c
///
/// The paste buffer is a separate linked list of `TextLine` nodes that
/// mirrors the editor's main text list.  Copy/cut populate it; paste
/// inserts it at the current cursor position.

use std::rc::Rc;
use std::cell::RefCell;

use crate::editor_state::{Buffer, TextLine};
use crate::text::txtalloc;
use crate::delete_ops;

// ──────────────────────────────────────────────────────────────────────────────
// Mark-mode flags (mirrors C enum values)
// ──────────────────────────────────────────────────────────────────────────────

#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum MarkMode {
    Inactive,
    Mark,
    Append,
    Prefix,
}

// ──────────────────────────────────────────────────────────────────────────────
// Mark state (lives on the EditorState or passed around)
// ──────────────────────────────────────────────────────────────────────────────

/// All mutable state associated with mark/paste operations.
pub struct MarkState {
    /// Current mark mode.
    pub mode: MarkMode,
    /// First line of the paste buffer (result of copy/cut).
    pub paste_buff: Option<Rc<RefCell<TextLine>>>,
    /// First line of the select buffer (being built during mark).
    pub fpste_line: Option<Rc<RefCell<TextLine>>>,
    /// Current line of the select buffer.
    pub cpste_line: Option<Rc<RefCell<TextLine>>>,
    /// Position within `cpste_line`.
    pub pst_pos: i32,
}

impl MarkState {
    pub fn new() -> Self {
        MarkState {
            mode: MarkMode::Inactive,
            paste_buff: None,
            fpste_line: None,
            cpste_line: None,
            pst_pos: 1,
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// Allocate a new paste line
// ──────────────────────────────────────────────────────────────────────────────

fn alloc_paste_line(max_len: usize) -> Rc<RefCell<TextLine>> {
    let rc = txtalloc();
    {
        let mut l = rc.borrow_mut();
        l.line = String::with_capacity(max_len);
        l.line_length = 1;
        l.max_length = max_len as i32 + 10;
    }
    rc
}

// ──────────────────────────────────────────────────────────────────────────────
// slct – initiate mark mode
// ──────────────────────────────────────────────────────────────────────────────

/// Begin marking text.  If mark is already active, cancel it.
pub fn slct(ms: &mut MarkState, buff: &Buffer, mode: MarkMode) {
    if ms.mode != MarkMode::Inactive {
        unmark_text(ms);
        return;
    }
    ms.mode = mode;
    let max_len = buff.curr_line.as_ref()
        .map(|l| l.borrow().max_length as usize)
        .unwrap_or(64);
    let new_line = alloc_paste_line(max_len);
    ms.fpste_line = Some(new_line.clone());
    ms.cpste_line = Some(new_line);
    ms.pst_pos = 1;
}

// ──────────────────────────────────────────────────────────────────────────────
// unmark_text – deactivate mark mode and discard select buffer
// ──────────────────────────────────────────────────────────────────────────────

pub fn unmark_text(ms: &mut MarkState) {
    ms.mode = MarkMode::Inactive;
    ms.fpste_line = None;
    ms.cpste_line = None;
    ms.pst_pos = 1;
}

// ──────────────────────────────────────────────────────────────────────────────
// copy – move the select buffer into paste_buff
// ──────────────────────────────────────────────────────────────────────────────

/// Copy the marked region into the paste buffer.
pub fn copy(ms: &mut MarkState) -> bool {
    if ms.mode == MarkMode::Inactive {
        return false;
    }
    let fpste = match ms.fpste_line.take() {
        Some(f) => f,
        None => { unmark_text(ms); return false; }
    };

    match ms.mode {
        MarkMode::Mark => {
            ms.paste_buff = Some(fpste);
        }
        MarkMode::Append => {
            // Append fpste chain onto the end of paste_buff
            if let Some(ref pb) = ms.paste_buff.clone() {
                let mut tail = pb.clone();
                loop {
                    let next = tail.borrow().next_line.clone();
                    match next {
                        Some(n) => tail = n,
                        None => break,
                    }
                }
                tail.borrow_mut().next_line = Some(fpste.clone());
                fpste.borrow_mut().prev_line = Some(tail);
            } else {
                ms.paste_buff = Some(fpste);
            }
        }
        MarkMode::Prefix => {
            // Prepend fpste chain before paste_buff
            let mut tail = fpste.clone();
            loop {
                let next = tail.borrow().next_line.clone();
                match next {
                    Some(n) => tail = n,
                    None => break,
                }
            }
            if let Some(ref pb) = ms.paste_buff {
                tail.borrow_mut().next_line = Some(pb.clone());
                pb.borrow_mut().prev_line = Some(tail);
            }
            ms.paste_buff = Some(fpste);
        }
        MarkMode::Inactive => {}
    }

    ms.mode = MarkMode::Inactive;
    ms.cpste_line = None;
    ms.pst_pos = 1;
    true
}

// ──────────────────────────────────────────────────────────────────────────────
// Collect marked text from buffer
// ──────────────────────────────────────────────────────────────────────────────

/// Collect the text of the current line from `start_pos` (0-based) to `end_pos`
/// into a new paste line node.
fn collect_partial_line(
    line_rc: &Rc<RefCell<TextLine>>,
    start: usize,
    end: usize,
) -> Rc<RefCell<TextLine>> {
    let text = {
        let l = line_rc.borrow();
        let s = start.min(l.line.len());
        let e = end.min(l.line.len());
        l.line[s..e].to_string()
    };
    let new_rc = txtalloc();
    {
        let mut n = new_rc.borrow_mut();
        n.line = text;
        n.line_length = n.line.len() as i32 + 1;
        n.max_length = n.line_length + 10;
    }
    new_rc
}

// ──────────────────────────────────────────────────────────────────────────────
// mark_collect – build select buffer from anchor to cursor
// ──────────────────────────────────────────────────────────────────────────────

/// Build the select buffer from `anchor_line/anchor_pos` to the current
/// cursor position.  Returns the first line of the newly built select buffer.
pub fn mark_collect(
    buff: &Buffer,
    anchor_line: Rc<RefCell<TextLine>>,
    anchor_pos: usize,
) -> Option<Rc<RefCell<TextLine>>> {
    let cursor_line_rc = buff.curr_line.as_ref()?.clone();
    let cursor_pos = (buff.position as usize).saturating_sub(1);

    // Determine direction: anchor is start, cursor is end (or vice-versa).
    // Walk from anchor_line toward cursor_line collecting text.

    let first_node = collect_partial_line(
        &anchor_line,
        anchor_pos,
        anchor_line.borrow().line.len(),
    );
    let mut tail = first_node.clone();

    // Are they the same line?
    if Rc::ptr_eq(&anchor_line, &cursor_line_rc) {
        let text = {
            let l = anchor_line.borrow();
            let s = anchor_pos.min(l.line.len());
            let e = cursor_pos.min(l.line.len());
            if s <= e { l.line[s..e].to_string() } else { l.line[e..s].to_string() }
        };
        let single = txtalloc();
        {
            let mut n = single.borrow_mut();
            n.line = text;
            n.line_length = n.line.len() as i32 + 1;
            n.max_length = n.line_length + 10;
        }
        return Some(single);
    }

    // Walk forward from anchor_line to cursor_line
    let mut cur = anchor_line.borrow().next_line.clone();
    while let Some(line_rc) = cur {
        let is_last = Rc::ptr_eq(&line_rc, &cursor_line_rc);
        let end_pos = if is_last { cursor_pos } else { line_rc.borrow().line.len() };
        let node = collect_partial_line(&line_rc, 0, end_pos);
        node.borrow_mut().prev_line = Some(tail.clone());
        tail.borrow_mut().next_line = Some(node.clone());
        tail = node;
        if is_last { break; }
        cur = line_rc.borrow().next_line.clone();
    }

    Some(first_node)
}

// ──────────────────────────────────────────────────────────────────────────────
// cut – cut marked text out of the buffer
// ──────────────────────────────────────────────────────────────────────────────

/// Cut the marked region: collect it into paste_buff and delete from buffer.
pub fn cut(
    ms: &mut MarkState,
    buff: &mut Buffer,
    anchor_line: Rc<RefCell<TextLine>>,
    anchor_pos: usize,
) -> bool {
    if ms.mode == MarkMode::Inactive { return false; }

    // Collect into select buffer
    if let Some(collected) = mark_collect(buff, anchor_line.clone(), anchor_pos) {
        ms.fpste_line = Some(collected);
        ms.cpste_line = ms.fpste_line.clone();
    }

    // Delete the region from the buffer
    // Simple approach: delete lines from anchor to cursor
    let cursor_line_rc = match buff.curr_line.as_ref() {
        Some(l) => l.clone(),
        None => return false,
    };

    if Rc::ptr_eq(&anchor_line, &cursor_line_rc) {
        // Single line: just delete the range
        let start = anchor_pos;
        let end = (buff.position as usize).saturating_sub(1);
        let (s, e) = if start <= end { (start, end) } else { (end, start) };
        {
            let mut line = cursor_line_rc.borrow_mut();
            if e <= line.line.len() {
                line.line.replace_range(s..e, "");
                line.line_length = line.line.len() as i32 + 1;
                line.changed = true;
            }
        }
        buff.position = (s + 1) as i32;
        buff.scr_horz = s as i32;
        buff.scr_pos  = buff.scr_horz;
        buff.abs_pos  = buff.scr_pos;
        buff.changed  = true;
    } else {
        // Multiple lines: truncate anchor line, remove intermediate lines,
        // truncate cursor line, then join anchor and cursor lines.
        {
            let mut al = anchor_line.borrow_mut();
            al.line.truncate(anchor_pos);
            al.line_length = al.line.len() as i32 + 1;
            al.changed = true;
        }
        // Remove lines between anchor and cursor
        loop {
            let next = anchor_line.borrow().next_line.clone();
            match next {
                None => break,
                Some(ref n) if Rc::ptr_eq(n, &cursor_line_rc) => break,
                Some(n) => {
                    // Unlink n
                    let nn = n.borrow().next_line.clone();
                    anchor_line.borrow_mut().next_line = nn.clone();
                    if let Some(ref nn2) = nn {
                        nn2.borrow_mut().prev_line = Some(anchor_line.clone());
                    }
                    buff.num_of_lines -= 1;
                }
            }
        }
        // Truncate cursor line from start to cursor_pos
        {
            let cursor_pos = (buff.position as usize).saturating_sub(1);
            let mut cl = cursor_line_rc.borrow_mut();
            let rest = if cursor_pos <= cl.line.len() {
                cl.line[cursor_pos..].to_string()
            } else {
                String::new()
            };
            cl.line = rest;
            cl.line_length = cl.line.len() as i32 + 1;
            cl.changed = true;
        }
        // Join cursor line onto anchor line
        let rest = cursor_line_rc.borrow().line.clone();
        {
            let mut al = anchor_line.borrow_mut();
            al.line.push_str(&rest);
            al.line_length = al.line.len() as i32 + 1;
            al.next_line = cursor_line_rc.borrow().next_line.clone();
            if let Some(ref nn) = al.next_line.clone() {
                nn.borrow_mut().prev_line = Some(anchor_line.clone());
            }
        }
        buff.curr_line = Some(anchor_line.clone());
        buff.position = (anchor_pos + 1) as i32;
        buff.scr_horz = anchor_pos as i32;
        buff.scr_pos  = buff.scr_horz;
        buff.abs_pos  = buff.scr_pos;
        buff.changed  = true;
        buff.num_of_lines -= 1;
    }

    // Move select buffer to paste buffer
    copy(ms);
    true
}

// ──────────────────────────────────────────────────────────────────────────────
// paste – insert paste buffer at current cursor position
// ──────────────────────────────────────────────────────────────────────────────

/// Insert the paste buffer at the current cursor position.
pub fn paste(ms: &MarkState, buff: &mut Buffer) -> bool {
    let paste_first = match ms.paste_buff.as_ref() {
        Some(p) => p.clone(),
        None => return false,
    };
    if ms.mode != MarkMode::Inactive { return false; }

    // Walk the paste buffer chain, inserting each line
    let mut pline = paste_first;
    loop {
        let text = pline.borrow().line.clone();
        let has_next = pline.borrow().next_line.is_some();

        // Insert characters from this paste line at the cursor
        delete_ops::insert_string(buff, &text);

        if has_next {
            // Split at cursor and move to next line
            split_line_at_cursor(buff);
            let next_p = pline.borrow().next_line.clone().unwrap();
            pline = next_p;
        } else {
            break;
        }
    }
    true
}

/// Split the current line at the cursor, moving the rest to a new next-line.
fn split_line_at_cursor(buff: &mut Buffer) {
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
    let new_line = txtalloc();
    {
        let mut nl = new_line.borrow_mut();
        nl.line = rest;
        nl.line_length = nl.line.len() as i32 + 1;
        nl.max_length = nl.line_length + 10;
        nl.line_number = line_rc.borrow().line_number + 1;
        nl.prev_line = Some(line_rc.clone());
        nl.next_line = line_rc.borrow().next_line.clone();
    }
    if let Some(ref nx) = new_line.borrow().next_line.clone() {
        nx.borrow_mut().prev_line = Some(new_line.clone());
    }
    line_rc.borrow_mut().next_line = Some(new_line.clone());
    buff.curr_line = Some(new_line);
    buff.num_of_lines += 1;
    buff.absolute_lin += 1;
    buff.position = 1;
    buff.scr_horz = 0;
    buff.scr_pos  = 0;
    buff.abs_pos  = 0;
    let (_, height) = crate::ui::get_terminal_size();
    let text_height = (height as i32) - 1;
    if buff.scr_vert < text_height - 1 { buff.scr_vert += 1; }
    else { buff.window_top += 1; }
    buff.changed = true;
}

// ──────────────────────────────────────────────────────────────────────────────
// Convenience: get anchor info for integration with main.rs
// ──────────────────────────────────────────────────────────────────────────────

/// Snapshot of where marking started (stored by main.rs when slct is called).
pub struct MarkAnchor {
    pub line: Rc<RefCell<TextLine>>,
    pub pos: usize, // 0-based byte offset
}

impl MarkAnchor {
    pub fn from_buffer(buff: &Buffer) -> Option<Self> {
        let line = buff.curr_line.as_ref()?.clone();
        let pos = (buff.position as usize).saturating_sub(1);
        Some(MarkAnchor { line, pos })
    }
}
