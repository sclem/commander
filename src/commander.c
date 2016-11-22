#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "mongoose.h"
#include "cJSON.h"
#include "util.h"

static const char *PORT_ENV = "COMMANDER_PORT";
static const char *WEBROOT_ENV = "COMMANDER_WEBROOT";
static const char *COMMANDS_ENV = "COMMANDS_PATH";

static struct mg_serve_http_opts opts;

typedef struct command {
    char *path;
    char *command;
} cmd;

static cmd **commandlist;

static char *get_command(const char *path) {
    for (int i = 0;; i++) {
        struct command *curr = commandlist[i];
        if (&(*curr) == NULL) {
            break;
        }
        if (strcmp(curr->path, path) == 0) {
            return curr->command;
        }
    }
    return NULL;
}

static void handle_404(struct mg_connection *c, int ev, void *p) {
    if (ev == MG_EV_HTTP_REQUEST) {
        mg_http_send_error(c, 404, "404 not found");
    }
}

static void send_resp(struct mg_connection *c, int code, const char *message) {
    mg_printf(c, "%s",
              "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\nContent-Type: "
              "application/json\r\n\r\n");
    mg_send_http_chunk(c, message, strlen(message));
    mg_send_http_chunk(c, "", 0);
    c->flags |= MG_F_SEND_AND_CLOSE;
}

static void handle_req(struct mg_connection *c, int ev, void *p) {
    struct http_message *hm = (struct http_message *)p;

    char *uri = malloc(sizeof(char) * hm->uri.len);
    strncpy(uri, hm->uri.p, hm->uri.len);
    char *cmd = get_command(uri);
    free(uri);
    if (!cmd) {
        send_resp(c, 404, "{\"message\":\"unknown\"}");
        return;
    }
    int ret = system(cmd);
    if (ret == 0) {
        send_resp(c, 200, "{\"message\":\"success\"}");
    } else {
        send_resp(c, 400, "{\"message\":\"error\"}");
    }
}

static void handle_root(struct mg_connection *c, int ev, void *p) {
    struct http_message *hm = (struct http_message *)p;
    mg_serve_http(c, hm, opts);
}

static char *read_file(char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        printf("Cannot open file\n");
        exit(1);
    }
    fseek(f, 0, SEEK_END);
    long flen = ftell(f);
    rewind(f);

    char *buf = (char *)malloc((flen + 1) * sizeof(char));
    fread(buf, flen, 1, f);
    fclose(f);

    return buf;
}

int main(int argc, char **argv) {
    struct mg_mgr mgr;
    struct mg_connection *c;

    opts.document_root = getsetenv(WEBROOT_ENV, "public");
    opts.index_files = "index.html";
    opts.enable_directory_listing = "no";

    char *commands_filepath = getsetenv(COMMANDS_ENV, "commands.json");

    mg_mgr_init(&mgr, NULL);

    char *port = getsetenv(PORT_ENV, "8080");
    c = mg_bind(&mgr, port, handle_404);
    if (c == NULL) {
        printf("Failed to create listener. Exiting.\n");
        return 1;
    }

    char *data = read_file(commands_filepath);
    cJSON *root = cJSON_Parse(data);
    cJSON *curr = root->child;
    commandlist = malloc(sizeof(cmd));
    int i = 0;
    while (curr) {
        if (i > 0) {
            commandlist = realloc(commandlist, sizeof(cmd) * (i + 1));
        }
        cmd *cm = malloc(sizeof(cmd));
        cm->path = cJSON_GetObjectItem(curr, "path")->valuestring;
        cm->command = cJSON_GetObjectItem(curr, "command")->valuestring;
        commandlist[i++] = cm;
        printf("Registering path %s for command: \"%s\"\n", cm->path,
               cm->command);
        mg_register_http_endpoint(c, cm->path, handle_req);
        curr = curr->next;
    }

    mg_register_http_endpoint(c, "/", handle_root);

    printf("Starting web server on port %s\n", port);

    mg_set_protocol_http_websocket(c);

    while (true) {
        mg_mgr_poll(&mgr, 1000);
    }

    mg_mgr_free(&mgr);
    return 0;
}
