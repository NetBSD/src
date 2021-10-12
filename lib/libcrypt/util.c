/*	$NetBSD: util.c,v 1.3 2021/10/12 15:25:39 nia Exp $	*/
/*
 * Copyright 1997 Niels Provos <provos@physnet.uni-hamburg.de>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Niels Provos.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/cdefs.h>
#if !defined(lint)
__RCSID("$NetBSD: util.c,v 1.3 2021/10/12 15:25:39 nia Exp $");
#endif /* not lint */

#include <sys/types.h>
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>

#include "crypt.h"

/* traditional unix "B64" encoding */
static const unsigned char itoa64[] =		/* 0 ... 63 => ascii - 64 */
	"./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

/* standard base64 encoding, used by Argon2 */
static const unsigned char itoabase64[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

crypt_private int
getnum(const char *str, size_t *num)
{
	char *ep;
	unsigned long rv;

	if (str == NULL) {
		*num = 0;
		return 0;
	}

	rv = strtoul(str, &ep, 0);

	if (str == ep || *ep) {
		errno = EINVAL;
		return -1;
	}

	if (errno == ERANGE && rv == ULONG_MAX)
		return -1;
	*num = (size_t)rv;
	return 0;
}

crypt_private void
__crypt_to64(char *s, uint32_t v, int n)
{

	while (--n >= 0) {
		*s++ = itoa64[v & 0x3f];
		v >>= 6;
	}
}

crypt_private void
__crypt_tobase64(char *s, uint32_t v, int n)
{

	while (--n >= 0) {
		*s++ = itoabase64[v & 0x3f];
		v >>= 6;
	}
}
