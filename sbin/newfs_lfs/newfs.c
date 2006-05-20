/*	$NetBSD: newfs.c,v 1.15.2.2 2006/05/20 22:46:22 riz Exp $	*/

/*-
 * Copyright (c) 1989, 1992, 1993
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
__COPYRIGHT("@(#) Copyright (c) 1989, 1992, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)newfs.c	8.5 (Berkeley) 5/24/95";
#else
__RCSID("$NetBSD: newfs.c,v 1.15.2.2 2006/05/20 22:46:22 riz Exp $");
#endif
#endif /* not lint */

/*
 * newfs: friendly front end to mkfs
 */
#include <sys/param.h>
#include <sys/ucred.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/disklabel.h>
#include <sys/file.h>
#include <sys/mount.h>
#include <sys/sysctl.h>
#include <sys/time.h>

#include <ufs/ufs/dir.h>
#include <ufs/ufs/dinode.h>
#include <ufs/lfs/lfs.h>

#include <disktab.h>
#include <err.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <paths.h>
#include <util.h>
#include "config.h"
#include "extern.h"
#include "bufcache.h"

#define	COMPAT			/* allow non-labeled disks */

int	Nflag = 0;		/* run without writing file system */
int	fssize;			/* file system size */
int	sectorsize;		/* bytes/sector */
int	fsize = 0;		/* fragment size */
int	bsize = 0;		/* block size */
int	ibsize = 0;		/* inode block size */
int	interleave = 0;		/* segment interleave */
int	minfree = MINFREE;	/* free space threshold */
int     minfreeseg = 0;         /* segments not counted in bfree total */
int     resvseg = 0;            /* free segments reserved for the cleaner */
u_int32_t roll_id = 0;		/* roll-forward id */
u_long	memleft;		/* virtual memory available */
caddr_t	membase;		/* start address of memory based filesystem */
#ifdef COMPAT
char	*disktype;
int	unlabeled;
#endif
int	preen = 0;		/* Coexistence with fsck_lfs */

char	device[MAXPATHLEN];
char	*progname, *special;

static struct disklabel *getdisklabel(char *, int);
static struct disklabel *debug_readlabel(int);
#ifdef notdef
static void rewritelabel(char *, int, struct disklabel *);
#endif
static int64_t strsuftoi64(const char *, const char *, int64_t, int64_t, int *);
static void usage(void);

/* CHUNKSIZE should be larger than MAXPHYS */
#define CHUNKSIZE (1024 * 1024)

static size_t
auto_segsize(int fd, off_t len, int version)
{
	off_t off, bw;
	time_t start, finish;
	char buf[CHUNKSIZE];
	long seeks;
	size_t final;
	int i;
	
	/* First, get sequential access bandwidth */
	time(&start);
	finish = start;
	for (off = 0; finish - start < 10; off += CHUNKSIZE) {
		if (pread(fd, buf, CHUNKSIZE, off) < 0)
			break;
		time(&finish);
	}
	/* Bandwidth = bytes / sec */
	/* printf("%ld bytes in %ld seconds\n", (long)off, (long)(finish - start)); */
	bw = off / (finish - start);

	/* Second, seek time */
	time(&start);
	finish = start; /* structure copy */
	for (seeks = 0; finish - start < 10; ) {
		off = (((double)rand()) * len) / (off_t)RAND_MAX;
		if (pread(fd, buf, dbtob(1), off) < 0)
			break;
		time(&finish);
		++seeks;
	}
	/* printf("%ld seeks in %ld seconds\n", (long)seeks, (long)(finish - start)); */
	/* Seek time in units/sec */
	seeks /= (finish - start);
	if (seeks == 0)
		seeks = 1;

	printf("bw = %ld B/s, seek time %ld ms (%ld seeks/s)\n",
		(long)bw, 1000/seeks, seeks);
	final = dbtob(btodb(4 * bw / seeks));
	if (version == 1) {
		for (i = 0; final; final >>= 1, i++)
			;
		final = 1 << i;
	}
	printf("using initial segment size %ld\n", (long)final);
	return final;
}

int
main(int argc, char **argv)
{
	int version, ch;
	struct partition *pp;
	struct disklabel *lp;
	struct stat st;
	int debug, force, fsi, fso, segsize, maxpartitions;
	uint secsize = 0;
	daddr_t start;
	char *cp, *opstring;
	int byte_sized = 0;
	int r;

	version = DFL_VERSION;		/* what version of lfs to make */

	if ((progname = strrchr(*argv, '/')) != NULL)
		++progname;
	else
		progname = *argv;

	maxpartitions = getmaxpartitions();
	if (maxpartitions > 26)
		fatal("insane maxpartitions value %d", maxpartitions);

	opstring = "AB:b:DFf:I:i:LM:m:NO:R:r:S:s:v:";

	debug = force = segsize = start = 0;
	while ((ch = getopt(argc, argv, opstring)) != -1)
		switch(ch) {
		case 'A':	/* Adaptively configure segment size */
			segsize = -1;
			break;
		case 'B':	/* LFS segment size */
		        segsize = strsuftoi64("segment size", optarg, LFS_MINSEGSIZE, INT64_MAX, NULL);
			break;
		case 'D':
			debug = 1;
			break;
		case 'F':
			force = 1;
			break;
		case 'I':
		        interleave = strsuftoi64("interleave", optarg, 0, INT64_MAX, NULL);
			break;
		case 'L':	/* Compatibility only */
			break;
		case 'M':
		  	minfreeseg = strsuftoi64("minfreeseg", optarg, 0, INT64_MAX, NULL);
			break;
		case 'N':
			Nflag++;
			break;
		case 'O':
		  	start = strsuftoi64("start", optarg, 0, INT64_MAX, NULL);
			break;
		case 'R':
		  	resvseg = strsuftoi64("resvseg", optarg, 0, INT64_MAX, NULL);
			break;
		case 'S':
		  	secsize = strsuftoi64("sector size", optarg, 1, INT64_MAX, NULL);
			if (secsize <= 0 || (secsize & (secsize - 1)))
				fatal("%s: bad sector size", optarg);
			break;
#ifdef COMPAT
		case 'T':
			disktype = optarg;
			break;  
#endif 
		case 'b':
		  	bsize = strsuftoi64("block size", optarg, LFS_MINBLOCKSIZE, INT64_MAX, NULL);
			break;
		case 'f':
		  	fsize = strsuftoi64("fragment size", optarg, LFS_MINBLOCKSIZE, INT64_MAX, NULL);
			break;
		case 'i':
		  	ibsize = strsuftoi64("inode block size", optarg, LFS_MINBLOCKSIZE, INT64_MAX, NULL);
			break;
		case 'm':
		  	minfree = strsuftoi64("free space %", optarg, 0, 99, NULL);
			break;
		case 'r':
		  	roll_id = strsuftoi64("roll-forward id", optarg, 1, UINT_MAX, NULL);
			break;
		case 's':
		        fssize = strsuftoi64("file system size", optarg, 0, INT64_MAX, &byte_sized);
			break;
		case 'v':
		        version = strsuftoi64("file system version", optarg, 1, LFS_VERSION, NULL);
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc != 2 && argc != 1)
		usage();

	/*
	 * If the -N flag isn't specified, open the output file.  If no path
	 * prefix, try /dev/r%s and then /dev/%s.
	 */
	special = argv[0];
	if (strchr(special, '/') == NULL) {
		(void)snprintf(device, sizeof(device), "%sr%s", _PATH_DEV,
		    special);
		if (stat(device, &st) == -1)
			(void)snprintf(device, sizeof(device), "%s%s",
			    _PATH_DEV, special);
		special = device;
	}
	if (!Nflag) {
		fso = open(special,
		    (debug ? O_CREAT : 0) | O_RDWR, DEFFILEMODE);
		if (fso < 0)
			fatal("%s: %s", special, strerror(errno));
	} else
		fso = -1;

	/* Open the input file. */
	fsi = open(special, O_RDONLY);
	if (fsi < 0)
		fatal("%s: %s", special, strerror(errno));
	if (fstat(fsi, &st) < 0)
		fatal("%s: %s", special, strerror(errno));


	if (!S_ISCHR(st.st_mode)) {
		if (debug) {
			lp = debug_readlabel(fsi);
			pp = &lp->d_partitions[0];
		} else {
			static struct partition dummy_pp;
			lp = NULL;
			pp = &dummy_pp;
			pp->p_fstype = FS_BSDLFS;
			if (secsize == 0)
				secsize = 512;
			pp->p_size = st.st_size / secsize;
		}
	} else {
		cp = strchr(argv[0], '\0') - 1;
		if (!debug
		    && ((*cp < 'a' || *cp > ('a' + maxpartitions - 1))
		    && !isdigit((unsigned char)*cp)))
			fatal("%s: can't figure out file system partition", argv[0]);

#ifdef COMPAT
		if (disktype == NULL)
			disktype = argv[1];
#endif
		lp = getdisklabel(special, fsi);

		if (isdigit((unsigned char)*cp))
			pp = &lp->d_partitions[0];
		else
			pp = &lp->d_partitions[*cp - 'a'];
		if (pp->p_size == 0)
			fatal("%s: `%c' partition is unavailable", argv[0], *cp);
	}

	if (secsize == 0)
		secsize = lp->d_secsize;

	/* From here on out fssize is in sectors */
	if (byte_sized) {
		fssize /= secsize;
	}

	/* If force, make the partition look like an LFS */
	if (force) {
		pp->p_fstype = FS_BSDLFS;
		pp->p_size = fssize;
		/* 0 means to use defaults */
		pp->p_fsize  = 0;
		pp->p_frag   = 0;
		pp->p_sgs    = 0;
	} else
		if (fssize != 0 && fssize < pp->p_size)
			pp->p_size = fssize;

	/* Try autoconfiguring segment size, if asked to */
	if (segsize == -1) {
		if (!S_ISCHR(st.st_mode)) {
			warnx("%s is not a character special device, ignoring -A", special);
			segsize = 0;
		} else
			segsize = auto_segsize(fsi, dbtob(pp->p_size), version);
	}

	/* If we're making a LFS, we break out here */
	r = make_lfs(fso, secsize, pp, minfree, bsize, fsize, segsize,
		      minfreeseg, resvseg, version, start, ibsize, interleave,
                      roll_id);
	if (debug)
		bufstats();
	exit(r);
}

#ifdef COMPAT
char lmsg[] = "%s: can't read disk label; disk type must be specified";
#else
char lmsg[] = "%s: can't read disk label";
#endif

static struct disklabel *
getdisklabel(char *s, int fd)
{
	static struct disklabel lab;

	if (ioctl(fd, DIOCGDINFO, (char *)&lab) < 0) {
#ifdef COMPAT
		if (disktype) {
			struct disklabel *lp;

			unlabeled++;
			lp = getdiskbyname(disktype);
			if (lp == NULL)
				fatal("%s: unknown disk type", disktype);
			return (lp);
		}
#endif
		(void)fprintf(stderr,
		    "%s: ioctl (GDINFO): %s\n", progname, strerror(errno));
		fatal(lmsg, s);
	}
	return (&lab);
}


static struct disklabel *
debug_readlabel(int fd)
{
	static struct disklabel lab;
	int n;

	if ((n = read(fd, &lab, sizeof(struct disklabel))) < 0)
		fatal("unable to read disk label: %s", strerror(errno));
	else if (n < sizeof(struct disklabel))
		fatal("short read of disklabel: %d of %ld bytes", n,
			(u_long) sizeof(struct disklabel));
	return(&lab);
}

#ifdef notdef
static void
rewritelabel(char *s, int fd, struct disklabel *lp)
{
#ifdef COMPAT
	if (unlabeled)
		return;
#endif
	lp->d_checksum = 0;
	lp->d_checksum = dkcksum(lp);
	if (ioctl(fd, DIOCWDINFO, (char *)lp) < 0) {
		(void)fprintf(stderr,
		    "%s: ioctl (WDINFO): %s\n", progname, strerror(errno));
		fatal("%s: can't rewrite disk label", s);
	}
#if __vax__
	if (lp->d_type == DTYPE_SMD && lp->d_flags & D_BADSECT) {
		int i;
		int cfd;
		daddr_t alt;
		off_t loff;
		char specname[64];
		char blk[1024];
		char *cp;

		/*
		 * Make name for 'c' partition.
		 */
		strlcpy(specname, s, sizeof(specname));
		cp = specname + strlen(specname) - 1;
		if (!isdigit(*cp))
			*cp = 'c';
		cfd = open(specname, O_WRONLY);
		if (cfd < 0)
			fatal("%s: %s", specname, strerror(errno));
		if ((loff = getlabeloffset()) < 0)
			fatal("getlabeloffset: %s", strerror(errno));
		memset(blk, 0, sizeof(blk));
		*(struct disklabel *)(blk + loff) = *lp;
		alt = lp->d_ncylinders * lp->d_secpercyl - lp->d_nsectors;
		for (i = 1; i < 11 && i < lp->d_nsectors; i += 2) {
			if (lseek(cfd, (off_t)((alt + i) * lp->d_secsize),
			    SEEK_SET) == -1)
				fatal("lseek to badsector area: %s",
				    strerror(errno));
			if (write(cfd, blk, lp->d_secsize) < lp->d_secsize)
				fprintf(stderr,
				    "%s: alternate label %d write: %s\n",
				    progname, i/2, strerror(errno));
		}
		close(cfd);
	}
#endif /* vax */
}
#endif /* notdef */

static int64_t
strsuftoi64(const char *desc, const char *arg, int64_t min, int64_t max, int *num_suffix)
{
	int64_t result, r1;
	int shift = 0;
	char	*ep;

	errno = 0;
	r1 = strtoll(arg, &ep, 10);
	if (ep[0] != '\0' && ep[1] != '\0')
		errx(1, "%s `%s' is not a valid number.", desc, arg);
	switch (ep[0]) {
	case '\0':
	case 's': case 'S':
		if (num_suffix != NULL)
			*num_suffix = 0;
		break;
	case 'g': case 'G':
		shift += 10;
		/* FALLTHROUGH */
	case 'm': case 'M':
		shift += 10;
		/* FALLTHROUGH */
	case 'k': case 'K':
		shift += 10;
		/* FALLTHROUGH */
	case 'b': case 'B':
		if (num_suffix != NULL)
			*num_suffix = 1;
		break;
	default:
		errx(1, "`%s' is not a valid suffix for %s.", ep, desc);
	}
	result = r1 << shift;
	if (errno == ERANGE || result >> shift != r1)
		errx(1, "%s `%s' is too large to convert.", desc, arg);
	if (result < min)
		errx(1, "%s `%s' (%" PRId64 ") is less than the minimum (%" PRId64 ").",
		    desc, arg, result, min);
	if (result > max)
		errx(1, "%s `%s' (%" PRId64 ") is greater than the maximum (%" PRId64 ").",
		    desc, arg, result, max);
	return result;
}

void
usage()
{
	fprintf(stderr, "usage: newfs_lfs [ -fsoptions ] special-device\n");
	fprintf(stderr, "where fsoptions are:\n");
	fprintf(stderr, "\t-A (autoconfigure segment size)\n");
	fprintf(stderr, "\t-B segment size in bytes\n");
	fprintf(stderr, "\t-D (debug)\n");
	fprintf(stderr, "\t-M count of segments not counted in bfree\n");
	fprintf(stderr,
	    "\t-N (do not create file system, just print out parameters)\n");
	fprintf(stderr, "\t-O first segment offset in sectors\n");
	fprintf(stderr, "\t-R count of segments reserved for the cleaner\n");
	fprintf(stderr, "\t-b block size in bytes\n");
	fprintf(stderr, "\t-f frag size in bytes\n");
	fprintf(stderr, "\t-m minimum free space %%\n");
	fprintf(stderr, "\t-s file system size in sectors\n");
	fprintf(stderr, "\t-v version\n");
	exit(1);
}
