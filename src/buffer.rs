// Buffer management module

use std::rc::Rc;
use std::cell::RefCell;
use crate::editor_state::Buffer;

// Allocate a new buffer
pub fn buf_alloc() -> Rc<RefCell<Buffer>> {
    Rc::new(RefCell::new(Buffer {
        name: String::new(),
        first_line: None,
        curr_line: None,
        scr_vert: 0,
        scr_horz: 0,
        scr_pos: 0,
        position: 0,
        abs_pos: 0,
        lines: 0,
        last_line: 0,
        last_col: 0,
        num_of_lines: 0,
        absolute_lin: 0,
        window_top: 0,
        file_name: None,
        full_name: None,
        changed: false,
        main_buffer: false,
        edit_buffer: false,
    }))
}
