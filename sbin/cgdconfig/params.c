/* $NetBSD: params.c,v 1.26 2015/06/16 23:18:54 christos Exp $ */

/*-
 * Copyright (c) 2002, 2003 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: params.c,v 1.26 2015/06/16 23:18:54 christos Exp $");
#endif

#include <sys/types.h>

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <util.h>

#include "params.h"
#include "pkcs5_pbkdf2.h"
#include "utils.h"
#include "extern.h"

static void	params_init(struct params *);

static void	print_kvpair_cstr(FILE *, int, const char *, const char *);
static void	print_kvpair_string(FILE *, int, const char *, const string_t *);
static void	print_kvpair_int(FILE *, int, const char *, size_t);
static void	print_kvpair_b64(FILE *, int, int, const char *, bits_t *);

static void	spaces(FILE *, int);

/* keygen defaults */
#define DEFAULT_SALTLEN		128
#define DEFAULT_ITERATION_TIME	2000000		/* 1 second in microseconds */

/* crypto defaults functions */
static struct crypto_defaults {
	char	alg[32];
	int	keylen;
} crypto_defaults[] = {
	{ "aes-cbc",		128 },
	{ "3des-cbc",		192 },
	{ "blowfish-cbc",	128 }
};

static int	crypt_defaults_lookup(const char *);

struct params *
params_new(void)
{
	struct params	*p;

	p = emalloc(sizeof(*p));
	params_init(p);
	return p;
}

static void
params_init(struct params *p)
{

	p->algorithm = NULL;
	p->ivmeth = NULL;
	p->key = NULL;
	p->keylen = (size_t)-1;
	p->bsize = (size_t)-1;
	p->verify_method = VERIFY_UNKNOWN;
	p->dep_keygen = NULL;
	p->keygen = NULL;
}

void
params_free(struct params *p)
{

	if (!p)
		return;
	string_free(p->algorithm);
	string_free(p->ivmeth);
	keygen_free(p->dep_keygen);
	keygen_free(p->keygen);
}

struct params *
params_combine(struct params *p1, struct params *p2)
{
	struct params *p;

	if (p1)
		p = p1;
	else
		p = params_new();

	if (!p2)
		return p;

	if (p2->algorithm)
		string_assign(&p->algorithm, p2->algorithm);
	if (p2->ivmeth)
		string_assign(&p->ivmeth, p2->ivmeth);
	if (p2->keylen != (size_t)-1)
		p->keylen = p2->keylen;
	if (p2->bsize != (size_t)-1)
		p->bsize = p2->bsize;
	if (p2->verify_method != VERIFY_UNKNOWN)
		p->verify_method = p2->verify_method;

	p->dep_keygen = keygen_combine(p->dep_keygen, p2->dep_keygen);
	keygen_addlist(&p->keygen, p2->keygen);

	/*
	 * at this point we should have moved all allocated data
	 * in p2 into p, so we can free it.
	 */
	free(p2);
	return p;
}

int
params_filldefaults(struct params *p)
{
	size_t	i;

	if (p->verify_method == VERIFY_UNKNOWN)
		p->verify_method = VERIFY_NONE;
	if (!p->ivmeth)
		p->ivmeth = string_fromcharstar("encblkno1");
	if (p->keylen == (size_t)-1) {
		i = crypt_defaults_lookup(string_tocharstar(p->algorithm));
		if (i != (size_t)-1) {
			p->keylen = crypto_defaults[i].keylen;
		} else {
			warnx("could not determine key length for unknown "
			    "algorithm \"%s\"",
			    string_tocharstar(p->algorithm));
			return -1;
		}
	}
	return 0;
}

/*
 * params_verify traverses the parameters and all of the keygen methods
 * looking for inconsistencies.  It outputs warnings on non-fatal errors
 * such as unknown encryption methods, but returns failure on fatal
 * conditions such as a PKCS5_PBKDF2 keygen without a salt.  It is intended
 * to run before key generation.
 */

int
params_verify(const struct params *p)
{
	static const char *encblkno[] = {
	    "encblkno", "encblkno1", "encblkno8"
	};
	static size_t i;
	const char *meth;

	if (!p->algorithm) {
		warnx("unspecified algorithm");
		return 0;
	}
	/*
	 * we only warn for the encryption method so that it is possible
	 * to use an older cgdconfig(8) with a new kernel that supports
	 * additional crypto algorithms.
	 */
	if (crypt_defaults_lookup(string_tocharstar(p->algorithm)) == -1)
		warnx("unknown algorithm \"%s\"(warning)",
		    string_tocharstar(p->algorithm));
	/* same rationale with IV methods. */
	if (!p->ivmeth) {
		warnx("unspecified IV method");
		return 0;
	}

	meth = string_tocharstar(p->ivmeth);
	for (i = 0; i < __arraycount(encblkno); i++)
		if (strcmp(encblkno[i], meth) == 0)
			break;

	if (i == __arraycount(encblkno))
		warnx("unknown IV method \"%s\" (warning)", meth);

	if (p->keylen == (size_t)-1) {
		warnx("unspecified key length");
		return 0;
	}

	return keygen_verify(p->keygen);
}

struct params *
params_algorithm(string_t *in)
{
	struct params *p = params_new();

	p->algorithm = in;
	return p;
}

struct params *
params_ivmeth(string_t *in)
{
	struct params *p = params_new();

	p->ivmeth = in;
	return p;
}

struct params *
params_keylen(size_t in)
{
	struct params *p = params_new();

	p->keylen = in;
	return p;
}

struct params *
params_bsize(size_t in)
{
	struct params *p = params_new();

	p->bsize = in;
	return p;
}

struct params *
params_verify_method(string_t *in)
{
	struct params *p = params_new();
	const char *vm = string_tocharstar(in);

	if (!strcmp("none", vm))
		p->verify_method = VERIFY_NONE;
	if (!strcmp("disklabel", vm))
		p->verify_method = VERIFY_DISKLABEL;
	if (!strcmp("ffs", vm))
		p->verify_method = VERIFY_FFS;
	if (!strcmp("re-enter", vm))
		p->verify_method = VERIFY_REENTER;
	if (!strcmp("mbr", vm))
		p->verify_method = VERIFY_MBR;
	if (!strcmp("gpt", vm))
		p->verify_method = VERIFY_GPT;

	string_free(in);

	if (p->verify_method == VERIFY_UNKNOWN)
		warnx("params_setverify_method: unrecognized "
		    "verify method \"%s\"", vm);
	return p;
}

struct params *
params_keygen(struct keygen *in)
{
	struct params *p = params_new();

	p->keygen = in;
	return p;
}

struct params *
params_dep_keygen(struct keygen *in)
{
	struct params *p = params_new();

	p->dep_keygen = in;
	return p;
}

struct keygen *
keygen_new(void)
{
	struct keygen *kg;

	kg = emalloc(sizeof(*kg));
	kg->kg_method = KEYGEN_UNKNOWN;
	kg->kg_iterations = (size_t)-1;
	kg->kg_salt = NULL;
	kg->kg_key = NULL;
	kg->kg_cmd = NULL;
	kg->next = NULL;
	return kg;
}

void
keygen_free(struct keygen *kg)
{

	if (!kg)
		return;
	bits_free(kg->kg_salt);
	bits_free(kg->kg_key);
	string_free(kg->kg_cmd);
	keygen_free(kg->next);
	free(kg);
}

/*
 * keygen_verify traverses the keygen structures and ensures
 * that the appropriate information is available.
 */

int
keygen_verify(const struct keygen *kg)
{

	if (!kg)
		return 1;
	switch (kg->kg_method) {
	case KEYGEN_PKCS5_PBKDF2_OLD:
		if (kg->kg_iterations == (size_t)-1) {
			warnx("keygen pkcs5_pbkdf2 must provide `iterations'");
			return 0;
		}
		if (kg->kg_key)
			warnx("keygen pkcs5_pbkdf2 does not need a `key'");
		if (!kg->kg_salt) {
			warnx("keygen pkcs5_pbkdf2 must provide a salt");
			return 0;
		}
		if (kg->kg_cmd)
			warnx("keygen pkcs5_pbkdf2 does not need a `cmd'");
		break;
	case KEYGEN_PKCS5_PBKDF2_SHA1:
		if (kg->kg_iterations == (size_t)-1) {
			warnx("keygen pkcs5_pbkdf2/sha1 must provide `iterations'");
			return 0;
		}
		if (kg->kg_key)
			warnx("keygen pkcs5_pbkdf2/sha1 does not need a `key'");
		if (!kg->kg_salt) {
			warnx("keygen pkcs5_pbkdf2/sha1 must provide a salt");
			return 0;
		}
		if (kg->kg_cmd)
			warnx("keygen pkcs5_pbkdf2/sha1 does not need a `cmd'");
		break;
	case KEYGEN_STOREDKEY:
		if (kg->kg_iterations != (size_t)-1)
			warnx("keygen storedkey does not need `iterations'");
		if (!kg->kg_key) {
			warnx("keygen storedkey must provide a key");
			return 0;
		}
		if (kg->kg_salt)
			warnx("keygen storedkey does not need `salt'");
		if (kg->kg_cmd)
			warnx("keygen storedkey does not need `cmd'");
		break;
	case KEYGEN_RANDOMKEY:
	case KEYGEN_URANDOMKEY:
		if (kg->kg_iterations != (size_t)-1)
			warnx("keygen [u]randomkey does not need `iterations'");
		if (kg->kg_key)
			warnx("keygen [u]randomkey does not need `key'");
		if (kg->kg_salt)
			warnx("keygen [u]randomkey does not need `salt'");
		if (kg->kg_cmd)
			warnx("keygen [u]randomkey does not need `cmd'");
		break;
	case KEYGEN_SHELL_CMD:
		if (kg->kg_iterations != (size_t)-1)
			warnx("keygen shell_cmd does not need `iterations'");
		if (kg->kg_key)
			warnx("keygen shell_cmd does not need `key'");
		if (kg->kg_salt)
			warnx("keygen shell_cmd does not need `salt'");
		if (!kg->kg_cmd) {
			warnx("keygen shell_cmd must provide a `cmd'");
			return 0;
		}
		break;
	}
	return keygen_verify(kg->next);
}

struct keygen *
keygen_generate(int method)
{
	struct keygen *kg;

	kg = keygen_new();
	if (!kg)
		return NULL;

	kg->kg_method = method;
	return kg;
}

/*
 * keygen_filldefaults walks the keygen list and fills in
 * default values.  The defaults may be either calibrated
 * or randomly generated so this function is designed to be
 * called when generating a new parameters file, not when
 * reading a parameters file.
 */

int
keygen_filldefaults(struct keygen *kg, size_t keylen)
{

	if (!kg)
		return 0;
	switch (kg->kg_method) {
	case KEYGEN_RANDOMKEY:
	case KEYGEN_URANDOMKEY:
	case KEYGEN_SHELL_CMD:
		break;
	case KEYGEN_PKCS5_PBKDF2_OLD:
	case KEYGEN_PKCS5_PBKDF2_SHA1:
		kg->kg_salt = bits_getrandombits(DEFAULT_SALTLEN, 1);
		kg->kg_iterations = pkcs5_pbkdf2_calibrate(BITS2BYTES(keylen),
		    DEFAULT_ITERATION_TIME);
		if (kg->kg_iterations < 1) {
			warnx("could not calibrate pkcs5_pbkdf2");
			return -1;
		}
		break;
	case KEYGEN_STOREDKEY:
		/* Generate a random stored key */
		kg->kg_key = bits_getrandombits(keylen, 1);
		if (!kg->kg_key) {
			warnx("can't generate random bits for storedkey");
			return -1;
		}
		break;
	default:
		return -1;
	}

	return keygen_filldefaults(kg->next, keylen);
}

struct keygen *
keygen_combine(struct keygen *kg1, struct keygen *kg2)
{
	if (!kg1 && !kg2)
		return NULL;

	if (!kg1)
		kg1 = keygen_new();

	if (!kg2)
		return kg1;

	if (kg2->kg_method != KEYGEN_UNKNOWN)
		kg1->kg_method = kg2->kg_method;

	if (kg2->kg_iterations != (size_t)-1 && kg2->kg_iterations > 0)
		kg1->kg_iterations = kg2->kg_iterations;

	if (kg2->kg_salt)
		bits_assign(&kg1->kg_salt, kg2->kg_salt);

	if (kg2->kg_key)
		bits_assign(&kg1->kg_key, kg2->kg_key);

	if (kg2->kg_cmd)
		string_assign(&kg1->kg_cmd, kg2->kg_cmd);

	return kg1;
}

struct keygen *
keygen_method(string_t *in)
{
	struct keygen *kg = keygen_new();
	const char *kgm = string_tocharstar(in);

	if (!strcmp("pkcs5_pbkdf2", kgm))
		kg->kg_method = KEYGEN_PKCS5_PBKDF2_OLD;
	if (!strcmp("pkcs5_pbkdf2/sha1", kgm))
		kg->kg_method = KEYGEN_PKCS5_PBKDF2_SHA1;
	if (!strcmp("randomkey", kgm))
		kg->kg_method = KEYGEN_RANDOMKEY;
	if (!strcmp("storedkey", kgm))
		kg->kg_method = KEYGEN_STOREDKEY;
	if (!strcmp("urandomkey", kgm))
		kg->kg_method = KEYGEN_URANDOMKEY;
	if (!strcmp("shell_cmd", kgm))
		kg->kg_method = KEYGEN_SHELL_CMD;

	string_free(in);

	if (kg->kg_method == KEYGEN_UNKNOWN)
		warnx("unrecognized key generation method \"%s\"", kgm);
	return kg;
}

struct keygen *
keygen_set_method(struct keygen *kg, string_t *in)
{

	return keygen_combine(kg, keygen_method(in));
}

struct keygen *
keygen_salt(bits_t *in)
{
	struct keygen *kg = keygen_new();

	kg->kg_salt = in;
	return kg;
}

struct keygen *
keygen_iterations(size_t in)
{
	struct keygen *kg = keygen_new();

	kg->kg_iterations = in;
	return kg;
}

void
keygen_addlist(struct keygen **l, struct keygen *e)
{
	struct keygen *t;

	if (*l) {
		t = *l;
		for (;t->next; t = t->next)
			;
		t->next = e;
	} else {
		*l = e;
	}
}

struct keygen *
keygen_key(bits_t *in)
{
	struct keygen *kg = keygen_new();

	kg->kg_key = in;
	return kg;
}

struct keygen *
keygen_cmd(string_t *in)
{
	struct keygen *kg = keygen_new();

	kg->kg_cmd = in;
	return kg;
}

struct params *
params_fget(FILE *f)
{
	struct params *p;

	p = cgdparsefile(f);

	if (!p)
		return NULL;

	/*
	 * We deal with the deprecated keygen structure by prepending it
	 * to the list of keygens, so that the rest of the code does not
	 * have to deal with this backwards compat issue.  The deprecated
	 * ``xor_key'' field may be stored in p->dep_keygen->kg_key.  If
	 * it exists, we construct a storedkey keygen struct as well.  Also,
	 * default the iteration count to 128 as the old code did.
	 */

	if (p->dep_keygen) {
		if (p->dep_keygen->kg_iterations == (size_t)-1)
			p->dep_keygen->kg_iterations = 128;
		p->dep_keygen->next = p->keygen;
		if (p->dep_keygen->kg_key) {
			p->keygen = keygen_generate(KEYGEN_STOREDKEY);
			p->keygen->kg_key = p->dep_keygen->kg_key;
			p->dep_keygen->kg_key = NULL;
			p->keygen->next = p->dep_keygen;
		} else {
			p->keygen = p->dep_keygen;
		}
		p->dep_keygen = NULL;
	}
	return p;
}

struct params *
params_cget(const char *fn)
{
	struct params	*p;
	FILE		*f;

	if ((f = fopen(fn, "r")) == NULL) {
		warn("failed to open params file \"%s\"", fn);
		return NULL;
	}
	p = params_fget(f);
	(void)fclose(f);
	return p;
}

#define WRAP_COL	50
#define TAB_COL		8

static void
spaces(FILE *f, int len)
{

	while (len-- > 0)
		(void)fputc(' ', f);
}

static void
print_kvpair_cstr(FILE *f, int ts, const char *key, const char *val)
{

	spaces(f, ts);
	(void)fprintf(f, "%s %s;\n", key, val);
}

static void
print_kvpair_string(FILE *f, int ts, const char *key, const string_t *val)
{

	print_kvpair_cstr(f, ts, key, string_tocharstar(val));
}

static void
print_kvpair_int(FILE *f, int ts, const char *key, size_t val)
{
	char	*tmp;

	if (!key || val == (size_t)-1)
		return;

	if (asprintf(&tmp, "%zu", val) == -1)
		err(1, NULL);
	print_kvpair_cstr(f, ts, key, tmp);
	free(tmp);
}

/*
 * prints out a base64 encoded k-v pair to f.  It encodes the length
 * of the bitstream as a 32bit unsigned integer in network byte order
 * up front.
 */

static void
print_kvpair_b64(FILE *f, int curpos, int ts, const char *key, bits_t *val)
{
	string_t	*str;
	int		 i;
	int		 len;
	int		 pos;
	const char	*out;

	if (!key || !val)
		return;

	str = bits_encode(val);
	out = string_tocharstar(str);
	len = strlen(out);

	spaces(f, ts);
	(void)fprintf(f, "%s ", key);
	curpos += ts + strlen(key) + 1;
	ts = curpos;

	for (i=0, pos=curpos; i < len; i++, pos++) {
		if (pos > WRAP_COL) {
			(void)fprintf(f, " \\\n");
			spaces(f, ts);
			pos = ts;
		}
		(void)fputc(out[i], f);
	}
	(void)fprintf(f, ";\n");
	string_free(str);
}

int
keygen_fput(struct keygen *kg, int ts, FILE *f)
{
	int	curpos = 0;

	if (!kg)
		return 0;
	(void)fprintf(f, "keygen ");
	curpos += strlen("keygen ");
	switch (kg->kg_method) {
	case KEYGEN_STOREDKEY:
		(void)fprintf(f, "storedkey ");
		curpos += strlen("storedkey ");
		print_kvpair_b64(f, curpos, 0, "key", kg->kg_key);
		break;
	case KEYGEN_RANDOMKEY:
		(void)fprintf(f, "randomkey;\n");
		break;
	case KEYGEN_URANDOMKEY:
		(void)fprintf(f, "urandomkey;\n");
		break;
	case KEYGEN_PKCS5_PBKDF2_OLD:
		(void)fprintf(f, "pkcs5_pbkdf2 {\n");
		print_kvpair_int(f, ts, "iterations", kg->kg_iterations);
		print_kvpair_b64(f, 0, ts, "salt", kg->kg_salt);
		(void)fprintf(f, "};\n");
		break;
	case KEYGEN_PKCS5_PBKDF2_SHA1:
		(void)fprintf(f, "pkcs5_pbkdf2/sha1 {\n");
		print_kvpair_int(f, ts, "iterations", kg->kg_iterations);
		print_kvpair_b64(f, 0, ts, "salt", kg->kg_salt);
		(void)fprintf(f, "};\n");
		break;
	default:
		warnx("keygen_fput: %d not a valid method", kg->kg_method);
		break;
	}
	return keygen_fput(kg->next, ts, f);
}

int
params_fput(struct params *p, FILE *f)
{
	int	ts = 0;		/* tabstop of 0 spaces */

	print_kvpair_string(f, ts, "algorithm", p->algorithm);
	print_kvpair_string(f, ts, "iv-method", p->ivmeth);
	print_kvpair_int(f, ts, "keylength", p->keylen);
	print_kvpair_int(f, ts, "blocksize", p->bsize);
	switch (p->verify_method) {
	case VERIFY_NONE:
		print_kvpair_cstr(f, ts, "verify_method", "none");
		break;
	case VERIFY_DISKLABEL:
		print_kvpair_cstr(f, ts, "verify_method", "disklabel");
		break;
	case VERIFY_FFS:
		print_kvpair_cstr(f, ts, "verify_method", "ffs");
		break;
	case VERIFY_REENTER:
		print_kvpair_cstr(f, ts, "verify_method", "re-enter");
		break;
	case VERIFY_MBR:
		print_kvpair_cstr(f, ts, "verify_method", "mbr");
		break;
	case VERIFY_GPT:
		print_kvpair_cstr(f, ts, "verify_method", "gpt");
		break;
	default:
		warnx("unsupported verify_method (%d)", p->verify_method);
		return -1;
	}
	return keygen_fput(p->keygen, TAB_COL, f);
}

int
params_cput(struct params *p, const char *fn)
{
	FILE	*f;

	if (fn && *fn) {
		if ((f = fopen(fn, "w")) == NULL) {
			warn("could not open outfile \"%s\"", fn);
			return -1;
		}
	} else {
		f = stdout;
	}
	return params_fput(p, f);
}

static int
crypt_defaults_lookup(const char *alg)
{
	unsigned	i;

	for (i=0; i < (sizeof(crypto_defaults) / sizeof(crypto_defaults[0])); i++)
		if (!strcmp(alg, crypto_defaults[i].alg))
			return i;

	return -1;
}
