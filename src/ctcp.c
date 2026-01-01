/*
 * ctcp.c
 * Client-to-client protocol (CTCP) event handling
 * Author: Michael Czigler
 * License: MIT
 */

#include "ctcp.h"

/**
 * ctcp_handle_clientinfo() - Handle CTCP CLIENTINFO request
 * @network: Network connection structure
 * @event: Event structure containing request details
 * @output: Output buffer structure (unused)
 *
 * Responds to CTCP CLIENTINFO queries with a list of supported CTCP
 * commands: PING, ACTION, CLIENTINFO, DCC, PING, TIME, VERSION.
 * Only responds to PRIVMSG, not NOTICE.
 */
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

/**
 * ctcp_handle_ping() - Handle CTCP PING request
 * @network: Network connection structure
 * @event: Event structure containing ping data
 * @output: Output buffer structure (unused)
 *
 * Responds to CTCP PING queries by echoing back any provided arguments.
 * Only responds to PRIVMSG, not NOTICE.
 */
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

/**
 * ctcp_handle_time() - Handle CTCP TIME request
 * @network: Network connection structure
 * @event: Event structure containing request details
 * @output: Output buffer structure (unused)
 *
 * Responds to CTCP TIME queries with the current local time in a formatted
 * string. Only responds to PRIVMSG, not NOTICE.
 */
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

/**
 * ctcp_handle_version() - Handle CTCP VERSION request
 * @network: Network connection structure
 * @event: Event structure containing request details
 * @output: Output buffer structure (unused)
 *
 * Responds to CTCP VERSION queries with the client name and version number.
 * Only responds to PRIVMSG, not NOTICE.
 */
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
