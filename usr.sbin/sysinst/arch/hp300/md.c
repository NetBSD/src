/*	$NetBSD: md.c,v 1.4.14.1 2018/05/21 04:36:20 pgoyette Exp $ */

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
 * 3. The name of Piermont Information Systems Inc. may not be used to endorse
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

/* md.c -- Machine specific code for hp300 */
/* This file is in close sync with pmax, sparc, vax and x68k md.c */

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

void
md_init(void)
{
}

void
md_init_set_status(int flags)
{
	(void)flags;
}

int
md_get_info(void)
{
	char buf[1024];
	int fd;
	char dev_name[100];
	struct disklabel disklabel;

	snprintf(dev_name, sizeof(dev_name), "/dev/r%sc", pm->diskdev);

	fd = open(dev_name, O_RDONLY, 0);
	if (fd < 0) {
		if (logfp)
			(void)fprintf(logfp, "Can't open %s\n", dev_name);
		endwin();
		fprintf(stderr, "Can't open %s\n", dev_name);
		exit(1);
	}
	if (ioctl(fd, DIOCGDINFO, &disklabel) == -1) {
		if (logfp)
			(void)fprintf(logfp, "Can't read disklabel on %s.\n",
				dev_name);
		endwin();
		fprintf(stderr, "Can't read disklabel on %s.\n", dev_name);
		close(fd);
		exit(1);
	}
	if (disklabel.d_secsize != 512) {
		endwin();
		fprintf(stderr, "Non-512byte/sector disk is not supported.\n");
		close(fd);
		exit(1);
	}

	pm->dlcyl = disklabel.d_ncylinders;
	pm->dlhead = disklabel.d_ntracks;
	pm->dlsec = disklabel.d_nsectors;
	pm->sectorsize = disklabel.d_secsize;
	pm->dlcylsize = disklabel.d_secpercyl;
	pm->dlsize = pm->dlcyl*pm->dlhead*pm->dlsec;

	if (read(fd, buf, 1024) < 0) {
		endwin();
		fprintf(stderr, "Can't read %s\n", dev_name);
		close(fd);
		exit(1);
	}

	/* We will preserve the first cylinder as PART_BOOT for bootloader. */
	pm->ptstart = 0;

	close(fd);

	return 1;
}

/*
 * md back-end code for menu-driven BSD disklabel editor.
 */
int
md_make_bsd_partitions(void)
{

	return make_bsd_partitions();
}

/*
 * any additional partition validation
 */
int
md_check_partitions(void)
{
	/* hp300 partitions must be in order of the range. */
	int part, last;
	uint32_t start;

	start = 0;
	last = PART_A - 1;
	for (part = PART_A; part < MAXPARTITIONS; part++) {
		if (part == PART_RAW || part == PART_BOOT)
			continue;
		if (last >= PART_A && pm->bsdlabel[part].pi_size > 0) {
			msg_display(MSG_emptypart, part+'a');
			process_menu(MENU_ok, NULL);
			return 0;
		}
		if (pm->bsdlabel[part].pi_size == 0) {
			if (last < PART_A)
				last = part;
		} else {
			if (start >= pm->bsdlabel[part].pi_offset) {
				msg_display(MSG_ordering, part+'a');
				if (ask_yesno(NULL))
					return 0;
			}
			start = pm->bsdlabel[part].pi_offset;
		}
	}

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
 * hook called after upgrade() or install() has finished setting
 * up the target disk but immediately before the user is given the
 * ``disks are now set up'' message.
 *
 * On hp300, we use this opportunity to install the boot blocks.
 */
int
md_post_newfs(void)
{
	/* boot blocks ... */
	msg_display(MSG_dobootblks, pm->diskdev);
	if (run_program(RUN_DISPLAY | RUN_NO_CLEAR,
	    "/usr/sbin/installboot /dev/r%sc /usr/mdec/uboot.lif", pm->diskdev))
		process_menu(MENU_ok,
		    __UNCONST("Warning: disk is probably not bootable"));
	return 0;
}

int
md_post_extract(void)
{
	return 0;
}

void
md_cleanup_install(void)
{
#ifdef notyet			/* sed is too large for ramdisk */
	enable_rc_conf();
#endif
}

int
md_pre_update(void)
{
	return 1;
}

/* Upgrade support */
int
md_update(void)
{
	md_post_newfs();
	return 1;
}

/*
 * Used in bsddisklabel.c as BOOT_SIZE
 */
int
hp300_boot_size(void)
{
	int i;

	i = pm->dlcylsize;
	if (i >= 1024) /* XXX: bsddisklabel.c has a hack. */
		i = pm->dlcylsize * pm->sectorsize * 2;

	return i;
}

int
md_pre_mount()
{
	return 0;
}
