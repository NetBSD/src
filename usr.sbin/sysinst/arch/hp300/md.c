/*	$NetBSD: md.c,v 1.12 2022/01/29 16:01:18 martin Exp $ */

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

bool
md_get_info(struct install_partition_desc *install)
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

	return true;
}

/*
 * md back-end code for menu-driven BSD disklabel editor.
 */
int
md_make_bsd_partitions(struct install_partition_desc *install)
{

	return make_bsd_partitions(install);
}

/*
 * any additional partition validation
 */
bool
md_check_partitions(struct install_partition_desc *install)
{
	/* hp300 partitions must be in order of the range. */
	daddr_t last_end = 0;
	size_t i;
	char desc[STRSIZE];

	for (i = 0; i < install->num; i++) {
		if (i > 0) {
			/* skip raw part and similar */
			if (install->infos[i].cur_flags &
			    (PTI_SEC_CONTAINER|PTI_PSCHEME_INTERNAL|
			    PTI_RAW_PART))
				continue;

			if (install->infos[i].cur_start < last_end) {
				snprintf(desc, sizeof desc,
				    "%zu (%s)", i,
				    install->infos[i].mount);
				msg_fmt_display(MSG_ordering, "%s", desc);
				if (ask_yesno(NULL))
					return false;
			}
		}
		last_end = install->infos[i].cur_start + install->infos[i].size;
	}

	return true;
}

/*
 * hook called before writing new disklabel.
 */
bool
md_pre_disklabel(struct install_partition_desc *install,
    struct disk_partitions *parts)
{
	return true;
}

/*
 * hook called after writing disklabel to new target disk.
 */
bool
md_post_disklabel(struct install_partition_desc *install,
    struct disk_partitions *parts)
{
	return true;
}

/*
 * hook called after upgrade() or install() has finished setting
 * up the target disk but immediately before the user is given the
 * ``disks are now set up'' message.
 *
 * On hp300, we use this opportunity to install the boot blocks.
 */
int
md_post_newfs(struct install_partition_desc *install)
{
	/* boot blocks ... */
	msg_fmt_display(MSG_dobootblks, "%s", pm->diskdev);
	if (run_program(RUN_DISPLAY | RUN_NO_CLEAR,
	    "/usr/sbin/installboot /dev/r%sc /usr/mdec/uboot.lif", pm->diskdev))
		process_menu(MENU_ok,
		    __UNCONST("Warning: disk is probably not bootable"));

	wclear(stdscr);
	touchwin(stdscr);
	clearok(stdscr, 1);
	refresh();

	return 0;
}

int
md_post_extract(struct install_partition_desc *install, bool upgrade)
{
	return 0;
}

void
md_cleanup_install(struct install_partition_desc *install)
{
#ifdef notyet			/* sed is too large for ramdisk */
	enable_rc_conf();
#endif
}

int
md_pre_update(struct install_partition_desc *install)
{
	return 1;
}

/* Upgrade support */
int
md_update(struct install_partition_desc *install)
{
	md_post_newfs(install);
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
md_pre_mount(struct install_partition_desc *install, size_t ndx)
{
	return 0;
}

#ifdef HAVE_GPT
bool
md_gpt_post_write(struct disk_partitions *parts, part_id root_id,
    bool root_is_new, part_id efi_id, bool efi_is_new)
{
	/* no GPT boot support, nothing needs to be done here */
	return true;
}
#endif

bool
md_parts_use_wholedisk(struct disk_partitions *parts)
{
	return parts_use_wholedisk(parts, 0, NULL);
}

