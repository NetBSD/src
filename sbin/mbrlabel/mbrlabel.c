/*	$NetBSD: mbrlabel.c,v 1.19 2002/02/07 02:14:02 ross Exp $	*/

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
__RCSID("$NetBSD: mbrlabel.c,v 1.19 2002/02/07 02:14:02 ross Exp $");
#endif /* not lint */

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include <sys/param.h>
#define FSTYPENAMES
#include <sys/disklabel.h>
#include <sys/disklabel_mbr.h>
#include <sys/ioctl.h>

#include "dkcksum.h"
#include "extern.h"

int	main(int, char **);
void	usage(void);
void	getlabel(int);
void	setlabel(int, int);
int	getparts(int, u_int32_t, u_int32_t, int);
int	nbsdtype(int);
u_int16_t	getshort(void *);
u_int32_t	getlong(void *);

struct disklabel label;

void
getlabel(int sd)
{

	if (ioctl(sd, DIOCGDINFO, &label) < 0) {
		perror("get label");
		exit(1);
	}
	/*
	 * Some ports seem to not set the number of partitions
	 * correctly, albeit they seem to set the raw partition ok!
	 */
	if (label.d_npartitions <= getrawpartition())
		label.d_npartitions = getrawpartition() + 1;
}

void
setlabel(int sd, int doraw)
{

	label.d_checksum = 0;
	label.d_checksum = dkcksum(&label);
	if (ioctl(sd, doraw ? DIOCWDINFO : DIOCSDINFO, &label) < 0) {
		perror("set label");
		exit(1);
	}
}

static struct typetab {
	int mbrtype;
	int nbsdtype;
} typetable[] = {
	{ MBR_PTYPE_386BSD,	FS_BSDFFS },
	{ MBR_PTYPE_FAT12,	FS_MSDOS },
	{ MBR_PTYPE_FAT16B,	FS_MSDOS },
	{ MBR_PTYPE_FAT16L,	FS_MSDOS },
	{ MBR_PTYPE_FAT16S,	FS_MSDOS },
	{ MBR_PTYPE_FAT32,	FS_MSDOS },
	{ MBR_PTYPE_FAT32L,	FS_MSDOS },
	{ MBR_PTYPE_LNXEXT2,	FS_EX2FS },
	{ MBR_PTYPE_LNXSWAP,	FS_SWAP },
	{ MBR_PTYPE_NETBSD,	FS_BSDFFS },
	{ MBR_PTYPE_NTFS,	FS_NTFS },
	{ 0, 0 }
};

int
nbsdtype(int type)
{
	struct typetab *tt;

	for (tt = typetable; tt->mbrtype; tt++)
		if (tt->mbrtype == type)
			return (tt->nbsdtype);
	return (FS_OTHER);
}

u_int16_t
getshort(void *p)
{
	unsigned char *cp = p;

	return (cp[0] | (cp[1] << 8));
}

u_int32_t
getlong(void *p)
{
	unsigned char *cp = p;

	return (cp[0] | (cp[1] << 8) | (cp[2] << 16) | (cp[3] << 24));
}

int
getparts(int sd, u_int32_t off, u_int32_t extoff, int verbose)
{
	unsigned char		buf[DEV_BSIZE];
	struct mbr_partition	parts[NMBRPART];
	struct partition	npe;
	off_t			loff;
	int			i, j, unused, changed;

	changed = 0;
	loff = (off_t)off * DEV_BSIZE;

	if (lseek(sd, loff, SEEK_SET) != loff) {
		perror("seek label");
		exit(1);
	}
	if (read(sd, buf, sizeof buf) != DEV_BSIZE) {
		perror("read label");
		exit(1);
	}
	if (getshort(buf + MBR_MAGICOFF) != MBR_MAGIC)
		return (changed);
	memcpy(parts, buf + MBR_PARTOFF, sizeof parts);

				/* scan partition table */
	for (i = 0; i < NMBRPART; i++) {
		if (parts[i].mbrp_typ == 0 ||
				/* extended partitions are handled below */
		    MBR_IS_EXTENDED(parts[i].mbrp_typ))
			continue;

		memset((void *)&npe, 0, sizeof(npe));
		npe.p_size = getlong(&parts[i].mbrp_size);
		npe.p_offset = getlong(&parts[i].mbrp_start) + off;
		npe.p_fstype = nbsdtype(parts[i].mbrp_typ);

				/* find existing entry, or first free slot */
		unused = -1;	/* flag as no free slot */
		if (verbose)
			printf(
			    "Found %s partition; size %u (%u MB), offset %u\n",
			    fstypenames[npe.p_fstype],
			    npe.p_size, npe.p_size / 2048, npe.p_offset);
		for (j = 0; j < label.d_npartitions; j++) {
			struct partition *lpe;

			if (j == RAW_PART)
				continue;
			lpe = &label.d_partitions[j];
			if (lpe->p_size == npe.p_size &&
			    lpe->p_offset == npe.p_offset
#ifdef notyet
			    && (lpe->p_fstype == npe.p_fstype ||
			     lpe->p_fstype == FS_UNUSED) */
#endif
			     ) {
				if (verbose)
					printf(
			    "  skipping existing %s partition at slot %c.\n",
					    fstypenames[lpe->p_fstype],
					    j + 'a');
				unused = -2;	/* flag as existing */
				break;
			}
			if (unused == -1 && lpe->p_size == 0 &&
			    lpe->p_fstype == FS_UNUSED)
				unused = j;
		}
		if (unused == -2)
			continue;	/* entry exists, skip... */
		if (unused == -1) {
			if (label.d_npartitions < MAXPARTITIONS) {
				unused = label.d_npartitions;
				label.d_npartitions++;
			} else {
				printf(
				"  WARNING: no slots free for %s partition.\n",
				    fstypenames[npe.p_fstype]);
				continue;
			}
		}

		if (verbose)
			printf("  adding %s partition to slot %c.\n",
			    fstypenames[npe.p_fstype], unused + 'a');
		switch (npe.p_fstype) {
		case FS_BSDFFS:
			npe.p_size = 16384;	/* XXX */
			npe.p_fsize = 1024;
			npe.p_frag = 8;
			npe.p_cpg = 16;
			break;
#ifdef	__does_not_happen__
		case FS_BSDLFS:
			npe.p_size = 16384;	/* XXX */
			npe.p_fsize = 1024;
			npe.p_frag = 8;
			npe.p_sgs = XXX;
			break;
#endif
		}
		changed++;
		label.d_partitions[unused] = npe;
	}

				/* recursively scan extended partitions */
	for (i = 0; i < NMBRPART; i++) {
		u_int32_t poff;

		if (MBR_IS_EXTENDED(parts[i].mbrp_typ)) {
			poff = getlong(&parts[i].mbrp_start) + extoff;
			changed += getparts(sd, poff,
			    extoff ? extoff : poff, verbose);
		}
	}
	return (changed);
}

void
usage(void)
{
	fprintf(stderr, "Usage: %s [-fqrw] rawdisk\n", getprogname());
	exit(1);
}


int
main(int argc, char **argv)
{
	int	sd, ch, changed;
	char	name[MAXPATHLEN];
	int	force;			/* force label update */
	int	raw;			/* update on-disk label as well */
	int	verbose;		/* verbose output */
	int	write_it;		/* update in-core label if changed */

	force = 0;
	raw = 0;
	verbose = 1;
	write_it = 0;
	while ((ch = getopt(argc, argv, "fqrw")) != -1) {
		switch (ch) {
		case 'f':
			force = 1;
			break;
		case 'q':
			verbose = 0;
			break;
		case 'r':
			raw = 1;
			break;
		case 'w':
			write_it = 1;
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;
	if (argc != 1)
		usage();

	if ((sd = opendisk(argv[0], O_RDWR, name, (size_t)MAXPATHLEN, 0)) < 0) {
		perror(argv[0]);
		exit(1);
	}
	getlabel(sd);
	changed = getparts(sd, MBR_BBSECTOR, 0, verbose);

	if (verbose) {
		putchar('\n');
		showpartitions(stdout, &label, 0);
		putchar('\n');
	}
	if (write_it) {
		if (! changed && ! force)
			printf("No change; not updating disk label.\n");
		else {
			if (verbose)
				printf("Updating in-core %sdisk label.\n",
				    raw ? "and on-disk " : "");
			setlabel(sd, raw);
		}
	} else {
		printf("Not updating disk label.\n");
	}
	close(sd);
	return (0);
}
