/*
 * Copyright 1995-2021 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

/*
 * SHA-1 low level APIs are deprecated for public use, but still ok for
 * internal use.
 */
#include "internal/deprecated.h"

#include <stdio.h>
#include <string.h>
#include <openssl/crypto.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include "crypto/sha.h"

unsigned char *SHA1(const unsigned char *d, size_t n, unsigned char *md)
{
    static unsigned char m[SHA_DIGEST_LENGTH];

    if (md == NULL)
        md = m;
    return EVP_Q_digest(NULL, "SHA1", NULL, d, n, md, NULL) ? md : NULL;
}
