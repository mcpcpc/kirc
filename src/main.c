#include "network.h"

int kirc_init(kirc_t *ctx)
{
    return 0;
}

int kirc_args(kirc_t *ctx, int argc, char *argv)
{
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
