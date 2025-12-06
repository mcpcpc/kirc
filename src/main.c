#include "network.h"

static int kirc_init(kirc_t *ctx)
{
    memset(ctx, 0, sizeof(*ctx));

    size_t hostname_n = sizeof(ctx->hostname) - 1;
    strncpy(ctx->hostname, "irc.libera.chat", hostname_n);

    size_t port_n = sizeof(ctx->port) - 1;
    strncpy(ctx->port, "6667", port_n);

    return 0;
}

static int kirc_args(kirc_t *ctx, int *argc, char **argv[])
{
    if (argc < 2) {
        fprintf(stderr, "%s: no arguments\n", argv[0]);
        return -1;
    }

    int opt;

    while ((opt = getopt(argc, argv, "")) > 0) {
        switch (opt) {
        case 's':  /* hostname */
            size_t hostname_n = sizeof(ctx->hostname) - 1;
            strncpy(ctx->hostname, optarg, hostname_n);
            break;

        case 'p':  /* port */
            size_t port_n = sizeof(ctx->port) - 1;
            strncpy(ctx->port, optarg, port_n);
            break;

        case 'n':  /* nickname */
            size_t nickname_n = sizeof(ctx->nickname) - 1;
            strncpy(ctx->nickname, optarg, nickname_n);
            break;

        case 'r':  /* realname */
            size_t realname_n = sizeof(ctx->realname) - 1;
            strncpy(ctx->realname, optarg, realname_n);
            break;

        case 'u':  /* username */
            size_t username_n = sizeof(ctx->username) - 1;
            strncpy(ctx->username, optarg, username_n);
            break;

        case 'k':  /* password */
            size_t password_n = sizeof(ctx->password) - 1;
            strncpy(ctx->password, optarg, password_n);
            break;

        case 'l':  /* log file path */
            size_t log_n = sizeof(ctx->log) - 1;
            strncpy(ctx->log, optarg, log_n);
            break

        case ':':
            fprintf(stderr, "%s: missing -%d value\n",
                argv[0], opt);
            return -1;

        case '?':
            fprintf(stderr, "%s: unknown -%d argument\n",
                argv[0], opt);
            return -1;

        default:
            return -1;
        }
    }

    return 0;
}

static int kirc_run(kirc_t *ctx)
{
    return 0;
}

int main(int argc, char *argv[])
{
    kirc_t ctx;

    if (kirc_init(&ctx) < 0) {
        return 1;
    }

    if (kirc_args(&ctx, &argc, &argv) < 0) {
        return 1;
    }

    if (kirc_run(&ctx) < 0) {
        return 1;
    }

    return 0;
}
