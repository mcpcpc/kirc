#include "editor.h"
#include "log.h"
#include "network.h"
#include "parser.h"
#include "render.h"
#include "terminal.h"
#include "utf8.h"


static void handle_user_command(network_t *net,
        render_t  *render, const char *line)
{
    if (!line || !*line) {
        return;
    }

    if (line[0] == '/') {
        network_send(net, "%s\r\n", line + 1);
        return;
    }

    if (line[0] == '@') {
        const char *space = strchr(line, ' ');
        if (!space) {
            return;
        }

        size_t target_len = (size_t)(space - (line + 1));
        char target[CHA_MAX];

        if (target_len >= sizeof(target)) {
            return;
        }

        memcpy(target, line + 1, target_len);
        target[target_len] = '\0';

        const char *msg = space + 1;

        network_send(net, "PRIVMSG %s :%s\r\n", target, msg);
        return;
    }

    if (render->default_channel[0] != '\0') {
        network_send(net, "PRIVMSG #%s :%s\r\n",
            render->default_channel, line);
    }
}

int main(int argc, char *argv[])
{
    terminal_t term  = {0};
    utf8_t     utf8  = {0};
    history_t  hist  = {0};
    editor_t   editor = {0};
    parser_t   parser = {0};
    render_t   render = {0};
    log_t      log    = {0};
    network_t  net    = {0};

    /* ---------------- CLI ---------------- */

    int opt;
    while ((opt = getopt(argc, argv, "s:p:c:n:r:u:k:a:o:evV")) != -1) {
        switch (opt) {
        case 's': net.host = optarg; break;
        case 'p': net.port = optarg; break;
        case 'c': strncpy(render.default_channel,
                           optarg,
                           sizeof(render.default_channel) - 1);
                  break;
        case 'n': net.nick = optarg; break;
        case 'r': net.real = optarg; break;
        case 'u': net.user = optarg; break;
        case 'k': net.pass = optarg; break;
        case 'a': net.auth = optarg; break;
        case 'o': log.path = optarg; break;
        case 'e': net.sasl_enabled = 1; break;
        case 'v': render.verbose = 1; break;
        case 'V': version(); break;
        default:  usage();
        }
    }

    if (!net.host) net.host = "irc.libera.chat";
    if (!net.port) net.port = "6667";

    if (!net.nick) {
        fputs("Nick is required\n", stderr);
        return 1;
    }

    if (!net.user) net.user = net.nick;
    if (!net.real) net.real = net.nick;

    /* ---------------- Init subsystems ---------------- */

    if (terminal_open(&term) == -1) return 1;
    if (terminal_enable_raw(&term) == -1) return 1;
    atexit((void (*)(void))terminal_disable_raw);

    if (utf8_detect(&utf8, term.tty_fd, STDOUT_FILENO) == -1)
        return 1;

    if (history_init(&hist, HIS_MAX) == -1)
        return 1;

    char editbuf[MSG_MAX];

    editor.prompt   = render.default_channel;
    editor.buffer   = editbuf;
    editor.buffer_size = sizeof(editbuf);
    editor.history  = &hist;
    editor.utf8     = &utf8;
    editor.terminal = &term;

    editor_reset(&editor);

    /* ---------------- Network connect ---------------- */

    if (network_connect(&net) == -1) {
        return 1;
    }

    if (net.sasl_enabled && net.auth) {
        network_send(&net, "CAP REQ :sasl\r\n");
    }

    network_send(&net, "NICK %s\r\n", net.nick);
    network_send(&net, "USER %s 0 * :%s\r\n",
        net.user, net.real);

    if (net.sasl_enabled && net.auth) {
        network_send(&net, "AUTHENTICATE PLAIN\r\n");
        network_send(&net, "AUTHENTICATE %s\r\n", net.auth);
        network_send(&net, "CAP END\r\n");
    }

    if (net.pass) {
        network_send(&net, "PASS %s\r\n", net.pass);
    }

    struct pollfd fds[2] = {
        { .fd = term.tty_fd, .events = POLLIN },
        { .fd = net.socket_fd, .events = POLLIN }
    };

    for (;;) {
        render_editor_line(&render, &editor);

        if (poll(fds, 2, -1) == -1) {
            if (errno == EINTR) continue;
            perror("poll");
            break;
        }

        if (fds[0].revents & POLLIN) {
            int rc = editor_process_key(&editor);

            if (rc > 0) {
                history_add(&hist, editor.buffer);
                handle_user_command(&net, &render, editor.buffer);
                editor_reset(&editor);
            } else if (rc < 0) {
                puts("\r\n");
                break;
            }
        }

        if (fds[1].revents & POLLIN) {
            int rc = network_poll(&net);

            if (rc == -1 || rc == -2) {
                puts("\rConnection closed.");
                break;
            }

            if (rc > 0) {
                char *start = net.rx_buffer;

                for (;;) {
                    char *eol = strstr(start, "\r\n");
                    if (!eol) break;

                    *eol = '\0';

                    if (render.verbose && log.path) {
                        log_append(&log, start);
                    }

                    irc_event_t ev;
                    if (parser_feed(&parser, start, &ev) > 0) {

                        if (ev.type == IRC_EVENT_PING) {
                            network_send(&net,
                                         "PONG :%s\r\n",
                                         ev.message ? ev.message : "");
                        }

                        if (ev.type == IRC_EVENT_NUMERIC &&
                            ev.numeric == 1 &&
                            render.default_channel[0] != '\0')
                        {
                            network_send(&net,
                                         "JOIN #%s\r\n",
                                         render.default_channel);
                        }

                        render_event(&render, &ev,
                            terminal_columns(&term));
                    }

                    start = eol + 2;
                }

                size_t remain =
                    net.rx_buffer + net.rx_len - start;

                memmove(net.rx_buffer, start, remain);
                net.rx_len = remain;
            }
        }
    }

    history_free(&hist);
    terminal_disable_raw(&term);
    close(term.tty_fd);
    close(net.socket_fd);

    return 0;
}
