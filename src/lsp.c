#include "../include/lsp.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/wait.h>
#include <fcntl.h>

lsp_server_t *lsp_server = NULL;

int lsp_init(const char *server_command) {
    if (lsp_server) return 0; // already initialized

    lsp_server = calloc(1, sizeof(lsp_server_t));
    if (!lsp_server) return -1;

    // Create pipes
    if (pipe(lsp_server->to_server) == -1 || pipe(lsp_server->from_server) == -1) {
        free(lsp_server);
        return -1;
    }

    lsp_server->pid = fork();
    if (lsp_server->pid == -1) {
        close(lsp_server->to_server[0]);
        close(lsp_server->to_server[1]);
        close(lsp_server->from_server[0]);
        close(lsp_server->from_server[1]);
        free(lsp_server);
        return -1;
    }

    if (lsp_server->pid == 0) { // child
        close(lsp_server->to_server[1]);
        close(lsp_server->from_server[0]);
        dup2(lsp_server->to_server[0], STDIN_FILENO);
        dup2(lsp_server->from_server[1], STDOUT_FILENO);
        dup2(lsp_server->from_server[1], STDERR_FILENO);
        execl("/bin/sh", "sh", "-c", server_command, NULL);
        exit(1);
    } else { // parent
        close(lsp_server->to_server[0]);
        close(lsp_server->from_server[1]);
        // Start reader thread
        pthread_create(&lsp_server->reader_thread, NULL, lsp_reader_thread, NULL);
    }

    return 0;
}

void lsp_shutdown(void) {
    if (!lsp_server) return;

    kill(lsp_server->pid, SIGTERM);
    waitpid(lsp_server->pid, NULL, 0);
    close(lsp_server->to_server[1]);
    close(lsp_server->from_server[0]);
    pthread_join(lsp_server->reader_thread, NULL);
    free(lsp_server);
    lsp_server = NULL;
}

void lsp_send_request(const char *method, cJSON *params) {
    if (!lsp_server) return;

    cJSON *request = cJSON_CreateObject();
    cJSON_AddStringToObject(request, "jsonrpc", "2.0");
    cJSON_AddNumberToObject(request, "id", ++lsp_server->request_id);
    cJSON_AddStringToObject(request, "method", method);
    if (params) cJSON_AddItemToObject(request, "params", params);

    char *json_str = cJSON_Print(request);
    dprintf(lsp_server->to_server[1], "Content-Length: %zu\r\n\r\n%s", strlen(json_str), json_str);
    free(json_str);
    cJSON_Delete(request);
}

void lsp_did_open(const char *uri, const char *language_id, const char *text) {
    cJSON *params = cJSON_CreateObject();
    cJSON *text_document = cJSON_CreateObject();
    cJSON_AddStringToObject(text_document, "uri", uri);
    cJSON_AddStringToObject(text_document, "languageId", language_id);
    cJSON_AddStringToObject(text_document, "text", text);
    cJSON_AddNumberToObject(text_document, "version", 1);
    cJSON_AddItemToObject(params, "textDocument", text_document);
    lsp_send_request("textDocument/didOpen", params);
}

void lsp_did_change(const char *uri, int version, const char *text) {
    cJSON *params = cJSON_CreateObject();
    cJSON *text_document = cJSON_CreateObject();
    cJSON_AddStringToObject(text_document, "uri", uri);
    cJSON_AddNumberToObject(text_document, "version", version);
    cJSON *content_changes = cJSON_CreateArray();
    cJSON *change = cJSON_CreateObject();
    cJSON_AddStringToObject(change, "text", text);
    cJSON_AddItemToArray(content_changes, change);
    cJSON_AddItemToObject(params, "textDocument", text_document);
    cJSON_AddItemToObject(params, "contentChanges", content_changes);
    lsp_send_request("textDocument/didChange", params);
}

void lsp_completion(const char *uri, int line, int character, void (*callback)(cJSON *result)) {
    cJSON *params = cJSON_CreateObject();
    cJSON *text_document = cJSON_CreateObject();
    cJSON_AddStringToObject(text_document, "uri", uri);
    cJSON *position = cJSON_CreateObject();
    cJSON_AddNumberToObject(position, "line", line);
    cJSON_AddNumberToObject(position, "character", character);
    cJSON_AddItemToObject(params, "textDocument", text_document);
    cJSON_AddItemToObject(params, "position", position);
    lsp_send_request("textDocument/completion", params);
    // Note: callback not implemented yet
}

void lsp_diagnostics(const char *uri, void (*callback)(cJSON *result)) {
    // Diagnostics are sent asynchronously, handled in reader thread
}

void *lsp_reader_thread(void *arg) {
    char buffer[4096];
    ssize_t n;
    while ((n = read(lsp_server->from_server[0], buffer, sizeof(buffer))) > 0) {
        // Parse JSON response
        cJSON *response = cJSON_ParseWithLength(buffer, n);
        if (response) {
            // Handle response
            cJSON_Delete(response);
        }
    }
    return NULL;
}
