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

    // Initialize terminal UI (use windows module for proper raw-mode setup)
    if let Err(e) = windows::set_up_term() {
        eprintln!("Failed to initialize terminal: {}", e);
        return;
    }

    // If a file was given on the command line, load it (or open a blank buffer
    // for a brand-new path).  With no argument the editor starts with an
    // empty, unnamed buffer – the user can save it later via Ctrl+S, which
    // will prompt for a file name.
    let mut journal_file: Option<std::fs::File> = None;
    if !editor.files.is_empty() {
        let file_name = editor.files[0].clone();
        editor.load_file(&file_name);
        // Open a journal for crash recovery if journalling is on
        if editor.journ_on {
            let jpath = journal::journal_name(&file_name, None);
            if let Ok(jf) = editor.curr_buff.as_ref().map_or(
                Err(std::io::Error::new(std::io::ErrorKind::Other, "no buffer")),
                |b| journal::open_journal_for_write(&mut b.borrow_mut(), &jpath, &file_name),
            ) {
                let _ = journal::add_to_journal_db(Some(&file_name), &jpath);
                journal_file = Some(jf);
            }
        }
    }

    // Mark / cut / copy / paste state
    let mut mark_state = mark::MarkState::new();
    let mut mark_anchor: Option<mark::MarkAnchor> = None;

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
                        save_file(&mut editor, &mut journal_file); // Ctrl+S – save
                    }
                    KeyCode::Char('z') if key.modifiers.contains(KeyModifiers::CONTROL) => {
                        undo(&mut editor); // Ctrl+Z – undo
                    }
                    KeyCode::Char('t') if key.modifiers.contains(KeyModifiers::CONTROL) => {
                        if let Some(buff_rc) = editor.curr_buff.clone() {
                            motion::top(&mut *buff_rc.borrow_mut()); // Ctrl+T – top of file
                        }
                    }
                    KeyCode::Char('b') if key.modifiers.contains(KeyModifiers::CONTROL) => {
                        // Ctrl+B – search backward (was bottom; top/bottom are PageUp/Down)
                        if let Some(ref s) = editor.srch_str.clone() {
                            if let Some(buff_rc) = editor.curr_buff.clone() {
                                search::search_backward(
                                    &mut *buff_rc.borrow_mut(), s, editor.case_sen);
                            }
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
                        // Ctrl+N – repeat last search forward
                        if let Some(ref s) = editor.srch_str.clone() {
                            if let Some(buff_rc) = editor.curr_buff.clone() {
                                search::search_forward(
                                    &mut *buff_rc.borrow_mut(), s, editor.case_sen);
                            }
                        }
                    }
                    KeyCode::Char('g') if key.modifiers.contains(KeyModifiers::CONTROL) => {
                        // Ctrl+G – goto line
                        let input = get_user_input("Goto line: ");
                        if let Ok(n) = input.trim().parse::<i32>() {
                            if let Some(buff_rc) = editor.curr_buff.clone() {
                                motion::goto_line(&mut *buff_rc.borrow_mut(), n);
                            }
                        }
                    }
                    KeyCode::Char('j') if key.modifiers.contains(KeyModifiers::CONTROL) => {
                        // Ctrl+J – format / reflow paragraph
                        if let Some(buff_rc) = editor.curr_buff.clone() {
                            format::format_paragraph(
                                &mut *buff_rc.borrow_mut(),
                                editor.left_margin,
                                editor.right_margin,
                                false,
                            );
                        }
                    }
                    KeyCode::Char('a') if key.modifiers.contains(KeyModifiers::CONTROL) => {
                        // Ctrl+A – toggle auto-format (apply one auto_format pass)
                        if let Some(buff_rc) = editor.curr_buff.clone() {
                            format::auto_format(
                                &mut *buff_rc.borrow_mut(),
                                editor.right_margin,
                            );
                        }
                    }

                    // ── Mark / cut / copy / paste (F1–F4) ───────────────────────────────
                    KeyCode::F(1) => {
                        // F1 – begin mark
                        if let Some(buff_rc) = editor.curr_buff.clone() {
                            let buff = buff_rc.borrow();
                            mark::slct(&mut mark_state, &*buff, mark::MarkMode::Mark);
                            mark_anchor = mark::MarkAnchor::from_buffer(&*buff);
                        }
                    }
                    KeyCode::F(2) => {
                        // F2 – copy marked region
                        mark::copy(&mut mark_state);
                    }
                    KeyCode::F(3) => {
                        // F3 – cut marked region
                        if let Some(buff_rc) = editor.curr_buff.clone() {
                            if let Some(anchor) = mark_anchor.take() {
                                mark::cut(
                                    &mut mark_state,
                                    &mut *buff_rc.borrow_mut(),
                                    anchor.line,
                                    anchor.pos,
                                );
                            }
                        }
                    }
                    KeyCode::F(4) => {
                        // F4 – paste
                        if let Some(buff_rc) = editor.curr_buff.clone() {
                            mark::paste(&mark_state, &mut *buff_rc.borrow_mut());
                        }
                    }

                    // ── Generic printable character – must come last among Char arms ──
                    KeyCode::Char(c) => {
                        insert_char(&mut editor, c);
                        // Auto-format on space/tab if right_margin is set
                        if editor.right_margin > 0 && (c == ' ' || c == '\t') {
                            if let Some(buff_rc) = editor.curr_buff.clone() {
                                format::auto_format(
                                    &mut *buff_rc.borrow_mut(),
                                    editor.right_margin,
                                );
                            }
                        }
                    }
                    KeyCode::Enter => {
                        // New line
                        insert_newline(&mut editor);
                    }
                    KeyCode::Backspace => {
                        // Backspace: delete character before cursor using delete_ops module.
                        // Record undo before the deletion so Ctrl+Z can restore it.
                        if let Some(buff_rc) = editor.curr_buff.clone() {
                            {
                                let buff = buff_rc.borrow();
                                if let Some(ref line_rc) = buff.curr_line {
                                    let pos = buff.position as usize;
                                    if pos > 1 {
                                        let line = line_rc.borrow();
                                        if let Some(ch) = line.line.chars().nth(pos - 2) {
                                            editor.last_action = Some(
                                                crate::editor_state::LastAction::DeleteChar {
                                                    line: line_rc.clone(),
                                                    pos: pos - 2,
                                                    ch,
                                                }
                                            );
                                        }
                                    }
                                }
                            }
                            delete_ops::backspace(&mut *buff_rc.borrow_mut());
                        }
                    }
                    KeyCode::Delete => {
                        // Delete: delete character at cursor using delete_ops module
                        if let Some(buff_rc) = editor.curr_buff.clone() {
                            delete_ops::delete_forward(&mut *buff_rc.borrow_mut());
                        }
                    }
                    KeyCode::Left => {
                        if key.modifiers.contains(KeyModifiers::CONTROL) {
                            // Ctrl+Left – previous word (motion module)
                            if let Some(buff_rc) = editor.curr_buff.clone() {
                                motion::prev_word(&mut *buff_rc.borrow_mut());
                            }
                        } else {
                            // Left – move one char left (motion module)
                            if let Some(buff_rc) = editor.curr_buff.clone() {
                                motion::move_left(&mut *buff_rc.borrow_mut());
                            }
                        }
                    }
                    KeyCode::Right => {
                        if key.modifiers.contains(KeyModifiers::CONTROL) {
                            // Ctrl+Right – advance word (motion module)
                            if let Some(buff_rc) = editor.curr_buff.clone() {
                                motion::adv_word(&mut *buff_rc.borrow_mut());
                            }
                        } else {
                            // Right – move one char right (motion module)
                            if let Some(buff_rc) = editor.curr_buff.clone() {
                                motion::move_right(&mut *buff_rc.borrow_mut());
                            }
                        }
                    }
                    KeyCode::Home => {
                        // Home – beginning of line (motion module)
                        if let Some(buff_rc) = editor.curr_buff.clone() {
                            motion::bol(&mut *buff_rc.borrow_mut());
                        }
                    }
                    KeyCode::End => {
                        // End – end of line (motion module)
                        if let Some(buff_rc) = editor.curr_buff.clone() {
                            motion::eol(&mut *buff_rc.borrow_mut());
                        }
                    }
                    KeyCode::Up => {
                        // Up – move cursor up (motion module)
                        if let Some(buff_rc) = editor.curr_buff.clone() {
                            motion::move_up(&mut *buff_rc.borrow_mut());
                        }
                    }
                    KeyCode::Down => {
                        // Down – move cursor down (motion module)
                        if let Some(buff_rc) = editor.curr_buff.clone() {
                            motion::move_down(&mut *buff_rc.borrow_mut());
                        }
                    }
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
                                0 => save_file(&mut editor, &mut journal_file),
                                1 => break,
                                2 => {
                                    let search_str = get_user_input("Search: ");
                                    if !search_str.is_empty() {
                                        editor.srch_str = Some(search_str.clone());
                                        if let Some(buff_rc) = editor.curr_buff.clone() {
                                            search::search_forward(
                                                &mut *buff_rc.borrow_mut(),
                                                &search_str,
                                                editor.case_sen,
                                            );
                                        }
                                    }
                                }
                                3 => {
                                    let search_str = get_user_input("Search: ");
                                    let replace_str = get_user_input("Replace: ");
                                    if !search_str.is_empty() {
                                        editor.srch_str = Some(search_str.clone());
                                        if let Some(buff_rc) = editor.curr_buff.clone() {
                                            search::replace_next(
                                                &mut *buff_rc.borrow_mut(),
                                                &search_str,
                                                &replace_str,
                                                editor.case_sen,
                                            );
                                        }
                                    }
                                }
                                4 => {
                                    // Replace all occurrences
                                    let search_str = get_user_input("Search (replace all): ");
                                    let replace_str = get_user_input("Replace with: ");
                                    if !search_str.is_empty() {
                                        editor.srch_str = Some(search_str.clone());
                                        if let Some(buff_rc) = editor.curr_buff.clone() {
                                            let count = search::replace_all(
                                                &mut *buff_rc.borrow_mut(),
                                                &search_str,
                                                &replace_str,
                                                editor.case_sen,
                                            );
                                            let _ = ui::print_at(0, 0,
                                                &format!("Replaced {} occurrences", count));
                                        }
                                    }
                                }
                                5 => help::help(None), // Help
                                _ => {}
                            }
                        }
                    }
                    _ => {
                        // Ignore other keys for now
                    }
                }

                // Journal changed lines after every keystroke if journalling is on
                if editor.journ_on {
                    if let Some(ref mut jf) = journal_file {
                        if let Some(buff_rc) = editor.curr_buff.clone() {
                            let buff = buff_rc.borrow();
                            if let Some(ref line_rc) = buff.curr_line {
                                if line_rc.borrow().changed {
                                    let _ = journal::write_journal(jf, line_rc);
                                }
                            }
                        }
                    }
                }
            }
            Err(e) => {
                eprintln!("Failed to read key: {}", e);
                break;
            }
        }
    }

    // Remove journal on clean exit
    if editor.journ_on {
        if let Some(ref buff_rc) = editor.curr_buff {
            let buff = buff_rc.borrow();
            if let Some(ref fname) = buff.file_name {
                let jpath = journal::journal_name(fname, None);
                let _ = journal::remove_journal_file(&jpath, fname);
            }
        }
    }

    // Restore terminal (use windows module)
    if let Err(e) = windows::restore_term() {
        eprintln!("Failed to restore terminal: {}", e);
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
            " aee  {}{}  |  Ln {} Col {}  |  ^Q Quit  ^S Save  ^F Find  ^G Goto  ESC Menu",
            file_label, changed_mark, buff.absolute_lin, buff.position - 1
        )
    } else {
        " aee  |  ^Q Quit  ^S Save  ^F Find  ^G Goto  ESC Menu".to_string()
    };
    ui::print_status_bar(0, &info_text, width)?;

    // ── Determine language for syntax highlighting ───────────────────────────
    let lang: &str = if let Some(buff) = &editor.curr_buff {
        let buff = buff.borrow();
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

        // ── LSP semantic tokens for this file (if any) ──────────────────────
        // Build a `file://` URI from the buffer's canonical path so we can
        // look up cached semantic tokens produced by the language server.
        let lsp_tokens_for_file: Option<(&Vec<lsp::SemanticToken>, &Vec<String>)> =
            editor.lsp_client.as_ref().and_then(|lsp| {
                let uri = buff.full_name.as_ref().map(|p| format!("file://{}", p))?;
                let tokens = lsp.get_semantic_tokens(&uri)?;
                Some((tokens, &lsp.token_type_legend))
            });

        // Draw lines with stateful syntax highlighting (tracks /* */ block comments)
        let mut in_block_comment = false;
        // LSP uses 0-based line numbers; our window_top is 1-based.
        let mut lsp_line_idx = (buff.window_top as u32).saturating_sub(1);
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

            if !editor.nohighlight {
                if let Some((all_tokens, legend)) = lsp_tokens_for_file {
                    // Filter semantic tokens that belong to this display line.
                    let line_tokens: Vec<&lsp::SemanticToken> = all_tokens
                        .iter()
                        .filter(|t| t.line == lsp_line_idx)
                        .collect();

                    // Only use LSP path when the server actually provided tokens
                    // for this line; otherwise fall through to the local highlighter
                    // so the screen is never left blank.
                    if !line_tokens.is_empty() {
                        let owned_tokens: Vec<lsp::SemanticToken> =
                            line_tokens.into_iter().cloned().collect();
                        let spans =
                            highlighting::highlight_line_lsp(display_text, &owned_tokens, legend);
                        ui::print_highlighted_owned(0, y, &spans)?;
                        y += 1u16;
                        lsp_line_idx += 1;
                        if y >= height { break; }
                        current_line = line_data.next_line.clone();
                        continue;
                    }
                }

                if lang != "text" {
                    let (spans, new_state) =
                        highlighting::highlight_line_with_state(display_text, lang, in_block_comment);
                    in_block_comment = new_state;
                    ui::print_highlighted(0, y, &spans)?;
                } else {
                    ui::print_at(0, y, display_text)?;
                }
            } else {
                ui::print_at(0, y, display_text)?;
            }

            y += 1u16;
            lsp_line_idx += 1;
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
    if let Some(buff) = &editor.curr_buff {
        let mut buff = buff.borrow_mut();
        let curr_line = buff.curr_line.clone();
        if let Some(line) = curr_line {
            let mut line = line.borrow_mut();
            let pos = buff.position as usize;
            if pos <= line.line.len() + 1 {
                let safe_pos = pos.saturating_sub(1).min(line.line.len());
                line.line.insert(safe_pos, ch);
                line.line_length = line.line.len() as i32;
                line.changed = true;
                buff.position += 1;
                buff.abs_pos += 1;
                buff.scr_horz += 1;
                buff.changed = true;
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
            let safe_pos = pos.saturating_sub(1).min(line.line.len());

            // Split the line
            let rest = line.line.split_off(safe_pos);
            line.line_length = line.line.len() as i32;
            line.changed = true;

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
            buff.abs_pos = 0;
            buff.scr_horz = 0;
            buff.absolute_lin += 1;
            buff.changed = true;

            let (_, height) = ui::get_terminal_size();
            let text_height = (height as i32) - 1;
            if buff.scr_vert < text_height - 1 {
                buff.scr_vert += 1;
            } else {
                buff.window_top += 1;
            }
        }
    }
}

fn save_file(editor: &mut editor_state::EditorState, journal_file: &mut Option<std::fs::File>) {
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
            buff.file_name = Some(name.clone());
            buff.full_name = Some(crate::file_ops::get_full_path(&name, ""));
        }
        // Open a journal for the newly-named file
        if editor.journ_on && journal_file.is_none() {
            if let Some(ref buff_rc) = editor.curr_buff {
                let fname = buff_rc.borrow().file_name.clone().unwrap_or_default();
                let jpath = journal::journal_name(&fname, None);
                if let Ok(jf) = journal::open_journal_for_write(
                    &mut buff_rc.borrow_mut(), &jpath, &fname,
                ) {
                    let _ = journal::add_to_journal_db(Some(&fname), &jpath);
                    *journal_file = Some(jf);
                }
            }
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
                // Remove journal on successful save (clean state)
                if editor.journ_on {
                    let jpath = journal::journal_name(&file_name, None);
                    let _ = journal::remove_journal_file(&jpath, &file_name);
                    *journal_file = None;
                }
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

fn undo(editor: &mut editor_state::EditorState) {
    if let Some(action) = editor.last_action.take() {
        match action {
            crate::editor_state::LastAction::InsertChar { line, pos } => {
                let mut l = line.borrow_mut();
                if pos > 0 && pos <= l.line.len() {
                    l.line.remove(pos - 1);
                    l.line_length = l.line.len() as i32;
                    if let Some(buff) = &editor.curr_buff {
                        let mut b = buff.borrow_mut();
                        if Rc::ptr_eq(&line, &b.curr_line.clone().unwrap()) && b.position > pos as i32 {
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
        "Save File       ^S",
        "Quit            ^Q",
        "Search          ^F",
        "Replace next    ",
        "Replace all     ",
        "Help            ",
    ];

    let (width, height) = ui::get_terminal_size();
    let menu_width = 30u16;
    let menu_height = menu_items.len() as u16 + 4;
    let start_x = (width - menu_width) / 2;
    let start_y = (height - menu_height) / 2;

    let mut selected = 0;

    loop {
        // Clear and draw menu
        ui::clear_screen().unwrap();

        // Title
        ui::print_at(start_x + 2, start_y, "── Main Menu ──").unwrap();

        // Separator
        for x in start_x..start_x + menu_width {
            ui::print_at(x, start_y + 1, "─").unwrap();
        }

        // Items
        for (i, &item) in menu_items.iter().enumerate() {
            let prefix = if i == selected { "►" } else { " " };
            ui::print_at(start_x + 1, start_y + 2 + i as u16, &format!("{} {}", prefix, item)).unwrap();
        }

        // Borders
        for y in start_y..start_y + menu_height {
            ui::print_at(start_x, y, "│").unwrap();
            ui::print_at(start_x + menu_width - 1, y, "│").unwrap();
        }
        for x in start_x..start_x + menu_width {
            ui::print_at(x, start_y + menu_height - 1, "─").unwrap();
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
