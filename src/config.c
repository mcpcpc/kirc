/*
 * config.c
 * Configuration parsing and initialization
 * Author: Michael Czigler
 * License: MIT
 */

#include "config.h"

/**
 * config_validate_port() - Validate a port number string
 * @value: String representation of the port number
 *
 * Validates that the given string is a valid port number. Checks for
 * non-numeric characters, range limits, and conversion errors.
 *
 * Return: 0 if valid port, -1 if invalid or out of range
 */
static int config_validate_port(const char *value)
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

/**
 * config_parse_channels() - Parse comma or pipe-separated channel list
 * @ctx: IRC context structure to store parsed channels
 * @value: String containing channel names separated by ',' or '|'
 *
 * Parses a string of channel names and stores them in the context structure.
 * Supports both comma and pipe delimiters. Limited to KIRC_CHANNEL_LIMIT
 * channels.
 */
static void config_parse_channels(struct kirc_context *ctx, char *value)
{
    char *tok = NULL;
    size_t idx = 0;

    for (tok = strtok(value, ",|"); tok != NULL && idx < KIRC_CHANNEL_LIMIT; 
        tok = strtok(NULL, ",|")) {
        safecpy(ctx->channels[idx], tok, sizeof(ctx->channels[idx]));
        idx++;
    }
}

/**
 * config_parse_mechanism() - Parse SASL authentication mechanism
 * @ctx: IRC context structure to store authentication settings
 * @value: String containing mechanism and credentials
 *
 * Parses SASL authentication mechanism (EXTERNAL or PLAIN) with optional
 * credentials. For PLAIN, expects format "PLAIN:authzid:authcid:password"
 * and base64-encodes the credentials. For EXTERNAL, no additional data needed.
 */
static void config_parse_mechanism(struct kirc_context *ctx, char *value)
{
    char *mechanism = strtok(value, ":");

    if (mechanism == NULL) {
        return;
    }

    if (strcmp(mechanism, "EXTERNAL") == 0) {
        ctx->mechanism = SASL_EXTERNAL;
        return;
    } else if (strcmp(mechanism, "PLAIN") == 0) {
        ctx->mechanism = SASL_PLAIN;
    } else {
        return;  /* invalid mechanism */
    }

    char *token = strtok(NULL, "");

    if (token == NULL) {
        return;  /* invalid token */
    }

    int count = 0;
    
    for (int i = 0; token[i] != '\0'; ++i) {
        if (token[i] == ':') count++;
    }

    if (count == 2) {
        size_t plain_len = 0;
        char plain[MESSAGE_MAX_LEN];
        char *p = plain;

        char *authzid = strtok(token, ":");
        
        if (authzid == NULL) {
            return;
        }

        size_t authzid_len = strnlen(authzid, 255);
        memcpy(p, authzid, authzid_len);
        p += authzid_len;
        *p++ = '\0';
        plain_len += authzid_len + 1;

        char *authcid = strtok(NULL, ":");

        if (authcid == NULL) {
            return;
        }

        size_t authcid_len = strnlen(authcid, 255);
        memcpy(p, authcid, authcid_len);
        p += authcid_len;
        *p++ = '\0';
        plain_len += authcid_len + 1;

        char *passwd = strtok(NULL, "");

        if (passwd == NULL) {
            return;
        }

        size_t passwd_len = strnlen(passwd, 255);
        memcpy(p, passwd, passwd_len);
        plain_len += passwd_len;

        base64_encode(ctx->auth, plain, plain_len);
        memzero(plain, sizeof(plain));
    } else {
        safecpy(ctx->auth, token, sizeof(ctx->auth));
    }
}

/**
 * config_apply_env() - Apply environment variable to configuration
 * @ctx: IRC context structure (unused)
 * @env_name: Name of the environment variable to read
 * @dest: Destination buffer for the value
 * @dest_size: Size of the destination buffer
 *
 * Reads an environment variable and copies its value to the destination
 * buffer if the variable exists and is non-empty.
 *
 * Return: 0 on success or if variable doesn't exist, error code from safecpy
 */
static int config_apply_env(struct kirc_context *ctx, const char *env_name, 
        char *dest, size_t dest_size)
{
    char *env = getenv(env_name);
    if (env && *env) {
        return safecpy(dest, env, dest_size);
    }
    return 0;
}

/**
 * config_init() - Initialize configuration with defaults and environment
 * @ctx: IRC context structure to initialize
 *
 * Initializes the configuration context with default values and applies
 * settings from environment variables (KIRC_SERVER, KIRC_PORT, KIRC_CHANNELS,
 * KIRC_REALNAME, KIRC_USERNAME, KIRC_PASSWORD, KIRC_AUTH). Validates port
 * numbers and parses authentication mechanisms.
 *
 * Return: 0 on success, -1 if port validation fails
 */
int config_init(struct kirc_context *ctx)
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

/**
 * config_parse_args() - Parse command-line arguments
 * @ctx: IRC context structure to populate with parsed values
 * @argc: Argument count
 * @argv: Argument vector
 *
 * Parses command-line options using getopt. Supports:
 *   -s server, -p port, -r realname, -u username, -k password,
 *   -c channels, -a auth_mechanism
 * The nickname is required as a positional argument.
 *
 * Return: 0 on success, -1 on error or invalid arguments
 */
int config_parse_args(struct kirc_context *ctx, int argc, char *argv[])
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

/**
 * config_free() - Securely clear sensitive configuration data
 * @ctx: IRC context structure to clean up
 *
 * Securely zeroes out sensitive fields (auth token and password) from the
 * configuration context to prevent them from remaining in memory.
 *
 * Return: 0 on success, -1 if secure clearing fails
 */
int config_free(struct kirc_context *ctx)
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
