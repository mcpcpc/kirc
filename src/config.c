/*
 * config.c
 * Configuration parsing and initialization
 * Author: Michael Czigler
 * License: MIT
 */

#include "config.h"
#include "helper.h"

int config_validate_port(const char *value)
{
    if ((value == NULL) || (*value == '\0')) {
        return -1;
    }

    errno = 0;
    char *endptr;

    long port = strtol(value, &endptr, 10);

    if ((endptr == value) || (*endptr != '\0')) {
        return -1;
    }

    if ((errno == ERANGE && (port == LONG_MAX || port == LONG_MIN)) || 
        ((port > KIRC_PORT_RANGE_MAX) || (port < 0))) {
        return -1;
    }

    return 0;
}

void config_parse_channels(kirc_context_t *ctx, char *value)
{
    char *tok = NULL;
    size_t siz = 0, idx = 0;

    for (tok = strtok(value, ",|"); tok != NULL && idx < KIRC_CHANNEL_LIMIT; 
         tok = strtok(NULL, ",|")) {
        siz = sizeof(ctx->channels[idx]);
        safecpy(ctx->channels[idx], tok, siz);
        idx++;
    }
}

void config_parse_mechanism(kirc_context_t *ctx, char *value)
{
    char *mechanism = strtok(value, ":");
    char *token = strtok(NULL, ":");
    size_t siz = 0;

    if (strcmp(mechanism, "EXTERNAL") == 0) {
        ctx->mechanism = SASL_EXTERNAL;
    } else if (strcmp(mechanism, "PLAIN") == 0) {
        ctx->mechanism = SASL_PLAIN;
        siz = sizeof(ctx->auth);
        safecpy(ctx->auth, token, siz);
    }
}

static int config_apply_env(kirc_context_t *ctx, const char *env_name, 
                           char *dest, size_t dest_size)
{
    char *env = getenv(env_name);
    if (env && *env) {
        return safecpy(dest, env, dest_size);
    }
    return 0;
}

int config_init(kirc_context_t *ctx)
{
    memset(ctx, 0, sizeof(*ctx));
    
    size_t siz = 0;

    /* Set defaults */
    siz = sizeof(ctx->server);
    safecpy(ctx->server, KIRC_DEFAULT_SERVER, siz);
    
    siz = sizeof(ctx->port);
    safecpy(ctx->port, KIRC_DEFAULT_PORT, siz);

    ctx->mechanism = SASL_NONE;

    /* Load environment variables */
    config_apply_env(ctx, "KIRC_SERVER", ctx->server, sizeof(ctx->server));

    char *env_port = getenv("KIRC_PORT");
    if (env_port && *env_port) {
        if (config_validate_port(env_port) < 0) {
            fprintf(stderr, "invalid port number in KIRC_PORT\n");
            return -1;
        }
        siz = sizeof(ctx->port);
        safecpy(ctx->port, env_port, siz);
    }

    char *env_channels = getenv("KIRC_CHANNELS");
    if (env_channels && *env_channels) {
        config_parse_channels(ctx, env_channels);
    }

    config_apply_env(ctx, "KIRC_REALNAME", ctx->realname, sizeof(ctx->realname));
    config_apply_env(ctx, "KIRC_USERNAME", ctx->username, sizeof(ctx->username));
    config_apply_env(ctx, "KIRC_PASSWORD", ctx->password, sizeof(ctx->password));

    char *env_auth = getenv("KIRC_AUTH");
    if (env_auth && *env_auth) {
        config_parse_mechanism(ctx, env_auth);
    }

    return 0;
}

int config_parse_args(kirc_context_t *ctx, int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "%s: no arguments\n", argv[0]);
        return -1;
    }

    int opt;
    size_t siz = 0;

    while ((opt = getopt(argc, argv, "s:p:r:u:k:c:a:")) > 0) {
        switch (opt) {
        case 's':  /* server */
            siz = sizeof(ctx->server);
            safecpy(ctx->server, optarg, siz);
            break;

        case 'p':  /* port */
            if (config_validate_port(optarg) < 0) {
                fprintf(stderr, "invalid port number\n");
                return -1;
            }
            siz = sizeof(ctx->port);
            safecpy(ctx->port, optarg, siz);
            break;

        case 'r':  /* realname */
            siz = sizeof(ctx->realname);
            safecpy(ctx->realname, optarg, siz);
            break;

        case 'u':  /* username */
            siz = sizeof(ctx->username);
            safecpy(ctx->username, optarg, siz);
            break;

        case 'k':  /* password */
            siz = sizeof(ctx->password);
            safecpy(ctx->password, optarg, siz);
            break;

        case 'c':  /* channel(s) */
            config_parse_channels(ctx, optarg);
            break;

        case 'a':  /* SASL authentication */
            config_parse_mechanism(ctx, optarg);
            break;

        case ':':
            fprintf(stderr, "%s: missing -%c value\n", argv[0], opt);
            return -1;

        case '?':
            fprintf(stderr, "%s: unknown argument\n", argv[0]);
            return -1;

        default:
            return -1;
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "nickname not specified\n");
        return -1;
    }

    size_t nickname_n = sizeof(ctx->nickname);
    safecpy(ctx->nickname, argv[optind], nickname_n);

    return 0;
}

int config_free(kirc_context_t *ctx)
{
    size_t siz = sizeof(ctx->auth);

    if (memzero(ctx->auth, siz) < 0) {
        fprintf(stderr, "auth token value not safely cleared\n");
        return -1;
    }

    siz = sizeof(ctx->password);

    if (memzero(ctx->password, siz) < 0) {
        fprintf(stderr, "password value not safely cleared\n");
        return -1;
    }

    return 0;
}
