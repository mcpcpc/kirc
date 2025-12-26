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

            if (ctcp_len < sizeof(ctcp) - 1) {
                copy_n = ctcp_len;
            } else {
                copy_n = sizeof(ctcp) - 1;
            }

            strncpy(ctcp, protocol->message + 1, copy_n);
            ctcp[copy_n] = '\0';

            char *command = strtok(ctcp, " ");
            char *args = strtok(NULL, "");

            if (command != NULL) {
                if (strcmp(command, "ACTION") == 0) {
                    protocol->event = PROTOCOL_EVENT_CTCP_ACTION;
                    if (args != NULL) {
                        siz = sizeof(protocol->message) - 1;
                        strncpy(protocol->message, args, siz);
                        protocol->message[siz] = '\0';
                    } else {
                        protocol->message[0] = '\0';
                    }
                } else if (strcmp(command, "VERSION") == 0) {
                    protocol->event = PROTOCOL_EVENT_CTCP_VERSION;
                    if (args != NULL) {
                        siz = sizeof(protocol->params) - 1;
                        strncpy(protocol->params, args, siz);
                        protocol->params[siz] = '\0';
                    } else {
                        protocol->params[0] = '\0';
                    }
                } else if (strcmp(command, "PING") == 0) {
                    protocol->event = PROTOCOL_EVENT_CTCP_PING;
                    if (args != NULL) {
                        siz = sizeof(protocol->message) - 1;
                        strncpy(protocol->message, args, siz);
                        protocol->message[siz] = '\0';
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
                        siz = sizeof(protocol->message) - 1;
                        strncpy(protocol->message, args, siz);
                        protocol->message[siz] = '\0';
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

int protocol_init(protocol_t *protocol, kirc_t *ctx)
{
    memset(protocol, 0, sizeof(*protocol));

    protocol->ctx = ctx;
    protocol->event = PROTOCOL_EVENT_NONE;

    return 0;
}

int protocol_parse(protocol_t *protocol, char *line)
{

    size_t raw_n = sizeof(protocol->raw) - 1;
    strncpy(protocol->raw, line, raw_n);
    protocol->raw[raw_n] = '\0';

    if (strncmp(line, "PING", 4) == 0) {
        protocol->event = PROTOCOL_EVENT_PING;
        size_t message_n = sizeof(protocol->message) - 1;
        strncpy(protocol->message, line + 6, message_n);
        protocol->message[message_n] = '\0';
        return 0;
    }

    if (strncmp(line, "AUTHENTICATE +", 14) == 0) {
        protocol->event = PROTOCOL_EVENT_EXT_AUTHENTICATE;
        return 0;
    }

    if (strncmp(line, "ERROR", 5) == 0) {
        protocol->event = PROTOCOL_EVENT_ERROR;
        size_t message_n = sizeof(protocol->message) - 1;
        strncpy(protocol->message, line + 7, message_n);
        protocol->message[message_n] = '\0';
        return 0;
    }

    char *token = strtok(line, " ");
    if (token == NULL) return -1;
    char *prefix = token + 1;
    char *suffix = strtok(NULL, ":");
    char *message = strtok(NULL, "\r");

    if (message != NULL) {
        size_t message_n = sizeof(protocol->message) - 1;
        strncpy(protocol->message, message, message_n);
        protocol->message[message_n] = '\0';
    }

    char *nickname = strtok(prefix, "!");

    if (nickname != NULL) {
        size_t nickname_n = sizeof(protocol->nickname) - 1;
        strncpy(protocol->nickname, nickname, nickname_n);
        protocol->nickname[nickname_n] = '\0';
    }

    char *command = strtok(suffix, "#& ");

    if (command != NULL) {
        size_t command_n = sizeof(protocol->command) - 1;
        strncpy(protocol->command, command, command_n);
        protocol->command[command_n] = '\0';
    }

    char *channel = strtok(NULL, " \r");

    if (channel != NULL) {
        size_t channel_n = sizeof(protocol->channel) - 1;
        strncpy(protocol->channel, channel, channel_n);
        protocol->channel[channel_n] = '\0';
    }

    char *params = strtok(NULL, ":\r");

    if (params != NULL) {
        size_t params_n = sizeof(protocol->params) - 1;
        strncpy(protocol->params, params, params_n);
        protocol->params[params_n] = '\0';
    }

    for (int i = 0; map[i].command != NULL; i++) {
        if (strcmp(protocol->command, map[i].command) == 0) {
            protocol->event = map[i].event;
            break;
        }
    }

    protocol_ctcp_parse(protocol);

    return 0;
}

int protocol_handle(protocol_t *protocol)
{
    switch (protocol->event) {
    case PROTOCOL_EVENT_CTCP_ACTION:
        protocol_ctcp_action(protocol);
        break;

    case PROTOCOL_EVENT_CTCP_CLIENTINFO:
    case PROTOCOL_EVENT_CTCP_DCC:
    case PROTOCOL_EVENT_CTCP_PING:
    case PROTOCOL_EVENT_CTCP_TIME:
    case PROTOCOL_EVENT_CTCP_VERSION:
        protocol_ctcp_info(protocol);
        break;

    case PROTOCOL_EVENT_JOIN:
    case PROTOCOL_EVENT_PART:
    case PROTOCOL_EVENT_PING:
    case PROTOCOL_EVENT_QUIT:
        break;

    case PROTOCOL_EVENT_PRIVMSG:
        protocol_privmsg(protocol);
        break;

    case PROTOCOL_EVENT_NICK:
        protocol_nick(protocol);
        break;

    case PROTOCOL_EVENT_NOTICE:
        protocol_notice(protocol);
        break;

    case PROTOCOL_EVENT_KICK:
    case PROTOCOL_EVENT_EXT_AUTHENTICATE:
    case PROTOCOL_EVENT_EXT_CAP:
    case PROTOCOL_EVENT_MODE:
    case PROTOCOL_EVENT_TOPIC:
    case PROTOCOL_EVENT_001_RPL_WELCOME:
    case PROTOCOL_EVENT_002_RPL_YOURHOST:
    case PROTOCOL_EVENT_003_RPL_CREATED:
    case PROTOCOL_EVENT_004_RPL_MYINFO:
    case PROTOCOL_EVENT_005_RPL_BOUNCE:
    case PROTOCOL_EVENT_042_RPL_YOURID:
    case PROTOCOL_EVENT_200_RPL_TRACELINK:
    case PROTOCOL_EVENT_201_RPL_TRACECONNECTING:
    case PROTOCOL_EVENT_202_RPL_TRACEHANDSHAKE:
    case PROTOCOL_EVENT_203_RPL_TRACEUNKNOWN:
    case PROTOCOL_EVENT_204_RPL_TRACEOPERATOR:
    case PROTOCOL_EVENT_205_RPL_TRACEUSER:
    case PROTOCOL_EVENT_206_RPL_TRACESERVER:
    case PROTOCOL_EVENT_207_RPL_TRACESERVICE:
    case PROTOCOL_EVENT_208_RPL_TRACENEWTYPE:
    case PROTOCOL_EVENT_209_RPL_TRACECLASS:
    case PROTOCOL_EVENT_211_RPL_STATSLINKINFO:
    case PROTOCOL_EVENT_212_RPL_STATSCOMMANDS:
    case PROTOCOL_EVENT_213_RPL_STATSCLINE:
    case PROTOCOL_EVENT_215_RPL_STATSILINE:
    case PROTOCOL_EVENT_216_RPL_STATSKLINE:
    case PROTOCOL_EVENT_218_RPL_STATSYLINE:
    case PROTOCOL_EVENT_219_RPL_ENDOFSTATS:
    case PROTOCOL_EVENT_221_RPL_UMODEIS:
    case PROTOCOL_EVENT_234_RPL_SERVLIST:
    case PROTOCOL_EVENT_235_RPL_SERVLISTEND:
    case PROTOCOL_EVENT_241_RPL_STATSLLINE:
    case PROTOCOL_EVENT_242_RPL_STATSUPTIME:
    case PROTOCOL_EVENT_243_RPL_STATSOLINE:
    case PROTOCOL_EVENT_244_RPL_STATSHLINE:
    case PROTOCOL_EVENT_245_RPL_STATSSLINE:
    case PROTOCOL_EVENT_250_RPL_STATSCONN:
    case PROTOCOL_EVENT_251_RPL_LUSERCLIENT:
    case PROTOCOL_EVENT_252_RPL_LUSEROP:
    case PROTOCOL_EVENT_253_RPL_LUSERUNKNOWN:
    case PROTOCOL_EVENT_254_RPL_LUSERCHANNELS:
    case PROTOCOL_EVENT_255_RPL_LUSERME:
    case PROTOCOL_EVENT_256_RPL_ADMINME:
    case PROTOCOL_EVENT_257_RPL_ADMINLOC1:
    case PROTOCOL_EVENT_258_RPL_ADMINLOC2:
    case PROTOCOL_EVENT_259_RPL_ADMINEMAIL:
    case PROTOCOL_EVENT_261_RPL_TRACELOG:
    case PROTOCOL_EVENT_263_RPL_TRYAGAIN:
    case PROTOCOL_EVENT_265_RPL_LOCALUSERS:
    case PROTOCOL_EVENT_266_RPL_GLOBALUSERS:
    case PROTOCOL_EVENT_300_RPL_NONE:
    case PROTOCOL_EVENT_301_RPL_AWAY:
    case PROTOCOL_EVENT_302_RPL_USERHOST:
    case PROTOCOL_EVENT_303_RPL_ISON:
    case PROTOCOL_EVENT_305_RPL_UNAWAY:
    case PROTOCOL_EVENT_306_RPL_NOWAWAY:
    case PROTOCOL_EVENT_311_RPL_WHOISUSER:
    case PROTOCOL_EVENT_312_RPL_WHOISSERVER:
    case PROTOCOL_EVENT_313_RPL_WHOISOPERATOR:
    case PROTOCOL_EVENT_314_RPL_WHOWASUSER:
    case PROTOCOL_EVENT_315_RPL_ENDOFWHO:
    case PROTOCOL_EVENT_317_RPL_WHOISIDLE:
    case PROTOCOL_EVENT_318_RPL_ENDOFWHOIS:
    case PROTOCOL_EVENT_319_RPL_WHOISCHANNELS:
    case PROTOCOL_EVENT_322_RPL_LIST:
    case PROTOCOL_EVENT_323_RPL_LISTEND:
    case PROTOCOL_EVENT_324_RPL_CHANNELMODEIS:
    case PROTOCOL_EVENT_328_RPL_CHANNEL_URL:
    case PROTOCOL_EVENT_331_RPL_NOTOPIC:
    case PROTOCOL_EVENT_332_RPL_TOPIC:
    case PROTOCOL_EVENT_333_RPL_TOPICWHOTIME:
    case PROTOCOL_EVENT_341_RPL_INVITING:
    case PROTOCOL_EVENT_346_RPL_INVITELIST:
    case PROTOCOL_EVENT_347_RPL_ENDOFINVITELIST:
    case PROTOCOL_EVENT_348_RPL_EXCEPTLIST:
    case PROTOCOL_EVENT_349_RPL_ENDOFEXCEPTLIST:
    case PROTOCOL_EVENT_351_RPL_VERSION:
    case PROTOCOL_EVENT_352_RPL_WHOREPLY:
    case PROTOCOL_EVENT_353_RPL_NAMREPLY:
    case PROTOCOL_EVENT_364_RPL_LINKS:
    case PROTOCOL_EVENT_365_RPL_ENDOFLINKS:
    case PROTOCOL_EVENT_366_RPL_ENDOFNAMES:
    case PROTOCOL_EVENT_367_RPL_BANLIST:
    case PROTOCOL_EVENT_368_RPL_ENDOFBANLIST:
    case PROTOCOL_EVENT_369_RPL_ENDOFWHOWAS:
    case PROTOCOL_EVENT_371_RPL_INFO:
    case PROTOCOL_EVENT_372_RPL_MOTD:
    case PROTOCOL_EVENT_374_RPL_ENDOFINFO:
    case PROTOCOL_EVENT_375_RPL_MOTDSTART:
    case PROTOCOL_EVENT_376_RPL_ENDOFMOTD:
    case PROTOCOL_EVENT_381_RPL_YOUREOPER:
    case PROTOCOL_EVENT_382_RPL_REHASHING:
    case PROTOCOL_EVENT_383_RPL_YOURESERVICE:
    case PROTOCOL_EVENT_391_RPL_TIME:
    case PROTOCOL_EVENT_392_RPL_USERSSTART:
    case PROTOCOL_EVENT_393_RPL_USERS:
    case PROTOCOL_EVENT_394_RPL_ENDOFUSERS:
    case PROTOCOL_EVENT_395_RPL_NOUSERS:
    case PROTOCOL_EVENT_396_RPL_HOSTHIDDEN:
    case PROTOCOL_EVENT_704_RPL_HELPSTART:
    case PROTOCOL_EVENT_705_RPL_HELPTXT:
    case PROTOCOL_EVENT_706_RPL_ENDOFHELP:
    case PROTOCOL_EVENT_900_RPL_LOGGEDIN:
    case PROTOCOL_EVENT_901_RPL_LOGGEDOUT:
    case PROTOCOL_EVENT_903_RPL_SASLSUCCESS:
    case PROTOCOL_EVENT_908_RPL_SASLMECHS:
        protocol_info(protocol);
        break;

    case PROTOCOL_EVENT_ERROR:
    case PROTOCOL_EVENT_400_ERR_UNKNOWNERROR:
    case PROTOCOL_EVENT_401_ERR_NOSUCHNICK:
    case PROTOCOL_EVENT_402_ERR_NOSUCHSERVER:
    case PROTOCOL_EVENT_403_ERR_NOSUCHCHANNEL:
    case PROTOCOL_EVENT_404_ERR_CANNOTSENDTOCHAN:
    case PROTOCOL_EVENT_405_ERR_TOOMANYCHANNELS:
    case PROTOCOL_EVENT_406_ERR_WASNOSUCHNICK:
    case PROTOCOL_EVENT_407_ERR_TOOMANYTARGETS:
    case PROTOCOL_EVENT_408_ERR_NOSUCHSERVICE:
    case PROTOCOL_EVENT_409_ERR_NOORIGIN:
    case PROTOCOL_EVENT_411_ERR_NORECIPIENT:
    case PROTOCOL_EVENT_412_ERR_NOTEXTTOSEND:
    case PROTOCOL_EVENT_413_ERR_NOTOPLEVEL:
    case PROTOCOL_EVENT_414_ERR_WILDTOPLEVEL:
    case PROTOCOL_EVENT_415_ERR_BADMASK:
    case PROTOCOL_EVENT_421_ERR_UNKNOWNCOMMAND:
    case PROTOCOL_EVENT_422_ERR_NOMOTD:
    case PROTOCOL_EVENT_423_ERR_NOADMININFO:
    case PROTOCOL_EVENT_424_ERR_FILEERROR:
    case PROTOCOL_EVENT_431_ERR_NONICKNAMEGIVEN:
    case PROTOCOL_EVENT_432_ERR_ERRONEUSNICKNAME:
    case PROTOCOL_EVENT_433_ERR_NICKNAMEINUSE:
    case PROTOCOL_EVENT_436_ERR_NICKCOLLISION:
    case PROTOCOL_EVENT_441_ERR_USERNOTINCHANNEL:
    case PROTOCOL_EVENT_442_ERR_NOTONCHANNEL:
    case PROTOCOL_EVENT_443_ERR_USERONCHANNEL:
    case PROTOCOL_EVENT_444_ERR_NOLOGIN:
    case PROTOCOL_EVENT_445_ERR_SUMMONDISABLED:
    case PROTOCOL_EVENT_446_ERR_USERSDISABLED:
    case PROTOCOL_EVENT_451_ERR_NOTREGISTERED:
    case PROTOCOL_EVENT_461_ERR_NEEDMOREPARAMS:
    case PROTOCOL_EVENT_462_ERR_ALREADYREGISTERED:
    case PROTOCOL_EVENT_463_ERR_NOPERMFORHOST:
    case PROTOCOL_EVENT_464_ERR_PASSWDMISMATCH:
    case PROTOCOL_EVENT_465_ERR_YOUREBANNEDCREEP:
    case PROTOCOL_EVENT_467_ERR_KEYSET:
    case PROTOCOL_EVENT_470_ERR_LINKCHANNEL:
    case PROTOCOL_EVENT_471_ERR_CHANNELISFULL:
    case PROTOCOL_EVENT_472_ERR_UNKNOWNMODE:
    case PROTOCOL_EVENT_473_ERR_INVITEONLYCHAN:
    case PROTOCOL_EVENT_474_ERR_BANNEDFROMCHAN:
    case PROTOCOL_EVENT_475_ERR_BADCHANNELKEY:
    case PROTOCOL_EVENT_476_ERR_BADCHANMASK:
    case PROTOCOL_EVENT_477_ERR_NEEDREGGEDNICK:
    case PROTOCOL_EVENT_478_ERR_BANLISTFULL:
    case PROTOCOL_EVENT_481_ERR_NOPRIVILEGES:
    case PROTOCOL_EVENT_482_ERR_CHANOPRIVSNEEDED:
    case PROTOCOL_EVENT_483_ERR_CANTKILLSERVER:
    case PROTOCOL_EVENT_485_ERR_UNIQOPRIVSNEEDED:
    case PROTOCOL_EVENT_491_ERR_NOOPERHOST:
    case PROTOCOL_EVENT_501_ERR_UMODEUNKNOWNFLAG:
    case PROTOCOL_EVENT_502_ERR_USERSDONTMATCH:
    case PROTOCOL_EVENT_902_ERR_NICKLOCKED:
    case PROTOCOL_EVENT_904_ERR_SASLFAIL:
    case PROTOCOL_EVENT_905_ERR_SASLTOOLONG:
    case PROTOCOL_EVENT_906_ERR_SASLABORTED:
    case PROTOCOL_EVENT_907_ERR_SASLALREADY:
        protocol_error(protocol);
        break;

    default:
        protocol_raw(protocol);
        break;
    }

    return 0;
}
