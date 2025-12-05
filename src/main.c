#include "kirc.h"

static int kirc_init(kirc_context_t *ctx)
{
    memset(ctx, 0, sizeof(*ctx));

    size_t hostname_n = sizeof(ctx->hostname) - 1;
    strncpy(ctx->hostname, "irc.libera.chat", hostname_n);

    size_t port_n = sizeof(ctx->port) - 1;
    strncpy(ctx->port, "6667", port_n);

    ctx->family_hint = AF_UNSPEC;

    const char *env;
    
    return 0;
}

int main(int argc, char *argv[])
{
    kirc_context_t ctx;
    kirc_error_t err = KIRC_OK;

    kirc_init(&ctx);
    
    return (err == KIRC_OK) ? 0 : 1;
}
