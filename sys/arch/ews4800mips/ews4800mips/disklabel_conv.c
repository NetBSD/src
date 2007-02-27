/*	$NetBSD: disklabel_conv.c,v 1.1.28.1 2007/02/27 16:50:19 yamt Exp $	*/

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: disklabel_conv.c,v 1.1.28.1 2007/02/27 16:50:19 yamt Exp $");

#include <sys/systm.h>
#include <sys/param.h>

#ifndef _KERNEL
#include "local.h"
#endif

#include <sys/disklabel.h>

/*
 * NetBSD/ews4800mips EWS-UX compatible disk layout.
 *
 * 0============================+ cylinder 0
 * 1	1stboot	program (4KB)	|
 * .				.
 * 7----------------------------+
 * 8	PDINFO			| UX unvisible region.
 * 9    BSD disklabel           | I (uch) decided to locate disklabel here.
 * .				.
 * .	error log	(UX)	|
 * +============================+ cylinder 1
 * |	logical_start	(UX)	|
 * |	VTOC			|
 * .				. UX Boot block	(partition 7)
 * .				. 100 blocks (default. at least 2 blocks)
 * .				.
 * .				.
 * +============================+
 * |   BFS			|
 * |		boot loader	|
 * .		disklabel	.
 * .		and etc.	. BFS		(partition 3)
 * .				.
 * .				.
 * .				.
 * -----------------------------+
 */

#define	_BFS_SIZE			16384	/* 8MB */
#define	_BOOTBLOCK_SIZE			100	/* UX default */
#define	_FAKE_TRACKS_PER_CYLINDER	16
#define	_FAKE_SECTORS_PER_TRACK		1

void
disklabel_set_default(struct disklabel *d)
{

	d->d_magic = DISKMAGIC;
	d->d_magic2 = DISKMAGIC;
	d->d_secsize = DEV_BSIZE;
	d->d_npartitions = MAXPARTITIONS;
}

void
vtoc_set_default( struct cpu_disklabel *ux, struct disklabel *d)
{
	struct pdinfo_sector *pdinfo = &ux->pdinfo;
	struct vtoc_sector *vtoc = &ux->vtoc;
	struct ux_partition *bfs;
	struct ux_partition *boot;
	struct ux_partition *bsdraw;
	int nsectors, logical_sector, cylinder_blocks, res;

	memset(vtoc, 0, sizeof *vtoc);
	memset(pdinfo, 0, sizeof *pdinfo);
	if (d)
		cylinder_blocks = d->d_ntracks * d->d_nsectors;
	else
		cylinder_blocks =
		    _FAKE_TRACKS_PER_CYLINDER * _FAKE_SECTORS_PER_TRACK;
	logical_sector = cylinder_blocks;

	pdinfo->drive_id = 0x5c512000;	/* Fake for EWS-UX */
	pdinfo->magic = PDINFO_MAGIC;
	pdinfo->version = PDINFO_VERSION;
	pdinfo->logical_sector = logical_sector;
	pdinfo->ux.errorlog_sector = logical_sector - 1;
	pdinfo->ux.errorlog_size_byte = DEV_BSIZE;

	if (d) { /* use drivers disk geometory */
		pdinfo->geometory.cylinders_per_drive = d->d_ncylinders;
		pdinfo->geometory.tracks_per_cylinder = d->d_ntracks;
		pdinfo->geometory.sectors_per_track = d->d_nsectors;
		pdinfo->geometory.bytes_per_sector = d->d_secsize;
		nsectors = d->d_ncylinders * d->d_ntracks * d->d_nsectors;
	} else { /* set fake */
		pdinfo->geometory.sectors_per_track =
		    _FAKE_SECTORS_PER_TRACK;
		pdinfo->geometory.tracks_per_cylinder =
		    _FAKE_TRACKS_PER_CYLINDER;
		pdinfo->geometory.cylinders_per_drive = 0x1fffffff;
		pdinfo->geometory.bytes_per_sector = DEV_BSIZE;
		nsectors = 0x1fffffff;
	}

	/* following magic numbers are required for EWS-UX */
	pdinfo->device_depend[15] = 0xfb7e10;
	pdinfo->device_depend[16] = 0x200;
	pdinfo->device_depend[17] = 0x10;

	vtoc->magic = VTOC_MAGIC;
	vtoc->version = VTOC_VERSION;
	vtoc->sector_size_byte = DEV_BSIZE;
	vtoc->npartitions = VTOC_MAXPARTITIONS;

	boot = &vtoc->partition[7];
	boot->tag = VTOC_TAG_BOOT;
	boot->flags = VTOC_FLAG_UNMOUNT;
	boot->start_sector = 0;
	boot->nsectors = _BOOTBLOCK_SIZE;

	bfs = &vtoc->partition[3];
	bfs->tag = VTOC_TAG_STAND;
	bfs->flags = 0;
	bfs->start_sector = _BOOTBLOCK_SIZE;

	res = nsectors - bfs->start_sector;
	bfs->nsectors = res > _BFS_SIZE ? _BFS_SIZE : res;

	bsdraw = &vtoc->partition[RAW_PART];
	bsdraw->tag = VTOC_TAG_NONAME;
	bsdraw->flags = VTOC_FLAG_UNMOUNT;
	bsdraw->start_sector = -pdinfo->logical_sector;
	bsdraw->nsectors = nsectors;
}

void
disklabel_to_vtoc(struct cpu_disklabel *ux, struct disklabel *d)
{
	struct pdinfo_sector *pdinfo = &ux->pdinfo;
	struct vtoc_sector *vtoc = &ux->vtoc;
	struct ux_partition *up;
	struct partition *p;
	uint32_t offset = pdinfo->logical_sector;
	int i;

	pdinfo->geometory.cylinders_per_drive = d->d_ncylinders;
	pdinfo->geometory.tracks_per_cylinder = d->d_ntracks;
	pdinfo->geometory.sectors_per_track = d->d_nsectors;
	pdinfo->geometory.bytes_per_sector = d->d_secsize;

	vtoc->npartitions = d->d_npartitions;
	vtoc->sector_size_byte = d->d_secsize;

	up = vtoc->partition;
	p = d->d_partitions;
	for (i = 0; i < vtoc->npartitions; i++, up++, p++) {
		if ((up->nsectors = p->p_size) != 0)
			up->start_sector = p->p_offset - offset;
		else
			up->start_sector = 0;

		switch (p->p_fstype) {
		case FS_BOOT:
			up->tag = VTOC_TAG_BOOT;
			up->flags = VTOC_FLAG_UNMOUNT;
			break;
		case FS_SYSVBFS:
			up->tag = VTOC_TAG_STAND;
			break;
		case FS_SWAP:
			up->tag = VTOC_TAG_SWAP;
			up->flags = VTOC_FLAG_UNMOUNT;
			break;
		case FS_BSDFFS:
			up->tag = __VTOC_TAG_BSDFFS;
			break;
		case FS_UNUSED:
			if (i != RAW_PART && p->p_size > 0) {
				up->tag = VTOC_TAG_RAWDISK;
				up->flags = VTOC_FLAG_UNMOUNT;
			}
			break;
		default:
			break;
		}
	}
}

void
vtoc_to_disklabel(struct cpu_disklabel *ux, struct disklabel *d)
{
	struct pdinfo_sector *pdinfo = &ux->pdinfo;
	struct vtoc_sector *vtoc = &ux->vtoc;
	struct ux_partition *up;
	struct partition *p;
	uint32_t offset = pdinfo->logical_sector;
	int i;

	d->d_secsize = pdinfo->geometory.bytes_per_sector;
	d->d_nsectors = pdinfo->geometory.sectors_per_track;
	d->d_ntracks = pdinfo->geometory.tracks_per_cylinder;
	d->d_ncylinders = pdinfo->geometory.cylinders_per_drive;
	d->d_secpercyl = d->d_nsectors * d->d_ntracks;
	d->d_secperunit = d->d_ncylinders * d->d_secpercyl;

	d->d_npartitions = vtoc->npartitions;
	d->d_secsize = vtoc->sector_size_byte;

	up = vtoc->partition;
	p = d->d_partitions;
	for (i = 0; i < vtoc->npartitions; i++, up++, p++) {

		if ((p->p_size = up->nsectors) != 0)
			p->p_offset = up->start_sector + offset;
		else
			p->p_offset = 0;

		switch (up->tag) {
		case VTOC_TAG_BOOT:
			p->p_fstype = FS_BOOT;
			break;
		case VTOC_TAG_STAND:
			p->p_fstype = FS_SYSVBFS;
			break;
		case VTOC_TAG_RAWDISK:
			p->p_fstype = FS_UNUSED;
			break;
		case VTOC_TAG_SWAP:
			p->p_fstype = FS_SWAP;
			break;
		case VTOC_TAG_NONAME:
		case VTOC_TAG_ROOT:
		case VTOC_TAG_USR:
		case VTOC_TAG_VAR:
		case VTOC_TAG_HOME:
			p->p_fstype = FS_SYSV;
			break;
		default:
			if (up->nsectors != 0)
				p->p_fstype = FS_SYSV;
			else
				p->p_fstype = FS_UNUSED;
			break;
		}
	}

	d->d_checksum = 0;
	d->d_checksum = dkcksum(d);
}

bool
disklabel_sanity(struct disklabel *d)
{

	if (d->d_magic != DISKMAGIC || d->d_magic2 != DISKMAGIC ||
	    dkcksum(d) != 0)
		return false;

	return true;
}
