/*	$NetBSD: aout2elf.c,v 1.2 2002/02/03 22:44:21 skrll Exp $
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

/* aout2elf.c -- routines for upgrading an a.out system to elf */

#include <sys/param.h>
#include <sys/exec.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defs.h"
#include "md.h"
#include "msg_defs.h"
#include "menu_defs.h"

/* Local prototypes */
static int is_aout_shared_lib(const char *name);
static void handle_aout_x_libs(const char *srcdir, const char *tgtdir);
static int handle_aout_libs(const char *dir, int op, const void *arg);

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
		close(fd);
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
		run_prog(0, NULL, "mv -f %s %s", src, tgtdir);
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
	char *fullname;
	const char *destdir;
	int n;

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

		asprintf(&fullname, "%s/%s", dir, dp->d_name);

		if (!is_aout_shared_lib(fullname))
			goto endloop;

		switch (op) {
		case LIB_COUNT:
			n++;
			break;
		case LIB_MOVE:
			run_prog(0, NULL, "mv -f %s %s/%s",
			    fullname, destdir, dp->d_name);
			break;
		}
		
endloop:
		free(fullname);
	}

	closedir(dd);

	return n;
}

int
move_aout_libs()
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
		strcpy(prefix, target_expand("/emul"));
		if (lstat(prefix, &st) == 0) {
			run_prog(0, NULL, "mv -f %s %s", prefix,
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
	strcpy(src, concat_paths(prefix, "aout"));
	if (lstat(src, &st) == 0) {
		run_prog(0, NULL, "mv -f %s %s", src,
		    concat_paths(prefix, "aout.old"));
		backedup = 1;
	}

	/*
	 * We have created /emul if needed. Since no previous /emul/aout
	 * existed, we'll use a symbolic link in /emul to /usr/aout, to
	 * avoid overflowing the root partition.
	 */
	strcpy(prefix, target_expand("/usr/aout"));
	run_prog(RUN_FATAL, MSG_aoutfail, "mkdir -p %s", prefix);
	run_prog(RUN_FATAL, MSG_aoutfail, "ln -s %s %s",
	    "/usr/aout", src);

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
	strcpy(src, concat_paths(prefix, "usr/lib"));
	run_prog(0, NULL, "mv -f %s %s", src,
	    concat_paths(prefix, "usr/lib.old"));
	strcpy(src, concat_paths(prefix, "etc/ld.so.conf"));
	run_prog(0, NULL, "mv -f %s %s", src,
	    concat_paths(prefix, "etc/ld.so.conf.old"));
	run_prog(RUN_FATAL, MSG_aoutfail, "mkdir -p %s ",
	    concat_paths(prefix, "usr/lib"));
	run_prog(RUN_FATAL, MSG_aoutfail, "mkdir -p %s ",
	    concat_paths(prefix, "etc"));

	strcpy(src, target_expand("/etc/ld.so.conf"));
	run_prog(RUN_FATAL, MSG_aoutfail, "mv -f %s %s", src,
	    concat_paths(prefix, "etc/ld.so.conf"));

	strcpy(src, target_expand("/usr/lib"));
	n = handle_aout_libs(src, LIB_MOVE,
	    concat_paths(prefix, "usr/lib"));

	run_prog(RUN_FATAL, MSG_aoutfail, "mkdir -p %s ",
	    concat_paths(prefix, "usr/X11R6/lib"));

	strcpy(src, target_expand("/usr/X11R6/lib"));
	handle_aout_x_libs(src, concat_paths(prefix, "usr/X11R6/lib"));

	if (backedup) {
		msg_display(MSG_emulbackup);
		process_menu(MENU_ok);
	}

	return n;
}
