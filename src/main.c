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
            ctx->nickname = optarg;
            break;

        case 'r':  /* realname */
            ctx->realname = optarg;
            break;

        case 'u':  /* username */
            ctx->username = optarg;
            break;

        case 'k':  /* password */
            ctx->password = optarg;
            break;

        case 'a':  /* token */
            ctx->token = optarg;
            break

        case 'l':  /* log file path */
            ctx->log = optarg;
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
