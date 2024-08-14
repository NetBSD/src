/*	$NetBSD: make_exfatfs.c,v 1.1.2.8 2024/08/14 15:28:44 perseant Exp $	*/

/*-
 * Copyright (c) 2022 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Konrad E. Schroder <perseant@hhhh.org>.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
/*-
 * Copyright (c) 1991, 1993
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
#if 0
static char sccsid[] = "@(#)lfs.c	8.5 (Berkeley) 5/24/95";
#else
__RCSID("$NetBSD: make_exfatfs.c,v 1.1.2.8 2024/08/14 15:28:44 perseant Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/disk.h>
#include <sys/time.h>
#include <sys/mount.h>
#include <sys/stat.h>

/* Override certain things to make <ufs/lfs/lfs.h> work */
# undef simple_lock
# define simple_lock(x)
# undef simple_unlock
# define simple_unlock(x)
# define vnode uvnode
# define buf ubuf
# define panic call_panic
#include <fs/exfatfs/exfatfs.h>
#include <fs/exfatfs/exfatfs_cksum.h>
#include <fs/exfatfs/exfatfs_dirent.h>
#include <fs/exfatfs/exfatfs_extern.h>
#include <fs/exfatfs/exfatfs_tables.h>

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <util.h>

#include "extern.h"

#include "bufcache.h"
#include "vnode.h"


extern int Nflag; /* Don't write anything */
extern int Qflag; /* Quiet */
extern int Vflag; /* Verbose */

void pwarn(const char *, ...);
static void check_asserts(struct exfatfs *);

int
make_exfatfs(struct uvnode *devvp, struct exfatfs *fs,
	     uint16_t *uclabel, int uclabellen,
	     uint16_t *uctable, size_t uctablesize,
	     char *xbootcode,
	     uint32_t fatalign, uint32_t heapalign)
{
	uint32_t clust, nclust, nnclust;
	struct exfatfs_dirent_allocation_bitmap dirent_bitmap[2];
	struct exfatfs_dirent_upcase_table dirent_upcase;
	struct exfatfs_dirent_volume_label dirent_label;
	daddr_t daddr;
	char *data;
	struct buf *bp;
	uint32_t fatspersec;
	unsigned int i, j, bi, bit, byte;
	off_t resid;
	off_t end, start, progress, oprogress;
	uint32_t secsize = (1 << fs->xf_BytesPerSectorShift);

	if (Vflag) {
		printf("VolumeLength: %lu (0x%lx)\n",
			(unsigned long)fs->xf_VolumeLength,
			(unsigned long)fs->xf_VolumeLength);
		printf("Serial Number: 0x%lx (%lu)\n",
			(unsigned long)fs->xf_VolumeSerialNumber,
               		(unsigned long)fs->xf_VolumeSerialNumber);
		printf("Sector shift: %u (size %u)\n",
			(unsigned)fs->xf_BytesPerSectorShift,
			(unsigned)secsize);
		printf("Cluster shift: %u (size %u)\n",
			(unsigned)fs->xf_SectorsPerClusterShift,
			(unsigned)(1 << (fs->xf_SectorsPerClusterShift + fs->xf_BytesPerSectorShift)));
	}

	/*
	 * Apply alignments and compute the number of clusters
	 * to fill in the remaining fields of the superblock.
	 */
	
 	/* The FAT immediately follows the boot blocks */
	fs->xf_FatOffset = 24;
	/* and some optional alignment padding */
	if (fatalign) {
		uint32_t falignoff = fs->xf_PartitionOffset % fatalign;
		fs->xf_FatOffset = roundup(fs->xf_FatOffset + falignoff, fatalign);
	}
	if (Vflag)
		printf("FatOffset: 0x%lx (byte 0x%llx)\n",
		       (unsigned long)fs->xf_FatOffset,
		       (unsigned long long)fs->xf_FatOffset << fs->xf_BytesPerSectorShift);

	/* Find the total number of "blocks" (clusters) on this volume */
	/* This is an overestimate, not accounting for the FAT itself */
	nclust = (fs->xf_VolumeLength - fs->xf_FatOffset) >> fs->xf_SectorsPerClusterShift;

	if (fs->xf_FatLength == 0)
		fs->xf_FatLength = howmany((nclust + 2) * sizeof(uint32_t), secsize);
	else if (nclust > fs->xf_FatLength * secsize / sizeof(uint32_t) - 2)
		nclust = fs->xf_FatLength * secsize / sizeof(uint32_t) - 2;
	fs->xf_ClusterHeapOffset = fs->xf_FatOffset + fs->xf_NumberOfFats * fs->xf_FatLength;
	if (Vflag) {
		printf("FatLength: %lx\n",
			(unsigned long)fs->xf_FatLength);
	}

	/* Recalculate to take FATs into account */
	nclust = (fs->xf_VolumeLength - fs->xf_ClusterHeapOffset) >> fs->xf_SectorsPerClusterShift;
	/* Make sure it fits in our given FAT */
	nnclust = fs->xf_FatLength * secsize / sizeof(uint32_t) - 2;
	if (nclust > nnclust) {
		fs->xf_ClusterHeapOffset += (nclust - nnclust)
			<< fs->xf_SectorsPerClusterShift;
		nclust = nnclust;
	}
	if (heapalign) {
		uint32_t halignoff = fs->xf_PartitionOffset % heapalign;
		uint32_t nho = roundup(fs->xf_ClusterHeapOffset
					 + halignoff, heapalign);
		if ((fs->xf_VolumeLength - nho) >> fs->xf_SectorsPerClusterShift <= 0) {
			if (Vflag)
				printf("Volume is too small to align cluster heap to %lu sectors\n",
					(unsigned long)heapalign);
		} else {
			fs->xf_ClusterHeapOffset = nho;
			nclust = (fs->xf_VolumeLength - fs->xf_ClusterHeapOffset) >> fs->xf_SectorsPerClusterShift;
		}
	}
	fs->xf_ClusterCount = nclust;
	if (Vflag) {
		printf("File system contains %d clusters\n",
		       (int)fs->xf_ClusterCount);
		printf("ClusterHeapOffset: 0x%lx (byte 0x%llx)\n",
		       (unsigned long)fs->xf_ClusterHeapOffset,
		       (unsigned long long)fs->xf_ClusterHeapOffset << fs->xf_BytesPerSectorShift);
	}

	assert(fs->xf_ClusterHeapOffset >= fs->xf_FatOffset
            + fs->xf_FatLength * fs->xf_NumberOfFats);
	
	/*
	 * Prepare root directory entries.
	 * There are three of these at 32B each.
	 * Thus all of these will fit into a single cluster,
	 * even if the cluster size is the minimum of 4kB.
	 * They will even all fit into a single sector.
	 */

	/* Keep track of clusters as we allocate them */
	clust = 2; /* XXX symbol pls */

	/* First root directory entry: Volume Label */
	memset(&dirent_label, 0, sizeof(dirent_label));
	dirent_label.xd_entryType = XD_ENTRYTYPE_VOLUME_LABEL
		| XD_ENTRYTYPE_INUSE_MASK;
	dirent_label.xd_characterCount = uclabellen;
	memcpy(dirent_label.xd_volumeLabel, uclabel, uclabellen * 2);

	/* Second root directory entry: Allocation Bitmap */
	/* This has cluster data, like a file */
	for (bi = 0; bi < fs->xf_NumberOfFats; ++bi) {
		memset(&dirent_bitmap[bi], 0, sizeof(dirent_bitmap));
		dirent_bitmap[bi].xd_entryType = XD_ENTRYTYPE_ALLOC_BITMAP
			| XD_ENTRYTYPE_INUSE_MASK;
		dirent_bitmap[bi].xd_bitmapFlags = bi; /* Primary bitmap */
		dirent_bitmap[bi].xd_firstCluster = clust;
		dirent_bitmap[bi].xd_dataLength = howmany(nclust, NBBY);
		if (Vflag)
			printf("First cluster of bitmap region #%d: 0x%lx\n", bi,
			       (unsigned long)dirent_bitmap[bi].xd_firstCluster);
		clust += howmany(howmany(nclust, NBBY),
				 EXFATFS_CSIZE(fs));
	}

	/* Third root directory entry: Up-case Table */
	/* This too has cluster data, like a file */
	memset(&dirent_upcase, 0, sizeof(dirent_upcase));
	dirent_upcase.xd_entryType = XD_ENTRYTYPE_UPCASE_TABLE
		| XD_ENTRYTYPE_INUSE_MASK;
	dirent_upcase.xd_firstCluster = clust;
	dirent_upcase.xd_dataLength = uctablesize;
	dirent_upcase.xd_tableChecksum = 
		exfatfs_cksum32(0, (uint8_t *)uctable, uctablesize,
				NULL, 0);
	if (Vflag)
		printf("First cluster of upcase map: 0x%lx\n",
		       (unsigned long)dirent_upcase.xd_firstCluster);
	/* Size of upcase table, in clusters */
	clust += howmany(dirent_upcase.xd_dataLength, EXFATFS_CSIZE(fs));

	/*
	 * Write the upcase table to disk.
	 */
	daddr = EXFATFS_LC2D(fs, dirent_upcase.xd_firstCluster);
	resid = uctablesize;
	for (i = 0; resid > 0; i += EXFATFS_LSIZE(fs), resid -= EXFATFS_LSIZE(fs)) {
		if (!Nflag) {
			bp = getblk(devvp, daddr, EXFATFS_LSIZE(fs));
 			memset(bp->b_data, 0, EXFATFS_LSIZE(fs));
			memcpy(bp->b_data, ((const char *)uctable) + i,
				MIN(EXFATFS_LSIZE(fs), resid));
			if (Vflag)
				printf(" write upcase sector size %d (%d) at bn 0x%lx\n",
		       			EXFATFS_LSIZE(fs), (int)resid, (unsigned long)bp->b_blkno);
			bwrite(bp);
			++daddr;
		}
	}

	fs->xf_FirstClusterOfRootDirectory = clust;
	if (Vflag)
		printf("First cluster of root directory: 0x%lx (byte 0x%llx)\n",
		       (unsigned long)fs->xf_FirstClusterOfRootDirectory,
		       (unsigned long long)EXFATFS_LC2D(fs, fs->xf_FirstClusterOfRootDirectory) * secsize);
	

	/*
	 * Write the first bitmap sector.
	 */
	if (!Nflag) {
		for (bi = 0; bi < fs->xf_NumberOfFats; ++bi) {
			daddr = EXFATFS_LC2D(fs, dirent_bitmap[bi].xd_firstCluster);
			bp = getblk(devvp, daddr, EXFATFS_LSIZE(fs));
			memset(bp->b_data, 0, EXFATFS_LSIZE(fs));

			/* Mark off the clusters we've allocated */
			start = 2;
			for (i = start; i <= fs->xf_FirstClusterOfRootDirectory; i++) {
				/* It might take more than one bitmap cluster */
				if ((i - start) == NBBY * EXFATFS_LSIZE(fs)) {
					if (Vflag)
						printf(" write used bitmap sector size %d at bn 0x%lx\n",
			       				EXFATFS_LSIZE(fs), (unsigned long)bp->b_blkno);
					bwrite(bp);
					start = i;
					daddr += EXFATFS_L2D(fs, 1);
					bp = getblk(devvp, daddr, EXFATFS_LSIZE(fs));
					memset(bp->b_data, 0, EXFATFS_LSIZE(fs));
				}
				bit = ((i - start) & (NBBY - 1));
				byte = (i - start) / NBBY;
				if (Vflag > 1)
					printf("  set bit %d byte %d bn 0x%lx for cluster %lld\n", bit, byte, (unsigned long)bp->b_blkno, (long long)i);
				((char *)bp->b_data)[byte] |= (1 << bit);
			}
			if (Vflag)
				printf(" write used bitmap sector size %d at bn 0x%lx\n",
					EXFATFS_LSIZE(fs), (unsigned long)bp->b_blkno);
			bwrite(bp);

			/* Now write blank pages for the rest of the bitmap */
			progress = oprogress = 0;
			start = daddr + EXFATFS_L2D(fs, 1);
			if (bi == 0 && fs->xf_NumberOfFats > 1)
				end = EXFATFS_LC2D(fs, dirent_bitmap[1].xd_firstCluster);
			else
				end = EXFATFS_LC2D(fs, dirent_upcase.xd_firstCluster);
			for (daddr = start; daddr < end; ++daddr) {
				if (!Nflag) {
					size_t size = EXFATFS_LSIZE(fs);
					if (daddr < EXFATFS_LC2D(fs, end) - MAXPHYS / EXFATFS_LSIZE(fs))
						size = MAXPHYS;
					bp = getblk(devvp, daddr, size);
					memset(bp->b_data, 0, size);
					if (Vflag)
						printf(" write zero bitmap block size %zd at bn 0x%lx\n",
				       		size, (unsigned long)bp->b_blkno);
					else if (!Qflag) {
						progress = 80 * (daddr - start) / (end - start);
						for (i = oprogress; i < progress; i++) {
							printf(".");
							fflush(stdout);
						}
						oprogress = progress;
					}
					bwrite(bp);
					if (size > (size_t)EXFATFS_LSIZE(fs))
						daddr += size / EXFATFS_LSIZE(fs) - 1;
				}
			}
			if (!Vflag && !Qflag) {
				for (i = progress; i < 80; i++)
					printf(".");
				fflush(stdout);
				printf("\n");
			}
		}
	}

	/* Write the root directory following the bitmap */
	daddr = EXFATFS_LC2D(fs, fs->xf_FirstClusterOfRootDirectory);
	if (!Nflag) {
		bp = getblk(devvp, daddr, EXFATFS_LSIZE(fs));
		data = bp->b_data;
		memset(data, 0, EXFATFS_LSIZE(fs));
		
		memcpy(data, &dirent_label, sizeof(dirent_label));
		data += sizeof(dirent_label);

		for (bi = 0; bi < fs->xf_NumberOfFats; ++bi) {
			memcpy(data, &dirent_bitmap[bi], sizeof(dirent_bitmap[bi]));
			data += sizeof(dirent_bitmap[bi]);
		}

		memcpy(data, &dirent_upcase, sizeof(dirent_upcase));
		
		if (Vflag)
			printf("Write root directory at 0x%lx (0x%llx)\n",
			       (unsigned long)daddr,
			       (unsigned long long)daddr * secsize);

		bwrite(bp);
	}

	/* Fill the rest of the root directory cluster with zero */
	for (++daddr; EXFATFS_D2LC(fs, daddr)
		     == fs->xf_FirstClusterOfRootDirectory; ++daddr) {
		if (!Nflag) {
			bp = getblk(devvp, daddr, EXFATFS_LSIZE(fs));
			memset(bp->b_data, 0x00, EXFATFS_LSIZE(fs));
			if (Vflag)
				printf(" write zero root directory sector at bn 0x%lx\n",
				       (unsigned long)bp->b_blkno);
			bwrite(bp);
		}
	}

	/*
	 * Write the FAT(s).
	 */
	for (bi = 0; bi < fs->xf_NumberOfFats; ++bi) {
		progress = oprogress = 0;
		end = howmany(fs->xf_ClusterCount, FATBSIZE(fs) / sizeof(uint32_t));
		for (i = 0; i < end; ++i) {
			if (!Nflag) {
#if 1
				size_t size = MAXPHYS;
				if (size > (size_t)(end - i) * FATBSIZE(fs)) {
					size = (end - i) * FATBSIZE(fs);
					/* printf(" reset size to %zd for i=%lld\n",
						size, (long long)i); */
				}
#else
				size_t size = FATBSIZE(fs);
				if (i >= (MAXPHYS / size) && howmany(fs->xf_ClusterCount, FATBSIZE(fs) / sizeof(uint32_t)) - i > (MAXPHYS / size))
					size = MAXPHYS;
#endif
				fatspersec = size / sizeof(uint32_t);
				daddr = (fs->xf_FatOffset + bi * fs->xf_FatLength + i)
					* (FATBSIZE(fs) / DEV_BSIZE);
				bp = getblk(devvp, daddr, size);
				memset(bp->b_data, 0, size);
				for (j = 0; j < fatspersec; j++) {
					uint32_t sec = i * (FATBSIZE(fs) / sizeof(uint32_t)) + j;
					uint32_t v;
					
					if (sec == 0) {
						v = 0xFFFFFFF8;
					} else if (sec == dirent_bitmap[0].xd_firstCluster - 1
						|| sec == dirent_bitmap[fs->xf_NumberOfFats - 1].xd_firstCluster - 1
					   	|| sec == dirent_upcase.xd_firstCluster - 1
					   	|| sec == fs->xf_FirstClusterOfRootDirectory - 1
					   	|| sec >= fs->xf_FirstClusterOfRootDirectory) {
						/* No file or end of file */
						v = 0xFFFFFFFF;
					} else {
						v = sec + 1;
					}
	
					((uint32_t *)bp->b_data)[j] = v;
				}
				if (Vflag) {
					printf(" write fat sector size %zd at bn 0x%lx\n",
				       	size, (unsigned long)bp->b_blkno);
				} else if (!Qflag) {
					progress = 80 * i / end;
					for (j = oprogress; j < progress; j++) {
						printf(".");
						fflush(stdout);
					}
					oprogress = progress;
				}
				bwrite(bp);
				if (size > (size_t)FATBSIZE(fs))
					i += (size / FATBSIZE(fs)) - 1;
			}
		}
		if (!Vflag && !Qflag) {
			for (i = progress; i < 80; i++)
				printf(".");
			fflush(stdout);
			printf("\n");
		}
	}

	fs->xf_PercentInUse = 100 * (fs->xf_FirstClusterOfRootDirectory - 1)
		/ (fs->xf_ClusterCount);

	/*
	 * If given extended boot code,
	 * write the boot code sectors.
	 */
	if (xbootcode != NULL) {
		for (i = 0; i < 8; i++) {
			bp = getblk(devvp, i + 1, BSSIZE(fs));
			memcpy(bp->b_data, xbootcode + i * BSSIZE(fs),
			       BSSIZE(fs));
			bwrite(bp);
		}
	}

	check_asserts(fs);
	
	/*
	 * Finally, write the boot sector.
	 */
	if (Vflag)
		printf("Write boot sector\n");
	if (!Nflag)
		exfatfs_write_sb(fs, 1);

	return 0;
}

/*
 * Compatibility with fsck_lfs, since the "generic" LFS userland code uses it.
 */
void
pwarn(const char *fmt, ...)
{
        va_list ap;

        va_start(ap, fmt);
        vfprintf(stderr, fmt, ap);
        va_end(ap);
	putc('\n', stderr);
}

void
fatal(const char *fmt, ...)
{
        va_list ap;

        va_start(ap, fmt);
        vfprintf(stderr, fmt, ap);
        va_end(ap);
	putc('\n', stderr);
}

long dev_bsize = DEV_BSIZE;

/*
 * Compatibility with exfatfs_extern.c
 */
int exfatfs_getnewvnode(struct exfatfs *, struct uvnode *,
                        uint32_t, uint32_t, unsigned,
                        void *, struct uvnode **);

int
exfatfs_getnewvnode(struct exfatfs *fs, struct uvnode *dvp,
		    uint32_t clust, uint32_t off, unsigned type,
		    void *extra, struct uvnode **vpp)
{
        return 0;
}

int exfatfs_finish_mountfs(struct exfatfs *unused);

int
exfatfs_finish_mountfs(struct exfatfs *unused)
{
	return 0;
}

int exfatfs_bitmap_init(struct exfatfs *);

int
exfatfs_bitmap_init(struct exfatfs *unused)
{
	return 0;
}

static void
check_asserts(struct exfatfs *fs) {
	assert(fs->xf_FatOffset >= 24);
	assert(fs->xf_FatOffset <= fs->xf_ClusterHeapOffset
	    - (fs->xf_FatLength * fs->xf_NumberOfFats));
	assert(fs->xf_FatLength >= ((fs->xf_ClusterCount + 2) * 4)
	    >> fs->xf_BytesPerSectorShift);
	assert(fs->xf_FatLength <= (fs->xf_ClusterHeapOffset - fs->xf_FatOffset)
	    / fs->xf_NumberOfFats);
	assert(fs->xf_ClusterHeapOffset >= fs->xf_FatOffset
	    + fs->xf_FatLength * fs->xf_NumberOfFats);
	assert(fs->xf_ClusterHeapOffset <= ~(u_int32_t)0);
	assert(fs->xf_ClusterHeapOffset <= fs->xf_VolumeLength
					- (fs->xf_ClusterCount
	    << fs->xf_SectorsPerClusterShift));
	assert(fs->xf_ClusterCount >= (fs->xf_VolumeLength
					- fs->xf_ClusterHeapOffset)
	    >> fs->xf_SectorsPerClusterShift);
	assert(fs->xf_ClusterCount <= ~(u_int32_t)0 - 10);
	assert(fs->xf_FirstClusterOfRootDirectory >= 2);
	assert(fs->xf_FirstClusterOfRootDirectory <= fs->xf_ClusterCount + 1);
}
