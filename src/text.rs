// Text handling module

use std::rc::Rc;
use std::cell::RefCell;
use crate::editor_state::TextLine;

// Allocate a new text line
pub fn txtalloc() -> Rc<RefCell<TextLine>> {
    Rc::new(RefCell::new(TextLine {
        line: String::new(),
        line_number: 0,
        max_length: 0,
        vert_len: 1,
        file_info: crate::editor_state::AeFileInfo {
            info_location: 0,
            prev_info: 0,
            next_info: 0,
            line_location: 0,
        },
        changed: false,
        prev_line: None,
        next_line: None,
        line_length: 0,
    }))
}

// Resize line
pub fn resiz_line(factor: i32, rline: &mut crate::editor_state::TextLine, _rpos: i32) {
    rline.max_length += factor;
    // In Rust, String automatically manages memory, but we can reserve capacity
    rline.line.reserve(rline.max_length as usize);
}

// Insert character into line
pub fn insert_char(line: &mut String, pos: usize, ch: char) {
    line.insert(pos, ch);
}

// Delete character from line
pub fn delete_char(line: &mut String, pos: usize) {
    if pos < line.len() {
        line.remove(pos);
    }
}
