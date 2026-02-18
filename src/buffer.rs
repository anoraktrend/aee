// Buffer management – mirrors buf_alloc() in aee.c / windows.c

use std::rc::Rc;
use std::cell::RefCell;
use crate::editor_state::Buffer;

/// Allocate and zero-initialise a new Buffer (mirrors C `buf_alloc()`).
pub fn buf_alloc() -> Rc<RefCell<Buffer>> {
    Rc::new(RefCell::new(Buffer {
        name:           String::new(),
        first_line:     None,
        next_buff:      None,
        curr_line:      None,
        scr_vert:       0,
        scr_horz:       0,
        scr_pos:        0,
        position:       1,
        abs_pos:        0,
        lines:          0,
        last_line:      0,
        last_col:       0,
        num_of_lines:   0,
        absolute_lin:   1,
        window_top:     0,
        file_name:      None,
        full_name:      None,
        orig_dir:       None,
        changed:        false,
        main_buffer:    false,
        edit_buffer:    false,
        dos_file:       false,
        journalling:    false,
        journal_file:   None,
        fileinfo_mtime: 0,
        fileinfo_size:  0,
    }))
}
