/*	$NetBSD: svhlabel.c,v 1.7 2013/02/07 10:44:45 apb Exp $	*/

/*
 * Copyright (C) 2007 Stephen M. Rumble.
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
__RCSID("$NetBSD: svhlabel.c,v 1.7 2013/02/07 10:44:45 apb Exp $");
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

#include <fs/efs/efs.h>
#include <fs/efs/efs_sb.h>

#include "dkcksum.h"
#include "extern.h"

__dead static void	usage(void);
static void	getlabel(int);
static void	setlabel(int, int);
static int	getparts(int, int);
static int	is_efs(int, uint32_t);
static struct sgi_boot_block *convert_sgi_boot_block(unsigned char *);

struct disklabel label;
static int	rawpart;

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
	if (label.d_npartitions <= rawpart)
		label.d_npartitions = rawpart + 1;
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
	struct sgi_boot_block  *vh;
	struct partition	npe;
	int			i, j, changed;

	changed = 0;

	if (lseek(sd, 0, SEEK_SET) == -1) {
		perror("seek vh");
		exit(1);
	}
	if ((i = read(sd, buf, sizeof(buf))) != DEV_BSIZE) {
		perror("read vh");
		exit(1);
	}
	vh = convert_sgi_boot_block(buf);

	if (vh->magic != SGI_BOOT_BLOCK_MAGIC)
		return (changed);

	if (label.d_secsize != SGI_BOOT_BLOCK_BLOCKSIZE)
		changed++;
	label.d_secsize = SGI_BOOT_BLOCK_BLOCKSIZE;

	for (i = j = 0; i < SGI_BOOT_BLOCK_MAXPARTITIONS; i++) {
		if (vh->partitions[i].blocks == 0)
			continue;

		if (j == MAXPARTITIONS)
			break;

		switch (vh->partitions[i].type) {
		case SGI_PTYPE_EFS:
		/*
		 * For some reason, my IRIX CDs list EFS partitions as SYSV!?
		 */
		case SGI_PTYPE_SYSV:
			if (is_efs(sd, vh->partitions[i].first)) {
				npe.p_fstype = FS_EFS;
				npe.p_size = vh->partitions[i].blocks;
				npe.p_offset = vh->partitions[i].first;
				npe.p_fsize = 0;
				npe.p_frag = 0;
				npe.p_cpg = 0;
			}
			break;

		case SGI_PTYPE_VOLUME:
			if (label.d_secperunit != (uint32_t)vh->partitions[i].blocks)
				changed++;
			label.d_secperunit = vh->partitions[i].blocks; 
			continue;
		
		default:
			continue;
		} 

		if (j >= label.d_npartitions)
			break;

		if (j == rawpart) {
			if (++j >= label.d_npartitions)
				break;
		}

		if (memcmp(&label.d_partitions[j], &npe, sizeof(npe)) != 0) {
			label.d_partitions[j] = npe;
			changed++;
		}

		j++;
	}

	/* XXX - fudge */
	if (label.d_nsectors != 1 || label.d_ntracks != 1 ||
	    label.d_secpercyl != 1 || label.d_ncylinders != label.d_secperunit)
		changed++;
	label.d_nsectors = 1;
	label.d_ntracks = 1;
	label.d_secpercyl = 1;
	label.d_ncylinders = label.d_secperunit;

	i = rawpart;
	if (label.d_partitions[i].p_fstype != FS_UNUSED ||
	    label.d_partitions[i].p_offset != 0 ||
	    label.d_partitions[i].p_size != label.d_secperunit) {
		label.d_partitions[i].p_fstype = FS_UNUSED;
		label.d_partitions[i].p_offset = 0;
		label.d_partitions[i].p_size = label.d_secperunit;
		changed++;
	}

	return (changed);
}

static int
is_efs(int sd, uint32_t blkoff)
{
	struct efs_sb sb;
	off_t oldoff;

	if ((oldoff = lseek(sd, 0, SEEK_CUR)) == -1) {
		perror("is_efs lseek 0");
		exit(1);
	}

	blkoff *= SGI_BOOT_BLOCK_BLOCKSIZE;
	if (lseek(sd, blkoff + (EFS_BB_SB * EFS_BB_SIZE), SEEK_SET) == -1) {
		perror("is_efs lseek 1");
		exit(1);
	}

	if (read(sd, &sb, sizeof(sb)) != sizeof(sb)) {
		perror("is_efs read");
		exit(1);
	}

	if (lseek(sd, oldoff, SEEK_SET) == -1) {
		perror("is_efs lseek 2");
		exit(1);
	}

	BE32TOH(sb.sb_magic);

	return (sb.sb_magic == EFS_SB_MAGIC || sb.sb_magic == EFS_SB_NEWMAGIC);
}

static struct sgi_boot_block *
convert_sgi_boot_block(unsigned char *buf)
{
	struct sgi_boot_block *vh;
	int i;

	vh = (struct sgi_boot_block *)buf;

	BE32TOH(vh->magic);
	BE16TOH(vh->root);
	BE16TOH(vh->swap);

	BE16TOH(vh->dp.dp_cyls);
	BE16TOH(vh->dp.dp_shd0);
	BE16TOH(vh->dp.dp_trks0);
	BE16TOH(vh->dp.dp_secs);
	BE16TOH(vh->dp.dp_secbytes);
	BE16TOH(vh->dp.dp_interleave);
	BE32TOH(vh->dp.dp_flags);
	BE32TOH(vh->dp.dp_datarate);
	BE32TOH(vh->dp.dp_nretries);
	BE32TOH(vh->dp.dp_mspw);
	BE16TOH(vh->dp.dp_xgap1);
	BE16TOH(vh->dp.dp_xsync);
	BE16TOH(vh->dp.dp_xrdly);
	BE16TOH(vh->dp.dp_xgap2);
	BE16TOH(vh->dp.dp_xrgate);
	BE16TOH(vh->dp.dp_xwcont);

	for (i = 0; i < SGI_BOOT_BLOCK_MAXVOLDIRS; i++) {
		BE32TOH(vh->voldir[i].block);
		BE32TOH(vh->voldir[i].bytes);
	}

	for (i = 0; i < SGI_BOOT_BLOCK_MAXPARTITIONS; i++) {
		BE32TOH(vh->partitions[i].blocks);
		BE32TOH(vh->partitions[i].first);
		BE32TOH(vh->partitions[i].type);
	}

	BE32TOH(vh->checksum);

	return (vh);
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

	rawpart = getrawpartition();
	if (rawpart < 0)
		err(EXIT_FAILURE, "getrawpartition");

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
raw = 0; /* XXX */
			setlabel(sd, raw);
		}
	} else {
		printf("Not updating disk label.\n");
	}
	close(sd);
	return (0);
}
