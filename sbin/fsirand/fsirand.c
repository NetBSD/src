/*	$NetBSD: fsirand.c,v 1.17 2001/11/16 04:25:43 lukem Exp $	*/

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
__RCSID("$NetBSD: fsirand.c,v 1.17 2001/11/16 04:25:43 lukem Exp $");
#endif /* lint */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <sys/disklabel.h>
#include <sys/ioctl.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include <ufs/ufs/inode.h>
#include <ufs/ufs/ufs_bswap.h>

#include <ufs/ffs/fs.h>
#include <ufs/ffs/ffs_extern.h>

static void usage(void);
static void getsblock(int, const char *, struct fs *);
static void fixinodes(int, struct fs *, struct disklabel *, int, long);
static void statussig(int);

int main(int, char *[]);

int	needswap, ino, imax;
time_t	tstart;


static void
usage(void)
{

	(void) fprintf(stderr,
	    "Usage: %s [-F] [-p] [-x <constant>] <special>\n",
	    getprogname());
	exit(1);
}


/*
 * getsblock():
 *	Return the superblock 
 */
static void
getsblock(int fd, const char *name, struct fs *fs)
{

	if (lseek(fd, (off_t) SBOFF , SEEK_SET) == (off_t) -1)
		err(1, "%s: cannot seek to superblock", name);

	if (read(fd, fs, SBSIZE) != SBSIZE)
		err(1, "%s: cannot read superblock", name);

	if (fs->fs_magic != FS_MAGIC)  {
		if(fs->fs_magic == bswap32(FS_MAGIC)) {
			needswap = 1;
			ffs_sb_swap(fs, fs);
		} else
			errx(1, "%s: bad superblock magic number", name);
	}

	if (fs->fs_ncg < 1)
		errx(1, "%s: bad ncg in superblock", name);

	if (fs->fs_cpg < 1)
		errx(1, "%s: bad cpg in superblock", name);

	if (fs->fs_ncg * fs->fs_cpg < fs->fs_ncyl ||
	    (fs->fs_ncg - 1) * fs->fs_cpg >= fs->fs_ncyl)
		errx(1, "%s: bad number of cylinders in superblock", name);

	if (fs->fs_sbsize > SBSIZE)
		errx(1, "%s: superblock too large", name);
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
	int i;

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
				printf("inode %10d   gen 0x%08x\n", ino,
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

/*
 * statussig():
 *	display current status
 */
void
statussig(int dummy)
{
	char	msgbuf[256];
	int	len, deltat;
	time_t	tnow, elapsed;

	(void)time(&tnow);
	elapsed = tnow - tstart;
	len = snprintf(msgbuf, sizeof(msgbuf),
	    "fsirand: completed inode %d of %d (%3.2f%%)",
	    ino, imax, (ino * 100.0) / imax);
	if (imax - ino) {
		deltat = tstart - tnow + (1.0 * (tnow - tstart)) / ino * imax;
		len += snprintf(msgbuf + len, sizeof(msgbuf) - len,
		    ", finished in %d:%02d\n", deltat / 60, deltat % 60);
	} else {
		len += snprintf(msgbuf + len, sizeof(msgbuf) - len, "\n");
	}
	write(STDERR_FILENO, msgbuf, len);
}

int
main(int argc, char *argv[])
{
	const char *special;
	char buf[SBSIZE], device[MAXPATHLEN];
	struct fs *fs = (struct fs *) buf;
	struct disklabel lab;
	long xorval;
	char *ep;
	struct timeval tv;
	int fd, c, Fflag, pflag, openflags;

	xorval = 0;
	Fflag = pflag = 0;

	while ((c = getopt(argc, argv, "Fpx:")) != -1)
		switch (c) {
		case 'F':
			Fflag++;
			break;
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

	special = argv[0];
	openflags = pflag ? O_RDONLY : O_RDWR;
	if (Fflag)
		fd = open(special, openflags);
	else {
		fd = opendisk(special, openflags, device, sizeof(device), 0);
		special = device;
	}
	if (fd == -1)
		err(1, "Cannot open `%s'", special);

	if (Fflag) {
		memset(&lab, 0, sizeof(lab));
		lab.d_secsize = DEV_BSIZE;	/* XXX */
	} else {
		if (ioctl(fd, DIOCGDINFO, &lab) == -1)
			err(1, "%s: cannot get disklabel information", special);
	}

	time(&tstart);
	(void)signal(SIGINFO, statussig);
	getsblock(fd, special, fs);
	fixinodes(fd, fs, &lab, pflag, xorval);

	(void) close(fd);
	return 0;
}
