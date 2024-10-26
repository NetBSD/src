/* srp-crypto.h
 *
 * Copyright (c) 2018-2021 Apple Computer, Inc. All rights reserved.
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
 * DNS SIG(0) signature generation for DNSSD SRP using mbedtls.
 *
 * Functions required for loading, saving, and generating public/private keypairs, extracting the public key
 * into KEY RR data, and computing signatures.
 */

#ifndef __SRP_CRYPTO_H
#define __SRP_CRYPTO_H

#include "srp.h"

// Anonymous key structure, depends on the target.
typedef struct srp_key srp_key_t;
typedef struct hmac_key hmac_key_t;
struct hmac_key {
    int algorithm;
    dns_name_t *NONNULL name;
    uint8_t *NONNULL secret;
    int length;
};

#define ECDSA_KEY_SIZE             64
#define ECDSA_KEY_PART_SIZE        32
#define ECDSA_SHA256_HASH_SIZE     32
#define ECDSA_SHA256_SIG_SIZE      64
#define ECDSA_SHA256_SIG_PART_SIZE 32

#define SIG_HEADERLEN    11
#define SIG_STATIC_RDLEN 18

#define dnssec_keytype_ecdsa 13

#define SRP_SHA256_DIGEST_SIZE 32
#define SRP_SHA256_BLOCK_SIZE  64
#define SRP_HMAC_TYPE_SHA256   1

#ifdef SRP_CRYPTO_MACOS_INTERNAL
#include <CoreFoundation/CoreFoundation.h>
#include <Security/Security.h>
// #include <Security/SecTransform.h>
#include <CoreServices/CoreServices.h>

struct srp_key {
    SecKeyRef NONNULL public;
    SecKeyRef NONNULL private;
};

// An ECDSASHA256 signature in ASN.1 DER format is 0x30 | x | 0x02 | y | r | 0x02 | z | s, where x is the
// length of the whole sequence (minus the first byte), y is the encoded length of r, and z is
// the encoded length of s.
       // type               offset in output buffer      sub-template    size of output buffer
       // ----               -----------------------      ------------    ---------------------
#define ECDSA_SIG_TEMPLATE(name)                                                                          \
    static const SecAsn1Template sig_template[] = {                                                       \
        { SEC_ASN1_SEQUENCE, 0,                                 NULL,     sizeof(raw_signature_data_t) }, \
        { SEC_ASN1_INTEGER,  offsetof(raw_signature_data_t, r), NULL,     0 },                            \
        { SEC_ASN1_INTEGER,  offsetof(raw_signature_data_t, s), NULL,     0 },                            \
        { 0,                 0,                           NULL,           0 }                             \
    };

#if !TARGET_OS_IPHONE && !TARGET_OS_TV && !TARGET_OS_WATCH
#endif // MACOS only
#endif // SRP_CRYPTO_MACOS_INTERNAL

#ifdef SRP_CRYPTO_MBEDTLS
#include <mbedtls/error.h>
#include <mbedtls/pk.h>
#include <mbedtls/md.h>
#include <mbedtls/ecp.h>
#include <mbedtls/ecdsa.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/sha256.h>
#include <mbedtls/base64.h>

// Works just fine with mbedtls.
#define KEYCOPY_WORKS 1

// The SRP key includes both the ecdsa key and the pseudo-random number generator context, so that we can
// use the PRNG for signing as well as generating keys.   The PRNG is seeded with a high-entropy data source.
// This structure assumes that we are just using this one key; if we want to support multiple keys then
// the entropy source and PRNG should be shared by all keys (of course, that's not thread-safe, so...)
struct srp_key {
    mbedtls_pk_context key;
};

// Uncomment the following line to print the data being feed into the hash operation for debugging purpose.
// #define DEBUG_SHA256
#ifdef DEBUG_SHA256
int srp_mbedtls_sha256_update_ret(const char *NONNULL thing_name,
                                  mbedtls_sha256_context *NONNULL sha, uint8_t *NONNULL message, size_t msglen);
int srp_mbedtls_sha256_finish_ret(mbedtls_sha256_context *NONNULL sha, uint8_t *NONNULL hash);
#else
#define srp_mbedtls_sha256_update_ret(name, ...) mbedtls_sha256_update_ret(__VA_ARGS__)
#define srp_mbedtls_sha256_finish_ret mbedtls_sha256_finish_ret
#endif // DEBUG_SHA256
#ifdef THREAD_DEVKIT_ADK
#define mbedtls_strerror(code, buf, bufsize) snprintf(buf, bufsize, "%d", (int)(code))
#endif

// The following entry points must be provided by the host for hosts that use mbedtls signing.

// The SRP host is expected to load the SRP-specific host key out of stable storage.
// If no key has previously been stored, this function must return kDNSServiceErr_NoSuchKey.
// If the key doesn't fit in the buffer, this function must return kDNSServiceErr_NoMemory.
// Otherwise, the function is expected to copy the key into the buffer and store the key length
// through the length pointer, and return kDNSServiceErr_NoError.
int srp_load_key_data(void *NULLABLE host_context, const char *NONNULL key_name,
                      uint8_t *NONNULL buffer, uint16_t *NONNULL length, uint16_t buffer_size);

// The SRP host is expected to store the SRP-specific host key in stable storage.
// If the key store fails, the server returns a relevant kDNSServiceErr_* error,
// such as kDNSServiceErr_NoMemory.  Otherwise, the function returns kDNSServiceErr_NoError.
// It is generally expected that storing the key will not fail--if it does fail, SRP can't
// function.
int srp_store_key_data(void *NULLABLE host_context, const char *NONNULL key_name, uint8_t *NONNULL buffer,
                       uint16_t length);

int srp_remove_key_file(void *NULLABLE host_context, const char *NONNULL key_name);
#endif // SRP_CRYPTO_MBEDTLS

// sign_*.c:
void srp_keypair_free(srp_key_t *NONNULL key);
uint64_t srp_random64(void);
uint32_t srp_random32(void);
uint16_t srp_random16(void);
bool srp_randombytes(uint8_t *NONNULL dest, size_t num);
uint8_t srp_key_algorithm(srp_key_t *NONNULL key);
size_t srp_pubkey_length(srp_key_t *NONNULL key);
size_t srp_signature_length(srp_key_t *NONNULL key);
size_t srp_pubkey_copy(uint8_t *NONNULL buf, size_t max, srp_key_t *NONNULL key);
int srp_sign(uint8_t *NONNULL output, size_t max, uint8_t *NONNULL message, size_t msglen,
             uint8_t *NONNULL rdata, size_t rdlen, srp_key_t *NONNULL key);

// verify_*.c:
bool srp_sig0_verify(dns_wire_t *NONNULL message, dns_rr_t *NONNULL key, dns_rr_t *NONNULL signature);
void srp_print_key(srp_key_t *NONNULL key);

// hash_*.c:
void srp_hmac_iov(hmac_key_t *NONNULL key, uint8_t *NONNULL output, size_t max, struct iovec *NONNULL iov, int count);
int srp_base64_parse(char *NONNULL src, size_t *NONNULL len_ret, uint8_t *NONNULL buf, size_t buflen);
#endif // __SRP_CRYPTO_H

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 108
// indent-tabs-mode: nil
// End:
