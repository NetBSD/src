/*	$NetBSD: mbrlabel.c,v 1.9 2000/12/24 01:50:29 wiz Exp $	*/

/*
 * Copyright (C) 1998 Wolfgang Solfrank.
 * Copyright (C) 1998 TooLs GmbH.
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

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: mbrlabel.c,v 1.9 2000/12/24 01:50:29 wiz Exp $");
#endif /* not lint */

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include <sys/param.h>
#include <sys/disklabel.h>
#include <sys/disklabel_mbr.h>
#include <sys/ioctl.h>

#include "dkcksum.h"

#define	FIRSTPART	0

int main(int, char **);
void usage(void);
void getlabel(int);
void setlabel(int);
int getparts(int, int, u_int32_t, u_int32_t);
int nbsdtype(int);
u_int32_t getlong(void *p);

struct disklabel label;

void
getlabel(int sd)
{
	struct partition save;

	if (ioctl(sd, DIOCGDINFO, &label) < 0) {
		perror("get label");
		exit(1);
	}
	save = label.d_partitions[RAW_PART];
	memset(label.d_partitions, 0, sizeof label.d_partitions);
	label.d_partitions[RAW_PART] = save;
	/*
	 * Some ports seem to not set the number of partitions
	 * correctly, albeit they seem to set the raw partiton ok!
	 */
	if (label.d_npartitions <= RAW_PART)
		label.d_npartitions = RAW_PART + 1;
}

void
setlabel(int sd)
{
	label.d_checksum = 0;
	label.d_checksum = dkcksum(&label);
	if (ioctl(sd, DIOCSDINFO, &label) < 0) {
		perror("set label");
		exit(1);
	}
}

static struct typetab {
	int mbrtype;
	int nbsdtype;
} typetable[] = {
	{ MBR_PTYPE_NETBSD, FS_BSDFFS },
	{ MBR_PTYPE_386BSD, FS_BSDFFS },
	{ MBR_PTYPE_FAT12, FS_MSDOS },
	{ MBR_PTYPE_FAT16S, FS_MSDOS },
	{ MBR_PTYPE_FAT16B, FS_MSDOS },
	{ MBR_PTYPE_FAT32, FS_MSDOS },
	{ MBR_PTYPE_FAT32L, FS_MSDOS },
	{ MBR_PTYPE_FAT16L, FS_MSDOS },
	{ MBR_PTYPE_LNXEXT2, FS_EX2FS },
	{ 0, 0 }
};

int
nbsdtype(int type)
{
	struct typetab *tt;

	for (tt = typetable; tt->mbrtype; tt++)
		if (tt->mbrtype == type)
			return tt->nbsdtype;
	return FS_OTHER;
}

u_int32_t
getlong(void *p)
{
	unsigned char *cp = p;

	return cp[0] | (cp[1] << 8) | (cp[2] << 16) | (cp[3] << 24);
}

int
getparts(int sd, int np, u_int32_t off, u_int32_t eoff)
{
	unsigned char buf[DEV_BSIZE];
	struct mbr_partition parts[NMBRPART];
	off_t loff = 0;	/* XXX this nonsense shuts up GCC 2.7.2.2 */
	int i;

	loff = (off_t)off * DEV_BSIZE;

	if (lseek(sd, loff, SEEK_SET) != loff) {
		perror("seek label");
		exit(1);
	}
	if (read(sd, buf, DEV_BSIZE) != DEV_BSIZE) {
		perror("read label");
		exit(1);
	}
	if (buf[0x1fe] != 0x55 || buf[0x1ff] != 0xaa)
		return np;
	memcpy(parts, buf + MBR_PARTOFF, sizeof parts);
	for (i = 0; i < NMBRPART; i++) {
		switch (parts[i].mbrp_typ) {
		case 0:
			/* Nothing to do */
			break;
		case MBR_PTYPE_EXT:
		case MBR_PTYPE_EXT_LBA:
		case MBR_PTYPE_EXT_LNX:
			/* Will be handled below */
			break;
		default:
			label.d_partitions[np].p_size = getlong(&parts[i].mbrp_size);
			label.d_partitions[np].p_offset = getlong(&parts[i].mbrp_start) + off;
			label.d_partitions[np].p_fstype = nbsdtype(parts[i].mbrp_typ);
			switch (label.d_partitions[np].p_fstype) {
			case FS_BSDFFS:
				label.d_partitions[np].p_size = 16384;
				label.d_partitions[np].p_fsize = 1024;
				label.d_partitions[np].p_frag = 8;
				label.d_partitions[np].p_cpg = 16;
				break;
#ifdef	__does_not_happen__
			case FS_BSDLFS:
				label.d_partitions[np].p_size = 16384;
				label.d_partitions[np].p_fsize = 1024;
				label.d_partitions[np].p_frag = 8;
				label.d_partitions[np].p_sgs = XXX;
				break;
#endif
			}
			np++;
			break;
		}
		if (np >MAXPARTITIONS)
			return np;
		if (np == RAW_PART)
			np++;
	}
	for (i = 0; i < NMBRPART; i++) {
		u_int32_t poff;

		switch (parts[i].mbrp_typ) {
		case MBR_PTYPE_EXT:
		case MBR_PTYPE_EXT_LBA:
		case MBR_PTYPE_EXT_LNX:
			poff = getlong(&parts[i].mbrp_start) + eoff;
			np = getparts(sd, np, poff, eoff ? eoff : poff);
			break;
		default:
			break;
		}
		if (np >MAXPARTITIONS)
			return np;
	}
	return np;
}

void
usage(void)
{
	fprintf(stderr, "usage: mbrlabel rawdisk\n");
	exit(1);
}

int
main(int argc, char **argv)
{
	int sd;
	int np;
	char name[MAXPATHLEN];

	if (argc != 2)
		usage();

	if ((sd = opendisk(*++argv, O_RDWR, name, MAXPATHLEN, 0)) < 0) {
		perror(*argv);
		exit(1);
	}
	getlabel(sd);
	np = getparts(sd, FIRSTPART, MBR_BBSECTOR, 0);
	if (np > label.d_npartitions)
		label.d_npartitions = np;
	setlabel(sd);
	close(sd);
	return 0;
}
