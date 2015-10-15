/*	$NetBSD: newfs.c,v 1.30 2015/10/15 06:24:33 dholland Exp $	*/

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
__COPYRIGHT("@(#) Copyright (c) 1989, 1992, 1993\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)newfs.c	8.5 (Berkeley) 5/24/95";
#else
__RCSID("$NetBSD: newfs.c,v 1.30 2015/10/15 06:24:33 dholland Exp $");
#endif
#endif /* not lint */

/*
 * newfs: friendly front end to mkfs
 */
#include <sys/param.h>
#include <sys/ucred.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/mount.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <sys/disk.h>

#include <ufs/lfs/lfs.h>

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
#include "partutil.h"

#define	COMPAT			/* allow non-labeled disks */

#ifdef COMPAT
const char lmsg[] = "%s: can't read disk label; disk type must be specified";
#else
const char lmsg[] = "%s: can't read disk label";
#endif

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
#endif
int	preen = 0;		/* Coexistence with fsck_lfs */

char	device[MAXPATHLEN];
char	*progname, *special;

extern long	dev_bsize;		/* device block size */

static int64_t strsuftoi64(const char *, const char *, int64_t, int64_t, int *);
static int strtoint(const char *, const char *, int, int);
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
		off = (((double)rand()) * (btodb(len))) / ((off_t)RAND_MAX + 1);
		if (pread(fd, buf, dbtob(1), dbtob(off)) < 0)
			err(1, "pread");
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
	struct disk_geom geo;
	struct dkwedge_info dkw;
	struct stat st;
	int debug, force, fsi, fso, lfs_segsize, maxpartitions, bitwidth;
	uint secsize = 0;
	daddr_t start;
	const char *opstring;
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

	opstring = "AB:b:DFf:I:i:LM:m:NO:R:r:S:s:v:w:";

	start = debug = force = lfs_segsize = bitwidth = 0;
	while ((ch = getopt(argc, argv, opstring)) != -1)
		switch(ch) {
		case 'A':	/* Adaptively configure segment size */
			lfs_segsize = -1;
			break;
		case 'B':	/* LFS segment size */
		        lfs_segsize = strsuftoi64("segment size", optarg, LFS_MINSEGSIZE, INT64_MAX, NULL);
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
		case 'w':
			bitwidth = strtoint("bit width", optarg, 32, 64);
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc != 2 && argc != 1)
		usage();

	if (bitwidth != 32 && bitwidth != 64 && bitwidth != 0) {
		errx(1, "bit width %d is not sensible; please use 32 or 64",
		     bitwidth);
	}
	if (bitwidth == 64 && version < 2) {
		errx(1, "Cannot make a 64-bit version 1 volume");
	}

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
		fso = open(special, O_RDWR, DEFFILEMODE);
		if (debug && fso < 0) {
			/* Create a file of the requested size. */
			fso = open(special, O_CREAT | O_RDWR, DEFFILEMODE);
			if (fso >= 0) {
				char buf[512];
				int i;
				for (i = 0; i < fssize; i++)
					write(fso, buf, sizeof(buf));
				lseek(fso, 0, SEEK_SET);
			}
		}
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
		if (!S_ISREG(st.st_mode)) {
			fatal("%s: neither a character special device "
			      "nor a regular file", special);
		}
		(void)strcpy(dkw.dkw_ptype, DKW_PTYPE_LFS); 
		if (secsize == 0)
			secsize = 512;
		dkw.dkw_size = st.st_size / secsize;
	} else {
#ifdef COMPAT
		if (disktype == NULL)
			disktype = argv[1];
#endif
		if (getdiskinfo(special, fsi, disktype, &geo, &dkw) == -1)
			errx(1, lmsg, special);

		if (dkw.dkw_size == 0)
			fatal("%s: is zero sized", argv[0]);
		if (!force && strcmp(dkw.dkw_ptype, DKW_PTYPE_LFS) != 0)
			fatal("%s: is not `%s', but `%s'", argv[0],
			    DKW_PTYPE_LFS, dkw.dkw_ptype);
	}

	if (secsize == 0)
		secsize = geo.dg_secsize;

	/* Make device block size available to low level routines */
	dev_bsize = secsize;

	/* From here on out fssize is in sectors */
	if (byte_sized) {
		fssize /= secsize;
	}

	/* If force, make the partition look like an LFS */
	if (force) {
		(void)strcpy(dkw.dkw_ptype, DKW_PTYPE_LFS); 
		if (fssize) {
			dkw.dkw_size = fssize;
		}
	} else
		if (fssize != 0 && fssize < dkw.dkw_size)
			dkw.dkw_size = fssize;

	/*
	 * default to 64-bit for large volumes; note that we test for
	 * 2^31 sectors and not 2^32, because block numbers (daddr_t)
	 * are signed and negative values can/will cause interesting
	 * complications.
	 */
	if (bitwidth == 0) {
		if (dkw.dkw_size > 0x7fffffff) {
			bitwidth = 64;
		} else {
			bitwidth = 32;
		}
	}

	if (dkw.dkw_size > 0x7fffffff && bitwidth == 32) {
		if (dkw.dkw_size >= 0xfffffffe) {
			/* block numbers -1 and -2 are magic; must not exist */
			errx(1, "This volume is too large for a 32-bit LFS.");
		}
		/* User does this at own risk */
		warnx("Using negative block numbers; not recommended. "
		      "You should probably use -w 64.");
	}

	/* Try autoconfiguring segment size, if asked to */
	if (lfs_segsize == -1) {
		if (!S_ISCHR(st.st_mode)) {
			warnx("%s is not a character special device, ignoring -A", special);
			lfs_segsize = 0;
		} else
			lfs_segsize = auto_segsize(fsi, dkw.dkw_size / secsize,
			    version);
	}

	/* If we're making a LFS, we break out here */
	r = make_lfs(fso, secsize, &dkw, minfree, bsize, fsize, lfs_segsize,
	    minfreeseg, resvseg, version, start, ibsize, interleave, roll_id,
	    bitwidth);
	if (debug)
		bufstats();
	exit(r);
}

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

static int
strtoint(const char *desc, const char *arg, int min, int max)
{
	long result;
	char *s;

	errno = 0;
	result = strtol(arg, &s, 10);
	if (errno || *s != '\0') {
		errx(1, "%s `%s' is not a valid number.", desc, arg);
	}
#if LONG_MAX > INT_MAX
	if (result > INT_MAX || result < INT_MIN) {
		errx(1, "%s `%s' is out of range.", desc, arg);
	}
#endif
	if (result < min || result > max) {
		errx(1, "%s `%s' is out of range.", desc, arg);
	}

	return (int)result;
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
