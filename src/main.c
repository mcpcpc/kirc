/*
 * main.c
 * Main entry point for the kirc IRC client
 * Author: Michael Czigler
 * License: MIT
 */

#include "config.h"
#include "ctcp.h"
#include "dcc.h"
#include "editor.h"
#include "helper.h"
#include "network.h"
#include "protocol.h"
#include "terminal.h"
#include "transport.h"

static int kirc_run(struct kirc_context *ctx)
{
    struct editor editor;

    if (editor_init(&editor, ctx) < 0) {
        fprintf(stderr, "editor_init failed\n");
        return -1;
    }

    struct transport transport;

    if (transport_init(&transport, ctx) < 0) {
        fprintf(stderr, "transport_init failed\n");
        return -1;
    }

    struct network network;

    if (network_init(&network, &transport, ctx) < 0) {
        fprintf(stderr, "network_init failed\n");
        return -1;
    }

    struct dcc dcc;

    if (dcc_init(&dcc, ctx) < 0) {
        fprintf(stderr, "dcc_init failed\n");
        network_free(&network);
        return -1;
    }

    if (network_connect(&network) < 0) {
        fprintf(stderr, "network_connect failed\n");
        dcc_free(&dcc);
        network_free(&network);
        return -1;
    }

    if (network_send_credentials(&network) < 0) {
        fprintf(stderr, "network_send_credentials failed\n");
        dcc_free(&dcc);
        network_free(&network);
        return -1;
    }

    size_t siz = sizeof(ctx->target);
    safecpy(ctx->target, ctx->channels[0], siz);

    struct terminal terminal;

    if (terminal_init(&terminal, ctx) < 0) {
        fprintf(stderr, "terminal_init failed\n");
        dcc_free(&dcc);
        network_free(&network);
        return -1;
    }

    if (terminal_enable_raw(&terminal) < 0) {
        fprintf(stderr, "terminal_enable_raw failed\n");
        terminal_disable_raw(&terminal);
        dcc_free(&dcc);
        network_free(&network);
        return -1;
    }

    struct pollfd fds[2] = {
        { .fd = STDIN_FILENO, .events = POLLIN },
        { .fd = network.transport->fd, .events = POLLIN }
    };

    for (;;) {
        int rc = poll(fds, 2, -1);

        if (rc == -1) {
            if (errno == EINTR) {
                continue;
            }

            terminal_disable_raw(&terminal);
            fprintf(stderr, "poll error: %s\n",
                strerror(errno));
            break;
        }

        if (rc == 0) {
            continue;
        }

        if (fds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) {
            terminal_disable_raw(&terminal);
            fprintf(stderr, "stdin error or hangup\n");
            break;
        }

        if (fds[1].revents & (POLLERR | POLLHUP | POLLNVAL)) {
            terminal_disable_raw(&terminal);
            fprintf(stderr, "network connection error or closed\n");
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

            editor_handle(&editor);
        }

        if (fds[1].revents & POLLIN) {
            int recv = network_receive(&network);

            if (recv < 0) {
                terminal_disable_raw(&terminal);
                fprintf(stderr, "network_receive error\n");
                break;
            }

            if (recv > 0) {
                char *msg = network.buffer;
                size_t remaining = network.len;

                for (;;) {
                    char *eol = find_message_end(msg, remaining);
                    if (!eol) break; /* no complete message found */

                    *eol = '\0';

                    struct protocol protocol;
                    protocol_init(&protocol, ctx);
                    protocol_parse(&protocol, msg);

                    switch(protocol.event) {
                    case PROTOCOL_EVENT_CTCP_CLIENTINFO:
                    case PROTOCOL_EVENT_CTCP_PING:
                    case PROTOCOL_EVENT_CTCP_TIME:
                    case PROTOCOL_EVENT_CTCP_VERSION:
                        ctcp_handle_event(&network, &protocol);
                        break;

                    case PROTOCOL_EVENT_CTCP_DCC:
                        if (strcmp(protocol.command, "PRIVMSG") == 0) {
                            dcc_request(&dcc, protocol.nickname,
                                protocol.message);
                        }
                        break;

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

                    protocol_handle(&protocol);

                    msg = eol + 2;
                    remaining = network.buffer + network.len - msg;
                }

                if (remaining > 0) {
                    if ((remaining < sizeof(network.buffer)) &&
                        (msg > network.buffer)) {
                        memmove(network.buffer, msg, remaining);
                    }
                    network.len = remaining;
                } else {
                    network.len = 0;
                }

                editor_handle(&editor);
            }
            
        }

        dcc_process(&dcc);
    }

    terminal_disable_raw(&terminal);
    dcc_free(&dcc);
    network_free(&network);

    return 0;
}

int main(int argc, char *argv[])
{
    struct kirc_context ctx;

    if (config_init(&ctx) < 0) {
        return EXIT_FAILURE;
    }

    if (config_parse_args(&ctx, argc, argv) < 0) {
        config_free(&ctx);
        return EXIT_FAILURE;
    }

    if (kirc_run(&ctx) < 0) {
        config_free(&ctx);
        return EXIT_FAILURE;
    }

    config_free(&ctx);

    return EXIT_SUCCESS;
}
