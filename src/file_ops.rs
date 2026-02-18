// file_ops.rs – file I/O routines
// Mirrors src/file.c (get_full_path, ae_basename, ae_dirname, resolve_name,
// get_file, write_file, diff_file, show_pwd, etc.)

use std::env;
use std::fs;
use std::io::{self, Write};
use std::path::{Path, PathBuf};
use std::os::unix::fs::MetadataExt;

use crate::editor_state::EditorState;

// ────────────────────────────────────────────────────────────────────────────
// Path utilities (mirrors file.c)
// ────────────────────────────────────────────────────────────────────────────

/// Return the basename portion of a path (mirrors C `ae_basename()`).
pub fn ae_basename(path: &str) -> String {
    Path::new(path)
        .file_name()
        .and_then(|n| n.to_str())
        .unwrap_or(path)
        .to_string()
}

/// Return the directory portion of a path or `None` if none (mirrors C `ae_dirname()`).
pub fn ae_dirname(path: &str) -> Option<String> {
    Path::new(path)
        .parent()
        .and_then(|p| p.to_str())
        .map(|s| s.to_string())
        .filter(|s| !s.is_empty())
}

/// Return the canonical full path for `path` optionally anchored at
/// `orig_path` (mirrors C `get_full_path()`).
pub fn get_full_path(path: &str, orig_path: &str) -> String {
    let base = if !orig_path.is_empty() {
        PathBuf::from(orig_path)
    } else {
        env::current_dir().unwrap_or_else(|_| PathBuf::from("."))
    };

    let full = if path.is_empty() {
        base
    } else if Path::new(path).is_absolute() {
        PathBuf::from(path)
    } else {
        base.join(path)
    };

    fs::canonicalize(&full)
        .unwrap_or(full)
        .to_str()
        .unwrap_or("")
        .to_string()
}

/// Expand `~/`, `~user/`, and `$VAR` references in a name.
/// Mirrors C `resolve_name()` in file.c.
pub fn resolve_name(name: &str) -> String {
    let expanded = expand_tilde(name);
    expand_env_vars(&expanded)
}

fn expand_tilde(name: &str) -> String {
    if name.starts_with("~/") {
        if let Ok(home) = env::var("HOME") {
            return format!("{}{}", home, &name[1..]);
        }
    } else if name.starts_with('~') {
        // ~user/... form – find the slash
        if let Some(slash_pos) = name.find('/') {
            let user_name = &name[1..slash_pos];
            // Try passwd lookup (simplified: just check $HOME if it's the current user)
            let current_user = env::var("USER").or_else(|_| env::var("LOGNAME")).unwrap_or_default();
            if user_name == current_user {
                if let Ok(home) = env::var("HOME") {
                    return format!("{}{}", home, &name[slash_pos..]);
                }
            }
            // Fall back to /home/<user>
            return format!("/home/{}{}", user_name, &name[slash_pos..]);
        }
    }
    name.to_string()
}

fn expand_env_vars(s: &str) -> String {
    let mut result = String::with_capacity(s.len() * 2);
    let mut chars  = s.chars().peekable();

    while let Some(ch) = chars.next() {
        if ch == '$' {
            let mut var_name = String::new();
            let braced = chars.peek() == Some(&'{');
            if braced { chars.next(); } // consume '{'
            while let Some(&c) = chars.peek() {
                if braced && c == '}' { chars.next(); break; }
                if !braced && (c == '/' || c == '$' || c == ' ') { break; }
                var_name.push(c);
                chars.next();
            }
            if let Ok(val) = env::var(&var_name) {
                result.push_str(&val);
            } else {
                result.push('$');
                result.push_str(&var_name);
            }
        } else {
            result.push(ch);
        }
    }
    result
}

// ────────────────────────────────────────────────────────────────────────────
// Reading files (mirrors get_file() / get_line() in file.c)
// ────────────────────────────────────────────────────────────────────────────

/// Read the contents of `file_name` and return as a String.
/// Mirrors the high-level behaviour of `get_file()` in file.c.
pub fn get_file(file_name: &str) -> io::Result<String> {
    fs::read_to_string(file_name)
}

// ────────────────────────────────────────────────────────────────────────────
// Writing files (mirrors write_file() in file.c)
// ────────────────────────────────────────────────────────────────────────────

/// Write the current buffer to `file_name`.
/// Returns `true` on success (mirrors C `write_file()` return value).
pub fn write_file(state: &mut EditorState, file_name: &str) -> bool {
    let buff_rc = match state.curr_buff.clone() { Some(b) => b, None => return false };

    let file = match fs::File::create(file_name) {
        Ok(f)  => f,
        Err(_) => return false,
    };
    let mut writer = io::BufWriter::new(file);

    let dos_file = buff_rc.borrow().dos_file;
    let first    = buff_rc.borrow().first_line.clone();
    let mut current = first;

    while let Some(line_rc) = current {
        let (content, next) = {
            let line = line_rc.borrow();
            (line.line.clone(), line.next_line.clone())
        };
        if writer.write_all(content.as_bytes()).is_err() { return false; }
        if dos_file {
            if writer.write_all(b"\r\n").is_err() { return false; }
        } else {
            if writer.write_all(b"\n").is_err()   { return false; }
        }
        current = next;
    }

    if writer.flush().is_err() { return false; }

    // Update cached stat info.
    if let Ok(meta) = fs::metadata(file_name) {
        let mut buff = buff_rc.borrow_mut();
        buff.fileinfo_mtime = meta.mtime() as u64;
        buff.fileinfo_size  = meta.size();
        buff.changed = false;
    }

    true
}

// ────────────────────────────────────────────────────────────────────────────
// show_pwd (mirrors show_pwd() in file.c)
// ────────────────────────────────────────────────────────────────────────────

/// Return the current working directory as a String.
pub fn show_pwd() -> String {
    env::current_dir()
        .map(|p| p.to_str().unwrap_or("").to_string())
        .unwrap_or_else(|_| "unknown".to_string())
}

// ────────────────────────────────────────────────────────────────────────────
// diff_file (mirrors diff_file() in file.c)
// ────────────────────────────────────────────────────────────────────────────

/// Run `diff` against the on-disk version of the current file and return
/// the output as a String. Mirrors `diff_file()` in file.c.
pub fn diff_file(state: &EditorState) -> Option<String> {
    let buff_rc  = state.curr_buff.as_ref()?;
    let full_name = buff_rc.borrow().full_name.clone()?;

    let output = std::process::Command::new("diff")
        .arg(&full_name)
        .arg("-")
        .output()
        .ok()?;

    Some(String::from_utf8_lossy(&output.stdout).to_string())
}

// ────────────────────────────────────────────────────────────────────────────
// Directory creation (mirrors create_dir() in journal.c / file.c)
// ────────────────────────────────────────────────────────────────────────────

/// Recursively create `path` if it does not already exist.
pub fn create_dir(path: &str) -> io::Result<()> {
    fs::create_dir_all(path)
}
