/*
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <stdio.h> 
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <pwd.h>
#include <errno.h>
#include <argon2.h>

#include <err.h>
#include "crypt.h"

/* defaults pulled from run.c */
#define HASHLEN		32
#define T_COST_DEF 	3 
#define LOG_M_COST_DEF 	12 /* 2^12 = 4 MiB */
#define LANES_DEF 	1
#define THREADS_DEF 	1
#define OUTLEN_DEF 	32
#define MAX_PASS_LEN 	128

#define ARGON2_CONTEXT_INITIALIZER	\
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
	T_COST_DEF, LOG_M_COST_DEF,\
	LANES_DEF, THREADS_DEF, \
	ARGON2_VERSION_NUMBER, 0, 0, ARGON2_DEFAULT_FLAGS}

#define ARGON2_ARGON2_STR	"argon2"
#define ARGON2_ARGON2I_STR	"argon2i"
#define ARGON2_ARGON2D_STR	"argon2d"
#define ARGON2_ARGON2ID_STR	"argon2id"


/*
 * Some macros for constant-time comparisons. These work over values in
 * the 0..255 range. Returned value is 0x00 on "false", 0xFF on "true".
 */
#define EQ(x, y) ((((0U - ((unsigned)(x) ^ (unsigned)(y))) >> 8) & 0xFF) ^ 0xFF)
#define GT(x, y) ((((unsigned)(y) - (unsigned)(x)) >> 8) & 0xFF)
#define GE(x, y) (GT(y, x) ^ 0xFF)
#define LT(x, y) GT(y, x)
#define LE(x, y) GE(y, x)

static unsigned
b64_char_to_byte(int c)
{
    unsigned x;

    x = (GE(c, 'A') & LE(c, 'Z') & (c - 'A')) |
        (GE(c, 'a') & LE(c, 'z') & (c - ('a' - 26))) |
        (GE(c, '0') & LE(c, '9') & (c - ('0' - 52))) | (EQ(c, '+') & 62) |
        (EQ(c, '/') & 63);
    return x | (EQ(x, 0) & (EQ(c, 'A') ^ 0xFF));
}

static const char *
from_base64(void *dst, size_t *dst_len, const char *src)
{
	size_t len;
	unsigned char *buf;
	unsigned acc, acc_len;

	buf = (unsigned char *)dst;
	len = 0;
	acc = 0;
	acc_len = 0;
	for (;;) {
		unsigned d;

		d = b64_char_to_byte(*src);
		if (d == 0xFF) {
			break;
		}
		src++;
		acc = (acc << 6) + d;
		acc_len += 6;
		if (acc_len >= 8) {
			acc_len -= 8;
			if ((len++) >= *dst_len) {
				return NULL;
			}
			*buf++ = (acc >> acc_len) & 0xFF;
		}
	}

	/*
	 * If the input length is equal to 1 modulo 4 (which is
	 * invalid), then there will remain 6 unprocessed bits;
	 * otherwise, only 0, 2 or 4 bits are buffered. The buffered
	 * bits must also all be zero.
	 */
	if (acc_len > 4 || (acc & (((unsigned)1 << acc_len) - 1)) != 0) {
		return NULL;
	}
	*dst_len = len;
	return src;
}

/* process params to argon2 */
/* we don't force param order as input, */
/* but we do provide the expected order to argon2 api */
static int
decode_option(argon2_context *ctx, argon2_type *atype, const char *option) 
{
	size_t tmp = 0;
        char *in = 0, *inp;
        char *a = 0;
        char *p = 0;
	size_t sl;
	int error = 0;

        in = (char *)strdup(option);
	inp = in;

	if (*inp == '$') inp++;

	a = strsep(&inp, "$");

	sl = strlen(a);

	if (sl == strlen(ARGON2_ARGON2I_STR) && 
	   !(strcmp(ARGON2_ARGON2I_STR, a))) {
		*atype=Argon2_i;
	} else if (sl == strlen(ARGON2_ARGON2D_STR) && 
	        !(strcmp(ARGON2_ARGON2D_STR, a))) {
		*atype=Argon2_d;
	}
	else if (sl == strlen(ARGON2_ARGON2ID_STR) && 
	        !(strcmp(ARGON2_ARGON2ID_STR, a))) {
		*atype=Argon2_id;
	} else { /* default to id, we assume simple mistake */
		/* don't abandon yet */
		*atype=Argon2_id;
	}

	a = strsep(&inp, "$");

	/* parse the version number of the hash, if it's there */
	if (strncmp(a, "v=", 2) == 0) {
		a += 2;
		if ((getnum(a, &tmp))<0) { /* on error, default to current */
			/* should start thinking about aborting */
			ctx->version = ARGON2_VERSION_10;
		} else {
			ctx->version = tmp;
		}
		a = strsep(&inp, "$");
	} else {
		/*
		 * This is a parameter list, not a version number, use the
		 * default version.
		 */
		ctx->version = ARGON2_VERSION_10;
	}

	/* parse labelled argon2 params */
	/* m_cost (m)
	 * t_cost (t)
	 * threads (p)
	 */
	while ((p = strsep(&a, ","))) {
		switch (*p) {
			case 'm':
				p += strlen("m=");
				if ((getnum(p, &tmp)) < 0) {
					--error;
				} else {
					ctx->m_cost = tmp;
				}
				break;
			case 't':
				p += strlen("t=");
				if ((getnum(p, &tmp)) < 0) {
					--error;
				} else {
					ctx->t_cost = tmp;
				}
				break;
			case 'p':
				p += strlen("p=");
				if ((getnum(p, &tmp)) < 0) {
					--error;
				} else {
					ctx->threads = tmp;
				}
				break;
			default:
				return -1;

		}
	}

	a = strsep(&inp, "$");

	sl = ctx->saltlen;

	if (from_base64(ctx->salt, &sl, a) == NULL)
		return -1;

	ctx->saltlen = sl;

	a = strsep(&inp, "$");

	if (a) {
		snprintf((char *)ctx->pwd, ctx->pwdlen, "%s", a);
	} else {
		/* don't care if passwd hash is missing */
		/* if missing, most likely coming from */
		/* pwhash or similar */ 
	}

	/* free our token buffer */
        free(in);

	/* 0 on success, <0 otherwise */
        return error;
}

crypt_private char * 
__crypt_argon2(const char *pw, const char * salt)
{
	/* we use the libargon2 api to generate */
	/* return code */
	int rc = 0;
	/* output buffer */
	char ebuf[32];
	/* argon2 variable, default to id */
	argon2_type atype = Argon2_id;
	/* default to current argon2 version */
	/* argon2 context to collect params */
	argon2_context ctx = ARGON2_CONTEXT_INITIALIZER;
	/* argon2 encoded buffer */
	char encodebuf[256];
	/* argon2 salt buffer */
	char saltbuf[128];
	/* argon2 pwd buffer */
	char pwdbuf[128];
	/* returned static buffer */
	static char rbuf[512];

	/* clear buffers */
	explicit_memset(rbuf, 0, sizeof(rbuf));

	/* we use static buffers to avoid allocation */
	/* and easier cleanup */
	ctx.out = (uint8_t *)ebuf;
	ctx.outlen = sizeof(ebuf);

	ctx.out = (uint8_t *)encodebuf;
	ctx.outlen = sizeof(encodebuf);

	ctx.salt = (uint8_t *)saltbuf;
	ctx.saltlen = sizeof(saltbuf);

	ctx.pwd = (uint8_t *)pwdbuf;
	ctx.pwdlen = sizeof(pwdbuf);

	/* decode salt string to argon2 params */
	/* argon2 context for param collection */
	rc = decode_option(&ctx, &atype, salt);

	if (rc < 0) {
		/* unable to parse input params */
		return 0;
	}

	rc = argon2_hash(ctx.t_cost, ctx.m_cost,
	    ctx.threads, pw, strlen(pw), ctx.salt, ctx.saltlen,
	    ebuf, sizeof(ebuf), encodebuf, sizeof(encodebuf),
	    atype, ctx.version);

	if (rc != ARGON2_OK) {
		fprintf(stderr, "argon2: failed: %s\n",
		    argon2_error_message(rc));
		return 0;
	}

	memcpy(rbuf, encodebuf, sizeof(encodebuf));

	/* clear buffers */
	explicit_memset(ebuf, 0, sizeof(ebuf));
	explicit_memset(encodebuf, 0, sizeof(encodebuf));
	explicit_memset(saltbuf, 0, sizeof(saltbuf));
	explicit_memset(pwdbuf, 0, sizeof(pwdbuf));

	/* return encoded str */
	return rbuf;
}
