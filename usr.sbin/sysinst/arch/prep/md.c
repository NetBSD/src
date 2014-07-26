/*	$NetBSD: md.c,v 1.1 2014/07/26 19:30:46 dholland Exp $	*/

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

int prep_nobootfix = 0, prep_rawdevfix = 0, prep_bootpart = PART_BOOT;

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
	return set_bios_geom_with_mbr_guess();
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
	int part;

	/* we need to find a boot partition, otherwise we can't write our
	 * "bootblock".  We make the assumption that the user hasn't done
	 * something stupid, like move it away from the MBR partition.
	 */
	for (part = PART_A; part < MAXPARTITIONS; part++)
		if (bsdlabel[part].pi_fstype == FS_BOOT) {
			prep_bootpart = part;
			return 1;
		}

	msg_display(MSG_prepnobootpart);
	process_menu(MENU_ok, NULL);
	return 0;
}

/*
 * hook called before writing new disklabel.
 */
int
md_pre_disklabel(void)
{
	msg_display(MSG_dofdisk);

	/* write edited MBR onto disk. */
	if (write_mbr(diskdev, &mbr, 1) != 0) {
		msg_display(MSG_wmbrfail);
		process_menu(MENU_ok, NULL);
		return 1;
	}
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
 */
int
md_post_newfs(void)
{
	return 0;
}

int
md_post_extract(void)
{
	char rawdev[100], bootpart[100], bootloader[100];

	/* if we can't make it bootable, just punt */
	if (prep_nobootfix)
		return 0;

	process_menu(MENU_prepconsole, NULL);
	if (yesno == 1)
		snprintf(bootloader, 100, "/usr/mdec/boot_com0");
	else
		snprintf(bootloader, 100, "/usr/mdec/boot");

	snprintf(rawdev, 100, "/dev/r%s%c", diskdev, 'a' + getrawpartition());
	snprintf(bootpart, 100, "/dev/r%s%c", diskdev, 'a' + prep_bootpart);
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
md_cleanup_install(void)
{
#ifndef DEBUG
	enable_rc_conf();
#endif
	run_program(0, "rm -f %s", target_expand("/.bootimage"));
}

int
md_pre_update(void)
{
	struct mbr_partition *part;
	mbr_info_t *ext;
	int i;

	read_mbr(diskdev, &mbr);
	/* do a sanity check of the partition table */
	for (ext = &mbr; ext; ext = ext->extended) {
		part = ext->mbr.mbr_parts;
		for (i = 0; i < MBR_PART_COUNT; part++, i++) {
			if (part->mbrp_type != MBR_PTYPE_PREP)
				continue;
			if (part->mbrp_size < (MIN_PREP_BOOT/512)) {
				msg_display(MSG_preptoosmall);
				msg_display_add(MSG_prepnobootpart, 0);
				process_menu(MENU_yesno, NULL);
				if (!yesno)
					return 0;
				prep_nobootfix = 1;
			}
			if (part->mbrp_start == 0)
				prep_rawdevfix = 1;
		}
	}
	if (md_check_partitions() == 0)
		prep_nobootfix = 1;
	return 1;
}

/* Upgrade support */
int
md_update(void)
{
	md_post_newfs();
	return 1;
}

int
md_check_mbr(mbr_info_t *mbri)
{
	mbr_info_t *ext;
	struct mbr_partition *part;
	int i;

	for (ext = mbri; ext; ext = ext->extended) {
		part = ext->mbr.mbr_parts;
		for (i = 0; i < MBR_PART_COUNT; part++, i++) {
			if (part->mbrp_type != MBR_PTYPE_PREP)
				continue;
			bootstart = part->mbrp_start;
			bootsize = part->mbrp_size;
			break;
		}
	}
	if (bootsize < (MIN_PREP_BOOT/512)) {
		msg_display(MSG_preptoosmall);
		msg_display_add(MSG_reeditpart, 0);
		process_menu(MENU_yesno, NULL);
		if (!yesno)
			return 0;
		return 1;
	}
	if (bootstart == 0 || bootsize == 0) {
		msg_display(MSG_nopreppart);
		msg_display_add(MSG_reeditpart, 0);
		process_menu(MENU_yesno, NULL);
		if (!yesno)
			return 0;
		return 1;
	}
	return 2;
}

int
md_mbr_use_wholedisk(mbr_info_t *mbri)
{
	struct mbr_sector *mbrs = &mbri->mbr;
	mbr_info_t *ext;
	struct mbr_partition *part;

	part = &mbrs->mbr_parts[0];
	/* Set the partition information for full disk usage. */
	while ((ext = mbri->extended)) {
		mbri->extended = ext->extended;
		free(ext);
	}
	memset(part, 0, MBR_PART_COUNT * sizeof *part);
#ifdef BOOTSEL
	memset(&mbri->mbrb, 0, sizeof mbri->mbrb);
#endif
	part[0].mbrp_type = MBR_PTYPE_PREP;
	part[0].mbrp_size = PREP_BOOT_SIZE/512;
	part[0].mbrp_start = bsec;
	part[0].mbrp_flag = MBR_PFLAG_ACTIVE;

	part[1].mbrp_type = MBR_PTYPE_NETBSD;
	part[1].mbrp_size = dlsize - (bsec + PREP_BOOT_SIZE/512);
	part[1].mbrp_start = bsec + PREP_BOOT_SIZE/512;
	part[1].mbrp_flag = 0;

	ptstart = part[1].mbrp_start;
	ptsize = part[1].mbrp_size;
	bootstart = part[0].mbrp_start;
	bootsize = part[0].mbrp_size;
	return 1;
}


int
md_pre_mount()
{
	return 0;
}
