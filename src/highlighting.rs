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
}

/// Determine whether `word` is a keyword for the given language.
pub fn is_keyword(word: &str, lang: &str) -> bool {
    match lang {
        "c" | "cpp" | "c++" => C_KEYWORDS.contains(word),
        "rust" => RUST_KEYWORDS.contains(word),
        _ => C_KEYWORDS.contains(word) || RUST_KEYWORDS.contains(word),
    }
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

// ── LSP semantic-token helpers ───────────────────────────────────────────────

/// Map an LSP token-type name (from the server's legend) to a `TokenKind`.
pub fn lsp_token_type_to_kind(type_name: &str) -> TokenKind {
    match type_name {
        "keyword" | "modifier"                          => TokenKind::Keyword,
        "comment"                                       => TokenKind::Comment,
        "string"                                        => TokenKind::StringLiteral,
        "number"                                        => TokenKind::Number,
        "operator"                                      => TokenKind::Operator,
        "namespace" | "type" | "class" | "enum"
        | "interface" | "struct" | "typeParameter"
        | "enumMember" | "event" | "function"
        | "method" | "macro"                            => TokenKind::Identifier,
        _                                               => TokenKind::Identifier,
    }
}

/// Build a complete span list for `line` using LSP semantic tokens.
///
/// `tokens` must already be filtered to only those on this line number and
/// must be sorted by `start_char` ascending.  `legend` is the server's
/// `tokenTypes` array (maps `token_type` index → type name).
///
/// The function returns owned `String` spans so that the caller is not
/// constrained to the lifetime of `line`.  Gaps between tokens are emitted
/// as `TokenKind::Identifier` (plain text).
pub fn highlight_line_lsp(
    line: &str,
    tokens: &[crate::lsp::SemanticToken],
    legend: &[String],
) -> Vec<(String, TokenKind)> {
    let mut spans: Vec<(String, TokenKind)> = Vec::new();
    let chars: Vec<char> = line.chars().collect();
    let char_len = chars.len();
    let mut cursor = 0usize; // current char index

    for tok in tokens {
        let start = tok.start_char as usize;
        let end   = (start + tok.length as usize).min(char_len);

        // Gap before this token
        if start > cursor {
            let gap: String = chars[cursor..start].iter().collect();
            spans.push((gap, TokenKind::Identifier));
        }

        if end > start {
            let text: String = chars[start..end].iter().collect();
            let type_name = legend
                .get(tok.token_type as usize)
                .map(|s| s.as_str())
                .unwrap_or("");
            spans.push((text, lsp_token_type_to_kind(type_name)));
        }

        cursor = end;
    }

    // Trailing text after the last token
    if cursor < char_len {
        let tail: String = chars[cursor..].iter().collect();
        spans.push((tail, TokenKind::Identifier));
    }

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
