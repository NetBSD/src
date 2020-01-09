/*	$NetBSD: md.c,v 1.11 2020/01/09 13:22:31 martin Exp $ */

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

/* md.c -- shark machine specific routines */

#include <stdio.h>
#include <curses.h>
#include <unistd.h>
#include <fcntl.h>
#include <util.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/param.h>

#include "defs.h"
#include "md.h"
#include "msg_defs.h"
#include "menu_defs.h"

int boardtype = BOARD_TYPE_NORMAL;

void
md_init(void)
{
	int rv;

	rv =run_program(RUN_SILENT|RUN_ERROR_OK, "sh -c 'ofctl -p / model | "
	    "fgrep \"Raspberry Pi\"'");
	if (rv != 0)
		return;

	/* this is some kind of Raspberry Pi */
	boardtype = BOARD_TYPE_RPI;
}

void
md_init_set_status(int flags)
{
	if (boardtype == BOARD_TYPE_RPI)
		set_kernel_set(SET_KERNEL_RPI);
}

bool
md_get_info(struct install_partition_desc *install)
{

	if (pm->no_mbr || pm->no_part)
		return true;

	if (pm->parts == NULL) {

		const struct disk_partitioning_scheme *ps =
		    select_part_scheme(pm, NULL, true, NULL);

		if (!ps)
			return false;

		struct disk_partitions *parts =
		   (*ps->create_new_for_disk)(pm->diskdev,
		   0, pm->dlsize, pm->dlsize, true, NULL);
		if (!parts)
			return false;

		pm->parts = parts;
		if (ps->size_limit > 0 && pm->dlsize > ps->size_limit)
			pm->dlsize = ps->size_limit;
	}

	if (boardtype == BOARD_TYPE_RPI)
		return set_bios_geom_with_mbr_guess(pm->parts);

	return true;
}

/*
 * md back-end code for menu-driven BSD disklabel editor.
 */
bool
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
	size_t i;

	if (boardtype == BOARD_TYPE_NORMAL)
		return true;
	if (boardtype == BOARD_TYPE_RPI) {
		for (i = 0; i < install->num; i++)
			if (install->infos[i].fs_type == FS_MSDOS)
				return true;

		msg_display(MSG_nomsdospart);
		process_menu(MENU_ok, NULL);
	}
	return false;
}

/*
 * hook called before writing new disklabel.
 */
bool
md_pre_disklabel(struct install_partition_desc *install,
    struct disk_partitions *parts)
{

	if (parts->parent == NULL)
		return true;	/* no outer partitions */

	parts = parts->parent;

	msg_display_subst(MSG_dofdisk, 3, parts->disk,
	    msg_string(parts->pscheme->name),
	    msg_string(parts->pscheme->short_name));

	/* write edited "MBR" onto disk. */
	if (!parts->pscheme->write_to_disk(parts)) {
		msg_display(MSG_wmbrfail);
		process_menu(MENU_ok, NULL);
		return false;
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
 */
int
md_post_newfs(struct install_partition_desc *install)
{
	return 0;
}

int
md_post_extract(struct install_partition_desc *install)
{
	char kernelbin[100];

	if (boardtype == BOARD_TYPE_NORMAL)
		return 0;
	if (boardtype == BOARD_TYPE_RPI) {
		snprintf(kernelbin, 100, "%s/netbsd.img", targetroot_mnt);
		if (file_exists_p(kernelbin)) {
			run_program(RUN_DISPLAY,
			    "/bin/cp %s /targetroot/boot/kernel.img", kernelbin);
		} else {
			msg_display(MSG_rpikernelmissing);
			process_menu(MENU_ok, NULL);
			return 1;
		}
	}
	return 0;
}

void
md_cleanup_install(struct install_partition_desc *install)
{
#ifndef DEBUG
	enable_rc_conf();
	add_rc_conf("sshd=YES\n");
	add_rc_conf("dhcpcd=YES\n");
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

int
md_check_mbr(struct disk_partitions *parts, mbr_info_t *mbri, bool quiet)
{
	mbr_info_t *ext;
	struct mbr_partition *part;
	int i, hasboot=0;

	if (boardtype == BOARD_TYPE_NORMAL)
		return 2;
	/* raspi code */
	if (boardtype == BOARD_TYPE_RPI) {
		for (ext = mbri; ext; ext = ext->extended) {
			part = ext->mbr.mbr_parts;
			for (i=0, hasboot=0; i < MBR_PART_COUNT; part++, i++) {
				if (part->mbrp_type != MBR_PTYPE_FAT16L &&
				    part->mbrp_type != MBR_PTYPE_FAT32L)
					continue;
				hasboot = 1;
				break;
			}
		}
		if (!hasboot) {
			if (quiet)
				return 2;
			msg_display(MSG_nomsdospart);
			return ask_reedit(parts);
		}
	}
	return 2;
}

bool
md_parts_use_wholedisk(struct disk_partitions *parts)
{
	part_id nbsd, boot;
	struct disk_part_info info;
	daddr_t offset;

	/*
	 * XXX - set (U)EFI install depending on boardtype
	 */

	if (boardtype == BOARD_TYPE_NORMAL) {
		/* this keeps it from creating /boot as msdos */
		pm->bootsize = 0;
		return parts_use_wholedisk(parts, 0, NULL);
	}

	/* raspi code */
	if (boardtype == BOARD_TYPE_RPI) {

		for (boot = 0; boot < parts->num_part; boot++) {
			if (!parts->pscheme->get_part_info(parts, boot, &info))
				continue;
			if (info.nat_type == NULL)
				continue;
			if (info.nat_type->generic_ptype == PT_FAT)
				break;
		}

		if (boot >= parts->num_part) {
			/* It's hopelessly corrupt, punt for now */
			msg_display(MSG_nomsdospart);
			process_menu(MENU_ok, NULL);
			return false;
		}
		pm->bootstart = info.start;
		pm->bootsize = info.size;
		offset = info.start + info.size + 1;
		memset(&info, 0, sizeof info);
		info.start = offset;
		info.size = parts->pscheme->max_free_space_at(parts, offset);
		info.nat_type = parts->pscheme->get_generic_part_type(PT_root);
		info.fs_type = FS_BSDFFS;
		info.fs_sub_type = 2;    

		nbsd = parts->pscheme->add_partition(parts, &info, NULL);
		if (nbsd == NO_PART)
			return false;

		parts->pscheme->get_part_info(parts, nbsd, &info);
		pm->ptstart = info.start;
		pm->ptsize = info.size;
		return true;
	}

	return parts_use_wholedisk(parts, 0, NULL);
}

/* returns false if no write-back of parts is required */
bool
md_mbr_update_check(struct disk_partitions *parts, mbr_info_t *mbri)
{
	return false;
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
