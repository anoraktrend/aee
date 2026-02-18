// UI module using crossterm for terminal control

use crossterm::{
    terminal::{self, ClearType},
    execute,
    style::{self, Color, SetForegroundColor, ResetColor},
    cursor,
    event::{self, Event, KeyEvent},
};
use std::io::{stdout, Write};

use crate::highlighting::TokenKind;

// Initialize terminal
pub fn init_ui() -> Result<(), Box<dyn std::error::Error>> {
    let mut stdout = stdout();
    execute!(stdout, terminal::EnterAlternateScreen)?;
    execute!(stdout, cursor::Hide)?;
    terminal::enable_raw_mode()?;
    Ok(())
}

// Cleanup terminal
pub fn cleanup_ui() -> Result<(), Box<dyn std::error::Error>> {
    let mut stdout = stdout();
    execute!(stdout, style::ResetColor)?;
    execute!(stdout, cursor::Show)?;
    execute!(stdout, terminal::LeaveAlternateScreen)?;
    terminal::disable_raw_mode()?;
    Ok(())
}

// Get terminal size
pub fn get_terminal_size() -> (u16, u16) {
    terminal::size().unwrap_or((80, 24))
}

// Clear screen
pub fn clear_screen() -> Result<(), Box<dyn std::error::Error>> {
    let mut stdout = stdout();
    execute!(stdout, terminal::Clear(ClearType::All))?;
    Ok(())
}

// Move cursor
pub fn move_cursor(x: u16, y: u16) -> Result<(), Box<dyn std::error::Error>> {
    let mut stdout = stdout();
    execute!(stdout, cursor::MoveTo(x, y))?;
    Ok(())
}

// Print plain text at position
pub fn print_at(x: u16, y: u16, text: &str) -> Result<(), Box<dyn std::error::Error>> {
    let mut stdout = stdout();
    execute!(stdout, cursor::MoveTo(x, y))?;
    execute!(stdout, style::Print(text))?;
    Ok(())
}

/// Print a syntax-highlighted line at (x, y).
/// Each span is (text_slice, token_kind) from the highlighter.
pub fn print_highlighted(
    x: u16,
    y: u16,
    spans: &[(&str, TokenKind)],
) -> Result<(), Box<dyn std::error::Error>> {
    let mut out = stdout();
    execute!(out, cursor::MoveTo(x, y))?;
    for (text, kind) in spans {
        let color = token_color(kind);
        execute!(out, SetForegroundColor(color), style::Print(text))?;
    }
    execute!(out, ResetColor)?;
    out.flush()?;
    Ok(())
}

/// Print the status / info bar with standout (inverted) colors.
pub fn print_status_bar(y: u16, text: &str, width: u16) -> Result<(), Box<dyn std::error::Error>> {
    let mut out = stdout();
    // Pad to full width
    let padded = format!("{:<width$}", text, width = width as usize);
    execute!(
        out,
        cursor::MoveTo(0, y),
        SetForegroundColor(Color::Black),
        style::SetBackgroundColor(Color::Cyan),
        style::Print(padded),
        style::SetBackgroundColor(style::Color::Reset),
        ResetColor
    )?;
    out.flush()?;
    Ok(())
}

fn token_color(kind: &TokenKind) -> Color {
    match kind {
        TokenKind::Keyword      => Color::Yellow,
        TokenKind::Comment      => Color::DarkGrey,
        TokenKind::StringLiteral => Color::Green,
        TokenKind::Number       => Color::Cyan,
        TokenKind::Operator     => Color::Magenta,
        TokenKind::Identifier   => Color::White,
        TokenKind::Whitespace   => Color::Reset,
        TokenKind::Other        => Color::Reset,
    }
}

// Read key input
pub fn read_key() -> Result<KeyEvent, Box<dyn std::error::Error>> {
    loop {
        if let Event::Key(key) = event::read()? {
            return Ok(key);
        }
    }
}

// Stub window helpers kept for API compatibility
pub fn make_com_win() -> i32 { 0 }
pub fn make_help_win() -> i32 { 0 }
pub fn make_info_win(_height: i32) -> i32 { 0 }
pub fn paint_menu(_menu_list: &[String], _menu_win: i32, _max_width: i32, _max_height: i32, _current: i32) {}
pub fn draw_line(_vertical: i32, _horizontal: i32, _ptr: &str, _t_pos: i32, _dr_l: &str) {}
