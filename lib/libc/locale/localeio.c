/*	$NetBSD: localeio.c,v 1.1.8.1 2009/01/15 03:24:07 snj Exp $	*/
/*
 * Copyright (c) 2008, The NetBSD Foundation, Inc.
 * All rights reserved.
 * 
 * This code is derived from software contributed to The NetBSD Foundation
 * by Brian Ginsbach.
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
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: localeio.c,v 1.1.8.1 2009/01/15 03:24:07 snj Exp $");
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include <sys/types.h>
#include <sys/stat.h>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <locale.h>
#include <paths.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "localeio.h"

int
__loadlocale(const char *name, size_t nstr, size_t nbytes,
    size_t localesize, void *currentlocale)
{
	int fd, i, ret;
	unsigned char **ap, *buf, *bp, *cp, *cbp, *ebp;
	unsigned char ***locale;
	struct stat st;
	size_t bufsize;

	_DIAGASSERT(name != NULL);
	_DIAGASSERT(localesize != 0);
	_DIAGASSERT(currentlocale != NULL);

	if ((fd = open(name, O_RDONLY)) == -1)
		return ENOENT;

	if ((fstat(fd, &st) == -1) || !S_ISREG(st.st_mode) ||
	    (st.st_size <= 0)) {
		ret = EFTYPE;
		goto error1;
	}

	bufsize = localesize + (size_t)st.st_size;
	if ((buf = malloc(bufsize)) == NULL) {
		ret = ENOMEM;
		goto error1;
	}

	bp = buf + localesize;
	if (read(fd, bp, (size_t)st.st_size) != st.st_size) {
		ret = EFTYPE;
		goto error2;
	}

	ap = (unsigned char **)(void *)buf;
	for (i = 0, ebp = buf + bufsize; i < nstr; i++) {
		ap[i] = bp;
		while (bp != ebp && *bp != '\n')
			bp++;
		if (bp == ebp) {
			ret = EFTYPE;
			goto error2;
		}
		*bp++ = '\0';
	}

	cp = buf + (sizeof(unsigned char *) * nstr);
	for (i = 0, cbp = bp; i < nbytes; i++) {
		int n;

		while (bp != ebp && *bp != '\n')
			bp++;
		if (bp == ebp) {
			ret = EFTYPE;
			goto error2;
		}
		/* ignore overflow/underflow and bad characters */
		n = (unsigned char)strtol((char *)cbp, NULL, 0);
		cp[i] = (unsigned char)(n & CHAR_MAX);
		cbp = bp;
	}

	locale = currentlocale;

	*locale = (unsigned char **)(void *)buf;
	(void)close(fd);
	return 0;

error2:
	free(buf);

error1:
	(void)close(fd);
	return ret;
}
