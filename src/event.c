/*
 * event.c
 * Message event handling
 * Author: Michael Czigler
 * License: MIT
 */

#include "event.h"

/* Event dispatch table - used only for parsing */
static const struct event_dispatch_table event_table[] = {
    { "CAP",     EVENT_EXT_CAP },
    { "JOIN",    EVENT_JOIN },
    { "KICK",    EVENT_KICK },
    { "MODE",    EVENT_MODE },
    { "NICK",    EVENT_NICK },
    { "NOTICE",  EVENT_NOTICE },
    { "PART",    EVENT_PART },
    { "PING",    EVENT_PING },
    { "PRIVMSG", EVENT_PRIVMSG },
    { "QUIT",    EVENT_QUIT },
    { "TOPIC",   EVENT_TOPIC },
    { "001",     EVENT_001_RPL_WELCOME },
    { "002",     EVENT_002_RPL_YOURHOST },
    { "003",     EVENT_003_RPL_CREATED },
    { "004",     EVENT_004_RPL_MYINFO },
    { "005",     EVENT_005_RPL_BOUNCE },
    { "042",     EVENT_042_RPL_YOURID },
    { "200",     EVENT_200_RPL_TRACELINK },
    { "201",     EVENT_201_RPL_TRACECONNECTING },
    { "202",     EVENT_202_RPL_TRACEHANDSHAKE },
    { "203",     EVENT_203_RPL_TRACEUNKNOWN },
    { "204",     EVENT_204_RPL_TRACEOPERATOR },
    { "205",     EVENT_205_RPL_TRACEUSER },
    { "206",     EVENT_206_RPL_TRACESERVER },
    { "207",     EVENT_207_RPL_TRACESERVICE },
    { "208",     EVENT_208_RPL_TRACENEWTYPE },
    { "209",     EVENT_209_RPL_TRACECLASS },
    { "211",     EVENT_211_RPL_STATSLINKINFO },
    { "212",     EVENT_212_RPL_STATSCOMMANDS },
    { "213",     EVENT_213_RPL_STATSCLINE},
    { "215",     EVENT_215_RPL_STATSILINE },
    { "216",     EVENT_216_RPL_STATSKLINE },
    { "218",     EVENT_218_RPL_STATSYLINE },
    { "219",     EVENT_219_RPL_ENDOFSTATS },
    { "221",     EVENT_221_RPL_UMODEIS },
    { "234",     EVENT_234_RPL_SERVLIST },
    { "235",     EVENT_235_RPL_SERVLISTEND },
    { "241",     EVENT_241_RPL_STATSLLINE },
    { "242",     EVENT_242_RPL_STATSUPTIME },
    { "243",     EVENT_243_RPL_STATSOLINE },
    { "244",     EVENT_244_RPL_STATSHLINE },
    { "245",     EVENT_245_RPL_STATSSLINE },
    { "250",     EVENT_250_RPL_STATSCONN },
    { "251",     EVENT_251_RPL_LUSERCLIENT },
    { "252",     EVENT_252_RPL_LUSEROP },
    { "253",     EVENT_253_RPL_LUSERUNKNOWN },
    { "254",     EVENT_254_RPL_LUSERCHANNELS },
    { "255",     EVENT_255_RPL_LUSERME },
    { "256",     EVENT_256_RPL_ADMINME },
    { "257",     EVENT_257_RPL_ADMINLOC1 },
    { "258",     EVENT_258_RPL_ADMINLOC2 },
    { "259",     EVENT_259_RPL_ADMINEMAIL },
    { "261",     EVENT_261_RPL_TRACELOG },
    { "263",     EVENT_263_RPL_TRYAGAIN },
    { "265",     EVENT_265_RPL_LOCALUSERS },
    { "266",     EVENT_266_RPL_GLOBALUSERS },
    { "300",     EVENT_300_RPL_NONE },
    { "301",     EVENT_301_RPL_AWAY },
    { "302",     EVENT_302_RPL_USERHOST },
    { "303",     EVENT_303_RPL_ISON },
    { "305",     EVENT_305_RPL_UNAWAY },
    { "306",     EVENT_306_RPL_NOWAWAY },
    { "311",     EVENT_311_RPL_WHOISUSER },
    { "312",     EVENT_312_RPL_WHOISSERVER },
    { "313",     EVENT_313_RPL_WHOISOPERATOR },
    { "314",     EVENT_314_RPL_WHOWASUSER },
    { "315",     EVENT_315_RPL_ENDOFWHO },
    { "317",     EVENT_317_RPL_WHOISIDLE },
    { "318",     EVENT_318_RPL_ENDOFWHOIS },
    { "319",     EVENT_319_RPL_WHOISCHANNELS },
    { "322",     EVENT_322_RPL_LIST },
    { "323",     EVENT_323_RPL_LISTEND },
    { "324",     EVENT_324_RPL_CHANNELMODEIS },
    { "328",     EVENT_328_RPL_CHANNEL_URL },
    { "331",     EVENT_331_RPL_NOTOPIC },
    { "332",     EVENT_332_RPL_TOPIC },
    { "333",     EVENT_333_RPL_TOPICWHOTIME },
    { "341",     EVENT_341_RPL_INVITING },
    { "346",     EVENT_346_RPL_INVITELIST },
    { "347",     EVENT_347_RPL_ENDOFINVITELIST },
    { "348",     EVENT_348_RPL_EXCEPTLIST },
    { "349",     EVENT_349_RPL_ENDOFEXCEPTLIST },
    { "351",     EVENT_351_RPL_VERSION },
    { "352",     EVENT_352_RPL_WHOREPLY },
    { "353",     EVENT_353_RPL_NAMREPLY },
    { "364",     EVENT_364_RPL_LINKS },
    { "365",     EVENT_365_RPL_ENDOFLINKS },
    { "366",     EVENT_366_RPL_ENDOFNAMES },
    { "367",     EVENT_367_RPL_BANLIST },
    { "368",     EVENT_368_RPL_ENDOFBANLIST },
    { "369",     EVENT_369_RPL_ENDOFWHOWAS },
    { "375",     EVENT_375_RPL_MOTDSTART },
    { "371",     EVENT_371_RPL_INFO },
    { "372",     EVENT_372_RPL_MOTD },
    { "374",     EVENT_374_RPL_ENDOFINFO },
    { "376",     EVENT_376_RPL_ENDOFMOTD },
    { "381",     EVENT_381_RPL_YOUREOPER },
    { "382",     EVENT_382_RPL_REHASHING },
    { "383",     EVENT_383_RPL_YOURESERVICE },
    { "391",     EVENT_391_RPL_TIME },
    { "392",     EVENT_392_RPL_USERSSTART },
    { "393",     EVENT_393_RPL_USERS },
    { "394",     EVENT_394_RPL_ENDOFUSERS },
    { "395",     EVENT_395_RPL_NOUSERS },
    { "396",     EVENT_396_RPL_HOSTHIDDEN },
    { "400",     EVENT_400_ERR_UNKNOWNERROR },
    { "401",     EVENT_401_ERR_NOSUCHNICK },
    { "402",     EVENT_402_ERR_NOSUCHSERVER },
    { "403",     EVENT_403_ERR_NOSUCHCHANNEL },
    { "404",     EVENT_404_ERR_CANNOTSENDTOCHAN },
    { "405",     EVENT_405_ERR_TOOMANYCHANNELS },
    { "406",     EVENT_406_ERR_WASNOSUCHNICK },
    { "407",     EVENT_407_ERR_TOOMANYTARGETS },
    { "408",     EVENT_408_ERR_NOSUCHSERVICE },
    { "409",     EVENT_409_ERR_NOORIGIN },
    { "411",     EVENT_411_ERR_NORECIPIENT },
    { "412",     EVENT_412_ERR_NOTEXTTOSEND },
    { "413",     EVENT_413_ERR_NOTOPLEVEL },
    { "414",     EVENT_414_ERR_WILDTOPLEVEL },
    { "415",     EVENT_415_ERR_BADMASK },
    { "421",     EVENT_421_ERR_UNKNOWNCOMMAND },
    { "422",     EVENT_422_ERR_NOMOTD },
    { "423",     EVENT_423_ERR_NOADMININFO },
    { "424",     EVENT_424_ERR_FILEERROR },
    { "431",     EVENT_431_ERR_NONICKNAMEGIVEN },
    { "432",     EVENT_432_ERR_ERRONEUSNICKNAME },
    { "433",     EVENT_433_ERR_NICKNAMEINUSE },
    { "436",     EVENT_436_ERR_NICKCOLLISION },
    { "441",     EVENT_441_ERR_USERNOTINCHANNEL },
    { "442",     EVENT_442_ERR_NOTONCHANNEL },
    { "443",     EVENT_443_ERR_USERONCHANNEL },
    { "444",     EVENT_444_ERR_NOLOGIN },
    { "445",     EVENT_445_ERR_SUMMONDISABLED },
    { "446",     EVENT_446_ERR_USERSDISABLED },
    { "451",     EVENT_451_ERR_NOTREGISTERED },
    { "461",     EVENT_461_ERR_NEEDMOREPARAMS },
    { "462",     EVENT_462_ERR_ALREADYREGISTERED },
    { "463",     EVENT_463_ERR_NOPERMFORHOST },
    { "464",     EVENT_464_ERR_PASSWDMISMATCH },
    { "465",     EVENT_465_ERR_YOUREBANNEDCREEP },
    { "467",     EVENT_467_ERR_KEYSET },
    { "470",     EVENT_470_ERR_LINKCHANNEL },
    { "471",     EVENT_471_ERR_CHANNELISFULL },
    { "472",     EVENT_472_ERR_UNKNOWNMODE },
    { "473",     EVENT_473_ERR_INVITEONLYCHAN },
    { "474",     EVENT_474_ERR_BANNEDFROMCHAN },
    { "475",     EVENT_475_ERR_BADCHANNELKEY },
    { "476",     EVENT_476_ERR_BADCHANMASK },
    { "477",     EVENT_477_ERR_NEEDREGGEDNICK },
    { "478",     EVENT_478_ERR_BANLISTFULL },
    { "481",     EVENT_481_ERR_NOPRIVILEGES },
    { "482",     EVENT_482_ERR_CHANOPRIVSNEEDED },
    { "483",     EVENT_483_ERR_CANTKILLSERVER },
    { "485",     EVENT_485_ERR_UNIQOPRIVSNEEDED },
    { "491",     EVENT_491_ERR_NOOPERHOST },
    { "501",     EVENT_501_ERR_UMODEUNKNOWNFLAG },
    { "502",     EVENT_502_ERR_USERSDONTMATCH },
    { "524",     EVENT_524_ERR_HELPNOTFOUND },
    { "704",     EVENT_704_RPL_HELPSTART },
    { "705",     EVENT_705_RPL_HELPTXT },
    { "706",     EVENT_706_RPL_ENDOFHELP },
    { "900",     EVENT_900_RPL_LOGGEDIN },
    { "901",     EVENT_901_RPL_LOGGEDOUT },
    { "902",     EVENT_902_ERR_NICKLOCKED },
    { "903",     EVENT_903_RPL_SASLSUCCESS },
    { "904",     EVENT_904_ERR_SASLFAIL },
    { "905",     EVENT_905_ERR_SASLTOOLONG },
    { "906",     EVENT_906_ERR_SASLABORTED },
    { "907",     EVENT_907_ERR_SASLALREADY },
    { "908",     EVENT_908_RPL_SASLMECHS },
    { NULL,      EVENT_NONE }
};

int event_init(struct event *event, struct kirc_context *ctx)
{
    memset(event, 0, sizeof(*event));

    event->ctx = ctx;
    event->type = EVENT_NONE;

    return 0;
}

static int event_ctcp_parse(struct event *event)
{
    if (((event->type == EVENT_PRIVMSG) ||
        (event->type == EVENT_NOTICE)) &&
        (event->message[0] == '\001')) {
        char ctcp[MESSAGE_MAX_LEN];
        size_t siz = sizeof(event->message);
        size_t len = strnlen(event->message, siz);
        size_t end = 1;

        while ((end < len) && event->message[end] != '\001') {
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

            safecpy(ctcp, event->message + 1, copy_n); 

            char *command = strtok(ctcp, " ");
            char *args = strtok(NULL, "");

            if (command != NULL) {
                if (strcmp(command, "ACTION") == 0) {
                    event->type = EVENT_CTCP_ACTION;
                    if (args != NULL) {
                        siz = sizeof(event->message);
                        safecpy(event->message, args, siz);
                    } else {
                        event->message[0] = '\0';
                    }
                } else if (strcmp(command, "VERSION") == 0) {
                    event->type = EVENT_CTCP_VERSION;
                    if (args != NULL) {
                        siz = sizeof(event->params);
                        safecpy(event->params, args, siz);
                    } else {
                        event->params[0] = '\0';
                    }
                } else if (strcmp(command, "PING") == 0) {
                    event->type = EVENT_CTCP_PING;
                    if (args != NULL) {
                        siz = sizeof(event->message);
                        safecpy(event->message, args, siz);
                    } else {
                        event->message[0] = '\0';
                    }
                } else if (strcmp(command, "TIME") == 0) {
                    event->type = EVENT_CTCP_TIME;
                } else if (strcmp(command, "CLIENTINFO") == 0) {
                    event->type = EVENT_CTCP_CLIENTINFO;
                } else if (strcmp(command, "DCC") == 0) {
                    event->type = EVENT_CTCP_DCC;
                    if (args != NULL) {
                        siz = sizeof(event->message);
                        safecpy(event->message, args, siz);
                    } else {
                        event->message[0] = '\0';
                    }
                } else {
                    event->message[0] = '\0';
                }
            }
        }
    }

    return 0;
}

int event_parse(struct event *event, char *line)
{

    char line_copy[MESSAGE_MAX_LEN];
    safecpy(line_copy, line, sizeof(line_copy));
    safecpy(event->raw, line, sizeof(event->raw));

    if (strncmp(line, "PING", 4) == 0) {
        event->type = EVENT_PING;
        size_t message_n = sizeof(event->message);
        safecpy(event->message, line + 6, message_n);
        return 0;
    }

    if (strncmp(line, "AUTHENTICATE +", 14) == 0) {
        event->type = EVENT_EXT_AUTHENTICATE;
        return 0;
    }

    if (strncmp(line, "ERROR", 5) == 0) {
        event->type = EVENT_ERROR;
        size_t message_n = sizeof(event->message);
        safecpy(event->message, line + 7, message_n);
        return 0;
    }

    char *token = strtok(line_copy, " ");

    if (token == NULL) {
        return -1;
    }

    char *prefix = token + 1;
    char *suffix = strtok(NULL, ":");

    if (suffix == NULL) {
        return -1;
    }

    char *message = strtok(NULL, "\r");

    if (message != NULL) {
        size_t message_n = sizeof(event->message);
        safecpy(event->message, message, message_n);
    }

    char *nickname = strtok(prefix, "!");

    if (nickname != NULL) {
        size_t nickname_n = sizeof(event->nickname);
        safecpy(event->nickname, nickname, nickname_n);
    }

    char *command = strtok(suffix, "#& ");

    if (command != NULL) {
        size_t command_n = sizeof(event->command);
        safecpy(event->command, command, command_n);
    }

    char *channel = strtok(NULL, " \r");

    if (channel != NULL) {
        size_t channel_n = sizeof(event->channel);
        safecpy(event->channel, channel, channel_n);
    }

    char *params = strtok(NULL, ":\r");

    if (params != NULL) {
        size_t params_n = sizeof(event->params);
        safecpy(event->params, params, params_n);
    }

    for (int i = 0; event_table[i].command != NULL; i++) {
        if (strcmp(event->command, event_table[i].command) == 0) {
            event->type = event_table[i].type;
            break;
        }
    }

    event_ctcp_parse(event);

    return 0;
}
