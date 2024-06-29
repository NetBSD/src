/*	$NetBSD: fsck_exfatfs.c,v 1.1.2.1 2024/06/29 19:43:25 perseant Exp $	*/

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
__RCSID("$NetBSD: fsck_exfatfs.c,v 1.1.2.1 2024/06/29 19:43:25 perseant Exp $");
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

#include <err.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <paths.h>
#include <util.h>

#define vnode uvnode
#define buf ubuf
#include <fs/exfatfs/exfatfs.h>
#include <fs/exfatfs/exfatfs_conv.h>
#include <fs/exfatfs/exfatfs_extern.h>
#include <fs/exfatfs/exfatfs_inode.h>

#include "extern.h"
#include "bufcache.h"
#include "partutil.h"
#include "fsutil.h"
#include "fsck_exfatfs.h"
#include "exfatfs.h"
#include "pass0.h"
#include "pass1.h"
#include "pass2.h"

#define MINBLOCKSIZE 4096

int	debug;
int	Nflag = 0;		/* no to everything: read-only */
int	Pflag = 0;		/* preen */
int	Qflag = 0;		/* quiet */
int	Yflag = 0;		/* yes to everything */

/* Per filesystem, XXX should be encapsulated? */
int	resolved = 1;
int	fsdirty = 0;
int	problems = 0;
int	bitmap_discontiguous = 0;

/* Per filesystem, XXX should be encapsulated? */
unsigned long long total_files = 0;
unsigned long long frag_files = 0;
unsigned clusters_used = 0;

/* Per filesystem, XXX should be encapsulated? */
uint64_t fssize;		/* file system size */
uint32_t sectorsize;		/* bytes/sector */
uint32_t align = 0;		/* block size */
uint32_t csize = 0;		/* block size */
extern unsigned dev_bsize;
extern int g_devfd;
uint8_t *bitmap;
LIST_HEAD(dup, ino_entry) duplist;
char	device[MAXPATHLEN];
char	*progname, *special, *disktype = NULL;

static void usage(void);

static void
efun(int eval, const char *fmt, ...)
{
        va_list ap;
        va_start(ap, fmt);
        verr(1, fmt, ap);
        va_end(ap);
}

int
reply(const char *question)
{
        int persevere;
        char c;

        if (Pflag)
                pfatal("INTERNAL ERROR: GOT TO reply()");
        persevere = !strcmp(question, "CONTINUE");
        printf("\n");
        if (!persevere && Nflag) {
                printf("%s? no\n\n", question);
                resolved = 0;
                return (0);
        }
        if (Yflag || (persevere && Nflag)) {
                printf("%s? yes\n\n", question);
                return (1);
        }
        do      {
                printf("%s? [yn] ", question);
                (void) fflush(stdout);
                c = getc(stdin);
                while (c != '\n' && getc(stdin) != '\n') {
                        if (feof(stdin)) {
                                resolved = 0;
                                return (0);
                        }
                }
        } while (c != 'y' && c != 'Y' && c != 'n' && c != 'N');
        printf("\n");
        if (c == 'y' || c == 'Y')
                return (1);
        resolved = 0;
        return (0);
}

int
main(int argc, char **argv)
{
	int ch;
	struct ubuf *bp;
	struct uvnode *devvp;
	struct stat st;
	int devfd, force;
	uint secsize = 0;
	const char *opstring;
	int r = 0, error;
	daddr_t sbaddr, bn;
	struct exfatfs fsd, *fs = &fsd;
	struct xfinode *xip;
	uint32_t clust, oclust;
	struct exfatfs_dirent_allocation_bitmap *dentp0, *dentp, *endp;
	struct dkwedge_info dkw;
	struct disk_geom geo;

	if ((progname = strrchr(*argv, '/')) != NULL)
		++progname;
	else
		progname = *argv;

	debug = force = 0;
	sbaddr = 0x0;
	opstring = "5b:dfhnpqy";
	while ((ch = getopt(argc, argv, opstring)) != -1)
		switch(ch) {
		case '5':
			bitmap_discontiguous = 1;
			break;
		case 'b':
			sbaddr = strtoul(optarg, NULL, 0);
			break;
		case 'd':
			++debug;
			break;
		case 'f':
			force = 1;
			break;
		case 'n':
			Nflag++;
			break;
		case 'p':
			Pflag++;
			break;
		case 'q':
			Qflag++;
			break;
		case 'y':
			Yflag++;
			break;
			
		case 'h':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc < 1)
		usage();


	/*
	 * If the -n flag isn't specified, open the output file.  If no path
	 * prefix, try /dev/r%s and then /dev/%s.
	 */
	/* XXX look up device name in /etc/fstab */
	for (special = *argv; special != NULL; special = *++argv) {
		if (strchr(special, '/') == NULL) {
			(void)snprintf(device, sizeof(device), "%sr%s", _PATH_DEV,
				       special);
			if (stat(device, &st) == -1)
				(void)snprintf(device, sizeof(device), "%s%s",
					       _PATH_DEV, special);
			special = device;
		}
		devfd = open(special, (Nflag ? O_RDONLY : O_RDWR), DEFFILEMODE);
		if (devfd < 0)
			err(1, "%s", special);

		/*
		 * To check PartitionOffset and VolumeLength we need to
		 * fetch the partition information.
		 */
		if (fstat(devfd, &st) < 0)
			pfatal("%s: %s", special, strerror(errno));
		if (!S_ISCHR(st.st_mode)) {
			if (!S_ISREG(st.st_mode)) {
				pfatal("%s: neither a character special device "
				      "nor a regular file", special);
			}
			if (secsize == 0)
				secsize = 512;
			dkw.dkw_size = st.st_size / secsize;
		} else {
			if (getdiskinfo(special, devfd,
					disktype, &geo, &dkw) == -1)
				errx(1, "%s: can't read disk label", special);
			
			if (dkw.dkw_size == 0)
				pfatal("%s: is zero sized", argv[0]);
		}

		/*
		 * Initialize buffer cache.
		 */
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
		
		/* Read first superblock from the given address */
		/* XXX try both if none specified */
		if ((error = bread(devvp, sbaddr, DEV_BSIZE, 0, &bp)) != 0) {
			err(1, "Read sb at bn %lu\n", (unsigned long)sbaddr);
		}
		
		memset(fs, 0, sizeof(*fs));
		/* XXX endian conversion? */
		fs->xf_exfatdfs = *(struct exfatdfs *)bp->b_data;
		bp->b_flags |= B_INVAL;
		brelse(bp, 0);
		
		fs->xf_devvp = devvp;
		
		/* Determine sector size */
		secsize = 1 << fs->xf_BytesPerSectorShift;
		if (secsize == 0)
			secsize = DEV_BSIZE;
		dev_bsize = secsize;

		if (debug)
			pwarn("VolumeLength = %lu\n", (unsigned long)fs->xf_VolumeLength);
		
		/* Verify boot blocks */
		pass0(fs, &dkw);
		
		register_vget((void *)fs, exfatfs_vget, exfatfs_freevnode);
		g_devfd = devfd;

		/*
		 * Create in-memory bitmap.  XXX What if it is too big?
		 * XXX process one cluster's worth at a time?
		 * XXX Cluster max size is 32MB.  Can one cluster be too big?
		 */
		bitmap = ecalloc(1, fs->xf_ClusterCount / NBBY);
		
		/*
		 * Set up rootvp.  There is no directory entry so fake one.
		 * The root directory doesn't store its length anywhere.
		 * Walk its FAT to find out how long the file is.
		 */
		xip = exfatfs_newxfinode(fs, ROOTDIRCLUST, ROOTDIRENTRY);

		/* Fake file entry */
		xip->xi_direntp[0] = exfatfs_newdirent();
		SET_DFE_FILE_ATTRIBUTES(xip, XD_FILEATTR_DIRECTORY);
		
		/* Fake file stream entry, including data length */
		xip->xi_direntp[1] = exfatfs_newdirent();
		SET_DSE_FIRSTCLUSTER(xip, fs->xf_FirstClusterOfRootDirectory);
		SET_DSE_DATALENGTH(xip, 0);
		clust = fs->xf_FirstClusterOfRootDirectory;
		while (clust != 0xFFFFFFFF) {
			if (debug)
				fprintf(stderr, "rootvp cluster %d\n", clust);
			/* Mark it off in the bitmap */
			setbit(bitmap, clust - 2);
			++clusters_used;
			
			/* Read the FAT to find the next cluster */
			bread(fs->xf_devvp, EXFATFS_FATBLK(fs, clust),
			      SECSIZE(fs), 0, &bp);
			clust = ((uint32_t *)bp->b_data)
				[EXFATFS_FATOFF(clust)];
			brelse(bp, 0);
			SET_DSE_DATALENGTH(xip, GET_DSE_DATALENGTH(xip)
					   + CLUSTERSIZE(fs));
		}
		SET_DSE_VALIDDATALENGTH(xip, GET_DSE_DATALENGTH(xip));

		fs->xf_rootvp = vget3(fs, ROOTINO(fs), xip);

		/* Set up bitmapvp */
		exfatfs_bmap_shared(fs->xf_rootvp, 0, NULL, &bn, NULL);
		bread(fs->xf_devvp, bn, SECSIZE(fs), 0, &bp);
		dentp = dentp0 =
			(struct exfatfs_dirent_allocation_bitmap *)bp->b_data;
		endp = dentp + EXFATFS_FSSEC2DIRENT(fs, 1);
		while (dentp < endp) {
			if (dentp->xd_entryType == XD_ENTRYTYPE_ALLOC_BITMAP
			    || dentp->xd_entryType == XD_ENTRYTYPE_UPCASE_TABLE) {
				/* Fake file entries */
				xip = exfatfs_newxfinode(fs,
							 EXFATFS_HWADDR2CLUSTER(fs, bp->b_blkno),
							 dentp - dentp0);
				xip->xi_direntp[0] = exfatfs_newdirent();
				xip->xi_direntp[1] = exfatfs_newdirent();
				SET_DSE_FIRSTCLUSTER(xip, dentp->xd_firstCluster);
				SET_DSE_DATALENGTH(xip, dentp->xd_dataLength);
				if (dentp->xd_entryType == XD_ENTRYTYPE_ALLOC_BITMAP) {
					fs->xf_bitmapvp = vget3(fs, INUM(xip), xip);
					vref(fs->xf_bitmapvp);
				} else {
					fs->xf_upcasevp = vget3(fs, INUM(xip), xip);
					vref(fs->xf_upcasevp);
				}
			}
			++dentp;
		}
		brelse(bp, 0);

		/* Count bitmap file's clusters */
		clust = GET_DSE_FIRSTCLUSTER(VTOXI(fs->xf_bitmapvp));
		if (debug)
			fprintf(stderr, "bitmap file length %llu (%llu clusters)\n",
				(long long)GET_DSE_DATALENGTH(VTOXI(fs->xf_bitmapvp)),
				(long long)GET_DSE_DATALENGTH(VTOXI(fs->xf_bitmapvp)) / CLUSTERSIZE(fs));
		while (clust != 0xFFFFFFFF) {
			if (debug)
				fprintf(stderr, "bitmap file cluster %d\n", clust);
			/* Mark it off in the bitmap */
			setbit(bitmap, clust - 2);
			++clusters_used;
			
			/* Read the FAT to find the next cluster */
			bread(fs->xf_devvp, EXFATFS_FATBLK(fs, clust),
			      SECSIZE(fs), 0, &bp);

			oclust = clust;
			clust = ((uint32_t *)bp->b_data)
				[EXFATFS_FATOFF(clust)];
			if (clust != 0xFFFFFFFF && clust != oclust + 1) {
				if (!bitmap_discontiguous)
					pwarn("WARNING: %lu AND %lu NOT CONSECUTIVE: BITMAP DATA DISCONTIGUOUS\n", (unsigned long)oclust, (unsigned long)clust);
				bitmap_discontiguous = 1;
			}

			brelse(bp, 0);
		}
		++total_files;
		
		/* Count upcase file's clusters - XXX merge with above */
		clust = GET_DSE_FIRSTCLUSTER(VTOXI(fs->xf_upcasevp));
		while (clust != 0xFFFFFFFF) {
			if (debug)
				fprintf(stderr, "upcase file cluster %d\n", clust);
			/* Mark it off in the bitmap */
			setbit(bitmap, clust - 2);
			++clusters_used;

			/* Read the FAT to find the next cluster */
			bread(fs->xf_devvp, EXFATFS_FATBLK(fs, clust),
			      SECSIZE(fs), 0, &bp);
			clust = ((uint32_t *)bp->b_data)
				[EXFATFS_FATOFF(clust)];
			brelse(bp, 0);
		}
		++total_files;
		
		/*
		 * Check the filesystem
		 */

		/* Map all known files to verify FAT and reconstruct bitmap */
		/* (We will detect duplicate allocations as we go.) */
		LIST_INIT(&duplist);
		pass1(fs, &duplist, bitmap);

		/* Resolve duplicates (deterministically somehow) */
		
		/* Compare observed against stored bitmap */
		pass2(fs, bitmap);
		
		/* Mark filesystem clean */
		
		/* Report file count and cluster usage */
		
		close(devfd);
	}

	printf("%llu files, %u/%u clusters allocated, %0.1f%% fragmentation\n",
	       total_files, clusters_used, fs->xf_ClusterCount,
	       (frag_files * 100.0) / total_files);

	if (Nflag)
		r = (problems ? 2 : 0);
	
	if (debug)
		bufstats();
	exit(r);
}

void
usage(void)
{
	fprintf(stderr, "usage: fsck_exfatfs [ -fsoptions ] special-device\n");
	fprintf(stderr, "where fsoptions are:\n");
	fprintf(stderr,
		"\t-b bn\tuse the boot block at the given address\n");
	fprintf(stderr, "\t-d\t(debug)\n");
	fprintf(stderr, "\t-f\t(force)\n");
	fprintf(stderr, "\t-n\t(answer no to all questions)\n");
	fprintf(stderr, "\t-p\t(preen mode)\n");
	fprintf(stderr, "\t-q\t(quiet mode)\n");
	fprintf(stderr, "\t-v\t(verbose mode)\n");
	fprintf(stderr, "\t-y\t(answer yes to all questions)\n");
	exit(1);
}
