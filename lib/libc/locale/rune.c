/*	$NetBSD: rune.c,v 1.12 2001/04/17 20:12:31 kleink Exp $	*/

/*-
 * Copyright (c)1999 Citrus Project,
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*-
 * Copyright (c) 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Paul Borman at Krystal Technologies.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)rune.c	8.1 (Berkeley) 6/4/93";
#else
__RCSID("$NetBSD: rune.c,v 1.12 2001/04/17 20:12:31 kleink Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

#include "rune.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "rune_local.h"

static int readrange __P((_RuneLocale *, _RuneRange *, _FileRuneRange *, void *, FILE *));
static void _freeentry __P((_RuneRange *));

static int
readrange(_RuneLocale *rl, _RuneRange *rr, _FileRuneRange *frr, void *lastp,
	FILE *fp)
{
	int i;
	_RuneEntry *re;
	_FileRuneEntry fre;

	_DIAGASSERT(rl != NULL);
	_DIAGASSERT(rr != NULL);
	_DIAGASSERT(frr != NULL);
	_DIAGASSERT(lastp != NULL);
	_DIAGASSERT(fp != NULL);

	re = (_RuneEntry *)rl->__rune_variable;

	rr->__nranges = ntohl(frr->__nranges);
	if (rr->__nranges == 0) {
		rr->__rune_ranges = NULL;
		return 0;
	}

	rr->__rune_ranges = re;
	for (i = 0; i < rr->__nranges; i++) {
		if (fread(&fre, sizeof(fre), 1, fp) != 1)
			return -1;

		re->__min = ntohl((u_int32_t)fre.__min);
		re->__max = ntohl((u_int32_t)fre.__max);
		re->__map = ntohl((u_int32_t)fre.__map);
		re++;

		if ((void *)re > lastp)
			return -1;
	}
	rl->__rune_variable = re;
	return 0;
}

static int
readentry(_RuneRange *rr, FILE *fp)
{
	_RuneEntry *re;
	size_t l, i, j;
	int error;

	_DIAGASSERT(rr != NULL);
	_DIAGASSERT(fp != NULL);

	re = rr->__rune_ranges;
	for (i = 0; i < rr->__nranges; i++) {
		if (re[i].__map != 0) {
			re[i].__rune_types = NULL;
			continue;
		}

		l = re[i].__max - re[i].__min + 1;
		re[i].__rune_types = malloc(l * sizeof(_RuneType));
		if (!re[i].__rune_types) {
			error = ENOBUFS;
			goto fail;
		}
		memset(re[i].__rune_types, 0, l * sizeof(_RuneType));

		if (fread(re[i].__rune_types, sizeof(_RuneType), l, fp) != l)
			goto fail2;

		for (j = 0; j < l; j++)
			re[i].__rune_types[j] = ntohl(re[i].__rune_types[j]);
	}
	return 0;

fail:
	for (j = 0; j < i; j++) {
		free(re[j].__rune_types);
		re[j].__rune_types = NULL;
	}
	return error;
fail2:
	for (j = 0; j <= i; j++) {
		free(re[j].__rune_types);
		re[j].__rune_types = NULL;
	}
	return errno;
}

/* XXX: temporary implementation */
static void
find_codeset(_RuneLocale *rl)
{
	char *top, *codeset, *tail;

	rl->__rune_codeset = NULL;
	if (!(top=strstr(rl->__rune_variable, _RUNE_CODESET)))
		return;
	tail = strpbrk(top, " \t");
	codeset = top + sizeof(_RUNE_CODESET)-1;
	if (tail) {
		*top = *tail;
		*tail = '\0';
		rl->__rune_codeset = strdup(codeset);
		strcpy(top+1, tail+1);
			
	} else {
		*top='\0';
		rl->__rune_codeset = strdup(codeset);
	}
}

void
_freeentry(_RuneRange *rr)
{
	_RuneEntry *re;
	int i;

	_DIAGASSERT(rr != NULL);

	re = rr->__rune_ranges;
	for (i = 0; i < rr->__nranges; i++) {
		if (re[i].__rune_types)
			free(re[i].__rune_types);
		re[i].__rune_types = NULL;
	}
}

_RuneLocale *
_Read_RuneMagi(fp)
	FILE *fp;
{
	/* file */
	_FileRuneLocale frl;
	/* host data */
	char *hostdata;
	size_t hostdatalen;
	void *lastp;
	_RuneLocale *rl;
	struct stat sb;
	int x;

	_DIAGASSERT(fp != NULL);

	if (fstat(fileno(fp), &sb) < 0)
		return NULL;

	if (sb.st_size < sizeof(_RuneLocale))
		return NULL;
	/* XXX more validation? */

	/* Someone might have read the magic number once already */
	rewind(fp);

	if (fread(&frl, sizeof(frl), 1, fp) != 1)
		return NULL;
	if (memcmp(frl.__magic, _RUNE_MAGIC_1, sizeof(frl.__magic)))
		return NULL;

	hostdatalen = sizeof(*rl) + ntohl((u_int32_t)frl.__variable_len) +
	    ntohl(frl.__runetype_ext.__nranges) * sizeof(_RuneEntry) +
	    ntohl(frl.__maplower_ext.__nranges) * sizeof(_RuneEntry) +
	    ntohl(frl.__mapupper_ext.__nranges) * sizeof(_RuneEntry);

	if ((hostdata = malloc(hostdatalen)) == NULL)
		return NULL;
	memset(hostdata, 0, hostdatalen);
	lastp = hostdata + hostdatalen;

	rl = (_RuneLocale *)(void *)hostdata;
	rl->__rune_variable = rl + 1;

	memcpy(rl->__magic, frl.__magic, sizeof(rl->__magic));
	memcpy(rl->__encoding, frl.__encoding, sizeof(rl->__encoding));

	rl->__invalid_rune = ntohl((u_int32_t)frl.__invalid_rune);
	rl->__variable_len = ntohl((u_int32_t)frl.__variable_len);

	for (x = 0; x < _CACHED_RUNES; ++x) {
		rl->__runetype[x] = ntohl(frl.__runetype[x]);

		/* XXX assumes rune_t = u_int32_t */
		rl->__maplower[x] = ntohl((u_int32_t)frl.__maplower[x]);
		rl->__mapupper[x] = ntohl((u_int32_t)frl.__mapupper[x]);
	}

	if (readrange(rl, &rl->__runetype_ext, &frl.__runetype_ext, lastp, fp))
	{
		free(hostdata);
		return NULL;
	}
	if (readrange(rl, &rl->__maplower_ext, &frl.__maplower_ext, lastp, fp))
	{
		free(hostdata);
		return NULL;
	}
	if (readrange(rl, &rl->__mapupper_ext, &frl.__mapupper_ext, lastp, fp))
	{
		free(hostdata);
		return NULL;
	}

	if (readentry(&rl->__runetype_ext, fp) < 0) {
		free(hostdata);
		return NULL;
	}

	if ((u_int8_t *)rl->__rune_variable + rl->__variable_len >
	    (u_int8_t *)lastp) {
		_freeentry(&rl->__runetype_ext);
		free(hostdata);
		return NULL;
	}
	if (rl->__variable_len == 0)
		rl->__rune_variable = NULL;
	else if (fread(rl->__rune_variable, rl->__variable_len, 1, fp) != 1) {
		_freeentry(&rl->__runetype_ext);
		free(hostdata);
		return NULL;
	}
	find_codeset(rl);

	/* error if we have junk at the tail */
	if (ftell(fp) != sb.st_size) {
		_freeentry(&rl->__runetype_ext);
		free(hostdata);
		return NULL;
	}

	return(rl);
}

void
_NukeRune(rl)
	_RuneLocale *rl;
{

	_DIAGASSERT(rl != NULL);

	_freeentry(&rl->__runetype_ext);
	if (rl->__rune_codeset)
		free(rl->__rune_codeset);
	free(rl);
}

/*
 * read in old LC_CTYPE declaration file, convert into runelocale info
 */
#define _CTYPE_PRIVATE
#include <limits.h>
#include <ctype.h>

_RuneLocale *
_Read_CTypeAsRune(fp)
	FILE *fp;
{
	char id[sizeof(_CTYPE_ID) - 1];
	u_int32_t i, len;
	u_int8_t *new_ctype = NULL;
	int16_t *new_toupper = NULL, *new_tolower = NULL;
	/* host data */
	char *hostdata;
	size_t hostdatalen;
	_RuneLocale *rl;
	struct stat sb;
	int x;

	_DIAGASSERT(fp != NULL);

	if (fstat(fileno(fp), &sb) < 0)
		return NULL;

	if (sb.st_size < sizeof(id))
		return NULL;
	/* XXX more validation? */

	/* Someone might have read the magic number once already */
	rewind(fp);

	if (fread(id, sizeof(id), 1, fp) != 1)
		goto bad;
	if (memcmp(id, _CTYPE_ID, sizeof(id)) != 0)
		goto bad;

	if (fread(&i, sizeof(u_int32_t), 1, fp) != 1) 
		goto bad;
	if ((i = ntohl(i)) != _CTYPE_REV)
		goto bad;

	if (fread(&len, sizeof(u_int32_t), 1, fp) != 1)
		goto bad;
	if ((len = ntohl(len)) != _CTYPE_NUM_CHARS)
		goto bad;

	if ((new_ctype = malloc(sizeof(u_int8_t) * (1 + len))) == NULL ||
	    (new_toupper = malloc(sizeof(int16_t) * (1 + len))) == NULL ||
	    (new_tolower = malloc(sizeof(int16_t) * (1 + len))) == NULL)
		goto bad;
	new_ctype[0] = 0;
	if (fread(&new_ctype[1], sizeof(u_int8_t), len, fp) != len)
		goto bad;
	new_toupper[0] = EOF;
	if (fread(&new_toupper[1], sizeof(int16_t), len, fp) != len)
		goto bad;
	new_tolower[0] = EOF;
	if (fread(&new_tolower[1], sizeof(int16_t), len, fp) != len)
		goto bad;

	hostdatalen = sizeof(*rl);

	if ((hostdata = malloc(hostdatalen)) == NULL)
		goto bad;
	memset(hostdata, 0, hostdatalen);
	rl = (_RuneLocale *)(void *)hostdata;
	rl->__rune_variable = NULL;

	memcpy(rl->__magic, _RUNE_MAGIC_1, sizeof(rl->__magic));
	memcpy(rl->__encoding, "NONE", 4);

	rl->__invalid_rune = _DefaultRuneLocale.__invalid_rune;	/*XXX*/
	rl->__variable_len = 0;

	for (x = 0; x < _CACHED_RUNES; ++x) {
		if (x > len)
			continue;

		/*
		 * TWEAKS!
		 * - old locale file declarations do not have proper _B
		 *   in many cases.
		 * - isprint() declaration in ctype.h incorrectly uses _B.
		 *   _B means "isprint but !isgraph", not "isblank" with the
		 *   declaration.
		 * - _X and _CTYPE_X have negligible difference in meaning.
		 * - we don't set digit value, fearing that it would be
		 *   too much of hardcoding.  we may need to revisit it.
		 */

		if (new_ctype[1 + x] & _U)
			rl->__runetype[x] |= _CTYPE_U;
		if (new_ctype[1 + x] & _L)
			rl->__runetype[x] |= _CTYPE_L;
		if (new_ctype[1 + x] & _N)
			rl->__runetype[x] |= _CTYPE_D;
		if (new_ctype[1 + x] & _S)
			rl->__runetype[x] |= _CTYPE_S;
		if (new_ctype[1 + x] & _P)
			rl->__runetype[x] |= _CTYPE_P;
		if (new_ctype[1 + x] & _C)
			rl->__runetype[x] |= _CTYPE_C;
		/* derived flag bits, duplicate of ctype.h */
		if (new_ctype[1 + x] & (_U | _L))
			rl->__runetype[x] |= _CTYPE_A;
		if (new_ctype[1 + x] & (_N | _X))
			rl->__runetype[x] |= _CTYPE_X;
		if (new_ctype[1 + x] & (_P|_U|_L|_N))
			rl->__runetype[x] |= _CTYPE_G;
		/* we don't really trust _B in the file.  see above. */
		if (new_ctype[1 + x] & _B)
			rl->__runetype[x] |= _CTYPE_B;
		if ((new_ctype[1 + x] & (_P|_U|_L|_N|_B)) || x == ' ')
			rl->__runetype[x] |= (_CTYPE_R | _CTYPE_SW1);
		if (x == ' ' || x == '\t')
			rl->__runetype[x] |= _CTYPE_B;

		/* XXX may fail on non-8bit encoding only */
		rl->__mapupper[x] = ntohs(new_toupper[1 + x]);
		rl->__maplower[x] = ntohs(new_tolower[1 + x]);
	}

	/*
	 * __runetable_to_netbsd_ctype() will be called from
	 * setlocale.c:loadlocale(), and fill old ctype table.
	 */

	free(new_ctype);
	free(new_toupper);
	free(new_tolower);
	return(rl);

bad:
	if (new_ctype)
		free(new_ctype);
	if (new_toupper)
		free(new_toupper);
	if (new_tolower)
		free(new_tolower);
	if (hostdata)
		free(hostdata);
	return NULL;
}
