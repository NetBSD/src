/*	$NetBSD: pw_gensalt.c,v 1.13.2.1 2024/08/23 16:40:48 martin Exp $	*/

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
 *
 * from OpenBSD: pwd_gensalt.c,v 1.9 1998/07/05 21:08:32 provos Exp
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: pw_gensalt.c,v 1.13.2.1 2024/08/23 16:40:48 martin Exp $");
#endif /* not lint */

#include <sys/syslimits.h>
#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <err.h>
#include <grp.h>
#include <pwd.h>
#include <util.h>
#include <time.h>
#include <errno.h>

#include "crypt.h"

#ifdef HAVE_ARGON2
#include <argon2.h>
#define ARGON2_ARGON2_STR       "argon2"
#define ARGON2_ARGON2I_STR      "argon2i"
#define ARGON2_ARGON2D_STR      "argon2d"
#define ARGON2_ARGON2ID_STR     "argon2id"

crypt_private int
estimate_argon2_params(argon2_type, uint32_t *, uint32_t *, uint32_t *);
#endif /* HAVE_ARGON2 */

static const struct pw_salt {
	const char *name;
	int (*gensalt)(char *, size_t, const char *);
} salts[] = {
	{ "old", __gensalt_old },
	{ "new", __gensalt_new },
	{ "newsalt", __gensalt_new },
	{ "md5", __gensalt_md5 },
	{ "sha1", __gensalt_sha1 },
	{ "blowfish", __gensalt_blowfish },
#ifdef HAVE_ARGON2
	/* argon2 default to argon2id */
	{ "argon2", __gensalt_argon2id},
	{ "argon2id", __gensalt_argon2id},
	{ "argon2i", __gensalt_argon2i},
	{ "argon2d", __gensalt_argon2d},
#endif /* HAVE_ARGON2 */
	{ NULL, NULL }
};

crypt_private int
/*ARGSUSED2*/
__gensalt_old(char *salt, size_t saltsiz, const char *option)
{
	if (saltsiz < 3) {
		errno = ENOSPC;
		return -1;
	}
	__crypt_to64(&salt[0], arc4random(), 2);
	salt[2] = '\0';
	return 0;
}

crypt_private int
/*ARGSUSED2*/
__gensalt_new(char *salt, size_t saltsiz, const char* option)
{
	size_t nrounds;

	if (saltsiz < 10) {
		errno = ENOSPC;
		return -1;
	}

	if (getnum(option, &nrounds) == -1)
		return -1;

	/* Check rounds, 24 bit is max */
	if (nrounds < 7250)
		nrounds = 7250;
	else if (nrounds > 0xffffff)
		nrounds = 0xffffff;
	salt[0] = _PASSWORD_EFMT1;
	__crypt_to64(&salt[1], (uint32_t)nrounds, 4);
	__crypt_to64(&salt[5], arc4random(), 4);
	salt[9] = '\0';
	return 0;
}

crypt_private int
/*ARGSUSED2*/
__gensalt_md5(char *salt, size_t saltsiz, const char *option)
{
	if (saltsiz < 13) {  /* $1$8salt$\0 */
		errno = ENOSPC;
		return -1;
	}
	salt[0] = _PASSWORD_NONDES;
	salt[1] = '1';
	salt[2] = '$';
	__crypt_to64(&salt[3], arc4random(), 4);
	__crypt_to64(&salt[7], arc4random(), 4);
	salt[11] = '$';
	salt[12] = '\0';
	return 0;
}

crypt_private int
__gensalt_sha1(char *salt, size_t saltsiz, const char *option)
{
	int n;
	size_t nrounds;

	if (getnum(option, &nrounds) == -1)
		return -1;
	n = snprintf(salt, saltsiz, "%s%u$", SHA1_MAGIC,
	    __crypt_sha1_iterations(nrounds));
	/*
	 * The salt can be up to 64 bytes, but 8
	 * is considered enough for now.
	 */
	if ((size_t)n + 9 >= saltsiz)
		return 0;
	__crypt_to64(&salt[n], arc4random(), 4);
	__crypt_to64(&salt[n + 4], arc4random(), 4);
	salt[n + 8] = '$';
	salt[n + 9] = '\0';
	return 0;
}

#ifdef HAVE_ARGON2
static int
__gensalt_argon2_decode_option(char *dst, size_t dlen,
    const char *option, argon2_type atype)
{
	char *in = 0;
	char *a = 0;
	size_t tmp = 0;
	int error = 0;
	uint32_t memory = 0;
	uint32_t time = 0;
	uint32_t threads = 0;

	memset(dst, 0, dlen);

	if (option == NULL) {
		goto done;
	}

	in = strdup(option);

	while ((a = strsep(&in, ",")) != NULL) {
		switch (*a) {
			case 'm':
				a += strlen("m=");
				if ((getnum(a, &tmp)) == -1) {
					--error;
				} else {
					memory = tmp;
				}
				break;
			case 't':
				a += strlen("t=");
				if ((getnum(a, &tmp)) == -1) {
					--error;
				} else {
					time = tmp;
				}
				break;
			case 'p':
				a += strlen("p=");
				if ((getnum(a, &tmp)) == -1) {
					--error;
				} else {
					threads = tmp;
				}
				break;
			default:
				--error;
		}
	}

	free(in);

done:
	/*
	 * If parameters are unspecified, calculate some reasonable
	 * ones based on system time.
	 */
	if (memory < ARGON2_MIN_MEMORY ||
	    time < ARGON2_MIN_TIME ||
	    threads < ARGON2_MIN_THREADS) {
		estimate_argon2_params(atype, &time, &memory, &threads);
	}

	snprintf(dst, dlen, "m=%d,t=%d,p=%d", memory, time, threads);

	return error;
}


static int
__gensalt_argon2(char *salt, size_t saltsiz,
    const char *option, argon2_type atype)
{
	int rc;
	int n;
	char buf[64];

	/* get param, enforcing order and applying defaults */
	if ((rc = __gensalt_argon2_decode_option(buf,
	    sizeof(buf), option, atype)) < 0) {
		return 0;
	}

	n = snprintf(salt, saltsiz, "$%s$v=%d$%s$",
		argon2_type2string(atype,0), ARGON2_VERSION_NUMBER, buf);

	if ((size_t)n + 16 >= saltsiz) {
		return 0;
	}

	__crypt_tobase64(&salt[n], arc4random(), 4);
	__crypt_tobase64(&salt[n + 4], arc4random(), 4);
	__crypt_tobase64(&salt[n + 8], arc4random(), 4);
	__crypt_tobase64(&salt[n + 12], arc4random(), 4);

	salt[n + 16] = '$';
	salt[n + 17] = '\0';

	return 0;
}

/* argon2 variant-specific hooks to generic */
crypt_private int
__gensalt_argon2id(char *salt, size_t saltsiz, const char *option)
{
	return __gensalt_argon2(salt, saltsiz, option, Argon2_id);
}

crypt_private int
__gensalt_argon2i(char *salt, size_t saltsiz, const char *option)
{
	return __gensalt_argon2(salt, saltsiz, option, Argon2_i);
}

crypt_private int
__gensalt_argon2d(char *salt, size_t saltsiz, const char *option)
{
	return __gensalt_argon2(salt, saltsiz, option, Argon2_d);
}

#endif /* HAVE_ARGON2 */


int
pw_gensalt(char *salt, size_t saltlen, const char *type, const char *option)
{
	const struct pw_salt *sp;

	for (sp = salts; sp->name; sp++)
		if (strcmp(sp->name, type) == 0)
			return (*sp->gensalt)(salt, saltlen, option);

	errno = EINVAL;
	return -1;
}
