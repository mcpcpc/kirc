#include "editor.h"
#include "log.h"
#include "network.h"
#include "parse.h"
#include "render.h"
#include "terminal.h"
#include "utf8.h"

void kirc_init(kirc_context_t *ctx)
{
}

void kirc_run(kirc_context_t *ctx)
{
}

void kirc_shutdown(kirc_context_t *ctx)
{
}

int main(int argc, char *argv[])
{
    kirc_context_t ctx;

    kirc_init(&ctx);
    kirc_run(&ctx);
    kirc_shutdown(&ctx);
    
    return 0;
}
