/*	$NetBSD: md.c,v 1.14 2022/01/29 16:01:20 martin Exp $	*/

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

/* md.c -- prep machine specific routines */

#include <sys/param.h>
#include <sys/sysctl.h>
#include <stdio.h>
#include <util.h>
#include <machine/cpu.h>

#include "defs.h"
#include "md.h"
#include "msg_defs.h"
#include "menu_defs.h"
#include "endian.h"

int prep_nobootfix = 0, prep_rawdevfix = 0;
size_t prep_bootpart = ~0U;

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

	res = set_bios_geom_with_mbr_guess(pm->parts);
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
	size_t part;

	/* we need to find a boot partition, otherwise we can't write our
	 * "bootblock".  We make the assumption that the user hasn't done
	 * something stupid, like move it away from the MBR partition.
	 */
	for (part = 0; part < install->num; part++)
		if (install->infos[part].fs_type == FS_BOOT) {
			prep_bootpart = part;
			return true;
		}

	msg_display(MSG_prepnobootpart);
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
md_post_extract(struct install_partition_desc *install, bool upgrade)
{
	char rawdev[100], bootpart[100], bootloader[100];
	int contype;

	/* if we can't make it bootable, just punt */
	if (prep_nobootfix)
		return 0;

	process_menu(MENU_prepconsole, &contype);
	if (contype == 1)
		snprintf(bootloader, 100, "/usr/mdec/boot_com0");
	else
		snprintf(bootloader, 100, "/usr/mdec/boot");

	snprintf(rawdev, 100, "/dev/r%s%c", pm->diskdev,
		(char)('a' + getrawpartition()));
	snprintf(bootpart, 100, "/dev/r%s%c", pm->diskdev,
		(char)('a' + prep_bootpart));
	if (prep_rawdevfix)
		run_program(RUN_DISPLAY|RUN_CHROOT,
		    "/usr/mdec/mkbootimage -b %s -k /netbsd "
		    "-r %s /.bootimage", bootloader, rawdev);
	else
		run_program(RUN_DISPLAY|RUN_CHROOT,
		    "/usr/mdec/mkbootimage -s -b %s -k /netbsd /.bootimage",
		    bootloader);
	run_program(RUN_DISPLAY|RUN_CHROOT, "/bin/dd if=/.bootimage of=%s "
	    "bs=512 conv=sync", bootpart);

	return 0;
}

void
md_cleanup_install(struct install_partition_desc *install)
{
#ifndef DEBUG
	enable_rc_conf();
#endif
	run_program(0, "rm -f %s", target_expand("/.bootimage"));
}

int
md_pre_update(struct install_partition_desc *install)
{
	size_t i;

	/* do a sanity check of the partition table */
	for (i = 0; i < install->num; i++) {
		if (install->infos[i].fs_type != PART_BOOT_TYPE)
			continue;
		if (install->infos[i].size < (int)(MIN_PREP_BOOT/512)) {
			msg_display(MSG_preptoosmall);
			msg_fmt_display_add(MSG_prepnobootpart, "%d", 0);
			if (!ask_yesno(NULL))
				return 0;
			prep_nobootfix = 1;
		}
		if (install->infos[i].cur_start == 0)
			prep_rawdevfix = 1;
	}
	if (!md_check_partitions(install))
		prep_nobootfix = 1;
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
md_check_mbr(struct disk_partitions *parts, mbr_info_t *mbri, bool quiet)
{
	mbr_info_t *ext;
	struct mbr_partition *part;
	int i;

	for (ext = mbri; ext; ext = ext->extended) {
		part = ext->mbr.mbr_parts;
		for (i = 0; i < MBR_PART_COUNT; part++, i++) {
			if (part->mbrp_type != MBR_PTYPE_PREP)
				continue;
			pm->bootstart = part->mbrp_start;
			pm->bootsize = part->mbrp_size;
			break;
		}
	}
	if (pm->bootsize < (int)(MIN_PREP_BOOT/512)) {
		msg_display(MSG_preptoosmall);
		return ask_reedit(parts);
	}
	if (pm->bootstart == 0 || pm->bootsize == 0) {
		if (quiet)
			return 0;
		msg_display(MSG_nopreppart);
		return ask_reedit(parts);
	}
	return 2;
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

int
md_pre_mount(struct install_partition_desc *install, size_t ndx)
{
	return 0;
}

bool
md_mbr_update_check(struct disk_partitions *parts, mbr_info_t *mbri)
{
	return false;	/* no change, no need to write back */
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

