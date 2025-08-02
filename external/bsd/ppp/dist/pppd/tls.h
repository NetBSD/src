/*
 * Copyright (c) 2021 Eivind Næss. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The name(s) of the authors of this software must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission.
 *
 * THE AUTHORS OF THIS SOFTWARE DISCLAIM ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef PPP_TLS_H
#define PPP_TLS_H

#include "pppdconf.h"

/**
 * Structure used in verifying the peer certificate
 */
struct tls_info;

/**
 * Initialize the SSL library
 */
int tls_init(void);

/**
 * Get the SSL_METHOD
 */
const SSL_METHOD* tls_method(void);

/**
 * Configure the SSL options
 */
int tls_set_opts(SSL_CTX *ctx);

/**
 * Configure the SSL context's max TLS version
 */
int tls_set_version(SSL_CTX *ctx, const char *max_version);

/** 
 * Configure the SSL context's verify callback
 */
int tls_set_verify(SSL_CTX *ctx, int depth);

/**
 * Configure the SSL verify information
 */
int tls_set_verify_info(SSL *ssl, const char *peer_name, const char *peer_cert_file, 
        bool client, struct tls_info **out);

/**
 * Free the tls_info structure and it's members
 */
void tls_free_verify_info(struct tls_info **in);

/**
 * Configure the SSL context's CRL details
 */
int tls_set_crl(SSL_CTX *ctx, const char *crl_dir, const char *crl_file);

/**
 * Configure the SSL context's CA verify locations
 */
int tls_set_ca(SSL_CTX *ctx, const char *ca_dir, const char *ca_file);

/**
 * Log all errors from ssl library
 */
void tls_log_sslerr( void );

#endif	/* PPP_TLS_H */
