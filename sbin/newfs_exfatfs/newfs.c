/*	$NetBSD: newfs.c,v 1.1.2.2 2024/07/24 00:46:18 perseant Exp $	*/

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
__RCSID("$NetBSD: newfs.c,v 1.1.2.2 2024/07/24 00:46:18 perseant Exp $");
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

#include <fs/exfatfs/exfatfs.h>
#include <fs/exfatfs/exfatfs_conv.h>

#include <err.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <paths.h>
#include <util.h>
#include "extern.h"
#include "bufcache.h"
#include "partutil.h"

#define MINBLOCKSIZE 4096

const char lmsg[] = "%s: can't read disk label";

int	Nflag = 0;		/* run without writing file system */
int	Vflag = 0;		/* verbose */
uint64_t fssize;		/* file system size */
uint32_t sectorsize;		/* bytes/sector */
uint32_t align = 0;		/* fat alignment (bytes) */
uint32_t csize = 0;		/* block size */
uint32_t heapalign = 0;		/* cluster heap alignment (bytes) */
uint32_t serial = 0;
uint32_t partition_offset = 0;
uint32_t fatlength = 0;

char	device[MAXPATHLEN];
char	*progname, *special, *disktype = NULL;

extern long	dev_bsize;		/* device block size */

static int64_t strsuftoi64(const char *, const char *, int64_t, int64_t, int *);
static void usage(void);

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
	int debug, force, fsi, fso, maxpartitions;
	uint secsize = 0;
	int byte_sized = 0;
	const char *label = "NONE GIVEN";
	uint16_t uclabel[11];
	uint16_t uclabellen;
	uint64_t diskbytes;
	char *bootcodefile = NULL, *bootcode = NULL;
	char *uctablefile = NULL, *uctable = NULL;
	size_t uctablesize = 0;
	int r;

	if ((progname = strrchr(*argv, '/')) != NULL)
		++progname;
	else
		progname = *argv;

	maxpartitions = getmaxpartitions();
	if (maxpartitions > 26)
		fatal("insane maxpartitions value %d", maxpartitions);

	debug = force = 0;
	memset(&dkw, 0, sizeof(dkw));
	while ((ch = getopt(argc, argv, "a:b:c:Dh:FL:l:No:S:s:T:u:v#:")) != -1)
		switch(ch) {
		case 'D':
			debug = 1;
			break;
		case 'F':
			force = 1;
			break;
		case 'L':
			label = optarg;
			break;
		case 'N':
			Nflag++;
			break;
		case 'S':
		  	secsize = strsuftoi64("sector size", optarg, 1,
					      INT64_MAX, NULL);
			if (secsize <= 0 || (secsize & (secsize - 1)))
				fatal("%s: bad sector size", optarg);
			break;
		case 'T':
			disktype = optarg;
			break;
		case 'a':
		  	align = strsuftoi64("alignment", optarg,
					    MINBLOCKSIZE, INT64_MAX, NULL);
			break;
		case 'b':
			bootcodefile = optarg;
			break;
		case 'c':
		  	csize = strsuftoi64("cluster size", optarg,
					    MINBLOCKSIZE, INT64_MAX, NULL);
			break;
		case 'h':
		  	heapalign = strsuftoi64("heap alignment", optarg,
						MINBLOCKSIZE, INT64_MAX, NULL);
			break;
		case 'l':
			fatlength = strtoul(optarg, NULL, 10);
			break;
		case 'o':
			dkw.dkw_offset = strtoul(optarg, NULL, 10);
			break;
		case 's':
		        fssize = strsuftoi64("file system size", optarg,
					     0, INT64_MAX, &byte_sized);
			break;
		case 'u':
			uctablefile = optarg;
			break;
		case 'v':
			Vflag++;
			break;
		case '#':
			serial = strtoul(optarg, NULL, 10);
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
		fso = open(special, O_RDWR, DEFFILEMODE);
		if (debug && fso < 0) {
			/* Create a file of the requested size. */
			fso = open(special, O_CREAT | O_RDWR, DEFFILEMODE);
			if (fso >= 0) {
				char buf[512];
				unsigned int i;
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
		(void)strcpy(dkw.dkw_ptype, DKW_PTYPE_EXFAT); 
		if (secsize == 0)
			secsize = 512;
		dkw.dkw_size = st.st_size / secsize;
	} else {
		if (getdiskinfo(special, fsi, disktype, &geo, &dkw) == -1)
			errx(1, lmsg, special);

		if (dkw.dkw_size == 0)
			fatal("%s: is zero sized", argv[0]);
		if (!force && strcmp(dkw.dkw_ptype, DKW_PTYPE_EXFAT) != 0)
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
	if (byte_sized) {
		fssize /= secsize;
	}

	/* If force, make the partition look like EXFAT */
	if (force) {
		(void)strcpy(dkw.dkw_ptype, DKW_PTYPE_EXFAT); 
		if (fssize) {
			dkw.dkw_size = fssize;
		}
	} else
		if (fssize != 0 && fssize < dkw.dkw_size)
			dkw.dkw_size = fssize;

	/* If csize not specified, switch based on the partition size */
	/* This table is based on Windows defaults */
	diskbytes = dkw.dkw_size * (off_t)dev_bsize;
	if (csize <= 0) {
#define KILOBYTE (off_t)1024
#define MEGABYTE (1024 * KILOBYTE)
#define GIGABYTE (1024 * MEGABYTE)
#define TERABYTE (1024 * GIGABYTE)
#ifdef USE_WINDOWS_DEFAULTS
		if (diskbytes < 256 * MEGABYTE)
			csize = 4 * KILOBYTE;
		else if (diskbytes < 32 * GIGABYTE)
			csize = 32 * KILOBYTE;
		else
			csize = 128 * KILOBYTE;
		if (Vflag)
			printf("Using Windows default cluster size %d"
				" for filesystem size %lld\n",
				(int)csize, (unsigned long long)diskbytes);
#else /* SD Card Association recommendations */
		if (diskbytes <= 8 * MEGABYTE)
			csize = 8 * KILOBYTE;
		else if (diskbytes <= 1 * GIGABYTE)
			csize = 16 * KILOBYTE;
		else if (diskbytes <= 128 * GIGABYTE)
			csize = 32 * KILOBYTE;
		else if (diskbytes <= 512 * GIGABYTE)
			csize = 128 * KILOBYTE;
		else if (diskbytes <= 2 * TERABYTE)
			csize = 256 * KILOBYTE;
		else
			csize = 512 * KILOBYTE;
		if (Vflag)
			printf("Using SD Card Association recommended"
				" cluster size %d"
				" for filesystem size %lld\n",
				(int)csize, (unsigned long long)diskbytes);
#endif
	} else {
		if (Vflag)
			printf("Using specified cluster size %d", (int)csize);
	}

	if (align <= 0) {
#ifdef USE_WINDOWS_DEFAULTS
		align = 0;
#else /* SD Card Association recommendations */
		if (diskbytes <= 8 * MEGABYTE)
			align = 8 * KILOBYTE;
		else if (diskbytes <= 64 * MEGABYTE)
			align = 16 * KILOBYTE;
		else if (diskbytes <= 256 * MEGABYTE)
			align = 32 * KILOBYTE;
		else if (diskbytes <= 2 * GIGABYTE)
			align = 64 * KILOBYTE;
		else if (diskbytes <= 32 * GIGABYTE)
			align = 4 * MEGABYTE;
		else if (diskbytes <= 128 * GIGABYTE)
			align = 16 * MEGABYTE;
		else if (diskbytes <= 512 * GIGABYTE)
			align = 32 * MEGABYTE;
		else
			align = 64 * MEGABYTE;
#endif
	}

	/* Convert given label from UTF8 into UCS2 */
	uclabellen = exfatfs_utf8ucs2str((const uint8_t *)label,
					 strlen(label), uclabel,
					 EXFATFS_LABELMAX);

	/* Load bootcode from file if requested */
	if (bootcodefile != NULL) {
		bootcode = (char *)malloc(390);
		FILE *fp = fopen(bootcodefile, "rb");
		if (fp == NULL || fread(bootcode, 390, 1, fp) != 1) {
			perror(bootcodefile);
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
		uctable = malloc(uctablesize);
		fp = fopen(uctablefile, "rb");
		if (fp == NULL || fread(uctable, uctablesize, 1, fp) != 1) {
			perror(uctablefile);
			exit(1);
		}
		fclose(fp);
	}

	/* Make the filesystem */
	r = make_exfatfs(fso, secsize, &dkw, csize,
			 uclabel, uclabellen, serial,
			 (uint16_t *)uctable, uctablesize, bootcode);
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

void
usage(void)
{
	fprintf(stderr, "usage: newfs_exfatfs [ -fsoptions ] special-device\n");
	fprintf(stderr, "where fsoptions are:\n");
	fprintf(stderr, "\t-D (debug)\n");
	fprintf(stderr, "\t-F (force)\n");
	fprintf(stderr,
	    "\t-N (do not create file system, just print out parameters)\n");
	fprintf(stderr, "\t-S sector size\n");
	fprintf(stderr, "\t-a alignment in bytes\n");
	fprintf(stderr, "\t-c cluster size in bytes\n");
	fprintf(stderr, "\t-s file system size in sectors\n");
	exit(1);
}
