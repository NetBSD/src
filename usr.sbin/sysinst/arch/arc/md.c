/*	$NetBSD: md.c,v 1.2.2.2 2014/08/10 07:00:25 tls Exp $ */

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

/* md.c -- arc machine specific routines */

#include <sys/param.h>
#include <sys/sysctl.h>
#include <stdio.h>
#include <util.h>
#include <machine/cpu.h>

#include "defs.h"
#include "md.h"
#include "msg_defs.h"
#include "menu_defs.h"

/*
 * ARC BIOS reognizes only FAT, so we have to have a FAT partition
 * to store our native bootloader.
 */
static int nobootfs = 0;
static int bootpart_fat12 = PART_BOOT_FAT12;

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
	int i;
	int part;
	int maxpart = getmaxpartitions();
	int partstart;
	int part_raw, part_bsd;
	int ptend;
	int no_swap = 0;
	partinfo *p;

	/*
	 * Initialize global variables that track space used on this disk.
	 * Standard 4.4BSD 8-partition labels always cover whole disk.
	 */
	if (pm->ptsize == 0)
		pm->ptsize = pm->dlsize - pm->ptstart;
	if (pm->dlsize == 0)
		pm->dlsize = pm->ptstart + pm->ptsize;

	partstart = pm->ptstart;
	ptend = pm->ptstart + pm->ptsize;

	/* Ask for layout type -- standard or special */
	msg_display(MSG_layout,
		    pm->ptsize / (MEG / pm->sectorsize),
		    DEFROOTSIZE + DEFSWAPSIZE + DEFUSRSIZE,
		    DEFROOTSIZE + DEFSWAPSIZE + DEFUSRSIZE + XNEEDMB);

	process_menu(MENU_layout, NULL);

	/* Set so we use the 'real' geometry for rounding, input in MB */
	pm->current_cylsize = pm->dlcylsize;
	set_sizemultname_meg();

	/* Build standard partitions */
	memset(&pm->bsdlabel, 0, sizeof pm->bsdlabel);

	/* Set initial partition types to unused */
	for (part = 0 ; part < maxpart ; ++part)
		pm->bsdlabel[part].pi_fstype = FS_UNUSED;

	/* Whole disk partition */
	part_raw = getrawpartition();
	if (part_raw == -1)
		part_raw = PART_C;	/* for sanity... */
	pm->bsdlabel[part_raw].pi_offset = 0;
	pm->bsdlabel[part_raw].pi_size = pm->dlsize;

	if (part_raw == PART_D) {
		/* Probably a system that expects an i386 style mbr */
		part_bsd = PART_C;
		pm->bsdlabel[PART_C].pi_offset = pm->ptstart;
		pm->bsdlabel[PART_C].pi_size = pm->ptsize;
	} else {
		part_bsd = part_raw;
	}

	if (pm->bootsize != 0) {
		pm->bsdlabel[PART_BOOT_FAT12].pi_fstype = FS_MSDOS;
		pm->bsdlabel[PART_BOOT_FAT12].pi_size = pm->bootsize;
		pm->bsdlabel[PART_BOOT_FAT12].pi_offset = pm->bootstart;
		pm->bsdlabel[PART_BOOT_FAT12].pi_flags |= PART_BOOT_FAT12_PI_FLAGS;
		strlcpy(pm->bsdlabel[PART_BOOT_FAT12].pi_mount,
		    PART_BOOT_FAT12_PI_MOUNT,
		    sizeof pm->bsdlabel[PART_BOOT_FAT12].pi_mount);
	}

#ifdef PART_REST
	pm->bsdlabel[PART_REST].pi_offset = 0;
	pm->bsdlabel[PART_REST].pi_size = pm->ptstart;
#endif

	/*
	 * Save any partitions that are outside the area we are
	 * going to use.
	 * In particular this saves details of the other MBR
	 * partitions on a multiboot i386 system.
	 */
	 for (i = maxpart; i--;) {
		if (pm->bsdlabel[i].pi_size != 0)
			/* Don't overwrite special partitions */
			continue;
		p = &pm->oldlabel[i];
		if (p->pi_fstype == FS_UNUSED || p->pi_size == 0)
			continue;
		if (layoutkind == LY_USEEXIST) {
			if (PI_ISBSDFS(p))
				p->pi_flags |= PIF_MOUNT;
		} else {
			if (p->pi_offset < pm->ptstart + pm->ptsize &&
			    p->pi_offset + p->pi_size > pm->ptstart)
				/* Not outside area we are allocating */
				continue;
			if (p->pi_fstype == FS_SWAP)
				no_swap = 1;
		}
		pm->bsdlabel[i] = pm->oldlabel[i];
	 }

	if (layoutkind == LY_USEEXIST) {
		/* XXX Check we have a sensible layout */
		;
	} else
		get_ptn_sizes(partstart, ptend - partstart, no_swap);

	/*
	 * OK, we have a partition table. Give the user the chance to
	 * edit it and verify it's OK, or abort altogether.
	 */
 edit_check:
	if (edit_and_check_label(pm->bsdlabel, maxpart, part_raw, part_bsd) == 0) {
		msg_display(MSG_abort);
		return 0;
	}
	if (md_check_partitions() == 0)
		goto edit_check;

	/* Disk name */
	msg_prompt(MSG_packname, pm->bsddiskname, pm->bsddiskname, sizeof pm->bsddiskname);

	/* save label to disk for MI code to update. */
	(void)savenewlabel(pm->bsdlabel, maxpart);

	/* Everything looks OK. */
	return 1;
}

/*
 * any additional partition validation
 */
int
md_check_partitions(void)
{
	int part;

	/* we need to find a boot partition, otherwise we can't write our
	 * bootloader.  We make the assumption that the user hasn't done
	 * something stupid, like move it away from the MBR partition.
	 */
	for (part = PART_A; part < MAXPARTITIONS; part++)
		if (pm->bsdlabel[part].pi_fstype == FS_MSDOS) {
			bootpart_fat12 = part;
			return 1;
		}

	msg_display(MSG_nobootpartdisklabel);
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
	if (write_mbr(pm->diskdev, &mbr, 1) != 0) {
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
	if (get_ramsize() <= 32)
		set_swap(pm->diskdev, pm->bsdlabel);
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
	if (!nobootfs) {
		msg_display(msg_string(MSG_copybootloader), pm->diskdev);
		cp_to_target("/usr/mdec/boot", PART_BOOT_FAT12_PI_MOUNT);
	}

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
	msg_display(MSG_howtoboot);
	process_menu(MENU_ok, NULL);
}

int
md_pre_update(void)
{
	struct mbr_partition *part;
	mbr_info_t *ext;
	int i;

	if (get_ramsize() <= 32)
		set_swap(pm->diskdev, NULL);

	read_mbr(pm->diskdev, &mbr);
	/* do a sanity check of the partition table */
	for (ext = &mbr; ext; ext = ext->extended) {
		part = ext->mbr.mbr_parts;
		for (i = 0; i < MBR_PART_COUNT; part++, i++) {
			if (part->mbrp_type != MBR_PTYPE_FAT12)
				continue;
			if (part->mbrp_size < (MIN_FAT12_BOOT / 512)) {
				msg_display(MSG_boottoosmall);
				msg_display_add(MSG_nobootpart, 0);
				process_menu(MENU_yesno, NULL);
				if (!yesno)
					return 0;
				nobootfs = 1;
			}
		}
	}
	if (md_check_partitions() == 0)
		nobootfs = 1;
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
			if (part->mbrp_type == MBR_PTYPE_FAT12) {
				pm->bootstart = part->mbrp_start;
				pm->bootsize = part->mbrp_size;
				break;
			}
		}
	}
	if (pm->bootsize < (MIN_FAT12_BOOT / 512)) {
		msg_display(MSG_boottoosmall);
		msg_display_add(MSG_reeditpart, 0);
		process_menu(MENU_yesno, NULL);
		if (!yesno)
			return 0;
		return 1;
	}
	if (pm->bootstart == 0 || pm->bootsize == 0) {
		msg_display(MSG_nobootpart);
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
	part[0].mbrp_type = MBR_PTYPE_FAT12;
	part[0].mbrp_size = FAT12_BOOT_SIZE / 512;
	part[0].mbrp_start = bsec;
	part[0].mbrp_flag = MBR_PFLAG_ACTIVE;

	part[1].mbrp_type = MBR_PTYPE_NETBSD;
	part[1].mbrp_size = pm->dlsize - (bsec + FAT12_BOOT_SIZE / 512);
	part[1].mbrp_start = bsec + FAT12_BOOT_SIZE / 512;
	part[1].mbrp_flag = 0;

	pm->ptstart = part[1].mbrp_start;
	pm->ptsize = part[1].mbrp_size;
	pm->bootstart = part[0].mbrp_start;
	pm->bootsize = part[0].mbrp_size;
	return 1;
}

int
md_pre_mount()
{
	return 0;
}
