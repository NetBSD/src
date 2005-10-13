/*	$NetBSD: wcsncpy_test.c,v 1.1 2005/10/13 21:15:49 tnozaki Exp $	*/

/*-
 * Copyright (c)2005 Citrus Project,
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
 *
 */
 
#include <string.h>
#include <wchar.h>

int
main(void)
{
#define	CASESIZE	32
	wchar_t buf[CASESIZE + 1];
	const wchar_t *str[] = {
		L"0",
		L"01",
		L"012",
		L"0123",
		L"01234",
		L"012345",
		L"0123456",
		L"01234567",
		L"012345678",
		L"0123456789",
		NULL
	}, **p;
	int i;

	for (p = str; *p; ++p) {
		wmemset(buf, (wchar_t)0xdeadbeef, CASESIZE + 1);
		wcsncpy(buf, *p, CASESIZE);
		if (wcscmp(buf, *p))
			return (1);
		for (i = wcslen(*p); i < CASESIZE; ++i) {
			if (buf[i] != L'\0')
				return (2);
		}
		if (buf[CASESIZE] != (wchar_t)0xdeadbeef)
			return (3);
	}
	return (0);
}
