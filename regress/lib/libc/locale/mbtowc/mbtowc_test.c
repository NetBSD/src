/* $NetBSD: mbtowc_test.c,v 1.1.2.3 2008/01/09 01:37:30 matt Exp $ */

/*-
 * Copyright (c)2007 Citrus Project,
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
#include <assert.h>
#include <errno.h>
#include <locale.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
main(int argc, char *argv[])
{
	int i;
	const char *testcase;
	char path[PATH_MAX + 1];
	char *s;
	size_t stateful, n, ret;
	FILE *fp;

	for (i = 1; i < argc; ++i) {

		testcase = (const char *)argv[i];
		s = setlocale(LC_CTYPE, (const char *)testcase);
		assert(s != NULL && !strcmp(s, testcase));
		stateful = wctomb(NULL, L'\0');

		/* initialize internal state */
		ret = mbtowc(NULL, NULL, 0);
		assert(stateful ? ret : !ret);

		snprintf(path, sizeof(path), FILEDIR"/%s", testcase);
		fp = fopen(path, "r");
		assert(fp != NULL);

		/* illegal multibyte sequence case */
		s = fgetln(fp, &n);
		assert(s != NULL);
		ret = mbtowc(NULL, s, n - 1);
		assert(ret == (size_t)-1 && errno == EILSEQ);

		/* if this is stateless encoding, this re-initialization is not required. */
		if (stateful) {
			/* re-initialize internal state */
			ret = mbtowc(NULL, NULL, 0);
			assert(stateful ? ret : !ret);
		}

		/* valid multibyte sequence case */
		s = fgetln(fp, &n);
		errno = 0;
		ret = mbtowc(NULL, s, n - 1);
		assert(ret != (size_t)-1 && errno == 0);

		fclose(fp);
	}
	return 0;
}
