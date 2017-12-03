/*	$NetBSD: mbr.c,v 1.4.6.1 2017/12/03 11:36:34 jdolecek Exp $	*/

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
 * All rights reserved.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/bootblock.h>

#include <lib/libsa/byteorder.h>
#include <lib/libsa/stand.h>

#include "mbr.h"

static int find_mbr_part(struct of_dev *, uint32_t, char *,
    struct disklabel *, uint32_t, uint8_t, int);
static void make_dos_label(struct disklabel *, uint32_t);

/*
 * Find a valid MBR disklabel.
 */
int
search_mbr_label(struct of_dev *devp, u_long off, char *buf,
    struct disklabel *lp, u_long off0)
{
	static uint8_t fat_types[] = {
		MBR_PTYPE_FAT12, MBR_PTYPE_FAT16S, MBR_PTYPE_FAT16B,
		MBR_PTYPE_FAT32, MBR_PTYPE_FAT32L, MBR_PTYPE_FAT16L
	};
	size_t read;
	uint32_t poff;
	int i;

	/* Find a disklabel in a NetBSD or 386BSD partition. */
	poff = find_mbr_part(devp, off, buf, lp, 0, MBR_PTYPE_NETBSD, 0);
#ifdef COMPAT_386BSD_MBRPART
	if (poff == 0) {
		poff = find_mbr_part(devp, off, buf, lp, 0,
		    MBR_PTYPE_386BSD, 0);
		if (poff != 0)
			printf("WARNING: old BSD partition ID!\n");
	}
#endif
	if (poff != 0) {
		if (strategy(devp, F_READ, poff + LABELSECTOR, DEV_BSIZE,
		    buf, &read) == 0 && read == DEV_BSIZE)
			if (getdisklabel(buf, lp) == NULL)
				return 0;
	}

	/*
	 * No BSD partition with a valid disklabel found, so try to
	 * construct a label from a DOS partition.
	 */
	for (i = 0; i < sizeof(fat_types); i++) {
		poff = find_mbr_part(devp, off, buf, lp, 0, fat_types[i], 0);
		if (poff != 0) {
			make_dos_label(lp, poff);
			return 0;
		}
	}

	return ERDLAB;
}

static int
find_mbr_part(struct of_dev *devp, uint32_t off, char *buf,
    struct disklabel *lp, uint32_t off0, uint8_t ptype, int recursion)
{
	size_t read;
	struct mbr_partition *p;
	int i;
	uint32_t poff;

	if (strategy(devp, F_READ, off, DEV_BSIZE, buf, &read)
	    || read != DEV_BSIZE)
		return 0;

	if (*(uint16_t *)&buf[MBR_MAGIC_OFFSET] != sa_htole16(MBR_MAGIC))
		return 0;

	if (recursion++ <= 1)
		off0 += off;

	for (p = (struct mbr_partition *)(buf + MBR_PART_OFFSET), i = 0;
	     i < MBR_PART_COUNT; i++, p++) {
		if (p->mbrp_type == ptype) {
			recursion--;
			return sa_le32toh(p->mbrp_start) + off0;
		}
		else if (p->mbrp_type == MBR_PTYPE_EXT) {
			poff = find_mbr_part(devp, sa_le32toh(p->mbrp_start),
			    buf, lp, off0, ptype, recursion);
			if (poff != 0) {
				recursion--;
				return poff;
			}
			if (strategy(devp, F_READ, off, DEV_BSIZE, buf, &read)
			    || read != DEV_BSIZE) {
				recursion--;
				return 0;
			}
		}
	}

	return 0;
}

static void
make_dos_label(struct disklabel *lp, uint32_t poff)
{
	int i;

	/* clear all partitions */
	lp->d_npartitions = RAW_PART + 1;
	for (i = 0; i < MAXPARTITIONS; i++) {
		lp->d_partitions[i].p_size = 0;
		lp->d_partitions[i].p_offset = 0;
		lp->d_partitions[i].p_fstype = 0;
	}

	/* set DOS partition as root partition */
	lp->d_partitions[0].p_offset = poff;
	lp->d_partitions[0].p_fstype = FS_MSDOS;

	/* disklabel is valid */
	lp->d_magic = lp->d_magic2 = DISKMAGIC;
	lp->d_checksum = 0;
	lp->d_checksum = dkcksum(lp);
}
