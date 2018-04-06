/* $OpenBSD: hash.c,v 1.5 2018/01/13 00:24:09 naddy Exp $ */
/*
 * Public domain. Author: Christian Weisgerber <naddy@openbsd.org>
 * API compatible reimplementation of function from nacl
 */
#include "includes.h"
__RCSID("$NetBSD: hash.c,v 1.6 2018/04/06 18:59:00 christos Exp $");

#include "crypto_api.h"

#include <stdarg.h>

#include "digest.h"
#include "log.h"
#include "ssherr.h"

int
crypto_hash_sha512(unsigned char *out, const unsigned char *in,
    unsigned long long inlen)
{
	int r;

	if ((r = ssh_digest_memory(SSH_DIGEST_SHA512, in, inlen, out,
	    crypto_hash_sha512_BYTES)) != 0)
		fatal("%s: %s", __func__, ssh_err(r));
	return 0;
}
