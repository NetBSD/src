/* $NetBSD: params.c,v 1.2 2002/10/12 21:02:18 elric Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Roland C. Dowdeswell.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD");
#endif

#include <sys/types.h>

#include <errno.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <util.h>

/* include the resolver gunk in order that we can use b64 routines */
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>

#include "params.h"
#include "utils.h"

static int	params_setstring(char **, const char *);
static int	params_setbinary(u_int8_t **, int *, const char *, int);
static int	params_setb64(u_int8_t **, int *, const char *);

static void	eatwhite(char *);
static int	take_action(struct params *, FILE *, const char *, char *);
static void	print_kvpair_str(FILE *, const char *, const char *);
static void	print_kvpair_int(FILE *, const char *, int);
static void	print_kvpair_b64(FILE *, const char *, const char *, int);

static void	free_notnull(void *);

/* crypt defaults functions */
#define CRYPT_DEFAULTKEYSIZE	0x01

static int	crypt_int_lookup(char *, int);
static int	crypt_int_lookup_aes_cbc(int);
static int	crypt_int_lookup_3des_cbc(int);
static int	crypt_int_lookup_blowfish_cbc(int);

void
params_init(struct params *p)
{

	p->alg = NULL;
	p->ivmeth = NULL;
	p->key = NULL;
	p->keylen = -1;
	p->bsize = -1;
	p->keygen_method = KEYGEN_UNKNOWN;
	p->keygen_salt = NULL;
	p->keygen_saltlen = -1;
	p->keygen_iterations = -1;
	p->verify_method = VERIFY_UNKNOWN;
	p->key_hash = NULL;
	p->key_hashlen = -1;
	p->xor_key = NULL;
	p->xor_keylen = -1;
}

void
params_free(struct params *p)
{

	free_notnull(p->alg);
	free_notnull(p->ivmeth);
	free_notnull(p->keygen_salt);
	free_notnull(p->key_hash);
	free_notnull(p->xor_key);
}

int
params_filldefaults(struct params *p)
{

	if (p->keygen_method == KEYGEN_UNKNOWN)
		p->keygen_method = KEYGEN_PKCS5_PBKDF2;
	if (p->verify_method == VERIFY_UNKNOWN)
		p->verify_method = VERIFY_NONE;
	if (!p->ivmeth)
		params_setivmeth(p, "encblkno");
	if (p->keylen == -1)
		p->keylen = crypt_int_lookup(p->alg, CRYPT_DEFAULTKEYSIZE);
	if (p->keylen == -1) {
		fprintf(stderr, "Could not determine key length\n");
		return -1;
	}
	if (p->keygen_iterations < 1)
		p->keygen_iterations = 128;
	return 0;
}

int
params_changed(const struct params *c)
{

	if (c->alg || c->ivmeth || c->key || c->keylen != -1 ||
	    c->bsize != -1 || c->keygen_method || c->keygen_salt ||
	    c->keygen_saltlen != -1 || c->keygen_iterations != -1 ||
	    c->key_hash)
		return 1;
	return 0;
}

static int
params_setstring(char **s, const char *in)
{

	free_notnull(*s);
	*s = strdup(in);
	if (!in)
		return -1;
	return 0;
}

/*
 * params_setbinary allocates a buffer of at least len bits and
 * fills it in.  It returns the number of bits in *l and the buffer
 * in *s.
 */

static int
params_setbinary(u_int8_t **s, int *l, const char *in, int len)
{

	*l = len;
	len = BITS2BYTES(len);
	*s = malloc(len);
	if (!*s)
		return -1;
	memcpy(*s, in, len);
	return 0;
}

/*
 * params_setb64 reads an encoded base64 stream.  We interpret
 * the first 32 bits as an unsigned integer in network byte order
 * specifying the number of bits in the stream.
 */

static int
params_setb64(u_int8_t **s, int *l, const char *in)
{
	int	 len;
	int	 nbits;
	char	*tmp;

	len = strlen(in);
	tmp = malloc(len);
	if (!tmp)
		return -1;

	len = __b64_pton(in, tmp, len);

	if (len == -1) {
		fprintf(stderr, "params_setb64: mangled base64 stream\n");
		return -1;
	}

	nbits = ntohl(*((u_int32_t *)tmp));
	if (nbits > (len - 4) * 8) {
		fprintf(stderr, "params_setb64: encoded bits claim to be "
		    "longer than they are (nbits=%u, stream len=%u bytes)\n",
		    (unsigned)nbits, (unsigned)len);
		return -1;
	}

	*s = malloc(BITS2BYTES(nbits));
	if (!*s) {
		free(tmp);
		return -1;
	}

	memcpy(*s, tmp + 4, BITS2BYTES(nbits));
	
	*l = nbits;
	return *l;
}

int
params_setalgorithm(struct params *p, const char *in)
{

	return params_setstring(&p->alg, in);
}

int
params_setivmeth(struct params *p, const char *in)
{

	return params_setstring(&p->ivmeth, in);
}

int
params_setkeylen(struct params *p, int keylen)
{

	if (!keylen) {
		fprintf(stderr, "zero keylen not permitted\n");
		return -1;
	}
	p->keylen = keylen;
	return 0;
}

int
params_setbsize(struct params *p, int bsize)
{

	if (!bsize) {
		fprintf(stderr, "zero blocksize not permitted\n");
		return -1;
	}
	p->bsize = bsize;
	return 0;
}

int
params_setkeygen_method(struct params *p, int in)
{

	switch (in) {
	case KEYGEN_RANDOMKEY:
	case KEYGEN_PKCS5_PBKDF2:
		break;
	default:
		fprintf(stderr, "params_setkeygen_method: unsupported "
		    "keygen_method (%d)\n", in);
		return -1;
	}

	p->keygen_method = in;
	return 0;
}

int
params_setkeygen_method_str(struct params *p, const char *in)
{

	if (!strcmp("pkcs5_pbkdf2", in))
		return params_setkeygen_method(p, KEYGEN_PKCS5_PBKDF2);
	if (!strcmp("randomkey", in))
		return params_setkeygen_method(p, KEYGEN_RANDOMKEY);

	fprintf(stderr, "unrecognized key generation method \"%s\"\n", in);
	return -1;
}

int
params_setkeygen_salt(struct params *p, const char *in, int len)
{

	return params_setbinary(&p->keygen_salt, &p->keygen_saltlen, in, len);
}

int
params_setkeygen_salt_b64(struct params *p, const char *in)
{

	return params_setb64(&p->keygen_salt, &p->keygen_saltlen, in);
}

int
params_setverify_method(struct params *p, int in)
{

	switch (in) {
	case VERIFY_NONE:
	case VERIFY_DISKLABEL:
		break;
	default:
		fprintf(stderr, "params_setverify_method: unsupported "
		    "verify_method (%d)\n", in);
		return -1;
	}
	p->verify_method = in;
	return 0;
}

int
params_setverify_method_str(struct params *p, const char *in)
{

	if (!strcmp("none", in))
		return params_setverify_method(p, VERIFY_NONE);
	if (!strcmp("disklabel", in))
		return params_setverify_method(p, VERIFY_DISKLABEL);

	fprintf(stderr, "params_setverify_method: unrecognized verify method "
	    "\"%s\"\n", in);
	return -1;
}

int
params_setxor_key(struct params *p, const char *in, int len)
{

	return params_setbinary(&p->xor_key, &p->xor_keylen, in, len);
}

int
params_setxor_key_b64(struct params *p, const char *in)
{

	return params_setb64(&p->xor_key, &p->xor_keylen, in);
}

int
params_setkey_hash(struct params *p, const char *in, int len)
{

	return params_setbinary(&p->key_hash, &p->key_hashlen, in, len);
}

int
params_setkey_hash_b64(struct params *p, const char *in)
{

	return params_setb64(&p->key_hash, &p->key_hashlen, in);
}

/* eatwhite simply removes all the whitespace from a string, in line */
static void
eatwhite(char *s)
{
	int	i, j;

	for (i=0,j=0; s[i]; i++)
		if (s[i] != ' ' && s[i] != '\t' && s[i] != '\n')
			s[j++] = s[i];
	s[j++] = '\0';
}

static int
take_action(struct params *c, FILE *f, const char *key, char *val)
{
	int	ret;

	eatwhite(val);

	if (!strcmp(key, "algorithm")) {
		return params_setalgorithm(c, val);
	} else if (!strcmp(key, "iv-method")) {
		return params_setivmeth(c, val);
	} else if (!strcmp(key, "keylength")) {
		return params_setkeylen(c, atoi(val));
	} else if (!strcmp(key, "blocksize")) {
		return params_setbsize(c, atoi(val));
	} else if (!strcmp(key, "keygen_method")) {
		return params_setkeygen_method_str(c, val);
	} else if (!strcmp(key, "keygen_salt")) {
		ret = params_setkeygen_salt_b64(c, val);
		if (ret < 0) {
			fprintf(stderr, "keygen_salt improperly encoded\n");
			return -1;
		}
	} else if (!strcmp(key, "xor_key")) {
		ret = params_setxor_key_b64(c, val);
		if (ret < 0) {
			fprintf(stderr, "xor_key improperly encoded\n");
			return -1;
		}
	} else if (!strcmp(key, "verify_method")) {
		return params_setverify_method_str(c, val);
	} else if (!strcmp(key, "key_hash")) {
		ret = params_setkey_hash_b64(c, val);
		if (ret < 0) {
			fprintf(stderr, "key_hash improperly encoded\n");
			return -1;
		}
	} else {
		fprintf(stderr, "unrecognised keyword (%s, %s)\n", key, val);
		return -1;
	}
	return 0;
}

int
params_fget(struct params *p, FILE *f)
{
	size_t	 len;
	size_t	 lineno;
	int	 ret;
	char	*line;
	char	*val;

	lineno = 0;
	for (;;) {
		line = fparseln(f, &len, &lineno, "\\\\#", FPARSELN_UNESCALL);
		if (!line)
			break;
		if (!*line)
			continue;

		/*
		 * our parameters file has two tokens per line,
		 * so we cut it up that way and ignore other cases.
		 */
		val = strpbrk(line, " \t");
		if (!val) {
			fprintf(stderr, "syntax error on line %lu\n",
			    (u_long)lineno);
			return -1;
		}
		*val++ = '\0';

		ret = take_action(p, f, line, val);
		if (ret) {
			fprintf(stderr, "parse failure on line %lu\n",
			    (u_long)lineno);
			return -1;
		}
	}
	return 0;
}

int
params_cget(struct params *p, const char *fn)
{
	FILE	*f;

	f = fopen(fn, "r");
	if (!f) {
		fprintf(stderr, "failed to open params file \"%s\": %s\n",
		    fn, strerror(errno));
		return -1;
	}
	return params_fget(p, f);
}

static void
print_kvpair_str(FILE *f, const char *key, const char *val)
{

	if (key && val)
		fprintf(f, "%-15.15s%s\n", key, val);
}

static void
print_kvpair_int(FILE *f, const char *key, int val)
{

	if (key && val != -1)
		fprintf(f, "%-15.15s%d\n", key, val);
}

/*
 * prints out a base64 encoded k-v pair to f.  It encodes the length
 * of the bitstream as a 32bit unsigned integer in network byte order
 * up front.
 */

static void
print_kvpair_b64(FILE *f, const char *key, const char *val, int vallen)
{
	int	 col;
	int	 i;
	int	 len;
	char	*out;
	char	*tmp;

	if (!key || !val || vallen == -1)
		return;

	/* compute the total size of the input stream */
	len = BITS2BYTES(vallen) + 4;

	tmp = malloc(len);
	out = malloc(len * 2);
	/* XXXrcd: errors ? */
	if (!tmp || !out)
		abort();	/* lame error handling here... */

	/* stuff the length up front */
	*((u_int32_t *)tmp) = htonl(vallen);
	memcpy(tmp + 4, val, len - 4);

	len = __b64_ntop(tmp, len, out, len * 2);
	free(tmp);

	fprintf(f, "%-15.15s", key);
	col = 0;
	for (i=0; i < len; i++) {
		fputc(out[i], f);
		if (col++ > 50) {
			fprintf(f, " \\\n%-15.15s", "");
			col = 0;
		}
	}
	fprintf(f, "\n");
	free(out);
}

int
params_fput(struct params *p, FILE *f)
{

	print_kvpair_str(f, "algorithm", p->alg);
	print_kvpair_str(f, "iv-method", p->ivmeth);
	print_kvpair_int(f, "keylength", p->keylen);
	print_kvpair_int(f, "blocksize", p->bsize);
	switch (p->verify_method) {
	case VERIFY_NONE:
		print_kvpair_str(f, "verify_method", "none");
		break;
	case VERIFY_DISKLABEL:
		print_kvpair_str(f, "verify_method", "disklabel");
		break;
	default:
		fprintf(stderr, "unsupported verify_method (%d)\n",
		    p->verify_method);
		return -1;
	}
	switch (p->keygen_method) {
	case KEYGEN_RANDOMKEY:
		print_kvpair_str(f, "keygen_method", "randomkey");
		break;
	case KEYGEN_PKCS5_PBKDF2:
		print_kvpair_str(f, "keygen_method", "pkcs5_pbkdf2");
		print_kvpair_b64(f, "keygen_salt", p->keygen_salt,
		    p->keygen_saltlen);
		print_kvpair_b64(f, "xor_key", p->xor_key, p->xor_keylen);
		print_kvpair_b64(f, "key_hash", p->key_hash, p->key_hashlen);
		break;
	default:
		fprintf(stderr, "unsupported keygen_method (%d)\n",
		    p->keygen_method);
		return -1;
	}
	return 0;
}

static int
crypt_int_lookup(char *alg, int type)
{

	if (!strcmp(alg, "aes-cbc"))
		return crypt_int_lookup_aes_cbc(type);
	if (!strcmp(alg, "3des-cbc"))
		return crypt_int_lookup_3des_cbc(type);
	if (!strcmp(alg, "blowfish-cbc"))
		return crypt_int_lookup_blowfish_cbc(type);

	return -1;
}

static int
crypt_int_lookup_aes_cbc(int type)
{

	switch (type) {
	case CRYPT_DEFAULTKEYSIZE:
		return 256;
	}

	return -1;
}

static int
crypt_int_lookup_3des_cbc(int type)
{

	switch (type) {
	case CRYPT_DEFAULTKEYSIZE:
		return 192;
	}

	return -1;
}

static int
crypt_int_lookup_blowfish_cbc(int type)
{

	switch (type) {
	case CRYPT_DEFAULTKEYSIZE:
		return 128;
	}

	return -1;
}

static void
free_notnull(void *mem)
{

	if (!mem)
		free(mem);
}
