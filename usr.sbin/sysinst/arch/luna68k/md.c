/*	$NetBSD: md.c,v 1.11 2023/01/06 18:14:56 martin Exp $	*/

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

/* md.c -- luna68k machine specific routines */

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include <unistd.h>
#include <fcntl.h>
#include <util.h>

#include "defs.h"
#include "md.h"
#include "msg_defs.h"
#include "menu_defs.h"

#define PART_BOOT_BSIZE	4096
#define PART_BOOT_FSIZE	512

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
	struct disklabel disklabel;
	int fd;
	char dev_name[100];

	snprintf(dev_name, sizeof(dev_name), "/dev/r%sc", pm->diskdev);

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

	pm->dlcyl = disklabel.d_ncylinders;
	pm->dlhead = disklabel.d_ntracks;
	pm->dlsec = disklabel.d_nsectors;
	pm->sectorsize = disklabel.d_secsize;
	pm->dlcylsize = disklabel.d_secpercyl;

	/*
	 * Compute whole disk size. Take max of (pm->dlcyl*pm->dlhead*pm->dlsec)
	 * and secperunit,  just in case the disk is already labelled.
	 * (If our new label's RAW_PART size ends up smaller than the
	 * in-core RAW_PART size  value, updating the label will fail.)
	 */
	pm->dlsize = pm->dlcyl * pm->dlhead * pm->dlsec;
	if (disklabel.d_secperunit > pm->dlsize)
		pm->dlsize = disklabel.d_secperunit;

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

static part_id
find_boot_part(struct install_partition_desc *install)
{
	for (size_t i = 0; i < install->num; i++) {
		if (install->infos[i].fs_type != PART_BOOT_TYPE)
			continue;
		if (install->infos[i].fs_version != PART_BOOT_SUBT)
			continue;
		if (install->infos[i].size < (PART_BOOT/512))
			continue;
		if (install->infos[i].cur_start > 0)
			continue;
		return i;
	}
	return NO_PART;
}

/*
 * any additional partition validation
 */
bool
md_check_partitions(struct install_partition_desc *install)
{

	/*
	 * Make sure that a boot partition (old 4.3BSD UFS) is prepared
	 * properly for our native bootloader.
	 */
	if (find_boot_part(install) != NO_PART)
		return true;

	msg_display(MSG_nobootpartdisklabel);
	process_menu(MENU_ok, NULL);
	return false;
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

static int
copy_bootloader(const char *diskdev)
{
	const char *mntdir = "/mnt2";

	msg_fmt_display(MSG_copybootloader, "%s", diskdev);
	if (!run_program(RUN_SILENT | RUN_ERROR_OK,
	    "mount %s %s", diskdev, mntdir)) {
		mnt2_mounted = 1;
		run_program(0, "/bin/cp /usr/mdec/boot %s", mntdir);
		run_program(RUN_SILENT | RUN_ERROR_OK, "umount %s", mntdir);
		mnt2_mounted = 0;
	} else {
		/* XXX print proper error message */
		return 1;
	}
	return 0;
}

/*
 * hook called after install() has finished setting up the target disk
 * but immediately before the user is given the ``disks are now set up''
 * message.
 */
int
md_post_newfs(struct install_partition_desc *install)
{
	char rdisk[STRSIZE], disk[STRSIZE];
	part_id boot_part = find_boot_part(install);

	if (boot_part == NO_PART)
		return 1;

	if (!install->infos[boot_part].parts->pscheme->get_part_device(
	    install->infos[boot_part].parts,
	    install->infos[boot_part].cur_part_id,
 	    rdisk, sizeof rdisk, NULL, raw_dev_name, true, true))
		return 1;
	if (!install->infos[boot_part].parts->pscheme->get_part_device(
	    install->infos[boot_part].parts,
	    install->infos[boot_part].cur_part_id,
	    disk, sizeof disk, NULL, plain_name, true, true))
		return 1;

	if (run_program(RUN_DISPLAY | RUN_PROGRESS,
	    "/sbin/newfs -V2 -O 0 -b %d -f %d %s",
	    PART_BOOT_BSIZE, PART_BOOT_FSIZE, rdisk))
		return 1;
	return copy_bootloader(disk);
}

int
md_post_extract(struct install_partition_desc *install, bool upgrade)
{

	return 0;
}

void
md_cleanup_install(struct install_partition_desc *install)
{

#ifndef DEBUG
	enable_rc_conf();
#endif
	msg_display(MSG_howtoboot);
	process_menu(MENU_ok, NULL);
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
	const char *mntdir = "/mnt2";
	char bootpath[MAXPATHLEN];
	struct stat sb;
	bool hasboot = false;
	char disk[STRSIZE];
	part_id boot_part = find_boot_part(install);

	if (boot_part == NO_PART)
		return 0;

	if (!install->infos[boot_part].parts->pscheme->get_part_device(
	    install->infos[boot_part].parts,
	    install->infos[boot_part].cur_part_id,
 	    disk, sizeof disk, NULL, plain_name, true, true))
		return 0;

	/*
	 * Check if there is a boot UFS parttion and it has the old bootloader.
	 * We'll update bootloader only if the old one was installed.
	 */
	if (!run_program(RUN_SILENT | RUN_ERROR_OK,
	    "mount -r %s %s", disk, mntdir)) {
		mnt2_mounted = 1;
		snprintf(bootpath, sizeof(bootpath), "%s/%s", mntdir, "boot");
		if (stat(bootpath, &sb) == 0 && S_ISREG(sb.st_mode))
			hasboot = true;
		run_program(RUN_SILENT | RUN_ERROR_OK, "umount %s", mntdir);
		mnt2_mounted = 0;
		if (hasboot)
			(void)copy_bootloader(disk);
	}
	return 1;
}

int
md_pre_mount(struct install_partition_desc *install, size_t ndx)
{
	return 0;
}

bool
md_parts_use_wholedisk(struct disk_partitions *parts)
{
	return parts_use_wholedisk(parts, 0, NULL);
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
