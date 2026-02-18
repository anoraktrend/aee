#![allow(dead_code)]

/// Journal / crash-recovery – ported from src/journal.c
///
/// A journal file records every changed line when the cursor leaves it.
/// On an unclean exit the editor can recover from the journal file.
///
/// The on-disk layout per line is:
///
///   [prev_info : u64] [next_info : u64] [line_location : u64] [line_length : i32]
///
/// followed by the raw line bytes at `line_location`.
///
/// A master database at `~/.aeeinfo` tracks which files are currently
/// being journalled.

use std::fs::{self, File};
use std::io::{self, Read, Seek, SeekFrom, Write};
use std::path::{Path, PathBuf};
use std::time::{SystemTime, UNIX_EPOCH};
use std::rc::Rc;
use std::cell::RefCell;

use crate::editor_state::{Buffer, TextLine};
use crate::text::txtalloc;

// ──────────────────────────────────────────────────────────────────────────────
// Constants
// ──────────────────────────────────────────────────────────────────────────────

/// Sentinel value for "no further lines" – mirrors C `NO_FURTHER_LINES`.
pub const NO_FURTHER_LINES: u64 = u64::MAX;

// ──────────────────────────────────────────────────────────────────────────────
// Low-level binary I/O helpers
// ──────────────────────────────────────────────────────────────────────────────

fn write_u64(f: &mut File, v: u64) -> io::Result<()> {
    f.write_all(&v.to_ne_bytes())
}

fn write_i32(f: &mut File, v: i32) -> io::Result<()> {
    f.write_all(&v.to_ne_bytes())
}

fn read_u64(f: &mut File) -> io::Result<u64> {
    let mut buf = [0u8; 8];
    f.read_exact(&mut buf)?;
    Ok(u64::from_ne_bytes(buf))
}

fn read_i32(f: &mut File) -> io::Result<i32> {
    let mut buf = [0u8; 4];
    f.read_exact(&mut buf)?;
    Ok(i32::from_ne_bytes(buf))
}

// ──────────────────────────────────────────────────────────────────────────────
// write_journal – write one line to the journal file
// ──────────────────────────────────────────────────────────────────────────────

/// Append the line text to the journal file and update the line's location
/// metadata. Then call `update_journal_entry`.
pub fn write_journal(
    journ_file: &mut File,
    line_rc: &Rc<RefCell<TextLine>>,
) -> io::Result<()> {
    let end = journ_file.seek(SeekFrom::End(0))?;
    line_rc.borrow_mut().file_info.line_location = end;

    let bytes = {
        let l = line_rc.borrow();
        l.line.as_bytes().to_vec()
    };
    journ_file.write_all(&bytes)?;
    update_journal_entry(journ_file, line_rc)?;
    line_rc.borrow_mut().changed = false;
    Ok(())
}

// ──────────────────────────────────────────────────────────────────────────────
// update_journal_entry – rewrite the info block for a line
// ──────────────────────────────────────────────────────────────────────────────

/// Seek to `info_location` and write the four metadata words for `line`.
/// If `info_location == NO_FURTHER_LINES` (uninitialised), calls
/// `journ_info_init` instead.
pub fn update_journal_entry(
    journ_file: &mut File,
    line_rc: &Rc<RefCell<TextLine>>,
) -> io::Result<()> {
    let info_loc = line_rc.borrow().file_info.info_location;
    if info_loc == NO_FURTHER_LINES {
        return journ_info_init(journ_file, line_rc);
    }
    journ_file.seek(SeekFrom::Start(info_loc))?;
    let (prev, next, line_loc, line_len) = {
        let l = line_rc.borrow();
        (l.file_info.prev_info,
         l.file_info.next_info,
         l.file_info.line_location,
         l.line_length)
    };
    write_u64(journ_file, prev)?;
    write_u64(journ_file, next)?;
    write_u64(journ_file, line_loc)?;
    write_i32(journ_file, line_len)?;
    Ok(())
}

// ──────────────────────────────────────────────────────────────────────────────
// journ_info_init – first-time write of info block
// ──────────────────────────────────────────────────────────────────────────────

fn journ_info_init(
    journ_file: &mut File,
    line_rc: &Rc<RefCell<TextLine>>,
) -> io::Result<()> {
    let end = journ_file.seek(SeekFrom::End(0))?;
    line_rc.borrow_mut().file_info.info_location = end;

    // Wire up prev/next pointers
    let (prev_info, next_info) = {
        let l = line_rc.borrow();
        let prev = l.prev_line.as_ref()
            .map(|p| p.borrow().file_info.info_location)
            .unwrap_or(NO_FURTHER_LINES);
        let next = l.next_line.as_ref()
            .map(|n| n.borrow().file_info.info_location)
            .unwrap_or(NO_FURTHER_LINES);
        (prev, next)
    };
    {
        let mut l = line_rc.borrow_mut();
        l.file_info.prev_info = prev_info;
        l.file_info.next_info = next_info;
    }
    // Update neighbouring lines so they point at us
    if let Some(prev_rc) = line_rc.borrow().prev_line.clone() {
        prev_rc.borrow_mut().file_info.next_info = end;
        update_journal_entry(journ_file, &prev_rc)?;
    }
    if let Some(next_rc) = line_rc.borrow().next_line.clone() {
        next_rc.borrow_mut().file_info.prev_info = end;
        update_journal_entry(journ_file, &next_rc)?;
    }
    update_journal_entry(journ_file, line_rc)
}

// ──────────────────────────────────────────────────────────────────────────────
// read_journal_entry – read one info block + text
// ──────────────────────────────────────────────────────────────────────────────

fn read_journal_entry(
    journ_file: &mut File,
    line_rc: &Rc<RefCell<TextLine>>,
) -> io::Result<()> {
    let info_loc = line_rc.borrow().file_info.info_location;
    journ_file.seek(SeekFrom::Start(info_loc))?;
    let prev  = read_u64(journ_file)?;
    let next  = read_u64(journ_file)?;
    let tloc  = read_u64(journ_file)?;
    let tlen  = read_i32(journ_file)?;
    {
        let mut l = line_rc.borrow_mut();
        l.file_info.prev_info     = prev;
        l.file_info.next_info     = next;
        l.file_info.line_location = tloc;
        l.line_length             = tlen;
    }
    // Read the text
    journ_file.seek(SeekFrom::Start(tloc))?;
    let len = tlen.max(0) as usize;
    let mut bytes = vec![0u8; len];
    journ_file.read_exact(&mut bytes)?;
    line_rc.borrow_mut().line = String::from_utf8_lossy(&bytes).into_owned();
    Ok(())
}

// ──────────────────────────────────────────────────────────────────────────────
// recover_from_journal – rebuild the buffer from a journal file
// ──────────────────────────────────────────────────────────────────────────────

/// Read a journal file and reconstruct the buffer it describes.
/// Returns `Ok(original_file_name)` or an `Err`.
pub fn recover_from_journal(
    buff: &mut Buffer,
    journal_path: &Path,
) -> io::Result<String> {
    let mut jf = File::open(journal_path)?;

    // The first record in the file is the original file name followed by '\n'.
    let mut orig_name = String::new();
    let mut byte = [0u8; 1];
    loop {
        let n = jf.read(&mut byte)?;
        if n == 0 { return Err(io::Error::new(io::ErrorKind::UnexpectedEof, "empty journal")); }
        if byte[0] == b'\n' { break; }
        orig_name.push(byte[0] as char);
    }

    // Byte offset just after the '\n' is where the first info block lives.
    let first_info_loc = jf.stream_position()?;

    // Reconstruct linked list
    let first_line_rc = buff.first_line.as_ref()
        .ok_or_else(|| io::Error::new(io::ErrorKind::Other, "no first line"))?
        .clone();

    first_line_rc.borrow_mut().file_info.info_location = first_info_loc;

    let mut line_rc = first_line_rc;
    loop {
        read_journal_entry(&mut jf, &line_rc)?;
        let next_info = line_rc.borrow().file_info.next_info;
        if next_info == NO_FURTHER_LINES { break; }
        let new_line = txtalloc();
        {
            let mut nl = new_line.borrow_mut();
            nl.file_info.info_location = next_info;
            nl.file_info.next_info = 0;
            nl.prev_line = Some(line_rc.clone());
        }
        line_rc.borrow_mut().next_line = Some(new_line.clone());
        buff.num_of_lines += 1;
        line_rc = new_line;
    }

    buff.changed = true;
    Ok(orig_name)
}

// ──────────────────────────────────────────────────────────────────────────────
// open_journal_for_write – create the journal file and write initial entry
// ──────────────────────────────────────────────────────────────────────────────

/// Open the journal file for writing, record `full_name` as the first line,
/// and initialise the first text-line entry.
pub fn open_journal_for_write(
    buff: &mut Buffer,
    journal_path: &Path,
    full_name: &str,
) -> io::Result<File> {
    let mut jf = File::create(journal_path)?;
    if !full_name.is_empty() {
        jf.write_all(full_name.as_bytes())?;
        jf.write_all(b"\n")?;
    }
    jf.write_all(b"\n")?;

    if let Some(ref first_rc) = buff.first_line.clone() {
        journ_info_init(&mut jf, first_rc)?;
    }
    Ok(jf)
}

// ──────────────────────────────────────────────────────────────────────────────
// remove_journal_file – delete the journal and its db entry
// ──────────────────────────────────────────────────────────────────────────────

/// Close and delete the journal file, and remove its entry from `~/.aeeinfo`.
pub fn remove_journal_file(journal_path: &Path, full_name: &str) -> io::Result<()> {
    fs::remove_file(journal_path)?;
    rm_journal_db_entry(journal_path, full_name)
}

// ──────────────────────────────────────────────────────────────────────────────
// Journal name generation
// ──────────────────────────────────────────────────────────────────────────────

/// Generate a journal file path for `file_name`.
/// If `journal_dir` is Some, place it there; otherwise put it next to the file
/// with a ".rv" extension.
pub fn journal_name(file_name: &str, journal_dir: Option<&str>) -> PathBuf {
    let base_name = Path::new(file_name)
        .file_name()
        .and_then(|n| n.to_str())
        .unwrap_or("unnamed");

    let stem = if base_name.is_empty() {
        // Generate a timestamp-based name
        let secs = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .map(|d| d.as_secs())
            .unwrap_or(0);
        format!("{:012}", secs)
    } else {
        base_name.to_string()
    };

    let name_with_ext = format!("{}.rv", stem);

    match journal_dir {
        Some(dir) => {
            let mut p = PathBuf::from(dir);
            p.push(&name_with_ext);
            p
        }
        None => {
            let base = Path::new(file_name).parent().unwrap_or(Path::new("."));
            let mut p = PathBuf::from(base);
            p.push(&name_with_ext);
            p
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// Journal database (~/.aeeinfo) – track active journal sessions
// ──────────────────────────────────────────────────────────────────────────────

/// One entry in the journal database.
#[derive(Debug, Clone)]
pub struct JournalDbEntry {
    pub file_name: Option<String>,
    pub journal_name: String,
}

fn db_path() -> PathBuf {
    let home = std::env::var("HOME").unwrap_or_else(|_| ".".to_string());
    PathBuf::from(home).join(".aeeinfo")
}

fn lock_path() -> PathBuf {
    let home = std::env::var("HOME").unwrap_or_else(|_| ".".to_string());
    PathBuf::from(home).join(".aeeinfo.L")
}

/// Acquire an exclusive lock file.  Returns `Ok(())` or `Err`.
fn lock_journal_db() -> io::Result<()> {
    use std::fs::OpenOptions;
    let lp = lock_path();
    for _ in 0..3 {
        match OpenOptions::new().write(true).create_new(true).open(&lp) {
            Ok(_) => return Ok(()),
            Err(_) => std::thread::sleep(std::time::Duration::from_secs(1)),
        }
    }
    Err(io::Error::new(io::ErrorKind::WouldBlock, "could not acquire journal lock"))
}

fn unlock_journal_db() {
    let _ = fs::remove_file(lock_path());
}

/// Read all entries from `~/.aeeinfo`.
pub fn read_journal_db() -> io::Result<Vec<JournalDbEntry>> {
    let path = db_path();
    if !path.exists() { return Ok(vec![]); }
    let content = fs::read_to_string(&path)?;
    let mut entries = Vec::new();
    for line in content.lines() {
        let parts: Vec<&str> = line.splitn(4, ' ').collect();
        if parts.len() < 3 { continue; }
        let file_name_len: usize = parts[0].parse().unwrap_or(0);
        let journ_name_len: usize = parts[1].parse().unwrap_or(0);
        let rest = if parts.len() >= 4 {
            format!("{} {}", parts[2], parts[3])
        } else {
            parts[2].to_string()
        };
        let (file_part, journ_part) = if file_name_len > 0 {
            let fp = rest[..file_name_len.min(rest.len())].to_string();
            let jp_start = (file_name_len + 1).min(rest.len());
            let jp = rest[jp_start..(jp_start + journ_name_len).min(rest.len())].to_string();
            (Some(fp), jp)
        } else {
            let jp = rest[..journ_name_len.min(rest.len())].to_string();
            (None, jp)
        };
        entries.push(JournalDbEntry { file_name: file_part, journal_name: journ_part });
    }
    Ok(entries)
}

/// Write all entries back to `~/.aeeinfo`.
fn write_journal_db(entries: &[JournalDbEntry]) -> io::Result<()> {
    let path = db_path();
    let mut f = File::create(&path)?;
    for e in entries {
        match &e.file_name {
            Some(fn_) => writeln!(f, "{} {} {} {}", fn_.len(), e.journal_name.len(), fn_, e.journal_name)?,
            None       => writeln!(f, "0 {} {}", e.journal_name.len(), e.journal_name)?,
        }
    }
    Ok(())
}

/// Add a new entry to `~/.aeeinfo`.
pub fn add_to_journal_db(file_name: Option<&str>, journal_path: &Path) -> io::Result<()> {
    lock_journal_db()?;
    let mut entries = read_journal_db().unwrap_or_default();
    entries.push(JournalDbEntry {
        file_name: file_name.map(|s| s.to_string()),
        journal_name: journal_path.to_string_lossy().into_owned(),
    });
    let result = write_journal_db(&entries);
    unlock_journal_db();
    result
}

/// Remove the entry for `journal_path` from `~/.aeeinfo`.
pub fn rm_journal_db_entry(journal_path: &Path, _full_name: &str) -> io::Result<()> {
    lock_journal_db()?;
    let entries = read_journal_db().unwrap_or_default();
    let jname = journal_path.to_string_lossy();
    let filtered: Vec<JournalDbEntry> = entries
        .into_iter()
        .filter(|e| e.journal_name != jname.as_ref())
        .collect();
    let result = if filtered.is_empty() {
        let _ = fs::remove_file(db_path());
        Ok(())
    } else {
        write_journal_db(&filtered)
    };
    unlock_journal_db();
    result
}
