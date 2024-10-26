/* srp-tls.h
 *
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
 * TLS Shim definitions.   These entry points should in principle work for any TLS
 * library, with the addition of a single shim file, for example tls-mbedtls.c.
 */

#ifndef __SRP_TLS_H
#define __SRP_TLS_H
// Anonymous key structure, depends on the target.
typedef struct srp_key srp_key_t;

#ifdef SRP_CRYPTO_MBEDTLS_INTERNAL
#include <mbedtls/certs.h>
#include <mbedtls/x509.h>
#include <mbedtls/ssl.h>

struct tls_context {
    struct mbedtls_ssl_context context;
    enum { handshake_in_progress, handshake_complete } state;
};
#endif // SRP_CRYPTO_MBEDTLS_INTERNAL
#ifdef SRP_CRYPTO_MACOS
#include "tls-macos.h"
#endif

// tls_*.c:
bool srp_tls_init(void);
bool srp_tls_client_init(void);
bool srp_tls_server_init(const char *NULLABLE cacert_file,
			 const char *NULLABLE srvcrt_file, const char *NULLABLE server_key_file);
bool srp_tls_accept_setup(comm_t *NONNULL comm);
bool srp_tls_listen_callback(comm_t *NONNULL comm);
bool srp_tls_connect_callback(comm_t *NONNULL comm);
ssize_t srp_tls_read(comm_t *NONNULL comm, unsigned char *NONNULL buf, size_t max);
void srp_tls_context_free(comm_t *NONNULL comm);
ssize_t srp_tls_write(comm_t *NONNULL comm, struct iovec *NONNULL iov, int iov_len);

/*!
 *  @brief
 *      Configure TLS with the identity.
 *
 *  @param context
 *      Context passed to this function to let it finish the configuration process.
 *
 *  @discussion
 *      This function can only be called after srp_tls_init() is called to fetch the necessary identity.
 *
 */
void
srp_tls_configure(void *NULLABLE context);

/*!
 *  @brief
 *      Gets the remaining valid time for the TLS certificate that is initialized by <code>srp_tls_init()</code>.
 *
 *  @result
 *      The remaining time valid time before TLS certificate expires.
 *
 *  @discussion
 *      <code>srp_tls_init()</code> has to be called to initialize the TLS certificate before calling this function.
 */
uint32_t
srp_tls_get_next_rotation_time(void);

/*!
 *  @brief
 *      Destroy the TLS certificate that is initialized by <code>srp_tls_init()</code>.
 *
 *  @discussion
 *      <code>srp_tls_init()</code> has to be called to initialize the TLS certificate before calling this function.
 */
void
srp_tls_dispose(void);

/*!
 *  @brief
 *      Schedule a wake up event so that the TLS certificate that is expiring
 *      soon can be replaced with a newer one.
 *
 *  @param tls_listener_wakeup
 *      The pointer to the tls_listener_wakeup context that can be used to
 *      schedule a wakeup event.
 *
 *  @param tls_listener_to_rotate
 *      The TLS listener object created before that needs to be rotated.
 *
 *  @discussion
 *      The TLS rotation time is controlled by
 *      <code>TLS_CERTIFICATE_VALID_PERIOD_SECS</code>.
 */
void
schedule_tls_certificate_rotation(wakeup_t * NULLABLE * NONNULL tls_listener_wakeup,
	comm_t * NONNULL tls_listener_to_rotate);

#endif // __SRP_TLS_H

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 108
// indent-tabs-mode: nil
// End:
