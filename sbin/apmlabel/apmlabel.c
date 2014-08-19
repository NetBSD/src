/*	$NetBSD: apmlabel.c,v 1.2.8.1 2014/08/20 00:02:24 tls Exp $	*/

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
__RCSID("$NetBSD: apmlabel.c,v 1.2.8.1 2014/08/20 00:02:24 tls Exp $");
#endif /* not lint */

#include <stdio.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include <sys/param.h>
#define FSTYPENAMES
#include <sys/disklabel.h>
#include <sys/bootblock.h>
#include <sys/ioctl.h>

#include "dkcksum.h"
#include "extern.h"

__dead static void	usage(void);
static void	getlabel(int);
static void	setlabel(int, int);
static int	getparts(int, int);
static struct apple_drvr_map *convert_drvr_map(unsigned char *);
static struct apple_part_map_entry *convert_part_map_entry(unsigned char *);

static struct disklabel label;

static void
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

static void
setlabel(int sd, int doraw)
{
	int one = 1;

	label.d_checksum = 0;
	label.d_checksum = dkcksum(&label);
	if (ioctl(sd, doraw ? DIOCWDINFO : DIOCSDINFO, &label) < 0) {
		perror("set label");
		exit(1);
	}
	if (!doraw)
		/* If we haven't written to the disk, don't discard on close */
		ioctl(sd, DIOCKLABEL, &one);

}

static int
getparts(int sd, int verbose)
{
	unsigned char		buf[DEV_BSIZE];
	struct apple_drvr_map	*drvr;
	struct apple_part_map_entry *part;
	struct partition	npe;
	uint16_t		blksize, partcnt;
	int			i, j, unused, changed;
	uint64_t		temp;

	changed = 0;

	if (lseek(sd, 0, SEEK_SET) == -1) {
		perror("seek drvr map");
		exit(1);
	}
	if ((i=read(sd, buf, sizeof(buf))) != DEV_BSIZE) {
		perror("read drvr map");
		exit(1);
	}
	drvr = convert_drvr_map(buf);

	if (drvr->sbSig != APPLE_DRVR_MAP_MAGIC)
		return (changed);
	blksize = drvr->sbBlockSize;

	partcnt = 1;
	
	for (i = 0; i < partcnt; i++) {
		if (lseek(sd, (i+1)*blksize, SEEK_SET) == -1) {
			perror("seek part");
			exit(1);
		}
		if (read(sd, buf, sizeof(buf)) != DEV_BSIZE) {
			perror("read part");
			exit(1);
		}

		part = convert_part_map_entry(buf);

		if (part->pmSig != APPLE_PART_MAP_ENTRY_MAGIC)
			return (changed);
		if (i == 0)
			partcnt = part->pmMapBlkCnt;
		/* XXX: consistency checks? */

		memset((void *)&npe, 0, sizeof(npe));

		if (strcasecmp((char *)part->pmPartType,
			APPLE_PART_TYPE_MAC) == 0
		    || strcasecmp((char *)part->pmPartType, "Apple_HFSX") == 0)
			npe.p_fstype = FS_HFS;
		else if (strcasecmp((char *)part->pmPartType,
			     "Apple_UFS") == 0) {
			npe.p_fstype = FS_APPLEUFS;
			npe.p_size = 16384;	/* XXX */
			npe.p_fsize = 1024;
			npe.p_frag = 8;
			npe.p_cpg = 16;
		}
		else
			continue;

		temp = (uint64_t)part->pmDataCnt * (uint64_t)blksize;
		if (temp % label.d_secsize != 0) {
		    warnx("partition size not multiple of sector size"
			  ", skipping");
		    continue;
		}
		npe.p_size = temp / label.d_secsize;
		temp = (uint64_t)(part->pmPyPartStart + part->pmLgDataStart)
		    * (uint64_t)blksize;
		if (temp % label.d_secsize != 0) {
		    warnx("partition offset not multiple of sector size"
			  ", skipping");
		    continue;
		}
		npe.p_offset = temp / label.d_secsize;

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
		changed++;
		label.d_partitions[unused] = npe;
	}

	return (changed);
}

static struct apple_drvr_map *
convert_drvr_map(unsigned char *buf)
{
	struct apple_drvr_map *drvr;
	int i;

	drvr = (struct apple_drvr_map *)buf;

	BE16TOH(drvr->sbSig);
	BE16TOH(drvr->sbBlockSize);
	BE32TOH(drvr->sbBlkCount);
	BE16TOH(drvr->sbDevType);
	BE16TOH(drvr->sbDevID);
	BE32TOH(drvr->sbData);
	BE16TOH(drvr->sbDrvrCount);
	for (i=0; i<APPLE_DRVR_MAP_MAX_DESCRIPTORS; i++) {
		BE32TOH(drvr->sb_dd[i].descBlock);
		BE16TOH(drvr->sb_dd[i].descSize);
		BE16TOH(drvr->sb_dd[i].descType);
	}

	return drvr;
}

static struct apple_part_map_entry *
convert_part_map_entry(unsigned char *buf)
{
	struct apple_part_map_entry *part;

	part = (struct apple_part_map_entry *)buf;

	BE16TOH(part->pmSig);
	BE16TOH(part->pmSigPad);
	BE32TOH(part->pmMapBlkCnt);
	BE32TOH(part->pmPyPartStart);
	BE32TOH(part->pmPartBlkCnt);
	BE32TOH(part->pmLgDataStart);
	BE32TOH(part->pmDataCnt);
	BE32TOH(part->pmPartStatus);
	BE32TOH(part->pmLgBootStart);
	BE32TOH(part->pmBootSize);
	BE32TOH(part->pmBootLoad);
	BE32TOH(part->pmBootLoad2);
	BE32TOH(part->pmBootEntry);
	BE32TOH(part->pmBootEntry2);
	BE32TOH(part->pmBootCksum);

	return part;
}

static void
usage(void)
{
	fprintf(stderr, "usage: %s [-fqrw] rawdisk\n",
	    getprogname());
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

	if ((sd = opendisk(argv[0], write_it ? O_RDWR : O_RDONLY, name,
	    (size_t)MAXPATHLEN, 1)) < 0) {
		perror(argv[0]);
		exit(1);
	}
	getlabel(sd);
	changed = getparts(sd, verbose);

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
