#include "editor.h"
#include "network.h"
#include "protocol.h"
#include "terminal.h"

static void kirc_parse_channels(kirc_t *ctx,
        char *value)
{
    char *tok = NULL;
    size_t siz = 0, idx = 0;

    for (tok = strtok(value, ",|"); tok != NULL; tok = strtok(NULL, ",|")) {
        siz = sizeof(ctx->channels[idx]) - 1;
        strncpy(ctx->channels[idx], tok, siz);
        idx++;
    }
}

static void kirc_parse_mechanism(kirc_t *ctx,
        char *value)
{
    char *mechanism = strtok(value, ":");
    char *token = strtok(NULL, ":");
    size_t siz = 0;

    if (strcmp(mechanism, "EXTERNAL") == 0) {
        ctx->mechanism = SASL_EXTERNAL;
    } else if (strcmp(mechanism, "PLAIN") == 0) {
        ctx->mechanism = SASL_PLAIN;
        siz = sizeof(ctx->auth) - 1;
        strncpy(ctx->auth, token, siz);
    }
}

static int kirc_init(kirc_t *ctx)
{
    memset(ctx, 0, sizeof(*ctx));
    
    size_t siz = 0;

    siz = sizeof(ctx->server) - 1;
    strncpy(ctx->server, "irc.libera.chat", siz);
    
    siz = sizeof(ctx->port) - 1;
    strncpy(ctx->port, "6667", siz);

    ctx->mechanism = SASL_NONE;

    char *env;

    env = getenv("KIRC_SERVER");
    if (env && *env) {
        siz = sizeof(ctx->server) - 1;
        strncpy(ctx->server, env, siz);
    }

    env = getenv("KIRC_PORT");
    if (env && *env) {
        siz = sizeof(ctx->port) - 1;
        strncpy(ctx->port, env, siz);
    }

    env = getenv("KIRC_CHANNELS");
    if (env && *env) {
        kirc_parse_channels(ctx, env);
    }

    env = getenv("KIRC_REALNAME");
    if (env && *env) {
        siz = sizeof(ctx->realname) - 1;
        strncpy(ctx->realname, env, siz);
    }

    env = getenv("KIRC_USERNAME");
    if (env && *env) {
        siz = sizeof(ctx->username) - 1;
        strncpy(ctx->username, env, siz);
    }

    env = getenv("KIRC_PASSWORD");
    if (env && *env) {
        siz = sizeof(ctx->password) - 1;
        strncpy(ctx->password, env, siz);
    }

    env = getenv("KIRC_AUTH");
    if (env && *env) {
        kirc_parse_mechanism(ctx, env);
    }

    return 0;
}

static int kirc_args(kirc_t *ctx, int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "%s: no arguments\n", argv[0]);
        return -1;
    }

    int opt;
    size_t siz = 0;

    while ((opt = getopt(argc, argv, "s:p:r:u:k:c:a:")) > 0) {
        switch (opt) {
        case 's':  /* server */
            siz = sizeof(ctx->server) - 1;
            strncpy(ctx->server, optarg, siz);
            break;

        case 'p':  /* port */
            siz = sizeof(ctx->port) - 1;
            strncpy(ctx->port, optarg, siz);
            break;

        case 'r':  /* realname */
            siz = sizeof(ctx->realname) - 1;
            strncpy(ctx->realname, optarg, siz);
            break;

        case 'u':  /* username */
            siz = sizeof(ctx->username) - 1;
            strncpy(ctx->username, optarg, siz);
            break;

        case 'k':  /* password */
            siz = sizeof(ctx->password) - 1;
            strncpy(ctx->password, optarg, siz);
            break;

        case 'c':  /* channel(s) */
            kirc_parse_channels(ctx, optarg);
            break;

        case 'a':  /* SASL authentication */
            kirc_parse_mechanism(ctx, optarg);
            break;

        case ':':
            fprintf(stderr, "%s: missing -%c value\n",
                argv[0], opt);
            return -1;

        case '?':
            fprintf(stderr, "%s: unknown argument\n",
                argv[0]);
            return -1;

        default:
            return -1;
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "nickname not specified\n");
        return -1;
    }

    size_t nickname_n = sizeof(ctx->nickname) - 1;
    strncpy(ctx->nickname, argv[optind], nickname_n);

    return 0;
}

static int kirc_run(kirc_t *ctx)
{
    editor_t editor;

    if (editor_init(&editor, ctx) < 0) {
        fprintf(stderr, "editor_init failed\n");
        return -1;
    }

    network_t network;

    if (network_init(&network, ctx) < 0) {
        fprintf(stderr, "network_init failed\n");
        return -1;
    }

    if (network_connect(&network) < 0) {
        fprintf(stderr, "network_connect failed\n");
        return -1;
    }

    if (ctx->mechanism != SASL_NONE) {
        network_send(&network, "CAP REQ :sasl\r\n");
    }

    network_send(&network, "NICK %s\r\n", ctx->nickname);
    
    char *username, *realname;
    
    if (ctx->username[0] != '\0') {
        username = ctx->username;
    } else {
        username = ctx->nickname;
    }

    if (ctx->realname[0] != '\0') {
        realname = ctx->realname;
    } else {
        realname = ctx->nickname;
    }

    network_send(&network, "USER %s - - :%s\r\n",
        username, realname);

    if (ctx->mechanism != SASL_NONE) {
        network_send(&network, "AUTHENTICATE %s\r\n",
            (ctx->mechanism == SASL_EXTERNAL) ? "EXTERNAL" : "PLAIN");
    }

    if (ctx->password[0] != '\0') {
        network_send(&network, "PASS %s\r\n",
            ctx->password);
    }

    size_t selected_n = sizeof(ctx->selected) - 1;
    strncpy(ctx->selected, ctx->channels[0], selected_n);

    terminal_t terminal;

    if (terminal_init(&terminal, ctx) < 0) {
        fprintf(stderr, "terminal_init failed\n");
        return -1;
    }

    if (terminal_enable_raw(&terminal) < 0) {
        fprintf(stderr, "terminal_enable_raw: failed\n");
        return -1;
    }

    struct pollfd fds[2] = {
        { .fd = STDIN_FILENO, .events = POLLIN },
        { .fd = network.fd, .events = POLLIN }
    };

    for (;;) {
        if (poll(fds, 2, -1) == -1) {
            if (errno == EINTR) continue;
            //perror("poll");
            break;
        }

        if (fds[0].revents & POLLIN) {
            editor_process_key(&editor);

            if (editor.event == EDITOR_EVENT_TERMINATE)
                break;

            if (editor.event == EDITOR_EVENT_SEND) {
                char *msg = editor_last_entry(&editor);
                network_command_handler(&network, msg);
            }

            editor_render(&editor);
        }

        if (fds[1].revents & POLLIN) {
            if (network_receive(&network) > 0) {
                char *msg = network.buffer;

                for (;;) {
                    char *eol = strstr(msg, "\r\n");
                    if (!eol) break; 

                    *eol = '\0';

                    protocol_t protocol;
                    protocol_init(&protocol, ctx);
                    protocol_parse(&protocol, msg);

                    switch(protocol.event) {
                    case PROTOCOL_EVENT_PING:
                        network_send(&network, "PONG :%s\r\n",
                            protocol.message);
                        break;

                    case PROTOCOL_EVENT_EXT_AUTHENTICATE:
                        network_authenticate(&network);
                        break;

                    case PROTOCOL_EVENT_001_RPL_WELCOME:
                        network_join_channels(&network);
                        break;

                    default:
                        break; 
                    }

                    protocol_render(&protocol);

                    msg = eol + 2;
                }

                size_t remain = network.buffer
                    + network.len - msg;
                memmove(network.buffer, msg, remain);
                network.len = remain;

                editor_render(&editor);
            }
            
        }

    }

    terminal_disable_raw(&terminal);
    network_free(&network);

    return 0;
}

int main(int argc, char *argv[])
{
    kirc_t ctx;

    if (kirc_init(&ctx) < 0) {
        return EXIT_FAILURE;
    }

    if (kirc_args(&ctx, argc, argv) < 0) {
        return EXIT_FAILURE;
    }

    if (kirc_run(&ctx) < 0) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
