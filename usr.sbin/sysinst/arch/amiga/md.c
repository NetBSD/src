/*	$NetBSD: md.c,v 1.9 2022/06/26 09:14:14 martin Exp $ */

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

/* md.c -- amiga machine specific routines */

#include <stdio.h>
#include <util.h>
#include <sys/param.h>
#include <machine/cpu.h>
#include <sys/sysctl.h>

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
	return true;
}

/*
 * md back-end code for menu-driven BSD disklabel editor.
 */
int
md_make_bsd_partitions(struct install_partition_desc *install)
{
	return 1;
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
	return false;
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

#ifdef DISKLABEL_NO_ONDISK_VERIFY
/*
 * hook to check if disklabel returned by readdisklabel(9) via DIOCGDINFO
 * seems the default one, on ports that have no BSD disklabel on disks.
 */
bool
md_disklabel_is_default(const struct disklabel *lp)
{
	bool maybe_default =
	    lp->d_npartitions == RAW_PART + 1 &&
	    lp->d_partitions[RAW_PART].p_size == 0x1fffffff &&
	    lp->d_partitions[0].p_size == lp->d_partitions[RAW_PART].p_size &&
	    lp->d_partitions[0].p_offset == 0 &&
	    lp->d_partitions[0].p_fstype == FS_BSDFFS;

	return maybe_default;
}
#endif

/*
 * hook called after upgrade() or install() has finished setting
 * up the target disk but immediately before the user is given the
 * ``disks are now set up'' message.
 */
int
md_post_newfs(struct install_partition_desc *install)
{
	/* boot blocks ... */
	msg_fmt_display(MSG_dobootblks, "%s", pm->diskdev);
	return run_program(RUN_DISPLAY,
	    "/usr/mdec/installboot -v /usr/mdec/xxboot /dev/r%sa", pm->diskdev);
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

int
md_pre_mount(struct install_partition_desc *install, size_t ndx)
{
	return 0;
}

#ifdef HAVE_GPT
/*
 * New GPT partitions have been written, update bootloader or remember
 * data untill needed in md_post_newfs
 */
bool
md_gpt_post_write(struct disk_partitions *parts, part_id root_id,
    bool root_is_new, part_id efi_id, bool efi_is_new)
{

	return true;
}
#endif

bool
md_parts_use_wholedisk(struct disk_partitions *parts)
{
	return parts_use_wholedisk(parts, 0, NULL);
}

