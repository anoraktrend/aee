// UI module using crossterm for terminal control

use crossterm::{
    terminal::{self, ClearType},
    execute, queue,
    style::{self, Color},
    cursor,
    event::{self, DisableMouseCapture, EnableMouseCapture, Event, KeyCode, KeyEvent},
};
use std::io::stdout;

// Initialize terminal
pub fn init_ui() -> Result<(), Box<dyn std::error::Error>> {
    let mut stdout = stdout();
    execute!(stdout, terminal::EnterAlternateScreen)?;
    execute!(stdout, cursor::Hide)?;
    execute!(stdout, event::EnableMouseCapture)?;
    terminal::enable_raw_mode()?;
    Ok(())
}

// Cleanup terminal
pub fn cleanup_ui() -> Result<(), Box<dyn std::error::Error>> {
    let mut stdout = stdout();
    execute!(stdout, event::DisableMouseCapture)?;
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

// Print text at position
pub fn print_at(x: u16, y: u16, text: &str) -> Result<(), Box<dyn std::error::Error>> {
    let mut stdout = stdout();
    execute!(stdout, cursor::MoveTo(x, y))?;
    execute!(stdout, style::Print(text))?;
    Ok(())
}

// Read key input
pub fn read_key() -> Result<KeyEvent, Box<dyn std::error::Error>> {
    loop {
        if let Event::Key(key) = event::read()? {
            return Ok(key);
        }
    }
}

// Create command window (stub)
pub fn make_com_win() -> i32 {
    // Stub
    0
}

// Create help window (stub)
pub fn make_help_win() -> i32 {
    // Stub
    0
}

// Create info window (stub)
pub fn make_info_win(_height: i32) -> i32 {
    // Stub
    0
}

// Paint menu (stub)
pub fn paint_menu(_menu_list: &[String], _menu_win: i32, _max_width: i32, _max_height: i32, _current: i32) {
    // Stub
}

// Draw line on screen (stub)
pub fn draw_line(_vertical: i32, _horizontal: i32, _ptr: &str, _t_pos: i32, _dr_l: &str) {
    // Stub
}
