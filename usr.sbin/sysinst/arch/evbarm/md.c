/*	$NetBSD: md.c,v 1.22 2022/01/29 16:01:17 martin Exp $ */

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
#include <sys/sysctl.h>

#include "defs.h"
#include "md.h"
#include "msg_defs.h"
#include "menu_defs.h"

int boardtype = BOARD_TYPE_NORMAL;

#define	SBSA_MODEL_STR	"netbsd,generic-acpi"
#define	RPI_MODEL_STR	"raspberrypi,"

void
md_init(void)
{
	static const int mib[2] = {CTL_HW, HW_MODEL};
	size_t len;
	char *cpu_model;

	sysctl(mib, 2, NULL, &len, NULL, 0);
	cpu_model = malloc(len);
	sysctl(mib, 2, cpu_model, &len, NULL, 0);

	if (strstr(cpu_model, RPI_MODEL_STR) != NULL)
		/* this is some kind of Raspberry Pi */
		boardtype = BOARD_TYPE_RPI;
	else if (strstr(cpu_model, SBSA_MODEL_STR) != NULL)
		/* some SBSA compatible machine */
		boardtype = BOARD_TYPE_ACPI;
	else
		/* unknown, assume u-boot + dtb */
		boardtype = BOARD_TYPE_NORMAL;

	free(cpu_model);
}

void
md_init_set_status(int flags)
{

	/*
	 * we will extract kernel variants piecewise manually
	 * later, just fetch the kernel set, do not unpack it.
	 */
	set_kernel_set(SET_KERNEL_1);
	set_noextract_set(SET_KERNEL_1);
}

bool
md_get_info(struct install_partition_desc *install)
{
	int res;

	if (pm->no_mbr || pm->no_part)
		return true;

again:
	if (pm->parts == NULL) {

		const struct disk_partitioning_scheme *ps =
		    select_part_scheme(pm, NULL, true, NULL);

		if (!ps)
			return false;

		struct disk_partitions *parts =
		   (*ps->create_new_for_disk)(pm->diskdev,
		   0, pm->dlsize, true, NULL);
		if (!parts)
			return false;

		pm->parts = parts;
		if (ps->size_limit > 0 && pm->dlsize > ps->size_limit)
			pm->dlsize = ps->size_limit;
	}

	/*
	 * If the selected scheme does not need two-stage partitioning
	 * (like GPT), do not bother to edit the outer partitions.
	 */
	if (pm->parts->pscheme->secondary_partitions == NULL ||
	    pm->parts->pscheme->secondary_scheme == NULL)
		return true;

	res = edit_outer_parts(pm->parts);
	if (res == 0)
		return false;
	else if (res == 1)
		return true;

	pm->parts->pscheme->destroy_part_scheme(pm->parts);
	pm->parts = NULL;
	goto again;
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
	size_t i;

	for (i = 0; i < install->num; i++)
		if (install->infos[i].fs_type == FS_MSDOS)
			return true;

	msg_display(MSG_nomsdospart);
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

	/*
	 * RAW_PART is 2 on evbarm and bad things happen if we
	 * write the MBR first and then the disklabel - so postpone
	 * the MBR to md_post_disklabel(), unlike other architecturs.
	 */
	return true;
}

/*
 * hook called after writing disklabel to new target disk.
 */
bool
md_post_disklabel(struct install_partition_desc *install,
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
evbarm_extract_finalize(int update)
{
	distinfo *dist;
	char kernelbin[100];
	int (*saved_fetch_fn)(const char *);
#ifdef	_LP64
#define EFIBOOT	"/usr/mdec/bootaa64.efi"
#else
#define EFIBOOT	"/usr/mdec/bootarm.efi"
#endif

	dist = get_set_distinfo(SET_KERNEL_1);
	if (dist == NULL)
		return 0;

	saved_fetch_fn = fetch_fn;
	extract_file_to(dist, false, "/", "./netbsd", false);
	fetch_fn = NULL;
	make_target_dir("/boot/EFI/boot");
	if (target_file_exists_p(EFIBOOT))
		cp_within_target(EFIBOOT, "/boot/EFI/boot", 0);

	if (boardtype == BOARD_TYPE_ACPI) {
		fetch_fn = saved_fetch_fn;
		return 0;
	}
	if (boardtype == BOARD_TYPE_NORMAL) {
		extract_file_to(dist, false, "/boot", "./netbsd.ub", false);
		fetch_fn = saved_fetch_fn;
		return 0;
	}
	if (boardtype == BOARD_TYPE_RPI) {
		extract_file_to(dist, false, "/boot", "./netbsd.img", false);
		fetch_fn = saved_fetch_fn;
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
	fetch_fn = saved_fetch_fn;
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

	return 2;
}

bool
md_parts_use_wholedisk(struct disk_partitions *parts)
{
	struct disk_part_info boot_part = {
		.size = boardtype == BOARD_TYPE_NORMAL ?
		    PART_BOOT_LARGE/parts->bytes_per_sector :
		    PART_BOOT/parts->bytes_per_sector,
		.fs_type = PART_BOOT_TYPE,
		.fs_sub_type = boardtype == BOARD_TYPE_NORMAL ?
		    MBR_PTYPE_FAT32L : MBR_PTYPE_FAT16L,
	};

	return parts_use_wholedisk(parts, 1, &boot_part);
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

void
evbarm_part_defaults(struct pm_devs *my_pm, struct part_usage_info *infos,
    size_t num_usage_infos)
{
	size_t i;

	for (i = 0; i < num_usage_infos; i++) {
		if (infos[i].fs_type == PART_BOOT_TYPE &&
		    infos[i].mount[0] != 0 &&
		    strcmp(infos[i].mount, PART_BOOT_MOUNT) == 0) {
			infos[i].size = boardtype == BOARD_TYPE_NORMAL ?
			    PART_BOOT_LARGE/my_pm->parts->bytes_per_sector :
			    PART_BOOT/my_pm->parts->bytes_per_sector;
			infos[i].fs_version = boardtype == BOARD_TYPE_NORMAL ?
			    MBR_PTYPE_FAT32L : MBR_PTYPE_FAT16L; 
			return;
		}
	}
}

