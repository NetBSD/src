/*	$NetBSD: efichar.c,v 1.1.2.2 2018/03/30 06:20:11 pgoyette Exp $	*/

/*-
 * Copyright (c) 2010 Marcel Moolenaar
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

#include <sys/cdefs.h>
#if 0
__FBSDID("$FreeBSD: head/stand/efi/libefi/efichar.c 328061 2018-01-16 20:35:54Z tsoome $");
#endif

#include "efiboot.h"

size_t
ucs2len(const CHAR16 *str)
{
	size_t i;

	i = 0;
	while (*str++)
		i++;
	return i;
}

/*
 * If nm were converted to utf8, what what would strlen
 * return on the resulting string?
 */
static size_t
utf8_len_of_ucs2(const CHAR16 *nm)
{
	size_t len;
	CHAR16 c;

	len = 0;
	while (*nm) {
		c = *nm++;
		if (c > 0x7ff)
			len += 3;
		else if (c > 0x7f)
			len += 2;
		else
			len++;
	}

	return len;
}

int
ucs2_to_utf8(const CHAR16 *nm, char **name)
{
	size_t len, sz;
	CHAR16 c;
	char *cp;
	int freeit = *name == NULL;

	sz = utf8_len_of_ucs2(nm) + 1;
	len = 0;
	if (*name != NULL)
		cp = *name;
	else
		cp = *name = AllocatePool(sz);
	if (*name == NULL)
		return ENOMEM;

	while (*nm) {
		c = *nm++;
		if (c > 0x7ff) {
			if (len++ < sz)
				*cp++ = (char)(0xE0 | (c >> 12));
			if (len++ < sz)
				*cp++ = (char)(0x80 | ((c >> 6) & 0x3f));
			if (len++ < sz)
				*cp++ = (char)(0x80 | (c & 0x3f));
		} else if (c > 0x7f) {
			if (len++ < sz)
				*cp++ = (char)(0xC0 | ((c >> 6) & 0x1f));
			if (len++ < sz)
				*cp++ = (char)(0x80 | (c & 0x3f));
		} else {
			if (len++ < sz)
				*cp++ = (char)(c & 0x7f);
		}
	}

	if (len >= sz) {
		/* Absent bugs, we'll never return EOVERFLOW */
		if (freeit) {
			FreePool(*name);
			*name = NULL;
		}
		return EOVERFLOW;
	}
	*cp++ = '\0';

	return 0;
}

int
utf8_to_ucs2(const char *name, CHAR16 **nmp, size_t *len)
{
	CHAR16 *nm;
	size_t sz;
	uint32_t ucs4;
	int c, bytes;
	int freeit = *nmp == NULL;

	sz = strlen(name) * 2 + 2;
	if (*nmp == NULL)
		*nmp = AllocatePool(sz);
	if (*nmp == NULL)
		return ENOMEM;
	nm = *nmp;
	*len = sz;

	ucs4 = 0;
	bytes = 0;
	while (sz > 1 && *name != '\0') {
		c = *name++;
		/*
		 * Conditionalize on the two major character types:
		 * initial and followup characters.
		 */
		if ((c & 0xc0) != 0x80) {
			/* Initial characters. */
			if (bytes != 0)
				goto ilseq;
			if ((c & 0xf8) == 0xf0) {
				ucs4 = c & 0x07;
				bytes = 3;
			} else if ((c & 0xf0) == 0xe0) {
				ucs4 = c & 0x0f;
				bytes = 2;
			} else if ((c & 0xe0) == 0xc0) {
				ucs4 = c & 0x1f;
				bytes = 1;
			} else {
				ucs4 = c & 0x7f;
				bytes = 0;
			}
		} else {
			/* Followup characters. */
			if (bytes > 0) {
				ucs4 = (ucs4 << 6) + (c & 0x3f);
				bytes--;
			} else if (bytes == 0)
				goto ilseq;
		}
		if (bytes == 0) {
			if (ucs4 > 0xffff)
				goto ilseq;
			*nm++ = (CHAR16)ucs4;
			sz -= 2;
		}
	}
	if (sz < 2) {
		if (freeit) {
			FreePool(nm);
			*nmp = NULL;
		}
		return EINVAL;
	}
	sz -= 2;
	*nm = 0;
	*len -= sz;
	return 0;
ilseq:
	if (freeit) {
		FreePool(nm);
		*nmp = NULL;
	}
	return EILSEQ;
}
