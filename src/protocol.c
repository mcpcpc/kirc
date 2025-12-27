/*
 * protocol.c
 * IRC protocol handling
 * Author: Michael Czigler
 * License: MIT
 */

#include "protocol.h"

static void protocol_get_time(char *out)
{
    time_t current;
    time(&current);
    struct tm *info = localtime(&current);
    strftime(out, KIRC_TIMESTAMP_SIZE,
        KIRC_TIMESTAMP_FORMAT, info);
}

static void protocol_noop(protocol_t *protocol)
{
    (void)protocol;  /* no operation */
}

static void protocol_raw(protocol_t *protocol)
{
    char hhmm[KIRC_TIMESTAMP_SIZE];
    protocol_get_time(hhmm);

    printf("\r" CLEAR_LINE DIM "%s" RESET
        " " REVERSE "%s" RESET "\r\n",
        hhmm, protocol->raw);
}

static void protocol_info(protocol_t *protocol)
{
    char hhmm[KIRC_TIMESTAMP_SIZE];
    protocol_get_time(hhmm);

    printf("\r" CLEAR_LINE DIM "%s %s" RESET "\r\n",
        hhmm, protocol->message);
}

static void protocol_error(protocol_t *protocol)
{
    char hhmm[KIRC_TIMESTAMP_SIZE];
    protocol_get_time(hhmm);

    printf("\r" CLEAR_LINE DIM "%s" RESET
        " " BOLD_RED "%s" RESET "\r\n",
        hhmm, protocol->message);
}

static void protocol_notice(protocol_t *protocol)
{
    char hhmm[KIRC_TIMESTAMP_SIZE];
    protocol_get_time(hhmm);

    printf("\r" CLEAR_LINE DIM "%s" RESET
        " " BOLD_BLUE "%s" RESET " %s\r\n",
        hhmm, protocol->nickname, protocol->message);
}

static void protocol_privmsg_direct(protocol_t *protocol)
{
    char hhmm[KIRC_TIMESTAMP_SIZE];
    protocol_get_time(hhmm);

    printf("\r" CLEAR_LINE DIM "%s" RESET
        " " BOLD_BLUE "%s" RESET " " BLUE "%s" RESET "\r\n",
        hhmm, protocol->nickname, protocol->message);
}

static void protocol_privmsg_indirect(protocol_t *protocol)
{
    char hhmm[KIRC_TIMESTAMP_SIZE];
    protocol_get_time(hhmm);

    printf("\r" CLEAR_LINE DIM "%s" RESET
        " " BOLD "%s" RESET " [%s]: %s\r\n",
        hhmm, protocol->nickname, protocol->channel,
        protocol->message);

}

static void protocol_privmsg(protocol_t *protocol)
{
    char *channel = protocol->channel;
    char *nickname = protocol->ctx->nickname;

    if (strcmp(channel, nickname) == 0) {
        protocol_privmsg_direct(protocol);
    } else {
        protocol_privmsg_indirect(protocol);
    }
}

static void protocol_nick(protocol_t *protocol)
{
    char hhmm[KIRC_TIMESTAMP_SIZE];
    protocol_get_time(hhmm);

    
    if (strcmp(protocol->nickname, protocol->ctx->nickname) == 0) {
        size_t siz = sizeof(protocol->ctx->nickname) - 1;
        strncpy(protocol->ctx->nickname, protocol->message, siz);
        protocol->ctx->nickname[siz] = '\0';
        printf("\r" CLEAR_LINE
            DIM "%s you are now known as %s" RESET "\r\n",
            hhmm, protocol->message);
    } else {
        printf("\r" CLEAR_LINE
            DIM "%s %s is now known as %s" RESET "\r\n",
            hhmm, protocol->nickname, protocol->message);
    }

}

static void protocol_ctcp_action(protocol_t *protocol)
{
    char hhmm[KIRC_TIMESTAMP_SIZE];
    protocol_get_time(hhmm);

    printf("\r" CLEAR_LINE DIM "%s * %s %s" RESET "\r\n",
        hhmm, protocol->nickname, protocol->message);
}

static void protocol_ctcp_info(protocol_t *protocol)
{
    char hhmm[KIRC_TIMESTAMP_SIZE];
    protocol_get_time(hhmm);

    const char *label = "";

    switch(protocol->event) {
    case PROTOCOL_EVENT_CTCP_CLIENTINFO:
        label = "CLIENTINFO";
        break;

    case PROTOCOL_EVENT_CTCP_DCC:
        label = "DCC";
        break;

    case PROTOCOL_EVENT_CTCP_PING:
        label = "PING";
        break;

    case PROTOCOL_EVENT_CTCP_TIME:
        label = "TIME";
        break;

    case PROTOCOL_EVENT_CTCP_VERSION: 
        label = "VERSION";
        break;

    default:
        label = "CTCP";
        break;
    }

    if (protocol->params[0] != '\0') {
        printf("\r" CLEAR_LINE DIM "%s " RESET
            BOLD_BLUE "%s" RESET " %s: %s\r\n",
            hhmm, protocol->nickname, label,
            protocol->params);
    } else if (protocol->message[0] != '\0') {
        printf("\r" CLEAR_LINE DIM "%s " RESET
            BOLD_BLUE "%s" RESET " %s: %s\r\n",
            hhmm, protocol->nickname, label,
            protocol->message);
    } else {
        printf("\r" CLEAR_LINE DIM "%s " RESET
            BOLD_BLUE "%s" RESET " %s\r\n",
            hhmm, protocol->nickname, label);
    }
}

static int protocol_ctcp_parse(protocol_t *protocol)
{
    if (((protocol->event == PROTOCOL_EVENT_PRIVMSG) ||
        (protocol->event == PROTOCOL_EVENT_NOTICE)) &&
        (protocol->message[0] == '\001')) {
        char ctcp[MESSAGE_MAX_LEN];
        size_t siz = sizeof(protocol->message);
        size_t len = strnlen(protocol->message, siz);
        size_t end = 1;

        while ((end < len) && protocol->message[end] != '\001') {
            end++;
        }

        size_t ctcp_len = (end > 1) ? end - 1 : 0;

        if (ctcp_len > 0) {
            size_t copy_n;

            if (ctcp_len < sizeof(ctcp)) {
                copy_n = ctcp_len + 1;
            } else {
                copy_n = sizeof(ctcp);
            }

            safecpy(ctcp, protocol->message + 1, copy_n); 

            char *command = strtok(ctcp, " ");
            char *args = strtok(NULL, "");

            if (command != NULL) {
                if (strcmp(command, "ACTION") == 0) {
                    protocol->event = PROTOCOL_EVENT_CTCP_ACTION;
                    if (args != NULL) {
                        siz = sizeof(protocol->message);
                        safecpy(protocol->message, args, siz);
                    } else {
                        protocol->message[0] = '\0';
                    }
                } else if (strcmp(command, "VERSION") == 0) {
                    protocol->event = PROTOCOL_EVENT_CTCP_VERSION;
                    if (args != NULL) {
                        siz = sizeof(protocol->params);
                        safecpy(protocol->params, args, siz);
                    } else {
                        protocol->params[0] = '\0';
                    }
                } else if (strcmp(command, "PING") == 0) {
                    protocol->event = PROTOCOL_EVENT_CTCP_PING;
                    if (args != NULL) {
                        siz = sizeof(protocol->message);
                        safecpy(protocol->message, args, siz);
                    } else {
                        protocol->message[0] = '\0';
                    }
                } else if (strcmp(command, "TIME") == 0) {
                    protocol->event = PROTOCOL_EVENT_CTCP_TIME;
                } else if (strcmp(command, "CLIENTINFO") == 0) {
                    protocol->event = PROTOCOL_EVENT_CTCP_CLIENTINFO;
                } else if (strcmp(command, "DCC") == 0) {
                    protocol->event = PROTOCOL_EVENT_CTCP_DCC;
                    if (args != NULL) {
                        siz = sizeof(protocol->message);
                        safecpy(protocol->message, args, siz);
                    } else {
                        protocol->message[0] = '\0';
                    }
                } else {
                    protocol->message[0] = '\0';
                }
            }
        }
    }

    return 0;
}

static const protocol_dispatch_t dispatch_table[] = {
    { PROTOCOL_EVENT_CTCP_ACTION,               protocol_ctcp_action },
    { PROTOCOL_EVENT_CTCP_CLIENTINFO,           protocol_ctcp_info },
    { PROTOCOL_EVENT_CTCP_DCC,                  protocol_ctcp_info },
    { PROTOCOL_EVENT_CTCP_PING,                 protocol_ctcp_info },
    { PROTOCOL_EVENT_CTCP_TIME,                 protocol_ctcp_info },
    { PROTOCOL_EVENT_CTCP_VERSION,              protocol_ctcp_info },
    { PROTOCOL_EVENT_EXT_AUTHENTICATE,          protocol_noop },
    { PROTOCOL_EVENT_JOIN,                      protocol_noop },
    { PROTOCOL_EVENT_PART,                      protocol_noop },
    { PROTOCOL_EVENT_PING,                      protocol_noop },
    { PROTOCOL_EVENT_QUIT,                      protocol_noop },
    { PROTOCOL_EVENT_PRIVMSG,                   protocol_privmsg },
    { PROTOCOL_EVENT_NICK,                      protocol_nick },
    { PROTOCOL_EVENT_NOTICE,                    protocol_notice },
    { PROTOCOL_EVENT_KICK,                      protocol_info },
    { PROTOCOL_EVENT_EXT_CAP,                   protocol_info },
    { PROTOCOL_EVENT_MODE,                      protocol_info },
    { PROTOCOL_EVENT_TOPIC,                     protocol_info },
    { PROTOCOL_EVENT_001_RPL_WELCOME,           protocol_info },
    { PROTOCOL_EVENT_002_RPL_YOURHOST,          protocol_info },
    { PROTOCOL_EVENT_003_RPL_CREATED,           protocol_info },
    { PROTOCOL_EVENT_004_RPL_MYINFO,            protocol_info },
    { PROTOCOL_EVENT_005_RPL_BOUNCE,            protocol_info },
    { PROTOCOL_EVENT_042_RPL_YOURID,            protocol_info },
    { PROTOCOL_EVENT_200_RPL_TRACELINK,         protocol_info },
    { PROTOCOL_EVENT_201_RPL_TRACECONNECTING,   protocol_info },
    { PROTOCOL_EVENT_202_RPL_TRACEHANDSHAKE,    protocol_info },
    { PROTOCOL_EVENT_203_RPL_TRACEUNKNOWN,      protocol_info },
    { PROTOCOL_EVENT_204_RPL_TRACEOPERATOR,     protocol_info },
    { PROTOCOL_EVENT_205_RPL_TRACEUSER,         protocol_info },
    { PROTOCOL_EVENT_206_RPL_TRACESERVER,       protocol_info },
    { PROTOCOL_EVENT_207_RPL_TRACESERVICE,      protocol_info },
    { PROTOCOL_EVENT_208_RPL_TRACENEWTYPE,      protocol_info },
    { PROTOCOL_EVENT_209_RPL_TRACECLASS,        protocol_info },
    { PROTOCOL_EVENT_211_RPL_STATSLINKINFO,     protocol_info },
    { PROTOCOL_EVENT_212_RPL_STATSCOMMANDS,     protocol_info },
    { PROTOCOL_EVENT_213_RPL_STATSCLINE,        protocol_info },
    { PROTOCOL_EVENT_215_RPL_STATSILINE,        protocol_info },
    { PROTOCOL_EVENT_216_RPL_STATSKLINE,        protocol_info },
    { PROTOCOL_EVENT_218_RPL_STATSYLINE,        protocol_info },
    { PROTOCOL_EVENT_219_RPL_ENDOFSTATS,        protocol_info },
    { PROTOCOL_EVENT_221_RPL_UMODEIS,           protocol_info },
    { PROTOCOL_EVENT_234_RPL_SERVLIST,          protocol_info },
    { PROTOCOL_EVENT_235_RPL_SERVLISTEND,       protocol_info },
    { PROTOCOL_EVENT_241_RPL_STATSLLINE,        protocol_info },
    { PROTOCOL_EVENT_242_RPL_STATSUPTIME,       protocol_info },
    { PROTOCOL_EVENT_243_RPL_STATSOLINE,        protocol_info },
    { PROTOCOL_EVENT_244_RPL_STATSHLINE,        protocol_info },
    { PROTOCOL_EVENT_245_RPL_STATSSLINE,        protocol_info },
    { PROTOCOL_EVENT_250_RPL_STATSCONN,         protocol_info },
    { PROTOCOL_EVENT_251_RPL_LUSERCLIENT,       protocol_info },
    { PROTOCOL_EVENT_252_RPL_LUSEROP,           protocol_info },
    { PROTOCOL_EVENT_253_RPL_LUSERUNKNOWN,      protocol_info },
    { PROTOCOL_EVENT_254_RPL_LUSERCHANNELS,     protocol_info },
    { PROTOCOL_EVENT_255_RPL_LUSERME,           protocol_info },
    { PROTOCOL_EVENT_256_RPL_ADMINME,           protocol_info },
    { PROTOCOL_EVENT_257_RPL_ADMINLOC1,         protocol_info },
    { PROTOCOL_EVENT_258_RPL_ADMINLOC2,         protocol_info },
    { PROTOCOL_EVENT_259_RPL_ADMINEMAIL,        protocol_info },
    { PROTOCOL_EVENT_261_RPL_TRACELOG,          protocol_info },
    { PROTOCOL_EVENT_263_RPL_TRYAGAIN,          protocol_info },
    { PROTOCOL_EVENT_265_RPL_LOCALUSERS,        protocol_info },
    { PROTOCOL_EVENT_266_RPL_GLOBALUSERS,       protocol_info },
    { PROTOCOL_EVENT_300_RPL_NONE,              protocol_info },
    { PROTOCOL_EVENT_301_RPL_AWAY,              protocol_info },
    { PROTOCOL_EVENT_302_RPL_USERHOST,          protocol_info },
    { PROTOCOL_EVENT_303_RPL_ISON,              protocol_info },
    { PROTOCOL_EVENT_305_RPL_UNAWAY,            protocol_info },
    { PROTOCOL_EVENT_306_RPL_NOWAWAY,           protocol_info },
    { PROTOCOL_EVENT_311_RPL_WHOISUSER,         protocol_info },
    { PROTOCOL_EVENT_312_RPL_WHOISSERVER,       protocol_info },
    { PROTOCOL_EVENT_313_RPL_WHOISOPERATOR,     protocol_info },
    { PROTOCOL_EVENT_314_RPL_WHOWASUSER,        protocol_info },
    { PROTOCOL_EVENT_315_RPL_ENDOFWHO,          protocol_info },
    { PROTOCOL_EVENT_317_RPL_WHOISIDLE,         protocol_info },
    { PROTOCOL_EVENT_318_RPL_ENDOFWHOIS,        protocol_info },
    { PROTOCOL_EVENT_319_RPL_WHOISCHANNELS,     protocol_info },
    { PROTOCOL_EVENT_322_RPL_LIST,              protocol_info },
    { PROTOCOL_EVENT_323_RPL_LISTEND,           protocol_info },
    { PROTOCOL_EVENT_324_RPL_CHANNELMODEIS,     protocol_info },
    { PROTOCOL_EVENT_328_RPL_CHANNEL_URL,       protocol_info },
    { PROTOCOL_EVENT_331_RPL_NOTOPIC,           protocol_info },
    { PROTOCOL_EVENT_332_RPL_TOPIC,             protocol_info },
    { PROTOCOL_EVENT_333_RPL_TOPICWHOTIME,      protocol_info },
    { PROTOCOL_EVENT_341_RPL_INVITING,          protocol_info },
    { PROTOCOL_EVENT_346_RPL_INVITELIST,        protocol_info },
    { PROTOCOL_EVENT_347_RPL_ENDOFINVITELIST,   protocol_info },
    { PROTOCOL_EVENT_348_RPL_EXCEPTLIST,        protocol_info },
    { PROTOCOL_EVENT_349_RPL_ENDOFEXCEPTLIST,   protocol_info }, 
    { PROTOCOL_EVENT_351_RPL_VERSION,           protocol_info },
    { PROTOCOL_EVENT_352_RPL_WHOREPLY,          protocol_info },
    { PROTOCOL_EVENT_353_RPL_NAMREPLY,          protocol_info },
    { PROTOCOL_EVENT_364_RPL_LINKS,             protocol_info },
    { PROTOCOL_EVENT_365_RPL_ENDOFLINKS,        protocol_info },
    { PROTOCOL_EVENT_366_RPL_ENDOFNAMES,        protocol_info },
    { PROTOCOL_EVENT_367_RPL_BANLIST,           protocol_info },
    { PROTOCOL_EVENT_368_RPL_ENDOFBANLIST,      protocol_info },
    { PROTOCOL_EVENT_369_RPL_ENDOFWHOWAS,       protocol_info },
    { PROTOCOL_EVENT_371_RPL_INFO,              protocol_info },
    { PROTOCOL_EVENT_372_RPL_MOTD,              protocol_info },
    { PROTOCOL_EVENT_374_RPL_ENDOFINFO,         protocol_info },
    { PROTOCOL_EVENT_375_RPL_MOTDSTART,         protocol_info },
    { PROTOCOL_EVENT_376_RPL_ENDOFMOTD,         protocol_info },
    { PROTOCOL_EVENT_381_RPL_YOUREOPER,         protocol_info },
    { PROTOCOL_EVENT_382_RPL_REHASHING,         protocol_info },
    { PROTOCOL_EVENT_383_RPL_YOURESERVICE,      protocol_info },
    { PROTOCOL_EVENT_391_RPL_TIME,              protocol_info },
    { PROTOCOL_EVENT_392_RPL_USERSSTART,        protocol_info },
    { PROTOCOL_EVENT_393_RPL_USERS,             protocol_info },
    { PROTOCOL_EVENT_394_RPL_ENDOFUSERS,        protocol_info },
    { PROTOCOL_EVENT_395_RPL_NOUSERS,           protocol_info },
    { PROTOCOL_EVENT_396_RPL_HOSTHIDDEN,        protocol_info },
    { PROTOCOL_EVENT_704_RPL_HELPSTART,         protocol_info },
    { PROTOCOL_EVENT_705_RPL_HELPTXT,           protocol_info },
    { PROTOCOL_EVENT_706_RPL_ENDOFHELP,         protocol_info },
    { PROTOCOL_EVENT_900_RPL_LOGGEDIN,          protocol_info },
    { PROTOCOL_EVENT_901_RPL_LOGGEDOUT,         protocol_info },
    { PROTOCOL_EVENT_903_RPL_SASLSUCCESS,       protocol_info },
    { PROTOCOL_EVENT_908_RPL_SASLMECHS,         protocol_info },
    { PROTOCOL_EVENT_ERROR,                     protocol_error },
    { PROTOCOL_EVENT_400_ERR_UNKNOWNERROR,      protocol_error },
    { PROTOCOL_EVENT_401_ERR_NOSUCHNICK,        protocol_error },
    { PROTOCOL_EVENT_402_ERR_NOSUCHSERVER,      protocol_error },
    { PROTOCOL_EVENT_403_ERR_NOSUCHCHANNEL,     protocol_error },
    { PROTOCOL_EVENT_404_ERR_CANNOTSENDTOCHAN,  protocol_error },
    { PROTOCOL_EVENT_405_ERR_TOOMANYCHANNELS,   protocol_error },
    { PROTOCOL_EVENT_406_ERR_WASNOSUCHNICK,     protocol_error },
    { PROTOCOL_EVENT_407_ERR_TOOMANYTARGETS,    protocol_error },
    { PROTOCOL_EVENT_408_ERR_NOSUCHSERVICE,     protocol_error },
    { PROTOCOL_EVENT_409_ERR_NOORIGIN,          protocol_error },
    { PROTOCOL_EVENT_411_ERR_NORECIPIENT,       protocol_error },
    { PROTOCOL_EVENT_412_ERR_NOTEXTTOSEND,      protocol_error },
    { PROTOCOL_EVENT_413_ERR_NOTOPLEVEL,        protocol_error },
    { PROTOCOL_EVENT_414_ERR_WILDTOPLEVEL,      protocol_error },
    { PROTOCOL_EVENT_415_ERR_BADMASK,           protocol_error },
    { PROTOCOL_EVENT_421_ERR_UNKNOWNCOMMAND,    protocol_error },
    { PROTOCOL_EVENT_422_ERR_NOMOTD,            protocol_error },
    { PROTOCOL_EVENT_423_ERR_NOADMININFO,       protocol_error },
    { PROTOCOL_EVENT_424_ERR_FILEERROR,         protocol_error },
    { PROTOCOL_EVENT_431_ERR_NONICKNAMEGIVEN,   protocol_error },
    { PROTOCOL_EVENT_432_ERR_ERRONEUSNICKNAME,  protocol_error },
    { PROTOCOL_EVENT_433_ERR_NICKNAMEINUSE,     protocol_error },
    { PROTOCOL_EVENT_436_ERR_NICKCOLLISION,     protocol_error },
    { PROTOCOL_EVENT_441_ERR_USERNOTINCHANNEL,  protocol_error },
    { PROTOCOL_EVENT_442_ERR_NOTONCHANNEL,      protocol_error },
    { PROTOCOL_EVENT_443_ERR_USERONCHANNEL,     protocol_error },
    { PROTOCOL_EVENT_444_ERR_NOLOGIN,           protocol_error },
    { PROTOCOL_EVENT_445_ERR_SUMMONDISABLED,    protocol_error },
    { PROTOCOL_EVENT_446_ERR_USERSDISABLED,     protocol_error },
    { PROTOCOL_EVENT_451_ERR_NOTREGISTERED,     protocol_error },
    { PROTOCOL_EVENT_461_ERR_NEEDMOREPARAMS,    protocol_error },
    { PROTOCOL_EVENT_462_ERR_ALREADYREGISTERED, protocol_error },
    { PROTOCOL_EVENT_463_ERR_NOPERMFORHOST,     protocol_error },
    { PROTOCOL_EVENT_464_ERR_PASSWDMISMATCH,    protocol_error },
    { PROTOCOL_EVENT_465_ERR_YOUREBANNEDCREEP,  protocol_error },
    { PROTOCOL_EVENT_467_ERR_KEYSET,            protocol_error },
    { PROTOCOL_EVENT_470_ERR_LINKCHANNEL,       protocol_error },
    { PROTOCOL_EVENT_471_ERR_CHANNELISFULL,     protocol_error },
    { PROTOCOL_EVENT_472_ERR_UNKNOWNMODE,       protocol_error },
    { PROTOCOL_EVENT_473_ERR_INVITEONLYCHAN,    protocol_error },
    { PROTOCOL_EVENT_474_ERR_BANNEDFROMCHAN,    protocol_error },
    { PROTOCOL_EVENT_475_ERR_BADCHANNELKEY,     protocol_error },
    { PROTOCOL_EVENT_476_ERR_BADCHANMASK,       protocol_error },
    { PROTOCOL_EVENT_477_ERR_NEEDREGGEDNICK,    protocol_error },
    { PROTOCOL_EVENT_478_ERR_BANLISTFULL,       protocol_error },
    { PROTOCOL_EVENT_481_ERR_NOPRIVILEGES,      protocol_error },
    { PROTOCOL_EVENT_482_ERR_CHANOPRIVSNEEDED,  protocol_error },
    { PROTOCOL_EVENT_483_ERR_CANTKILLSERVER,    protocol_error },
    { PROTOCOL_EVENT_485_ERR_UNIQOPRIVSNEEDED,  protocol_error },
    { PROTOCOL_EVENT_491_ERR_NOOPERHOST,        protocol_error },
    { PROTOCOL_EVENT_501_ERR_UMODEUNKNOWNFLAG,  protocol_error },
    { PROTOCOL_EVENT_502_ERR_USERSDONTMATCH,    protocol_error },
    { PROTOCOL_EVENT_524_ERR_HELPNOTFOUND,      protocol_error },
    { PROTOCOL_EVENT_902_ERR_NICKLOCKED,        protocol_error },
    { PROTOCOL_EVENT_904_ERR_SASLFAIL,          protocol_error },
    { PROTOCOL_EVENT_905_ERR_SASLTOOLONG,       protocol_error },
    { PROTOCOL_EVENT_906_ERR_SASLABORTED,       protocol_error },
    { PROTOCOL_EVENT_907_ERR_SASLALREADY,       protocol_error },
    { PROTOCOL_EVENT_NONE,                      NULL } 
};

int protocol_init(protocol_t *protocol, kirc_t *ctx)
{
    memset(protocol, 0, sizeof(*protocol));

    protocol->ctx = ctx;
    protocol->event = PROTOCOL_EVENT_NONE;

    return 0;
}

int protocol_parse(protocol_t *protocol, char *line)
{

    size_t raw_n = sizeof(protocol->raw);
    safecpy(protocol->raw, line, raw_n);

    if (strncmp(line, "PING", 4) == 0) {
        protocol->event = PROTOCOL_EVENT_PING;
        size_t message_n = sizeof(protocol->message);
        safecpy(protocol->message, line + 6, message_n);
        return 0;
    }

    if (strncmp(line, "AUTHENTICATE +", 14) == 0) {
        protocol->event = PROTOCOL_EVENT_EXT_AUTHENTICATE;
        return 0;
    }

    if (strncmp(line, "ERROR", 5) == 0) {
        protocol->event = PROTOCOL_EVENT_ERROR;
        size_t message_n = sizeof(protocol->message);
        safecpy(protocol->message, line + 7, message_n);
        return 0;
    }

    char *token = strtok(line, " ");
    if (token == NULL) return -1;
    char *prefix = token + 1;
    char *suffix = strtok(NULL, ":");
    char *message = strtok(NULL, "\r");

    if (message != NULL) {
        size_t message_n = sizeof(protocol->message);
        safecpy(protocol->message, message, message_n);
    }

    char *nickname = strtok(prefix, "!");

    if (nickname != NULL) {
        size_t nickname_n = sizeof(protocol->nickname);
        safecpy(protocol->nickname, nickname, nickname_n);
    }

    char *command = strtok(suffix, "#& ");

    if (command != NULL) {
        size_t command_n = sizeof(protocol->command);
        safecpy(protocol->command, command, command_n);
    }

    char *channel = strtok(NULL, " \r");

    if (channel != NULL) {
        size_t channel_n = sizeof(protocol->channel);
        safecpy(protocol->channel, channel, channel_n);
    }

    char *params = strtok(NULL, ":\r");

    if (params != NULL) {
        size_t params_n = sizeof(protocol->params);
        safecpy(protocol->params, params, params_n);
    }

    for (int i = 0; event_table[i].command != NULL; i++) {
        if (strcmp(protocol->command, event_table[i].command) == 0) {
            protocol->event = event_table[i].event;
            break;
        }
    }

    protocol_ctcp_parse(protocol);

    return 0;
}

int protocol_handle(protocol_t *protocol)
{
    for(int i = 0; dispatch_table[i].handler != NULL; ++i) {
        if (dispatch_table[i].event == protocol->event) {
            dispatch_table[i].handler(protocol);
            return 0;
        }
    }

    protocol_raw(protocol);

    return -1;
}
