/*
 * ctcp.c
 * Client-to-client protocol (CTCP) event handling
 * Author: Michael Czigler
 * License: MIT
 */

#include "ctcp.h"

void ctcp_handle_clientinfo(struct network *network, struct event *event)
{
    if (strcmp(event->command, "PRIVMSG") == 0) {
        network_send(network,
            "NOTICE %s :\001PING ACTION CLIENTINFO DCC PING TIME VERSION\001\r\n",
            event->nickname);
    }
}

void ctcp_handle_ping(struct network *network, struct event *event)
{
    if (strcmp(event->command, "PRIVMSG") == 0) {
        if (event->message[0] != '\0') {
            network_send(network,
                "NOTICE %s :\001PING %s\001\r\n",
                event->nickname, event->message);
        } else {
            network_send(network,
                "NOTICE %s :\001PING\001\r\n",
                event->nickname);
        }
    }
}

void ctcp_handle_time(struct network *network, struct event *event)
{
    if (strcmp(event->command, "PRIVMSG") == 0) {
        char tbuf[128];
        time_t now;
        time(&now);
        struct tm *info = localtime(&now);
        strftime(tbuf, sizeof(tbuf), "%c", info);
        network_send(network,
            "NOTICE %s :\001TIME %s\001\r\n",
            event->nickname, tbuf);
    }
}

void ctcp_handle_version(struct network *network, struct event *event)
{
    if (strcmp(event->command, "PRIVMSG") == 0) {
        network_send(network,
            "NOTICE %s :\001VERSION kirc %s\001\r\n", event->nickname, 
            KIRC_VERSION_MAJOR "." KIRC_VERSION_MINOR "." KIRC_VERSION_PATCH);
    }
}
