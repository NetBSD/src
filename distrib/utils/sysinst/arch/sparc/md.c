/*	$NetBSD: md.c,v 1.25 2000/12/31 13:08:08 jdc Exp $	*/

/*
 * Copyright 1997 Piermont Information Systems Inc.
 * All rights reserved.
 *
 * Based on code written by Philip A. Nelson for Piermont Information
 * Systems Inc.
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
 */

/* changes from the i386 version made by mrg */

/* md.c -- sparc machine specific routines */
/* This file is in close sync with pmax, vax, and x68k md.c */

#include <sys/types.h>
#include <sys/disklabel.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/exec.h>
#include <stdio.h>
#include <curses.h>
#include <unistd.h>
#include <fcntl.h>
#include <util.h>
#include <dirent.h>

#include "defs.h"
#include "md.h"
#include "msg_defs.h"
#include "menu_defs.h"
#include "bsddisklabel.c"

static int move_aout_libs (void);
static int handle_aout_libs (const char *dir, int op, const void *arg);
static int is_aout_shared_lib (const char *);
static void handle_aout_x_libs (const char *, const char *);

int
md_get_info(void)
{
	struct disklabel disklabel;
	int fd;
	char devname[100];

	snprintf(devname, 100, "/dev/r%sc", diskdev);

	fd = open(devname, O_RDONLY, 0);
	if (fd < 0) {
		if (logging)
			(void)fprintf(log, "Can't open %s\n", devname);
		endwin();
		fprintf(stderr, "Can't open %s\n", devname);
		exit(1);
	}
	if (ioctl(fd, DIOCGDINFO, &disklabel) == -1) {
		if (logging)
			(void)fprintf(log, "Can't read disklabel on %s.\n",
				devname);
		endwin();
		fprintf(stderr, "Can't read disklabel on %s.\n", devname);
		close(fd);
		exit(1);
	}
	close(fd);

	dlcyl = disklabel.d_ncylinders;
	dlhead = disklabel.d_ntracks;
	dlsec = disklabel.d_nsectors;
	sectorsize = disklabel.d_secsize;
	dlcylsize = disklabel.d_secpercyl;

	/*
	 * Compute whole disk size. Take max of (dlcyl*dlhead*dlsec)
	 * and secperunit,  just in case the disk is already labelled.
	 * (If our new label's RAW_PART size ends up smaller than the
	 * in-core RAW_PART size  value, updating the label will fail.)
	 */
	dlsize = dlcyl*dlhead*dlsec;
	if (disklabel.d_secperunit > dlsize)
		dlsize = disklabel.d_secperunit;

	/* Compute minimum NetBSD partition sizes (in sectors). */
	minfsdmb = STDNEEDMB * (MEG / sectorsize);

	return 1;
}

/*
 * hook called before writing new disklabel.
 */
int
md_pre_disklabel(void)
{
	return 0;
}

/*
 * hook called after writing disklabel to new target disk.
 */
int
md_post_disklabel(void)
{
	return 0;
}

/*
 * MD hook called after upgrade() or install() has finished setting
 * up the target disk but immediately before the user is given the
 * ``disks are now set up'' message, so that if power fails, they can
 * continue installation by booting the target disk and doing an
 * `upgrade'.
 *
 * On the sparc, we use this opportunity to install the boot blocks.
 */
int
md_post_newfs(void)
{
	/*
	 * Create a symlink of netbsd to netbsd.GENERIC
	 * XXX This is... less than ideal... but there is no md hook between
	 * get_and_unpack_sets() and sanity_check(), and we do not want to
	 * change kern.tgz until we replace the miniroot install
	 */
	symlink("netbsd.GENERIC",target_expand("/netbsd"));

	/* boot blocks ... */
	msg_display(MSG_dobootblks, diskdev);
	return (run_prog(RUN_DISPLAY, NULL, "/sbin/disklabel -W %s", diskdev) ||
		run_prog(RUN_DISPLAY, NULL, "/usr/mdec/binstall ffs /mnt"));
}

/*
 * some ports use this to copy the MD filesystem, we do not.
 */
int
md_copy_filesystem(void)
{
	return 0;
}

/*
 * md back-end code for menu-driven BSD disklabel editor.
 */
int
md_make_bsd_partitions(void)
{
	return(make_bsd_partitions());
}

/*
 * any additional partition validation
 */
int
md_check_partitions(void)
{
	return 1;
}

/* Upgrade support */
int
md_update(void)
{
	move_aout_libs();
	endwin();
	md_copy_filesystem();
	md_post_newfs();
	clearok(stdscr, TRUE);
	wmove(stdscr, 0, 0);
	wclear(stdscr);
	wrefresh(stdscr);
	return 1;
}

void
md_cleanup_install(void)
{
	char realfrom[STRSIZE];
	char realto[STRSIZE];
	char sedcmd[STRSIZE];

	strncpy(realfrom, target_expand("/etc/rc.conf"), STRSIZE);
	strncpy(realto, target_expand("/etc/rc.conf.install"), STRSIZE);

	sprintf(sedcmd, "sed 's/rc_configured=NO/rc_configured=YES/' < %s > %s",
	    realfrom, realto);
	if (logging)
		(void)fprintf(log, "%s\n", sedcmd);
	if (scripting)
		(void)fprintf(script, "%s\n", sedcmd);
	do_system(sedcmd);

	run_prog(RUN_FATAL, NULL, "mv -f %s %s", realto, realfrom);

	run_prog(0, NULL, "rm -f %s", target_expand("/sysinst"));
	run_prog(0, NULL, "rm -f %s", target_expand("/.termcap"));
	run_prog(0, NULL, "rm -f %s", target_expand("/.profile"));
}

int
md_pre_update()
{
	return 1;
}

void
md_init()
{
}

/*
 * a.out X libraries to move. These have not changed since 1.3.x
 */
static const char *x_libs[] = {
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

static int
move_aout_libs()
{
	int n;
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
		if (scripting)
			fprintf(script, "mkdir %s\n", prefix);
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
	run_prog(0, NULL, "mv -f %s %s", src,
	    concat_paths(prefix, "aout.old"));

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

	return n;
}
