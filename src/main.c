#include "editor.h"
#include "event.h"
#include "network.h"
#include "terminal.h"

static int kirc_init(kirc_t *ctx)
{
    memset(ctx, 0, sizeof(*ctx));

    size_t hostname_n = sizeof(ctx->hostname) - 1;
    strncpy(ctx->hostname, "irc.libera.chat", hostname_n);

    size_t port_n = sizeof(ctx->port) - 1;
    strncpy(ctx->port, "6667", port_n);

    ctx->tty_fd = STDIN_FILENO;

    return 0;
}

static int kirc_args(kirc_t *ctx, int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "%s: no arguments\n", argv[0]);
        return -1;
    }

    int opt;

    while ((opt = getopt(argc, argv, "s:p:n:r:u:k:l:")) > 0) {
        switch (opt) {
        case 's':  /* hostname */
            size_t hostname_n = sizeof(ctx->hostname) - 1;
            strncpy(ctx->hostname, optarg, hostname_n);
            break;

        case 'p':  /* port */
            size_t port_n = sizeof(ctx->port) - 1;
            strncpy(ctx->port, optarg, port_n);
            break;

        case 'n':  /* nickname */
            size_t nickname_n = sizeof(ctx->nickname) - 1;
            strncpy(ctx->nickname, optarg, nickname_n);
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

        case 'l':  /* log file path */
            size_t log_n = sizeof(ctx->log) - 1;
            strncpy(ctx->log, optarg, log_n);
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

    return 0;
}

static int kirc_run(kirc_t *ctx)
{
    if (network_connect(ctx) < 0) {
        fprintf(stderr, "network_connect failed\n");
        return 1;
    }

    if (ctx->nickname[0] == '\0') {
        fprintf(stderr, "nickname not specified\n");
        return 1;
    }

    network_send(ctx, "NICK %s\r\n", ctx->nickname);
    
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

    network_send(ctx, "USER %s - - :%s\r\n",
        username, realname);

    if (ctx->password[0] != '\0') {
        network_send(ctx, "PASS %s\r\n", ctx->password);
    }

    if (terminal_enable_raw(ctx) < 0) {
        fprintf(stderr, "terminal_enable_raw: failed\n");
        return 1;
    }

    struct pollfd fds[2] = {
        { .fd = ctx->tty_fd, .events = POLLIN },
        { .fd = ctx->socket_fd, .events = POLLIN }
    };

    for (;;) {
        if (poll(fds, 2, -1) == -1) {
            if (errno == EINTR) continue;
            perror("poll");
            break;
        }

        if (fds[0].revents & POLLIN) {
            int rc = editor_process_key(ctx);
            if (rc < 0) {
                break;
            }
        }

        if (fds[1].revents & POLLIN) {
            if (network_receive(ctx) > 0) {
                char *msg = ctx->socket_buffer;

                for (;;) {
                    char *eol = strstr(msg, "\r\n");
                    if (!eol) break;

                    event_t event;
                    event_init(&event, &msg);

                    if (event.type == EVENT_PING) {
                        network_send(ctx, "PONG\r\n");
                    }

                    msg = eol += 2;
                }
                
                size_t remain = ctx->socket_buffer
                    + ctx->socket_len - msg;
                memmove(ctx->socket_buffer, msg, remain);
                ctx->socket_len = remain;
            }
        }

    } 

    terminal_disable_raw(ctx);

    return 0;
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

    if (kirc_run(&ctx) < 0) {
        return 1;
    }

    return 0;
}
