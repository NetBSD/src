/*	$NetBSD: md.c,v 1.1.4.2 2014/08/19 23:45:47 tls Exp $ */

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
#include <sys/disklabel.h>
#include <sys/ioctl.h>
#include <sys/param.h>

#include "defs.h"
#include "md.h"
#include "msg_defs.h"
#include "menu_defs.h"

int boardtype = 0, rpi_bootpart = PART_A;

void
md_prelim_menu(void)
{
	/* get the boardtype from the user */
	process_menu(MENU_prelim, NULL);
}

void
md_init(void)
{
}

void
md_init_set_status(int flags)
{
	if (boardtype == BOARD_TYPE_RPI)
		set_kernel_set(SET_KERNEL_RPI);
}

int
md_get_info(void)
{
	struct disklabel disklabel;
	int fd;
	char dev_name[100];

	if (boardtype == BOARD_TYPE_RPI)
		return set_bios_geom_with_mbr_guess();

	if (no_mbr)
		return 1;

	if (read_mbr(diskdev, &mbr) < 0)
		memset(&mbr.mbr, 0, sizeof(mbr.mbr)-2);

	if (edit_mbr(&mbr) == 0)
		return 0;

	if (strncmp(diskdev, "wd", 2) == 0)
		disktype = "ST506";
	else
		disktype = "SCSI";

	snprintf(dev_name, 100, "/dev/r%sc", diskdev);

	fd = open(dev_name, O_RDONLY, 0);
	if (fd < 0) {
		endwin();
		fprintf(stderr, "Can't open %s\n", dev_name);
		exit(1);
	}
	if (ioctl(fd, DIOCGDINFO, &disklabel) == -1) {
		endwin();
		fprintf(stderr, "Can't read disklabel on %s.\n", dev_name);
		close(fd);
		exit(1);
	}
	close(fd);

	dlcyl = disklabel.d_ncylinders;
	dlhead = disklabel.d_ntracks;
	dlsec = disklabel.d_nsectors;
	sectorsize = disklabel.d_secsize;
	dlcylsize = disklabel.d_secpercyl;

	/*
	 * Compute whole disk size. Take max of (dlcyl*dlhead*dlsec)
	 * and secperunit,  just in case the disk is already labelled.
	 * (If our new label's RAW_PART size ends up smaller than the
	 * in-core RAW_PART size  value, updating the label will fail.)
	 */
	dlsize = dlcyl*dlhead*dlsec;
	if (disklabel.d_secperunit > dlsize)
		dlsize = disklabel.d_secperunit;

	return 1;
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

	if (boardtype == BOARD_TYPE_NORMAL)
		return 1;
	if (boardtype == BOARD_TYPE_RPI) {
		for (part = PART_A; part < MAXPARTITIONS; part++)
			if (bsdlabel[part].pi_fstype == FS_MSDOS) {
				rpi_bootpart = part;
				return 1;
			}

		msg_display(MSG_nomsdospart);
		process_menu(MENU_ok, NULL);
	}
	return 0;
}

/*
 * hook called before writing new disklabel.
 */
int
md_pre_disklabel(void)
{
	if (no_mbr)
		return 0;

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
	char kernelbin[100];

	if (boardtype == BOARD_TYPE_NORMAL)
		return 0;
	if (boardtype == BOARD_TYPE_RPI) {
		snprintf(kernelbin, 100, "%s/netbsd.bin", targetroot_mnt);
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
md_cleanup_install(void)
{
#ifndef DEBUG
	enable_rc_conf();
	add_rc_conf("sshd=YES\n");
	add_rc_conf("dhcpcd=YES\n");
#endif
}

int
md_pre_update(void)
{
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
md_pre_mount()
{
	return 0;
}

int
md_check_mbr(mbr_info_t *mbri)
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
				if (part->mbrp_type != MBR_PTYPE_FAT32L)
					continue;
				hasboot = 1;
				break;
			}
		}
		if (!hasboot) {
			msg_display(MSG_nomsdospart);
			msg_display_add(MSG_reeditpart, 0);
			process_menu(MENU_yesno, NULL);
			if (!yesno)
				return 0;
			return 1;
		}
	}
	return 2;
}

int
md_mbr_use_wholedisk(mbr_info_t *mbri)
{
	struct mbr_sector *mbrs = &mbri->mbr;
	struct mbr_partition *part;
	int offset;

	if (boardtype == BOARD_TYPE_NORMAL) {
		/* this keeps it from creating /boot as msdos */
		bootsize = 0;
		return mbr_use_wholedisk(mbri);
	}

	/* raspi code */
	if (boardtype == BOARD_TYPE_RPI) {
		part = &mbrs->mbr_parts[0];
		if (part[0].mbrp_type != MBR_PTYPE_FAT32L) {
			/* It's hopelessly corrupt, punt for now */
			msg_display(MSG_nomsdospart);
			process_menu(MENU_ok, NULL);
			return 0;
		}
		offset = part[0].mbrp_start + part[0].mbrp_size;
		part[1].mbrp_type = MBR_PTYPE_NETBSD;
		part[1].mbrp_size = dlsize - offset;
		part[1].mbrp_start = offset;
		part[1].mbrp_flag = 0;

		ptstart = part[1].mbrp_start;
		ptsize = part[1].mbrp_size;
		bootstart = part[0].mbrp_start;
		bootsize = part[0].mbrp_size;
		return 1;
	}

	return mbr_use_wholedisk(mbri);
}
