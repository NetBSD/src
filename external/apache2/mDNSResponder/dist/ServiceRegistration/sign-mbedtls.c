/* sign-mbedtls.c
 *
 * Copyright (c) 2018-2019 Apple Computer, Inc. All rights reserved.
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
 * DNS SIG(0) signature generation for DNSSD SRP using mbedtls.
 *
 * Functions required for loading, saving, and generating public/private keypairs, extracting the public key
 * into KEY RR data, and computing signatures.
 *
 * This is the implementation for mbedtls, e.g. on Thread Devices, Linux, and OpenWRT.
 */

#include <stdio.h>
#ifdef THREAD_DEVKIT_ADK
#include <openthread/random_noncrypto.h>
#include "HAPPlatformRandomNumber.h"
#else
#include <arpa/inet.h>
#ifdef LINUX_GETENTROPY
#define _GNU_SOURCE
#include <linux/random.h>
#include <sys/syscall.h>
#else
#include <sys/random.h>
#endif // LINUX_GETENTROPY
#endif // THREAD_DEVKIT_ADK
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "srp.h"
#include "dns-msg.h"
#include "srp-crypto.h"
#include "dns_sd.h"

// For debugging
#ifdef DEBUG_SHA256
int
srp_mbedtls_sha256_update_ret(const char *thing_name,
                              mbedtls_sha256_context *sha, uint8_t *data, size_t len)
{
    int i;
    fprintf(stderr, "%s %lu: ", thing_name, (unsigned long)len);
    if (len > 400) {
        len = 400;
    }

    for (i = 0; i < len; i++) {
        fprintf(stderr, "%02x", data[i]);
    }
    fputs("\n", stderr);
    return mbedtls_sha256_update_ret(sha, data, len);
}

int
srp_mbedtls_sha256_finish_ret(mbedtls_sha256_context *sha, uint8_t *hash)
{
    int i;
    int status = mbedtls_sha256_finish_ret(sha, hash);
    fprintf(stderr, "hash:     ");
    for (i = 0; i < ECDSA_SHA256_HASH_SIZE; i++) {
        fprintf(stderr, "%02x", hash[i]);
    }
    fputs("\n", stderr);
    return status;
}
#endif

// Key is stored in an opaque data structure, for mbedtls this is an mbedtls_pk_context.
// Function to read a public key from a KEY record
// Function to validate a signature given some data and a public key (not required on client)

// Function to free a key
void
srp_keypair_free(srp_key_t *key)
{
    mbedtls_pk_free(&key->key);
    free(key);
}

// Needed to seed the RNG with good entropy data.
static int
get_entropy(void *data, unsigned char *output, size_t len, size_t *outlen)
{
#ifdef THREAD_DEVKIT_ADK
    HAPPlatformRandomNumberFill(output, len);
    *outlen = len;
    return 0;
#else
#ifdef LINUX_GETENTROPY
    int result = syscall(SYS_getrandom, output, len, GRND_RANDOM);
#else
    int result = getentropy(output, len);
#endif
    (void)data;

    if (result != 0) {
        ERROR("getentropy returned %s", strerror(errno));
        return MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
    }
    *outlen = len;
#endif // THREAD_DEVKIT_ADK
    return 0;
}

// mbedtls on embedded devices seems to react poorly to multiple rng contexts, so we create just
// one and keep it around.   It would be nice if this got fixed, but it's actually more efficient
// to have one context, so not something we need to fix.
typedef struct rng_state {
    mbedtls_entropy_context entropy_context;
    mbedtls_ctr_drbg_context rng_context;
    char errbuf[64];
} rng_state_t;

static rng_state_t *rng_state;

bool
rng_state_fetch(void)
{
    int status;

    if (rng_state == NULL) {
        rng_state = calloc(1, sizeof *rng_state);
        if (rng_state == NULL) {
            ERROR("srp_random16(): no memory for state.");
            goto fail;
        }

        mbedtls_entropy_init(&rng_state->entropy_context);
        status = mbedtls_entropy_add_source(&rng_state->entropy_context, get_entropy,
                                            NULL, 1, MBEDTLS_ENTROPY_SOURCE_STRONG);
        if (status != 0) {
            mbedtls_strerror(status, rng_state->errbuf, sizeof rng_state->errbuf);
            ERROR("mbedtls_entropy_add_source failed: %s", rng_state->errbuf);
            goto fail;
        }

        mbedtls_ctr_drbg_init(&rng_state->rng_context);
        status = mbedtls_ctr_drbg_seed(&rng_state->rng_context,
                                       mbedtls_entropy_func, &rng_state->entropy_context, NULL, 0);

        if (status != 0) {
            mbedtls_strerror(status, rng_state->errbuf, sizeof rng_state->errbuf);
            ERROR("mbedtls_ctr_drbg_seed failed: %s", rng_state->errbuf);
        fail:
            free(rng_state);
            rng_state = NULL;
            return false;
        }
    }
    return true;
}

static srp_key_t *
srp_key_setup(void)
{
    srp_key_t *key = calloc(1, sizeof(*key));

    if (key == NULL) {
        return key;
    }

    mbedtls_pk_init(&key->key);
    if (rng_state_fetch()) {
        return key;
    }
    mbedtls_pk_free(&key->key);
    free(key);
    return NULL;
}

uint16_t
srp_random16()
{
    int status;
    uint16_t ret;
    char errbuf[64];
    if (rng_state_fetch()) {
        status = mbedtls_ctr_drbg_random(&rng_state->rng_context, (unsigned char *)&ret, sizeof ret);
        if (status != 0) {
            mbedtls_strerror(status, errbuf, sizeof errbuf);
            ERROR("mbedtls_ctr_drbg_random failed: %s", errbuf);
            return 0xffff;
        }
        return ret;
    }
    return 0xffff;
}

uint32_t
srp_random32()
{
    int status;
    uint32_t ret;
    char errbuf[64];
    if (rng_state_fetch()) {
        status = mbedtls_ctr_drbg_random(&rng_state->rng_context, (unsigned char *)&ret, sizeof ret);
        if (status != 0) {
            mbedtls_strerror(status, errbuf, sizeof errbuf);
            ERROR("mbedtls_ctr_drbg_random failed: %s", errbuf);
            return 0xffffffff;
        }
        return ret;
    }
    return 0xffffffff;
}

uint64_t
srp_random64()
{
    int status;
    uint64_t ret;
    char errbuf[64];
    if (rng_state_fetch()) {
        status = mbedtls_ctr_drbg_random(&rng_state->rng_context, (unsigned char *)&ret, sizeof ret);
        if (status != 0) {
            mbedtls_strerror(status, errbuf, sizeof errbuf);
            ERROR("mbedtls_ctr_drbg_random failed: %s", errbuf);
            return 0xffffffffffffffffull;
        }
        return ret;
    }
    return 0xffffffffffffffffull;
}

bool
srp_randombytes(uint8_t *dest, size_t num)
{
    int status;
    char errbuf[64];
    if (rng_state_fetch()) {
        status = mbedtls_ctr_drbg_random(&rng_state->rng_context, (unsigned char *)dest, num);
        if (status != 0) {
            mbedtls_strerror(status, errbuf, sizeof errbuf);
            ERROR("mbedtls_ctr_drbg_random failed: %s", errbuf);
            return false;
        }
        return true;
    }
    return false;
}

srp_key_t *
srp_load_key_from_buffer(const uint8_t *buffer, size_t length)
{
    srp_key_t *key;
    int status;
    char errbuf[64];

    key = srp_key_setup();
    if (key == NULL) {
        return NULL;
    }

    if ((status = mbedtls_pk_parse_key(&key->key, buffer, length, NULL, 0)) != 0) {
        mbedtls_strerror(status, errbuf, sizeof errbuf);
        ERROR("mbedtls_pk_parse_key failed: %s", errbuf);
    } else if (!mbedtls_pk_can_do(&key->key, MBEDTLS_PK_ECDSA)) {
        ERROR("Buffer does not contain a usable ECDSA key.");
    } else {
        return key;
    }
    srp_keypair_free(key);
    return NULL;
}

// Function to generate a key
srp_key_t *
srp_generate_key(void)
{
    srp_key_t *key;
    int status;
    char errbuf[64];
    const mbedtls_pk_info_t *key_type;

    INFO("srp_key_setup");
    key = srp_key_setup();
    if (key == NULL) {
        ERROR("srp_key_setup() failed.");
        return NULL;
    }
    key_type = mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY);
    if (key_type == NULL) {
        INFO("mbedtls_pk_info_from_type failed");
        return NULL;
    }

    INFO("mbedtls_pk_setup");
    if ((status = mbedtls_pk_setup(&key->key, key_type)) != 0) {
        mbedtls_strerror(status, errbuf, sizeof errbuf);
        ERROR("mbedtls_pk_setup failed: %s", errbuf);
    } else {
        INFO("mbedtls_pk_ecdsa_genkey");
        if ((status = mbedtls_ecdsa_genkey(mbedtls_pk_ec(key->key), MBEDTLS_ECP_DP_SECP256R1,
                                           mbedtls_ctr_drbg_random, &rng_state->rng_context)) != 0) {
            mbedtls_strerror(status, errbuf, sizeof errbuf);
            ERROR("mbedtls_ecdsa_genkey failed: %s", errbuf);
        } else {
            return key;
        }
    }
    srp_keypair_free(key);
    return NULL;
}

// Copy an srp_key_t into a buffer.   Key is not necessarily aligned with the beginning of the
// buffer; the return value, if not NULL, is the beginning of the key.   If NULL, the buffer wasn't
// big enough.
uint8_t *
srp_store_key_to_buffer(uint8_t *buffer, size_t *length, srp_key_t *key)
{
    size_t len = mbedtls_pk_write_key_der(&key->key, buffer, *length);
    uint8_t *ret;
    char errbuf[64];
    if (len <= 0) {
        mbedtls_strerror(len, errbuf, sizeof errbuf);
        ERROR("mbedtls_pk_write_key_der failed: %s", errbuf);
        return NULL;
    }
    ret = &buffer[*length - len];
    *length = len;
    return ret;
}

srp_key_t *
srp_get_key(const char *key_name, void *os_context)
{
    uint8_t buf[256];
    uint16_t buf_length;
    uint8_t *key_bytes;
    size_t keydata_length;
    int err;
    srp_key_t *key;

    err = srp_load_key_data(os_context, key_name, buf, &buf_length, sizeof buf);
    if (err == kDNSServiceErr_NoError) {
        key = srp_load_key_from_buffer(buf, buf_length);
        if (key == NULL) {
            INFO("load key fail");
            return NULL;
        }
        // Otherwise we have a key.
    } else if (err == kDNSServiceErr_NoSuchKey) {
        key = srp_generate_key();
        if (key == NULL) {
            INFO("gen key fail");
            return NULL;
        }
        keydata_length = sizeof buf;
        if ((key_bytes = srp_store_key_to_buffer(buf, &keydata_length, key)) == NULL) {
            INFO("store key fail");
            return NULL;
        }
        // Note that it's possible for key_bytes != buf.
        err = srp_store_key_data(os_context, key_name, key_bytes, (uint16_t)keydata_length);
        if (err != kDNSServiceErr_NoError) {
            INFO("store key data fail");
            return NULL;
        }
    } else {
        INFO("weird error %d", err);
        return NULL;
    }
    return key;
}

// Function to get the length of the public key
size_t
srp_pubkey_length(srp_key_t *key)
{
    return ECDSA_KEY_SIZE;
}

uint8_t
srp_key_algorithm(srp_key_t *key)
{
    return dnssec_keytype_ecdsa;
}

size_t
srp_signature_length(srp_key_t *key)
{
    return ECDSA_KEY_SIZE;
}

// Function to copy out the public key as binary data
size_t
srp_pubkey_copy(uint8_t *buf, size_t max, srp_key_t *key)
{
    mbedtls_ecp_keypair *ecp = mbedtls_pk_ec(key->key);
    char errbuf[64];
    int status;

    if (max < ECDSA_KEY_SIZE) {
        return 0;
    }

    // Currently ECP only.
    if ((status = mbedtls_mpi_write_binary(&ecp->Q.X, buf, ECDSA_KEY_PART_SIZE)) != 0 ||
        (status = mbedtls_mpi_write_binary(&ecp->Q.Y, buf + ECDSA_KEY_PART_SIZE, ECDSA_KEY_PART_SIZE)) != 0) {
        mbedtls_strerror(status, errbuf, sizeof errbuf);
        ERROR("mbedtls_mpi_write_binary: %s", errbuf);
        return 0;
    }

#ifdef MBEDTLS_PUBKEY_DUMP
    int i;
    fprintf(stderr, "pubkey %d: ", ECDSA_KEY_SIZE);
    for (i = 0; i < ECDSA_KEY_SIZE; i++) {
        fprintf(stderr, "%02x", buf[i]);
    }
    putc('\n', stderr);
#endif // MBEDTLS_PUBKEY_DUMP
    return ECDSA_KEY_SIZE;
}

// Function to generate a signature given some data and a private key
int
srp_sign(uint8_t *output, size_t max, uint8_t *message, size_t msglen, uint8_t *rr, size_t rdlen, srp_key_t *key)
{
    int success = 1;
    int status;
    unsigned char hash[ECDSA_SHA256_HASH_SIZE];
    char errbuf[64];
    mbedtls_sha256_context *sha;
    uint8_t shabuf[16 + sizeof(*sha)];
    uint32_t *sbp;
    mbedtls_ecp_keypair *ecp = mbedtls_pk_ec(key->key);
    mbedtls_mpi r, s;

    if (max < ECDSA_SHA256_SIG_SIZE) {
        ERROR("srp_sign: not enough space in output buffer (%lu) for signature (%d).",
              (unsigned long)max, ECDSA_SHA256_SIG_SIZE);
        return 0;
    }

    sbp = (uint32_t *)shabuf;
    sha = (mbedtls_sha256_context *)sbp;
    mbedtls_sha256_init(sha);
    memset(hash, 0, sizeof hash);
    mbedtls_mpi_init(&r);
    mbedtls_mpi_init(&s);

    // Calculate the hash across first the SIG RR (minus the signature) and then the message
    // up to but not including the SIG RR.
    status = mbedtls_sha256_starts_ret(sha, 0);
    if (status == 0) {
        status = srp_mbedtls_sha256_update_ret("rr", sha, rr, rdlen);
    }
    if (status == 0) {
        status = srp_mbedtls_sha256_update_ret("message", sha, message, msglen);
    }
    if (status == 0) {
        status = srp_mbedtls_sha256_finish_ret(sha, hash);
    }
    if (status != 0) {
        mbedtls_strerror(status, errbuf, sizeof errbuf);
        ERROR("mbedtls_sha_256 hash failed: %s", errbuf);
        success = 0;
        goto cleanup;
    }

    status = mbedtls_ecdsa_sign(&ecp->grp, &r, &s, &ecp->d, hash, sizeof hash,
                                mbedtls_ctr_drbg_random, &rng_state->rng_context);
    if (status != 0) {
        mbedtls_strerror(status, errbuf, sizeof errbuf);
        ERROR("mbedtls_ecdsa_sign failed: %s", errbuf);
        success = 0;
        goto cleanup;
    }

    if ((status = mbedtls_mpi_write_binary(&r, output, ECDSA_SHA256_SIG_PART_SIZE)) != 0 ||
        (status = mbedtls_mpi_write_binary(&s, output + ECDSA_SHA256_SIG_PART_SIZE,
                                           ECDSA_SHA256_SIG_PART_SIZE)) != 0) {
        mbedtls_strerror(status, errbuf, sizeof errbuf);
        ERROR("mbedtls_ecdsa_sign failed: %s", errbuf);
        success = 0;
        goto cleanup;
    }
cleanup:
    mbedtls_mpi_free(&r);
    mbedtls_mpi_free(&s);
    return success;
}

#ifndef THREAD_DEVKIT_ADK
int
srp_reset_key(const char *key_name, void *UNUSED os_context)
{
    return srp_remove_key_file(os_context, key_name);
}
#endif

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 108
// indent-tabs-mode: nil
// End:
