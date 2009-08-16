/*	$NetBSD: aout2elf.c,v 1.14 2009/08/16 17:12:48 pgoyette Exp $
 *
 * Copyright 1997 Piermont Information Systems Inc.
 * All rights reserved.
 *
 * Written by Philip A. Nelson for Piermont Information Systems Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Piermont Information Systems Inc.
 * 4. The name of Piermont Information Systems Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PIERMONT INFORMATION SYSTEMS INC. ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL PIERMONT INFORMATION SYSTEMS INC. BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/* aout2elf.c -- routines for upgrading an a.out system to ELF */

#include <sys/param.h>
#include <sys/exec.h>
#include <sys/exec_aout.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>

#include "defs.h"
#include "md.h"
#include "msg_defs.h"
#include "menu_defs.h"

/* Local prototypes */
static int is_aout_shared_lib(const char *name);
static void handle_aout_x_libs(const char *srcdir, const char *tgtdir);
static int handle_aout_libs(const char *dir, int op, const void *arg);
static char *target_realpath(const char *, char *);

#define LIB_COUNT	0
#define LIB_MOVE	1

/* XXX NAH. This probably needs moving to arch/<foo>/md.h
 *
 * a.out X libraries to move. These have not changed since 1.3.x
 */
const char *x_libs[] = {
	"libICE.so.6.3",
	"libPEX5.so.6.0",
	"libSM.so.6.0",
	"libX11.so.6.1",
	"libXIE.so.6.0",
	"libXaw.so.6.1",
	"libXext.so.6.3",
	"libXi.so.6.0",
	"libXmu.so.6.0",
	"libXp.so.6.2",
	"libXt.so.6.0",
	"libXtst.so.6.1",
	"liboldX.so.6.0",
};

static int
is_aout_shared_lib(const char *name)
{
	struct exec ex;
	struct stat st;
	int fd;

	if (stat(name, &st) < 0)
		return 0;
	if ((st.st_mode & (S_IFREG|S_IFLNK)) == 0)
		return 0;

	fd = open(name, O_RDONLY);
	if (fd < 0) {
		return 0;
	}
	if (read(fd, &ex, sizeof ex) < sizeof ex) {
		close(fd);
		return 0;
	}
	close(fd);
	if (N_GETMAGIC(ex) != ZMAGIC ||
	    (N_GETFLAG(ex) & EX_DYNAMIC) == 0)
		return 0;

	return 1;
}

static void
handle_aout_x_libs(const char *srcdir, const char *tgtdir)
{
	char src[MAXPATHLEN];
	int i;

	for (i = 0; i < (sizeof x_libs / sizeof (const char *)); i++) {
		snprintf(src, MAXPATHLEN, "%s/%s", srcdir, x_libs[i]);
		if (!is_aout_shared_lib(src))
			continue;
		run_program(0, "mv -f %s %s", src, tgtdir);
	}

	/*
	 * Don't care if it fails; X may not have been installed.
	 */
}

/*
 * Function to count or move a.out shared libraries.
 */
static int
handle_aout_libs(const char *dir, int op, const void *arg)
{
	DIR *dd;
	struct dirent *dp;
	char *full_name;
	const char *destdir = NULL; /* XXX -Wuninitialized [many] */
	int n;

	destdir = NULL;	/* XXX gcc */

	dd = opendir(dir);
	if (dd == NULL)
		return -1;

	n = 0;

	switch (op) {
	case LIB_COUNT:
		break;
	case LIB_MOVE:
		destdir = (const char *)arg;
		break;
	default:
		return -1;
	}

	while ((dp = readdir(dd)) != NULL) {
		/*
		 * strlen("libX.so")
		 */
		if (dp->d_namlen < 7)
			continue;
		if (strncmp(dp->d_name, "lib", 3) != 0)
			continue;

		if (asprintf(&full_name, "%s/%s", dir, dp->d_name) == -1)  {
			warn("Out of memory");
			continue;
		}

		if (!is_aout_shared_lib(full_name))
			goto endloop;

		switch (op) {
		case LIB_COUNT:
			n++;
			break;
		case LIB_MOVE:
			run_program(0, "mv -f %s %s/%s",
			    full_name, destdir, dp->d_name);
			break;
		}
		
endloop:
		free(full_name);
	}

	closedir(dd);

	return n;
}

static void
abort_libupdate(void)
{
	msg_display(MSG_aoutfail);
	process_menu(MENU_ok, NULL);
	exit(1);
}

int
move_aout_libs(void)
{
	int n, backedup = 0;
	char prefix[MAXPATHLEN], src[MAXPATHLEN];
	struct stat st;

	n = handle_aout_libs(target_expand("/usr/lib"), LIB_COUNT, NULL);
	if (n <= 0)
		return n;

	/*
	 * See if /emul/aout already exists, taking symlinks into
	 * account. If so, no need to create it, just use it.
	 */
	if (target_realpath("/emul/aout", prefix) != NULL && stat(prefix, &st) == 0)
		goto domove;

	/*
	 * See if /emul exists. If not, create it.
	 */
	if (target_realpath("/emul", prefix) == NULL || stat(prefix, &st) < 0) {
		strlcpy(prefix, target_expand("/emul"), sizeof(prefix));
		if (lstat(prefix, &st) == 0) {
			run_program(0, "mv -f %s %s", prefix,
			    target_expand("/emul.old"));
			backedup = 1;
		}
		scripting_fprintf(NULL, "mkdir %s\n", prefix);
		mkdir(prefix, 0755);
	}

	/*
	 * Can use strcpy, target_expand has made sure it fits into
	 * MAXPATHLEN. XXX all this copying is because concat_paths
	 * returns a pointer to a static buffer.
	 *
	 * If an old aout link exists (apparently pointing to nowhere),
	 * move it out of the way.
	 */
	strlcpy(src, concat_paths(prefix, "aout"), sizeof(src));
	if (lstat(src, &st) == 0) {
		run_program(0, "mv -f %s %s", src,
		    concat_paths(prefix, "aout.old"));
		backedup = 1;
	}

	/*
	 * We have created /emul if needed. Since no previous /emul/aout
	 * existed, we'll use a symbolic link in /emul to /usr/aout, to
	 * avoid overflowing the root partition.
	 */
	strlcpy(prefix, target_expand("/usr/aout"), sizeof(prefix));
	if (run_program(0, "mkdir -p %s", prefix))
		abort_libupdate();
	if (run_program(0, "ln -s %s %s", "/usr/aout", src))
		abort_libupdate();

domove:
	/*
	 * Rename etc and usr/lib if they already existed, so that we
	 * do not overwrite old files.
	 *
	 * Then, move /etc/ld.so.conf to /emul/aout/etc/ld.so.conf,
	 * and all a.out dynamic libraries from /usr/lib to
	 * /emul/aout/usr/lib. This is where the a.out code in ldconfig
	 * and ld.so respectively will find them.
	 */
	strlcpy(src, concat_paths(prefix, "usr/lib"), sizeof(src));
	run_program(0, "mv -f %s %s", src, concat_paths(prefix, "usr/lib.old"));
	strlcpy(src, concat_paths(prefix, "etc/ld.so.conf"), sizeof(src));
	run_program(0, "mv -f %s %s",
	    src, concat_paths(prefix, "etc/ld.so.conf.old"));
	if (run_program(0, "mkdir -p %s ", concat_paths(prefix, "usr/lib")))
		abort_libupdate();
	if (run_program(0, "mkdir -p %s ", concat_paths(prefix, "etc")))
		abort_libupdate();

	strlcpy(src, target_expand("/etc/ld.so.conf"), sizeof(src));
	if (run_program(0, "mv -f %s %s",
	    src, concat_paths(prefix, "etc/ld.so.conf")))
		abort_libupdate();

	strlcpy(src, target_expand("/usr/lib"), sizeof(src));
	n = handle_aout_libs(src, LIB_MOVE, concat_paths(prefix, "usr/lib"));

	if (run_program(0, "mkdir -p %s ",
	    concat_paths(prefix, "usr/X11R6/lib")))
		abort_libupdate();

	strlcpy(src, target_expand("/usr/X11R6/lib"), sizeof(src));
	handle_aout_x_libs(src, concat_paths(prefix, "usr/X11R6/lib"));

	if (backedup) {
		msg_display(MSG_emulbackup);
		process_menu(MENU_ok, NULL);
	}

	return n;
}

/*
 * XXXX had to include this to deal with symlinks in some places.
 * When the target * disk is mounted under /targetroot, absolute symlinks
 * on it don't work right.
 * This function will resolve them using the mountpoint as prefix.
 * Copied verbatim from libc, with added prefix handling.
 *
 * char *realpath(const char *path, char resolved_path[MAXPATHLEN]);
 *
 * Find the real name of path, by removing all ".", ".." and symlink
 * components.  Returns (resolved) on success, or (NULL) on failure,
 * in which case the path which caused trouble is left in (resolved).
 */
static char *
target_realpath(const char *path, char *resolved)
{
	struct stat sb;
	int fd, n, rootd, serrno, nlnk = 0;
	char *p, *q, wbuf[MAXPATHLEN];
	char solidus[2], empty[1];
	solidus[0] = '/';
	solidus[1] = '\0';
	empty[0] = '\0';

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
	if (target_prefix() != NULL && strcmp(target_prefix(), "") != 0)
		snprintf(resolved, MAXPATHLEN, "%s/%s", target_prefix(), path);
	else
		if (strlcpy(resolved, path, MAXPATHLEN) >= MAXPATHLEN) {
			errno = ENAMETOOLONG;
			goto err1;
		}
loop:
	q = strrchr(resolved, '/');
	if (q != NULL) {
		p = q + 1;
		if (q == resolved)
			q = solidus;
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
			n = readlink(p, wbuf, MAXPATHLEN - 1);
			if (n < 0)
				goto err1;
			wbuf[n] = '\0';
			if (wbuf[0] == '/')
				snprintf(resolved, MAXPATHLEN, "%s%s",
				    target_prefix(), wbuf);
			else
				strlcpy(resolved, wbuf, MAXPATHLEN);
			goto loop;
		}
		if (S_ISDIR(sb.st_mode)) {
			if (chdir(p) < 0)
				goto err1;
			p = empty;
		}
	}

	/*
	 * Save the last component name and get the full pathname of
	 * the current directory.
	 */
	if (strlcpy(wbuf, p, sizeof(wbuf)) >= sizeof(wbuf)) {
		errno = ENAMETOOLONG;
		goto err1;
	}

	/*
	 * Call the internal internal version of getcwd which
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
			if (strlcat(resolved, "/", MAXPATHLEN) >= MAXPATHLEN) {
				errno = ENAMETOOLONG;
				goto err1;
			}
		if (strlcat(resolved, wbuf, MAXPATHLEN) >= MAXPATHLEN) {
			errno = ENAMETOOLONG;
			goto err1;
		}
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
