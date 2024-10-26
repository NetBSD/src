/* hash.c
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
 * DNS SIG(0) hashature generation for DNSSD SRP using mbedtls.
 *
 * Functions required for loading, saving, and generating public/private keypairs, extracting the public key
 * into KEY RR data, and computing hashatures.
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
#include "srp-crypto.h"

// Function to generate a signature given some data and a private key
void
srp_hmac_iov(hmac_key_t *key, uint8_t *output, size_t max, struct iovec *iov, int count)
{
    int status;
    char errbuf[64];
    mbedtls_md_context_t ctx;
    const mbedtls_md_info_t *md_type;
    int digest_size;
	int i, line;
#define KABLOOIE line = __LINE__ - 1; goto kablooie

    switch(key->algorithm) {
    case  SRP_HMAC_TYPE_SHA256:
        md_type = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
        if (md_type == NULL) {
            ERROR("srp_hmac_iov: HMAC_SHA256 support missing");
            return;
        }
        digest_size = mbedtls_md_get_size(md_type);
        break;
    default:
        ERROR("srp_hmac_iov: unsupported HMAC hash algorithm: %d", key->algorithm);
        return;
    }
    if (max < digest_size) {
        ERROR("srp_hmac_iov: not enough space in output buffer (%lu) for hash (%d).",
              (unsigned long)max, digest_size);
        return;
    }

    if ((status = mbedtls_md_setup(&ctx, md_type, 1)) != 0) {
        KABLOOIE;
    kablooie:
        mbedtls_strerror(status, errbuf, sizeof errbuf);
        ERROR("srp_hmac_iov failed at hmac-mbedtls.c line %d: " PUB_S_SRP, line, errbuf);
    }

    if ((status = mbedtls_md_hmac_starts(&ctx, key->secret, key->length)) != 0) {
        KABLOOIE;
    }
	for (i = 0; i < count; i++) {
        if ((status = mbedtls_md_hmac_update(&ctx, iov[i].iov_base, iov[i].iov_len)) != 0) {
            KABLOOIE;
        }
	}
	if ((status = mbedtls_md_hmac_finish(&ctx, output)) != 0) {
        KABLOOIE;
	}
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
