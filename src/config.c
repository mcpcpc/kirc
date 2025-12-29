/*
 * config.c
 * Configuration parsing and initialization
 * Author: Michael Czigler
 * License: MIT
 */

#include "config.h"

int config_validate_port(const char *value)
{
    if ((value == NULL) || (*value == '\0')) {
        return -1;
    }

    /* Check for invalid chars before conversion */
    for (const char *p = value; *p != '\0'; ++p) {
        if ((*p < '0') || (*p > '9')) {
            return -1;
        }
    }

    errno = 0;
    char *endptr;

    long port = strtol(value, &endptr, 10);

    if ((endptr == value) || (*endptr != '\0')) {
        return -1;
    }

    if (errno == ERANGE) {
        return -1;
    }

    if ((port > KIRC_PORT_RANGE_MAX) || (port < 0)) {
        return -1;
    }

    return 0;
}

void config_parse_channels(kirc_context_t *ctx, char *value)
{
    char *tok = NULL;
    size_t idx = 0;

    for (tok = strtok(value, ",|"); tok != NULL && idx < KIRC_CHANNEL_LIMIT; 
        tok = strtok(NULL, ",|")) {
        safecpy(ctx->channels[idx], tok, sizeof(ctx->channels[idx]));
        idx++;
    }
}

void config_parse_mechanism(kirc_context_t *ctx, char *value)
{
    char *mechanism = strtok(value, ":");
    char *token = strtok(NULL, ":");

    if (strcmp(mechanism, "EXTERNAL") == 0) {
        ctx->mechanism = SASL_EXTERNAL;
    } else if (strcmp(mechanism, "PLAIN") == 0) {
        ctx->mechanism = SASL_PLAIN;
        safecpy(ctx->auth, token, sizeof(ctx->auth));
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

    safecpy(ctx->server, KIRC_DEFAULT_SERVER,
        sizeof(ctx->server));
    
    safecpy(ctx->port, KIRC_DEFAULT_PORT,
        sizeof(ctx->port));

    ctx->mechanism = SASL_NONE;

    config_apply_env(ctx, "KIRC_SERVER", ctx->server, sizeof(ctx->server));

    char *env_port = getenv("KIRC_PORT");
    if (env_port && *env_port) {
        if (config_validate_port(env_port) < 0) {
            fprintf(stderr, "invalid port number in KIRC_PORT\n");
            return -1;
        }

        safecpy(ctx->port, env_port, sizeof(ctx->port));
    }

    char *env_channels = getenv("KIRC_CHANNELS");

    if (env_channels && *env_channels) {
        config_parse_channels(ctx, env_channels);
    }

    config_apply_env(ctx, "KIRC_REALNAME", ctx->realname,
        sizeof(ctx->realname));

    config_apply_env(ctx, "KIRC_USERNAME", ctx->username,
        sizeof(ctx->username));

    config_apply_env(ctx, "KIRC_PASSWORD", ctx->password,
        sizeof(ctx->password));

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

    while ((opt = getopt(argc, argv, "s:p:r:u:k:c:a:")) > 0) {
        switch (opt) {
        case 's':  /* server */
            safecpy(ctx->server, optarg, sizeof(ctx->server));
            break;

        case 'p':  /* port */
            if (config_validate_port(optarg) < 0) {
                fprintf(stderr, "invalid port number\n");
                return -1;
            }
            safecpy(ctx->port, optarg, sizeof(ctx->port));
            break;

        case 'r':  /* realname */
            safecpy(ctx->realname, optarg, sizeof(ctx->realname));
            break;

        case 'u':  /* username */
            safecpy(ctx->username, optarg, sizeof(ctx->username));
            break;

        case 'k':  /* password */
            safecpy(ctx->password, optarg, sizeof(ctx->password));
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
    if (memzero(ctx->auth, sizeof(ctx->auth)) < 0) {
        fprintf(stderr, "auth token value not safely cleared\n");
        return -1;
    }

    if (memzero(ctx->password, sizeof(ctx->password)) < 0) {
        fprintf(stderr, "password value not safely cleared\n");
        return -1;
    }

    return 0;
}
