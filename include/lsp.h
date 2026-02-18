#ifndef LSP_H
#define LSP_H

#include <pthread.h>
#include "../third_party/cJSON/cJSON.h"
#include "../include/aee.h"

// LSP server structure
typedef struct {
    pid_t pid;
    int to_server[2];   // pipe to server stdin
    int from_server[2]; // pipe from server stdout
    pthread_t reader_thread;
    int request_id;
} lsp_server_t;

// Function declarations
int lsp_init(const char *server_command);
void lsp_shutdown(void);
void lsp_did_open(const char *uri, const char *language_id, const char *text);
void lsp_did_change(const char *uri, int version, const char *text);
void lsp_completion(const char *uri, int line, int character, void (*callback)(cJSON *result));
void lsp_diagnostics(const char *uri, void (*callback)(cJSON *result));
void *lsp_reader_thread(void *arg);

#endif // LSP_H
