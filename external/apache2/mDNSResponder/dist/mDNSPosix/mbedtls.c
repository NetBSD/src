/*
 * Copyright (c) 2019-2021 Apple Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * This file contains a TLS Shim that allows mDNSPosix to use mbedtls to do TLS session
 * establishment and also to accept TLS connections.
 */

#include "mDNSEmbeddedAPI.h"           // Defines the interface provided to the client layer above
#include "DNSCommon.h"
#include "mDNSPosix.h"               // Defines the specific types needed to run mDNS on this platform
#include "PlatformCommon.h"

#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#include <mbedtls/error.h>
#include <mbedtls/pk.h>
#include <mbedtls/ecp.h>
#include <mbedtls/ecdsa.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/sha256.h>
#include <mbedtls/base64.h>

#include <mbedtls/certs.h>
#include <mbedtls/x509.h>
#include <mbedtls/ssl.h>
#include <mbedtls/config.h>

// Posix TLS server context
struct TLSContext_struct {
    mbedtls_ssl_context context;
};

struct TLSServerContext_struct {
    mbedtls_x509_crt cert;
    mbedtls_pk_context key;
    mbedtls_x509_crt cacert;
    mbedtls_ssl_config config;
};

// Context that is shared amongs all TLS connections, regardless of which server cert/key is in use.
static mbedtls_entropy_context entropy;
static mbedtls_ctr_drbg_context ctr_drbg;

mDNSBool
mDNSPosixTLSInit(void)
{
    int status;

    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);

    status = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, NULL, 0);
    if (status != 0) {
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_ERROR, "Unable to seed RNG: %x", -status);
        return mDNSfalse;
    }
    return mDNStrue;
}

void
mDNSPosixTLSContextFree(TLSContext *tls)
{
    mbedtls_ssl_free(&tls->context);
    mDNSPlatformMemFree(tls);
}

TLSContext *
mDNSPosixTLSClientStateCreate(TCPSocket *sock)
{
    int status;
    TLSContext *tls;
    mbedtls_ssl_config config;

    status = mbedtls_ssl_config_defaults(&config, MBEDTLS_SSL_IS_CLIENT,
                                         MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
    if (status != 0) {
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_ERROR, "Unable to set ssl config defaults: %d", -status);
        return NULL;
    }
    mbedtls_ssl_conf_authmode(&config, (sock->flags & kTCPSocketFlags_TLSValidationNotRequired
                                        ? MBEDTLS_SSL_VERIFY_NONE
                                        : MBEDTLS_SSL_VERIFY_REQUIRED));

    tls = mDNSPlatformMemAllocateClear(sizeof(*tls));
    if (tls == mDNSNULL) {
        return tls;
    }

    status = mbedtls_ssl_setup(&tls->context, &config);
    if (status == 0) {
        if (sock->hostname) {
            char serverName[MAX_ESCAPED_DOMAIN_NAME];
            ConvertDomainNameToCString_withescape(sock->hostname, serverName, '\\');
            status = mbedtls_ssl_set_hostname(&tls->context, serverName);
        }
    }
    if (status != 0) {
        LogInfo("Unable to set up TLS listener state: %x", -status);
        mDNSPosixTLSContextFree(tls);
        return NULL;
    }
    return tls;
}

static int
tls_io_send(void *ctx, const unsigned char *buf, size_t len)
{
    ssize_t ret;
    TCPSocket *sock = ctx;
    ret = mDNSPosixWriteTCP(sock->events.fd, (const char *)buf, len);
    if (ret < 0) {
        if (errno == EAGAIN) {
            return MBEDTLS_ERR_SSL_WANT_WRITE;
        } else {
            return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
        }
    } else {
        return (int)ret;
    }
}

static int
tls_io_recv(void *ctx, unsigned char *buf, size_t max)
{
    ssize_t ret;
    TCPSocket *sock = ctx;
    mDNSBool closed = mDNSfalse;
    ret = mDNSPosixReadTCP(sock->events.fd, buf, max, &closed);
    if (ret < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            return MBEDTLS_ERR_SSL_WANT_READ;
        } else {
            return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
        }
    } else if (closed) {
        return MBEDTLS_ERR_SSL_CONN_EOF;
    } else {
        return (int)ret;
    }
}

mDNSBool
mDNSPosixTLSStart(TCPSocket *sock)
{
    int status;

    // Set up the I/O functions.
    mbedtls_ssl_set_bio(&sock->tls->context, sock, tls_io_send, tls_io_recv, NULL);

    // Start the TLS handshake
    status = mbedtls_ssl_handshake(&sock->tls->context);
    if (status != 0 && status != MBEDTLS_ERR_SSL_WANT_READ && status != MBEDTLS_ERR_SSL_WANT_WRITE) {
        LogInfo("TLS handshake failed: %x", -status);
        return mDNSfalse;
    }
    return mDNStrue;
}

TLSContext *
PosixTLSAccept(TCPListener *listenContext)
{
    int status;
    TLSContext *tls = mDNSPlatformMemAllocateClear(sizeof(*tls));

    if (tls == mDNSNULL) {
        return tls;
    }

    status = mbedtls_ssl_setup(&tls->context, &listenContext->tls->config);
    if (status != 0) {
        LogInfo("Unable to set up TLS listener state: %x", -status);
        mDNSPlatformMemFree(tls);
        return NULL;
    }
    return tls;
}

int
mDNSPosixTLSRead(TCPSocket *sock, void *buf, unsigned long buflen, mDNSBool *closed)
{
    int ret;

    // Shouldn't ever happen.
    if (!sock->tls) {
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_ERROR, "mDNSPosixTLSRead: called without TLS context!");
        *closed = mDNStrue;
        return 0;
    }

    ret = mbedtls_ssl_read(&sock->tls->context, buf, buflen);
    if (ret < 0) {
        switch (ret) {
        case MBEDTLS_ERR_SSL_WANT_READ:
            return 0;
        case MBEDTLS_ERR_SSL_WANT_WRITE:
            LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_ERROR, "Got SSL want write in TLS read!");
            // This means we got EWOULDBLOCK on a write operation.
            // Not implemented yet, but the right way to handle this is probably to
            // deselect read events until the socket is ready to write, then write,
            // and then re-enable read events.   What we don't want is to keep calling
            // read, because that will spin.
            return 0;
        case MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS:
            LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_ERROR, "Got async in progress in TLS read!");
            // No idea how to handle this yet.
            return 0;
#ifdef MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS
        case MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS:
            LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_ERROR, "Got crypto in progress in TLS read!");
            // No idea how to handle this.
            return 0;
#endif
        default:
            LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_ERROR, "Unexpected response from SSL read: %x", -ret);
            return -1;
        }
    } else {
        // mbedtls returns 0 for EOF, just like read(), but we need a different signal,
        // so we treat 0 as an error (for now).   In principle, we should get a notification
        // when the remote end is done writing, so a clean close should be different than
        // an abrupt close.
        if (ret == 0) {
            if (closed) {
                *closed = mDNStrue;
            }
            return -1;
        }
        return ret;
    }
}

int
mDNSPosixTLSWrite(TCPSocket *sock, const void *buf, unsigned long buflen)
{
    int ret;
    ret = mbedtls_ssl_write(&sock->tls->context, buf, buflen);
    if (ret < 0) {
        switch (ret) {
        case MBEDTLS_ERR_SSL_WANT_READ:
            return 0;
        case MBEDTLS_ERR_SSL_WANT_WRITE:
            LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_ERROR, "Got SSL want write in TLS read!");
            return 0;
        case MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS:
            LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_ERROR, "Got async in progress in TLS read!");
            return 0;
#ifdef MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS
        case MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS:
            LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_ERROR, "Got crypto in progress in TLS read!");
            return 0;
#endif
        default:
            LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_ERROR, "Unexpected response from SSL read: %x", -ret);
            return -1;
        }
    }
    return ret;
}

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 108
// indent-tabs-mode: nil
// End:
