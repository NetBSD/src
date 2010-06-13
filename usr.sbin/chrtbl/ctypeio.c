/* $NetBSD: ctypeio.c,v 1.5 2010/06/13 04:14:57 tnozaki Exp $ */

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
__RCSID("$NetBSD: ctypeio.c,v 1.5 2010/06/13 04:14:57 tnozaki Exp $");
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "bsdctype_local.h"
#include "ctypeio.h"

int
__savectype(const char *name, unsigned char *new_ctype,
    short *new_toupper, short *new_tolower)
{
	FILE *fp;
	uint32_t i, len = _CTYPE_CACHE_SIZE;

	_DIAGASSERT(name != NULL);
	_DIAGASSERT(new_ctype != NULL);
	_DIAGASSERT(new_toupper != NULL);
	_DIAGASSERT(new_tolower != NULL);

	if ((fp = fopen(name, "w")) == NULL)
		return 0;

	if (fwrite(_CTYPE_ID, sizeof(_CTYPE_ID) - 1, 1, fp) != 1)
		goto bad;

	i = htonl(_CTYPE_REV);
	if (fwrite(&i, sizeof(uint32_t), 1, fp) != 1) 
		goto bad;

	i = htonl(len);
	if (fwrite(&i, sizeof(uint32_t), 1, fp) != 1)
		goto bad;

	if (fwrite(&new_ctype[1], sizeof(uint8_t), len, fp) != len)
		goto bad;

#if BYTE_ORDER == LITTLE_ENDIAN
	for (i = 1; i <= len; i++) {
		new_toupper[i] = htons(new_toupper[i]);
		new_tolower[i] = htons(new_tolower[i]);
	}
#endif
	if (fwrite(&new_toupper[1], sizeof(int16_t), len, fp) != len)
		goto bad;

	if (fwrite(&new_tolower[1], sizeof(int16_t), len, fp) != len)
		goto bad;


	(void)fclose(fp);
	return 1;
bad:
	(void)fclose(fp);
	return 0;
}
