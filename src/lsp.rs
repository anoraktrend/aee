// LSP module for language server communication

use tokio::process::{Child, Command};
use serde_json::json;
use tokio::io::AsyncWriteExt;
use std::process::Stdio;

pub struct LspClient {
    _process: Child,
    stdin: tokio::process::ChildStdin,
    request_id: i64,
}

impl LspClient {
    pub async fn new(server_command: &str) -> Result<Self, Box<dyn std::error::Error>> {
        let mut child = Command::new("sh")
            .arg("-c")
            .arg(server_command)
            .stdin(Stdio::piped())
            .stdout(Stdio::piped())
            .spawn()?;

        let stdin = child.stdin.take().unwrap();

        Ok(LspClient {
            _process: child,
            stdin,
            request_id: 0,
        })
    }

    fn next_id(&mut self) -> i64 {
        self.request_id += 1;
        self.request_id
    }

    pub async fn shutdown(mut self) -> Result<(), Box<dyn std::error::Error>> {
        let request = json!({
            "jsonrpc": "2.0",
            "id": self.next_id(),
            "method": "shutdown",
            "params": null
        });
        let json_str = request.to_string();
        let message = format!("Content-Length: {}\r\n\r\n{}", json_str.len(), json_str);
        self.stdin.write_all(message.as_bytes()).await?;

        let request = json!({
            "jsonrpc": "2.0",
            "id": self.next_id(),
            "method": "exit",
            "params": null
        });
        let json_str = request.to_string();
        let message = format!("Content-Length: {}\r\n\r\n{}", json_str.len(), json_str);
        self.stdin.write_all(message.as_bytes()).await?;

        Ok(())
    }

    pub async fn did_open(&mut self, uri: &str, language_id: &str, text: &str) -> Result<(), Box<dyn std::error::Error>> {
        let params = json!({
            "textDocument": {
                "uri": uri,
                "languageId": language_id,
                "version": 1,
                "text": text
            }
        });
        let request = json!({
            "jsonrpc": "2.0",
            "id": self.next_id(),
            "method": "textDocument/didOpen",
            "params": params
        });
        let json_str = request.to_string();
        let message = format!("Content-Length: {}\r\n\r\n{}", json_str.len(), json_str);
        self.stdin.write_all(message.as_bytes()).await?;
        Ok(())
    }

    pub async fn did_change(&mut self, uri: &str, version: i32, text: &str) -> Result<(), Box<dyn std::error::Error>> {
        let params = json!({
            "textDocument": {
                "uri": uri,
                "version": version
            },
            "contentChanges": [
                {
                    "text": text
                }
            ]
        });
        let request = json!({
            "jsonrpc": "2.0",
            "id": self.next_id(),
            "method": "textDocument/didChange",
            "params": params
        });
        let json_str = request.to_string();
        let message = format!("Content-Length: {}\r\n\r\n{}", json_str.len(), json_str);
        self.stdin.write_all(message.as_bytes()).await?;
        Ok(())
    }

    pub async fn completion(&mut self, uri: &str, line: u32, character: u32) -> Result<(), Box<dyn std::error::Error>> {
        let params = json!({
            "textDocument": {
                "uri": uri
            },
            "position": {
                "line": line,
                "character": character
            }
        });
        let request = json!({
            "jsonrpc": "2.0",
            "id": self.next_id(),
            "method": "textDocument/completion",
            "params": params
        });
        let json_str = request.to_string();
        let message = format!("Content-Length: {}\r\n\r\n{}", json_str.len(), json_str);
        self.stdin.write_all(message.as_bytes()).await?;
        Ok(())
    }
}
