/*	$NetBSD: opendir.c,v 1.23 2003/05/29 18:29:59 nathanw Exp $	*/

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
static char sccsid[] = "@(#)opendir.c	8.7 (Berkeley) 12/10/94";
#else
__RCSID("$NetBSD: opendir.c,v 1.23 2003/05/29 18:29:59 nathanw Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"
#include "reentrant.h"
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/stat.h>

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __weak_alias
__weak_alias(opendir,_opendir)
#endif

/*
 * Open a directory.
 */
DIR *
opendir(name)
	const char *name;
{

	_DIAGASSERT(name != NULL);

	return (__opendir2(name, DTF_HIDEW|DTF_NODUP));
}

DIR *
__opendir2(name, flags)
	const char *name;
	int flags;
{
	DIR *dirp = NULL;
	int fd;
	int serrno;
	struct stat sb;
	int pagesz;
	int incr;
	int unionstack, nfsdir;
	struct statfs sfb;

	_DIAGASSERT(name != NULL);

	if ((fd = open(name, O_RDONLY | O_NONBLOCK)) == -1)
		return (NULL);
	if (fstat(fd, &sb) || !S_ISDIR(sb.st_mode)) {
		close(fd);
		errno = ENOTDIR;
		return (NULL);
	}
	if (fcntl(fd, F_SETFD, FD_CLOEXEC) == -1 ||
	    (dirp = (DIR *)malloc(sizeof(DIR))) == NULL) {
		goto error;
	}
	dirp->dd_buf = NULL;

	/*
	 * If the machine's page size is an exact multiple of DIRBLKSIZ,
	 * use a buffer that is cluster boundary aligned.
	 * Hopefully this can be a big win someday by allowing page trades
	 * to user space to be done by getdirentries()
	 */
	if (((pagesz = getpagesize()) % DIRBLKSIZ) == 0)
		incr = pagesz;
	else
		incr = DIRBLKSIZ;

	/*
	 * Determine whether this directory is the top of a union stack.
	 */

	if (fstatfs(fd, &sfb) < 0)
		goto error;

	if (flags & DTF_NODUP)
		unionstack = !(strncmp(sfb.f_fstypename, MOUNT_UNION,
		    MFSNAMELEN)) || (sfb.f_flags & MNT_UNION);
	else
		unionstack = 0;

	nfsdir = !(strncmp(sfb.f_fstypename, MOUNT_NFS, MFSNAMELEN));

	if (unionstack || nfsdir) {
		size_t len;
		size_t space;
		char *buf, *nbuf;
		char *ddptr;
		char *ddeptr;
		int n;
		struct dirent **dpv;

		/*
		 * The strategy here for directories on top of a union stack
		 * is to read all the directory entries into a buffer, sort
		 * the buffer, and remove duplicate entries by setting the
		 * inode number to zero.
		 *
		 * For directories on an NFS mounted filesystem, we try
	 	 * to get a consistent snapshot by trying until we have
		 * successfully read all of the directory without errors
		 * (i.e. 'bad cookie' errors from the server because
		 * the directory was modified). These errors should not
		 * happen often, but need to be dealt with.
		 */
retry:
		len = 0;
		space = 0;
		buf = 0;
		ddptr = 0;

		do {
			/*
			 * Always make at least DIRBLKSIZ bytes
			 * available to getdirentries
			 */
			if (space < DIRBLKSIZ) {
				space += incr;
				len += incr;
				nbuf = realloc(buf, len);
				if (nbuf == NULL) {
					dirp->dd_buf = buf;
					goto error;
				}
				buf = nbuf;
				ddptr = buf + (len - space);
			}

			dirp->dd_seek = lseek(fd, (off_t)0, SEEK_CUR);
			n = getdents(fd, ddptr, space);
			/*
			 * For NFS: EINVAL means a bad cookie error
			 * from the server. Keep trying to get a
			 * consistent view, in this case this means
			 * starting all over again.
			 */
			if (n == -1 && errno == EINVAL && nfsdir) {
				free(buf);
				lseek(fd, (off_t)0, SEEK_SET);
				goto retry;
			}
			if (n > 0) {
				ddptr += n;
				space -= n;
			}
		} while (n > 0);

		ddeptr = ddptr;
		flags |= __DTF_READALL;

		/*
		 * Re-open the directory.
		 * This has the effect of rewinding back to the
		 * top of the union stack and is needed by
		 * programs which plan to fchdir to a descriptor
		 * which has also been read -- see fts.c.
		 */
		if (flags & DTF_REWIND) {
			(void) close(fd);
			if ((fd = open(name, O_RDONLY)) == -1) {
				dirp->dd_buf = buf;
				goto error;
			}
		}

		/*
		 * There is now a buffer full of (possibly) duplicate
		 * names.
		 */
		dirp->dd_buf = buf;

		/*
		 * Go round this loop twice...
		 *
		 * Scan through the buffer, counting entries.
		 * On the second pass, save pointers to each one.
		 * Then sort the pointers and remove duplicate names.
		 */
		if (!nfsdir) {
			for (dpv = 0;;) {
				for (n = 0, ddptr = buf; ddptr < ddeptr;) {
					struct dirent *dp;

					dp = (struct dirent *)(void *)ddptr;
					if ((long)dp & 03)
						break;
					/*
					 * d_reclen is unsigned,
					 * so no need to compare <= 0
					 */
					if (dp->d_reclen > (ddeptr + 1 - ddptr))
						break;
					ddptr += dp->d_reclen;
					if (dp->d_fileno) {
						if (dpv)
							dpv[n] = dp;
						n++;
					}
				}

				if (dpv) {
					struct dirent *xp;

					/*
					 * This sort must be stable.
					 */
					mergesort(dpv, (size_t)n, sizeof(*dpv),
					    alphasort);

					dpv[n] = NULL;
					xp = NULL;

					/*
					 * Scan through the buffer in sort
					 * order, zapping the inode number
					 * of any duplicate names.
					 */
					for (n = 0; dpv[n]; n++) {
						struct dirent *dp = dpv[n];

						if ((xp == NULL) ||
						    strcmp(dp->d_name,
						      xp->d_name))
							xp = dp;
						else
							dp->d_fileno = 0;
						if (dp->d_type == DT_WHT &&
						    (flags & DTF_HIDEW))
							dp->d_fileno = 0;
					}

					free(dpv);
					break;
				} else {
					dpv = malloc((n + 1) *
					    sizeof(struct dirent *));
					if (dpv == NULL)
						break;
				}
			}
		}

		dirp->dd_len = len;
		dirp->dd_size = ddptr - dirp->dd_buf;
	} else {
		dirp->dd_len = incr;
		dirp->dd_buf = malloc((size_t)dirp->dd_len);
		if (dirp->dd_buf == NULL)
			goto error;
		dirp->dd_seek = 0;
		flags &= ~DTF_REWIND;
	}

	dirp->dd_loc = 0;
	dirp->dd_fd = fd;
	dirp->dd_flags = flags;

	/*
	 * Set up seek point for rewinddir.
	 */
#ifdef _REENTRANT
	if (__isthreaded) {
		if ((dirp->dd_lock = malloc(sizeof(mutex_t))) == NULL)
			goto error;
		mutex_init((mutex_t *)dirp->dd_lock, NULL);
	}
#endif
	dirp->dd_rewind = telldir(dirp);
	return (dirp);
error:
	serrno = errno;
	if (dirp && dirp->dd_buf)
		free(dirp->dd_buf);
	if (dirp)
		free(dirp);
	if (fd != -1)
		(void)close(fd);
	errno = serrno;
	return NULL;
}
