/*	$NetBSD: fsirand.c,v 1.14 2001/08/17 02:18:48 lukem Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
#ifndef lint
__RCSID("$NetBSD: fsirand.c,v 1.14 2001/08/17 02:18:48 lukem Exp $");
#endif /* lint */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <err.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <sys/disklabel.h>
#include <sys/ioctl.h>

#include <ufs/ufs/quota.h>
#include <ufs/ufs/inode.h>
#include <ufs/ufs/ufs_bswap.h>

#include <ufs/ffs/fs.h>
#include <ufs/ffs/ffs_extern.h>

static void usage(void);
static void getsblock(int, const char *, struct disklabel *, struct fs *);
static void fixinodes(int, struct fs *, struct disklabel *, int, long);

int main(int, char *[]);

int needswap = 0;

static void
usage(void)
{

	(void) fprintf(stderr, "Usage: %s [-x <constant>] [-p] <special>\n",
	    getprogname());
	exit(1);
}


/*
 * getsblock():
 *	Return the superblock 
 */
static void
getsblock(int fd, const char *name, struct disklabel *lab, struct fs *fs)
{
	struct partition *pp;
	char p;

	pp = NULL;
	p = name[strlen(name) - 1];
	if (p >= 'a' && p <= 'h')
		pp = &lab->d_partitions[p - 'a'];
	else if (isdigit((unsigned char) p))
		pp = &lab->d_partitions[0];
	else
		errx(1, "Invalid partition `%c'", p);

	if (pp->p_fstype != FS_BSDFFS)
		errx(1, "Not an FFS partition");

	if (lseek(fd, (off_t) SBOFF , SEEK_SET) == (off_t) -1)
		err(1, "Cannot seek to superblock");

	if (read(fd, fs, SBSIZE) != SBSIZE)
		err(1, "Cannot read superblock");

	if (fs->fs_magic != FS_MAGIC)  {
		if(fs->fs_magic == bswap32(FS_MAGIC)) {
			needswap = 1;
			ffs_sb_swap(fs, fs);
		} else
			errx(1, "Bad superblock magic number");
	}

	if (fs->fs_ncg < 1)
		errx(1, "Bad ncg in superblock");

	if (fs->fs_cpg < 1)
		errx(1, "Bad cpg in superblock");

	if (fs->fs_ncg * fs->fs_cpg < fs->fs_ncyl ||
	    (fs->fs_ncg - 1) * fs->fs_cpg >= fs->fs_ncyl)
		errx(1, "Bad number of cylinders in superblock");

	if (fs->fs_sbsize > SBSIZE)
		errx(1, "Superblock too large");
}

/*
 * fixinodes():
 *	Randomize the inode generation numbers
 */
static void
fixinodes(int fd, struct fs *fs, struct disklabel *lab, int pflag, long xorval)
{
	int inopb = INOPB(fs);
	int size = inopb * DINODE_SIZE;
	caddr_t buf;
	struct dinode *dip;
	int i, ino, imax;

	if ((buf = malloc(size)) == NULL)
		err(1, "Out of memory");

	for (ino = 0, imax = fs->fs_ipg * fs->fs_ncg; ino < imax;) {
		off_t sp;
		sp = (off_t) fsbtodb(fs, ino_to_fsba(fs, ino)) *
		     (off_t) lab->d_secsize;

		if (lseek(fd, sp, SEEK_SET) == (off_t) -1)
			err(1, "Seeking to inode %d failed", ino);

		if (read(fd, buf, size) != size)
			err(1, "Reading inodes %d+%d failed", ino, inopb);

		for (i = 0; i < inopb; i++) {
			dip = (struct dinode *)(buf + (i * DINODE_SIZE));
			if (pflag)
				printf("ino %d gen 0x%x\n", ino,
					ufs_rw32(dip->di_gen, needswap));
			else
				dip->di_gen = ufs_rw32(random() ^ xorval,
				    needswap);
			if (++ino > imax)
				errx(1, "Exceeded number of inodes");
		}

		if (pflag)
			continue;

		if (lseek(fd, sp, SEEK_SET) == (off_t) -1)
			err(1, "Seeking to inode %d failed", ino);

		if (write(fd, buf, size) != size)
			err(1, "Writing inodes %d+%d failed", ino, inopb);
	}
	free(buf);
}

int
main(int argc, char *argv[])
{
	char buf[SBSIZE];
	struct fs *fs = (struct fs *) buf;
	struct disklabel lab;
	int fd, c;
	long xorval = 0;
	char *ep;
	struct timeval tv;

	int pflag = 0;

	while ((c = getopt(argc, argv, "px:")) != -1)
		switch (c) {
		case 'p':
			pflag++;
			break;
		case 'x':
			xorval = strtol(optarg, &ep, 0);
			if ((xorval == LONG_MIN || xorval == LONG_MAX) &&
			    errno == ERANGE)
				err(1, "Out of range constant");
			if (*ep)
				errx(1, "Bad constant");
			break;
		default:
			usage();
		}

	argv += optind;
	argc -= optind;

	if (argc != 1)
		usage();

	(void) gettimeofday(&tv, NULL);
	srandom((unsigned) tv.tv_usec);

	if ((fd = open(argv[0], pflag ? O_RDONLY : O_RDWR)) == -1)
		err(1, "Cannot open `%s'", argv[0]);

	if (ioctl(fd, DIOCGDINFO, &lab) == -1)
		err(1, "Cannot get label information");

	getsblock(fd, argv[0], &lab, fs);
	fixinodes(fd, fs, &lab, pflag, xorval);

	(void) close(fd);
	return 0;
}
