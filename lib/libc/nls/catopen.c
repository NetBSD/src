/*	$NetBSD: catopen.c,v 1.7 1996/06/20 14:54:38 jtc Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by J.T. Conklin.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#define _NLS_PRIVATE

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <nl_types.h>

#define NLS_DEFAULT_PATH "/usr/share/nls/%L/%N.cat:/usr/share/nls/%N/%L"
#define NLS_DEFAULT_LANG "C"

static nl_catd load_msgcat();

nl_catd
_catopen(name, oflag)
	const char *name;
	int oflag;
{
	char tmppath[PATH_MAX];
	char *nlspath;
	char *lang;
	char *s, *t;
	nl_catd catd;
		
	if (name == NULL || *name == '\0')
		return (nl_catd) -1;

	/* absolute or relative path? */
	if (strchr (name, '/'))
		return load_msgcat(name);

	if ((nlspath = getenv ("NLSPATH")) == NULL) {
		nlspath = NLS_DEFAULT_PATH;
	}
	if ((lang = getenv ("LANG")) == NULL) {
		lang = NLS_DEFAULT_LANG;
	}

	s = nlspath;
	t = tmppath;	
	do {
		while (*s && *s != ':') {
			if (*s == '%') {
				switch (*(s+1)) {
				case 'L':	/* locale */
					strcpy(t, lang);
					t += strlen(lang);
					s += 2;
					break;
				case 'N':	/* name */
					strcpy(t, name);
					t += strlen(name);
					s += 2;
					break;
				case 'l':	/* lang */
				case 't':	/* territory */
				case 'c':	/* codeset */
					break;
				default:
					*t++ = *s++;
				}
			} else {
				*t++ = *s++;
			}
		}

		*t = '\0';
		catd = load_msgcat(tmppath);
		if (catd != (nl_catd) 0)
			return catd;

		s++;
		t = tmppath;
	} while (*s);

	return (nl_catd) 0;
}

static nl_catd
load_msgcat(path)
	const char *path;
{
	struct _nls_cat_hdr *cat_hdr;
	struct stat st;
	nl_catd catd;
	int fd;

	if ((fd = open (path, O_RDONLY)) == -1)
		return (nl_catd) 0;

	if (fstat(fd, &st) != 0) {
		close (fd);
		return (nl_catd) 0;
	}

	if ((catd = malloc (sizeof (*catd))) == 0) {
		close (fd);
		return (nl_catd) 0;
	}

	catd->__data = mmap(0, (size_t) st.st_size, PROT_READ, MAP_SHARED, fd, 0);
	close (fd);

	if (catd->__data == (void *) -1) {
		free (catd);
		return (nl_catd) 0;
	}
	catd->__size = st.st_size;

	cat_hdr = (struct _nls_cat_hdr *) catd->__data;
	if (ntohl(cat_hdr->__magic) != _NLS_MAGIC) {
		free (catd);
		close (fd);
		return (nl_catd) 0;
	}

	return catd;
}
