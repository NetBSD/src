/*	$NetBSD: newfs.c,v 1.1.2.6 2024/09/13 05:18:50 perseant Exp $	*/

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
__RCSID("$NetBSD: newfs.c,v 1.1.2.6 2024/09/13 05:18:50 perseant Exp $");
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

#include "vnode.h"
#include "bufcache.h"

# define vnode uvnode
# define buf ubuf
#include <fs/exfatfs/exfatfs.h>
#include <fs/exfatfs/exfatfs_conv.h>
#include <fs/exfatfs/exfatfs_extern.h>

#include <err.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <paths.h>
#include <util.h>
#include "defaults.h"
#include "extern.h"
#include "partutil.h"

#define MINBLOCKSIZE 4096

const char lmsg[] = "%s: can't read disk label";

int	Nflag = 0;		/* run without writing file system */
int	Qflag = 0;		/* quiet */
int	Vflag = 0;		/* verbose */
int	Wflag = 0;		/* overwrite */
uint64_t fssize;		/* file system size */
uint32_t sectorsize;		/* bytes/sector */
uint32_t fatalign = 0;		/* fat alignment (sectors) */
uint32_t csize = 0;		/* block size */
uint32_t heapalign = 0;		/* cluster heap alignment (sectors) */
uint32_t serial = 0;
uint32_t partition_offset = 0;

char	device[MAXPATHLEN];
char	*progname, *disktype = NULL;
const char *special;

extern long	dev_bsize;		/* device block size */

static int64_t strsuftoi64(const char *, const char *, int64_t, int64_t, int *);
static void usage(void);
static struct uvnode *bufcache_init(int);

/* CHUNKSIZE should be larger than MAXPHYS */
#define CHUNKSIZE (1024 * 1024)

#ifndef DKW_PTYPE_EXFAT
# define DKW_PTYPE_EXFAT DKW_PTYPE_FAT
#endif

int
main(int argc, char **argv)
{
	int ch;
	struct disk_geom geo;
	struct dkwedge_info dkw;
	struct stat st;
	struct uvnode *devvp;
	struct exfatfs *fs;
	int debug, Fflag, fsi = -1, fso = -1, maxpartitions, nfats = 0;
	uint secsize = 0, secshift = 0;
	int byte_sized = 0;
	const char *label = "NONE GIVEN";
	uint16_t uclabel[11];
	uint16_t uclabellen;
	char *bootcodefile = NULL;
	char *xbootcodefile = NULL, *xbootcode = NULL;
	char *uctablefile = NULL;
	uint16_t *uctable = NULL;
	size_t uctablesize = 0;
	int r;

	if ((progname = strrchr(*argv, '/')) != NULL)
		++progname;
	else
		progname = *argv;

	maxpartitions = getmaxpartitions();
	if (maxpartitions > 26)
		fatal("insane maxpartitions value %d", maxpartitions);

	debug = Fflag = 0;
	memset(&dkw, 0, sizeof(dkw));
	while ((ch = getopt(argc, argv, "a:b:c:Dh:FL:l:Nn:o:qS:s:T:u:vwx:#:")) != -1)
		switch(ch) {
		case 'D': /* debug */
			debug = 1;
			/* Set Fflag too since we will create a regular file */
			Fflag = 1;
			break;
		case 'F': /* Fflag create filesystem even on non disk */
			Fflag = 1;
			break;
		case 'L': /* specify label */
			label = optarg;
			break;
		case 'N': /* dry run */
			Nflag++;
			break;
		case 'S': /* sector size in bytes */
		  	secsize = strsuftoi64("sector size", optarg, 1,
					      INT64_MAX, NULL);
			if (secsize <= 0 || (secsize & (secsize - 1)))
				fatal("%s: bad sector size", optarg);
			secshift = ffs(secsize) - 1;
			break;
		case 'T': /* specify disk type XXX */
			disktype = optarg;
			break;
		case 'a': /* FAT alignment */
		  	fatalign = strsuftoi64("FAT alignment", optarg,
						0, INT64_MAX, NULL);
			break;
		case 'b': /* take bootcode from file */
			bootcodefile = optarg;
			break;
		case 'c': /* cluster (block) size */
		  	csize = strsuftoi64("cluster size", optarg,
					    MINBLOCKSIZE, INT64_MAX, NULL);
			break;
		case 'h': /* cluster heap alignment */
		  	heapalign = strsuftoi64("heap alignment", optarg,
						0, INT64_MAX, NULL);
			break;
		case 'n': /* number of FATs */
			nfats = strtol(optarg, NULL, 10);
			break;
		case 'o': /* partition offset */
			partition_offset = strtoul(optarg, NULL, 10);
			break;
		case 'q': /* quiet */
			++Qflag;
			break;
		case 's': /* size in DEV_BSIZE units */
		        fssize = strsuftoi64("file system size", optarg,
					     0, INT64_MAX, &byte_sized);
			break;
		case 'u': /* take uctable from file */
			uctablefile = optarg;
			break;
		case 'v': /* verbose */
			Vflag++;
			break;
		case 'w': /* Overwrite existing superblocks without reading */
			Wflag = 1;
			break;
		case 'x': /* take xbootcode from file */
			xbootcodefile = optarg;
			break;
		case '#': /* specify serial number */
			serial = strtoul(optarg, NULL, 0);
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc != 2 && argc != 1)
		usage();

	special = argv[0];

	/*
	 * Open the input file.
	 */
	if (!Fflag) {
		char specname[MAXPATHLEN];
		char rawname[MAXPATHLEN];
		const char *raw;

		/* Translate wedge name to device name, if necessary */
		raw = getfsspecname(specname, sizeof(specname), special);
		if (raw == NULL)
			err(1, "%s: %s", special, specname);
		special = getdiskrawname(rawname, sizeof(rawname), raw);
		if (special == NULL)
			special = raw;
		
		fsi = opendisk(special, O_RDONLY, device, sizeof(device), 0);
		special = device;
		if (fsi < 0)
			err(1, "%s: open for read", special);
	} else { /* Fflag: just open the named file */
		fsi = open(special, O_RDONLY);

		/* If debugging, create the file */
		if (debug && !Nflag && fsi < 0) {
			fso = open(special, O_CREAT | O_RDWR, DEFFILEMODE);
			if (fso >= 0) {
				char buf[512];
				unsigned int i;
				for (i = 0; i < fssize; i++)
					write(fso, buf, sizeof(buf));
				lseek(fso, 0, SEEK_SET);
			}
			/* Now try again */
			fsi = open(special, O_RDONLY);
		}
		if (fsi < 0)
			err(1, "%s", special);
	}
	if (fstat(fsi, &st) < 0)
		err(1, "%s", special);

	/*
	 * If the -N flag isn't specified, open the output file.
	 */
	if (!Nflag) {
		if (fso < 0)
			fso = open(special, O_RDWR, DEFFILEMODE);
		if (fso < 0)
			err(1, "%s", special);
	}

	if (Vflag)
		printf("making exFAT file system on %s\n", special);

	if (!S_ISCHR(st.st_mode)) {
		if (!S_ISREG(st.st_mode)) {
			fatal("%s: neither a character special device "
			      "nor a regular file", special);
		}
		(void)strcpy(dkw.dkw_ptype, DKW_PTYPE_EXFAT); 
		if (secsize == 0)
			secsize = DEV_BSIZE;
		dkw.dkw_size = st.st_size / secsize;
	} else {
		if (getdiskinfo(special, fsi, disktype, &geo, &dkw) == -1)
			errx(1, lmsg, special);

		if (dkw.dkw_size == 0)
			fatal("%s: is zero sized", argv[0]);
		if (!Fflag && strcmp(dkw.dkw_ptype, DKW_PTYPE_EXFAT) != 0)
			fatal("%s: is not `%s', but `%s'", argv[0],
			    DKW_PTYPE_EXFAT, dkw.dkw_ptype);
	}

	if (secsize == 0)
		secsize = geo.dg_secsize;
	if (secsize == 0)
		secsize = DEV_BSIZE;

	/* Make device block size available to low level routines */
	dev_bsize = secsize;

	/* From here on out fssize is in sectors */
	if (byte_sized)
		fssize /= secsize;

	/* If not specified, use partition offset from wedge */
	if (partition_offset <= 0)
		partition_offset = dkw.dkw_offset;

	/* If Fflag, make the partition look like EXFAT */
	if (Fflag) {
		(void)strcpy(dkw.dkw_ptype, DKW_PTYPE_EXFAT); 
		if (fssize)
			dkw.dkw_size = fssize;
	} else {
		if (fssize > dkw.dkw_size) {
			fprintf(stderr, "Requested filesystem size"
				" %lld does not fit in partition size %lld\n",
				(long long)fssize, (long long)dkw.dkw_size);
			exit(1);
		}
		if (fssize > 0)
			dkw.dkw_size = fssize;
	}

	/*
	 * Read default values from an existing exFAT superblock,
	 * if there is one; unless we were asked not to.
	 */
	devvp = bufcache_init(fso);
	if (!Wflag) {
		exfatfs_locate_valid_superblock(devvp, secshift ? secshift : DEV_BSHIFT, &fs);
		if (Vflag && fs != NULL)
			printf("Read default values from existing valid boot block\n");
		/*
		 * These alignments are not stored directly in the boot sector,
		 * so if we are copying from an existing filesystem we can't
		 * be sure what was intended.  Assume that both alignments should
		 * be powers of two and choose the smallest such alignment.
		 */
		if (fatalign == 0 && fs != NULL && fs->xf_FatOffset > 24) {
			fatalign = 1 << (ffs(fs->xf_FatOffset) - 1);
		}
		if (heapalign > 0 && fs != NULL && fs->xf_ClusterHeapOffset > 0) {
			heapalign = 1 << (ffs(fs->xf_ClusterHeapOffset) - 1);
		}
	}
	/* If there is none, or we were asked to ignore it, start with default values */
	if (fs == NULL) {
		fs = default_exfat_sb(&dkw);
		fatalign = default_fatalign(fs->xf_VolumeLength) >> (secshift ? secshift : DEV_BSHIFT);
		heapalign = default_heapalign(fs->xf_VolumeLength) >> (secshift ? secshift : DEV_BSHIFT);
	}
	fs->xf_devvp = devvp;

	/*
	 * Apply values from arguments
	 */
	if (sectorsize > 0) {
		if (secshift < 9 || secshift > 12) {
			fprintf(stderr, "Sector size %d is out of range\n",
				(int)sectorsize);
			exit(1);
		}
		if (secshift != fs->xf_BytesPerSectorShift) {
			int shiftdiff = secshift - fs->xf_BytesPerSectorShift;
			fs->xf_BytesPerSectorShift = secshift;
			fs->xf_SectorsPerClusterShift -= shiftdiff;
			fs->xf_PartitionOffset >>= shiftdiff;
			fs->xf_FatOffset >>= shiftdiff;
			fs->xf_ClusterHeapOffset >>= shiftdiff;
		}
	}
	if (csize > 0) {
		int cshift = ffs(csize) - 1 - fs->xf_BytesPerSectorShift;
		if (cshift < 0 || cshift > 25 - fs->xf_BytesPerSectorShift) {
			fprintf(stderr, "Cluster size %d is out of range\n",
				(int)csize);
			exit(1);
		}
		fs->xf_SectorsPerClusterShift = cshift;
	}
	if (partition_offset > 0)
		fs->xf_PartitionOffset = partition_offset;
	if (nfats > 0)
		fs->xf_NumberOfFats = nfats;
	if (serial != 0)
		fs->xf_VolumeSerialNumber = serial;

	/* Convert given label from UTF8 into UCS2 */
	uclabellen = exfatfs_utf8ucs2str((const uint8_t *)label,
					 strlen(label), uclabel,
					 EXFATFS_LABELMAX);

	/* Load bootcode from file if requested */
	if (bootcodefile != NULL) {
		FILE *fp = fopen(bootcodefile, "rb");
		if (fp == NULL || fread(fs->xf_BootCode, EXFATFS_BOOTCODE_SIZE, 1, fp) != 1) {
			perror(bootcodefile);
			exit(1);
		}
		fclose(fp);
	}

	/* Load extended bootcode from file if requested */
	if (xbootcodefile != NULL) {
		FILE *fp = fopen(xbootcodefile, "rb");
		xbootcode = malloc(8 * BSSIZE(fs));
		if (fp == NULL || fread(xbootcode, BSSIZE(fs), 8, fp) != 8) {
			perror(xbootcodefile);
			exit(1);
		}
		fclose(fp);
	}

	/* Load upcase table from file if requested */
	if (uctablefile != NULL) {
		FILE *fp;
		if (stat(uctablefile, &st) != 0) {
			perror(uctablefile);
			exit(1);
		}
		uctablesize = st.st_size;
		uctable = (uint16_t *)malloc(uctablesize);
		fp = fopen(uctablefile, "rb");
		if (fp == NULL || fread(uctable, uctablesize, 1, fp) != 1) {
			perror(uctablefile);
			exit(1);
		}
		fclose(fp);
		if (uctablesize < 28 * sizeof(uint16_t)) {
			fprintf(stderr, "Upcase table size %d too small\n",
				(int)uctablesize);
			exit(1);
		}
	} else {
		default_upcase_table(&uctable, &uctablesize);
		if (Vflag)
			printf("Using default upcase table of %zd bytes\n",
				uctablesize);
	}
	
	/* Make the filesystem */
	r = make_exfatfs(devvp, fs, uclabel, uclabellen,
			 uctable, uctablesize, xbootcode,
			 fatalign, heapalign);

	if (debug)
		bufstats();
	exit(r);
}

/*
 * Error function for buffer cache errors
 */
static void
efun(int eval, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	verr(1, fmt, ap);
	va_end(ap);
}

/*
 * Initialize buffer cache.
 */
static struct uvnode *
bufcache_init(int devfd)
{
	struct uvnode *devvp;

	vfs_init();
	bufinit(17); /* XXX arbitrary magic 17 */
	esetfunc(efun);
	devvp = ecalloc(1, sizeof(*devvp));
	devvp->v_fs = NULL;
	devvp->v_fd = devfd;
	devvp->v_strategy_op = raw_vop_strategy;
	devvp->v_bwrite_op = raw_vop_bwrite;
	devvp->v_bmap_op = raw_vop_bmap;
	LIST_INIT(&devvp->v_cleanblkhd);
	LIST_INIT(&devvp->v_dirtyblkhd);

	return devvp;
}

static int64_t
strsuftoi64(const char *desc, const char *arg, int64_t min, int64_t max, int *num_suffix)
{
	int64_t result, r1;
	int shift = 0;
	char	*ep;

	/* Allow input in hex, without suffix, as well */
	if (arg[0] == '0' && arg[1] == 'x') {
		if (num_suffix != NULL)
			*num_suffix = 0;
		return strtoll(arg, &ep, 0);
	}

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

static void
usage(void)
{
	fprintf(stderr, "usage: newfs_exfatfs [ -fsoptions ] special-device\n");
	fprintf(stderr, "where fsoptions are:\n");
	fprintf(stderr, "\t-# serial number\n");
	fprintf(stderr, "\t-D (debug)\n");
	fprintf(stderr, "\t-F (create on regular file)\n");
	fprintf(stderr, "\t-L label\n");
	fprintf(stderr, "\t-N (do not create file system)\n");
	fprintf(stderr, "\t-S sector size\n");
	fprintf(stderr, "\t-T disk type\n");
	fprintf(stderr, "\t-a FAT alignment in bytes\n");
	fprintf(stderr, "\t-b bootcode file\n");
	fprintf(stderr, "\t-c cluster size in bytes\n");
	fprintf(stderr, "\t-h cluster heap alignment in bytes\n");
	fprintf(stderr, "\t-n number of FATs (1 or 2)\n");
	fprintf(stderr, "\t-o partition offset in sectors\n");
	fprintf(stderr, "\t-q (quiet)\n");
	fprintf(stderr, "\t-s file system size in sectors\n");
	fprintf(stderr, "\t-u upcase table file\n");
	fprintf(stderr, "\t-v (verbose)\n");
	fprintf(stderr, "\t-w (ignore and overwrite existing parameters)\n");
	fprintf(stderr, "\t-x extended bootcode file\n");
	exit(1);
}
