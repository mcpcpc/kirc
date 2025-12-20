#include "protocol.h"

static void get_time(char *out)
{
    time_t current;
    time(&current);
    struct tm *info = localtime(&current);
    strftime(out, 6, "%H:%M", info);
}

static void protocol_raw(protocol_t *protocol)
{
    char hhmm[6];
    get_time(hhmm);

    printf("\r\x1b[0K\x1b[2m%s \x1b[0m\x1b[7m%s\x1b[0m\r\n",
        hhmm, protocol->raw);
}

static void protocol_info(protocol_t *protocol)
{
    char hhmm[6];
    get_time(hhmm);

    printf("\r\x1b[0K\x1b[2m%s %s\x1b[0m\r\n",
        hhmm, protocol->message);
}

static void protocol_error(protocol_t *protocol)
{
    char hhmm[6];
    get_time(hhmm);

    printf("\r\x1b[0K\x1b[2m%s \x1b[1;31m%s\x1b[0m\r\n",
        hhmm, protocol->message);
}

static void protocol_notice(protocol_t *protocol)
{
    char hhmm[6];
    get_time(hhmm);

    printf("\r\x1b[0K\x1b[2m%s\x1b[0m \x1b[1;34m%s\x1b[0m %s\r\n",
        hhmm, protocol->nickname, protocol->message);
}

static void protocol_privmsg_direct(protocol_t *protocol)
{
    char hhmm[6];
    get_time(hhmm);

    printf("\r\x1b[0K\x1b[2m%s\x1b[0m \x1b[1;34m%s\x1b[0m \x1b[34m%s\x1b[0m\r\n",
        hhmm, protocol->nickname, protocol->message);
}

static void protocol_privmsg_indirect(protocol_t *protocol)
{
    char hhmm[6];
    get_time(hhmm);

    printf("\r\x1b[0K\x1b[2m%s\x1b[0m \x1b[1m%s\x1b[0m %s\r\n",
        hhmm, protocol->nickname, protocol->message);
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
    char hhmm[6];
    get_time(hhmm);

    
    if (strcmp(protocol->nickname, protocol->ctx->nickname) == 0) {
        size_t siz = sizeof(protocol->ctx->nickname) - 1;
        strncpy(protocol->ctx->nickname, protocol->message, siz);
        printf("\r\x1b[0K\x1b[2m%s\x1b[0m \x1b[2m"
            "you are now known as %s\x1b[0m\r\n",
            hhmm, protocol->message);
    } else {
        printf("\r\x1b[0K\x1b[2m%s\x1b[0m \x1b[2m"
            "%s is now known as %s\x1b[0m\r\n",
            hhmm, protocol->nickname, protocol->message);
    }

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

    if (strncmp(line, "PING", 4) == 0) {
        protocol->event = PROTOCOL_EVENT_PING;
        size_t message_n = sizeof(protocol->message) - 1;
        strncpy(protocol->message, line + 6, message_n);
        return 0;
    }

    if (strncmp(line, "AUTHENTICATE +", 14) == 0) {
        protocol->event = PROTOCOL_EVENT_EXT_AUTHENTICATE;
        return 0;
    }

    char *prefix = strtok(line, " ") + 1;
    char *suffix = strtok(NULL, ":");
    char *message = strtok(NULL, "\r");

    if (message != NULL) {
        size_t message_n = sizeof(protocol->message) - 1;
        strncpy(protocol->message, message, message_n);
    }

    char *nickname = strtok(prefix, "!");

    if (nickname != NULL) {
        size_t nickname_n = sizeof(protocol->nickname) - 1;
        strncpy(protocol->nickname, nickname, nickname_n);
    }

    char *command = strtok(suffix, "#& ");

    if (command != NULL) {
        size_t command_n = sizeof(protocol->command) - 1;
        strncpy(protocol->command, command, command_n);
    }

    char *channel = strtok(NULL, " \r");

    if (channel != NULL) {
        size_t channel_n = sizeof(protocol->channel) - 1;
        strncpy(protocol->channel, channel, channel_n);
    }

    char *params = strtok(NULL, ":\r");

    if (params != NULL) {
        size_t params_n = sizeof(protocol->params) - 1;
        strncpy(protocol->params, params, params_n);
    }

    for (int i = 0; map[i].command != NULL; i++) {
        if (strcmp(protocol->command, map[i].command) == 0) {
            protocol->event = map[i].event;
            break;
        }
    }

    return 0;
}

int protocol_render(protocol_t *protocol)
{
    switch (protocol->event) {
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
    case PROTOCOL_EVENT_250_RPL_STATSCONN:
    case PROTOCOL_EVENT_251_RPL_LUSERCLIENT:
    case PROTOCOL_EVENT_252_RPL_LUSEROP:
    case PROTOCOL_EVENT_253_RPL_LUSERUNKNOWN:
    case PROTOCOL_EVENT_254_RPL_LUSERCHANNELS:
    case PROTOCOL_EVENT_255_RPL_LUSERME:
    case PROTOCOL_EVENT_265_RPL_LOCALUSERS:
    case PROTOCOL_EVENT_266_RPL_GLOBALUSERS:
    case PROTOCOL_EVENT_301_RPL_AWAY:
    case PROTOCOL_EVENT_311_RPL_WHOISUSER:
    case PROTOCOL_EVENT_312_RPL_WHOISSERVER:
    case PROTOCOL_EVENT_315_RPL_ENDOFWHO:
    case PROTOCOL_EVENT_318_RPL_ENDOFWHOIS:
    case PROTOCOL_EVENT_319_RPL_WHOISCHANNELS:
    case PROTOCOL_EVENT_328_RPL_CHANNEL_URL:
    case PROTOCOL_EVENT_332_RPL_TOPIC:
    case PROTOCOL_EVENT_333_RPL_TOPICWHOTIME:
    case PROTOCOL_EVENT_351_RPL_VERSION:
    case PROTOCOL_EVENT_352_RPL_WHOREPLY:
    case PROTOCOL_EVENT_353_RPL_NAMREPLY:
    case PROTOCOL_EVENT_366_RPL_ENDOFNAMES:
    case PROTOCOL_EVENT_371_RPL_INFO:
    case PROTOCOL_EVENT_372_RPL_MOTD:
    case PROTOCOL_EVENT_374_RPL_ENDOFINFO:
    case PROTOCOL_EVENT_375_RPL_MOTDSTART:
    case PROTOCOL_EVENT_376_RPL_ENDOFMOTD:
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
    case PROTOCOL_EVENT_461_ERR_NEEDMOREPARAMS:
    case PROTOCOL_EVENT_465_ERR_YOUREBANNEDCREEP:
    case PROTOCOL_EVENT_470_ERR_LINKCHANNEL:
    case PROTOCOL_EVENT_481_ERR_NOPRIVILEGES:
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
