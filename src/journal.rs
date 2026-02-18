// journal.rs – crash-recovery journalling
// Mirrors src/journal.c (write_journal, update_journal_entry, journ_info_init,
// remove_journ_line, read_journal_entry, recover_from_journal,
// open_journal_for_write, remove_journal_file, journal_name,
// add_to_journal_db, rm_journal_db_entry, read_journal_db, write_db_file,
// lock_journal_fd, unlock_journal_fd).
//
// Public API is shaped to match calls in main.rs:
//   journal_name(fname, dir_opt)               -> String
//   open_journal_for_write(buff, jpath, fname) -> io::Result<File>
//   add_to_journal_db(fname_opt, jpath)
//   remove_journal_file(jpath, fname)
//   write_journal(jf, line_rc)

use std::fs::{self, File, OpenOptions};
use std::io::{self, Read, Seek, SeekFrom, Write};
use std::rc::Rc;
use std::cell::RefCell;
use std::time::{SystemTime, UNIX_EPOCH};

use crate::editor_state::{Buffer, TextLine, NO_FURTHER_LINES};
use crate::file_ops::{ae_basename, resolve_name};
use crate::text::txtalloc;

// ────────────────────────────────────────────────────────────────────────────
// Binary I/O helpers
// ────────────────────────────────────────────────────────────────────────────

const U64_SIZE: usize = std::mem::size_of::<u64>();
const I32_SIZE: usize = std::mem::size_of::<i32>();

fn write_u64(f: &mut File, v: u64) -> io::Result<()> { f.write_all(&v.to_ne_bytes()) }
fn write_i32(f: &mut File, v: i32) -> io::Result<()> { f.write_all(&v.to_ne_bytes()) }
fn read_u64(f: &mut File) -> io::Result<u64> {
    let mut buf = [0u8; U64_SIZE];
    f.read_exact(&mut buf)?;
    Ok(u64::from_ne_bytes(buf))
}
fn read_i32(f: &mut File) -> io::Result<i32> {
    let mut buf = [0u8; I32_SIZE];
    f.read_exact(&mut buf)?;
    Ok(i32::from_ne_bytes(buf))
}

// ────────────────────────────────────────────────────────────────────────────
// journal_name  (called as `journal::journal_name(&fname, None)`)
// ────────────────────────────────────────────────────────────────────────────

/// Compute the journal file path for `file_name`.
/// `journal_dir` is `None` to use the file's own directory, or
/// `Some(dir)` to place the journal in a specific directory.
pub fn journal_name(file_name: &str, journal_dir: Option<&str>) -> String {
    let base = if !file_name.is_empty() {
        ae_basename(file_name)
    } else {
        let secs = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .map(|d| d.as_secs())
            .unwrap_or(0);
        format!("{:014}", secs)
    };

    match journal_dir {
        Some(dir) if !dir.is_empty() => format!("{}/{}.rv", dir, base),
        _ => format!("{}.rv", base),
    }
}

// ────────────────────────────────────────────────────────────────────────────
// write_journal  (mirrors C `write_journal()`)
// ────────────────────────────────────────────────────────────────────────────

/// Write the current content of `line` to the end of the journal file, then
/// update the info record for that line.
pub fn write_journal(
    journ_fd: &mut File,
    line_rc: &Rc<RefCell<TextLine>>,
) -> io::Result<()> {
    let _ = journ_fd.seek(SeekFrom::End(0));
    let loc = journ_fd.stream_position()?;
    {
        let mut line = line_rc.borrow_mut();
        line.file_info.line_location = loc;
        let content = line.line.clone();
        journ_fd.write_all(content.as_bytes())?;
        journ_fd.write_all(b"\0")?;
    }
    update_journal_entry(journ_fd, line_rc)?;
    line_rc.borrow_mut().changed = false;
    Ok(())
}

// ────────────────────────────────────────────────────────────────────────────
// update_journal_entry / journ_info_init / remove_journ_line
// ────────────────────────────────────────────────────────────────────────────

fn update_journal_entry(
    journ_fd: &mut File,
    line_rc: &Rc<RefCell<TextLine>>,
) -> io::Result<()> {
    let fi = line_rc.borrow().file_info.clone();
    if fi.info_location == NO_FURTHER_LINES {
        journ_info_init(journ_fd, line_rc)?;
        return Ok(());
    }
    journ_fd.seek(SeekFrom::Start(fi.info_location))?;
    write_u64(journ_fd, fi.prev_info)?;
    write_u64(journ_fd, fi.next_info)?;
    write_u64(journ_fd, fi.line_location)?;
    let ll = line_rc.borrow().line_length;
    write_i32(journ_fd, ll)
}

fn journ_info_init(
    journ_fd: &mut File,
    line_rc: &Rc<RefCell<TextLine>>,
) -> io::Result<()> {
    journ_fd.seek(SeekFrom::End(0))?;
    let loc = journ_fd.stream_position()?;

    {
        let mut line = line_rc.borrow_mut();
        line.file_info.info_location = loc;

        if let Some(ref prev_rc) = line.prev_line.clone() {
            line.file_info.prev_info = prev_rc.borrow().file_info.info_location;
            prev_rc.borrow_mut().file_info.next_info = loc;
        }
        if let Some(ref next_rc) = line.next_line.clone() {
            line.file_info.next_info = next_rc.borrow().file_info.info_location;
            next_rc.borrow_mut().file_info.prev_info = loc;
        } else {
            line.file_info.next_info = NO_FURTHER_LINES;
        }
    }

    update_journal_entry(journ_fd, line_rc)?;

    let (prev_opt, next_opt) = {
        let line = line_rc.borrow();
        (line.prev_line.clone(), line.next_line.clone())
    };
    if let Some(prev_rc) = prev_opt { update_journal_entry(journ_fd, &prev_rc)?; }
    if let Some(next_rc) = next_opt {
        if line_rc.borrow().file_info.next_info != NO_FURTHER_LINES {
            update_journal_entry(journ_fd, &next_rc)?;
        }
    }
    Ok(())
}

/// Remove a line from the journal linked list (mirrors C `remove_journ_line()`).
pub fn remove_journ_line(
    journ_fd: &mut File,
    line_rc: &Rc<RefCell<TextLine>>,
) -> io::Result<()> {
    let (prev_opt, next_opt) = {
        let line = line_rc.borrow();
        (line.prev_line.clone(), line.next_line.clone())
    };

    match (prev_opt, next_opt) {
        (None, Some(ref next_rc)) => {
            let loc = line_rc.borrow().file_info.info_location;
            next_rc.borrow_mut().file_info.info_location = loc;
            update_journal_entry(journ_fd, next_rc)?;
            if let Some(ref nn) = next_rc.borrow().next_line.clone() {
                nn.borrow_mut().file_info.prev_info = loc;
                update_journal_entry(journ_fd, nn)?;
            }
        }
        (Some(ref prev_rc), None) => {
            prev_rc.borrow_mut().file_info.next_info = NO_FURTHER_LINES;
            update_journal_entry(journ_fd, prev_rc)?;
        }
        (Some(ref prev_rc), Some(ref next_rc)) => {
            let next_loc = next_rc.borrow().file_info.info_location;
            let prev_loc = prev_rc.borrow().file_info.info_location;
            prev_rc.borrow_mut().file_info.next_info = next_loc;
            next_rc.borrow_mut().file_info.prev_info = prev_loc;
            update_journal_entry(journ_fd, prev_rc)?;
            update_journal_entry(journ_fd, next_rc)?;
        }
        (None, None) => {}
    }
    Ok(())
}

// ────────────────────────────────────────────────────────────────────────────
// read_journal_entry
// ────────────────────────────────────────────────────────────────────────────

fn read_journal_entry(
    journ_fd: &mut File,
    line_rc: &Rc<RefCell<TextLine>>,
) -> io::Result<()> {
    let info_loc = line_rc.borrow().file_info.info_location;
    journ_fd.seek(SeekFrom::Start(info_loc))?;

    let prev_info     = read_u64(journ_fd)?;
    let next_info     = read_u64(journ_fd)?;
    let line_location = read_u64(journ_fd)?;
    let line_length   = read_i32(journ_fd)?;

    {
        let mut line = line_rc.borrow_mut();
        line.file_info.prev_info     = prev_info;
        line.file_info.next_info     = next_info;
        line.file_info.line_location = line_location;
        line.line_length             = line_length;
    }

    journ_fd.seek(SeekFrom::Start(line_location))?;
    let len = (line_length - 1).max(0) as usize;
    let mut buf = vec![0u8; len];
    journ_fd.read_exact(&mut buf)?;
    let text = String::from_utf8_lossy(&buf).to_string();

    let mut line = line_rc.borrow_mut();
    line.line        = text;
    line.max_length  = line_length;
    Ok(())
}

// ────────────────────────────────────────────────────────────────────────────
// open_journal_for_write
// (called as `journal::open_journal_for_write(&mut buff, &jpath, &fname)`)
// ────────────────────────────────────────────────────────────────────────────

/// Create (or truncate) the journal file at `jpath` and write the file-name
/// header, then initialise the info record for the first line.
/// Returns the open `File` handle.
pub fn open_journal_for_write(
    buffer: &mut Buffer,
    jpath: &str,
    fname: &str,
) -> io::Result<File> {
    let mut fd = OpenOptions::new()
        .write(true)
        .create(true)
        .truncate(true)
        .open(jpath)?;

    // Header: full file name + newline.
    if !fname.is_empty() {
        fd.write_all(fname.as_bytes())?;
    }
    fd.write_all(b"\n")?;

    // Initialise journal entry for the first line.
    if let Some(ref first_rc) = buffer.first_line {
        journ_info_init(&mut fd, first_rc)?;
    }

    buffer.journalling  = true;
    buffer.journal_file = Some(jpath.to_string());
    Ok(fd)
}

// ────────────────────────────────────────────────────────────────────────────
// add_to_journal_db
// (called as `journal::add_to_journal_db(Some(&fname), &jpath)`)
// ────────────────────────────────────────────────────────────────────────────

/// Add an entry to ~/.aeeinfo.
pub fn add_to_journal_db(fname: Option<&str>, jpath: &str) -> io::Result<()> {
    let mut list = read_journal_db_inner()?;
    list.push(JournalDbEntry {
        file_name:    fname.map(|s| s.to_string()),
        journal_name: jpath.to_string(),
    });
    write_db_file_inner(&list)
}

// ────────────────────────────────────────────────────────────────────────────
// remove_journal_file
// (called as `journal::remove_journal_file(&jpath, &fname)`)
// ────────────────────────────────────────────────────────────────────────────

/// Delete the journal file from disk and remove its entry from ~/.aeeinfo.
pub fn remove_journal_file(jpath: &str, _fname: &str) -> io::Result<()> {
    let _ = fs::remove_file(jpath);
    if let Ok(mut list) = read_journal_db_inner() {
        list.retain(|e| e.journal_name != jpath);
        let _ = write_db_file_inner(&list);
    }
    Ok(())
}

// ────────────────────────────────────────────────────────────────────────────
// recover_from_journal
// ────────────────────────────────────────────────────────────────────────────

/// Read the edit buffer back from the journal file at `file_name`.
pub fn recover_from_journal(
    buffer: &mut Buffer,
    file_name: &str,
) -> io::Result<()> {
    let mut journ_fd = File::open(file_name)?;

    // Skip header (up to first '\n').
    let mut header = Vec::new();
    let mut byte   = [0u8; 1];
    loop {
        journ_fd.read_exact(&mut byte)?;
        if byte[0] == b'\n' { break; }
        header.push(byte[0]);
    }
    let stored_name = String::from_utf8_lossy(&header).to_string();
    let start = journ_fd.stream_position()?;

    if buffer.first_line.is_none() {
        buffer.first_line = Some(txtalloc());
    }

    let first_rc = buffer.first_line.clone().unwrap();
    first_rc.borrow_mut().file_info.info_location = start;

    let cols = crate::ui::COLS();
    let mut current   = first_rc.clone();
    let mut num_lines = 0i32;
    loop {
        read_journal_entry(&mut journ_fd, &current)?;
        {
            let mut line = current.borrow_mut();
            let ll = line.line_length;
            line.vert_len   = (crate::ui::scanline_raw(&line.line, ll) / cols) + 1;
            line.max_length = ll;
        }
        num_lines += 1;

        let next_info = current.borrow().file_info.next_info;
        if next_info == NO_FURTHER_LINES { break; }

        let next_line = txtalloc();
        next_line.borrow_mut().file_info.info_location = next_info;
        next_line.borrow_mut().prev_line               = Some(current.clone());
        next_line.borrow_mut().line_number             = current.borrow().line_number + 1;
        current.borrow_mut().next_line                 = Some(next_line.clone());
        current = next_line;
    }

    buffer.num_of_lines = num_lines;
    buffer.curr_line    = buffer.first_line.clone();

    if buffer.full_name.is_none() || buffer.full_name.as_deref() == Some("") {
        if !stored_name.is_empty() {
            buffer.full_name = Some(stored_name.clone());
            buffer.file_name = Some(ae_basename(&stored_name));
        }
    }
    Ok(())
}

// ────────────────────────────────────────────────────────────────────────────
// Journal database (~/.aeeinfo)
// ────────────────────────────────────────────────────────────────────────────

fn db_file_path()   -> String { resolve_name("~/.aeeinfo") }
fn lock_file_path() -> String { resolve_name("~/.aeeinfo.L") }

fn lock_journal_fd() -> bool {
    OpenOptions::new()
        .write(true)
        .create_new(true)
        .open(lock_file_path())
        .is_ok()
}

fn unlock_journal_fd() { let _ = fs::remove_file(lock_file_path()); }

#[derive(Debug, Clone)]
struct JournalDbEntry {
    file_name:    Option<String>,
    journal_name: String,
}

fn read_journal_db_inner() -> io::Result<Vec<JournalDbEntry>> {
    // Best-effort lock; proceed even if we can't lock.
    let _locked = lock_journal_fd();
    let content = match fs::read_to_string(db_file_path()) {
        Ok(c)  => c,
        Err(e) if e.kind() == io::ErrorKind::NotFound => {
            if _locked { unlock_journal_fd(); }
            return Ok(Vec::new());
        }
        Err(e) => {
            if _locked { unlock_journal_fd(); }
            return Err(e);
        }
    };

    let mut list = Vec::new();
    for line in content.lines() {
        let mut parts = line.split_whitespace();
        let file_len:  usize = parts.next().and_then(|v| v.parse().ok()).unwrap_or(0);
        let _journ_len: usize = parts.next().and_then(|v| v.parse().ok()).unwrap_or(0);
        let file_name  = if file_len > 0 { parts.next().map(|s| s.to_string()) } else { None };
        let journ_name = parts.next().unwrap_or("").to_string();
        if !journ_name.is_empty() {
            list.push(JournalDbEntry { file_name, journal_name: journ_name });
        }
    }
    if _locked { unlock_journal_fd(); }
    Ok(list)
}

fn write_db_file_inner(list: &[JournalDbEntry]) -> io::Result<()> {
    let _locked = lock_journal_fd();
    let mut out = String::new();
    for entry in list {
        let flen = entry.file_name.as_deref().map(|s| s.len()).unwrap_or(0);
        if let Some(ref f) = entry.file_name {
            out.push_str(&format!(
                "{} {} {} {}\n",
                flen, entry.journal_name.len(), f, entry.journal_name
            ));
        } else {
            out.push_str(&format!("0 {} {}\n", entry.journal_name.len(), entry.journal_name));
        }
    }
    fs::write(db_file_path(), out)?;
    if _locked { unlock_journal_fd(); }
    Ok(())
}
