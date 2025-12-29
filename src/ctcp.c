/*
 * ctcp.c
 * Client-to-client protocol (CTCP) event handling
 * Author: Michael Czigler
 * License: MIT
 */

#include "ctcp.h"

static void ctcp_handle_clientinfo(network_t *network, protocol_t *protocol)
{
    if (strcmp(protocol->command, "PRIVMSG") == 0) {
        network_send(network,
            "NOTICE %s :\001PING ACTION CLIENTINFO DCC PING TIME VERSION\001\r\n",
            protocol->nickname);
    }
}

static void ctcp_handle_ping(network_t *network, protocol_t *protocol)
{
    if (strcmp(protocol->command, "PRIVMSG") == 0) {
        if (protocol->message[0] != '\0') {
            network_send(network,
                "NOTICE %s :\001PING %s\001\r\n",
                protocol->nickname, protocol->message);
        } else {
            network_send(network,
                "NOTICE %s :\001PING\001\r\n",
                protocol->nickname);
        }
    }
}

static void ctcp_handle_time(network_t *network, protocol_t *protocol)
{
    if (strcmp(protocol->command, "PRIVMSG") == 0) {
        char tbuf[128];
        time_t now;
        time(&now);
        struct tm *info = localtime(&now);
        strftime(tbuf, sizeof(tbuf), "%c", info);
        network_send(network,
            "NOTICE %s :\001TIME %s\001\r\n",
            protocol->nickname, tbuf);
    }
}

static void ctcp_handle_version(network_t *network, protocol_t *protocol)
{
    if (strcmp(protocol->command, "PRIVMSG") == 0) {
        network_send(network,
            "NOTICE %s :\001VERSION kirc %s\001\r\n",
            protocol->nickname, 
            KIRC_VERSION_MAJOR "." KIRC_VERSION_MINOR "." KIRC_VERSION_PATCH);
    }
}

static const ctcp_dispatch_entry_t ctcp_dispatch_table[] = {
    { PROTOCOL_EVENT_CTCP_CLIENTINFO, ctcp_handle_clientinfo },
    { PROTOCOL_EVENT_CTCP_PING, ctcp_handle_ping },
    { PROTOCOL_EVENT_CTCP_TIME, ctcp_handle_time },
    { PROTOCOL_EVENT_CTCP_VERSION, ctcp_handle_version },
    { PROTOCOL_EVENT_NONE, NULL }  /* sentinel */
};

void ctcp_handle_event(network_t *network, protocol_t *protocol)
{
    for (const ctcp_dispatch_entry_t *entry = ctcp_dispatch_table; 
         entry->handler != NULL; 
         ++entry) {
        if (entry->event == protocol->event) {
            entry->handler(network, protocol);
            return;
        }
    }
}
