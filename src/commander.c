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

struct command {
    char *path;
    char *command;
};

static void ev_handler(struct mg_connection *c, int ev, void *p) {
    if (ev == MG_EV_HTTP_REQUEST) {
        mg_http_send_error(c, 404, "404 not found");
    }
}

static void send_resp(struct mg_connection *c, int code, const char *message) {
    mg_send_head(c, code, strlen(message),
                 "Content-Type: application/json\r\n");
    mg_send_http_chunk(c, message, strlen(message));
    mg_send_http_chunk(c, "", 0);
    c->flags |= MG_F_SEND_AND_CLOSE;
}

static void handle_req(struct mg_connection *c, int ev, void *p) {
    struct http_message *hm = (struct http_message *)p;
    printf("%s\n", hm->uri.p);
    int ret = 0;
    if (ret == 0) {
        send_resp(c, 200, "{\"message\":\"openvpn is running\"}");
    } else {
        send_resp(c, 400, "{\"message\":\"openvpn is not running\"}");
    }
}

static void handle_root(struct mg_connection *c, int ev, void *p) {
    struct http_message *hm = (struct http_message *)p;
    mg_serve_http(c, hm, opts);
}

char *read_file(char *path) {
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
    c = mg_bind(&mgr, port, ev_handler);
    if (c == NULL) {
        printf("Failed to create listener. Exiting.\n");
        return 1;
    }

    char *data = read_file(commands_filepath);
    cJSON *root = cJSON_Parse(data);
    cJSON *curr = root->child;
    void **commandarr = malloc(sizeof(void *));
    int i = 0;
    while (curr) {
        if (i > 0) {
            commandarr = realloc(commandarr, sizeof(void *) * (i + 1));
        }
        char *path = cJSON_GetObjectItem(curr, "path")->valuestring;
        char *command = cJSON_GetObjectItem(curr, "command")->valuestring;
        struct command *cmd = malloc(sizeof(command));
        cmd->path = path;
        cmd->command = command;
        *(commandarr + i) = cmd;

        printf("Registering path /%s for command: \"%s\"\n", path, command);
        char *newpath = malloc(sizeof(char) * (strlen(path) + 1));
        sprintf(newpath, "/%s", path);

        mg_register_http_endpoint(c, newpath, handle_req);
        free(newpath);
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
