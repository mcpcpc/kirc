/*
 * ctcp.c
 * Client-to-client protocol (CTCP) event handling
 * Author: Michael Czigler
 * License: MIT
 */

#include "ctcp.h"

void ctcp_handle_clientinfo(struct network *network, struct event *event, struct output *output)
{
    (void)output;
    const char *command = event->command;
    const char *nickname = event->nickname;
    
    if (strcmp(command, "PRIVMSG") == 0) {
        network_send(network,
            "NOTICE %s :\001PING ACTION CLIENTINFO DCC PING TIME VERSION\001\r\n",
            nickname);
    }
}

void ctcp_handle_ping(struct network *network, struct event *event, struct output *output)
{
    (void)output;
    const char *command = event->command;
    const char *nickname = event->nickname;
    const char *message = event->message;
    
    if (strcmp(command, "PRIVMSG") == 0) {
        if (message[0] != '\0') {
            network_send(network,
                "NOTICE %s :\001PING %s\001\r\n",
                nickname, message);
        } else {
            network_send(network,
                "NOTICE %s :\001PING\001\r\n",
                nickname);
        }
    }
}

void ctcp_handle_time(struct network *network, struct event *event, struct output *output)
{
    (void)output;
    const char *command = event->command;
    const char *nickname = event->nickname;
    
    if (strcmp(command, "PRIVMSG") == 0) {
        char tbuf[128];
        time_t now;
        time(&now);
        struct tm *info = localtime(&now);
        strftime(tbuf, sizeof(tbuf), "%c", info);
        network_send(network,
            "NOTICE %s :\001TIME %s\001\r\n",
            nickname, tbuf);
    }
}

void ctcp_handle_version(struct network *network, struct event *event, struct output *output)
{
    (void)output;
    const char *command = event->command;
    const char *nickname = event->nickname;
    
    if (strcmp(command, "PRIVMSG") == 0) {
        network_send(network,
            "NOTICE %s :\001VERSION kirc %s\001\r\n", nickname, 
            KIRC_VERSION_MAJOR "." KIRC_VERSION_MINOR "." KIRC_VERSION_PATCH);
    }
}
