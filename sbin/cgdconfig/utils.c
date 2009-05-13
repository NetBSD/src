/* $NetBSD: utils.c,v 1.19.4.1 2009/05/13 19:19:00 jym Exp $ */

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
__RCSID("$NetBSD: utils.c,v 1.19.4.1 2009/05/13 19:19:00 jym Exp $");
#endif

#include <sys/param.h>

#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <util.h>

/* include the resolver gunk in order that we can use b64 routines */
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>

#include "utils.h"


/* just strsep(3), but skips empty fields. */

static char *
strsep_getnext(char **stringp, const char *delim)
{
	char	*ret;

	ret = strsep(stringp, delim);
	while (ret && index(delim, *ret))
		ret = strsep(stringp, delim);
	return ret;
}

/*
 * this function returns a dynamically sized char ** of the words
 * in the line.  the caller is responsible for both free(3)ing
 * each word and the superstructure by calling words_free().
 */
char **
words(const char *line, int *num)
{
	int	  i = 0;
	int	  nwords = 0;
	char	 *cur;
	char	**ret;
	const char	 *tmp;
	char	 *tmp1, *tmpf;

	*num = 0;
	tmp = line;
	if (tmp[0] == '\0')
		return NULL;
	while (tmp[0]) {
		if ((tmp[1] == ' ' || tmp[1] == '\t' || tmp[1] == '\0') &&
		    (tmp[0] != ' ' && tmp[0] != '\t'))
			nwords++;
		tmp++;
	}
	ret = emalloc((nwords+1) * sizeof(char *));
	tmp1 = tmpf = estrdup(line);
	while ((cur = strsep_getnext(&tmpf, " \t")) != NULL)
		ret[i++] = estrdup(cur);
	ret[i] = NULL;
	free(tmp1);
	*num = nwords;
	return ret;
}

void
words_free(char **w, int num)
{
	int	i;

	for (i=0; i < num; i++)
		free(w[i]);
}

/*
 * this is a simple xor that has the same calling conventions as
 * memcpy(3).
 */

void
memxor(void *res, const void *src, size_t len)
{
	char *r;
	const char *s;
	size_t i;

	r = res;
	s = src;
	for (i = 0; i < len; i++)
		r[i] ^= s[i];
}

/*
 * well, a very simple set of string functions...
 *
 * The goal here is basically to manage length encoded strings,
 * but just for safety we nul terminate them anyway.
 */

/* for now we use a very simple encoding */

struct string {
	char	*text;
	size_t	 length;
};

string_t *
string_zero()
{
	string_t *out;

	out = emalloc(sizeof(*out));
	out->length = 0;
	out->text = NULL;
	return out;
}

string_t *
string_new(const char *intext, size_t inlength)
{
	string_t *out;

	out = emalloc(sizeof(*out));
	out->length = inlength;
	out->text = emalloc(out->length + 1);
	(void)memcpy(out->text, intext, out->length);
	out->text[out->length] = '\0';
	return out;
}

string_t *
string_dup(const string_t *in)
{

	return string_new(in->text, in->length);
}

void
string_free(string_t *s)
{

	if (!s)
		return;
	free(s->text);
	free(s);
}

void
string_assign(string_t **lhs, string_t *rhs)
{

	string_free(*lhs);
	*lhs = rhs;
}

string_t *
string_add(const string_t *a1, const string_t *a2)
{
	string_t *sum;

	sum = emalloc(sizeof(*sum));
	sum->length = a1->length + a2->length;
	sum->text = emalloc(sum->length + 1);
	(void)memcpy(sum->text, a1->text, a1->length);
	(void)memcpy(sum->text + a1->length, a2->text, a2->length);
	sum->text[sum->length] = '\0';
	return sum;
}

string_t *
string_add_d(string_t *a1, string_t *a2)
{
	string_t *sum;

	sum = string_add(a1, a2);
	string_free(a1);
	string_free(a2);
	return sum;
}

string_t *
string_fromcharstar(const char *in)
{

	return string_new(in, strlen(in));
}

const char *
string_tocharstar(const string_t *in)
{

	return in->text;
}

string_t *
string_fromint(int in)
{
	string_t *ret;

	ret = emalloc(sizeof(*ret));
	ret->length = asprintf(&ret->text, "%d", in);
	if (ret->text == NULL)
		err(1, NULL);
	return ret;
}

void
string_fprint(FILE *f, const string_t *s)
{
	(void)fwrite(s->text, s->length, 1, f);
}

struct bits {
	size_t	 length;
	char	*text;
};

bits_t *
bits_new(const void *buf, size_t len)
{
	bits_t	*b;

	b = emalloc(sizeof(*b));
	b->length = len;
	b->text = emalloc(BITS2BYTES(b->length));
	(void)memcpy(b->text, buf, BITS2BYTES(b->length));
	return b;
}

bits_t *
bits_dup(const bits_t *in)
{

	return bits_new(in->text, in->length);
}

void
bits_free(bits_t *b)
{

	if (!b)
		return;
	free(b->text);
	free(b);
}

void
bits_assign(bits_t **lhs, bits_t *rhs)
{

	bits_free(*lhs);
	*lhs = rhs;
}

const void *
bits_getbuf(bits_t *in)
{

	return in->text;
}

size_t
bits_len(bits_t *in)
{

	return in->length;
}

int
bits_match(const bits_t *b1, const bits_t *b2)
{
	size_t i;

	if (b1->length != b2->length)
		return 0;

	for (i = 0; i < BITS2BYTES(b1->length); i++)
		if (b1->text[i] != b2->text[i])
			return 0;

	return 1;
}

bits_t *
bits_xor(const bits_t *x1, const bits_t *x2)
{
	bits_t	*b;
	size_t	 i;

	b = emalloc(sizeof(*b));
	b->length = MAX(x1->length, x2->length);
	b->text = ecalloc(1, BITS2BYTES(b->length));
	for (i=0; i < BITS2BYTES(MIN(x1->length, x2->length)); i++)
		b->text[i] = x1->text[i] ^ x2->text[i];
	return b;
}

bits_t *
bits_xor_d(bits_t *x1, bits_t *x2)
{
	bits_t	*ret;

	ret = bits_xor(x1, x2);
	bits_free(x1);
	bits_free(x2);
	return ret;
}

/*
 * bits_decode() reads an encoded base64 stream.  We interpret
 * the first 32 bits as an unsigned integer in network byte order
 * specifying the number of bits in the stream to give a little
 * resilience.
 */

bits_t *
bits_decode(const string_t *in)
{
	bits_t	*ret;
	size_t	 len;
	size_t	 nbits;
	u_int32_t	*tmp;

	len = in->length;
	tmp = emalloc(len);

	len = __b64_pton(in->text, (void *)tmp, len);

	if (len == (size_t)-1) {
		warnx("bits_decode: mangled base64 stream");
		warnx("  %s", in->text);
		free(tmp);
		return NULL;
	}

	nbits = ntohl(*tmp);
	if (nbits > (len - sizeof(*tmp)) * NBBY) {
		warnx("bits_decode: encoded bits claim to be "
		    "longer than they are (nbits=%zu, stream len=%zu bytes)",
		    nbits, len);
		free(tmp);
		return NULL;
	}

	ret = bits_new(tmp + 1, nbits);
	free(tmp);
	return ret;
}

bits_t *
bits_decode_d(string_t *in)
{
	bits_t *ret;

	ret = bits_decode(in);
	string_free(in);
	return ret;
}

string_t *
bits_encode(const bits_t *in)
{
	string_t *ret;
	size_t	 len;
	char	*out;
	u_int32_t *tmp;

	if (!in)
		return NULL;

	/* compute the total size of the input stream */
	len = BITS2BYTES(in->length) + sizeof(*tmp);

	tmp = emalloc(len);
	out = emalloc(len * 2);
	/* stuff the length up front */
	*tmp = htonl(in->length);
	(void)memcpy(tmp + 1, in->text, len - sizeof(*tmp));

	if ((len = __b64_ntop((void *)tmp, len, out, len * 2)) == (size_t)-1) {
		free(out);
		free(tmp);
		return NULL;
	}
	ret = string_new(out, len);
	free(tmp);
	free(out);
	return ret;
}

string_t *
bits_encode_d(bits_t *in)
{
	string_t *ret;

	ret = bits_encode(in);
	bits_free(in);
	return ret;
}

bits_t *
bits_fget(FILE *f, size_t len)
{
	bits_t	*bits;
	int	 ret;

	bits = emalloc(sizeof(*bits));
	bits->length = len;
	bits->text = emalloc(BITS2BYTES(bits->length));
	ret = fread(bits->text, BITS2BYTES(bits->length), 1, f);
	if (ret != 1) {
		bits_free(bits);
		return NULL;
	}
	return bits;
}

bits_t *
bits_cget(const char *fn, size_t len)
{
	bits_t	*bits;
	FILE	*f;

	f = fopen(fn, "r");
	if (!f)
		return NULL;

	bits = bits_fget(f, len);
	(void)fclose(f);
	return bits;
}

bits_t *
bits_getrandombits(size_t len, int hard)
{

	return bits_cget((hard ? "/dev/random" : "/dev/urandom"), len);
}

void
bits_fprint(FILE *f, const bits_t *bits)
{
	string_t *s;

	s = bits_encode(bits);
	string_fprint(f, s);
	free(s);
}
