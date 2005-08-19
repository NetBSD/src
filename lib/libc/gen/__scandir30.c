/*	$NetBSD: __scandir30.c,v 1.1 2005/08/19 02:04:54 christos Exp $	*/

/*
 * Copyright (c) 1983, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
static char sccsid[] = "@(#)scandir.c	8.3 (Berkeley) 1/2/94";
#else
__RCSID("$NetBSD: __scandir30.c,v 1.1 2005/08/19 02:04:54 christos Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

/*
 * Scan the directory dirname calling selectfn to make a list of selected
 * directory entries then sort using qsort and compare routine dcomp.
 * Returns the number of entries and a pointer to a list of pointers to
 * struct dirent (through namelist). Returns -1 if there were any errors.
 */

#include "namespace.h"
#include <sys/types.h>
#include <sys/stat.h>

#include <assert.h>
#include <errno.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>

#ifdef __weak_alias
__weak_alias(scandir,_scandir)
#endif

#ifdef __LIBC12_SOURCE__
#define dirent dirent12
#endif

int
scandir(dirname, namelist, selectfn, dcomp)
	const char *dirname;
	struct dirent ***namelist;
	int (*selectfn) __P((const struct dirent *));
	int (*dcomp) __P((const void *, const void *));
{
	struct dirent *d, *p, **names, **newnames;
	size_t nitems, arraysz;
	struct stat stb;
	DIR *dirp;

	_DIAGASSERT(dirname != NULL);
	_DIAGASSERT(namelist != NULL);

	if ((dirp = opendir(dirname)) == NULL)
		return (-1);
	if (fstat(dirp->dd_fd, &stb) < 0)
		goto bad;

	/*
	 * estimate the array size by taking the size of the directory file
	 * and dividing it by a multiple of the minimum size entry. 
	 */
	arraysz = (size_t)(stb.st_size / 24);
	names = malloc(arraysz * sizeof(struct dirent *));
	if (names == NULL)
		goto bad;

	nitems = 0;
	while ((d = readdir(dirp)) != NULL) {
		if (selectfn != NULL && !(*selectfn)(d))
			continue;	/* just selected names */

		/*
		 * Check to make sure the array has space left and
		 * realloc the maximum size.
		 */
		if (nitems >= arraysz) {
			if (fstat(dirp->dd_fd, &stb) < 0)
				goto bad2;	/* just might have grown */
			arraysz = (size_t)(stb.st_size / 12);
			newnames = realloc(names,
			    arraysz * sizeof(struct dirent *));
			if (newnames == NULL)
				goto bad2;
			names = newnames;
		}

		/*
		 * Make a minimum size copy of the data
		 */
		p = (struct dirent *)malloc((size_t)_DIRENT_SIZE(d));
		if (p == NULL)
			goto bad2;
		p->d_fileno = d->d_fileno;
		p->d_reclen = d->d_reclen;
		p->d_type = d->d_type;
		p->d_namlen = d->d_namlen;
		memmove(p->d_name, d->d_name, (size_t)(p->d_namlen + 1));
		names[nitems++] = p;
	}
	closedir(dirp);
	if (nitems && dcomp != NULL)
		qsort(names, nitems, sizeof(struct dirent *), dcomp);
	*namelist = names;
	return (nitems);

bad2:
	while (nitems-- > 0)
		free(names[nitems]);
	free(names);
bad:
	closedir(dirp);
	return (-1);
}
