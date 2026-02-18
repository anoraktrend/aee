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
