/*	$NetBSD: df.c,v 1.49 2003/08/07 09:05:11 agc Exp $	*/

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
__RCSID("$NetBSD: df.c,v 1.49 2003/08/07 09:05:11 agc Exp $");
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
void	 prtstat(struct statfs *, int);
int	 selected(const char *);
void	 maketypelist(char *);
long	 regetmntinfo(struct statfs **, long);
void	 usage(void);
void	 prthumanval(int64_t, char *);
void	 prthuman(struct statfs *, long);

int	aflag, gflag, hflag, iflag, kflag, lflag, mflag, nflag, Pflag;
char	**typelist = NULL;

int
main(int argc, char *argv[])
{
	struct stat stbuf;
	struct statfs *mntbuf;
	long mntsize;
	int ch, i, maxwidth, width;
	char *mntpt;

	while ((ch = getopt(argc, argv, "aghiklmnPt:")) != -1)
		switch (ch) {
		case 'a':
			aflag = 1;
			break;
		case 'g':
			gflag = 1;
			break;
		case 'h':
			hflag = 1;
			break;
		case 'i':
			iflag = 1;
			break;
		case 'k':
			kflag = 1;
			break;
		case 'l':
			lflag = 1;
			break;
		case 'm':
			mflag = 1;
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
	argc -= optind;
	argv += optind;

	mntsize = getmntinfo(&mntbuf, MNT_NOWAIT);
	if (mntsize == 0)
		err(1, "retrieving information on mounted file systems");

	if (*argv == NULL) {
		mntsize = regetmntinfo(&mntbuf, mntsize);
	} else {
		mntbuf = malloc(argc * sizeof(struct statfs));
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
			if (!statfs(mntpt, &mntbuf[mntsize]))
				if (lflag &&
				    (mntbuf[mntsize].f_flags & MNT_LOCAL) == 0)
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
	struct statfs *mntbuf;

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
 * current (not cached) info.  Returns the new count of valid statfs bufs.
 */
long
regetmntinfo(struct statfs **mntbufp, long mntsize)
{
	int i, j;
	struct statfs *mntbuf;

	if (!lflag && typelist == NULL && aflag)
		return (nflag ? mntsize : getmntinfo(mntbufp, MNT_WAIT));

	mntbuf = *mntbufp;
	j = 0;
	for (i = 0; i < mntsize; i++) {
		if (!aflag && (mntbuf[i].f_flags & MNT_IGNORE) != 0)
			continue;
		if (lflag && (mntbuf[i].f_flags & MNT_LOCAL) == 0)
			continue;
		if (!selected(mntbuf[i].f_fstypename))
			continue;
		if (nflag)
			mntbuf[j] = mntbuf[i];
		else {
			struct statfs layerbuf = mntbuf[i];
			(void)statfs(mntbuf[i].f_mntonname, &mntbuf[j]);
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
prthumanval(int64_t bytes, char *pad)
{
	char buf[5];

	humanize_number(buf, sizeof(buf), bytes, "", HN_AUTOSCALE,
	    HN_B | HN_NOSPACE | HN_DECIMAL);

	(void)printf("%s %6s", pad, buf);
}

void
prthuman(struct statfs *sfsp, long used)
{
       prthumanval((int64_t)(sfsp->f_blocks) * (int64_t)(sfsp->f_bsize), "");
       prthumanval((int64_t)(used) * (int64_t)(sfsp->f_bsize), "  ");
       prthumanval((int64_t)(sfsp->f_bavail) * (int64_t)(sfsp->f_bsize), "   ");
}


/*
 * Convert statfs returned filesystem size into BLOCKSIZE units.
 * Attempts to avoid overflow for large filesystems.
 */
#define fsbtoblk(num, fsbs, bs)					\
	(((fsbs) != 0 && (fsbs) < (bs)) ?			\
	    (num) / ((bs) / (fsbs)) : (num) * ((fsbs) / (bs)))

/*
 * Print out status about a filesystem.
 */
void
prtstat(struct statfs *sfsp, int maxwidth)
{
	static long blocksize;
	static int headerlen, timesthrough;
	static char *header;
	long used, availblks, inodes;
	static char *full = "100%";

	if (maxwidth < 11)
		maxwidth = 11;
	if (++timesthrough == 1) {
		if (kflag) {
			blocksize = 1024;
			header = Pflag ? "1024-blocks" : "1K-blocks";
			headerlen = strlen(header);
		} else if (mflag) {
			blocksize = 1024 * 1024;
			header = Pflag ? "1048576-blocks" : "1M-blocks";
			headerlen = strlen(header);
		} else if (gflag) {
			blocksize = 1024 * 1024 * 1024;
			header = Pflag ? "1073741824-blocks" : "1G-blocks";
			headerlen = strlen(header);
		} else if (hflag) {
			header = "  Size";
			headerlen = strlen(header);
		} else
			header = getbsize(&headerlen, &blocksize);
		(void)printf("%-*.*s %s     Used %9s Capacity",
		    maxwidth, maxwidth, "Filesystem", header,
		    Pflag ? "Available" : "Avail");
		if (iflag)
			(void)printf("  iused    ifree  %%iused");
		(void)printf("  Mounted on\n");
	}
	(void)printf("%-*.*s", maxwidth, maxwidth, sfsp->f_mntfromname);
	used = sfsp->f_blocks - sfsp->f_bfree;
	availblks = sfsp->f_bavail + used;
	if (hflag)
		prthuman(sfsp, used);
	else
		(void)printf(" %*ld %8ld %9ld", headerlen,
		    fsbtoblk(sfsp->f_blocks, sfsp->f_bsize, blocksize),
		    fsbtoblk(used, sfsp->f_bsize, blocksize),
		    fsbtoblk(sfsp->f_bavail, sfsp->f_bsize, blocksize));
	(void)printf("%7s",
	    availblks == 0 ? full : strpct((u_long)used, (u_long)availblks, 0));
	if (iflag) {
		inodes = sfsp->f_files;
		used = inodes - sfsp->f_ffree;
		(void)printf(" %8ld %8ld %6s ", used, sfsp->f_ffree,
		    inodes == 0 ? full :
		    strpct((u_long)used, (u_long)inodes, 0));
	} else
		(void)printf("  ");
	(void)printf("  %s\n", sfsp->f_mntonname);
}

void
usage(void)
{

	(void)fprintf(stderr,
	    "Usage: %s [-aghiklmnP] [-t type] [file | file_system ...]\n",
	    getprogname());
	exit(1);
	/* NOTREACHED */
}
