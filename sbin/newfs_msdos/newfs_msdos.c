/*	$NetBSD: newfs_msdos.c,v 1.6 2001/02/19 22:56:21 cgd Exp $	*/

/*
 * Copyright (c) 1997 Christos Zoulas
 * Copyright (c) 1995, 1996 Joerg Wunsch
 *
 * All rights reserved.
 *
 * This program is free software.
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
 * THIS SOFTWARE IS PROVIDED BY THE DEVELOPERS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE DEVELOPERS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Create an MS-DOS (FAT) file system.
 *
 * Id: mkdosfs.c,v 1.4 1997/02/22 16:06:38 peter Exp
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: newfs_msdos.c,v 1.6 2001/02/19 22:56:21 cgd Exp $");
#endif /* not lint */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <memory.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>

/* From msdosfs */
#include <direntry.h>
#include <bpb.h>
#include <bootsect.h>

#include "bootcode.h"

struct descrip {
	/* our database key */
	unsigned        kilobytes;
	/* MSDOS 3.3 BPB fields */
	u_short         sectsiz;
	u_char          clustsiz;
	u_short         ressecs;
	u_char          fatcnt;
	u_short         rootsiz;
	u_short         totsecs;
	u_char          media;
	u_short         fatsize;
	u_short         trksecs;
	u_short         headcnt;
	u_short         hidnsec;
	/* MSDOS 4 BPB extensions */
	u_long          ext_totsecs;
	u_short         ext_physdrv;
	u_char          ext_extboot;
	char            ext_label[11];
	char            ext_fsysid[8];
};

static struct descrip table[] = {
	/* NB: must be sorted, starting with the largest format! */
	/*
         *  KB  sec cls res fat  rot   tot   med fsz spt hds hid
         * tot  phs ebt  label       fsysid
         */
	{ 1440, 512,  1,  1,  2, 224, 2880, 0xf0,  9, 18,  2,  0,
	     0,   0,  0, "4.4BSD     ", "FAT12   " },
	{ 1200, 512,  1,  1,  2, 224, 2400, 0xf9,  7, 15,  2,  0,
	     0,   0,  0, "4.4BSD     ", "FAT12   " },
	{  720, 512,  2,  1,  2, 112, 1440, 0xf9,  3,  9,  2,  0,
	     0,   0,  0, "4.4BSD     ", "FAT12   " },
	{  360, 512,  2,  1,  2, 112,  720, 0xfd,  2,  9,  2,  0,
	     0,   0,  0, "4.4BSD     ", "FAT12   " },
	{    0,   0,  0,  0,  0,   0,    0, 0x00,  0,  0,  0,  0,
	     0,   0,  0, "           ", "        " }
};

struct fat {
	u_int8_t media;		/* the media descriptor again */
	u_int8_t padded;	/* always 0xff */
	u_int8_t contents[1];	/* the `1' is a placeholder only */
}; 


int main __P((int, char *[]));

static void usage __P((void));
static size_t findformat __P((int));
static void setup_boot_sector_from_template __P((union bootsector *,
    struct descrip *));

static void
usage()
{

	(void) fprintf(stderr,
	    "Usage: %s [-f <kbytes>] [-L <label>] <device>\n", getprogname());
	exit(1);
}

/*
 *	Try to deduce a format appropriate for our disk.
 *	This is a bit tricky.  If the argument is a regular file, we can
 *	lseek() to its end and get the size reported.  If it's a device
 *	however, lseeking doesn't report us any useful number.  Instead,
 *	we try to seek just to the end of the device and try reading a
 *	block there.  In the case where we've hit exactly the device
 *	boundary, we get a zero read, and thus have found the size.
 *	Since our knowledge of distinct formats is limited anyway, this
 *	is not a big deal at all.
 */
static size_t
findformat(fd)
	int fd;
{
	struct stat     sb;
	off_t offs;


	if (fstat(fd, &sb) == -1)
		err(1, "Cannot fstat disk");	/* Cannot happen */

	if (S_ISREG(sb.st_mode)) {
		if (lseek(fd, (off_t) 0, SEEK_END) == -1 ||
		    (offs = lseek(fd, (off_t) 0, SEEK_CUR)) == -1)
			/* Hmm, hmm.  Hard luck. */
			return 0;
		(void) lseek(fd, (off_t) 0, SEEK_SET);
		return (size_t) (offs / 1024);
	} else if (S_ISCHR(sb.st_mode) || S_ISBLK(sb.st_mode)) {
		char            b[512];
		int             rv;
		struct descrip *dp;

		for (dp = table; dp->kilobytes != 0; dp++) {
			offs = dp->kilobytes * 1024;

			if (lseek(fd, offs, SEEK_SET) == -1)
				/* Uh-oh, lseek() is not supposed to fail. */
				return 0;

			if ((rv = read(fd, b, 512)) == 0)
				break;
			/*
			 * XXX The ENOSPC is for the bogus fd(4) driver
			 * return value.
			 */
			if (rv == -1 && errno != EINVAL && errno != ENOSPC)
				return 0;
			/* else: continue */
		}
		(void) lseek(fd, (off_t) 0, SEEK_SET);
		return dp->kilobytes;
	} else
		/* Outta luck. */
		return 0;
}


static void
setup_boot_sector_from_template(bs, dp)
	union bootsector *bs;
	struct descrip *dp;
{
	struct byte_bpb50 bpb;
	struct extboot *exb = (struct extboot *)bs->bs50.bsExt;

	assert(sizeof(bs->bs50) == 512);
	assert(sizeof(bootcode) == 512);

	(void) memcpy(&bs->bs50, bootcode, 512);

	putushort(bpb.bpbBytesPerSec, dp->sectsiz);
	bpb.bpbSecPerClust = dp->clustsiz;
	putushort(bpb.bpbResSectors, dp->ressecs);
	bpb.bpbFATs = dp->fatcnt;
	putushort(bpb.bpbRootDirEnts, dp->rootsiz);
	putushort(bpb.bpbSectors, dp->totsecs);
	bpb.bpbMedia = dp->media;
	putushort(bpb.bpbFATsecs, dp->fatsize);
	putushort(bpb.bpbSecPerTrack, dp->trksecs);
	putushort(bpb.bpbHeads, dp->headcnt);
	putulong(bpb.bpbHiddenSecs, dp->hidnsec);
	putulong(bpb.bpbHugeSectors, dp->ext_totsecs);

	exb->exDriveNumber = dp->ext_physdrv;
	exb->exBootSignature = dp->ext_extboot;

	/* assign a "serial number" :) */
	srandom((unsigned) time((time_t) 0));
	putulong(exb->exVolumeID, random());

	(void) memcpy(exb->exVolumeLabel, dp->ext_label,
	    MIN(sizeof(dp->ext_label), sizeof(exb->exVolumeLabel)));
	(void) memcpy(exb->exFileSysType, dp->ext_fsysid,
	    MIN(sizeof(dp->ext_fsysid), sizeof(exb->exFileSysType)));
	(void) memcpy(bs->bs50.bsBPB, &bpb, 
	    MIN(sizeof(bpb), sizeof(bs->bs50.bsBPB)));
}

int
main(argc, argv)
	int argc;
	char *argv[];
{
	union bootsector bs;
	struct descrip *dp;
	struct fat     *fat;
	struct direntry *rootdir;
	struct tm      *tp;
	time_t          now;
	int             c, i, fd, format = 0, rootdirsize, fatsz;
	const char     *label = 0;

	while ((c = getopt(argc, argv, "f:L:")) != -1)
		switch (c) {
		case 'f':
			format = atoi(optarg);
			break;

		case 'L':
			label = optarg;
			break;

		case '?':
		default:
			usage();
		}

	argc -= optind;
	argv += optind;

	if (argc != 1)
		usage();

	if ((fd = open(argv[0], O_RDWR | O_EXCL, 0)) == -1)
		err(1, "Cannot open `%s'", argv[0]);

	/* If no format specified, try to figure it out. */
	if (format == 0  && (format = findformat(fd)) == 0)
		errx(1, "Cannot determine size, must use -f format");

	for (dp = table; dp->kilobytes != 0; dp++)
		if (dp->kilobytes == format)
			break;

	if (dp->kilobytes == 0)
		errx(1, "Cannot find format description for %d KB", format);

	/* prepare and write the boot sector */
	setup_boot_sector_from_template(&bs, dp);

	/* if we've got an explicit label, use it */
	if (label)
		(void) strncpy(((struct extboot *)(bs.bs50.bsExt))
			       ->exVolumeLabel, label, 11);

	if (write(fd, &bs, sizeof bs) != sizeof bs)
		err(1, "Writing boot sector");

	/* now, go on with the FATs */
	if ((fat = (struct fat *) malloc(dp->sectsiz * dp->fatsize)) == NULL)
		err(1, "Out of memory (FAT table)");

	(void) memset(fat, 0, dp->sectsiz * dp->fatsize);

	fat->media = dp->media;
	fat->padded = 0xff;
	fat->contents[0] = 0xff;
	if (dp->totsecs > 20740 ||
	    (dp->totsecs == 0 && dp->ext_totsecs > 20740))
		/* 16-bit FAT */
		fat->contents[1] = 0xff;

	fatsz =  dp->sectsiz * dp->fatsize;
	for (i = 0; i < dp->fatcnt; i++)
		if (write(fd, fat, fatsz) != fatsz)
			err(1, "Writing FAT %d", i);

	free(fat);

	/* finally, build the root dir */
	rootdirsize = dp->rootsiz * sizeof(struct direntry);
	rootdirsize = roundup(rootdirsize, dp->clustsiz * dp->sectsiz);

	if ((rootdir = (struct direntry *) malloc(rootdirsize)) == NULL)
		err(1, "Out of memory (root directory)");

	(void) memset(rootdir, 0, rootdirsize);

	/* set up a volume label inside the root dir :) */
	if (label)
		(void) strncpy(rootdir->deName, label, 11);
	else
		(void) memcpy(rootdir->deName, dp->ext_label, 11);

	rootdir->deAttributes = ATTR_VOLUME;
	now = time((time_t) 0);
	tp = localtime(&now);
	rootdir->deCTime[0] = tp->tm_sec / 2;
	rootdir->deCTime[0] |= (tp->tm_min & 7) << 5;
	rootdir->deCTime[1] = ((tp->tm_min >> 3) & 7);
	rootdir->deCTime[1] |= tp->tm_hour << 3;
	rootdir->deCDate[0] = tp->tm_mday;
	rootdir->deCDate[0] |= ((tp->tm_mon + 1) & 7) << 5;
	rootdir->deCDate[1] = ((tp->tm_mon + 1) >> 3) & 1;
	rootdir->deCDate[1] |= (tp->tm_year - 80) << 1;

	if (write(fd, rootdir, rootdirsize) != rootdirsize)
		err(1, "Writing root directory");

	(void) close(fd);

	return 0;
}
