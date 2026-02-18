// Another Easy Editor - Rust version
// Converted from the original C implementation

mod editor_state;
mod text;
mod buffer;
mod ui;
mod file_ops;
mod highlighting;
mod lsp;
mod motion;
mod delete_ops;
mod search;
mod format;
mod mark;
mod journal;
mod help;
mod windows;

use std::env;
use std::rc::Rc;
use crossterm::event::{KeyCode, KeyModifiers};

#[tokio::main]
async fn main() {
    let args: Vec<String> = env::args().collect();

    let mut editor = editor_state::EditorState::new();
    editor.parse_options(&args);
    editor.initialize().await;

    // Suppress unused field warnings
    editor.use_unused_fields();

    // Initialize terminal UI
    if let Err(e) = ui::init_ui() {
        eprintln!("Failed to initialize UI: {}", e);
        return;
    }

    // If a file was given on the command line, load it (or open a blank buffer
    // for a brand-new path).  With no argument the editor starts with an
    // empty, unnamed buffer – the user can save it later via Ctrl+S, which
    // will prompt for a file name.
    if !editor.files.is_empty() {
        let file_name = editor.files[0].clone();
        editor.load_file(&file_name);
    }
    // No-file case: initialize() already created a blank buffer with one
    // empty line; curr_buff is Some(...) so draw_screen will render it and
    // show "[No File]" in the status bar.

    // Main editing loop
    loop {
        // Poll any pending LSP messages (non-blocking)
        if let Some(lsp) = &mut editor.lsp_client {
            lsp.poll_messages();
        }

        // Draw the screen
        if let Err(e) = draw_screen(&editor) {
            eprintln!("Failed to draw screen: {}", e);
            break;
        }

        // Read input
        match ui::read_key() {
            Ok(key) => {
                match key.code {
                    // ── All Ctrl+Char bindings must come BEFORE the generic Char(c) arm ──
                    KeyCode::Char('q') if key.modifiers.contains(KeyModifiers::CONTROL) => {
                        break; // Ctrl+Q – quit
                    }
                    KeyCode::Char('s') if key.modifiers.contains(KeyModifiers::CONTROL) => {
                        save_file(&mut editor); // Ctrl+S – save
                    }
                    KeyCode::Char('z') if key.modifiers.contains(KeyModifiers::CONTROL) => {
                        undo(&mut editor); // Ctrl+Z – undo
                    }
                    KeyCode::Char('t') if key.modifiers.contains(KeyModifiers::CONTROL) => {
                        if let Some(buff_rc) = editor.curr_buff.clone() {
                            motion::top(&mut *buff_rc.borrow_mut()); // Ctrl+T – top
                        }
                    }
                    KeyCode::Char('b') if key.modifiers.contains(KeyModifiers::CONTROL) => {
                        if let Some(buff_rc) = editor.curr_buff.clone() {
                            motion::bottom(&mut *buff_rc.borrow_mut()); // Ctrl+B – bottom
                        }
                    }
                    KeyCode::Char('w') if key.modifiers.contains(KeyModifiers::CONTROL) => {
                        if let Some(buff_rc) = editor.curr_buff.clone() {
                            delete_ops::del_word(&mut *buff_rc.borrow_mut()); // Ctrl+W – del word
                        }
                    }
                    KeyCode::Char('k') if key.modifiers.contains(KeyModifiers::CONTROL) => {
                        if let Some(buff_rc) = editor.curr_buff.clone() {
                            delete_ops::del_to_eol(&mut *buff_rc.borrow_mut()); // Ctrl+K – kill to eol
                        }
                    }
                    KeyCode::Char('f') if key.modifiers.contains(KeyModifiers::CONTROL) => {
                        let s = get_user_input("Search: ");
                        if !s.is_empty() {
                            editor.srch_str = Some(s.clone());
                            if let Some(buff_rc) = editor.curr_buff.clone() {
                                search::search_forward(
                                    &mut *buff_rc.borrow_mut(), &s, editor.case_sen);
                            }
                        }
                    }
                    KeyCode::Char('n') if key.modifiers.contains(KeyModifiers::CONTROL) => {
                        if let Some(ref s) = editor.srch_str.clone() {
                            if let Some(buff_rc) = editor.curr_buff.clone() {
                                search::search_forward(
                                    &mut *buff_rc.borrow_mut(), s, editor.case_sen);
                            }
                        }
                    }
                    // ── Generic printable character – must come last among Char arms ──
                    KeyCode::Char(c) => {
                        insert_char(&mut editor, c);
                    }
                    KeyCode::Enter => {
                        // New line
                        insert_newline(&mut editor);
                    }
                    KeyCode::Backspace => {
                        // Delete character
                        delete_char(&mut editor);
                    }
                    KeyCode::Left => {
                        if key.modifiers.contains(KeyModifiers::CONTROL) {
                            move_word_left(&mut editor);
                        } else {
                            move_cursor_left(&mut editor);
                        }
                    }
                    KeyCode::Right => {
                        if key.modifiers.contains(KeyModifiers::CONTROL) {
                            move_word_right(&mut editor);
                        } else {
                            move_cursor_right(&mut editor);
                        }
                    }
                    KeyCode::Home => {
                        // Move to line start
                        move_line_start(&mut editor);
                    }
                    KeyCode::End => {
                        // Move to line end
                        move_line_end(&mut editor);
                    }
                    KeyCode::Up => move_cursor_up(&mut editor),
                    KeyCode::Down => move_cursor_down(&mut editor),
                    KeyCode::PageUp => {
                        if let Some(buff_rc) = editor.curr_buff.clone() {
                            motion::prev_page(&mut *buff_rc.borrow_mut());
                        }
                    }
                    KeyCode::PageDown => {
                        if let Some(buff_rc) = editor.curr_buff.clone() {
                            motion::next_page(&mut *buff_rc.borrow_mut());
                        }
                    }
                    KeyCode::Esc => {
                        if let Some(selected) = show_main_menu() {
                            match selected {
                                0 => save_file(&mut editor),
                                1 => break,
                                2 => {
                                    let search_str = get_user_input("Search: ");
                                    if !search_str.is_empty() {
                                        search_forward(&mut editor, &search_str);
                                    }
                                }
                                3 => {
                                    let search_str = get_user_input("Search: ");
                                    let replace_str = get_user_input("Replace: ");
                                    if !search_str.is_empty() {
                                        replace_next(&mut editor, &search_str, &replace_str);
                                    }
                                }
                                4 => help::help(None), // Help
                                _ => {}
                            }
                        }
                    }
                    _ => {
                        // Ignore other keys for now
                    }
                }
            }
            Err(e) => {
                eprintln!("Failed to read key: {}", e);
                break;
            }
        }
    }

    // Cleanup UI
    if let Err(e) = ui::cleanup_ui() {
        eprintln!("Failed to cleanup UI: {}", e);
    }
}

fn draw_screen(editor: &editor_state::EditorState) -> Result<(), Box<dyn std::error::Error>> {
    ui::clear_screen()?;

    let (width, height) = ui::get_terminal_size();
    let info_height = 1u16;
    let text_start_y = info_height;

    // ── Info / status bar ────────────────────────────────────────────────────
    let info_text = if let Some(buff) = &editor.curr_buff {
        let buff = buff.borrow();
        let file_label = buff.file_name.as_deref().unwrap_or("[No File]");
        let changed_mark = if buff.changed { " [+]" } else { "" };
        format!(
            " aee  {}{}  |  Ln {} Col {}  |  ^Q Quit  ^S Save  ESC Menu",
            file_label, changed_mark, buff.absolute_lin, buff.position - 1
        )
    } else {
        " aee  |  ^Q Quit  ^S Save  ESC Menu".to_string()
    };
    ui::print_status_bar(0, &info_text, width)?;

    // ── Determine language for syntax highlighting ───────────────────────────
    let lang: &str = if let Some(buff) = &editor.curr_buff {
        let buff = buff.borrow();
        // We can't return a reference into the borrowed buff, so derive it here
        // by resolving the extension from the file name stored in the buffer.
        if let Some(ref fname) = buff.file_name {
            highlighting::lang_from_extension(fname)
        } else {
            "text"
        }
    } else {
        "text"
    };

    // ── Text area ─────────────────────────────────────────────────────────────
    if let Some(buff) = &editor.curr_buff {
        let buff = buff.borrow();
        let mut y = text_start_y;
        let mut current_line = buff.first_line.clone();
        let mut line_num = 1;

        // Skip to window_top
        while line_num < buff.window_top {
            if let Some(line) = current_line {
                current_line = line.borrow().next_line.clone();
                line_num += 1;
            } else {
                break;
            }
        }

        // Draw lines with stateful syntax highlighting (tracks /* */ block comments)
        let mut in_block_comment = false;
        while let Some(line) = current_line {
            let line_data = line.borrow();
            let raw = &line_data.line;
            // Truncate to terminal width (byte-safe)
            let display_text: &str = if raw.len() > width as usize {
                let mut end = width as usize;
                while !raw.is_char_boundary(end) { end -= 1; }
                &raw[..end]
            } else {
                raw.as_str()
            };

            if !editor.nohighlight && lang != "text" {
                let (spans, new_state) =
                    highlighting::highlight_line_with_state(display_text, lang, in_block_comment);
                in_block_comment = new_state;
                ui::print_highlighted(0, y, &spans)?;
            } else {
                ui::print_at(0, y, display_text)?;
            }

            y += 1u16;
            if y >= height { break; }
            current_line = line_data.next_line.clone();
        }
    }

    // ── Reposition cursor ────────────────────────────────────────────────────
    if let Some(buff) = &editor.curr_buff {
        let buff = buff.borrow();
        let cursor_y = buff.scr_vert as u16 + text_start_y;
        ui::move_cursor(buff.scr_horz as u16, cursor_y)?;
    }

    Ok(())
}

fn insert_char(editor: &mut editor_state::EditorState, ch: char) {
    // Stub implementation
    if let Some(buff) = &editor.curr_buff {
        let mut buff = buff.borrow_mut();
        let curr_line = buff.curr_line.clone();
        if let Some(line) = curr_line {
            let mut line = line.borrow_mut();
            let pos = buff.position as usize;
            if pos <= line.line.len() {
                line.line.insert(pos, ch);
                line.line_length = line.line.len() as i32;
                buff.position += 1;
                buff.abs_pos += 1;
                buff.scr_horz += 1;
                // Record undo
                editor.last_action = Some(crate::editor_state::LastAction::InsertChar {
                    line: buff.curr_line.clone().unwrap(),
                    pos: (buff.position - 1) as usize,
                });
            }
        }
    }
}

fn insert_newline(editor: &mut editor_state::EditorState) {
    if let Some(buff) = &editor.curr_buff {
        let mut buff = buff.borrow_mut();
        let curr_line = buff.curr_line.clone();
        if let Some(line_rc) = curr_line {
            let mut line = line_rc.borrow_mut();
            let pos = buff.position as usize;
            if pos <= line.line.len() {
                // Split the line
                let rest = line.line.split_off(pos);
                line.line_length = line.line.len() as i32;

                // Create new line
                let new_line = crate::text::txtalloc();
                {
                    let mut new_line = new_line.borrow_mut();
                    new_line.line = rest;
                    new_line.line_length = new_line.line.len() as i32;
                    new_line.max_length = new_line.line_length + 10;
                    new_line.line_number = line.line_number + 1;
                    new_line.vert_len = 1;
                }

                // Update links
                new_line.borrow_mut().prev_line = Some(line_rc.clone());
                new_line.borrow_mut().next_line = line.next_line.clone();
                if let Some(ref next) = line.next_line {
                    next.borrow_mut().prev_line = Some(new_line.clone());
                }
                line.next_line = Some(new_line.clone());

                // Update buffer
                buff.curr_line = Some(new_line);
                buff.num_of_lines += 1;
                buff.position = 1;
                buff.abs_pos = 1;
                buff.scr_horz = 0;
                buff.scr_vert += 1;
                buff.absolute_lin += 1;

                // TODO: Update line numbers for subsequent lines
            }
        }
    }
}

fn delete_char(editor: &mut editor_state::EditorState) {
    // Stub implementation
    if let Some(buff) = &editor.curr_buff {
        let mut buff = buff.borrow_mut();
        let curr_line = buff.curr_line.clone();
        if let Some(line) = curr_line {
            let mut line = line.borrow_mut();
            let pos = buff.position as usize;
            if pos > 0 && pos <= line.line.len() {
                let ch = line.line.chars().nth(pos - 1).unwrap_or('\0');
                // Record undo
                editor.last_action = Some(crate::editor_state::LastAction::DeleteChar {
                    line: buff.curr_line.clone().unwrap(),
                    pos: pos - 1,
                    ch,
                });
                line.line.remove(pos - 1);
                line.line_length = line.line.len() as i32;
                buff.position -= 1;
                buff.abs_pos -= 1;
                buff.scr_horz -= 1;
            }
        }
    }
}

fn move_word_left(editor: &mut editor_state::EditorState) {
    if let Some(buff) = &editor.curr_buff {
        let mut buff = buff.borrow_mut();
        let curr_line = buff.curr_line.clone();
        if let Some(line) = curr_line {
            let line = line.borrow();
            let pos = buff.position as usize;
            // Skip whitespace backwards, then skip non-whitespace
            let chars: Vec<char> = line.line.chars().collect();
            let mut p = pos.saturating_sub(1);
            while p > 0 && chars.get(p).map_or(true, |c| c.is_whitespace()) {
                p -= 1;
            }
            while p > 0 && !chars.get(p - 1).map_or(true, |c| c.is_whitespace()) {
                p -= 1;
            }
            let diff = (buff.position as usize).saturating_sub(p + 1);
            buff.position = (p + 1) as i32;
            buff.abs_pos = buff.position;
            buff.scr_horz -= diff as i32;
        }
    }
}

fn move_word_right(editor: &mut editor_state::EditorState) {
    if let Some(buff) = &editor.curr_buff {
        let mut buff = buff.borrow_mut();
        let curr_line = buff.curr_line.clone();
        if let Some(line) = curr_line {
            let line = line.borrow();
            let pos = buff.position as usize;
            let chars: Vec<char> = line.line.chars().collect();
            let len = chars.len();
            let mut p = pos;
            // Skip non-whitespace, then skip whitespace
            while p < len && !chars[p].is_whitespace() {
                p += 1;
            }
            while p < len && chars[p].is_whitespace() {
                p += 1;
            }
            let diff = p.saturating_sub(pos);
            buff.position = (p + 1) as i32;
            buff.abs_pos = buff.position;
            buff.scr_horz += diff as i32;
        }
    }
}

fn move_line_start(editor: &mut editor_state::EditorState) {
    if let Some(buff) = &editor.curr_buff {
        let mut buff = buff.borrow_mut();
        buff.position = 1;
        buff.abs_pos = 1;
        buff.scr_horz = 0;
    }
}

fn move_line_end(editor: &mut editor_state::EditorState) {
    if let Some(buff) = &editor.curr_buff {
        let mut buff = buff.borrow_mut();
        let curr_line = buff.curr_line.clone();
        if let Some(line) = curr_line {
            let line = line.borrow();
            let len = line.line_length;
            buff.position = len + 1;
            buff.abs_pos = buff.position;
            buff.scr_horz = len;
        }
    }
}

fn move_cursor_left(editor: &mut editor_state::EditorState) {
    if let Some(buff) = &editor.curr_buff {
        let mut buff = buff.borrow_mut();
        if buff.position > 1 {
            buff.position -= 1;
            buff.abs_pos -= 1;
            buff.scr_horz -= 1;
        }
    }
}

fn move_cursor_right(editor: &mut editor_state::EditorState) {
    if let Some(buff) = &editor.curr_buff {
        let mut buff = buff.borrow_mut();
        let curr_line = buff.curr_line.clone();
        if let Some(line) = curr_line {
            let line = line.borrow();
            if buff.position < line.line_length {
                buff.position += 1;
                buff.abs_pos += 1;
                buff.scr_horz += 1;
            }
        }
    }
}

fn move_cursor_up(editor: &mut editor_state::EditorState) {
    if let Some(buff) = &editor.curr_buff {
        let mut buff = buff.borrow_mut();
        let curr_line = buff.curr_line.clone();
        if let Some(line) = curr_line {
            let line = line.borrow();
            if let Some(ref prev_line) = line.prev_line {
                buff.curr_line = Some(prev_line.clone());
                let prev_line_len = prev_line.borrow().line_length;
                if buff.position > prev_line_len + 1 {
                    buff.position = prev_line_len + 1;
                    buff.abs_pos = buff.position;
                    buff.scr_horz = buff.position as i32 - 1;
                }
                if buff.scr_vert > 0 {
                    buff.scr_vert -= 1;
                } else if buff.window_top > 1 {
                    buff.window_top -= 1;
                }
                buff.absolute_lin -= 1;
            }
        }
    }
}

// fn move_cursor_down(editor: &mut editor_state::EditorState) {
//     if let Some(buff) = &editor.curr_buff {
//         let mut buff = buff.borrow_mut();
//         let curr_line = buff.curr_line.clone();
//         if let Some(line) = curr_line {
//             let line = line.borrow();
//             if let Some(ref next_line) = line.next_line {
//                 buff.curr_line = Some(next_line.clone());
//                 let next_line_len = next_line.borrow().line_length;
//                 if buff.position > next_line_len + 1 {
//                     buff.position = next_line_len + 1;
//                     buff.abs_pos = buff.position;
//                     buff.scr_horz = buff.position as i32 - 1;
//                 }
//                 let (_, height) = ui::get_terminal_size();
//                 let text_height = height as i32;
//                 let max_scr_vert = text_height - 1;
//                 if buff.scr_vert < max_scr_vert {
//                     buff.scr_vert += 1;
//                 } else {
//                     buff.window_top += 1;
//                 }
//                 buff.absolute_lin += 1;
//             }
//         }
//     }
// }

fn move_cursor_down(editor: &mut editor_state::EditorState) {
    if let Some(buff) = &editor.curr_buff {
        let mut buff = buff.borrow_mut();
        let curr_line = buff.curr_line.clone();
        if let Some(line) = curr_line {
            let line = line.borrow();
            if let Some(ref next_line) = line.next_line {
                buff.curr_line = Some(next_line.clone());
                let next_line_len = next_line.borrow().line_length;
                if buff.position > next_line_len + 1 {
                    buff.position = next_line_len + 1;
                    buff.abs_pos = buff.position;
                    buff.scr_horz = buff.position as i32 - 1;
                }
                let (_, height) = ui::get_terminal_size();
                let text_height = height as i32 - 1; // info height
                if buff.scr_vert < text_height - 1 {
                    buff.scr_vert += 1;
                } else {
                    buff.window_top += 1;
                }
                buff.absolute_lin += 1;
            }
        }
    }
}

fn save_file(editor: &mut editor_state::EditorState) {
    // If the buffer has no file name (opened without an argument), ask the
    // user for one before writing.
    let needs_name = editor.curr_buff
        .as_ref()
        .map(|b| b.borrow().file_name.is_none())
        .unwrap_or(false);

    if needs_name {
        let name = get_user_input("Save as: ");
        if name.is_empty() {
            return; // user cancelled
        }
        if let Some(ref buff_rc) = editor.curr_buff {
            let mut buff = buff_rc.borrow_mut();
            buff.file_name  = Some(name.clone());
            buff.full_name  = Some(crate::file_ops::get_full_path(&name, ""));
        }
    }

    if let Some(buff_rc) = &editor.curr_buff {
        let buff = buff_rc.borrow();
        if let Some(file_name) = &buff.file_name {
            let mut contents = String::new();
            let mut current_line = buff.first_line.clone();
            while let Some(line) = current_line {
                let line_data = line.borrow();
                contents.push_str(&line_data.line);
                contents.push('\n');
                current_line = line_data.next_line.clone();
            }
            // Remove trailing newline added after the last line
            if contents.ends_with('\n') && buff.num_of_lines > 0 {
                contents.pop();
            }
            let file_name = file_name.clone();
            drop(buff); // release borrow before the mutable borrow below
            if let Err(e) = std::fs::write(&file_name, contents) {
                eprintln!("Failed to save file: {}", e);
            } else {
                buff_rc.borrow_mut().changed = false;
            }
        }
    }
}

fn get_user_input(prompt: &str) -> String {
    ui::clear_screen().unwrap();
    ui::print_at(0, 0, prompt).unwrap();
    let mut input = String::new();
    let mut cursor_pos = prompt.len();
    ui::move_cursor(cursor_pos as u16, 0).unwrap();

    loop {
        match ui::read_key().unwrap().code {
            KeyCode::Char(c) => {
                input.push(c);
                cursor_pos += 1;
            }
            KeyCode::Backspace => {
                if !input.is_empty() {
                    input.pop();
                    cursor_pos -= 1;
                }
            }
            KeyCode::Enter => break,
            KeyCode::Esc => {
                input.clear();
                break;
            }
            _ => {}
        }
        ui::clear_screen().unwrap();
        ui::print_at(0, 0, &format!("{}{}", prompt, input)).unwrap();
        ui::move_cursor(cursor_pos as u16, 0).unwrap();
    }
    input
}

fn search_forward(editor: &mut editor_state::EditorState, search_str: &str) {
    if let Some(buff) = &editor.curr_buff {
        let mut buff = buff.borrow_mut();
        let mut current_line = buff.curr_line.clone();
        let mut line_offset = 0;
        while let Some(line) = current_line {
            let line_data = line.borrow();
            let start_pos = if line_offset == 0 { (buff.position as usize).saturating_sub(1) } else { 0 };
            if let Some(pos) = line_data.line[start_pos..].find(search_str) {
                let actual_pos = start_pos + pos;
                buff.position = (actual_pos + 1) as i32;
                buff.abs_pos = buff.position;
                buff.scr_horz = buff.position as i32 - 1;
                if line_offset > 0 {
                    buff.scr_vert += line_offset;
                    buff.absolute_lin += line_offset;
                }
                return;
            }
            current_line = line_data.next_line.clone();
            line_offset += 1;
        }
    }
}

fn replace_next(editor: &mut editor_state::EditorState, search_str: &str, replace_str: &str) {
    if let Some(buff) = &editor.curr_buff {
        let mut buff = buff.borrow_mut();
        let mut current_line = buff.curr_line.clone();
        let mut line_offset = 0;
        while let Some(line) = current_line {
            let mut line_data = line.borrow_mut();
            let start_pos = if line_offset == 0 { (buff.position as usize).saturating_sub(1) } else { 0 };
            if let Some(pos) = line_data.line[start_pos..].find(search_str) {
                let actual_pos = start_pos + pos;
                line_data.line.replace_range(actual_pos..actual_pos + search_str.len(), replace_str);
                line_data.line_length = line_data.line.len() as i32;
                buff.position = (actual_pos + replace_str.len() + 1) as i32;
                buff.abs_pos = buff.position;
                buff.scr_horz = buff.position as i32 - 1;
                if line_offset > 0 {
                    buff.scr_vert += line_offset;
                    buff.absolute_lin += line_offset;
                }
                return;
            }
            current_line = line_data.next_line.clone();
            line_offset += 1;
        }
    }
}

fn undo(editor: &mut editor_state::EditorState) {
    if let Some(action) = editor.last_action.take() {
        match action {
            crate::editor_state::LastAction::InsertChar { line, pos } => {
                let mut l = line.borrow_mut();
                if pos < l.line.len() {
                    l.line.remove(pos);
                    l.line_length = l.line.len() as i32;
                    if let Some(buff) = &editor.curr_buff {
                        let mut b = buff.borrow_mut();
                        if Rc::ptr_eq(&line, &b.curr_line.clone().unwrap()) && b.position > pos as i32 + 1 {
                            b.position -= 1;
                            b.abs_pos -= 1;
                            b.scr_horz -= 1;
                        }
                    }
                }
            }
            crate::editor_state::LastAction::DeleteChar { line, pos, ch } => {
                let mut l = line.borrow_mut();
                if pos <= l.line.len() {
                    l.line.insert(pos, ch);
                    l.line_length = l.line.len() as i32;
                    if let Some(buff) = &editor.curr_buff {
                        let mut b = buff.borrow_mut();
                        if Rc::ptr_eq(&line, &b.curr_line.clone().unwrap()) {
                            b.position = (pos + 1) as i32;
                            b.abs_pos = b.position;
                            b.scr_horz = b.position as i32 - 1;
                        }
                    }
                }
            }
        }
    }
}

fn show_main_menu() -> Option<usize> {
    let menu_items = vec![
        "Save File",
        "Quit",
        "Search",
        "Replace",
        "Help",
    ];

    let (width, height) = ui::get_terminal_size();
    let menu_width = 30;
    let menu_height = menu_items.len() as u16 + 4;
    let start_x = (width - menu_width) / 2;
    let start_y = (height - menu_height) / 2;

    let mut selected = 0;

    loop {
        // Clear and draw menu
        ui::clear_screen().unwrap();

        // Title
        ui::print_at(start_x + 2, start_y, "Main Menu").unwrap();

        // Separator
        for x in start_x..start_x + menu_width {
            ui::print_at(x, start_y + 1, "-").unwrap();
        }

        // Items
        for (i, &item) in menu_items.iter().enumerate() {
            let prefix = if i == selected { ">" } else { " " };
            ui::print_at(start_x + 1, start_y + 2 + i as u16, &format!("{} {}", prefix, item)).unwrap();
        }

        // Borders
        for y in start_y..start_y + menu_height {
            ui::print_at(start_x, y, "|").unwrap();
            ui::print_at(start_x + menu_width - 1, y, "|").unwrap();
        }
        for x in start_x..start_x + menu_width {
            ui::print_at(x, start_y + menu_height - 1, "-").unwrap();
        }

        // Position cursor
        ui::move_cursor(start_x + 1, start_y + 2 + selected as u16).unwrap();

        // Read key
        match ui::read_key().unwrap().code {
            KeyCode::Up => {
                if selected > 0 {
                    selected -= 1;
                }
            }
            KeyCode::Down => {
                if selected < menu_items.len() - 1 {
                    selected += 1;
                }
            }
            KeyCode::Enter => return Some(selected),
            KeyCode::Esc => return None,
            _ => {}
        }
    }
}
