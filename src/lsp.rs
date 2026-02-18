// LSP module — Language Server Protocol client
//
// Implements the JSON-RPC framing used by LSP, a background reader task, and
// the most important LSP requests needed for editor integration:
//   • initialize / initialized handshake
//   • textDocument/didOpen, didChange
//   • textDocument/completion
//   • textDocument/semanticTokens/full
//   • textDocument/publishDiagnostics (notification, server-initiated)

use std::collections::HashMap;
use std::process::Stdio;

use serde_json::{json, Value};
use tokio::io::{AsyncBufReadExt, AsyncReadExt, AsyncWriteExt, BufReader};
use tokio::process::{Child, Command};
use tokio::sync::mpsc;

// ── Semantic token standard types (LSP spec §3.16) ──────────────────────────

pub const TOKEN_TYPES: &[&str] = &[
    "namespace", "type", "class", "enum", "interface", "struct", "typeParameter",
    "parameter", "variable", "property", "enumMember", "event", "function",
    "method", "macro", "keyword", "modifier", "comment", "string", "number",
    "regexp", "operator",
];

// ── Decoded semantic token ───────────────────────────────────────────────────

#[derive(Debug, Clone)]
pub struct SemanticToken {
    pub line: u32,
    pub start_char: u32,
    pub length: u32,
    pub token_type: u32,
    pub token_modifiers: u32,
}

/// Decode the flat LSP semantic-token integer array into structured tokens.
/// The encoding is relative (deltaLine, deltaStartChar, length, type, mods).
pub fn decode_semantic_tokens(data: &[u64]) -> Vec<SemanticToken> {
    let mut tokens = Vec::new();
    let mut line = 0u32;
    let mut start_char = 0u32;

    for chunk in data.chunks_exact(5) {
        let delta_line = chunk[0] as u32;
        let delta_start = chunk[1] as u32;
        let length = chunk[2] as u32;
        let token_type = chunk[3] as u32;
        let token_modifiers = chunk[4] as u32;

        if delta_line > 0 {
            line += delta_line;
            start_char = delta_start;
        } else {
            start_char += delta_start;
        }

        tokens.push(SemanticToken { line, start_char, length, token_type, token_modifiers });
    }
    tokens
}

// ── Incoming message ─────────────────────────────────────────────────────────

#[derive(Debug)]
pub enum LspMessage {
    /// A response to one of our requests (has "id").
    Response { id: i64, result: Value },
    /// A response that carries an error.
    Error { id: i64, error: Value },
    /// A server-initiated notification (no "id").
    Notification { method: String, params: Value },
}

// ── Client ───────────────────────────────────────────────────────────────────

pub struct LspClient {
    _process: Child,
    stdin: tokio::process::ChildStdin,
    request_id: i64,
    /// Receives decoded messages from the background reader task.
    rx: mpsc::UnboundedReceiver<LspMessage>,
    /// Responses that arrived but haven't been consumed by wait_for_response yet.
    pending_responses: HashMap<i64, Value>,
    /// Latest diagnostics keyed by URI.
    pub diagnostics: HashMap<String, Value>,
    /// Latest semantic tokens keyed by URI.
    pub semantic_tokens: HashMap<String, Vec<SemanticToken>>,
    /// Token types advertised during initialize (mirrors TOKEN_TYPES by default).
    pub token_type_legend: Vec<String>,
}

impl LspClient {
    /// Start a language server and perform the initialize/initialized handshake.
    pub async fn new(server_command: &str) -> Result<Self, Box<dyn std::error::Error>> {
        let mut child = Command::new("sh")
            .arg("-c")
            .arg(server_command)
            .stdin(Stdio::piped())
            .stdout(Stdio::piped())
            .stderr(Stdio::null())
            .spawn()?;

        let stdin = child.stdin.take().ok_or("no stdin")?;
        let stdout = child.stdout.take().ok_or("no stdout")?;

        let (tx, rx) = mpsc::unbounded_channel::<LspMessage>();

        // ── Background reader task ───────────────────────────────────────────
        tokio::spawn(async move {
            let mut reader = BufReader::new(stdout);
            loop {
                // Read headers until blank line
                let mut content_length: Option<usize> = None;
                loop {
                    let mut header_line = String::new();
                    match reader.read_line(&mut header_line).await {
                        Ok(0) => return, // EOF
                        Err(_) => return,
                        Ok(_) => {}
                    }
                    let trimmed = header_line.trim();
                    if trimmed.is_empty() {
                        break;
                    }
                    if let Some(rest) = trimmed.strip_prefix("Content-Length:") {
                        content_length = rest.trim().parse().ok();
                    }
                }

                let len = match content_length {
                    Some(l) => l,
                    None => continue,
                };

                let mut body = vec![0u8; len];
                if reader.read_exact(&mut body).await.is_err() {
                    return;
                }

                let json: Value = match serde_json::from_slice(&body) {
                    Ok(v) => v,
                    Err(_) => continue,
                };

                let msg = if json.get("id").is_some() {
                    let id = json["id"].as_i64().unwrap_or(-1);
                    if json.get("error").is_some() {
                        LspMessage::Error { id, error: json["error"].clone() }
                    } else {
                        LspMessage::Response { id, result: json["result"].clone() }
                    }
                } else if let Some(method) = json["method"].as_str() {
                    LspMessage::Notification {
                        method: method.to_string(),
                        params: json["params"].clone(),
                    }
                } else {
                    continue;
                };

                if tx.send(msg).is_err() {
                    return;
                }
            }
        });

        let mut client = LspClient {
            _process: child,
            stdin,
            request_id: 0,
            rx,
            pending_responses: HashMap::new(),
            diagnostics: HashMap::new(),
            semantic_tokens: HashMap::new(),
            token_type_legend: TOKEN_TYPES.iter().map(|s| s.to_string()).collect(),
        };

        // Perform the mandatory initialize handshake
        client.do_initialize().await?;

        Ok(client)
    }

    // ── Helpers ──────────────────────────────────────────────────────────────

    fn next_id(&mut self) -> i64 {
        self.request_id += 1;
        self.request_id
    }

    async fn send_raw(&mut self, value: &Value) -> Result<(), Box<dyn std::error::Error>> {
        let json_str = value.to_string();
        let message = format!("Content-Length: {}\r\n\r\n{}", json_str.len(), json_str);
        self.stdin.write_all(message.as_bytes()).await?;
        self.stdin.flush().await?;
        Ok(())
    }

    /// Send a request and return its id.
    async fn send_request(&mut self, method: &str, params: Value) -> Result<i64, Box<dyn std::error::Error>> {
        let id = self.next_id();
        let request = json!({
            "jsonrpc": "2.0",
            "id": id,
            "method": method,
            "params": params
        });
        self.send_raw(&request).await?;
        Ok(id)
    }

    /// Send a notification (no id, no response expected).
    async fn send_notification(&mut self, method: &str, params: Value) -> Result<(), Box<dyn std::error::Error>> {
        let notification = json!({
            "jsonrpc": "2.0",
            "method": method,
            "params": params
        });
        self.send_raw(&notification).await?;
        Ok(())
    }

    /// Drain all pending messages from the reader task, storing them internally.
    pub fn poll_messages(&mut self) {
        loop {
            match self.rx.try_recv() {
                Ok(LspMessage::Response { id, result }) => {
                    self.pending_responses.insert(id, result);
                }
                Ok(LspMessage::Error { id, error }) => {
                    eprintln!("LSP error for request {}: {}", id, error);
                    self.pending_responses.insert(id, Value::Null);
                }
                Ok(LspMessage::Notification { method, params }) => {
                    self.handle_notification(&method, params);
                }
                Err(_) => break,
            }
        }
    }

    fn handle_notification(&mut self, method: &str, params: Value) {
        match method {
            "textDocument/publishDiagnostics" => {
                if let Some(uri) = params["uri"].as_str() {
                    self.diagnostics.insert(uri.to_string(), params["diagnostics"].clone());
                }
            }
            _ => {} // ignore other notifications
        }
    }

    /// Wait synchronously (via tokio blocking receive) for a specific request id.
    /// Times out after 2 seconds.  Returns None on timeout.
    pub async fn wait_for_response(&mut self, id: i64) -> Option<Value> {
        // Already buffered?
        if let Some(v) = self.pending_responses.remove(&id) {
            return Some(v);
        }

        let deadline = tokio::time::Instant::now() + tokio::time::Duration::from_secs(2);
        loop {
            if tokio::time::Instant::now() >= deadline {
                return None;
            }
            match tokio::time::timeout(
                tokio::time::Duration::from_millis(100),
                self.rx.recv(),
            )
            .await
            {
                Ok(Some(LspMessage::Response { id: rid, result })) if rid == id => {
                    return Some(result);
                }
                Ok(Some(LspMessage::Error { id: rid, .. })) if rid == id => {
                    return None;
                }
                Ok(Some(LspMessage::Response { id: rid, result })) => {
                    self.pending_responses.insert(rid, result);
                }
                Ok(Some(LspMessage::Error { id: rid, .. })) => {
                    self.pending_responses.insert(rid, Value::Null);
                }
                Ok(Some(LspMessage::Notification { method, params })) => {
                    self.handle_notification(&method, params);
                }
                _ => {}
            }
        }
    }

    // ── LSP handshake ─────────────────────────────────────────────────────────

    async fn do_initialize(&mut self) -> Result<(), Box<dyn std::error::Error>> {
        let params = json!({
            "processId": std::process::id(),
            "clientInfo": { "name": "aee", "version": "1.0.0" },
            "rootUri": null,
            "capabilities": {
                "textDocument": {
                    "synchronization": {
                        "didSave": true,
                        "willSave": false
                    },
                    "completion": {
                        "completionItem": {
                            "snippetSupport": false,
                            "documentationFormat": ["plaintext"]
                        }
                    },
                    "publishDiagnostics": {
                        "relatedInformation": false
                    },
                    "semanticTokens": {
                        "requests": {
                            "full": true,
                            "range": false
                        },
                        "tokenTypes": TOKEN_TYPES,
                        "tokenModifiers": [],
                        "formats": ["relative"],
                        "multilineTokenSupport": false,
                        "overlappingTokenSupport": false
                    }
                },
                "workspace": {
                    "workspaceFolders": false,
                    "didChangeWatchedFiles": { "dynamicRegistration": false }
                }
            }
        });

        let id = self.send_request("initialize", params).await?;
        // Wait for server's initialize result; store token legend if provided
        if let Some(result) = self.wait_for_response(id).await {
            if let Some(legend) = result
                .pointer("/capabilities/semanticTokensProvider/legend/tokenTypes")
                .and_then(|v| v.as_array())
            {
                self.token_type_legend = legend
                    .iter()
                    .filter_map(|t| t.as_str().map(|s| s.to_string()))
                    .collect();
            }
        }

        // Send the mandatory "initialized" notification
        self.send_notification("initialized", json!({})).await?;
        Ok(())
    }

    // ── Public API ────────────────────────────────────────────────────────────

    /// Notify the server that a document was opened.
    pub async fn did_open(
        &mut self,
        uri: &str,
        language_id: &str,
        text: &str,
    ) -> Result<(), Box<dyn std::error::Error>> {
        self.send_notification(
            "textDocument/didOpen",
            json!({
                "textDocument": {
                    "uri": uri,
                    "languageId": language_id,
                    "version": 1,
                    "text": text
                }
            }),
        )
        .await
    }

    /// Notify the server that a document changed (full-document sync).
    pub async fn did_change(
        &mut self,
        uri: &str,
        version: i32,
        text: &str,
    ) -> Result<(), Box<dyn std::error::Error>> {
        self.send_notification(
            "textDocument/didChange",
            json!({
                "textDocument": { "uri": uri, "version": version },
                "contentChanges": [{ "text": text }]
            }),
        )
        .await
    }

    /// Notify the server that a document was saved.
    pub async fn did_save(&mut self, uri: &str) -> Result<(), Box<dyn std::error::Error>> {
        self.send_notification(
            "textDocument/didSave",
            json!({ "textDocument": { "uri": uri } }),
        )
        .await
    }

    /// Request completion items at a position.
    /// Returns the raw JSON result (list or CompletionList).
    pub async fn completion(
        &mut self,
        uri: &str,
        line: u32,
        character: u32,
    ) -> Result<Option<Value>, Box<dyn std::error::Error>> {
        let id = self
            .send_request(
                "textDocument/completion",
                json!({
                    "textDocument": { "uri": uri },
                    "position": { "line": line, "character": character }
                }),
            )
            .await?;
        Ok(self.wait_for_response(id).await)
    }

    /// Request semantic tokens for an entire document.
    /// On success, decodes and caches them in `self.semantic_tokens[uri]`.
    pub async fn semantic_tokens_full(
        &mut self,
        uri: &str,
    ) -> Result<(), Box<dyn std::error::Error>> {
        let id = self
            .send_request(
                "textDocument/semanticTokens/full",
                json!({ "textDocument": { "uri": uri } }),
            )
            .await?;

        if let Some(result) = self.wait_for_response(id).await {
            if let Some(data_arr) = result["data"].as_array() {
                let data: Vec<u64> = data_arr
                    .iter()
                    .filter_map(|v| v.as_u64())
                    .collect();
                let tokens = decode_semantic_tokens(&data);
                self.semantic_tokens.insert(uri.to_string(), tokens);
            }
        }
        Ok(())
    }

    /// Graceful shutdown.
    pub async fn shutdown(mut self) -> Result<(), Box<dyn std::error::Error>> {
        let id = self.send_request("shutdown", Value::Null).await?;
        let _ = self.wait_for_response(id).await;
        self.send_notification("exit", json!({})).await?;
        Ok(())
    }

    /// Get cached diagnostics for a URI, if any.
    pub fn get_diagnostics(&self, uri: &str) -> Option<&Value> {
        self.diagnostics.get(uri)
    }

    /// Get cached semantic tokens for a URI, if any.
    pub fn get_semantic_tokens(&self, uri: &str) -> Option<&Vec<SemanticToken>> {
        self.semantic_tokens.get(uri)
    }
}
