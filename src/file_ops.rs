// File operations module

use std::fs::File;
use std::io::{self, Read};
use std::path::Path;

// Read file into buffer
pub fn get_file(file_name: &str) -> io::Result<String> {
    let mut file = File::open(file_name)?;
    let mut contents = String::new();
    file.read_to_string(&mut contents)?;
    Ok(contents)
}

// Get full path
pub fn get_full_path(path: &str, _orig_path: &str) -> String {
    if path.starts_with('~') {
        if let Ok(home) = std::env::var("HOME") {
            home + &path[1..]
        } else {
            path.to_string()
        }
    } else {
        path.to_string()
    }
}

// Get basename
pub fn ae_basename(path: &str) -> String {
    Path::new(path).file_name()
        .unwrap_or_default()
        .to_string_lossy()
        .to_string()
}
