/*	$NetBSD: fsirand.c,v 1.3 1997/03/14 00:00:26 cgd Exp $	*/

/*
 * Copyright (c) 1997 Christos Zoulas.  All rights reserved.
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
 *	This product includes software developed by Christos Zoulas.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 */

#ifndef lint
static char rcsid[] = "$NetBSD: fsirand.c,v 1.3 1997/03/14 00:00:26 cgd Exp $";
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

#include <ufs/ffs/fs.h>

static void usage __P((void));
static void getsblock __P((int, const char *, struct disklabel *, struct fs *));
static void fixinodes __P((int, struct fs *, struct disklabel *, int, long));

int main __P((int, char *[]));

static void
usage()
{
	extern char *__progname;

	(void) fprintf(stderr, "%s: [-p] <special>\n", __progname);
	exit(1);
}


/* getsblock():
 *	Return the superblock 
 */
static void
getsblock(fd, name, lab, fs)
	int fd;
	const char *name;
	struct disklabel *lab;
	struct fs *fs;
{
	struct partition *pp = NULL;
	char p = name[strlen(name) - 1];

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

	if (fs->fs_magic != FS_MAGIC)
		errx(1, "Bad superblock magic number");

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

/* fixinodes():
 *	Randomize the inode generation numbers
 */
static void
fixinodes(fd, fs, lab, pflag, xorval)
	int fd;
	struct fs *fs;
	struct disklabel *lab;
	int pflag;
	long xorval;
{
	int inopb = INOPB(fs);
	int size = inopb * sizeof(struct dinode);
	struct dinode *dibuf, *dip;
	int ino, imax;

	if ((dibuf = malloc(size)) == NULL)
		err(1, "Out of memory");

	for (ino = 0, imax = fs->fs_ipg * fs->fs_ncg; ino < imax;) {
		off_t sp = fsbtodb(fs, ino_to_fsba(fs, ino)) * lab->d_secsize;

		if (lseek(fd, sp, SEEK_SET) == (off_t) -1)
			err(1, "Seeking to inode %d failed", ino);

		if (read(fd, dibuf, size) != size)
			err(1, "Reading inodes %d+%d failed", ino, inopb);

		for (dip = dibuf; dip < &dibuf[inopb]; dip++) {
			if (pflag)
				printf("ino %d gen %x\n", ino, dip->di_gen);
			else
				dip->di_gen = random() ^ xorval;
			if (++ino > imax)
				errx(1, "Exceeded number of inodes");
		}

		if (pflag)
			continue;

		if (lseek(fd, sp, SEEK_SET) == (off_t) -1)
			err(1, "Seeking to inode %d failed", ino);

		if (write(fd, dibuf, size) != size)
			err(1, "Writing inodes %d+%d failed", ino, inopb);
	}
	free(dibuf);
}

int
main(argc, argv)
	int	argc;
	char	*argv[];
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
			if ((xorval = LONG_MIN || xorval == LONG_MAX) &&
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
