/*
 * config.h
 * Configuration parsing and initialization
 * Author: Michael Czigler
 * License: MIT
 */

#ifndef __KIRC_CONFIG_H
#define __KIRC_CONFIG_H

#include "kirc.h"

/* Initialize context with defaults and environment variables */
int config_init(kirc_context_t *ctx);

/* Parse command-line arguments */
int config_parse_args(kirc_context_t *ctx, int argc, char *argv[]);

/* Validate port number */
int config_validate_port(const char *value);

/* Parse comma-separated channel list */
void config_parse_channels(kirc_context_t *ctx, char *value);

/* Parse SASL authentication mechanism */
void config_parse_mechanism(kirc_context_t *ctx, char *value);

/* Free sensitive data securely */
int config_free(kirc_context_t *ctx);

#endif  // __KIRC_CONFIG_H
