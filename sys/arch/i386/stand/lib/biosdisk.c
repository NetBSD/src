/*	$NetBSD: biosdisk.c,v 1.24.2.2 2008/01/21 09:37:15 yamt Exp $	*/

/*
 * Copyright (c) 1996, 1998
 *	Matthias Drochner.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*
 * raw BIOS disk device for libsa.
 * needs lowlevel parts from bios_disk.S and biosdisk_ll.c
 * partly from netbsd:sys/arch/i386/boot/disk.c
 * no bad144 handling!
 *
 * A lot of this must match sys/kern/subr_disk_mbr.c
 */

/*
 * Ported to boot 386BSD by Julian Elischer (julian@tfs.com) Sept 1992
 *
 * Mach Operating System
 * Copyright (c) 1992, 1991 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */

#include <sys/types.h>
#include <sys/disklabel.h>
#include <sys/md5.h>
#include <sys/param.h>

#include <fs/cd9660/iso.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/saerrno.h>
#include <machine/stdarg.h>

#include "libi386.h"
#include "biosdisk_ll.h"
#include "biosdisk.h"
#ifdef _STANDALONE
#include "bootinfo.h"
#endif

#define BUFSIZE	2048	/* must be large enough for a CD sector */

struct biosdisk {
	struct biosdisk_ll ll;
	int             boff;
	char            buf[BUFSIZE];
};

#ifdef _STANDALONE
static struct btinfo_bootdisk bi_disk;
static struct btinfo_bootwedge bi_wedge;
#endif

#define	RF_PROTECTED_SECTORS	64	/* XXX refer to <.../rf_optnames.h> */

int
biosdisk_strategy(void *devdata, int flag, daddr_t dblk, size_t size,
		  void *buf, size_t *rsize)
{
	struct biosdisk *d;
	int blks, frag;

	if (flag != F_READ)
		return EROFS;

	d = (struct biosdisk *) devdata;

	if (d->ll.type == BIOSDISK_TYPE_CD)
		dblk = dblk * DEV_BSIZE / ISO_DEFAULT_BLOCK_SIZE;

	dblk += d->boff;

	blks = size / d->ll.secsize;
	if (blks && readsects(&d->ll, dblk, blks, buf, 0)) {
		if (rsize)
			*rsize = 0;
		return EIO;
	}

	/* needed for CD */
	frag = size % d->ll.secsize;
	if (frag) {
		if (readsects(&d->ll, dblk + blks, 1, d->buf, 0)) {
			if (rsize)
				*rsize = blks * d->ll.secsize;
			return EIO;
		}
		memcpy(buf + blks * d->ll.secsize, d->buf, frag);
	}

	if (rsize)
		*rsize = size;
	return 0;
}

static struct biosdisk *
alloc_biosdisk(int biosdev)
{
	struct biosdisk *d;

	d = alloc(sizeof(*d));
	if (d == NULL)
		return NULL;
	memset(d, 0, sizeof(*d));

	d->ll.dev = biosdev;
	if (set_geometry(&d->ll, NULL)) {
#ifdef DISK_DEBUG
		printf("no geometry information\n");
#endif
		dealloc(d, sizeof(*d));
		return NULL;
	}
	return d;
}

#ifndef NO_DISKLABEL
static int
check_label(struct biosdisk *d, int sector)
{
	struct disklabel *lp;

	/* find partition in NetBSD disklabel */
	if (readsects(&d->ll, sector + LABELSECTOR, 1, d->buf, 0)) {
#ifdef DISK_DEBUG
		printf("Error reading disklabel\n");
#endif
		return EIO;
	}
	lp = (struct disklabel *) (d->buf + LABELOFFSET);
	if (lp->d_magic != DISKMAGIC || dkcksum(lp)) {
#ifdef DISK_DEBUG
		printf("warning: no disklabel in sector %u\n", sector);
#endif
		return -1;
	}

	d->boff = sector;
	return 0;
}

static int
read_label(struct biosdisk *d)
{
	struct disklabel dflt_lbl;
	struct mbr_partition mbr[MBR_PART_COUNT];
	struct partition *p;
	int sector, i;
	int error;
	int typ;
	int ext_base, this_ext, next_ext;
#ifdef COMPAT_386BSD_MBRPART
	int sector_386bsd = -1;
#endif

	memset(&dflt_lbl, 0, sizeof(dflt_lbl));
	dflt_lbl.d_npartitions = 8;

	d->boff = 0;

	if (d->ll.type != BIOSDISK_TYPE_HD)
		/* No label on floppy and CD */
		return -1;

	/*
	 * find NetBSD Partition in DOS partition table
	 * XXX check magic???
	 */
	ext_base = 0;
	next_ext = 0;
	for (;;) {
		this_ext = ext_base + next_ext;
		next_ext = 0;
		if (readsects(&d->ll, this_ext, 1, d->buf, 0)) {
#ifdef DISK_DEBUG
			printf("error reading MBR sector %d\n", this_ext);
#endif
			return EIO;
		}
		memcpy(&mbr, ((struct mbr_sector *)d->buf)->mbr_parts,
		       sizeof(mbr));
		/* Look for NetBSD partition ID */
		for (i = 0; i < MBR_PART_COUNT; i++) {
			typ = mbr[i].mbrp_type;
			if (typ == 0)
				continue;
			sector = this_ext + mbr[i].mbrp_start;
#ifdef DISK_DEBUG
			printf("ptn type %d in sector %u\n", typ, sector);
#endif
			if (typ == MBR_PTYPE_NETBSD) {
				error = check_label(d, sector);
				if (error >= 0)
					return error;
			}
			if (MBR_IS_EXTENDED(typ)) {
				next_ext = mbr[i].mbrp_start;
				continue;
			}
#ifdef COMPAT_386BSD_MBRPART
			if (this_ext == 0 && typ == MBR_PTYPE_386BSD)
				sector_386bsd = sector;
#endif
			if (this_ext != 0) {
				if (dflt_lbl.d_npartitions >= MAXPARTITIONS)
					continue;
				p = &dflt_lbl.d_partitions[dflt_lbl.d_npartitions++];
			} else
				p = &dflt_lbl.d_partitions[i];
			p->p_offset = sector;
			p->p_size = mbr[i].mbrp_size;
			p->p_fstype = xlat_mbr_fstype(typ);
		}
		if (next_ext == 0)
			break;
		if (ext_base == 0) {
			ext_base = next_ext;
			next_ext = 0;
		}
	}

	sector = 0;
#ifdef COMPAT_386BSD_MBRPART
	if (sector_386bsd != -1) {
		printf("old BSD partition ID!\n");
		sector = sector_386bsd;
	}
#endif

	/*
	 * One of two things:
	 * 	1. no MBR
	 *	2. no NetBSD partition in MBR
	 *
	 * We simply default to "start of disk" in this case and
	 * press on.
	 */
	error = check_label(d, sector);
	if (error >= 0)
		return error;

	/*
	 * Nothing at start of disk, return info from mbr partitions.
	 */
	/* XXX fill it to make checksum match kernel one */
	dflt_lbl.d_checksum = dkcksum(&dflt_lbl);
	memcpy(d->buf, &dflt_lbl, sizeof(dflt_lbl));
	return 0;
}
#endif /* NO_DISKLABEL */

/* Determine likely partition for possible sector number of dos
 * partition.
 */

int
biosdisk_findpartition(int biosdev, u_int sector)
{
#ifdef NO_DISKLABEL
	return 0;
#else
	struct biosdisk *d;
	int partition = 0;
	struct disklabel *lp;
#ifdef DISK_DEBUG
	printf("looking for partition device %x, sector %u\n", biosdev, sector);
#endif

	/* Look for netbsd partition that is the dos boot one */
	d = alloc_biosdisk(biosdev);
	if (d == NULL)
		return 0;

	if (read_label(d) == 0) {
		lp = (struct disklabel *)(d->buf + LABELOFFSET);
		for (partition = lp->d_npartitions; --partition;){
			if (lp->d_partitions[partition].p_fstype == FS_UNUSED)
				continue;
			if (lp->d_partitions[partition].p_offset == sector)
				break;
		}
	}

	dealloc(d, sizeof(*d));
	return partition;
#endif /* NO_DISKLABEL */
}

int
biosdisk_open(struct open_file *f, ...)
/* struct open_file *f, int biosdev, int partition */
{
	va_list ap;
	struct biosdisk *d;
	int biosdev;
	int partition;
#ifndef NO_DISKLABEL
	struct disklabel *lp;
#endif
	int error = 0;

	va_start(ap, f);
	biosdev = va_arg(ap, int);
	d = alloc_biosdisk(biosdev);
	if (d == NULL) {
		error = ENXIO;
		goto out;
	}

	partition = va_arg(ap, int);
#ifdef _STANDALONE
	bi_disk.biosdev = d->ll.dev;
	bi_disk.partition = partition;
	bi_disk.labelsector = -1;

	bi_wedge.biosdev = d->ll.dev;
	bi_wedge.matchblk = -1;
#endif

#ifndef NO_DISKLABEL
	if (partition == RAW_PART)
		goto nolabel;
	error = read_label(d);
	if (error == -1) {
		error = 0;
		goto nolabel;
	}
	if (error)
		goto out;

	lp = (struct disklabel *) (d->buf + LABELOFFSET);
	if (partition >= lp->d_npartitions ||
	    lp->d_partitions[partition].p_fstype == FS_UNUSED) {
#ifdef DISK_DEBUG
		printf("illegal partition\n");
#endif
		error = EPART;
		goto out;
	}
#ifdef _STANDALONE
	bi_disk.labelsector = d->boff + LABELSECTOR;
	bi_disk.label.type = lp->d_type;
	memcpy(bi_disk.label.packname, lp->d_packname, 16);
	bi_disk.label.checksum = lp->d_checksum;

	bi_wedge.startblk = lp->d_partitions[partition].p_offset;
	bi_wedge.nblks = lp->d_partitions[partition].p_size;
	bi_wedge.matchblk = d->boff + LABELSECTOR;
	bi_wedge.matchnblks = 1;
	{
		MD5_CTX ctx;

		MD5Init(&ctx);
		MD5Update(&ctx, (void *) d->buf, 512);
		MD5Final(bi_wedge.matchhash, &ctx);
	}
#endif
	d->boff = lp->d_partitions[partition].p_offset;
	if (lp->d_partitions[partition].p_fstype == FS_RAID)
		d->boff += RF_PROTECTED_SECTORS;
nolabel:
#endif /* NO_DISKLABEL */

#ifdef DISK_DEBUG
	printf("partition @%d\n", d->boff);
#endif

#ifdef _STANDALONE
	BI_ADD(&bi_disk, BTINFO_BOOTDISK, sizeof(bi_disk));
	BI_ADD(&bi_wedge, BTINFO_BOOTWEDGE, sizeof(bi_wedge));
#endif

	f->f_devdata = d;
out:
        va_end(ap);
	if (error)
		dealloc(d, sizeof(struct biosdisk));
	return error;
}

#ifndef LIBSA_NO_FS_CLOSE
int
biosdisk_close(struct open_file *f)
{
	struct biosdisk *d = f->f_devdata;

	/* let the floppy drive go off */
	if (d->ll.type == BIOSDISK_TYPE_FD)
		delay(3000000);	/* 2s is enough on all PCs I found */

	dealloc(d, sizeof(struct biosdisk));
	f->f_devdata = NULL;
	return 0;
}
#endif

int
biosdisk_ioctl(struct open_file *f, u_long cmd, void *arg)
{
	return EIO;
}
