/*	$NetBSD: md.c,v 1.4 1999/08/16 08:29:07 abs Exp $ */

/*
 * Copyright 1997 Piermont Information Systems Inc.
 * All rights reserved.
 *
 * Based on code written by Philip A. Nelson for Piermont Information
 * Systems Inc.  Modified by Minoura Makoto for x68k.
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

/* md.c -- Machine specific code for x68k */
/* This file is in close sync with pmax, sparc, and vax md.c */

#include <stdio.h>
#include <unistd.h>
#include <sys/disklabel.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <util.h>

#include "defs.h"
#include "md.h"
#include "msg_defs.h"
#include "menu_defs.h"
#include "bsddisklabel.c"

#ifdef notyet
#undef NDOSPART 8
#define NDOSPART 16
typedef struct parttab {
	struct dos_partition dosparts[NDOSPART];
} parttab;

parttab md_disklabel;
int md_freepart;
int md_nfreepart;
#endif
int md_need_newdisk = 0;

/* prototypes */
static int md_newdisk __P((void));
#define COPYRIGHT "NetBSD/x68k SCSI primary boot. "		\
		  "(C) 1999 by The NetBSD Foundation, Inc. "	\
		  "Written by sysinst."

int
md_get_info(void)
{
	char buf[1024];
	int fd;
	char devname[100];
	struct disklabel disklabel;

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
	if (disklabel.d_secsize != 512) {
		endwin();
		fprintf(stderr, "Non-512byte/sector disk is not supported.\n");
		close(fd);
		exit(1);
	}

	dlcyl = disklabel.d_ncylinders;
	dlhead = disklabel.d_ntracks;
	dlsec = disklabel.d_nsectors;
	sectorsize = disklabel.d_secsize;
	dlcylsize = disklabel.d_secpercyl;
	dlsize = dlcyl*dlhead*dlsec;

	if (read(fd, buf, 1024) < 0) {
		endwin();
		fprintf(stderr, "Can't read %s\n", devname);
		close(fd);
		exit(1);
	}
	if (memcmp(buf, "X68SCSI1", 8) != 0)
		md_need_newdisk = 1;
#ifdef notyet
	else
		if (read(fd, md_disklabel, sizeof(md_disklabel)) < 0) {
			endwin();
			fprintf(stderr, "Can't read %s\n", devname);
			close(fd);
			exit(1);
		}
#endif
	/* preserve first 64 sectors for system. */
	dlsize -= 64;

	/* preserve existing partitions? */

	close(fd);

	/* Compute minimum NetBSD partition sizes (in sectors). */
	minfsdmb = STDNEEDMB * (MEG / sectorsize);

	return 1;
}

#ifndef DEBUG
static int
md_newdisk(void)
{
	int mbootfd;
	char devname[100];
	int fd;
	char buf[1024];
	size_t size = dlsize + 64;

	snprintf(devname, 100, "/dev/r%sc", diskdev);
	fd = open(devname, O_WRONLY);
	if (fd < 0) {
		endwin();
		fprintf(stderr, "Can't open %s\n", devname);
		exit(1);
	}

	msg_display(MSG_newdisk, diskdev, diskdev);

	/* Write disk mark */
	memset(buf, 0, 1024);
	sprintf(buf, "X68SCSI1%c%c%c%c%c%c%c%c%s",
		2, 0,
		(size>>24)&0xff, (size>>16)&0xff, (size>>8)&0xff, size&0xff,
		1, 0, COPYRIGHT);
	lseek(fd, 0, SEEK_SET);
	if (write(fd, buf, 1024) < 0) {
		endwin();
		close(fd);
		fprintf(stderr, "Can't write mark on %s\n", devname);
		exit(1);
	}

	/* Write primary boot */
	memset(buf, 0, 1024);
	mbootfd = open("/usr/mdec/mboot", O_RDONLY);
	if (mbootfd < 0) {
		endwin();
		close(fd);
		fprintf(stderr, "Can't read mboot.\n");
		exit(1);
	}
	if (read(mbootfd, buf, 1024) < 0) {
		endwin();
		close(fd);
		close(mbootfd);
		fprintf(stderr, "Can't read mboot.\n");
		exit(1);
	}
	close(mbootfd);
	if (write(fd, buf, 1024) != 1024) {
		endwin();
		close(fd);
		fprintf(stderr, "Can't write mboot.\n");
		exit(1);
	}

	/* Create empty partition map */
#ifdef notyet
	memset(&md_disklabel, 0, sizeof(md_disklabel));
	sprintf((char*) md_disklabel, "X68K%c%c%c%c%c%c%c%c%c%c%c%c",
		0, 0, 0, 32,
		(size>>24)&0xff, (size>>16)&0xff, (size>>8)&0xff, size&0xff,
		(size>>24)&0xff, (size>>16)&0xff, (size>>8)&0xff, size&0xff);
	if (write(fd, md_disklabel, 1024) < 0) {
		endwin();
		close(fd);
		fprintf(stderr, "Can't create partition table.\n");
		exit(1);
	}
#else
	memset(buf, 0, 1024);
	sprintf(buf, "X68K%c%c%c%c%c%c%c%c%c%c%c%c",
		0, 0, 0, 32,
		(size>>24)&0xff, (size>>16)&0xff, (size>>8)&0xff, size&0xff,
		(size>>24)&0xff, (size>>16)&0xff, (size>>8)&0xff, size&0xff);
	if (write(fd, buf, 1024) < 0) {
		endwin();
		close(fd);
		fprintf(stderr, "Can't create partition table.\n");
		exit(1);
	}
#endif

	close (fd);

	return 0;
}
#else
static int
md_newdisk(fd, size, buf)
	int fd;
	size_t size;
	char *buf;
{
	return 0;
}
#endif

#ifdef notyet
static int
md_check_partitions(void)
{
	int i, j;
	int preserve;

	/* check existing BSD partitions. */
	for (i = 0; i < NDOSPART; i++) {
		if (md_disklabel.dosparts[i].dp_size == 0)
			break;
		if (memcmp(md_disklabel.dosparts[i].dp_typename, "Human68k", 8)) {
			msg_display(MSG_existing);
			process_menu(MENU_noyes);
			preserve = yesno;
			break;
		}
	}
	emptylabel(bsdlabel);
	bsdlabel[C].pi_fstype = FS_UNUSED;
	bsdlabel[C].pi_offset = 0;
	bsdlabel[C].pi_size = dlsize;
	for (i = 0, j = A; i < NDOSPART;) {
		if (j == C) {
			j++;
			continue;
		}
		if (!preserve &&
		    memcmp(md_disklabel.dosparts[i].dp_typename,
			    "Human68k", 8)) {
			/* discard it. */
			i++;
			continue;
		}
		bsdlabel[j].pi_fstype = (i == 1) ? FS_SWAP : FS_BSDFFS;
		bsdlabel[j].pi_offset = md_disklabel.dosparts[i].dp_start*2;
		bsdlabel[j].pi_size = md_disklabel.dosparts[i].dp_size*2;
		i++;
		j++;
	}
	if (j > 6) {
		msg_display(MSG_nofreepart, diskdev);
		return 0;
	}
	md_nfreepart = 8 - j;

	/* check for free space */
	fsptsize = bsdlabel[A].pi_offset - 64;
	if (fptsize <= 0) {	/* XXX: should not be 0; minfsdb?  */
		msg_display(MSG_notfirst, diskdev);
		process_menu(MENU_ok);
		exit(1);
	}

	/* Partitions should be preserved in md_make_bsdpartitions() */
}
#endif

/*
 * hook called before writing new disklabel.
 */
int
md_pre_disklabel(void)
{
	if (md_need_newdisk)
		md_newdisk ();
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
 * On the x68k, we use this opportunity to install the boot blocks.
 */
int
md_post_newfs(void)
{
	/* boot blocks ... */
	msg_display(MSG_dobootblks, diskdev);
	return run_prog(0, 1, NULL,
	    "/usr/mdec/installboot -v /usr/mdec/sdboot /dev/r%sa",
	    diskdev);
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
	/* X68k partitions must be in order of the range. */
	int part, start = 0, last = A-1;

	for (part = A; part < 8; part++) {
		if (part == C)
			continue;
		if (last >= A && bsdlabel[part].pi_size > 0) {
			msg_display(MSG_emptypart, part+'a');
			process_menu(MENU_ok);
			return 0;
		}
		if (bsdlabel[part].pi_size == 0) {
			if (last < A)
				last = part;
		} else {
			if (start >= bsdlabel[part].pi_offset) {
				msg_display(MSG_ordering, part+'a');
				process_menu(MENU_yesno);
				if (yesno)
					return 0;
			}
			start = bsdlabel[part].pi_offset;
		}
	}

	return 1;
}

/* Upgrade support */
int
md_update(void)
{
	endwin();
	md_copy_filesystem();
	md_post_newfs();
	puts(CL);		/* XXX */
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

	run_prog(1, 0, NULL, "mv -f %s %s", realto, realfrom);

	run_prog(0, 0, NULL, "rm -f %s", target_expand("/sysinst"));
	run_prog(0, 0, NULL, "rm -f %s", target_expand("/.termcap"));
	run_prog(0, 0, NULL, "rm -f %s", target_expand("/.profile"));
}
