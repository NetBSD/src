/* tls-mbedtls.c
 *
 * Copyright (c) 2019-2021 Apple Computer, Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * DNS SIG(0) signature verification for DNSSD SRP using mbedtls.
 *
 * Provides functions for generating a public key validating context based on SIG(0) KEY RR data, and
 * validating a signature using a context generated with that public key.  Currently only ECDSASHA256 is
 * supported.
 */

#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#include "srp.h"
#define SRP_CRYPTO_MBEDTLS_INTERNAL
#include "dns-msg.h"
#include "srp-crypto.h"
#include "ioloop.h"
#include "srp-tls.h"

// Context that is shared amongs all TLS connections, regardless of which server cert/key is in use.
mbedtls_entropy_context entropy;
mbedtls_ctr_drbg_context ctr_drbg;

// For now, assume that we are using just one key and one server cert, plus the ca cert.  Consequently, we
// can treat this as global state.  If wanted later, we could make this its own structure.
mbedtls_x509_crt cacert_struct, *cacert = NULL;
mbedtls_x509_crt srvcert_struct, *srvcert = NULL;
mbedtls_pk_context srvkey;
mbedtls_ssl_config tls_server_config;
mbedtls_ssl_config tls_client_config;
mbedtls_ssl_config tls_opportunistic_config;

bool
srp_tls_init(void)
{
    int status;

    // Initialize the shared data structures.
    mbedtls_x509_crt_init(&srvcert_struct);
    mbedtls_pk_init(&srvkey);
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);

    status = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, NULL, 0);
    if (status != 0) {
        ERROR("Unable to seed RNG: %x", -status);
        return false;
    }
    return true;
}

static bool
mbedtls_config_init(mbedtls_ssl_config *config, int flags)
{
    int status = mbedtls_ssl_config_defaults(config, flags,
                                             MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
    if (status != 0) {
        ERROR("Unable to set up default TLS config state: %x", -status);
        return false;
    }

    mbedtls_ssl_conf_rng(config, mbedtls_ctr_drbg_random, &ctr_drbg);
    return true;
}

void
srp_tls_configure(void *const NULLABLE context)
{
}

bool
srp_tls_client_init(void)
{
    if (!mbedtls_config_init(&tls_client_config, MBEDTLS_SSL_IS_CLIENT)) {
        return false;
    }
    if (!mbedtls_config_init(&tls_opportunistic_config, MBEDTLS_SSL_IS_CLIENT)) {
        return false;
    }
    mbedtls_ssl_conf_authmode(&tls_opportunistic_config, MBEDTLS_SSL_VERIFY_OPTIONAL);
    return true;
}

bool
srp_tls_server_init(const char *cacert_file, const char *srvcert_file, const char *server_key_file)
{
    int status;

    // Load the public key and cert
    if (cacert_file != NULL) {
        status = mbedtls_x509_crt_parse_file(&cacert_struct, cacert_file);
        if (status != 0) {
            ERROR("Unable to parse ca cert file: %x", -status);
            return false;
        }
        cacert = &cacert_struct;
    }

    if (srvcert_file != NULL) {
        status = mbedtls_x509_crt_parse_file(&srvcert_struct, srvcert_file);
        if (status != 0) {
            ERROR("Unable to parse server cert file: %x", -status);
            return false;
        }
        srvcert = &srvcert_struct;
        if (srvcert_struct.next && cacert != NULL) {
            cacert = srvcert_struct.next;
        }
    }

    if (server_key_file != NULL) {
        status = mbedtls_pk_parse_keyfile(&srvkey, server_key_file, NULL);
        if (status != 0) {
            ERROR("Unable to parse server cert file: %x", -status);
            return false;
        }
    }

    if (!mbedtls_config_init(&tls_server_config, MBEDTLS_SSL_IS_SERVER)) {
        return false;
    }

    if (cacert != NULL) {
        mbedtls_ssl_conf_ca_chain(&tls_server_config, cacert, NULL);
    }

    status = mbedtls_ssl_conf_own_cert(&tls_server_config, srvcert, &srvkey);
    if (status != 0) {
        ERROR("Unable to configure own cert: %x", -status);
        return false;
    }
    return true;
}

static int
srp_tls_io_send(void *ctx, const unsigned char *buf, size_t len)
{
    ssize_t ret;
    comm_t *comm = ctx;
    ret = write(comm->io.fd, buf, len);
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
srp_tls_io_recv(void *ctx, unsigned char *buf, size_t max)
{
    ssize_t ret;
    comm_t *comm = ctx;
    ret = read(comm->io.fd, buf, max);
    if (ret < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            return MBEDTLS_ERR_SSL_WANT_READ;
        } else {
            return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
        }
    } else if (ret == 0) {
        return MBEDTLS_ERR_SSL_CONN_EOF;
    } else {
        return (int)ret;
    }
}

bool
srp_tls_listen_callback(comm_t *comm)
{
    int status;

    // Allocate the TLS config and state structures.
    comm->tls_context = calloc(1, sizeof *comm->tls_context);
    if (comm->tls_context == NULL) {
        return false;
    }
    status = mbedtls_ssl_setup(&comm->tls_context->context, &tls_server_config);
    if (status != 0) {
        ERROR("Unable to set up TLS listener state: %x", -status);
        return false;
    }

    // Set up the I/O functions.
    mbedtls_ssl_set_bio(&comm->tls_context->context, comm, srp_tls_io_send, srp_tls_io_recv, NULL);

    // Start the TLS handshake.
    status = mbedtls_ssl_handshake(&comm->tls_context->context);
    if (status != 0 && status != MBEDTLS_ERR_SSL_WANT_READ && status != MBEDTLS_ERR_SSL_WANT_WRITE) {
        ERROR("TLS handshake failed: %x", -status);
        srp_tls_context_free(comm);
        ioloop_close(&comm->io);
    }
    return true;
}

bool
srp_tls_connect_callback(comm_t *comm)
{
    int status;
    mbedtls_ssl_config *config = comm->opportunistic ? &tls_opportunistic_config : &tls_client_config;
    // Allocate the TLS config and state structures.
    comm->tls_context = calloc(1, sizeof *comm->tls_context);
    if (comm->tls_context == NULL) {
        return false;
    }
    status = mbedtls_ssl_setup(&comm->tls_context->context, config);
    if (status != 0) {
        ERROR("Unable to set up TLS connect state: %x", -status);
        return false;
    }

    // Set up the I/O functions.
    mbedtls_ssl_set_bio(&comm->tls_context->context, comm, srp_tls_io_send, srp_tls_io_recv, NULL);

    // Start the TLS handshake.
    status = mbedtls_ssl_handshake(&comm->tls_context->context);
    if (status != 0 && status != MBEDTLS_ERR_SSL_WANT_READ && status != MBEDTLS_ERR_SSL_WANT_WRITE) {
        ERROR("TLS handshake failed: %x", -status);
        srp_tls_context_free(comm);
        return false;
    }
    if (status == MBEDTLS_ERR_SSL_WANT_READ) {
        comm->tls_handshake_incomplete = true;
    }
    INFO(PRI_S_SRP ": TLS handshake progress %d", comm->name, -status);
    return true;
}

ssize_t
srp_tls_read(comm_t *comm, unsigned char *buf, size_t max)
{
    // If we aren't done with the TLS handshake, continue it.
    if (comm->tls_handshake_incomplete) {
        int status = mbedtls_ssl_handshake(&comm->tls_context->context);
        if (status != 0 && status != MBEDTLS_ERR_SSL_WANT_READ && status != MBEDTLS_ERR_SSL_WANT_WRITE) {
            ERROR("TLS handshake failed: %x", -status);
            srp_tls_context_free(comm);
            return -1;
        }
        if (status == 0) {
            comm->tls_handshake_incomplete = false;
            comm->connected(comm, comm->context);
        }
        INFO(PRI_S_SRP ": TLS handshake progress %d", comm->name, -status);
        return 0;
    }

    // Otherwise, read application data.
    int ret = mbedtls_ssl_read(&comm->tls_context->context, buf, max);
    if (ret < 0) {
        switch (ret) {
        case MBEDTLS_ERR_SSL_WANT_READ:
            return 0;
        case MBEDTLS_ERR_SSL_WANT_WRITE:
            ERROR("Got SSL want write in TLS read!");
            // This means we got EWOULDBLOCK on a write operation.
            // Not implemented yet, but the right way to handle this is probably to
            // deselect read events until the socket is ready to write, then write,
            // and then re-enable read events.   What we don't want is to keep calling
            // read, because that will spin.
            return 0;
        case MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS:
            ERROR("Got async in progress in TLS read!");
            // No idea how to handle this yet.
            return 0;
#ifdef MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS
        case MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS:
            ERROR("Got crypto in progress in TLS read!");
            // No idea how to handle this.
            return 0;
#endif
        default:
            ERROR("Unexpected response from SSL read: %x", -ret);
            return -1;
        }
    } else {
        // mbedtls returns 0 for EOF, just like read(), but we need a different signal,
        // so we treat 0 as an error (for now).   In principle, we should get a notification
        // when the remote end is done writing, so a clean close should be different than
        // an abrupt close.
        if (ret == 0) {
            ERROR("mbedtls_ssl_read returned zero.");
            return -1;
        }
        return ret;
    }
}

void
srp_tls_context_free(comm_t *comm)
{
    // Free any state that the TLS library allocated
    mbedtls_ssl_free(&comm->tls_context->context);
    // Free and forget the context data structure
    free(comm->tls_context);
    comm->tls_context = 0;
}

ssize_t
srp_tls_write(comm_t *comm, struct iovec *iov, int iov_len)
{
    int ret;
    int i;
    int bytes_written = 0;
    for (i = 0; i < iov_len; i++) {
        ret = mbedtls_ssl_write(&comm->tls_context->context, iov[i].iov_base, iov[i].iov_len);
        if (ret < 0) {
            switch (ret) {
            case MBEDTLS_ERR_SSL_WANT_READ:
                return bytes_written;
            case MBEDTLS_ERR_SSL_WANT_WRITE:
                ERROR("Got SSL want write in TLS read!");
                return bytes_written;
            case MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS:
                ERROR("Got async in progress in TLS read!");
                return bytes_written;
#ifdef MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS
            case MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS:
                ERROR("Got crypto in progress in TLS read!");
                return bytes_written;
#endif
            default:
                ERROR("Unexpected response from SSL read: %x", -ret);
                return -1;
            }
        } else if (ret != iov[i].iov_len) {
            return bytes_written + ret;
        } else {
            bytes_written += ret;
        }
    }
    return bytes_written;
}

// Dummy function for now; should eventually fetch the TLS context to use for validating
// a cert presented by a remote connection.
void
configure_tls(void *const NULLABLE UNUSED context)
{
}

void
schedule_tls_certificate_rotation(wakeup_t **const UNUSED tls_listener_wakeup,
	comm_t *const UNUSED tls_listener_to_rotate)
{
    ;
}

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 108
// indent-tabs-mode: nil
// End:
