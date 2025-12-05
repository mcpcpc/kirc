#include "kirc.h"

static int kirc_init(kirc_context_t *ctx)
{
    memset(ctx, 0, sizeof(*ctx));

    ctx->family_hint = AF_UNSPEC;

    const char *env;
    
    return 0;
}

int main(int argc, char *argv[])
{
    kirc_context_t ctx;
    kirc_error_t err = KIRC_OK;

    kirc_init(&ctx);
    kirc_free(&ctx);
    
    return (err == KIRC_OK) ? 0 : 1;
}
