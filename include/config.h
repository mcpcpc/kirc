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

int config_init(struct kirc_context *ctx);
int config_parse_args(struct kirc_context *ctx, int argc, char *argv[]);
int config_free(struct kirc_context *ctx);

#endif  // __KIRC_CONFIG_H
