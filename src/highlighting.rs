// Syntax highlighting module

use lazy_static::lazy_static;
use std::collections::HashSet;

lazy_static! {
    static ref C_KEYWORDS: HashSet<&'static str> = {
        let mut set = HashSet::new();
        let keywords = [
            "auto", "break", "case", "char", "const", "continue", "default", "do",
            "double", "else", "enum", "extern", "float", "for", "goto", "if",
            "int", "long", "register", "return", "short", "signed", "sizeof", "static",
            "struct", "switch", "typedef", "union", "unsigned", "void", "volatile", "while",
        ];
        for kw in &keywords {
            set.insert(*kw);
        }
        set
    };

    static ref RUST_KEYWORDS: HashSet<&'static str> = {
        let mut set = HashSet::new();
        let keywords = [
            "as", "async", "await", "break", "const", "continue", "crate", "dyn",
            "else", "enum", "extern", "false", "fn", "for", "if", "impl", "in",
            "let", "loop", "match", "mod", "move", "mut", "pub", "ref", "return",
            "self", "Self", "static", "struct", "super", "trait", "true", "type",
            "union", "unsafe", "use", "where", "while",
        ];
        for kw in &keywords {
            set.insert(*kw);
        }
        set
    };
}

/// Token types produced by the local syntax highlighter
#[derive(Debug, Clone, PartialEq)]
pub enum TokenKind {
    Keyword,
    Identifier,
    Number,
    StringLiteral,
    Comment,
    Operator,
    Whitespace,
    Other,
}

/// A highlighted span within a line
#[derive(Debug, Clone)]
pub struct Token {
    pub start: usize,
    pub end: usize,
    pub kind: TokenKind,
}

/// Determine whether `word` is a keyword for the given language.
pub fn is_keyword(word: &str, lang: &str) -> bool {
    match lang {
        "c" | "cpp" | "c++" => C_KEYWORDS.contains(word),
        "rust" => RUST_KEYWORDS.contains(word),
        _ => C_KEYWORDS.contains(word) || RUST_KEYWORDS.contains(word),
    }
}

/// Tokenise a single source line into highlighted spans.
pub fn tokenize_line(line: &str, lang: &str) -> Vec<Token> {
    let mut tokens = Vec::new();
    let chars: Vec<char> = line.chars().collect();
    let len = chars.len();
    let mut i = 0;

    while i < len {
        // Line comments: // ...
        if i + 1 < len && chars[i] == '/' && chars[i + 1] == '/' {
            tokens.push(Token { start: i, end: len, kind: TokenKind::Comment });
            break;
        }
        // Line comments: # ... (Python/shell)
        if chars[i] == '#' {
            tokens.push(Token { start: i, end: len, kind: TokenKind::Comment });
            break;
        }

        // String literals (double-quote)
        if chars[i] == '"' {
            let start = i;
            i += 1;
            while i < len {
                if chars[i] == '\\' {
                    i += 2; // skip escape
                } else if chars[i] == '"' {
                    i += 1;
                    break;
                } else {
                    i += 1;
                }
            }
            tokens.push(Token { start, end: i, kind: TokenKind::StringLiteral });
            continue;
        }

        // String literals (single-quote char)
        if chars[i] == '\'' {
            let start = i;
            i += 1;
            while i < len {
                if chars[i] == '\\' {
                    i += 2;
                } else if chars[i] == '\'' {
                    i += 1;
                    break;
                } else {
                    i += 1;
                }
            }
            tokens.push(Token { start, end: i, kind: TokenKind::StringLiteral });
            continue;
        }

        // Numbers
        if chars[i].is_ascii_digit() || (chars[i] == '-' && i + 1 < len && chars[i + 1].is_ascii_digit()) {
            let start = i;
            if chars[i] == '-' { i += 1; }
            while i < len && (chars[i].is_ascii_alphanumeric() || chars[i] == '.' || chars[i] == '_') {
                i += 1;
            }
            tokens.push(Token { start, end: i, kind: TokenKind::Number });
            continue;
        }

        // Identifiers / keywords
        if chars[i].is_alphabetic() || chars[i] == '_' {
            let start = i;
            while i < len && (chars[i].is_alphanumeric() || chars[i] == '_') {
                i += 1;
            }
            let word: String = chars[start..i].iter().collect();
            let kind = if is_keyword(&word, lang) {
                TokenKind::Keyword
            } else {
                TokenKind::Identifier
            };
            tokens.push(Token { start, end: i, kind });
            continue;
        }

        // Whitespace
        if chars[i].is_whitespace() {
            let start = i;
            while i < len && chars[i].is_whitespace() {
                i += 1;
            }
            tokens.push(Token { start, end: i, kind: TokenKind::Whitespace });
            continue;
        }

        // Everything else (operators, punctuation)
        tokens.push(Token { start: i, end: i + 1, kind: TokenKind::Operator });
        i += 1;
    }

    tokens
}

/// Guess the language from a file extension.
pub fn lang_from_extension(path: &str) -> &'static str {
    let ext = std::path::Path::new(path)
        .extension()
        .and_then(|e| e.to_str())
        .unwrap_or("");
    match ext {
        "c" | "h" => "c",
        "cpp" | "cc" | "cxx" | "hpp" => "cpp",
        "rs" => "rust",
        "py" => "python",
        "sh" | "bash" => "shell",
        _ => "text",
    }
}

/// Apply local syntax highlighting to a line string, returning a sequence of
/// (text_slice, token_kind) tuples that the UI layer can colour.
/// Ignores multi-line state; use `highlight_line_with_state` when you need
/// to carry block-comment context across lines.
pub fn highlight_line<'a>(line: &'a str, lang: &str) -> Vec<(&'a str, TokenKind)> {
    let (spans, _) = highlight_line_with_state(line, lang, false);
    spans
}

/// Stateful highlighter that ports the C `highlight_syntax` state machine.
///
/// `in_block_comment` – whether the previous line ended inside a `/* … */` comment.
/// Returns `(spans, still_in_block_comment)`.
pub fn highlight_line_with_state<'a>(
    line: &'a str,
    lang: &str,
    mut in_block_comment: bool,
) -> (Vec<(&'a str, TokenKind)>, bool) {
    let mut spans: Vec<(&'a str, TokenKind)> = Vec::new();
    let bytes = line.as_bytes();
    let len = bytes.len();
    let mut i = 0;

    macro_rules! push_span {
        ($start:expr, $end:expr, $kind:expr) => {
            if $start < $end {
                spans.push((&line[$start..$end], $kind));
            }
        };
    }

    while i < len {
        // ── Inside a block comment ────────────────────────────────────────────
        if in_block_comment {
            let start = i;
            while i < len {
                if i + 1 < len && bytes[i] == b'*' && bytes[i + 1] == b'/' {
                    i += 2;
                    in_block_comment = false;
                    break;
                }
                i += 1;
            }
            push_span!(start, i, TokenKind::Comment);
            continue;
        }

        // ── Start of block comment /*  ────────────────────────────────────────
        if i + 1 < len && bytes[i] == b'/' && bytes[i + 1] == b'*' {
            let start = i;
            i += 2;
            in_block_comment = true;
            while i < len {
                if i + 1 < len && bytes[i] == b'*' && bytes[i + 1] == b'/' {
                    i += 2;
                    in_block_comment = false;
                    break;
                }
                i += 1;
            }
            push_span!(start, i, TokenKind::Comment);
            continue;
        }

        // ── Line comment //  ──────────────────────────────────────────────────
        if i + 1 < len && bytes[i] == b'/' && bytes[i + 1] == b'/' {
            push_span!(i, len, TokenKind::Comment);
            i = len;
            continue;
        }
        // ── Shell / Python line comment #  ────────────────────────────────────
        if matches!(lang, "python" | "shell") && bytes[i] == b'#' {
            push_span!(i, len, TokenKind::Comment);
            i = len;
            continue;
        }

        // ── String literal "…"  ───────────────────────────────────────────────
        if bytes[i] == b'"' {
            let start = i;
            i += 1;
            while i < len {
                if bytes[i] == b'\\' { i += 2; }
                else if bytes[i] == b'"' { i += 1; break; }
                else { i += 1; }
            }
            push_span!(start, i, TokenKind::StringLiteral);
            continue;
        }

        // ── Char literal '…'  ────────────────────────────────────────────────
        // (skip in Rust where ' is also used for lifetimes)
        if bytes[i] == b'\'' && lang != "rust" {
            let start = i;
            i += 1;
            while i < len {
                if bytes[i] == b'\\' { i += 2; }
                else if bytes[i] == b'\'' { i += 1; break; }
                else { i += 1; }
            }
            push_span!(start, i, TokenKind::StringLiteral);
            continue;
        }

        // ── Number  ───────────────────────────────────────────────────────────
        if bytes[i].is_ascii_digit() {
            let start = i;
            while i < len && (bytes[i].is_ascii_alphanumeric() || bytes[i] == b'.' || bytes[i] == b'_') {
                i += 1;
            }
            push_span!(start, i, TokenKind::Number);
            continue;
        }

        // ── Identifier / keyword  ─────────────────────────────────────────────
        if bytes[i].is_ascii_alphabetic() || bytes[i] == b'_' {
            let start = i;
            while i < len && (bytes[i].is_ascii_alphanumeric() || bytes[i] == b'_') {
                i += 1;
            }
            let word = &line[start..i];
            let kind = if is_keyword(word, lang) { TokenKind::Keyword } else { TokenKind::Identifier };
            push_span!(start, i, kind);
            continue;
        }

        // ── Whitespace  ───────────────────────────────────────────────────────
        if bytes[i].is_ascii_whitespace() {
            let start = i;
            while i < len && bytes[i].is_ascii_whitespace() { i += 1; }
            push_span!(start, i, TokenKind::Whitespace);
            continue;
        }

        // ── Everything else (operators, punctuation)  ─────────────────────────
        push_span!(i, i + 1, TokenKind::Operator);
        i += 1;
    }

    (spans, in_block_comment)
}
