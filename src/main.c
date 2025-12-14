#include "editor.h"
#include "event.h"
#include "feed.h"
#include "network.h"
#include "terminal.h"

static void kirc_usage(const char *argv0)
{
    fprintf(stderr,
        "Usage: %s [args] <nickname>\n"
        "\n"
        "Optional Arguments:\n"
        "  -s <hostname>  Server hostname (default: irc.libera.chat)\n"
        "  -p <port>      Server port (default: 6667)\n"
        "  -c <channels>  List of channel(s) (default: #chat)\n"
        "  -r <realname>  User real name\n"
        "  -u <username>  Account username\n"
        "  -k <password>  Account password\n"
        "  -h             Show this help\n",
        argv0);
}

static int kirc_init(kirc_t *ctx)
{
    memset(ctx, 0, sizeof(*ctx));

    size_t hostname_n = sizeof(ctx->hostname) - 1;
    strncpy(ctx->hostname, "irc.libera.chat", hostname_n);

    size_t port_n = sizeof(ctx->port) - 1;
    strncpy(ctx->port, "6667", port_n);

    size_t channels_n = sizeof(ctx->channels[0]) - 1;
    strncpy(ctx->channels[0], "#chat", channels_n);

    ctx->tty_fd = STDIN_FILENO;
    ctx->lwidth = KIRC_LEFT_WIDTH;

    const char *env;

    env = getenv("KIRC_HOSTNAME");
    if (env && *env) {
        strncpy(ctx->hostname, env, hostname_n);
    }

    env = getenv("KIRC_PORT");
    if (env && *env) {
        strncpy(ctx->port, env, port_n);
    }

    env = getenv("KIRC_CHANNELS");
    if (env && *env) {
        size_t idx = 0;
        for (char *tok = strtok(optarg, ",|"); tok != NULL; tok = strtok(NULL, ",|")) {
            strncpy(ctx->channels[idx], tok, channels_n);
            idx++;
        }
    }

    env = getenv("KIRC_REALNAME");
    if (env && *env) {
        size_t realname_n = sizeof(ctx->realname) - 1;
        strncpy(ctx->realname, env, realname_n);
    }

    env = getenv("KIRC_USERNAME");
    if (env && *env) {
        size_t username_n = sizeof(ctx->username) - 1;
        strncpy(ctx->username, env, username_n);
    }

    env = getenv("KIRC_PASSWORD");
    if (env && *env) {
        size_t password_n = sizeof(ctx->password) - 1;
        strncpy(ctx->password, env, password_n);
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

    while ((opt = getopt(argc, argv, "s:p:r:u:k:c:h")) > 0) {
        switch (opt) {
        case 'h':  /* help */
            kirc_usage(argv[0]);
            return -1;

        case 's':  /* hostname */
            size_t hostname_n = sizeof(ctx->hostname) - 1;
            strncpy(ctx->hostname, optarg, hostname_n);
            break;

        case 'p':  /* port */
            size_t port_n = sizeof(ctx->port) - 1;
            strncpy(ctx->port, optarg, port_n);
            break;

        case 'r':  /* realname */
            size_t realname_n = sizeof(ctx->realname) - 1;
            strncpy(ctx->realname, optarg, realname_n);
            break;

        case 'u':  /* username */
            size_t username_n = sizeof(ctx->username) - 1;
            strncpy(ctx->username, optarg, username_n);
            break;

        case 'k':  /* password */
            size_t password_n = sizeof(ctx->password) - 1;
            strncpy(ctx->password, optarg, password_n);
            break;

        case 'c':  /* channel(s) */
            size_t idx = 0, channels_n = sizeof(ctx->channels[0]) - 1;
            for (char *tok = strtok(optarg, ",|"); tok != NULL; tok = strtok(NULL, ",|")) {
                strncpy(ctx->channels[idx], tok, channels_n);
                idx += 1;
            }
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

static kirc_error_t kirc_run(kirc_t *ctx)
{
    editor_t editor;

    if (editor_init(&editor, ctx) < 0) {
        fprintf(stderr, "editor_init failed\n");
        return KIRC_ERR_INTERNAL;
    }

    network_t network;

    if (network_init(&network, ctx) < 0) {
        fprintf(stderr, "network_init failed\n");
        return KIRC_ERR_INTERNAL;
    }

    if (network_connect(&network) < 0) {
        fprintf(stderr, "network_connect failed\n");
        return KIRC_ERR_INTERNAL;
    }

    if (ctx->nickname[0] == '\0') {
        fprintf(stderr, "nickname not specified\n");
        return KIRC_ERR_PARSE;
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

    if (ctx->password[0] != '\0') {
        network_send(&network, "PASS %s\r\n",
            ctx->password);
    }

    terminal_t terminal;

    if (terminal_init(&terminal, ctx) < 0) {
        fprintf(stderr, "terminal_init failed\n");
        return KIRC_ERR_INTERNAL;
    }

    if (terminal_enable_raw(&terminal) < 0) {
        fprintf(stderr, "terminal_enable_raw: failed\n");
        return KIRC_ERR_IO;
    }

    struct pollfd fds[2] = {
        { .fd = ctx->tty_fd, .events = POLLIN },
        { .fd = network.fd, .events = POLLIN }
    };

    for (;;) {
        if (poll(fds, 2, -1) == -1) {
            if (errno == EINTR) continue;
            //perror("poll");
            break;
        }

        if (fds[0].revents & POLLIN) {
            if (editor_process_key(&editor) < 0) {
                break;
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

                    event_t event;
                    event_init(&event, ctx, msg);

                    if (event.type == EVENT_PING) {
                        network_send(&network, "PONG\r\n");
                    }

                    if (event.type == EVENT_JOIN) {
                        for (int i = 0; ctx->channels[i][0] != '\0'; ++i) {
                            network_send(&network, "JOIN %s\r\n",
                                ctx->channels[i]);
                        }
                    }

                    feed_render(&event);

                    msg = eol + 2;
                }
                
                size_t remain = network.buffer
                    + network.len - msg;
                memmove(network.buffer, msg, remain);
                network.len = remain;
            }
            editor_render(&editor);
        }

    } 

    terminal_disable_raw(&terminal);

    return KIRC_OK;
}

int main(int argc, char *argv[])
{
    kirc_t ctx;

    if (kirc_init(&ctx) < 0) {
        return 1;
    }

    if (kirc_args(&ctx, argc, argv) < 0) {
        return 1;
    }

    kirc_error_t err = kirc_run(&ctx);

    return (err == KIRC_OK) ? 0 : 1;
}
