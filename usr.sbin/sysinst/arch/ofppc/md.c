/*	$NetBSD: md.c,v 1.13 2022/01/29 16:01:19 martin Exp $	*/

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

/* md.c -- ofppc machine specific routines */

#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/disklabel_rdb.h>
#include <stdio.h>
#include <util.h>
#include <machine/cpu.h>

#include "defs.h"
#include "md.h"
#include "msg_defs.h"
#include "menu_defs.h"
#include "endian.h"

static int check_rdb(void);
static uint32_t rdbchksum(void *);

/* We use MBR_PTYPE_PREP like port-prep does. */
static int nonewfsmsdos = 0, nobootfix = 0, noprepfix=0;
static part_id bootpart_fat12 = NO_PART, bootpart_binfo = NO_PART,
    bootpart_prep = NO_PART;
static int bootinfo_mbr = 1;
static int rdb_found = 0;

/* bootstart/bootsize are for the fat */
int binfostart, binfosize, bprepstart, bprepsize;

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

	if (check_rdb())
		return true;


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
#if 0
	int i;
	int part;
	int maxpart = getmaxpartitions();
	int partstart;
	int part_raw, part_bsd;
	int ptend;
	int no_swap = 0;
#endif

	if (rdb_found) {
#if 0
	/*
	 * XXX - need to test on real machine if the disklabel code
	 * deals with RDB partitions properly, otherwise write
	 * a read-only RDB backend
	 */
		/*
		 * We found RDB partitions on the disk, which cannot be
		 * modified by rewriting the disklabel.
		 * So just use what we have got.
		 */
		for (part = 0; part < maxpart; part++) {
			if (PI_ISBSDFS(&pm->bsdlabel[part])) {
				pm->bsdlabel[part].pi_flags |=
				    PIF_NEWFS | PIF_MOUNT;

				if (part == PART_A)
					strcpy(pm->bsdlabel[part].pi_mount, "/");
			}
		}

		part_bsd = part_raw = getrawpartition();
		if (part_raw == -1)
			part_raw = PART_C;	/* for sanity... */
		pm->bsdlabel[part_raw].pi_offset = 0;
		pm->bsdlabel[part_raw].pi_size = pm->dlsize;

		set_sizemultname_meg();
rdb_edit_check:
		if (edit_and_check_label(pm->bsdlabel, maxpart, part_raw,
		    part_bsd) == 0) {
			msg_display(MSG_abort);
			return 0;
		}
		if (md_check_partitions() == 0)
			goto rdb_edit_check;
#endif
		return 1;
	}

	/*
	 * Initialize global variables that track space used on this disk.
	 * Standard 4.4BSD 8-partition labels always cover whole disk.
	 */
	if (pm->ptsize == 0)
		pm->ptsize = pm->dlsize - pm->ptstart;
	if (pm->dlsize == 0)
		pm->dlsize = pm->ptstart + pm->ptsize;

#if 0
	partstart = pm->ptstart;
	ptend = pm->ptstart + pm->ptsize;

	/* Ask for layout type -- standard or special */
	msg_fmt_display(MSG_layout, "%d%d%d",
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
	if (binfosize != 0) {
		pm->bsdlabel[PART_BOOT_BINFO].pi_fstype = FS_OTHER;
		pm->bsdlabel[PART_BOOT_BINFO].pi_size = binfosize;
		pm->bsdlabel[PART_BOOT_BINFO].pi_offset = binfostart;
	}
	if (bprepsize != 0) {
		pm->bsdlabel[PART_BOOT_PREP].pi_fstype = FS_BOOT;
		pm->bsdlabel[PART_BOOT_PREP].pi_size = bprepsize;
		pm->bsdlabel[PART_BOOT_PREP].pi_offset = bprepstart;
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
	(void) savenewlabel(pm->bsdlabel, maxpart);

	/* Everything looks OK. */
	return 1;
#endif

	return make_bsd_partitions(install);
}

/*
 * any additional partition validation
 */
bool
md_check_partitions(struct install_partition_desc *install)
{
	struct disk_partitions *parts;
	struct disk_part_info info;
	int fprep=0, ffat=0;
	part_id part;

	if (rdb_found)
		return 1;

	if (install->num < 1)
		return false;
	parts = install->infos[0].parts;	/* disklabel parts */
	if (parts->parent)
		parts = parts->parent;		/* MBR parts */

	/* we need to find a boot partition, otherwise we can't create
	 * our msdos fs boot partition.  We make the assumption that
	 * the user hasn't done something stupid, like move it away
	 * from the MBR partition.
	 */
	for (part = 0; part < parts->num_part; part++) {
		if (!parts->pscheme->get_part_info(parts, part, &info))
			continue;

		if (info.fs_type == FS_MSDOS) {
			bootpart_fat12 = part;
			ffat++;
		} else if (info.fs_type == FS_BOOT) {
			bootpart_prep = part;
			fprep++;
		} else if (info.fs_type == FS_OTHER) {
			bootpart_binfo = part;
			fprep++;
		}
	}
	/* oh, the confusion */
	if (ffat >= 1 && fprep < 2) {
		noprepfix = 1;
		return true;
	}
	if (ffat < 1 && fprep >= 2) {
		nobootfix = 1;
		return true;
	}
	if (ffat >=1 && fprep >= 2) {
		return true;
	}

	msg_display(MSG_nobootpartdisklabel);
	process_menu(MENU_ok, NULL);
	nobootfix = 1;
	return false;
}

/*
 * hook called before writing new disklabel.
 */
bool
md_pre_disklabel(struct install_partition_desc *install,
    struct disk_partitions *parts)
{

	if (rdb_found)
		return true;


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
	char bootdev[100];

	if (pm->bootstart == 0 || pm->bootsize == 0 || rdb_found)
		return 0;

	snprintf(bootdev, sizeof bootdev, "/dev/r%s%c", pm->diskdev,
	    (char)('a'+bootpart_fat12));
	run_program(RUN_DISPLAY, "/sbin/newfs_msdos %s", bootdev);

	return 0;
}

/*
 * hook called after upgrade() or install() has finished setting
 * up the target disk but immediately before the user is given the
 * ``disks are now set up'' message.
 */
int
md_post_newfs(struct install_partition_desc *install)
{

	/* No bootblock. We use ofwboot from a partition visiable by OFW. */
	return 0;
}

int
md_post_extract(struct install_partition_desc *install, bool upgrade)
{
	char bootdev[100], bootbdev[100], version[64];
	struct disk_partitions *parts;

	/* if we can't make it bootable, just punt */
	if ((nobootfix && noprepfix) || rdb_found)
		return 0;

	snprintf(version, sizeof version, "NetBSD/%s %s", MACH, REL);
	run_program(RUN_DISPLAY, "/usr/mdec/mkbootinfo '%s' %d "
	    "/tmp/bootinfo.txt", version, bootinfo_mbr);

	if (!nobootfix) {
		run_program(RUN_DISPLAY, "/bin/mkdir -p /%s/boot/ppc",
		    target_prefix());
		run_program(RUN_DISPLAY, "/bin/mkdir -p /%s/boot/netbsd",
		    target_prefix());
		run_program(RUN_DISPLAY, "/bin/cp /usr/mdec/ofwboot "
		    "/%s/boot/netbsd", target_prefix());
		run_program(RUN_DISPLAY, "/bin/cp /tmp/bootinfo.txt "
		    "/%s/boot/ppc", target_prefix());
		run_program(RUN_DISPLAY, "/bin/cp /usr/mdec/ofwboot "
		    "/%s/boot/ofwboot", target_prefix());
	}

	if (!noprepfix && install != NULL && install->num > 0) {
		parts = install->infos[0].parts;	/* disklabel */
		if (parts->parent != NULL)
			parts = parts->parent;		/* MBR */

		parts->pscheme->get_part_device(parts, bootpart_prep,
		    bootdev, sizeof bootdev, NULL, raw_dev_name, true, true);
		parts->pscheme->get_part_device(parts, bootpart_prep,
		    bootbdev, sizeof bootbdev, NULL, plain_name, true, true);
		run_program(RUN_DISPLAY, "/bin/dd if=/dev/zero of=%s bs=512",
		    bootdev);
		run_program(RUN_DISPLAY, "/bin/dd if=/usr/mdec/ofwboot "
		    "of=%s bs=512", bootbdev);

		parts->pscheme->get_part_device(parts, bootpart_binfo,
		    bootdev, sizeof bootdev, NULL, raw_dev_name, true, true);
		parts->pscheme->get_part_device(parts, bootpart_binfo,
		    bootbdev, sizeof bootbdev, NULL, plain_name, true, true);
		run_program(RUN_DISPLAY, "/bin/dd if=/dev/zero of=%s bs=512",
		    bootdev);
		run_program(RUN_DISPLAY, "/bin/dd if=/tmp/bootinfo.txt "
		    "of=%s bs=512", bootbdev);
	}

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
#if 0
	struct mbr_partition *part;
	mbr_info_t *ext;
	int i;
#endif

	if (check_rdb())
		return 1;

#if 0
	read_mbr(pm->diskdev, &mbr);
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
				msg_fmt_display_add(MSG_nobootpartdisklabel,
				    "%d", 0);
				if (!ask_yesno(NULL))
					return 0;
				nobootfix = 1;
			}
		}
	}
#endif

	if (!md_check_partitions(install))
		return 0;

	return 1;
}

/* Upgrade support */
int
md_update(struct install_partition_desc *install)
{

	nonewfsmsdos = 1;
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
			if (part->mbrp_type == MBR_PTYPE_FAT12) {
				pm->bootstart = part->mbrp_start;
				pm->bootsize = part->mbrp_size;
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
	if (!(pm->bootsize >= (MIN_FAT12_BOOT/512) ||
		(binfosize >= (MIN_BINFO_BOOT/512) &&
		    bprepsize >= (MIN_PREP_BOOT/512)))) {
		if (quiet)
			return 0;
		msg_display(MSG_bootnotright);
		return ask_reedit(parts);
	}

	/* check the prep partitions */
	if ((binfosize > 0 || bprepsize > 0) &&
	    (binfosize < (MIN_BINFO_BOOT/512) ||
		bprepsize < (MIN_PREP_BOOT/512))) {
		if (quiet)
			return 0;
		msg_display(MSG_preptoosmall);
		return ask_reedit(parts);
	}

	/* check the fat12 parititons */
	if (pm->bootsize > 0 && pm->bootsize < (MIN_FAT12_BOOT/512)) {
		if (quiet)
			return 0;
		msg_display(MSG_boottoosmall);
		return ask_reedit(parts);
	}

	/* if both sets contain zero, thats bad */
	if ((pm->bootstart == 0 || pm->bootsize == 0) &&
	    (binfosize == 0 || binfostart == 0 ||
		bprepsize == 0 || bprepstart == 0)) {
		if (quiet)
			return 0;
		msg_display(MSG_nobootpart);
		return ask_reedit(parts);
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

bool
md_parts_use_wholedisk(struct disk_partitions *parts)
{
	struct disk_part_info boot_parts[] = 
	{
		{ .fs_type = FS_MSDOS, .size = FAT12_BOOT_SIZE/512 },
		{ .fs_type = FS_OTHER, .size = BINFO_BOOT_SIZE/512 },
		{ .fs_type = FS_BOOT, .size = PREP_BOOT_SIZE/512 }
	};

	return parts_use_wholedisk(parts, __arraycount(boot_parts),
	     boot_parts);
}

const char *md_disklabel_cmd(void)
{

	/* we cannot rewrite an RDB disklabel */
	if (rdb_found)
		return "sync No disklabel";

	return "disklabel -w -r";
}

static int
check_rdb(void)
{
	char buf[512], diskpath[MAXPATHLEN];
	struct rdblock *rdb;
	off_t blk;
	int fd;

	/* Find out if this disk has a valid RDB, before continuing. */
	rdb = (struct rdblock *)buf;
	fd = opendisk(pm->diskdev, O_RDONLY, diskpath, sizeof(diskpath), 0);
	if (fd < 0)
		return 0;
	for (blk = 0; blk < RDB_MAXBLOCKS; blk++) {
		if (pread(fd, rdb, 512, blk * 512) != 512)
			return 0;
		if (rdb->id == RDBLOCK_ID && rdbchksum(rdb) == 0) {
			rdb_found = 1;	/* do not repartition! */
			return 1;
		}
	}
	return 0;
}

static uint32_t
rdbchksum(void *bdata)
{
	uint32_t *blp, cnt, val;

	blp = bdata;
	cnt = blp[1];
	val = 0;
	while (cnt--)
		val += *blp++;
	return val;
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

