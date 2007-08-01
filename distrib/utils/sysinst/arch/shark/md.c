/*	$NetBSD: md.c,v 1.21 2007/08/01 14:49:42 jmmv Exp $	*/

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
 *
 */

/* md.c -- shark machine specific routines */

#include <stdio.h>
#include <curses.h>
#include <unistd.h>
#include <fcntl.h>
#include <util.h>
#include <sys/types.h>
#include <sys/disklabel.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include "defs.h"
#include "md.h"
#include "msg_defs.h"
#include "menu_defs.h"

/*
 * Clears the disk's first sector by writing all zeros over it.
 * Returns 0 on success, -1 on failure.  Leaves the disk's path in
 * the output diskpath buffer for further usage in error messages.
 */
static int
clear_mbr(const char *disk, char *diskpath, size_t diskpathlen)
{
	int fd;
	char sector[512];

	fd = opendisk(disk, O_WRONLY, diskpath, diskpathlen, 0);
	if (fd < 0)
		return -1;

	memset(sector, 0, sizeof(sector));
	if (pwrite(fd, &sector, sizeof(sector), 0) < 0) {
		close(fd);
		return -1;
	}

	close(fd);
	return 0;
}

int
md_get_info(void)
{
	struct disklabel disklabel;
	int fd;
	char dev_name[100];

	if (strncmp(diskdev, "wd", 2) == 0)
		disktype = "ST506";
	else
		disktype = "SCSI";

	snprintf(dev_name, 100, "/dev/r%sc", diskdev);

	fd = open(dev_name, O_RDONLY, 0);
	if (fd < 0) {
		endwin();
		fprintf(stderr, "Can't open %s\n", dev_name);
		exit(1);
	}
	if (ioctl(fd, DIOCGDINFO, &disklabel) == -1) {
		endwin();
		fprintf(stderr, "Can't read disklabel on %s.\n", dev_name);
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

	return 1;
}

int
md_pre_disklabel(void)
{
	char diskpath[MAXPATHLEN];

	if (clear_mbr(diskdev, diskpath, sizeof(diskpath)) == -1) {
		msg_display(MSG_badclearmbr, diskpath);
		process_menu(MENU_ok, NULL);
	}

	return 0;
}

int
md_post_disklabel(void)
{
	return 0;
}

int
md_post_newfs(void)
{
	return 0;
}

int
md_copy_filesystem(void)
{
	return 0;
}

int
md_make_bsd_partitions(void)
{
	return make_bsd_partitions();
}

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
	wrefresh(curscr);
	wmove(stdscr, 0, 0);
	wclear(stdscr);
	wrefresh(stdscr);
	return 1;
}

void
md_cleanup_install(void)
{
#ifndef DEBUG
	enable_rc_conf();

	add_rc_conf("wscons=YES\n");

	/* Configure a single screen. */
	run_program(RUN_CHROOT,
		    "sed -an -e '/^screen/s/^/#/;/^mux/s/^/#/;"
		    "H;$!d;g;w /etc/wscons.conf' /etc/wscons.conf");

	run_program(0, "rm -f %s", target_expand("/sysinst"));
	run_program(0, "rm -f %s", target_expand("/.termcap"));
	run_program(0, "rm -f %s", target_expand("/.profile"));
#endif
}

int
md_pre_update(void)
{
	return 1;
}

void
md_init(void)
{
}

int
md_post_extract(void)
{

	msg_display(MSG_setbootdevice);
	process_menu(MENU_ok, NULL);

	return 0;
}
