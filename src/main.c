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
#include "event.h"
#include "handler.h"
#include "helper.h"
#include "network.h"
#include "output.h"
#include "protocol.h"
#include "terminal.h"
#include "transport.h"

/**
 * kirc_register_handlers() - Register all IRC event handlers
 * @handler: Handler structure to configure
 *
 * Registers handler functions for all supported IRC events and CTCP commands.
 * Sets the default handler to protocol_raw for displaying unhandled events.
 * Maps event types to their corresponding protocol and CTCP handler functions.
 */
static void kirc_register_handlers(struct handler *handler) {
    handler_default(handler, protocol_raw);
    handler_register(handler, EVENT_CTCP_CLIENTINFO, ctcp_handle_clientinfo); 
    handler_register(handler, EVENT_CTCP_PING, ctcp_handle_ping);
    handler_register(handler, EVENT_CTCP_TIME, ctcp_handle_time);
    handler_register(handler, EVENT_CTCP_VERSION, ctcp_handle_version);
    handler_register(handler, EVENT_CTCP_ACTION, protocol_ctcp_action);
    handler_register(handler, EVENT_CTCP_DCC, protocol_ctcp_info);
    handler_register(handler, EVENT_ERROR, protocol_error);
    handler_register(handler, EVENT_EXT_AUTHENTICATE, protocol_authenticate);
    handler_register(handler, EVENT_JOIN, protocol_noop);
    handler_register(handler, EVENT_PART, protocol_part);
    handler_register(handler, EVENT_PING, protocol_ping);
    handler_register(handler, EVENT_QUIT, protocol_noop);
    handler_register(handler, EVENT_PRIVMSG, protocol_privmsg);
    handler_register(handler, EVENT_NICK, protocol_nick);
    handler_register(handler, EVENT_NOTICE, protocol_notice);
    handler_register(handler, EVENT_KICK, protocol_info);
    handler_register(handler, EVENT_EXT_CAP, protocol_info);
    handler_register(handler, EVENT_MODE, protocol_info);
    handler_register(handler, EVENT_TOPIC, protocol_info);
    handler_register(handler, EVENT_001_RPL_WELCOME, protocol_welcome);
    handler_register(handler, EVENT_002_RPL_YOURHOST, protocol_info);
    handler_register(handler, EVENT_003_RPL_CREATED, protocol_info);
    handler_register(handler, EVENT_004_RPL_MYINFO, protocol_info);
    handler_register(handler, EVENT_005_RPL_BOUNCE, protocol_info);
    handler_register(handler, EVENT_042_RPL_YOURID, protocol_info);
    handler_register(handler, EVENT_200_RPL_TRACELINK, protocol_info);
    handler_register(handler, EVENT_201_RPL_TRACECONNECTING, protocol_info);
    handler_register(handler, EVENT_202_RPL_TRACEHANDSHAKE, protocol_info);
    handler_register(handler, EVENT_203_RPL_TRACEUNKNOWN, protocol_info);
    handler_register(handler, EVENT_204_RPL_TRACEOPERATOR, protocol_info);
    handler_register(handler, EVENT_205_RPL_TRACEUSER, protocol_info);
    handler_register(handler, EVENT_206_RPL_TRACESERVER, protocol_info);
    handler_register(handler, EVENT_207_RPL_TRACESERVICE, protocol_info);
    handler_register(handler, EVENT_208_RPL_TRACENEWTYPE, protocol_info);
    handler_register(handler, EVENT_209_RPL_TRACECLASS, protocol_info);
    handler_register(handler, EVENT_211_RPL_STATSLINKINFO, protocol_info);
    handler_register(handler, EVENT_212_RPL_STATSCOMMANDS, protocol_info);
    handler_register(handler, EVENT_213_RPL_STATSCLINE, protocol_info);
    handler_register(handler, EVENT_215_RPL_STATSILINE, protocol_info);
    handler_register(handler, EVENT_216_RPL_STATSKLINE, protocol_info);
    handler_register(handler, EVENT_218_RPL_STATSYLINE, protocol_info);
    handler_register(handler, EVENT_219_RPL_ENDOFSTATS, protocol_info);
    handler_register(handler, EVENT_221_RPL_UMODEIS, protocol_info);
    handler_register(handler, EVENT_234_RPL_SERVLIST, protocol_info);
    handler_register(handler, EVENT_235_RPL_SERVLISTEND, protocol_info);
    handler_register(handler, EVENT_241_RPL_STATSLLINE, protocol_info);
    handler_register(handler, EVENT_242_RPL_STATSUPTIME, protocol_info);
    handler_register(handler, EVENT_243_RPL_STATSOLINE, protocol_info);
    handler_register(handler, EVENT_244_RPL_STATSHLINE, protocol_info);
    handler_register(handler, EVENT_245_RPL_STATSSLINE, protocol_info);
    handler_register(handler, EVENT_250_RPL_STATSCONN, protocol_info);
    handler_register(handler, EVENT_251_RPL_LUSERCLIENT, protocol_info);
    handler_register(handler, EVENT_252_RPL_LUSEROP, protocol_info);
    handler_register(handler, EVENT_253_RPL_LUSERUNKNOWN, protocol_info);
    handler_register(handler, EVENT_254_RPL_LUSERCHANNELS, protocol_info);
    handler_register(handler, EVENT_255_RPL_LUSERME, protocol_info);
    handler_register(handler, EVENT_256_RPL_ADMINME, protocol_info);
    handler_register(handler, EVENT_257_RPL_ADMINLOC1, protocol_info);
    handler_register(handler, EVENT_258_RPL_ADMINLOC2, protocol_info);
    handler_register(handler, EVENT_259_RPL_ADMINEMAIL, protocol_info);
    handler_register(handler, EVENT_261_RPL_TRACELOG, protocol_info);
    handler_register(handler, EVENT_263_RPL_TRYAGAIN, protocol_info);
    handler_register(handler, EVENT_265_RPL_LOCALUSERS, protocol_info);
    handler_register(handler, EVENT_266_RPL_GLOBALUSERS, protocol_info);
    handler_register(handler, EVENT_300_RPL_NONE, protocol_info);
    handler_register(handler, EVENT_301_RPL_AWAY, protocol_info);
    handler_register(handler, EVENT_302_RPL_USERHOST, protocol_info);
    handler_register(handler, EVENT_303_RPL_ISON, protocol_info);
    handler_register(handler, EVENT_305_RPL_UNAWAY, protocol_info);
    handler_register(handler, EVENT_306_RPL_NOWAWAY, protocol_info);
    handler_register(handler, EVENT_311_RPL_WHOISUSER, protocol_info);
    handler_register(handler, EVENT_312_RPL_WHOISSERVER, protocol_info);
    handler_register(handler, EVENT_313_RPL_WHOISOPERATOR, protocol_info);
    handler_register(handler, EVENT_314_RPL_WHOWASUSER, protocol_info);
    handler_register(handler, EVENT_315_RPL_ENDOFWHO, protocol_info);
    handler_register(handler, EVENT_317_RPL_WHOISIDLE, protocol_info);
    handler_register(handler, EVENT_318_RPL_ENDOFWHOIS, protocol_info);
    handler_register(handler, EVENT_319_RPL_WHOISCHANNELS, protocol_info);
    handler_register(handler, EVENT_322_RPL_LIST, protocol_info);
    handler_register(handler, EVENT_323_RPL_LISTEND, protocol_info);
    handler_register(handler, EVENT_324_RPL_CHANNELMODEIS, protocol_info);
    handler_register(handler, EVENT_328_RPL_CHANNEL_URL, protocol_info);
    handler_register(handler, EVENT_331_RPL_NOTOPIC, protocol_info);
    handler_register(handler, EVENT_332_RPL_TOPIC, protocol_info);
    handler_register(handler, EVENT_333_RPL_TOPICWHOTIME, protocol_info);
    handler_register(handler, EVENT_341_RPL_INVITING, protocol_info);
    handler_register(handler, EVENT_346_RPL_INVITELIST, protocol_info);
    handler_register(handler, EVENT_347_RPL_ENDOFINVITELIST, protocol_info);
    handler_register(handler, EVENT_348_RPL_EXCEPTLIST, protocol_info);
    handler_register(handler, EVENT_349_RPL_ENDOFEXCEPTLIST, protocol_info);
    handler_register(handler, EVENT_351_RPL_VERSION, protocol_info);
    handler_register(handler, EVENT_352_RPL_WHOREPLY, protocol_info);
    handler_register(handler, EVENT_353_RPL_NAMREPLY, protocol_info);
    handler_register(handler, EVENT_364_RPL_LINKS, protocol_info);
    handler_register(handler, EVENT_365_RPL_ENDOFLINKS, protocol_info);
    handler_register(handler, EVENT_366_RPL_ENDOFNAMES, protocol_info);
    handler_register(handler, EVENT_367_RPL_BANLIST, protocol_info);
    handler_register(handler, EVENT_368_RPL_ENDOFBANLIST, protocol_info);
    handler_register(handler, EVENT_369_RPL_ENDOFWHOWAS, protocol_info);
    handler_register(handler, EVENT_371_RPL_INFO, protocol_info);
    handler_register(handler, EVENT_372_RPL_MOTD, protocol_info);
    handler_register(handler, EVENT_374_RPL_ENDOFINFO, protocol_info);
    handler_register(handler, EVENT_375_RPL_MOTDSTART, protocol_info);
    handler_register(handler, EVENT_376_RPL_ENDOFMOTD, protocol_info);
    handler_register(handler, EVENT_381_RPL_YOUREOPER, protocol_info);
    handler_register(handler, EVENT_382_RPL_REHASHING, protocol_info);
    handler_register(handler, EVENT_383_RPL_YOURESERVICE, protocol_info);
    handler_register(handler, EVENT_391_RPL_TIME, protocol_info);
    handler_register(handler, EVENT_392_RPL_USERSSTART, protocol_info);
    handler_register(handler, EVENT_393_RPL_USERS, protocol_info);
    handler_register(handler, EVENT_394_RPL_ENDOFUSERS, protocol_info);
    handler_register(handler, EVENT_395_RPL_NOUSERS, protocol_info);
    handler_register(handler, EVENT_396_RPL_HOSTHIDDEN, protocol_info);
    handler_register(handler, EVENT_704_RPL_HELPSTART, protocol_info);
    handler_register(handler, EVENT_705_RPL_HELPTXT, protocol_info);
    handler_register(handler, EVENT_706_RPL_ENDOFHELP, protocol_info);
    handler_register(handler, EVENT_900_RPL_LOGGEDIN, protocol_info);
    handler_register(handler, EVENT_901_RPL_LOGGEDOUT, protocol_info);
    handler_register(handler, EVENT_903_RPL_SASLSUCCESS, protocol_info);
    handler_register(handler, EVENT_908_RPL_SASLMECHS, protocol_info);
    handler_register(handler, EVENT_400_ERR_UNKNOWNERROR, protocol_error);
    handler_register(handler, EVENT_401_ERR_NOSUCHNICK, protocol_error);
    handler_register(handler, EVENT_402_ERR_NOSUCHSERVER, protocol_error);
    handler_register(handler, EVENT_403_ERR_NOSUCHCHANNEL, protocol_error);
    handler_register(handler, EVENT_404_ERR_CANNOTSENDTOCHAN, protocol_error);
    handler_register(handler, EVENT_405_ERR_TOOMANYCHANNELS, protocol_error);
    handler_register(handler, EVENT_406_ERR_WASNOSUCHNICK, protocol_error);
    handler_register(handler, EVENT_407_ERR_TOOMANYTARGETS, protocol_error);
    handler_register(handler, EVENT_408_ERR_NOSUCHSERVICE, protocol_error);
    handler_register(handler, EVENT_409_ERR_NOORIGIN, protocol_error);
    handler_register(handler, EVENT_411_ERR_NORECIPIENT, protocol_error);
    handler_register(handler, EVENT_412_ERR_NOTEXTTOSEND, protocol_error);
    handler_register(handler, EVENT_413_ERR_NOTOPLEVEL, protocol_error);
    handler_register(handler, EVENT_414_ERR_WILDTOPLEVEL, protocol_error);
    handler_register(handler, EVENT_415_ERR_BADMASK, protocol_error);
    handler_register(handler, EVENT_421_ERR_UNKNOWNCOMMAND, protocol_error);
    handler_register(handler, EVENT_422_ERR_NOMOTD, protocol_error);
    handler_register(handler, EVENT_423_ERR_NOADMININFO, protocol_error);
    handler_register(handler, EVENT_424_ERR_FILEERROR, protocol_error);
    handler_register(handler, EVENT_431_ERR_NONICKNAMEGIVEN, protocol_error);
    handler_register(handler, EVENT_432_ERR_ERRONEUSNICKNAME, protocol_error);
    handler_register(handler, EVENT_433_ERR_NICKNAMEINUSE, protocol_error);
    handler_register(handler, EVENT_436_ERR_NICKCOLLISION, protocol_error);
    handler_register(handler, EVENT_441_ERR_USERNOTINCHANNEL, protocol_error);
    handler_register(handler, EVENT_442_ERR_NOTONCHANNEL, protocol_error);
    handler_register(handler, EVENT_443_ERR_USERONCHANNEL, protocol_error);
    handler_register(handler, EVENT_444_ERR_NOLOGIN, protocol_error);
    handler_register(handler, EVENT_445_ERR_SUMMONDISABLED, protocol_error);
    handler_register(handler, EVENT_446_ERR_USERSDISABLED, protocol_error);
    handler_register(handler, EVENT_451_ERR_NOTREGISTERED, protocol_error);
    handler_register(handler, EVENT_461_ERR_NEEDMOREPARAMS, protocol_error);
    handler_register(handler, EVENT_462_ERR_ALREADYREGISTERED, protocol_error);
    handler_register(handler, EVENT_463_ERR_NOPERMFORHOST, protocol_error);
    handler_register(handler, EVENT_464_ERR_PASSWDMISMATCH, protocol_error);
    handler_register(handler, EVENT_465_ERR_YOUREBANNEDCREEP, protocol_error);
    handler_register(handler, EVENT_467_ERR_KEYSET, protocol_error);
    handler_register(handler, EVENT_470_ERR_LINKCHANNEL, protocol_error);
    handler_register(handler, EVENT_471_ERR_CHANNELISFULL, protocol_error);
    handler_register(handler, EVENT_472_ERR_UNKNOWNMODE, protocol_error);
    handler_register(handler, EVENT_473_ERR_INVITEONLYCHAN, protocol_error);
    handler_register(handler, EVENT_474_ERR_BANNEDFROMCHAN, protocol_error);
    handler_register(handler, EVENT_475_ERR_BADCHANNELKEY, protocol_error);
    handler_register(handler, EVENT_476_ERR_BADCHANMASK, protocol_error);
    handler_register(handler, EVENT_477_ERR_NEEDREGGEDNICK, protocol_error);
    handler_register(handler, EVENT_478_ERR_BANLISTFULL, protocol_error);
    handler_register(handler, EVENT_481_ERR_NOPRIVILEGES, protocol_error);
    handler_register(handler, EVENT_482_ERR_CHANOPRIVSNEEDED, protocol_error);
    handler_register(handler, EVENT_483_ERR_CANTKILLSERVER, protocol_error);
    handler_register(handler, EVENT_485_ERR_UNIQOPRIVSNEEDED, protocol_error);
    handler_register(handler, EVENT_491_ERR_NOOPERHOST, protocol_error);
    handler_register(handler, EVENT_501_ERR_UMODEUNKNOWNFLAG, protocol_error);
    handler_register(handler, EVENT_502_ERR_USERSDONTMATCH, protocol_error);
    handler_register(handler, EVENT_524_ERR_HELPNOTFOUND, protocol_error);
    handler_register(handler, EVENT_902_ERR_NICKLOCKED, protocol_error);
    handler_register(handler, EVENT_904_ERR_SASLFAIL, protocol_error);
    handler_register(handler, EVENT_905_ERR_SASLTOOLONG, protocol_error);
    handler_register(handler, EVENT_906_ERR_SASLABORTED, protocol_error);
    handler_register(handler, EVENT_907_ERR_SASLALREADY, protocol_error);
}

/**
 * kirc_run() - Main IRC client event loop
 * @ctx: IRC context structure with connection settings
 *
 * Initializes all subsystems (editor, transport, network, DCC, handlers,
 * terminal, output), establishes the IRC connection, and runs the main
 * event loop. Polls stdin and network socket for events, processing user
 * input and IRC messages. Handles terminal raw mode and cleanup on exit.
 *
 * Return: 0 on clean exit, -1 on initialization or runtime error
 */
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

    struct handler handler;

    if (handler_init(&handler, ctx) < 0) {
        fprintf(stderr, "handler_init failed\n");
        dcc_free(&dcc);
        network_free(&network);
        return -1;
    }

    kirc_register_handlers(&handler);

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

    struct output output;

    if (output_init(&output, ctx) < 0) {
        fprintf(stderr, "output_init failed\n");
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

            if (editor.state == EDITOR_STATE_TERMINATE)
                break;

            if (editor.state == EDITOR_STATE_SEND) {
                char *msg = editor_last_entry(&editor);
                network_command_handler(&network, msg, &output);
                output_flush(&output);
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

                    struct event event;
                    event_init(&event, ctx);
                    event_parse(&event, msg);

                    handler_dispatch(&handler, &network, &event, &output);
                    dcc_handle(&dcc, &network, &event);

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

                output_flush(&output);
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

/**
 * main() - Program entry point
 * @argc: Argument count
 * @argv: Argument vector
 *
 * Initializes configuration from environment and command-line arguments,
 * runs the IRC client, and cleans up on exit. Ensures secure cleanup of
 * sensitive data.
 *
 * Return: EXIT_SUCCESS on normal termination, EXIT_FAILURE on error
 */
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
