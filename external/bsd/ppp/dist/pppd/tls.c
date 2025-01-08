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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509v3.h>

#include "pppd-private.h"
#include "tls.h"

/**
 * Structure used in verifying the peer certificate
 */
struct tls_info
{
    char *peer_name;
    X509 *peer_cert;
    bool client;
};


#if OPENSSL_VERSION_NUMBER < 0x10100000L

/*
 *  OpenSSL 1.1+ introduced a generic TLS_method()
 *  For older releases we substitute the appropriate method
 */
#define TLS_method SSLv23_method

#ifndef SSL_CTX_set_max_proto_version
/** Mimics SSL_CTX_set_max_proto_version for OpenSSL < 1.1 */
static inline int SSL_CTX_set_max_proto_version(SSL_CTX *ctx, long tls_ver_max)
{
    long sslopt = 0;

    if (tls_ver_max < TLS1_VERSION)
    {
        sslopt |= SSL_OP_NO_TLSv1;
    }
#ifdef SSL_OP_NO_TLSv1_1
    if (tls_ver_max < TLS1_1_VERSION)
    {
        sslopt |= SSL_OP_NO_TLSv1_1;
    }
#endif
#ifdef SSL_OP_NO_TLSv1_2
    if (tls_ver_max < TLS1_2_VERSION)
    {
        sslopt |= SSL_OP_NO_TLSv1_2;
    }
#endif
    SSL_CTX_set_options(ctx, sslopt);

    return 1;
}
#endif /* SSL_CTX_set_max_proto_version */

#endif /* OPENSSL_VERSION_NUMBER < 0x10100000L */


/*
 * Verify a certificate. Most of the work (signatures and issuer attributes checking)
 * is done by ssl; we check the CN in the peer certificate against the peer name.
 */
static int tls_verify_callback(int ok, X509_STORE_CTX *ctx)
{
    char subject[256];
    char cn_str[256];
    X509 *peer_cert;
    int err, depth;
    SSL *ssl;
    struct tls_info *inf;
    char *ptr1 = NULL, *ptr2 = NULL;

    peer_cert = X509_STORE_CTX_get_current_cert(ctx);
    err = X509_STORE_CTX_get_error(ctx);
    depth = X509_STORE_CTX_get_error_depth(ctx);

    dbglog("certificate verify depth: %d", depth);

    if (auth_required && !ok) {
        X509_NAME_oneline(X509_get_subject_name(peer_cert),
                  subject, 256);

        X509_NAME_get_text_by_NID(X509_get_subject_name(peer_cert),
                      NID_commonName, cn_str, 256);

        dbglog("Certificate verification error:\n depth: %d CN: %s"
               "\n err: %d (%s)\n", depth, cn_str, err,
               X509_verify_cert_error_string(err));

        return 0;
    }

    ssl = X509_STORE_CTX_get_ex_data(ctx,
                       SSL_get_ex_data_X509_STORE_CTX_idx());

    inf = (struct tls_info*) SSL_get_ex_data(ssl, 0);
    if (inf == NULL) {
        error("Error: SSL_get_ex_data returned NULL");
        return 0;
    }

    tls_log_sslerr();

    if (!depth) 
    {
        /* Verify certificate based on certificate type and extended key usage */
        if (tls_verify_key_usage) {
            int purpose = inf->client ? X509_PURPOSE_SSL_SERVER : X509_PURPOSE_SSL_CLIENT ;
            if (X509_check_purpose(peer_cert, purpose, 0) == 0) {
                error("Certificate verification error: nsCertType mismatch");
                return 0;
            }

#if OPENSSL_VERSION_NUMBER >= 0x10100000L
            int flags = inf->client ? XKU_SSL_SERVER : XKU_SSL_CLIENT;
            if (!(X509_get_extended_key_usage(peer_cert) & flags)) {
                error("Certificate verification error: invalid extended key usage");
                return 0;
            }
#endif
            info("Certificate key usage: OK");
        }

        /*
         * If acting as client and the name of the server wasn't specified
         * explicitely, we can't verify the server authenticity 
         */
        if (!tls_verify_method)
            tls_verify_method = TLS_VERIFY_NONE;

        if (!inf->peer_name || !strcmp(TLS_VERIFY_NONE, tls_verify_method)) {
            warn("Certificate verication disabled or no peer name was specified");
            return ok;
        }

        /* This is the peer certificate */
        X509_NAME_oneline(X509_get_subject_name(peer_cert),
                  subject, 256);

        X509_NAME_get_text_by_NID(X509_get_subject_name(peer_cert),
                      NID_commonName, cn_str, 256);

        /* Verify based on subject name */
        ptr1 = inf->peer_name;
        if (!strcmp(TLS_VERIFY_SUBJECT, tls_verify_method)) {
            ptr2 = subject;
        }

        /* Verify based on common name (default) */
        if (strlen(tls_verify_method) == 0 ||
            !strcmp(TLS_VERIFY_NAME, tls_verify_method)) {
            ptr2 = cn_str;
        }

        /* Match the suffix of common name */
        if (!strcmp(TLS_VERIFY_SUFFIX, tls_verify_method)) {
            int len = strlen(ptr1);
            int off = strlen(cn_str) - len;
            ptr2 = cn_str;
            if (off > 0) {
                ptr2 = cn_str + off;
            }
        }

        if (strcmp(ptr1, ptr2)) {
            error("Certificate verification error: CN (%s) != %s", ptr1, ptr2);
            return 0;
        }

        if (inf->peer_cert) { 
            if (X509_cmp(inf->peer_cert, peer_cert) != 0) {
                error("Peer certificate doesn't match stored certificate");
                return 0;
            }
        }

        info("Certificate CN: %s, peer name %s", cn_str, inf->peer_name);
    }

    return ok;
}

int tls_init()
{
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    SSL_library_init();
    SSL_load_error_strings();
#endif
    return 0;
}

int tls_set_verify(SSL_CTX *ctx, int depth) 
{
    SSL_CTX_set_verify_depth(ctx, depth);
    SSL_CTX_set_verify(ctx,
               SSL_VERIFY_PEER |
               SSL_VERIFY_FAIL_IF_NO_PEER_CERT,
               &tls_verify_callback);
    return 0;
}

int tls_set_verify_info(SSL *ssl, const char *peer_name, const char *peer_cert, 
        bool client, struct tls_info **out)
{
    if (out != NULL) {
        struct tls_info *tmp = calloc(1, sizeof(struct tls_info));
        if (!tmp) {
            fatal("Allocation error");
        }

        tmp->client = client;
        if (peer_name) {
            tmp->peer_name = strdup(peer_name);
        }

        if (peer_cert && strlen(peer_cert) > 0) {
            FILE *fp = fopen(peer_cert, "r");
            if (fp) {
                tmp->peer_cert = PEM_read_X509(fp, NULL, NULL, NULL);
                fclose(fp);
            }

            if (!tmp->peer_cert) {
                error("EAP-TLS: Error loading client certificate from file %s",
                     peer_cert);
                tls_free_verify_info(&tmp);
                return -1;
            }
        }

        SSL_set_ex_data(ssl, 0, tmp);
        *out = tmp;
        return 0;
    }

    return -1;
}

void tls_free_verify_info(struct tls_info **in) {
    if (in && *in) {
        struct tls_info *tmp = *in;
        if (tmp->peer_name) {
            free(tmp->peer_name);
        }
        if (tmp->peer_cert) {
            X509_free(tmp->peer_cert);
        }
        free(tmp);
        *in = NULL;
    }
}

const SSL_METHOD* tls_method() {
    return TLS_method();
}

int tls_set_version(SSL_CTX *ctx, const char *max_version)
{
#if defined(TLS1_2_VERSION)
    long tls_version = TLS1_2_VERSION; 
#elif defined(TLS1_1_VERSION)
    long tls_version = TLS1_1_VERSION; 
#else
    long tls_version = TLS1_VERSION; 
#endif

    /* As EAP-TLS+TLSv1.3 is highly experimental we offer the user a chance to override */
    if (max_version) {
        if (strncmp(max_version, "1.0", 3) == 0) {
            tls_version = TLS1_VERSION;
        }
        else if (strncmp(max_version, "1.1", 3) == 0) {
            tls_version = TLS1_1_VERSION;
        }
        else if (strncmp(max_version, "1.2", 3) == 0) {
#ifdef TLS1_2_VERSION
            tls_version = TLS1_2_VERSION;
#else
            warn("TLSv1.2 not available. Defaulting to TLSv1.1");
            tls_version = TLS_1_1_VERSION;
#endif
        }
        else if (strncmp(max_version, "1.3", 3) == 0) {
#ifdef TLS1_3_VERSION
            tls_version = TLS1_3_VERSION;
#else
            warn("TLSv1.3 not available.");
#endif
        }
    }

    dbglog("Setting max protocol version to 0x%X", tls_version);
    if (!SSL_CTX_set_max_proto_version(ctx, tls_version)) {
        error("Could not set max protocol version");
        return -1;
    }

    return 0;
}

int tls_set_opts(SSL_CTX *ctx) {
    
    /* Explicitly set the NO_TICKETS flag to support Win7/Win8 clients */
    SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3
#ifdef SSL_OP_NO_TICKET
    | SSL_OP_NO_TICKET
#endif
    | SSL_OP_NO_COMPRESSION
    );

    /* OpenSSL 1.1.1+ does not include RC4 ciphers by default.
     * This causes totally obsolete WinXP clients to fail. If you really
     * need ppp+EAP-TLS+openssl 1.1.1+WinXP then enable RC4 cipers and
     * make sure that you use an OpenSSL that supports them

    SSL_CTX_set_cipher_list(ctx, "RC4");
    */
    return 0;
}

int tls_set_crl(SSL_CTX *ctx, const char *crl_dir, const char *crl_file) 
{
    X509_STORE  *certstore = NULL;
    X509_LOOKUP *lookup = NULL;
    FILE *fp = NULL;
    int status = -1;

    if (crl_dir) {
        if (!(certstore = SSL_CTX_get_cert_store(ctx))) {
            error("Failed to get certificate store");
            goto done;
        }

        if (!(lookup =
             X509_STORE_add_lookup(certstore, X509_LOOKUP_hash_dir()))) {
            error("Store lookup for CRL failed");
            goto done;
        }

        X509_LOOKUP_add_dir(lookup, crl_dir, X509_FILETYPE_PEM);
        X509_STORE_set_flags(certstore, X509_V_FLAG_CRL_CHECK);
    }

    if (crl_file) {
        X509_CRL *crl = NULL;

        fp = fopen(crl_file, "r");
        if (!fp) {
            error("Cannot open CRL file '%s'", crl_file);
            goto done;
        }

        crl = PEM_read_X509_CRL(fp, NULL, NULL, NULL);
        if (!crl) {
            error("Cannot read CRL file '%s'", crl_file);
            goto done;
        }

        if (!(certstore = SSL_CTX_get_cert_store(ctx))) {
            error("Failed to get certificate store");
            goto done;
        }
        if (!X509_STORE_add_crl(certstore, crl)) {
            error("Cannot add CRL to certificate store");
            goto done;
        }
        X509_STORE_set_flags(certstore, X509_V_FLAG_CRL_CHECK);
    }

    status = 0;

done: 

    if (fp != NULL) {
        fclose(fp);
    }

    return status;
}

int tls_set_ca(SSL_CTX *ctx, const char *ca_dir, const char *ca_file) 
{
    if (ca_file && strlen(ca_file) == 0) {
        ca_file = NULL;
    }

    if (ca_dir && strlen(ca_dir) == 0) {
        ca_dir = NULL;
    }

    if (!SSL_CTX_load_verify_locations(ctx, ca_file, ca_dir)) {

        error("Cannot load verify locations");
        if (ca_file) {
            dbglog("CA certificate file = [%s]", ca_file);
        }

        if (ca_dir) {
            dbglog("CA certificate path = [%s]", ca_dir);
        }

        return -1;
    }

    return 0;
}

void tls_log_sslerr( void )
{
    unsigned long ssl_err = ERR_get_error();

    if (ssl_err != 0)
        dbglog("EAP-TLS SSL error stack:");
    while (ssl_err != 0) {
        dbglog( ERR_error_string( ssl_err, NULL ) );
        ssl_err = ERR_get_error();
    }
}

