/*-
 * Copyright (c)2000 Citrus Project,
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
 *	from: Id: timelocal.c,v 1.1 2000/05/25 14:14:18 minoura Exp
 */

/*-
 * Copyright (c) 1997 FreeBSD Inc.
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
 * $FreeBSD: src/lib/libc/stdtime/timelocal.c,v 1.2.2.2 1999/12/10 11:00:49 sheldonh Exp $
 */

#include <sys/localedef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syslimits.h>
#include <fcntl.h>
#include <locale.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "setlocale.h"
#include "timelocal.h"

static int split_lines __P((char *, const char *));
static int set_from_buf __P((_TimeLocale *, const char *, int));


/*
 * XXX: setlocale() should be reentrant??
 */

int
__time_load_locale(name)
	const char *name;
{
	static char *		locale_buf = NULL;
	static _TimeLocale	TimeLocale;
	int			num_lines;

	int			fd;
	char *			p;
	const char *		plim;
	char                    filename[PATH_MAX];
	struct stat		st;
	size_t			bufsize;

	if (name == NULL)
		goto no_locale;
	if (_CurrentTimeLocale != &_DefaultTimeLocale &&
	    _CurrentTimeLocale != &TimeLocale)
		goto no_locale;

	/*
	** Slurp the locale file into the cache.
	*/
	if (!_PathLocale)
		goto no_locale;
	/* Range checking not needed, 'name' size is limited */
	strcpy(filename, *_LocalePaths[LC_TIME]);
	strcat(filename, "/");
	strcat(filename, name);
	strcat(filename, "/LC_TIME");
	fd = open(filename, O_RDONLY);
	if (fd < 0)
		goto no_locale;
	if (fstat(fd, &st) != 0)
		goto bad_locale;
	if (st.st_size <= 0 || st.st_size > 50*1024)	/* XXX */
		goto bad_locale;
	bufsize = st.st_size;
	p = malloc(bufsize);
	if (p == NULL)
		goto bad_locale;
	plim = p + st.st_size;
	if (read(fd, p, (size_t) st.st_size) != st.st_size)
		goto bad_lbuf;
	if (close(fd) != 0)
		goto bad_lbuf;
	/*
	** Parse the locale file into localebuf.
	*/
	if (plim[-1] != '\n')
		goto bad_lbuf;
	num_lines = split_lines(p, plim);
	if (num_lines >= LCTIME_SIZE_FULL)
		num_lines = LCTIME_SIZE_FULL;
	else
		goto reset_locale;
	if (set_from_buf(&TimeLocale, p, num_lines) < 0)
		goto reset_locale;
	/*
	 * Record the successful parse.
	 */
	if (locale_buf)
		free (locale_buf);
	locale_buf = p;
	_CurrentTimeLocale = &TimeLocale;

	return 0;

reset_locale:
bad_lbuf:
	free(p);
bad_locale:
	(void) close(fd);
no_locale:
	return -1;
}

static int
split_lines(p, plim)
	char *p;
	const char *plim;
{
	int i;

	for (i = 0; p < plim; i++) {
		p = strchr(p, '\n');
		*p++ = '\0';
	}
	return i;
}

static int
set_from_buf(p, buf, num_lines)
	_TimeLocale *p;
	const char *buf;
	int num_lines;
{
	const char **ap = (void*) p;
	int i;

	if (num_lines != LCTIME_SIZE_FULL)
		return -1;

	for ( i = 0; i < num_lines; ++ap, ++i)
		*ap = buf += strlen(buf) + 1;

	return 0;
}
