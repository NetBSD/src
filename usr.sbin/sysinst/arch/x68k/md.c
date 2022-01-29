/*	$NetBSD: md.c,v 1.12 2022/01/29 16:01:21 martin Exp $ */

/*
 * Copyright 1997 Piermont Information Systems Inc.
 * All rights reserved.
 *
 * Based on code written by Philip A. Nelson for Piermont Information
 * Systems Inc.  Modified by Minoura Makoto for x68k.
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

/* md.c -- x68k machine specific routines */
/* This file is in close sync with pmax, sparc, and vax md.c */

#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <util.h>

#include "defs.h"
#include "md.h"
#include "msg_defs.h"
#include "menu_defs.h"

#ifdef notyet
#undef NDOSPART 8
#define NDOSPART 16
typedef struct parttab {
	struct dos_partition dosparts[NDOSPART];
} parttab;

parttab md_disklabel;
int md_freepart;
int md_nfreepart;
#endif /* notyet */

int md_need_newdisk = 0;

/* prototypes */
static int md_newdisk(void);

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
	char buf[1024];
	int fd;
	char dev_name[100];
	struct disklabel disklabel;

	snprintf(dev_name, 100, "/dev/r%sc", pm->diskdev);

	fd = open(dev_name, O_RDONLY, 0);
	if (fd < 0) {
		if (logfp)
			(void)fprintf(logfp, "Can't open %s\n", dev_name);
		endwin();
		fprintf(stderr, "Can't open %s\n", dev_name);
		exit(1);
	}
	if (ioctl(fd, DIOCGDINFO, &disklabel) == -1) {
		if (logfp)
			(void)fprintf(logfp, "Can't read disklabel on %s.\n",
				dev_name);
		endwin();
		fprintf(stderr, "Can't read disklabel on %s.\n", dev_name);
		close(fd);
		exit(1);
	}
	if (disklabel.d_secsize != 512) {
		endwin();
		fprintf(stderr, "Non-512byte/sector disk is not supported.\n");
		close(fd);
		exit(1);
	}

	pm->dlcyl = disklabel.d_ncylinders;
	pm->dlhead = disklabel.d_ntracks;
	pm->dlsec = disklabel.d_nsectors;
	pm->sectorsize = disklabel.d_secsize;
	pm->dlcylsize = disklabel.d_secpercyl;
	pm->dlsize = pm->dlcyl*pm->dlhead*pm->dlsec;

	if (read(fd, buf, 1024) < 0) {
		endwin();
		fprintf(stderr, "Can't read %s\n", dev_name);
		close(fd);
		exit(1);
	}
	if (memcmp(buf, "X68SCSI1", 8) != 0)
		md_need_newdisk = 1;
#ifdef notyet
	else
		if (read(fd, md_disklabel, sizeof(md_disklabel)) < 0) {
			endwin();
			fprintf(stderr, "Can't read %s\n", dev_name);
			close(fd);
			exit(1);
		}
#endif
	/* preserve first 64 sectors for system. */
	pm->ptstart = 64;

	/* preserve existing partitions? */

	close(fd);

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
	/* X68k partitions must be in order of the range. */
	daddr_t last_end = 0;
	size_t i;
	char desc[STRSIZE];

	for (i = 0; i < install->num; i++) {
		if (i > 0) {
			/* skip raw part and similar */
			if (install->infos[i].cur_flags &
			    (PTI_SEC_CONTAINER|PTI_PSCHEME_INTERNAL|
			    PTI_RAW_PART))
				continue;

			if (install->infos[i].cur_start < last_end) {
				snprintf(desc, sizeof desc,
				    "%zu (%s)", i,
				    install->infos[i].mount);
				msg_fmt_display(MSG_ordering, "%s", desc);
				if (ask_yesno(NULL))
					return false;
			}
		}
		last_end = install->infos[i].cur_start + install->infos[i].size;
	}

	return true;
}

#ifdef notyet
static int
md_check_partitions(void)
{
	int i, j;
	int preserve;

	/* check existing BSD partitions. */
	for (i = 0; i < NDOSPART; i++) {
		if (md_disklabel.dosparts[i].dp_size == 0)
			break;
		if (memcmp(md_disklabel.dosparts[i].dp_typename, "Human68k", 8)) {
			msg_display(MSG_existing);
			preserve = ask_noyes(NULL);
			break;
		}
	}
	emptylabel(pm->bsdlabel);
	pm->bsdlabel[C].pi_fstype = FS_UNUSED;
	pm->bsdlabel[C].pi_offset = 0;
	pm->bsdlabel[C].pi_size = pm->dlsize;
	for (i = 0, j = A; i < NDOSPART;) {
		if (j == C) {
			j++;
			continue;
		}
		if (!preserve &&
		    memcmp(md_disklabel.dosparts[i].dp_typename,
			    "Human68k", 8)) {
			/* discard it. */
			i++;
			continue;
		}
		pm->bsdlabel[j].pi_fstype = (i == 1) ? FS_SWAP : FS_BSDFFS;
		pm->bsdlabel[j].pi_offset = md_disklabel.dosparts[i].dp_start*2;
		pm->bsdlabel[j].pi_size = md_disklabel.dosparts[i].dp_size*2;
		i++;
		j++;
	}
	if (j > 6) {
		msg_fmt_display(MSG_nofreepart, "%s" pm->diskdev);
		return 0;
	}
	md_nfreepart = 8 - j;

	/* check for free space */
	fspm->ptsize = pm->bsdlabel[A].pi_offset - 64;
	if (fpm->ptsize <= 0) {	/* XXX: should not be 0; minfsdb?  */
		msg_fmt_display(MSG_notfirst, "%s", pm->diskdev);
		process_menu(MENU_ok);
		exit(1);
	}

	/* Partitions should be preserved in md_make_bsdpartitions() */
}
#endif /* notyet */

/*
 * hook called before writing new disklabel.
 */
bool
md_pre_disklabel(struct install_partition_desc *install,
    struct disk_partitions *parts)
{
	if (md_need_newdisk)
		md_newdisk ();
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
 * On the x68k, we use this opportunity to install the boot blocks.
 */
int
md_post_newfs(struct install_partition_desc *install)
{
	/* boot blocks ... */
	msg_fmt_display(MSG_dobootblks, "%s", pm->diskdev);
	cp_to_target("/usr/mdec/boot", "/boot");
	if (run_program(RUN_DISPLAY | RUN_NO_CLEAR,
	    "/usr/mdec/installboot.new /usr/mdec/sdboot_ufs /dev/r%sa",
	    pm->diskdev))
		process_menu(MENU_ok,
			__UNCONST("Warning: disk is probably not bootable"));

	wclear(stdscr);
	touchwin(stdscr);
	clearok(stdscr, 1);
	refresh();

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
md_newdisk(void)
{
	msg_fmt_display(MSG_newdisk, "%s%s", pm->diskdev, pm->diskdev);

	return run_program(RUN_FATAL|RUN_DISPLAY,
	    "/usr/mdec/newdisk -v %s", pm->diskdev);
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

