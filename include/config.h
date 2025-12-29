/*
 * config.h
 * Header for configuration module
 * Author: Michael Czigler
 * License: MIT
 */

#ifndef __KIRC_CONFIG_H
#define __KIRC_CONFIG_H

#include "kirc.h"
#include "helper.h"

/* Initialize context with defaults and environment variables */
int config_init(kirc_context_t *ctx);

/* Parse command-line arguments */
int config_parse_args(kirc_context_t *ctx, int argc, char *argv[]);

/* Free sensitive data securely */
int config_free(kirc_context_t *ctx);

#endif  // __KIRC_CONFIG_H
