/*	$NetBSD: tunefs.c,v 1.25 2001/11/09 09:05:52 lukem Exp $	*/

/*
 * Copyright (c) 1983, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1983, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)tunefs.c	8.3 (Berkeley) 5/3/95";
#else
__RCSID("$NetBSD: tunefs.c,v 1.25 2001/11/09 09:05:52 lukem Exp $");
#endif
#endif /* not lint */

/*
 * tunefs: change layout parameters to an existing file system.
 */
#include <sys/param.h>

#include <ufs/ufs/dinode.h>
#include <ufs/ffs/fs.h>
#include <ufs/ffs/ffs_extern.h>

#include <machine/bswap.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <fstab.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

/* the optimization warning string template */
#define	OPTWARN	"should optimize for %s with minfree %s %d%%"

union {
	struct	fs sb;
	char pad[MAXBSIZE];
} sbun;
#define	sblock sbun.sb
char buf[MAXBSIZE];

int	fi;
long	dev_bsize = 1;
int	needswap = 0;

static	void	bwrite(daddr_t, char *, int, const char *);
static	void	bread(daddr_t, char *, int, const char *);
static	int	getnum(const char *, const char *, int, int);
static	void	getsb(struct fs *, const char *);
static	void	usage(void);
int		main(int, char *[]);

int
main(int argc, char *argv[])
{
#define	OPTSTRINGBASE	"AFNa:d:e:g:h:k:m:o:t:"
#ifdef TUNEFS_SOFTDEP
	int		softdep;
#define	OPTSTRING	OPTSTRINGBASE ## "n:"
#else
#define	OPTSTRING	OPTSTRINGBASE
#endif
	int		i, ch, Aflag, Fflag, Nflag, openflags;
	struct fstab	*fs;
	const char	*special, *chg[2];
	char		device[MAXPATHLEN], rawspec[MAXPATHLEN], *p;
	int		maxbpg, maxcontig, minfree, rotdelay, optim, trackskew;
	int		avgfilesize, avgfpdir;

	Aflag = Fflag = Nflag = 0;
	maxbpg = maxcontig = minfree = rotdelay = optim = trackskew = -1;
	avgfilesize = avgfpdir = -1;
#ifdef TUNEFS_SOFTDEP
	softdep = -1;
#endif
	chg[FS_OPTSPACE] = "space";
	chg[FS_OPTTIME] = "time";

	while ((ch = getopt(argc, argv, OPTSTRING)) != -1) {
		switch (ch) {

		case 'A':
			Aflag++;
			break;

		case 'F':
			Fflag++;
			break;

		case 'N':
			Nflag++;
			break;

		case 'a':
			maxcontig = getnum(optarg,
			    "maximum contiguous block count", 1, INT_MAX);
			break;

		case 'd':
			rotdelay = getnum(optarg,
			    "rotational delay between contiguous blocks",
			    0, INT_MAX);
			break;

		case 'e':
			maxbpg = getnum(optarg,
			    "maximum blocks per file in a cylinder group",
			    1, INT_MAX);
			break;

		case 'g':
			avgfilesize = getnum(optarg,
			    "average file size", 1, INT_MAX);
			break;

		case 'h':
			avgfpdir = getnum(optarg,
			    "expected number of files per directory",
			    1, INT_MAX);
			break;

		case 'm':
			minfree = getnum(optarg,
			    "minimum percentage of free space", 0, 99);
			break;

#ifdef TUNEFS_SOFTDEP
		case 'n':
			if (strcmp(optarg, "enable") == 0)
				softdep = 1;
			else if (strcmp(optarg, "disable") == 0)
				softdep = 0;
			else {
				errx(10, "bad soft dependencies "
					"(options are `enable' or `disable')");
			}
			break;
#endif

		case 'o':
			if (strcmp(optarg, chg[FS_OPTSPACE]) == 0)
				optim = FS_OPTSPACE;
			else if (strcmp(optarg, chg[FS_OPTTIME]) == 0)
				optim = FS_OPTTIME;
			else
				errx(10,
				    "bad %s (options are `space' or `time')",
				    "optimization preference");
			break;

		case 'k':
		case 't':	/* for compatibility with old syntax */
			trackskew = getnum(optarg,
			    "track skew in sectors", 0, INT_MAX);
			break;

		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind; 
	if (argc != 1)
		usage();

	special = argv[0];
	openflags = Nflag ? O_RDONLY : O_RDWR;
	if (Fflag) {
		fi = open(special, openflags);
		if (fi == -1)
			err(1, "%s", special);
	} else {
		fs = getfsfile(special);
		if (fs) {
			if ((p = strrchr(fs->fs_spec, '/')) != NULL) {
				snprintf(rawspec, sizeof(rawspec), "%.*s/r%s",
				    (int)(p - fs->fs_spec), fs->fs_spec, p + 1);
				special = rawspec;
			} else
				special = fs->fs_spec;
		}
		fi = opendisk(special, openflags, device, sizeof(device), 0);
		if (fi == -1)
			err(1, "%s", errno == ENOENT ? special : device);
		special = device;
	}
	getsb(&sblock, special);

#define CHANGEVAL(old, new, type, suffix) do				\
	if ((new) != -1) {						\
		if ((new) == (old))					\
			warnx("%s remains unchanged at %d%s",		\
			    (type), (old), (suffix));			\
		else {							\
			warnx("%s changes from %d%s to %d%s",		\
			    (type), (old), (suffix), (new), (suffix));	\
			(old) = (new);					\
		}							\
	} while (/* CONSTCOND */0)

	warnx("tuning %s", special);
	CHANGEVAL(sblock.fs_maxcontig, maxcontig,
	    "maximum contiguous block count", "");
	CHANGEVAL(sblock.fs_rotdelay, rotdelay,
	    "rotational delay between contiguous blocks", "ms");
	CHANGEVAL(sblock.fs_maxbpg, maxbpg,
	    "maximum blocks per file in a cylinder group", "");
	CHANGEVAL(sblock.fs_minfree, minfree,
	    "minimum percentage of free space", "%");
	if (minfree != -1) {
		if (minfree >= MINFREE &&
		    sblock.fs_optim == FS_OPTSPACE)
			warnx(OPTWARN, "time", ">=", MINFREE);
		if (minfree < MINFREE &&
		    sblock.fs_optim == FS_OPTTIME)
			warnx(OPTWARN, "space", "<", MINFREE);
	}
#ifdef TUNEFS_SOFTDEP
	if (softdep == 1) {
		sblock.fs_flags |= FS_DOSOFTDEP;
		warnx("soft dependencies set");
	} else if (softdep == 0) {
		sblock.fs_flags &= ~FS_DOSOFTDEP;
		warnx("soft dependencies cleared");
	}
#endif
	if (optim != -1) {
		if (sblock.fs_optim == optim) {
			warnx("%s remains unchanged as %s",
			    "optimization preference",
			    chg[optim]);
		} else {
			warnx("%s changes from %s to %s",
			    "optimization preference",
			    chg[sblock.fs_optim], chg[optim]);
			sblock.fs_optim = optim;
			if (sblock.fs_minfree >= MINFREE &&
			    optim == FS_OPTSPACE)
				warnx(OPTWARN, "time", ">=", MINFREE);
			if (sblock.fs_minfree < MINFREE &&
			    optim == FS_OPTTIME)
				warnx(OPTWARN, "space", "<", MINFREE);
		}
	}
	CHANGEVAL(sblock.fs_trackskew, trackskew,
	    "track skew in sectors", "");
	CHANGEVAL(sblock.fs_avgfilesize, avgfilesize,
	    "average file size", "");
	CHANGEVAL(sblock.fs_avgfpdir, avgfpdir,
	    "expected number of files per directory", "");

	if (Nflag) {
		fprintf(stdout, "tunefs: current settings of %s\n", special);
		fprintf(stdout, "\tmaximum contiguous block count %d\n",
		    sblock.fs_maxcontig);
		fprintf(stdout,
		    "\trotational delay between contiguous blocks %dms\n",
		    sblock.fs_rotdelay);
		fprintf(stdout,
		    "\tmaximum blocks per file in a cylinder group %d\n",
		    sblock.fs_maxbpg);
		fprintf(stdout, "\tminimum percentage of free space %d%%\n",
		    sblock.fs_minfree);
#ifdef TUNEFS_SOFTDEP
		fprintf(stdout, "\tsoft dependencies: %s\n",
		    (sblock.fs_flags & FS_DOSOFTDEP) ? "on" : "off");
#endif
		fprintf(stdout, "\toptimization preference: %s\n",
		    chg[sblock.fs_optim]);
		fprintf(stdout, "\taverage file size: %d\n",
		    sblock.fs_avgfilesize);
		fprintf(stdout,
		    "\texpected number of files per directory: %d\n",
		    sblock.fs_avgfpdir);
		fprintf(stdout, "\ttrack skew %d sectors\n",
		    sblock.fs_trackskew);
		fprintf(stdout, "tunefs: no changes made\n");
		exit(0);
	}

	memcpy(buf, (char *)&sblock, SBSIZE);
	if (needswap)
		ffs_sb_swap((struct fs*)buf, (struct fs*)buf);
	bwrite((daddr_t)SBOFF / dev_bsize, buf, SBSIZE, special);
	if (Aflag)
		for (i = 0; i < sblock.fs_ncg; i++)
			bwrite(fsbtodb(&sblock, cgsblock(&sblock, i)),
			    buf, SBSIZE, special);
	close(fi);
	exit(0);
}

static int
getnum(const char *num, const char *desc, int min, int max)
{
	long	n;
	char	*ep;

	n = strtol(num, &ep, 10);
	if (ep[0] != '\0')
		errx(1, "Invalid number `%s' for %s", num, desc);
	if ((int) n < min)
		errx(1, "%s `%s' too small (minimum is %d)", desc, num, min);
	if ((int) n > max)
		errx(1, "%s `%s' too large (maximum is %d)", desc, num, max);
	return ((int)n);
}

static void
usage(void)
{

	fprintf(stderr, "Usage: tunefs [-AFN] tuneup-options special-device\n");
	fprintf(stderr, "where tuneup-options are:\n");
	fprintf(stderr, "\t-a maximum contiguous blocks\n");
	fprintf(stderr, "\t-d rotational delay between contiguous blocks\n");
	fprintf(stderr, "\t-e maximum blocks per file in a cylinder group\n");
	fprintf(stderr, "\t-g average file size\n");
	fprintf(stderr, "\t-h expected number of files per directory\n");
	fprintf(stderr, "\t-k track skew in sectors\n");
	fprintf(stderr, "\t-m minimum percentage of free space\n");
#ifdef TUNEFS_SOFTDEP
	fprintf(stderr, "\t-n soft dependencies (`enable' or `disable')\n");
#endif
	fprintf(stderr, "\t-o optimization preference (`space' or `time')\n");
	exit(2);
}

static void
getsb(struct fs *fs, const char *file)
{

	bread((daddr_t)SBOFF, (char *)fs, SBSIZE, file);
	if (fs->fs_magic != FS_MAGIC) {
		if (fs->fs_magic == bswap32(FS_MAGIC)) {
			warnx("%s: swapping byte order", file);
			needswap = 1;
			ffs_sb_swap(fs, fs);
		} else
			err(5, "%s: bad magic number", file);
	}
	dev_bsize = fs->fs_fsize / fsbtodb(fs, 1);
}

static void
bwrite(daddr_t blk, char *buffer, int size, const char *file)
{
	off_t	offset;

	offset = (off_t)blk * dev_bsize;
	if (lseek(fi, offset, SEEK_SET) == -1)
		err(6, "%s: seeking to %lld", file, (long long)offset);
	if (write(fi, buffer, size) != size)
		err(7, "%s: writing %d bytes", file, size);
}

static void
bread(daddr_t blk, char *buffer, int cnt, const char *file)
{
	off_t	offset;
	int	i;

	offset = (off_t)blk * dev_bsize;
	if (lseek(fi, offset, SEEK_SET) == -1)
		err(4, "%s: seeking to %lld", file, (long long)offset);
	if ((i = read(fi, buffer, cnt)) != cnt)
		errx(5, "%s: short read", file);
}
