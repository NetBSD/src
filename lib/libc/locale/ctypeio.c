/* $NetBSD: ctypeio.c,v 1.11 2010/01/17 23:12:30 wiz Exp $ */

/*
 * Copyright (c) 1997 Christos Zoulas.  All rights reserved.
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
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: ctypeio.c,v 1.11 2010/01/17 23:12:30 wiz Exp $");
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
#include <assert.h>
#define _CTYPE_PRIVATE
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "bsdctype.h"
#include "ctypeio.h"

int
__loadctype(const char * __restrict path, _BSDCTypeLocale ** __restrict pdata)
{
	FILE *fp;
	char id[sizeof(_CTYPE_ID) - 1];
	uint32_t i, len;
	char *ptr;
	uint8_t *new_ctype;
	int16_t *new_tolower, *new_toupper;
	_BSDCTypeLocale *data;

	_DIAGASSERT(path != NULL);
	_DIAGASSERT(pdata != NULL);

	fp = fopen(path, "r");
	if (fp == NULL)
		return ENOENT;

	if (fread(id, sizeof(id), 1, fp) != 1 ||
	    memcmp(id, _CTYPE_ID, sizeof(id)) != 0)
		goto bad0;

	if (fread(&i, sizeof(uint32_t), 1, fp) != 1 ||
	    (i = ntohl(i)) != _CTYPE_REV)
		goto bad0;

	if (fread(&len, sizeof(uint32_t), 1, fp) != 1 ||
	    (len = ntohl(len)) != _CTYPE_NUM_CHARS)
		goto bad0;

	ptr = malloc(sizeof(*data) + ((sizeof(uint8_t) +
	    sizeof(int16_t) + sizeof(int16_t)) * (len + 1)));
	if (ptr == NULL) {
		fclose(fp);
		return ENOMEM;
	}

	data = (_BSDCTypeLocale *)(void *)ptr;
	ptr += sizeof(*data);

	(new_ctype = (void *)ptr)[0] = (uint8_t)0;
	ptr += sizeof(uint8_t);
	if (fread((void *)ptr, sizeof(uint8_t), len, fp) != len)
		goto bad1;
	ptr += sizeof(uint8_t) * len;

	(new_toupper = (void *)ptr)[0] = (int16_t)EOF;
	ptr += sizeof(int16_t);
	if (fread((void *)ptr, sizeof(int16_t), len, fp) != len)
		goto bad1;
	ptr += sizeof(int16_t) * len;

	(new_tolower = (void *)ptr)[0] = (int16_t)EOF;
	ptr += sizeof(int16_t);
	if (fread((void *)ptr, sizeof(int16_t), len, fp) != len)
		goto bad1;

#if BYTE_ORDER == LITTLE_ENDIAN
	for (i = 1; i <= len; i++) {
		new_toupper[i] = ntohs(new_toupper[i]);
		new_tolower[i] = ntohs(new_tolower[i]);
	}
#endif

	fclose(fp);

	data->ctype_tab = (const unsigned char *)new_ctype;
	data->toupper_tab = (const short *)new_toupper;
	data->tolower_tab = (const short *)new_tolower;

	*pdata = data;

		return 0;

bad1:
	free(data);
bad0:
	fclose(fp);
	return EFTYPE;
}
