/*	$NetBSD: ah_core.c,v 1.12 1999/12/15 06:28:44 itojun Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * RFC1826/2402 authentication header.
 */

#if (defined(__FreeBSD__) && __FreeBSD__ >= 3) || defined(__NetBSD__)
#include "opt_inet.h"
#ifdef __NetBSD__	/*XXX*/
#include "opt_ipsec.h"
#endif
#endif

/* Some of operating systems have standard crypto checksum library */
#ifdef __NetBSD__
#define HAVE_MD5
#define HAVE_SHA1
#endif
#ifdef __FreeBSD__
#define HAVE_MD5
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/kernel.h>

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_var.h>
#include <netinet/in_pcb.h>

#ifdef INET6
#include <netinet6/ip6.h>
#if !(defined(__FreeBSD__) && __FreeBSD__ >= 3)
#include <netinet6/in6_pcb.h>
#endif
#include <netinet6/ip6_var.h>
#include <netinet6/icmp6.h>
#endif

#include <netinet6/ipsec.h>
#include <netinet6/ah.h>
#ifdef IPSEC_ESP
#include <netinet6/esp.h>
#endif
#include <netkey/keyv2.h>
#include <netkey/keydb.h>
#ifdef HAVE_MD5
#include <sys/md5.h>
#else
#include <crypto/md5.h>
#endif
#ifdef HAVE_SHA1
#include <sys/sha1.h>
#define SHA1_RESULTLEN	20
#else
#include <crypto/sha1.h>
#endif

#define	HMACSIZE	16

#ifdef INET6
#define ZEROBUFLEN	256
static char zerobuf[ZEROBUFLEN];
#endif

static int ah_sumsiz_1216 __P((struct secas *));
static int ah_sumsiz_zero __P((struct secas *));
static int ah_none_mature __P((struct secas *));
static void ah_none_init __P((struct ah_algorithm_state *,
	struct secas *));
static void ah_none_loop __P((struct ah_algorithm_state *, caddr_t, size_t));
static void ah_none_result __P((struct ah_algorithm_state *, caddr_t));
static int ah_keyed_md5_mature __P((struct secas *));
static void ah_keyed_md5_init __P((struct ah_algorithm_state *,
	struct secas *));
static void ah_keyed_md5_loop __P((struct ah_algorithm_state *, caddr_t,
	size_t));
static void ah_keyed_md5_result __P((struct ah_algorithm_state *, caddr_t));
static int ah_keyed_sha1_mature __P((struct secas *));
static void ah_keyed_sha1_init __P((struct ah_algorithm_state *,
	struct secas *));
static void ah_keyed_sha1_loop __P((struct ah_algorithm_state *, caddr_t,
	size_t));
static void ah_keyed_sha1_result __P((struct ah_algorithm_state *, caddr_t));
static int ah_hmac_md5_mature __P((struct secas *));
static void ah_hmac_md5_init __P((struct ah_algorithm_state *,
	struct secas *));
static void ah_hmac_md5_loop __P((struct ah_algorithm_state *, caddr_t,
	size_t));
static void ah_hmac_md5_result __P((struct ah_algorithm_state *, caddr_t));
static int ah_hmac_sha1_mature __P((struct secas *));
static void ah_hmac_sha1_init __P((struct ah_algorithm_state *,
	struct secas *));
static void ah_hmac_sha1_loop __P((struct ah_algorithm_state *, caddr_t,
	size_t));
static void ah_hmac_sha1_result __P((struct ah_algorithm_state *, caddr_t));

/* checksum algorithms */
/* NOTE: The order depends on SADB_AALG_x in netkey/keyv2.h */
struct ah_algorithm ah_algorithms[] = {
	{ 0, 0, 0, 0, 0, 0, },
	{ ah_sumsiz_1216, ah_hmac_md5_mature, 128, 128,
		ah_hmac_md5_init, ah_hmac_md5_loop, ah_hmac_md5_result, },
	{ ah_sumsiz_1216, ah_hmac_sha1_mature, 160, 160,
		ah_hmac_sha1_init, ah_hmac_sha1_loop, ah_hmac_sha1_result, },
	{ ah_sumsiz_1216, ah_keyed_md5_mature, 128, 128,
		ah_keyed_md5_init, ah_keyed_md5_loop, ah_keyed_md5_result, },
	{ ah_sumsiz_1216, ah_keyed_sha1_mature, 160, 160,
		ah_keyed_sha1_init, ah_keyed_sha1_loop, ah_keyed_sha1_result, },
	{ ah_sumsiz_zero, ah_none_mature, 0, 2048,
		ah_none_init, ah_none_loop, ah_none_result, },
};

static int
ah_sumsiz_1216(sa)
	struct secas *sa;
{
	if (!sa)
		return -1;
	if (sa->flags & SADB_X_EXT_OLD)
		return 16;
	else
		return 12;
}

static int
ah_sumsiz_zero(sa)
	struct secas *sa;
{
	if (!sa)
		return -1;
	return 0;
}

static int
ah_none_mature(sa)
	struct secas *sa;
{
	if (sa->type == SADB_SATYPE_AH) {
		printf("ah_none_mature: protocol and algorithm mismatch.\n");
		return 1;
	}
	return 0;
}

static void
ah_none_init(state, sa)
	struct ah_algorithm_state *state;
	struct secas *sa;
{
	state->foo = NULL;
}

static void
ah_none_loop(state, addr, len)
	struct ah_algorithm_state *state;
	caddr_t addr;
	size_t len;
{
}

static void
ah_none_result(state, addr)
	struct ah_algorithm_state *state;
	caddr_t addr;
{
}

static int
ah_keyed_md5_mature(sa)
	struct secas *sa;
{
	/* anything is okay */
	return 0;
}

static void
ah_keyed_md5_init(state, sa)
	struct ah_algorithm_state *state;
	struct secas *sa;
{
	if (!state)
		panic("ah_keyed_md5_init: what?");

	state->sa = sa;
#ifdef HAVE_MD5
	state->foo = (void *)malloc(sizeof(MD5_CTX), M_TEMP, M_NOWAIT);
	if (state->foo == NULL)
		panic("ah_keyed_md5_init: what?");
	MD5Init((MD5_CTX *)state->foo);
#else
	state->foo = NULL;
	md5_init();
#endif
	if (state->sa) {
#ifdef HAVE_MD5
		MD5Update((MD5_CTX *)state->foo, 
			(u_int8_t *)_KEYBUF(state->sa->key_auth),
			(u_int)_KEYLEN(state->sa->key_auth));
#else
		md5_loop((u_int8_t *)_KEYBUF(state->sa->key_auth),
			(u_int)_KEYLEN(state->sa->key_auth));
#endif

	    {
		/*
		 * Pad after the key.
		 * We cannot simply use md5_pad() since the function
		 * won't update the total length.
		 */
		size_t padlen;
		size_t keybitlen;
		u_int8_t buf[32];

		if (_KEYLEN(state->sa->key_auth) < 56)
			padlen = 64 - 8 - _KEYLEN(state->sa->key_auth);
		else
			padlen = 64 + 64 - 8 - _KEYLEN(state->sa->key_auth);
		keybitlen = _KEYLEN(state->sa->key_auth);
		keybitlen *= 8;

		buf[0] = 0x80;
#ifdef HAVE_MD5
		MD5Update((MD5_CTX *)state->foo, &buf[0], 1);
#else
		md5_loop(&buf[0], 1);
#endif
		padlen--;

		bzero(buf, sizeof(buf));
		while (sizeof(buf) < padlen) {
#ifdef HAVE_MD5
			MD5Update((MD5_CTX *)state->foo, &buf[0], sizeof(buf));
#else
			md5_loop(&buf[0], sizeof(buf));
#endif
			padlen -= sizeof(buf);
		}
		if (padlen) {
#ifdef HAVE_MD5
			MD5Update((MD5_CTX *)state->foo, &buf[0], padlen);
#else
			md5_loop(&buf[0], padlen);
#endif
		}

		buf[0] = (keybitlen >> 0) & 0xff;
		buf[1] = (keybitlen >> 8) & 0xff;
		buf[2] = (keybitlen >> 16) & 0xff;
		buf[3] = (keybitlen >> 24) & 0xff;
#ifdef HAVE_MD5
		MD5Update((MD5_CTX *)state->foo, buf, 8);
#else
		md5_loop(buf, 8);
#endif
	    }
	}
}

static void
ah_keyed_md5_loop(state, addr, len)
	struct ah_algorithm_state *state;
	caddr_t addr;
	size_t len;
{
	if (!state)
		panic("ah_keyed_md5_loop: what?");

#ifdef HAVE_MD5
	MD5Update((MD5_CTX *)state->foo, addr, len);
#else
	md5_loop((u_int8_t *)addr, (u_int)len);
#endif
}

static void
ah_keyed_md5_result(state, addr)
	struct ah_algorithm_state *state;
	caddr_t addr;
{
	u_char digest[16];

	if (!state)
		panic("ah_keyed_md5_result: what?");

	if (state->sa) {
#ifdef HAVE_MD5
		MD5Update((MD5_CTX *)state->foo, 
			(u_int8_t *)_KEYBUF(state->sa->key_auth),
			(u_int)_KEYLEN(state->sa->key_auth));
#else
		md5_loop((u_int8_t *)_KEYBUF(state->sa->key_auth),
			(u_int)_KEYLEN(state->sa->key_auth));
#endif
	}
#ifdef HAVE_MD5
	MD5Final(&digest[0], (MD5_CTX *)state->foo);
	free(state->foo, M_TEMP);
#else
	md5_pad();
	md5_result(&digest[0]);
#endif
	bcopy(&digest[0], (void *)addr, sizeof(digest));
}

static int
ah_keyed_sha1_mature(sa)
	struct secas *sa;
{
	struct ah_algorithm *algo;

	if (!sa->key_auth) {
		printf("esp_keyed_sha1_mature: no key is given.\n");
		return 1;
	}
	algo = &ah_algorithms[sa->alg_auth];
	if (sa->key_auth->sadb_key_bits < algo->keymin
	 || algo->keymax < sa->key_auth->sadb_key_bits) {
		printf("ah_keyed_sha1_mature: invalid key length %d.\n",
			sa->key_auth->sadb_key_bits);
		return 1;
	}

	return 0;
}

static void
ah_keyed_sha1_init(state, sa)
	struct ah_algorithm_state *state;
	struct secas *sa;
{
#ifdef HAVE_SHA1
	SHA1_CTX *ctxt;
#else
	struct sha1_ctxt *ctxt;
#endif

	if (!state)
		panic("ah_keyed_sha1_init: what?");

	state->sa = sa;
#ifdef HAVE_SHA1
	state->foo = (void *)malloc(sizeof(SHA1_CTX), M_TEMP, M_NOWAIT);
#else
	state->foo = (void *)malloc(sizeof(struct sha1_ctxt), M_TEMP, M_NOWAIT);
#endif
	if (!state->foo)
		panic("ah_keyed_sha1_init: what?");

#ifdef HAVE_SHA1
	ctxt = (SHA1_CTX *)state->foo;
	SHA1Init(ctxt);
#else
	ctxt = (struct sha1_ctxt *)state->foo;
	sha1_init(ctxt);
#endif

	if (state->sa) {
#ifdef HAVE_SHA1
		SHA1Update(ctxt, (u_int8_t *)_KEYBUF(state->sa->key_auth),
			(u_int)_KEYLEN(state->sa->key_auth));
#else
		sha1_loop(ctxt, (u_int8_t *)_KEYBUF(state->sa->key_auth),
			(u_int)_KEYLEN(state->sa->key_auth));
#endif

	    {
		/*
		 * Pad after the key.
		 */
		size_t padlen;
		size_t keybitlen;
		u_int8_t buf[32];

		if (_KEYLEN(state->sa->key_auth) < 56)
			padlen = 64 - 8 - _KEYLEN(state->sa->key_auth);
		else
			padlen = 64 + 64 - 8 - _KEYLEN(state->sa->key_auth);
		keybitlen = _KEYLEN(state->sa->key_auth);
		keybitlen *= 8;

		buf[0] = 0x80;
#ifdef HAVE_SHA1
		SHA1Update(ctxt, &buf[0], 1);
#else
		sha1_loop(ctxt, &buf[0], 1);
#endif
		padlen--;

		bzero(buf, sizeof(buf));
		while (sizeof(buf) < padlen) {
#ifdef HAVE_SHA1
			SHA1Update(ctxt, &buf[0], sizeof(buf));
#else
			sha1_loop(ctxt, &buf[0], sizeof(buf));
#endif
			padlen -= sizeof(buf);
		}
		if (padlen) {
#ifdef HAVE_SHA1
			SHA1Update(ctxt, &buf[0], padlen);
#else
			sha1_loop(ctxt, &buf[0], padlen);
#endif
		}

		buf[0] = (keybitlen >> 0) & 0xff;
		buf[1] = (keybitlen >> 8) & 0xff;
		buf[2] = (keybitlen >> 16) & 0xff;
		buf[3] = (keybitlen >> 24) & 0xff;
#ifdef HAVE_SHA1
		SHA1Update(ctxt, buf, 8);
#else
		sha1_loop(ctxt, buf, 8);
#endif
	    }
	}
}

static void
ah_keyed_sha1_loop(state, addr, len)
	struct ah_algorithm_state *state;
	caddr_t addr;
	size_t len;
{
#ifdef HAVE_SHA1
	SHA1_CTX *ctxt;
#else
	struct sha1_ctxt *ctxt;
#endif

	if (!state || !state->foo)
		panic("ah_keyed_sha1_loop: what?");
#ifdef HAVE_SHA1
	ctxt = (SHA1_CTX *)state->foo;
#else
	ctxt = (struct sha1_ctxt *)state->foo;
#endif

#ifdef HAVE_SHA1
	SHA1Update(ctxt, (caddr_t)addr, (size_t)len);
#else
	sha1_loop(ctxt, (caddr_t)addr, (size_t)len);
#endif
}

static void
ah_keyed_sha1_result(state, addr)
	struct ah_algorithm_state *state;
	caddr_t addr;
{
	u_char digest[SHA1_RESULTLEN];	/* SHA-1 generates 160 bits */
#ifdef HAVE_SHA1
	SHA1_CTX *ctxt;
#else
	struct sha1_ctxt *ctxt;
#endif

	if (!state || !state->foo)
		panic("ah_keyed_sha1_result: what?");
#ifdef HAVE_SHA1
	ctxt = (SHA1_CTX *)state->foo;
#else
	ctxt = (struct sha1_ctxt *)state->foo;
#endif

	if (state->sa) {
#ifdef HAVE_SHA1
		SHA1Update(ctxt, (u_int8_t *)_KEYBUF(state->sa->key_auth),
			(u_int)_KEYLEN(state->sa->key_auth));
#else
		sha1_loop(ctxt, (u_int8_t *)_KEYBUF(state->sa->key_auth),
			(u_int)_KEYLEN(state->sa->key_auth));
#endif
	}
#ifdef HAVE_SHA1
	SHA1Final((caddr_t)&digest[0], ctxt);
#else
	sha1_result(ctxt, (caddr_t)&digest[0]);
#endif
	bcopy(&digest[0], (void *)addr, HMACSIZE);

	free(state->foo, M_TEMP);
}

static int
ah_hmac_md5_mature(sa)
	struct secas *sa;
{
	struct ah_algorithm *algo;

	if (!sa->key_auth) {
		printf("esp_hmac_md5_mature: no key is given.\n");
		return 1;
	}
	algo = &ah_algorithms[sa->alg_auth];
	if (sa->key_auth->sadb_key_bits < algo->keymin
	 || algo->keymax < sa->key_auth->sadb_key_bits) {
		printf("ah_hmac_md5_mature: invalid key length %d.\n",
			sa->key_auth->sadb_key_bits);
		return 1;
	}

	return 0;
}

static void
ah_hmac_md5_init(state, sa)
	struct ah_algorithm_state *state;
	struct secas *sa;
{
	u_char *ipad;
	u_char *opad;
	u_char tk[16];
	u_char *key;
	size_t keylen;
	size_t i;
#ifdef HAVE_MD5
	MD5_CTX *ctxt;
#endif

	if (!state)
		panic("ah_hmac_md5_init: what?");

	state->sa = sa;
#ifdef HAVE_MD5
	state->foo = (void *)malloc(64 + 64 + sizeof(MD5_CTX), M_TEMP, M_NOWAIT);
#else
	state->foo = (void *)malloc(64 + 64, M_TEMP, M_NOWAIT);
#endif
	if (!state->foo)
		panic("ah_hmac_md5_init: what?");

	ipad = (u_char *)state->foo;
	opad = (u_char *)(ipad + 64);
#ifdef HAVE_MD5
	ctxt = (MD5_CTX *)(opad + 64);
#endif

	/* compress the key if necessery */
	if (64 < _KEYLEN(state->sa->key_auth)) {
#ifdef HAVE_MD5
		MD5Init(ctxt);
		MD5Update(ctxt, _KEYBUF(state->sa->key_auth),
			_KEYLEN(state->sa->key_auth));
		MD5Final(&tk[0], ctxt);
#else
		md5_init();
		md5_loop(_KEYBUF(state->sa->key_auth),
			_KEYLEN(state->sa->key_auth));
		md5_pad();
		md5_result(&tk[0]);
#endif
		key = &tk[0];
		keylen = 16;
	} else {
		key = _KEYBUF(state->sa->key_auth);
		keylen = _KEYLEN(state->sa->key_auth);
	}

	bzero(ipad, 64);
	bzero(opad, 64);
	bcopy(key, ipad, keylen);
	bcopy(key, opad, keylen);
	for (i = 0; i < 64; i++) {
		ipad[i] ^= 0x36;
		opad[i] ^= 0x5c;
	}

#ifdef HAVE_MD5
	MD5Init(ctxt);
	MD5Update(ctxt, ipad, 64);
#else
	md5_init();
	md5_loop(ipad, 64);
#endif
}

static void
ah_hmac_md5_loop(state, addr, len)
	struct ah_algorithm_state *state;
	caddr_t addr;
	size_t len;
{
#ifdef HAVE_MD5
	MD5_CTX *ctxt;
#endif

	if (!state || !state->foo)
		panic("ah_hmac_md5_loop: what?");
#ifdef HAVE_MD5
	ctxt = (MD5_CTX *)(((caddr_t)state->foo) + 128);
	MD5Update(ctxt, addr, len);
#else
	md5_loop((u_int8_t *)addr, (u_int)len);
#endif
}

static void
ah_hmac_md5_result(state, addr)
	struct ah_algorithm_state *state;
	caddr_t addr;
{
	u_char digest[16];
	u_char *ipad;
	u_char *opad;
#ifdef HAVE_MD5
	MD5_CTX *ctxt;
#endif

	if (!state || !state->foo)
		panic("ah_hmac_md5_result: what?");

	ipad = (u_char *)state->foo;
	opad = (u_char *)(ipad + 64);
#ifdef HAVE_MD5
	ctxt = (MD5_CTX *)(opad + 64);

	MD5Final(&digest[0], ctxt);

	MD5Init(ctxt);
	MD5Update(ctxt, opad, 64);
	MD5Update(ctxt, &digest[0], sizeof(digest));
	MD5Final(&digest[0], ctxt);
#else
	md5_pad();
	md5_result(&digest[0]);

	md5_init();
	md5_loop(opad, 64);
	md5_loop(&digest[0], sizeof(digest));
	md5_pad();
	md5_result(&digest[0]);
#endif

	bcopy(&digest[0], (void *)addr, HMACSIZE);

	free(state->foo, M_TEMP);
}

static int
ah_hmac_sha1_mature(sa)
	struct secas *sa;
{
	struct ah_algorithm *algo;

	if (!sa->key_auth) {
		printf("esp_hmac_sha1_mature: no key is given.\n");
		return 1;
	}
	algo = &ah_algorithms[sa->alg_auth];
	if (sa->key_auth->sadb_key_bits < algo->keymin
	 || algo->keymax < sa->key_auth->sadb_key_bits) {
		printf("ah_hmac_sha1_mature: invalid key length %d.\n",
			sa->key_auth->sadb_key_bits);
		return 1;
	}

	return 0;
}

static void
ah_hmac_sha1_init(state, sa)
	struct ah_algorithm_state *state;
	struct secas *sa;
{
	u_char *ipad;
	u_char *opad;
#ifdef HAVE_SHA1
	SHA1_CTX *ctxt;
#else
	struct sha1_ctxt *ctxt;
#endif
	u_char tk[SHA1_RESULTLEN];	/* SHA-1 generates 160 bits */
	u_char *key;
	size_t keylen;
	size_t i;

	if (!state)
		panic("ah_hmac_sha1_init: what?");

	state->sa = sa;
#ifdef HAVE_SHA1
	state->foo = (void *)malloc(64 + 64 + sizeof(SHA1_CTX),
			M_TEMP, M_NOWAIT);
#else
	state->foo = (void *)malloc(64 + 64 + sizeof(struct sha1_ctxt),
			M_TEMP, M_NOWAIT);
#endif
	if (!state->foo)
		panic("ah_hmac_sha1_init: what?");

	ipad = (u_char *)state->foo;
	opad = (u_char *)(ipad + 64);
#ifdef HAVE_SHA1
	ctxt = (SHA1_CTX *)(opad + 64);
#else
	ctxt = (struct sha1_ctxt *)(opad + 64);
#endif

	/* compress the key if necessery */
	if (64 < _KEYLEN(state->sa->key_auth)) {
#ifdef HAVE_SHA1
		SHA1Init(ctxt);
		SHA1Update(ctxt, _KEYBUF(state->sa->key_auth),
			_KEYLEN(state->sa->key_auth));
		SHA1Final(&tk[0], ctxt);
#else
		sha1_init(ctxt);
		sha1_loop(ctxt, _KEYBUF(state->sa->key_auth),
			_KEYLEN(state->sa->key_auth));
		sha1_result(ctxt, &tk[0]);
#endif
		key = &tk[0];
		keylen = SHA1_RESULTLEN;
	} else {
		key = _KEYBUF(state->sa->key_auth);
		keylen = _KEYLEN(state->sa->key_auth);
	}

	bzero(ipad, 64);
	bzero(opad, 64);
	bcopy(key, ipad, keylen);
	bcopy(key, opad, keylen);
	for (i = 0; i < 64; i++) {
		ipad[i] ^= 0x36;
		opad[i] ^= 0x5c;
	}

#ifdef HAVE_SHA1
	SHA1Init(ctxt);
	SHA1Update(ctxt, ipad, 64);
#else
	sha1_init(ctxt);
	sha1_loop(ctxt, ipad, 64);
#endif
}

static void
ah_hmac_sha1_loop(state, addr, len)
	struct ah_algorithm_state *state;
	caddr_t addr;
	size_t len;
{
#ifdef HAVE_SHA1
	SHA1_CTX *ctxt;
#else
	struct sha1_ctxt *ctxt;
#endif

	if (!state || !state->foo)
		panic("ah_hmac_sha1_loop: what?");

#ifdef HAVE_SHA1
	ctxt = (SHA1_CTX *)(((u_char *)state->foo) + 128);
	SHA1Update(ctxt, (caddr_t)addr, (size_t)len);
#else
	ctxt = (struct sha1_ctxt *)(((u_char *)state->foo) + 128);
	sha1_loop(ctxt, (caddr_t)addr, (size_t)len);
#endif
}

static void
ah_hmac_sha1_result(state, addr)
	struct ah_algorithm_state *state;
	caddr_t addr;
{
	u_char digest[SHA1_RESULTLEN];	/* SHA-1 generates 160 bits */
	u_char *ipad;
	u_char *opad;
#ifdef HAVE_SHA1
	SHA1_CTX *ctxt;
#else
	struct sha1_ctxt *ctxt;
#endif

	if (!state || !state->foo)
		panic("ah_hmac_sha1_result: what?");

	ipad = (u_char *)state->foo;
	opad = (u_char *)(ipad + 64);
#ifdef HAVE_SHA1
	ctxt = (SHA1_CTX *)(opad + 64);

	SHA1Final((caddr_t)&digest[0], ctxt);

	SHA1Init(ctxt);
	SHA1Update(ctxt, opad, 64);
	SHA1Update(ctxt, (caddr_t)&digest[0], sizeof(digest));
	SHA1Final((caddr_t)&digest[0], ctxt);
#else
	ctxt = (struct sha1_ctxt *)(opad + 64);

	sha1_result(ctxt, (caddr_t)&digest[0]);

	sha1_init(ctxt);
	sha1_loop(ctxt, opad, 64);
	sha1_loop(ctxt, (caddr_t)&digest[0], sizeof(digest));
	sha1_result(ctxt, (caddr_t)&digest[0]);
#endif

	bcopy(&digest[0], (void *)addr, HMACSIZE);

	free(state->foo, M_TEMP);
}

/*------------------------------------------------------------*/

/*
 * go generate the checksum.
 */
int
ah4_calccksum(m0, ahdat, algo, sa)
	struct mbuf *m0;
	caddr_t ahdat;
	struct ah_algorithm *algo;
	struct secas *sa;
{
	struct mbuf *m;
	int hdrtype;
	u_char *p;
	size_t advancewidth;
	struct ah_algorithm_state algos;
	int tlen;
	u_char sumbuf[AH_MAXSUMSIZE];
	int error = 0;

	hdrtype = -1;	/*dummy, it is called IPPROTO_IP*/

	m = m0;

	p = mtod(m, u_char *);

	(algo->init)(&algos, sa);

	advancewidth = 0;	/*safety*/

again:
	/* gory. */
	switch (hdrtype) {
	case -1:	/*first one*/
	    {
		/*
		 * copy ip hdr, modify to fit the AH checksum rule,
		 * then take a checksum.
		 * XXX need to care about source routing... jesus.
		 */
		struct ip iphdr;
		size_t hlen;

		bcopy((caddr_t)p, (caddr_t)&iphdr, sizeof(struct ip));
#ifdef _IP_VHL
		hlen = IP_VHL_HL(iphdr.ip_vhl) << 2;
#else
		hlen = iphdr.ip_hl << 2;
#endif
		iphdr.ip_ttl = 0;
		iphdr.ip_sum = htons(0);
		if (ip4_ah_cleartos) iphdr.ip_tos = 0;
		iphdr.ip_off = htons(ntohs(iphdr.ip_off) & ip4_ah_offsetmask);
		(algo->update)(&algos, (caddr_t)&iphdr, sizeof(struct ip));

		if (hlen != sizeof(struct ip)) {
			u_char *p;
			int i, j;
			int l, skip;
			u_char dummy[4];

			/*
			 * IP options processing.
			 * See RFC2402 appendix A.
			 */
			bzero(dummy, sizeof(dummy));
			p = mtod(m, u_char *);
			i = sizeof(struct ip);
			while (i < hlen) {
				skip = 1;
				switch (p[i + IPOPT_OPTVAL]) {
				case IPOPT_EOL:
				case IPOPT_NOP:
					l = 1;
					skip = 0;
					break;
				case IPOPT_SECURITY:	/* 0x82 */
				case 0x85:	/* Extended security */
				case 0x86:	/* Commercial security */
				case 0x94:	/* Router alert */
				case 0x95:	/* RFC1770 */
					l = p[i + IPOPT_OLEN];
					skip = 0;
					break;
				default:
					l = p[i + IPOPT_OLEN];
					skip = 1;
					break;
				}
				if (l <= 0 || hlen - i < l) {
					printf("ah4_input: invalid IP option "
						"(type=%02x len=%02x)\n",
						p[i + IPOPT_OPTVAL],
						p[i + IPOPT_OLEN]);
					break;
				}
				if (skip) {
					for (j = 0; j < l / sizeof(dummy); j++)
						(algo->update)(&algos, dummy, sizeof(dummy));

					(algo->update)(&algos, dummy, l % sizeof(dummy));
				} else
					(algo->update)(&algos, p + i, l);
				if (p[i + IPOPT_OPTVAL] == IPOPT_EOL)
					break;
				i += l;
			}
		}

		hdrtype = (iphdr.ip_p) & 0xff;
		advancewidth = hlen;
		break;
	    }

	case IPPROTO_AH:
	    {
		u_char dummy[4];
		int siz;
		int hdrsiz;

		hdrsiz = (sa->flags & SADB_X_EXT_OLD) ?
				sizeof(struct ah) : sizeof(struct newah);

		(algo->update)(&algos, p, hdrsiz);

		/* key data region. */
		siz = (*algo->sumsiz)(sa);
		bzero(&dummy[0], sizeof(dummy));
		while (sizeof(dummy) <= siz) {
			(algo->update)(&algos, dummy, sizeof(dummy));
			siz -= sizeof(dummy);
		}
		/* can't happen, but just in case */
		if (siz)
			(algo->update)(&algos, dummy, siz);

		/* padding region, just in case */
		siz = (((struct ah *)p)->ah_len << 2) - (*algo->sumsiz)(sa);
		if ((sa->flags & SADB_X_EXT_OLD) == 0)
			siz -= 4;		/* sequence number field */
		if (0 < siz) {
			/* RFC 1826 */
			(algo->update)(&algos, p + hdrsiz + (*algo->sumsiz)(sa),
				siz);
		}

		hdrtype = ((struct ah *)p)->ah_nxt;
		advancewidth = hdrsiz;
		advancewidth += ((struct ah *)p)->ah_len << 2;
		if ((sa->flags & SADB_X_EXT_OLD) == 0)
			advancewidth -= 4;	/* sequence number field */
		break;
	    }

	default:
		printf("ah4_calccksum: unexpected hdrtype=%x; "
			"treating rest as payload\n", hdrtype);
		/*fall through*/
	case IPPROTO_ICMP:
	case IPPROTO_IGMP:
	case IPPROTO_IPIP:
#ifdef INET6
	case IPPROTO_IPV6:
	case IPPROTO_ICMPV6:
#endif
	case IPPROTO_UDP:
	case IPPROTO_TCP:
	case IPPROTO_ESP:
		while (m) {
			tlen = m->m_len - (p - mtod(m, u_char *));
			(algo->update)(&algos, p, tlen);
			m = m->m_next;
			p = m ? mtod(m, u_char *) : NULL;
		}
		
		advancewidth = 0;	/*loop finished*/
		break;
	}

	if (advancewidth) {
		/* is it safe? */
		while (m && advancewidth) {
			tlen = m->m_len - (p - mtod(m, u_char *));
			if (advancewidth < tlen) {
				p += advancewidth;
				advancewidth = 0;
			} else {
				advancewidth -= tlen;
				m = m->m_next;
				if (m)
					p = mtod(m, u_char *);
				else {
					printf("ERR: hit the end-of-mbuf...\n");
					p = NULL;
				}
			}
		}

		if (m)
			goto again;
	}

	/* for HMAC algorithms... */
	(algo->result)(&algos, &sumbuf[0]);
	bcopy(&sumbuf[0], ahdat, (*algo->sumsiz)(sa));

	return error;
}

#ifdef INET6
/*
 * go generate the checksum. This function won't modify the mbuf chain
 * except AH itself.
 */
int
ah6_calccksum(m0, ahdat, algo, sa)
	struct mbuf *m0;
	caddr_t ahdat;
	struct ah_algorithm *algo;
	struct secas *sa;
{
	struct mbuf *m;
	int hdrtype;
	u_char *p;
	size_t advancewidth;
	struct ah_algorithm_state algos;
	int tlen;
	int error = 0;
	u_char sumbuf[AH_MAXSUMSIZE];
	int nest;

	hdrtype = -1;	/*dummy, it is called IPPROTO_IPV6 */

	m = m0;

	p = mtod(m, u_char *);

	(algo->init)(&algos, sa);

	advancewidth = 0;	/*safety*/
	nest = 0;

again:
	if (ip6_hdrnestlimit && (++nest > ip6_hdrnestlimit)) {
		ip6stat.ip6s_toomanyhdr++;
		error = EINVAL;	/*XXX*/
		goto bad;
	}

	/* gory. */
	switch (hdrtype) {
	case -1:	/*first one*/
	    {
		struct ip6_hdr ip6copy;

		bcopy(p, &ip6copy, sizeof(struct ip6_hdr));
		/* RFC2402 */
		ip6copy.ip6_flow = 0;
		ip6copy.ip6_vfc &= ~IPV6_VERSION_MASK;
		ip6copy.ip6_vfc |= IPV6_VERSION;
		ip6copy.ip6_hlim = 0;
		if (IN6_IS_ADDR_LINKLOCAL(&ip6copy.ip6_src))
			ip6copy.ip6_src.s6_addr16[1] = 0x0000;
		if (IN6_IS_ADDR_LINKLOCAL(&ip6copy.ip6_dst))
			ip6copy.ip6_dst.s6_addr16[1] = 0x0000;
		(algo->update)(&algos, (caddr_t)&ip6copy,
			       sizeof(struct ip6_hdr));
		hdrtype = (((struct ip6_hdr *)p)->ip6_nxt) & 0xff;
		advancewidth = sizeof(struct ip6_hdr);
		break;
	    }

	case IPPROTO_AH:
	    {
		u_char dummy[4];
		int siz;
		int hdrsiz;

		hdrsiz = (sa->flags & SADB_X_EXT_OLD) ?
				sizeof(struct ah) : sizeof(struct newah);

		(algo->update)(&algos, p, hdrsiz);

		/* key data region. */
		siz = (*algo->sumsiz)(sa);
		bzero(&dummy[0], 4);
		while (4 <= siz) {
			(algo->update)(&algos, dummy, 4);
			siz -= 4;
		}
		/* can't happen, but just in case */
		if (siz)	
			(algo->update)(&algos, dummy, siz);

		/* padding region, just in case */
		siz = (((struct ah *)p)->ah_len << 2) - (*algo->sumsiz)(sa);
		if ((sa->flags & SADB_X_EXT_OLD) == 0)
			siz -= 4;		/* sequence number field */
		if (0 < siz) {
			(algo->update)(&algos, p + hdrsiz + (*algo->sumsiz)(sa),
				siz);
		}

		hdrtype = ((struct ah *)p)->ah_nxt;
		advancewidth = hdrsiz;
		advancewidth += ((struct ah *)p)->ah_len << 2;
		if ((sa->flags & SADB_X_EXT_OLD) == 0)
			advancewidth -= 4;	/* sequence number field */
		break;
	    }

	 case IPPROTO_HOPOPTS:
	 case IPPROTO_DSTOPTS:
	 {
		 int hdrlen, optlen;
		 u_int8_t *optp, *lastp = p, *optend, opt;

		 tlen = m->m_len - (p - mtod(m, u_char *));
		 /* We assume all the options is contained in a single mbuf */
		 if (tlen < sizeof(struct ip6_ext)) {
			 error = EINVAL;
			 goto bad;
		 }
		 hdrlen  = (((struct ip6_ext *)p)->ip6e_len + 1) << 3;
		 hdrtype = (int)((struct ip6_ext *)p)->ip6e_nxt;
		 if (tlen < hdrlen) {
			 error = EINVAL;
			 goto bad;
		 }
		 optend = p + hdrlen;

		 /*
		  * ICV calculation for the options header including all
		  * options. This part is a little tricky since there are
		  * two type of options; mutable and immutable. Our approach
		  * is to calculate ICV for a consecutive immutable options
		  * at once. Here is an example. In the following figure,
		  * suppose that we've calculated ICV from the top of the
		  * header to MutableOpt1, which is a mutable option.
		  * lastp points to the end of MutableOpt1. Some immutable
		  * options follows MutableOpt1, and we encounter a new
		  * mutable option; MutableOpt2. optp points to the head
		  * of MutableOpt2. In this situation, uncalculated immutable
		  * field is the field from lastp to optp+2 (note that the
		  * type and the length fields are considered as immutable
		  * even in a mutable option). So we first calculate ICV
		  * for the field as immutable, then calculate from optp+2
		  * to the end of MutableOpt2, whose length is optlen-2,
		  * where optlen is the length of MutableOpt2. Finally,
		  * lastp is updated to point to the end of MutableOpt2
		  * for further calculation. The updated point is shown as
		  * lastp' in the figure.
		  *                                <------ optlen ----->
		  * -----------+-------------------+---+---+-----------+
		  * MutableOpt1|ImmutableOptions...|typ|len|MutableOpt2|
		  * -----------+-------------------+---+---+-----------+
		  *            ^                   ^       ^
		  *            lastp               optp    optp+2
		  *            <---- optp + 2 - lastp -----><-optlen-2->
		  *                                                    ^
		  *                                                    lastp'
		  */
		 for (optp = p + 2; optp < optend; optp += optlen) {
			 opt = optp[0];
			 if (opt == IP6OPT_PAD1) {
				 optlen = 1;
			 } else {
				 if (optp + 2 > optend) {
					 error = EINVAL; /* malformed option */
					 goto bad;
				 }
				 optlen = optp[1] + 2;
				 if (opt & IP6OPT_MUTABLE) {
					 /*
					  * ICV calc. for the (consecutive)
					  * immutable field followd by the
					  * option.
					  */
					 (algo->update)(&algos, lastp,
							optp + 2 - lastp);
					 if (optlen - 2 > ZEROBUFLEN) {
						 error = EINVAL; /* XXX */
						 goto bad;
					 }
					 /*
					  * ICV calc. for the mutable
					  * option using an all-0 buffer.
					  */
					 (algo->update)(&algos, zerobuf,
							optlen - 2);
					 lastp = optp + optlen;
				 }
			 }
		 }
		 /*
		  * Wrap up the calulation; compute ICV for the consecutive
		  * immutable options at the end of the header(if any).
		  */
		 (algo->update)(&algos, lastp, p + hdrlen - lastp);
		 advancewidth = hdrlen;
		 break;
	 }
	 case IPPROTO_ROUTING:
	 {
		 /*
		  * For an input packet, we can just calculate `as is'.
		  * For an output packet, we assume ip6_output have already
		  * made packet how it will be received at the final destination.
		  * So we'll only check if the header is malformed.
		  */
		 int hdrlen;

		 tlen = m->m_len - (p - mtod(m, u_char *));
		 /* We assume all the options is contained in a single mbuf */
		 if (tlen < sizeof(struct ip6_ext)) {
			 error = EINVAL;
			 goto bad;
		 }
		 hdrlen  = (((struct ip6_ext *)p)->ip6e_len + 1) << 3;
		 hdrtype = (int)((struct ip6_ext *)p)->ip6e_nxt;
		 if (tlen < hdrlen) {
			 error = EINVAL;
			 goto bad;
		 }
		 advancewidth = hdrlen;
		 (algo->update)(&algos, p, hdrlen);
		 break;
	 }
	default:
		printf("ah6_calccksum: unexpected hdrtype=%x; "
			"treating rest as payload\n", hdrtype);
		/*fall through*/
	case IPPROTO_ICMP:
	case IPPROTO_IGMP:
	case IPPROTO_IPIP:	/*?*/
	case IPPROTO_IPV6:
	case IPPROTO_ICMPV6:
	case IPPROTO_UDP:
	case IPPROTO_TCP:
	case IPPROTO_ESP:
		while (m) {
			tlen = m->m_len - (p - mtod(m, u_char *));
			(algo->update)(&algos, p, tlen);
			m = m->m_next;
			p = m ? mtod(m, u_char *) : NULL;
		}
		
		advancewidth = 0;	/*loop finished*/
		break;
	}

	if (advancewidth) {
		/* is it safe? */
		while (m && advancewidth) {
			tlen = m->m_len - (p - mtod(m, u_char *));
			if (advancewidth < tlen) {
				p += advancewidth;
				advancewidth = 0;
			} else {
				advancewidth -= tlen;
				m = m->m_next;
				if (m)
					p = mtod(m, u_char *);
				else {
					printf("ERR: hit the end-of-mbuf...\n");
					p = NULL;
				}
			}
		}

		if (m)
			goto again;
	}

	/* for HMAC algorithms... */
	(algo->result)(&algos, &sumbuf[0]);
	bcopy(&sumbuf[0], ahdat, (*algo->sumsiz)(sa));

	return(0);

  bad:
	return(error);
}
#endif
