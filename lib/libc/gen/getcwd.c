/*	$NetBSD: getcwd.c,v 1.29 2003/08/03 04:12:01 itojun Exp $	*/

/*
 * Copyright (c) 1989, 1991, 1993, 1995
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Jan-Simon Pendry.
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
static char sccsid[] = "@(#)getcwd.c	8.5 (Berkeley) 2/7/95";
#else
__RCSID("$NetBSD: getcwd.c,v 1.29 2003/08/03 04:12:01 itojun Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"
#include <sys/param.h>
#include <sys/stat.h>

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "extern.h"

#ifdef __weak_alias
__weak_alias(getcwd,_getcwd)
__weak_alias(realpath,_realpath)
#endif

#define	ISDOT(dp) \
	(dp->d_name[0] == '.' && (dp->d_name[1] == '\0' || \
	    (dp->d_name[1] == '.' && dp->d_name[2] == '\0')))


#if defined(__SVR4) || defined(__svr4__)
#define d_fileno d_ino
#endif

/*
 * char *realpath(const char *path, char resolved_path[MAXPATHLEN]);
 *
 * Find the real name of path, by removing all ".", ".." and symlink
 * components.  Returns (resolved) on success, or (NULL) on failure,
 * in which case the path which caused trouble is left in (resolved).
 */
char *
realpath(path, resolved)
	const char *path;
	char *resolved;
{
	struct stat sb;
	int fd, n, rootd, serrno, nlnk = 0;
	char *p, *q, wbuf[MAXPATHLEN];

	_DIAGASSERT(path != NULL);
	_DIAGASSERT(resolved != NULL);

	/* Save the starting point. */
	if ((fd = open(".", O_RDONLY)) < 0) {
		(void)strlcpy(resolved, ".", MAXPATHLEN);
		return (NULL);
	}

	/*
	 * Find the dirname and basename from the path to be resolved.
	 * Change directory to the dirname component.
	 * lstat the basename part.
	 *     if it is a symlink, read in the value and loop.
	 *     if it is a directory, then change to that directory.
	 * get the current directory name and append the basename.
	 */
	(void)strlcpy(resolved, path, MAXPATHLEN);
loop:
	q = strrchr(resolved, '/');
	if (q != NULL) {
		p = q + 1;
		if (q == resolved)
			q = "/";
		else {
			do {
				--q;
			} while (q > resolved && *q == '/');
			q[1] = '\0';
			q = resolved;
		}
		if (chdir(q) < 0)
			goto err1;
	} else
		p = resolved;

	/* Deal with the last component. */
	if (lstat(p, &sb) == 0) {
		if (S_ISLNK(sb.st_mode)) {
			if (nlnk++ >= MAXSYMLINKS) {
				errno = ELOOP;
				goto err1;
			}
			n = readlink(p, resolved, MAXPATHLEN-1);
			if (n < 0)
				goto err1;
			resolved[n] = '\0';
			goto loop;
		}
		if (S_ISDIR(sb.st_mode)) {
			if (chdir(p) < 0)
				goto err1;
			p = "";
		}
	}

	/*
	 * Save the last component name and get the full pathname of
	 * the current directory.
	 */
	(void)strlcpy(wbuf, p, sizeof(wbuf));

	/*
	 * Call the inernal internal version of getcwd which
	 * does a physical search rather than using the $PWD short-cut
	 */
	if (getcwd(resolved, MAXPATHLEN) == 0)
		goto err1;

	/*
	 * Join the two strings together, ensuring that the right thing
	 * happens if the last component is empty, or the dirname is root.
	 */
	if (resolved[0] == '/' && resolved[1] == '\0')
		rootd = 1;
	else
		rootd = 0;

	if (*wbuf) {
		if (strlen(resolved) + strlen(wbuf) + (rootd ? 0 : 1) + 1 >
		    MAXPATHLEN) {
			errno = ENAMETOOLONG;
			goto err1;
		}
		if (rootd == 0)
			(void)strcat(resolved, "/"); /* XXX: strcat is safe */
		(void)strcat(resolved, wbuf);	/* XXX: strcat is safe */
	}

	/* Go back to where we came from. */
	if (fchdir(fd) < 0) {
		serrno = errno;
		goto err2;
	}

	/* It's okay if the close fails, what's an fd more or less? */
	(void)close(fd);
	return (resolved);

err1:	serrno = errno;
	(void)fchdir(fd);
err2:	(void)close(fd);
	errno = serrno;
	return (NULL);
}

#ifdef OLD_GETCWD

char *
getcwd(pt, size)
	char *pt;
	size_t size;
{
	struct dirent *dp;
	DIR *dir;
	dev_t dev;
	ino_t ino;
	int first;
	char *bpt, *bup;
	struct stat s;
	dev_t root_dev;
	ino_t root_ino;
	size_t ptsize, upsize;
	int save_errno;
	char *ept, *eup, *up;
	size_t dlen;

	/*
	 * If no buffer specified by the user, allocate one as necessary.
	 * If a buffer is specified, the size has to be non-zero.  The path
	 * is built from the end of the buffer backwards.
	 */
	if (pt) {
		ptsize = 0;
		if (!size) {
			errno = EINVAL;
			return (NULL);
		}
		ept = pt + size;
	} else {
		if ((pt = malloc(ptsize = 1024 - 4)) == NULL)
			return (NULL);
		ept = pt + ptsize;
	}
	bpt = ept - 1;
	*bpt = '\0';

	/*
	 * Allocate bytes (1024 - malloc space) for the string of "../"'s.
	 * Should always be enough (it's 340 levels).  If it's not, allocate
	 * as necessary.  Special case the first stat, it's ".", not "..".
	 */
	if ((up = malloc(upsize = 1024 - 4)) == NULL)
		goto err;
	eup = up + MAXPATHLEN;
	bup = up;
	up[0] = '.';
	up[1] = '\0';

	/* Save root values, so know when to stop. */
	if (stat("/", &s))
		goto err;
	root_dev = s.st_dev;
	root_ino = s.st_ino;

	errno = 0;			/* XXX readdir has no error return. */

	for (first = 1;; first = 0) {
		/* Stat the current level. */
		if (lstat(up, &s))
			goto err;

		/* Save current node values. */
		ino = s.st_ino;
		dev = s.st_dev;

		/* Check for reaching root. */
		if (root_dev == dev && root_ino == ino) {
			*--bpt = '/';
			/*
			 * It's unclear that it's a requirement to copy the
			 * path to the beginning of the buffer, but it's always
			 * been that way and stuff would probably break.
			 */
			memmove(pt, bpt,  (size_t)(ept - bpt));
			free(up);
			return (pt);
		}

		/*
		 * Build pointer to the parent directory, allocating memory
		 * as necessary.  Max length is 3 for "../", the largest
		 * possible component name, plus a trailing NULL.
		 */
		if (bup + 3  + MAXNAMLEN + 1 >= eup) {
			if ((up = realloc(up, upsize *= 2)) == NULL)
				goto err;
			bup = up;
			eup = up + upsize;
		}
		*bup++ = '.';
		*bup++ = '.';
		*bup = '\0';

		/* Open and stat parent directory. */
		if (!(dir = opendir(up)) || fstat(dirfd(dir), &s))
			goto err;

		/* Add trailing slash for next directory. */
		*bup++ = '/';

		/*
		 * If it's a mount point, have to stat each element because
		 * the inode number in the directory is for the entry in the
		 * parent directory, not the inode number of the mounted file.
		 */
		save_errno = 0;
		if (s.st_dev == dev) {
			for (;;) {
				if (!(dp = readdir(dir)))
					goto notfound;
				if (dp->d_fileno == ino) {
#if defined(__SVR4) || defined(__svr4__) || defined(__linux__)
					dlen = strlen(dp->d_name);
#else
					dlen = dp->d_namlen;
#endif
					break;
				}
			}
		} else
			for (;;) {
				if (!(dp = readdir(dir)))
					goto notfound;
				if (ISDOT(dp))
					continue;
#if defined(__SVR4) || defined(__svr4__) || defined(__linux__)
				dlen = strlen(dp->d_name);
#else
				dlen = dp->d_namlen;
#endif
				memmove(bup, dp->d_name, dlen + 1);

				/* Save the first error for later. */
				if (lstat(up, &s)) {
					if (!save_errno)
						save_errno = errno;
					errno = 0;
					continue;
				}
				if (s.st_dev == dev && s.st_ino == ino)
					break;
			}

		/*
		 * Check for length of the current name, preceding slash,
		 * leading slash.
		 */
		if (bpt - pt <= dlen + (first ? 1 : 2)) {
			size_t len, off;

			if (!ptsize) {
				errno = ERANGE;
				goto err;
			}
			off = bpt - pt;
			len = ept - bpt;
			if ((pt = realloc(pt, ptsize *= 2)) == NULL)
				goto err;
			bpt = pt + off;
			ept = pt + ptsize;
			memmove(ept - len, bpt, len);
			bpt = ept - len;
		}
		if (!first)
			*--bpt = '/';
		bpt -= dlen;
		memmove(bpt, dp->d_name, dlen);
		(void)closedir(dir);

		/* Truncate any file name. */
		*bup = '\0';
	}

notfound:
	/*
	 * If readdir set errno, use it, not any saved error; otherwise,
	 * didn't find the current directory in its parent directory, set
	 * errno to ENOENT.
	 */
	if (!errno)
		errno = save_errno ? save_errno : ENOENT;
	/* FALLTHROUGH */
err:
	if (ptsize)
		free(pt);
	free(up);
	return (NULL);
}

#else /* New getcwd */

char *
getcwd(pt, size)
	char *pt;
	size_t size;
{
	size_t ptsize, bufsize;
	int len;
	
	/*
	 * If no buffer specified by the user, allocate one as necessary.
	 * If a buffer is specified, the size has to be non-zero.  The path
	 * is built from the end of the buffer backwards.
	 */
	if (pt) {
		ptsize = 0;
		if (!size) {
			errno = EINVAL;
			return (NULL);
		}
		bufsize = size;
	} else {
		if ((pt = malloc(ptsize = 1024 - 4)) == NULL)
			return (NULL);
		bufsize = ptsize;
	}
	for (;;) {
		len = __getcwd(pt, bufsize);
		if ((len < 0) && (size == 0) && (errno == ERANGE)) {
			if (ptsize > (MAXPATHLEN*4))
				return NULL;
			if ((pt = realloc(pt, ptsize *= 2)) == NULL)
				return NULL;
			bufsize = ptsize;
			continue;
		}
		break;
	}
	if (len < 0)
		return NULL;
	else
		return pt;
}

#endif
