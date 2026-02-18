// Syntax highlighting module

use lazy_static::lazy_static;
use std::collections::HashSet;

lazy_static! {
    static ref C_KEYWORDS: HashSet<String> = {
        let mut set = HashSet::new();
        let keywords = [
            "auto", "break", "case", "char", "const", "continue", "default", "do",
            "double", "else", "enum", "extern", "float", "for", "goto", "if",
            "int", "long", "register", "return", "short", "signed", "sizeof", "static",
            "struct", "switch", "typedef", "union", "unsigned", "void", "volatile", "while",
        ];
        for kw in &keywords {
            set.insert(kw.to_string());
        }
        set
    };
}

fn is_keyword(_word: &str) -> bool {
    // Stub
    false
}

pub fn highlight_syntax(_win: i32, _line: &str) {
    // Stub, since using cursive
}
