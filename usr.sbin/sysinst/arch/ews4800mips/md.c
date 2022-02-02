/*	$NetBSD: md.c,v 1.4.2.3 2022/02/02 04:25:39 msaitoh Exp $	*/

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

/* md.c -- ews4800mips machine specific routines */

#include <sys/types.h>
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

	snprintf(dev_name, 100, "/dev/r%s%c", pm->diskdev, 'a' + getrawpartition());

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

/*
 * any additional partition validation
 */
bool
md_check_partitions(struct install_partition_desc *install)
{
	return true;
}

/*
 * hook called before writing new disklabel.
 */
bool
md_pre_disklabel(struct install_partition_desc *install,
    struct disk_partitions *parts)
{
	part_id part;
	struct disk_part_info info;
	daddr_t boot_offset = ews4800mips_boot_offset();

	/* make sure the boot parition is at the right offset */
	for (part = 0; part < parts->num_part; part++) {
		if (!parts->pscheme->get_part_info(parts, part, &info))
			continue;
		if (info.flags & (PTI_SEC_CONTAINER|PTI_WHOLE_DISK|
		    PTI_PSCHEME_INTERNAL|PTI_RAW_PART))
			continue;
		if (info.fs_type != PART_BOOT_TYPE)
			continue;
		if (info.start == boot_offset)
			continue;
		info.start = boot_offset;
		parts->pscheme->set_part_info(parts, part, &info, NULL);
	}

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
 * On the ews4800mips, we use this opportunity to install the boot blocks.
 */
int
md_post_newfs(struct install_partition_desc *install)
{
	int flags;

	flags = RUN_DISPLAY | RUN_FATAL;
	cp_to_target("/usr/mdec/boot", "/stand/boot");
	run_program(flags, "/usr/sbin/installboot /dev/r%s%c %s",
	    pm->diskdev, 'a' + getrawpartition(),
	    "/usr/mdec/bootxx_bfs");

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
#ifndef DEBUG
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

static int
ews4800mips_boot_offset(void)
{
	return pm->dlcylsize;
}

/*
 * used in bsddisklabel.c as BOOT_SIZE
 */
int
ews4800mips_boot_size(void)
{
	int i;

	/*
	 * pm->dlcylsize	: PDINFO block
	 * pm->dlcylsize	: 100 block
	 */
	i = pm->dlcylsize + 100;
	if (i >= 1024) /* XXX bsddisklabel.c hack. convert to byte count. */
		i = pm->dlcylsize * pm->sectorsize * 2;

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

int
md_pre_mount(struct install_partition_desc *install, size_t ndx)
{
	return 0;
}

bool
md_parts_use_wholedisk(struct disk_partitions *parts)
{
	struct disk_part_info boot_part = {
		.size = PART_BOOT / 512,
		.fs_type = PART_BOOT_TYPE,
	};

	boot_part.nat_type = parts->pscheme->get_fs_part_type(
	    PT_root, boot_part.fs_type, boot_part.fs_sub_type);

	return parts_use_wholedisk(parts, 1, &boot_part);
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

