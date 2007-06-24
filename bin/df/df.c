/*	$NetBSD: df.c,v 1.73 2007/06/24 01:52:46 christos Exp $ */

/*
 * Copyright (c) 1980, 1990, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
__COPYRIGHT(
"@(#) Copyright (c) 1980, 1990, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)df.c	8.7 (Berkeley) 4/2/94";
#else
__RCSID("$NetBSD: df.c,v 1.73 2007/06/24 01:52:46 christos Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/mount.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <util.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern char *strpct(u_long, u_long, u_int);

int	 main(int, char *[]);
int	 bread(off_t, void *, int);
char	*getmntpt(char *);
void	 prtstat(struct statvfs *, int);
int	 selected(const char *);
void	 maketypelist(char *);
long	 regetmntinfo(struct statvfs **, long);
void	 usage(void);
void	 prthumanval(int64_t, const char *);
void	 prthuman(struct statvfs *, int64_t, int64_t);
const char *
	strpct64(uint64_t, uint64_t, u_int);

int	aflag, gflag, hflag, iflag, lflag, nflag, Pflag;
long	usize = 0;
char	**typelist = NULL;

int
main(int argc, char *argv[])
{
	struct stat stbuf;
	struct statvfs *mntbuf;
	long mntsize;
	int ch, i, maxwidth, width;
	char *mntpt;

	while ((ch = getopt(argc, argv, "aGghiklmnPt:")) != -1)
		switch (ch) {
		case 'a':
			aflag = 1;
			break;
		case 'G':
			hflag = 0;
			usize = 1024 * 1024 * 1024;
			break;
		case 'g':
			gflag = 1;
			break;
		case 'h':
			hflag = 1;
			usize = 0;
			break;
		case 'i':
			iflag = 1;
			break;
		case 'k':
			hflag = 0;
			usize = 1024;
			break;
		case 'l':
			lflag = 1;
			break;
		case 'm':
			hflag = 0;
			usize = 1024 * 1024;
			break;
		case 'n':
			nflag = 1;
			break;
		case 'P':
			Pflag = 1;
			break;
		case 't':
			if (typelist != NULL)
				errx(1, "only one -t option may be specified.");
			maketypelist(optarg);
			break;
		case '?':
		default:
			usage();
		}

	if (gflag && (Pflag || iflag))
		errx(1, "only one of -g and -P or -i may be specified");
	if (Pflag && iflag)
		errx(1, "only one of -P and -i may be specified");

	argc -= optind;
	argv += optind;

	mntsize = getmntinfo(&mntbuf, MNT_NOWAIT);
	if (mntsize == 0)
		err(1, "retrieving information on mounted file systems");

	if (*argv == NULL) {
		mntsize = regetmntinfo(&mntbuf, mntsize);
	} else {
		if ((mntbuf = malloc(argc * sizeof(*mntbuf))) == NULL)
			err(1, "can't allocate statvfs array");
		mntsize = 0;
		for (; *argv != NULL; argv++) {
			if (stat(*argv, &stbuf) < 0) {
				if ((mntpt = getmntpt(*argv)) == 0) {
					warn("%s", *argv);
					continue;
				}
			} else if (S_ISBLK(stbuf.st_mode)) {
				if ((mntpt = getmntpt(*argv)) == 0)
					mntpt = *argv;
			} else
				mntpt = *argv;
			/*
			 * Statfs does not take a `wait' flag, so we cannot
			 * implement nflag here.
			 */
			if (!statvfs(mntpt, &mntbuf[mntsize]))
				if (lflag &&
				    (mntbuf[mntsize].f_flag & MNT_LOCAL) == 0)
					warnx("Warning: %s is not a local %s",
					    *argv, "file system");
				else if
				    (!selected(mntbuf[mntsize].f_fstypename))
					warnx("Warning: %s mounted as a %s %s",
					    *argv,
					    mntbuf[mntsize].f_fstypename,
					    "file system");
				else
					++mntsize;
			else
				warn("%s", *argv);
		}
	}

	maxwidth = 0;
	for (i = 0; i < mntsize; i++) {
		width = strlen(mntbuf[i].f_mntfromname);
		if (width > maxwidth)
			maxwidth = width;
	}
	for (i = 0; i < mntsize; i++)
		prtstat(&mntbuf[i], maxwidth);
	exit(0);
	/* NOTREACHED */
}

char *
getmntpt(char *name)
{
	long mntsize, i;
	struct statvfs *mntbuf;

	mntsize = getmntinfo(&mntbuf, MNT_NOWAIT);
	for (i = 0; i < mntsize; i++) {
		if (!strcmp(mntbuf[i].f_mntfromname, name))
			return (mntbuf[i].f_mntonname);
	}
	return (0);
}

static enum { IN_LIST, NOT_IN_LIST } which;

int
selected(const char *type)
{
	char **av;

	/* If no type specified, it's always selected. */
	if (typelist == NULL)
		return (1);
	for (av = typelist; *av != NULL; ++av)
		if (!strncmp(type, *av, MFSNAMELEN))
			return (which == IN_LIST ? 1 : 0);
	return (which == IN_LIST ? 0 : 1);
}

void
maketypelist(char *fslist)
{
	int i;
	char *nextcp, **av;

	if ((fslist == NULL) || (fslist[0] == '\0'))
		errx(1, "empty type list");

	/*
	 * XXX
	 * Note: the syntax is "noxxx,yyy" for no xxx's and
	 * no yyy's, not the more intuitive "noyyy,noyyy".
	 */
	if (fslist[0] == 'n' && fslist[1] == 'o') {
		fslist += 2;
		which = NOT_IN_LIST;
	} else
		which = IN_LIST;

	/* Count the number of types. */
	for (i = 1, nextcp = fslist;
	    (nextcp = strchr(nextcp, ',')) != NULL; i++)
		++nextcp;

	/* Build an array of that many types. */
	if ((av = typelist = malloc((i + 1) * sizeof(char *))) == NULL)
		err(1, "can't allocate type array");
	av[0] = fslist;
	for (i = 1, nextcp = fslist;
	    (nextcp = strchr(nextcp, ',')) != NULL; i++) {
		*nextcp = '\0';
		av[i] = ++nextcp;
	}
	/* Terminate the array. */
	av[i] = NULL;
}

/*
 * Make a pass over the filesystem info in ``mntbuf'' filtering out
 * filesystem types not in ``fsmask'' and possibly re-stating to get
 * current (not cached) info.  Returns the new count of valid statvfs bufs.
 */
long
regetmntinfo(struct statvfs **mntbufp, long mntsize)
{
	int i, j;
	struct statvfs *mntbuf;

	if (!lflag && typelist == NULL && aflag)
		return (nflag ? mntsize : getmntinfo(mntbufp, MNT_WAIT));

	mntbuf = *mntbufp;
	j = 0;
	for (i = 0; i < mntsize; i++) {
		if (!aflag && (mntbuf[i].f_flag & MNT_IGNORE) != 0)
			continue;
		if (lflag && (mntbuf[i].f_flag & MNT_LOCAL) == 0)
			continue;
		if (!selected(mntbuf[i].f_fstypename))
			continue;
		if (nflag)
			mntbuf[j] = mntbuf[i];
		else {
			struct statvfs layerbuf = mntbuf[i];
			(void)statvfs(mntbuf[i].f_mntonname, &mntbuf[j]);
			/*
			 * If the FS name changed, then new data is for
			 * a different layer and we don't want it.
			 */
			if (memcmp(layerbuf.f_mntfromname,
			    mntbuf[j].f_mntfromname, MNAMELEN))
				mntbuf[j] = layerbuf;
		}
		j++;
	}
	return (j);
}

void
prthumanval(int64_t bytes, const char *pad)
{
	char buf[6];

	humanize_number(buf, sizeof(buf) - (bytes < 0 ? 0 : 1),
	    bytes, "", HN_AUTOSCALE,
	    HN_B | HN_NOSPACE | HN_DECIMAL);

	(void)printf("%s %6s", pad, buf);
}

void
prthuman(struct statvfs *sfsp, int64_t used, int64_t bavail)
{

	prthumanval(sfsp->f_blocks * sfsp->f_frsize, "");
	prthumanval(used * sfsp->f_frsize, "   ");
	prthumanval(bavail * sfsp->f_frsize, "	 ");
}

/*
 * Convert statvfs returned filesystem size into BLOCKSIZE units.
 * Attempts to avoid overflow for large filesystems.
 */
#define fsbtoblk(num, fsbs, bs)					\
	(((fsbs) != 0 && (fsbs) < (bs)) ?			\
	    (int64_t)(num) / (int64_t)((bs) / (fsbs)) :		\
	    (int64_t)(num) * ((fsbs) / (bs)))

/*
 * Print out status about a filesystem.
 */
void
prtstat(struct statvfs *sfsp, int maxwidth)
{
	static long blocksize;
	static int headerlen, timesthrough;
	static const char *header;
	static const char full[] = "100%";
	static const char empty[] = "  0%";
	int64_t used, availblks, inodes;
	int64_t bavail;

	if (gflag) {
		/*
		 * From SunOS-5.6:
		 *
		 * /var		      (/dev/dsk/c0t0d0s3 ):	    8192 block size	     1024 frag size  
		 *   984242 total blocks     860692 free blocks	  859708 available	   249984 total files
		 *   248691 free files	    8388611 filesys id	
		 *	ufs fstype	 0x00000004 flag	     255 filename length
		 *
		 */
		(void)printf("%10s (%-12s): %7ld block size %12ld frag size\n",
		    sfsp->f_mntonname, sfsp->f_mntfromname,
		    sfsp->f_iosize,	/* On UFS/FFS systems this is
					 * also called the "optimal
					 * transfer block size" but it
					 * is of course the file
					 * system's block size too.
					 */
		    sfsp->f_bsize);	/* not so surprisingly the
					 * "fundamental file system
					 * block size" is the frag
					 * size.
					 */
		(void)printf("%10" PRId64 " total blocks %10" PRId64 
		    " free blocks  %10" PRId64 " available\n",
		    (uint64_t)sfsp->f_blocks, (uint64_t)sfsp->f_bfree,
		    (uint64_t)sfsp->f_bavail);
		(void)printf("%10" PRId64 " total files	 %10" PRId64
		    " free files %12lx filesys id\n",
		    (uint64_t)sfsp->f_ffree, (uint64_t)sfsp->f_files,
		    sfsp->f_fsid);
		(void)printf("%10s fstype  %#15lx flag	%17ld filename "
		    "length\n", sfsp->f_fstypename, sfsp->f_flag,
		    sfsp->f_namemax);
		(void)printf("%10lu owner %17" PRId64 " syncwrites %12" PRId64
		    " asyncwrites\n\n", (unsigned long)sfsp->f_owner,
		    sfsp->f_syncwrites, sfsp->f_asyncwrites);

		/*
		 * a concession by the structured programming police to the
		 * indentation police....
		 */
		return;
	}
	if (maxwidth < 12)
		maxwidth = 12;
	if (++timesthrough == 1) {
		switch (blocksize = usize) {
		case 1024:
			header = Pflag ? "1024-blocks" : "1K-blocks";
			headerlen = strlen(header);
			break;
		case 1024 * 1024:
			header = Pflag ? "1048576-blocks" : "1M-blocks";
			headerlen = strlen(header);
			break;
		case 1024 * 1024 * 1024:
			header = Pflag ? "1073741824-blocks" : "1G-blocks";
			headerlen = strlen(header);
			break;
		default:
			if (hflag) {
				header = "  Size";
				headerlen = strlen(header);
			} else
				header = getbsize(&headerlen, &blocksize);
			break;
		}
		if (Pflag) {
			/*
			 * "Filesystem {1024,512}-blocks Used Available 
			 * Capacity Mounted on\n"
			 */
			(void)printf("Filesystem %s Used Available Capacity "
			    "Mounted on\n", header);
		} else {
			(void)printf("%-*.*s %s	      Used	Avail %%Cap",
			    maxwidth - (headerlen - 9),
			    maxwidth - (headerlen - 9),
			    "Filesystem", header);
			if (iflag)
				(void)printf("	  iUsed	 iAvail %%iCap");
			(void)printf(" Mounted on\n");
		}
	}
	used = sfsp->f_blocks - sfsp->f_bfree;
	bavail = sfsp->f_bfree - sfsp->f_bresvd;
	availblks = bavail + used;
	if (Pflag) {
		/*
		 * "%s %d %d %d %s %s\n", <file system name>, <total space>,
		 * <space used>, <space free>, <percentage used>,
		 * <file system root>		   
		 */
		(void)printf("%s %" PRId64 " %" PRId64 " %" PRId64 " %s %s\n",
		    sfsp->f_mntfromname,
		    fsbtoblk(sfsp->f_blocks, sfsp->f_bsize, blocksize),
		    fsbtoblk(used, sfsp->f_bsize, blocksize),
		    fsbtoblk(sfsp->f_bavail, sfsp->f_bsize, blocksize),
		    availblks == 0 ? full : strpct64((uint64_t) used,
		    (uint64_t) availblks, 0), sfsp->f_mntonname);
		/*
		 * another concession by the structured programming police to
		 * the indentation police....
		 *
		 * Note iflag cannot be set when Pflag is set.
		 */
		return;
	}

	(void)printf("%-*.*s", maxwidth, maxwidth, sfsp->f_mntfromname);

	if (hflag)
		prthuman(sfsp, used, bavail);
	else
		(void)printf("%10" PRId64 " %10" PRId64 " %10" PRId64,
		    fsbtoblk(sfsp->f_blocks, sfsp->f_frsize, blocksize),
		    fsbtoblk(used, sfsp->f_frsize, blocksize),
		    fsbtoblk(bavail, sfsp->f_frsize, blocksize));
	(void)printf(" %4s",
	    availblks == 0 ? full :
	    /* We know that these values are never negative */
	    strpct64((uint64_t)used, (uint64_t)availblks, 0));
	if (iflag) {
		inodes = sfsp->f_files;
		used = inodes - sfsp->f_ffree;
		(void)printf(" %8ld %8ld %4s",
		    (u_long)used, (u_long)sfsp->f_ffree,
		    inodes == 0 ? (used == 0 ? empty : full) :
		    strpct64((uint64_t)used, (uint64_t)inodes, 0));
	}
	(void)printf(" %s\n", sfsp->f_mntonname);
}

void
usage(void)
{

	(void)fprintf(stderr,
	    "Usage: %s [-aGghklmn] [-i|-p] [-t type] [file | "
	    "file_system ...]\n",
	    getprogname());
	exit(1);
	/* NOTREACHED */
}

const char *
strpct64(uint64_t numerator, uint64_t denominator, u_int digits)
{

	while (denominator > ULONG_MAX) {
		numerator >>= 1;
		denominator >>= 1;
	}
	return (strpct((u_long)numerator, (u_long)denominator, digits));
}
