/* $NetBSD: utils.c,v 1.24 2003/08/04 22:31:22 jschauma Exp $ */

/*-
 * Copyright (c) 1991, 1993, 1994
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
#ifndef lint
#if 0
static char sccsid[] = "@(#)utils.c	8.3 (Berkeley) 4/1/94";
#else
__RCSID("$NetBSD: utils.c,v 1.24 2003/08/04 22:31:22 jschauma Exp $");
#endif
#endif /* not lint */

#include <sys/mman.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <fts.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vis.h>

#include "extern.h"

int
set_utimes(const char *file, struct stat *fs)
{
    static struct timeval tv[2];

    TIMESPEC_TO_TIMEVAL(&tv[0], &fs->st_atimespec);
    TIMESPEC_TO_TIMEVAL(&tv[1], &fs->st_mtimespec);

    if (lutimes(file, tv)) {
	warn("lutimes: %s", file);
	return (1);
    }
    return (0);
}

int
copy_file(FTSENT *entp, int dne)
{
	static char buf[MAXBSIZE];
	struct stat to_stat, *fs;
	int ch, checkch, from_fd, rcount, rval, to_fd, wcount;
	char *p;
	
	if ((from_fd = open(entp->fts_path, O_RDONLY, 0)) == -1) {
		warn("%s", entp->fts_path);
		return (1);
	}

	fs = entp->fts_statp;

	/*
	 * If the file exists and we're interactive, verify with the user.
	 * If the file DNE, set the mode to be the from file, minus setuid
	 * bits, modified by the umask; arguably wrong, but it makes copying
	 * executables work right and it's been that way forever.  (The
	 * other choice is 666 or'ed with the execute bits on the from file
	 * modified by the umask.)
	 */
	if (!dne) {
		if (iflag) {
			(void)fprintf(stderr, "overwrite %s? ", to.p_path);
			checkch = ch = getchar();
			while (ch != '\n' && ch != EOF)
				ch = getchar();
			if (checkch != 'y' && checkch != 'Y') {
				(void)close(from_fd);
				return (0);
			}
		}
		/* overwrite existing destination file name */
		to_fd = open(to.p_path, O_WRONLY | O_TRUNC, 0);
	} else
		to_fd = open(to.p_path, O_WRONLY | O_TRUNC | O_CREAT,
		    fs->st_mode & ~(S_ISUID | S_ISGID));

	if (to_fd == -1 && fflag) {
		/*
		 * attempt to remove existing destination file name and
		 * create a new file
		 */
		(void)unlink(to.p_path);
		to_fd = open(to.p_path, O_WRONLY | O_TRUNC | O_CREAT,
			     fs->st_mode & ~(S_ISUID | S_ISGID));
	}

	if (to_fd == -1) {
		warn("%s", to.p_path);
		(void)close(from_fd);
		return (1);
	}

	rval = 0;

	/*
	 * There's no reason to do anything other than close the file
	 * now if it's empty, so let's not bother.
	 */

	if (fs->st_size > 0) {

		/*
		 * Mmap and write if less than 8M (the limit is so
		 * we don't totally trash memory on big files).
		 * This is really a minor hack, but it wins some CPU back.
		 */

		if (fs->st_size <= 8 * 1048576) {
			p = mmap(NULL, (size_t)fs->st_size, PROT_READ,
			    MAP_FILE|MAP_SHARED, from_fd, (off_t)0);
			if (p == MAP_FAILED) {
				goto mmap_failed;
			} else {
				(void) madvise(p, (size_t)fs->st_size,
				     MADV_SEQUENTIAL);
				if (write(to_fd, p, fs->st_size) !=
				    fs->st_size) {
					warn("%s", to.p_path);
					rval = 1;
				}
				if (munmap(p, fs->st_size) < 0) {
					warn("%s", entp->fts_path);
					rval = 1;
				}
			}
		} else {
mmap_failed:
			while ((rcount = read(from_fd, buf, MAXBSIZE)) > 0) {
				wcount = write(to_fd, buf, rcount);
				if (rcount != wcount || wcount == -1) {
					warn("%s", to.p_path);
					rval = 1;
					break;
				}
			}
			if (rcount < 0) {
				warn("%s", entp->fts_path);
				rval = 1;
			}
		}
	}

	if (rval == 1) {
		(void)close(from_fd);
		(void)close(to_fd);
		return (1);
	}

	if (pflag && setfile(fs, to_fd))
		rval = 1;
	/*
	 * If the source was setuid or setgid, lose the bits unless the
	 * copy is owned by the same user and group.
	 */
#define	RETAINBITS \
	(S_ISUID | S_ISGID | S_ISVTX | S_IRWXU | S_IRWXG | S_IRWXO)
	else if (fs->st_mode & (S_ISUID | S_ISGID) && fs->st_uid == myuid) {
		if (fstat(to_fd, &to_stat)) {
			warn("%s", to.p_path);
			rval = 1;
		} else if (fs->st_gid == to_stat.st_gid &&
		    fchmod(to_fd, fs->st_mode & RETAINBITS & ~myumask)) {
			warn("%s", to.p_path);
			rval = 1;
		}
	}
	(void)close(from_fd);
	if (close(to_fd)) {
		warn("%s", to.p_path);
		rval = 1;
	}
	/* set the mod/access times now after close of the fd */
	if (pflag && set_utimes(to.p_path, fs)) { 
	    rval = 1;
	}
	return (rval);
}

int
copy_link(FTSENT *p, int exists)
{
	int len;
	char target[MAXPATHLEN];

	if ((len = readlink(p->fts_path, target, sizeof(target)-1)) == -1) {
		warn("readlink: %s", p->fts_path);
		return (1);
	}
	target[len] = '\0';
	if (exists && unlink(to.p_path)) {
		warn("unlink: %s", to.p_path);
		return (1);
	}
	if (symlink(target, to.p_path)) {
		warn("symlink: %s", target);
		return (1);
	}
	return (pflag ? setfile(p->fts_statp, 0) : 0);
}

int
copy_fifo(struct stat *from_stat, int exists)
{
	if (exists && unlink(to.p_path)) {
		warn("unlink: %s", to.p_path);
		return (1);
	}
	if (mkfifo(to.p_path, from_stat->st_mode)) {
		warn("mkfifo: %s", to.p_path);
		return (1);
	}
	return (pflag ? setfile(from_stat, 0) : 0);
}

int
copy_special(struct stat *from_stat, int exists)
{
	if (exists && unlink(to.p_path)) {
		warn("unlink: %s", to.p_path);
		return (1);
	}
	if (mknod(to.p_path, from_stat->st_mode, from_stat->st_rdev)) {
		warn("mknod: %s", to.p_path);
		return (1);
	}
	return (pflag ? setfile(from_stat, 0) : 0);
}


/*
 * Function: setfile
 *
 * Purpose:
 *   Set the owner/group/permissions for the "to" file to the information
 *   in the stat structure.  If fd is zero, also call set_utimes() to set
 *   the mod/access times.  If fd is non-zero, the caller must do a utimes
 *   itself after close(fd).
 */
int
setfile(struct stat *fs, int fd)
{
	int rval, islink;

	rval = 0;
	islink = S_ISLNK(fs->st_mode);
	fs->st_mode &= S_ISUID | S_ISGID | S_IRWXU | S_IRWXG | S_IRWXO;

	/*
	 * Changing the ownership probably won't succeed, unless we're root
	 * or POSIX_CHOWN_RESTRICTED is not set.  Set uid/gid before setting
	 * the mode; current BSD behavior is to remove all setuid bits on
	 * chown.  If chown fails, lose setuid/setgid bits.
	 */
	if (fd ? fchown(fd, fs->st_uid, fs->st_gid) :
	    lchown(to.p_path, fs->st_uid, fs->st_gid)) {
		if (errno != EPERM) {
			warn("chown: %s", to.p_path);
			rval = 1;
		}
		fs->st_mode &= ~(S_ISUID | S_ISGID);
	}
	if (fd ? fchmod(fd, fs->st_mode) : lchmod(to.p_path, fs->st_mode)) {
		warn("chmod: %s", to.p_path);
		rval = 1;
	}

	if (!islink) {
		/*
		 * XXX
		 * NFS doesn't support chflags; ignore errors unless
		 * there's reason to believe we're losing bits.
		 * (Note, this still won't be right if the server
		 * supports flags and we were trying to *remove* flags
		 * on a file that we copied, i.e., that we didn't create.)
		 */
		errno = 0;
		if (fd ? fchflags(fd, fs->st_flags) :
		    chflags(to.p_path, fs->st_flags))
			if (errno != EOPNOTSUPP || fs->st_flags != 0) {
				warn("chflags: %s", to.p_path);
				rval = 1;
			}
	}
	/* if fd is non-zero, caller must call set_utimes() after close() */
	if (fd == 0 && set_utimes(to.p_path, fs))
	    rval = 1;
	return (rval);
}

void
usage(void)
{
	(void)fprintf(stderr,
	    "usage: %s [-R [-H | -L | -P]] [-f | -i] [-pv] src target\n"
	    "       %s [-R [-H | -L | -P]] [-f | -i] [-pv] src1 ... srcN directory\n",
	    getprogname(), getprogname());
	exit(1);
	/* NOTREACHED */
}

char *
printescaped(const char *src)
{
	size_t len;
	char *retval;

	len = strlen(src);
	if (len != 0 && SIZE_T_MAX/len <= 4) {
		errx(EXIT_FAILURE, "%s: name too long", src);
		/* NOTREACHED */
	}

	retval = (char *)malloc(4*len+1);
	if (retval != NULL) {
		if (stdout_ok)
			(void)strvis(retval, src, VIS_NL | VIS_CSTYLE);
		else
			(void)strcpy(retval, src);
		return retval;
	} else
		errx(EXIT_FAILURE, "out of memory!");
		/* NOTREACHED */
}
