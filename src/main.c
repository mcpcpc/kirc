#include "network.h"

int kirc_init(kirc_t *ctx)
{
    memset(ctx, 0, sizeof(*ctx));

    return 0;
}

int kirc_args(kirc_t *ctx, int argc, char *argv)
{
    if (argc < 2) {
        fprintf(stderr, "%s: no arguments\n", argv[0]);
        return -1;
    }

    int opt;

    while ((opt = getopt(argc, argv, "")) > 0) {
        switch (opt) {
        case 's':  /* hostname */
            ctx->hostname = optarg;
            break;

        case 'p':  /* port */
            ctx->port = optarg;
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
            fprintf(stderr, "%s: missing -%s value\n",
                argv[0], opt);
            return -1;

        case '?':
            fprintf(stderr, "%s: unknown -%s argument\n",
                argv[0], opt);
            return -1;

        default:
            return -1;
        }
    }

    return 0;
}

int kirc_run(kirc_t *ctx)
{
    return 0;
}

int main(int argc, char *argv[])
{
    kirc_context_t ctx;

    if (kirc_init(&ctx) < 0) {
        return 1;
    }

    if (kirc_args(&ctx, argc, argv) < 0) {
        return 1;
    }

    if (kirc_run(&ctx) < 0) {
        return 1;
    }

    return 0;
}
