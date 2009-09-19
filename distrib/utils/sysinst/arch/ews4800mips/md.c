/*	$NetBSD: md.c,v 1.7 2009/09/19 14:57:28 abs Exp $	*/

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

/* md.c -- ews4800mips machine specific routines */

#include <sys/types.h>
#include <sys/disklabel.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <stdio.h>
#include <curses.h>
#include <unistd.h>
#include <fcntl.h>
#include <util.h>

#include "defs.h"
#include "md.h"
#include "msg_defs.h"
#include "menu_defs.h"

static int ews4800mips_boot_offset(void);

void
md_init(void)
{
}

void
md_init_set_status(int minimal)
{
	(void)minimal;
}

int
md_get_info(void)
{
	struct disklabel disklabel;
	int fd;
	char dev_name[100];

	snprintf(dev_name, 100, "/dev/r%s%c", diskdev, 'a' + getrawpartition());

	fd = open(dev_name, O_RDONLY, 0);
	if (fd < 0) {
		endwin();
		fprintf (stderr, "Can't open %s\n", dev_name);
		exit(1);
	}
	if (ioctl(fd, DIOCGDINFO, &disklabel) == -1) {
		endwin();
		fprintf (stderr, "Can't read disklabel on %s.\n", dev_name);
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
	dlsize = dlcyl * dlhead * dlsec;
	if (disklabel.d_secperunit > dlsize)
		dlsize = disklabel.d_secperunit;

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
	return 1;
}

/*
 * hook called before writing new disklabel.
 */
int
md_pre_disklabel(void)
{
	bsdlabel[PART_BOOT].pi_offset = ews4800mips_boot_offset();

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
 * On the ews4800mips, we use this opportunity to install the boot blocks.
 */
int
md_post_newfs(void)
{
	int flags;

	flags = RUN_DISPLAY | RUN_FATAL;
	cp_to_target("/usr/mdec/boot", "/stand/boot");
	run_program(flags, "/usr/sbin/installboot /dev/r%s%c %s",
	    diskdev, 'a' + getrawpartition(),
	    "/usr/mdec/bootxx_bfs");

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
#ifndef DEBUG
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

static int
ews4800mips_boot_offset(void)
{
	return dlcylsize;
}

/*
 * used in bsddisklabel.c as BOOT_SIZE
 */
int
ews4800mips_boot_size(void)
{
	int i;

	/*
	 * dlcylsize	: PDINFO block
	 * dlcylsize	: 100 block
	 */
	i = dlcylsize + 100;
	if (i >= 1024) /* XXX bsddisklabel.c hack. convert to byte count. */
		i = dlcylsize * sectorsize * 2;

	return i;
}

/*
 * used in bsddisklabel.c as SYSVBFS_SIZE
 */
int
ews4800mips_sysvbfs_size(void)
{
	return (8 * 1024 * 1024) / 512;	/* 8MB */
}
