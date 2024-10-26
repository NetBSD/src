/* hmac-openssl.c
 *
 * Copyright (c) 2019 Apple Computer, Inc. All rights reserved.
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
 * Hashed message authentication code functions using OpenSSL.
 */

#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "srp.h"
#include "dns-msg.h"
#define SRP_CRYPTO_OPENSSL_INTERNAL
#include "srp-crypto.h"

// Function to generate a signature given some data and a private key
void
srp_hmac_iov(hmac_key_t *key, uint8_t *output, size_t max, struct iovec *iov, int count)
{
    int status;
    char errbuf[64];
    uint8_t kipad[SRP_SHA256_BLOCK_SIZE], kopad[SRP_SHA256_BLOCK_SIZE], intermediate[SRP_SHA256_HASH_SIZE];
    EVP_MD_CTX inner, outer;
    const EVP_MD md;
    int i;

    if (key->algorithm != SRP_HMAC_TYPE_SHA256) {
        ERROR("srp_hmac_iov: unsupported HMAC hash algorithm: %d", key->algorithm);
        return;
    }
    if (max < SRP_SHA256_HASH_SIZE) {
        ERROR("srp_hmac_iov: not enough space in output buffer (%lu) for hash (%d).",
              (unsigned long)max, SRP_SHA256_HASH_SIZE);
        return;
    }

    md = EVP_sha256();
    EVP_MD_CTX_INIT(&inner);

    // If the key is longer than the block size, hash it and use the digest as the key.
    if (key->length > SRP_SHA256_BLOCK_SIZE) {
        if ((status = EVP_DigestInit(&inner, md)) != 0) {
            ERROR("srp_hmac_iov failed to initialize key digest");
            return;
        }
        // Compute H(K)
        if ((status = EVP_DigestUpdate(&inner, key, key->length)) != 0) {
            ERROR("srp_hmac_iov failed to hash key");
            return;
        }
        if ((status = EVP_DigestFinal(&inner, intermediate, SRP_SHA256_HASH_SIZE)) != 0) {
            ERROR("srp_hmac_iov failed to digest key");
            return;
        }
        EVP_MD_CTX_INIT(&inner);
    }

    // Compute key ^ kipad and key ^ kopad
    for (i = 0; i < SRP_SHA256_BLOCK_SIZE; i++) {
        uint8_t byte = i >= key->length ? 0 : key->secret[i];
        kipad[i] = byte ^ 0x36;
        kopad[i] = byte ^ 0x5c;
    }

    if ((status = EVP_DigestInit(&inner, md)) != 0) {
        ERROR("srp_hmac_iov failed to initialize inner digest");
        return;
    }
    // Compute H(K xor ipad, text)
    if ((status = EVP_DigestUpdate(&inner, kipad, SRP_SHA256_BLOCK_SIZE)) != 0) {
        ERROR("srp_hmac_iov failed to hash ipad to inner digest");
        return;
    }
    for (i = 0; i < count; i++) {
        if ((status = EVP_DigestUpdate(&inner, iov[i].iov_base, iov[i].iov_len)) != 0) {
            ERROR("srp_hmac_iov failed to hash chunk %d to inner digest", i);
            return;
        }
    }
    if ((status = EVP_DigestFinal(&inner, intermediate, SRP_SHA256_HASH_SIZE)) != 0) {
        ERROR("srp_hmac_iov failed to hash ipad to inner digest");
        return;
    }

    // Compute H(K xor opad, H(K xor ipad, text))
    EVP_MD_CTX_INIT(&outer);
    if ((status = EVP_DigestInit(&outer, md)) != 0) {
        ERROR("srp_hmac_iov failed to initialize outer digest");
        return;
    }
    if ((status = EVP_DigestUpdate(&outer, kopad, SRP_SHA256_BLOCK_SIZE)) != 0) {
        ERROR("srp_hmac_iov failed to hash outer pad");
        return;
        goto kablooie;
    }
    if ((status = EVP_DigestUpdate(&outer, intermediate, SRP_SHA256_HASH_SIZE)) != 0) {
        ERROR("srp_hmac_iov failed to hash outer digest");
        return;
        goto kablooie;
    }
    if ((status = EVP_DigestFinal(&outer, output, max)) != 0) {
        ERROR("srp_hmac_iov failed to hash outer outer pad with inner digest");
        return;
    }
    // Bob's your uncle...
}

int
srp_base64_parse(char *src, size_t *len_ret, uint8_t *buf, size_t buflen)
{
    size_t slen = strlen(src);
    int ret = mbedtls_base64_decode(buf, buflen, len_ret, (const unsigned char *)src, slen);
    if (ret == MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL) {
        return ENOBUFS;
    } else if (ret == MBEDTLS_ERR_BASE64_INVALID_CHARACTER) {
        return EILSEQ;
    } else if (ret < 0) {
        return EINVAL;
    }
    return 0;
}

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 108
// indent-tabs-mode: nil
// End:
