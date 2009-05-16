/*	$NetBSD: md.c,v 1.5 2009/05/16 10:40:17 nonaka Exp $	*/

/*
 * Copyright 1997 Piermont Information Systems Inc.
 * All rights reserved.
 *
 * Written by Philip A. Nelson for Piermont Information Systems Inc.
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
 *
 */

/* md.c -- Machine specific code for ofppc */

#include <sys/param.h>
#include <sys/sysctl.h>

#include <stdio.h>
#include <util.h>

#include <machine/cpu.h>

#include "defs.h"
#include "msg_defs.h"
#include "menu_defs.h"
#include "md.h"
#include "endian.h"

/* We use MBR_PTYPE_PREP like port-prep does. */
static int nonewfsmsdos = 0, nobootfix = 0, noprepfix=0;
static int bootpart_fat12 = PART_BOOT_FAT12;
static int bootpart_binfo = PART_BOOT_BINFO;
static int bootpart_prep = PART_BOOT_PREP;
static int bootinfo_mbr = 1;

/* bootstart/bootsize are for the fat */
int binfostart, binfosize, bprepstart, bprepsize;

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
				bootstart = part->mbrp_start;
				bootsize = part->mbrp_size;
			} else if (part->mbrp_type == MBR_PTYPE_PREP &&
			    part->mbrp_size < 50) {
				/* this is the bootinfo partition */
				binfostart = part->mbrp_start;
				binfosize = part->mbrp_size;
				bootinfo_mbr = i+1;
			} else if (part->mbrp_type == MBR_PTYPE_PREP &&
			    part->mbrp_size > 50) {
				bprepstart = part->mbrp_start;
				bprepsize = part->mbrp_size;
			}
			break;
		}
	}
	
	/* we need to either have a pair of prep partitions, or a single
	 * fat.  if neither, things are broken. */
	if (!(bootsize >= (MIN_FAT12_BOOT/512) ||
		(binfosize >= (MIN_BINFO_BOOT/512) &&
		    bprepsize >= (MIN_PREP_BOOT/512)))) {
		msg_display(MSG_bootnotright);
		msg_display_add(MSG_reeditpart, 0);
		process_menu(MENU_yesno, NULL);
		if (!yesno)
			return 0;
		return 1;
	}

	/* check the prep partitions */
	if ((binfosize > 0 || bprepsize > 0) &&
	    (binfosize < (MIN_BINFO_BOOT/512) ||
		bprepsize < (MIN_PREP_BOOT/512))) {
		msg_display(MSG_preptoosmall);
		msg_display_add(MSG_reeditpart, 0);
		process_menu(MENU_yesno, NULL);
		if (!yesno)
			return 0;
		return 1;
	}
	
	/* check the fat12 parititons */
	if (bootsize > 0 && bootsize < (MIN_FAT12_BOOT/512)) {
		msg_display(MSG_boottoosmall);
		msg_display_add(MSG_reeditpart, 0);
		process_menu(MENU_yesno, NULL);
		if (!yesno)
			return 0;
		return 1;
	}

	/* if both sets contain zero, thats bad */
	if ((bootstart == 0 || bootsize == 0) &&
	    (binfosize == 0 || binfostart == 0 ||
		bprepsize == 0 || bprepstart == 0)) {
		msg_display(MSG_nobootpart);
		msg_display_add(MSG_reeditpart, 0);
		process_menu(MENU_yesno, NULL);
		if (!yesno)
			return 0;
		return 1;
	}
	return 2;
}

/*
 * NOTE, we use a reserved partition type, because some RS/6000 machines hang
 * hard if they find a FAT12, and if we use type prep, that indicates that
 * it should be read raw.
 * One partition for FAT12 booting
 * One partition for NetBSD
 * One partition to hold the bootinfo.txt file
 * One partition to hold ofwboot
 */

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

	part[0].mbrp_type = MBR_PTYPE_RESERVED_x21;
	part[0].mbrp_size = FAT12_BOOT_SIZE/512;
	part[0].mbrp_start = bsec;
	part[0].mbrp_flag = 0;

	part[1].mbrp_type = MBR_PTYPE_NETBSD;
	part[1].mbrp_size = dlsize - (bsec + FAT12_BOOT_SIZE/512 +
	    BINFO_BOOT_SIZE/512 + PREP_BOOT_SIZE/512);
	part[1].mbrp_start = bsec + FAT12_BOOT_SIZE/512 + BINFO_BOOT_SIZE/512 +
	    PREP_BOOT_SIZE/512;
	part[1].mbrp_flag = MBR_PFLAG_ACTIVE;

	part[2].mbrp_type = MBR_PTYPE_PREP;
	part[2].mbrp_size = BINFO_BOOT_SIZE/512;
	part[2].mbrp_start = bsec + FAT12_BOOT_SIZE/512;
	part[2].mbrp_flag = 0;

	part[3].mbrp_type = MBR_PTYPE_PREP;
	part[3].mbrp_size = PREP_BOOT_SIZE/512;
	part[3].mbrp_start = bsec + FAT12_BOOT_SIZE/512 + BINFO_BOOT_SIZE/512;
	part[3].mbrp_flag = 0;
	
	ptstart = part[1].mbrp_start;
	ptsize = part[1].mbrp_size;
	bootstart = part[0].mbrp_start;
	bootsize = part[0].mbrp_size;
	binfostart = part[2].mbrp_start;
	binfosize= part[2].mbrp_size;
	bprepstart = part[3].mbrp_start;
	bprepsize = part[3].mbrp_size;
	bootinfo_mbr = 4;

	return 1;
}

int
md_get_info(void)
{

	read_mbr(diskdev, &mbr);
	md_bios_info(diskdev);
	return edit_mbr(&mbr);
}

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

int
md_post_disklabel(void)
{
	char bootdev[100];

	if (bootstart == 0 || bootsize == 0)
		return 0;
	
	snprintf(bootdev, sizeof bootdev, "/dev/r%s%c", diskdev,
	    'a'+bootpart_fat12);
	run_program(RUN_DISPLAY, "/sbin/newfs_msdos %s", bootdev);
	
	return 0;
}

int
md_post_newfs(void)
{

	/* just in case */
	run_program(RUN_DISPLAY, "/sbin/umount /targetroot/boot");
	return 0;
}

int
md_copy_filesystem(void)
{

	return 0;
}


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
	if (ptsize == 0)
		ptsize = dlsize - ptstart;
	if (dlsize == 0)
		dlsize = ptstart + ptsize;

	partstart = ptstart;
	ptend = ptstart + ptsize;

	/* Ask for layout type -- standard or special */
	msg_display(MSG_layout,
		    ptsize / (MEG / sectorsize),
		    DEFROOTSIZE + DEFSWAPSIZE + DEFUSRSIZE,
		    DEFROOTSIZE + DEFSWAPSIZE + DEFUSRSIZE + XNEEDMB);

	process_menu(MENU_layout, NULL);

	/* Set so we use the 'real' geometry for rounding, input in MB */
	current_cylsize = dlcylsize;
	set_sizemultname_meg();

	/* Build standard partitions */
	memset(&bsdlabel, 0, sizeof bsdlabel);

	/* Set initial partition types to unused */
	for (part = 0 ; part < maxpart ; ++part)
		bsdlabel[part].pi_fstype = FS_UNUSED;

	/* Whole disk partition */
	part_raw = getrawpartition();
	if (part_raw == -1)
		part_raw = PART_C;	/* for sanity... */
	bsdlabel[part_raw].pi_offset = 0;
	bsdlabel[part_raw].pi_size = dlsize;

	if (part_raw == PART_D) {
		/* Probably a system that expects an i386 style mbr */
		part_bsd = PART_C;
		bsdlabel[PART_C].pi_offset = ptstart;
		bsdlabel[PART_C].pi_size = ptsize;
	} else {
		part_bsd = part_raw;
	}

	if (bootsize != 0) {
		bsdlabel[PART_BOOT_FAT12].pi_fstype = FS_MSDOS;
		bsdlabel[PART_BOOT_FAT12].pi_size = bootsize;
		bsdlabel[PART_BOOT_FAT12].pi_offset = bootstart;
		bsdlabel[PART_BOOT_FAT12].pi_flags |= PART_BOOT_FAT12_PI_FLAGS;
		strlcpy(bsdlabel[PART_BOOT_FAT12].pi_mount,
		    PART_BOOT_FAT12_PI_MOUNT,
		    sizeof bsdlabel[PART_BOOT_FAT12].pi_mount);
	}
	if (binfosize != 0) {
		bsdlabel[PART_BOOT_BINFO].pi_fstype = FS_OTHER;
		bsdlabel[PART_BOOT_BINFO].pi_size = binfosize;
		bsdlabel[PART_BOOT_BINFO].pi_offset = binfostart;
	}
	if (bprepsize != 0) {
		bsdlabel[PART_BOOT_PREP].pi_fstype = FS_BOOT;
		bsdlabel[PART_BOOT_PREP].pi_size = bprepsize;
		bsdlabel[PART_BOOT_PREP].pi_offset = bprepstart;
	}

#ifdef PART_REST
	bsdlabel[PART_REST].pi_offset = 0;
	bsdlabel[PART_REST].pi_size = ptstart;
#endif

	/*
	 * Save any partitions that are outside the area we are
	 * going to use.
	 * In particular this saves details of the other MBR
	 * partitions on a multiboot i386 system.
	 */
	 for (i = maxpart; i--;) {
		if (bsdlabel[i].pi_size != 0)
			/* Don't overwrite special partitions */
			continue;
		p = &oldlabel[i];
		if (p->pi_fstype == FS_UNUSED || p->pi_size == 0)
			continue;
		if (layoutkind == 4) {
			if (PI_ISBSDFS(p))
				p->pi_flags |= PIF_MOUNT;
		} else {
			if (p->pi_offset < ptstart + ptsize &&			
			    p->pi_offset + p->pi_size > ptstart)
				/* Not outside area we are allocating */
				continue;
			if (p->pi_fstype == FS_SWAP)
				no_swap = 1;
		}
		bsdlabel[i] = oldlabel[i];
	 }

	if (layoutkind == 4) {
		/* XXX Check we have a sensible layout */
		;
	} else
		get_ptn_sizes(partstart, ptend - partstart, no_swap);

	/*
	 * OK, we have a partition table. Give the user the chance to
	 * edit it and verify it's OK, or abort altogether.
	 */
 edit_check:
	if (edit_and_check_label(bsdlabel, maxpart, part_raw, part_bsd) == 0) {
		msg_display(MSG_abort);
		return 0;
	}
	if (md_check_partitions() == 0)
		goto edit_check;

	/* Disk name */
	msg_prompt(MSG_packname, bsddiskname, bsddiskname, sizeof bsddiskname);

	/* save label to disk for MI code to update. */
	(void) savenewlabel(bsdlabel, maxpart);

	/* Everything looks OK. */
	return (1);
}

int
md_check_partitions(void)
{
	int part, fprep=0, ffat=0;

	/* we need to find a boot partition, otherwise we can't create
	 * our msdos fs boot partition.  We make the assumption that
	 * the user hasn't done something stupid, like move it away
	 * from the MBR partition.
	 */
	for (part = PART_A; part < MAXPARTITIONS; part++) {
		if (bsdlabel[part].pi_fstype == FS_MSDOS) {
			bootpart_fat12 = part;
			ffat++;
		} else if (bsdlabel[part].pi_fstype == FS_BOOT) {
			bootpart_prep = part;
			fprep++;
		} else if (bsdlabel[part].pi_fstype == FS_OTHER) {
			bootpart_binfo = part;
			fprep++;
		}
	}
	/* oh, the confusion */
	if (ffat >= 1 && fprep < 2)
		return 1;
	if (ffat < 1 && fprep >= 2)
		return 2;
	if (ffat >=1 && fprep >= 2)
		return 3;

	msg_display(MSG_nobootpartdisklabel);
	process_menu(MENU_ok, NULL);
	return 0;
}

/* Upgrade support */
int
md_update(void)
{

	endwin();
	nonewfsmsdos = 1;
	md_copy_filesystem();
	md_post_newfs();
	wrefresh(curscr);
	wmove(stdscr, 0, 0);
	wclear(stdscr);
	wrefresh(stdscr);
	return 1;
}

void
md_cleanup_install(void)
{

	enable_rc_conf();
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
			if (part->mbrp_type == MBR_PTYPE_PREP &&
			    part->mbrp_size > 50)
				bootinfo_mbr = i+1;
			if (part->mbrp_type == MBR_PTYPE_RESERVED_x21 &&
			    part->mbrp_size < (MIN_FAT12_BOOT/512)) {
				msg_display(MSG_boottoosmall);
				msg_display_add(MSG_nobootpartdisklabel, 0);
				process_menu(MENU_yesno, NULL);
				if (!yesno)
					return 0;
				nobootfix = 1;
			}
		}
	}

	i = md_check_partitions();
	switch (i) {
	case 0: nobootfix=1; noprepfix=1; break;
	case 1: noprepfix=1; break;
	case 2: nobootfix=1; break;
	default: break;
	}
	
	return 1;
}

int
md_bios_info(char *dev)
{
	int cyl, head;
	daddr_t sec;

	msg_display(MSG_nobiosgeom, dlcyl, dlhead, dlsec);
	if (guess_biosgeom_from_mbr(&mbr, &cyl, &head, &sec) >= 0)
		msg_display_add(MSG_biosguess, cyl, head, sec);
	set_bios_geom(cyl, head, sec);
	return 0;
}

int
md_post_extract(void)
{
	char bootdev[100], bootbdev[100], version[64];

	/* if we can't make it bootable, just punt */
	if (nobootfix && noprepfix)
		return 0;

	snprintf(version, sizeof version, "NetBSD/%s %s", MACH, REL);
	run_program(RUN_DISPLAY, "/usr/mdec/mkbootinfo '%s' %d "
	    "/tmp/bootinfo.txt", version, bootinfo_mbr);
	
	if (!nobootfix) {
		snprintf(bootdev, sizeof bootdev, "/dev/r%s%c", diskdev,
		    'a'+bootpart_fat12);
		snprintf(bootbdev, sizeof bootbdev, "/dev/%s%c", diskdev,
		    'a'+bootpart_fat12);

		if (nonewfsmsdos == 0) 
			run_program(RUN_DISPLAY, "/sbin/newfs_msdos %s",
			    bootdev);
		run_program(RUN_DISPLAY, "/sbin/mount_msdos %s /mnt2",
		    bootbdev);
		run_program(RUN_DISPLAY, "/bin/mkdir -p /mnt2/ppc");
		run_program(RUN_DISPLAY, "/bin/mkdir -p /mnt2/netbsd");
		run_program(RUN_DISPLAY, "/bin/cp /usr/mdec/ofwboot "
		    "/mnt2/netbsd");
		run_program(RUN_DISPLAY, "/bin/cp /tmp/bootinfo.txt "
		    "/mnt2/ppc");
		run_program(RUN_DISPLAY,
		    "/bin/cp /usr/mdec/ofwboot /mnt2/ofwboot");
		run_program(RUN_DISPLAY, "/sbin/umount /mnt2");
	}

	if (!noprepfix) {
		snprintf(bootdev, sizeof bootdev, "/dev/r%s%c", diskdev,
		    'a'+bootpart_prep);
		snprintf(bootbdev, sizeof bootbdev, "/dev/%s%c", diskdev,
		    'a'+bootpart_prep);
		run_program(RUN_DISPLAY, "/bin/dd if=/dev/zero of=%s bs=512",
		    bootdev);
		run_program(RUN_DISPLAY, "/bin/dd if=/usr/mdec/ofwboot "
		    "of=%s bs=512", bootbdev);
		
		snprintf(bootdev, sizeof bootdev, "/dev/r%s%c", diskdev,
		    'a'+bootpart_binfo);
		snprintf(bootbdev, sizeof bootbdev, "/dev/%s%c", diskdev,
		    'a'+bootpart_binfo);
		run_program(RUN_DISPLAY, "/bin/dd if=/dev/zero of=%s bs=512",
		    bootdev);
		run_program(RUN_DISPLAY, "/bin/dd if=/tmp/bootinfo.txt "
		    "of=%s bs=512", bootbdev);
	}
	
	return 0;
}

void
md_init(void)
{

	/* Nothing to do */
}

void
md_init_set_status(int minimal)
{
	(void)minimal;
}
