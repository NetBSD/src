/*	$NetBSD: exfatfs_extern.c,v 1.1.2.1 2024/06/29 19:43:26 perseant Exp $	*/

/*-
 * Copyright (c) 2022 The NetBSD Foundation, Inc.
 * All rights reserved.
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
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/param.h>
#include <sys/stdbool.h>
#include <sys/vnode.h>

#ifdef _KERNEL
# include <sys/buf.h>
# include <sys/malloc.h>
# include <sys/rwlock.h>
# include <miscfs/specfs/specdev.h>  /* For v_rdev */
#else
# include <stdlib.h>
# include <string.h>
# include <assert.h>
# include <stdio.h>

# include <sys/queue.h>
# include "bufcache.h"
# include "vnode.h"
# define vnode uvnode
# define buf ubuf
typedef struct uvvnode uvnode_t;
# define vnode_t uvnode_t
#endif

#include <fs/exfatfs/exfatfs.h>
#include <fs/exfatfs/exfatfs_cksum.h>
#include <fs/exfatfs/exfatfs_conv.h>
#include <fs/exfatfs/exfatfs_dirent.h>
#include <fs/exfatfs/exfatfs_extern.h>
#include <fs/exfatfs/exfatfs_inode.h>
#include <fs/exfatfs/exfatfs_extern.h>
#include <fs/exfatfs/exfatfs_trie.h>
#include <fs/exfatfs/exfatfs_vfsops.h>

/* #define EXFATFS_EXTERN_DEBUG */

#ifdef EXFATFS_EXTERN_DEBUG
# define DPRINTF(x) printf x
# ifndef _KERNEL
#  include <stdio.h>
# endif /* !_KERNEL */
#else
# define DPRINTF(x)
#endif

#ifdef _KERNEL
#define EXFATFS_DEBUG
extern struct pool exfatfs_xfinode_pool;
extern struct pool exfatfs_dirent_pool;
#endif /* _KERNEL */

static const char * exfatfs_check_bootblock(struct exfatfs *fs);
static int read_rootdir (struct exfatfs *fs);
extern int exfatfs_finish_mountfs(struct exfatfs *fs);
#ifdef EXFATFS_FENCE
static int checkzero(void *p, int len);
#endif /* EXFATFS_FENCE */

static unsigned long serial;

/*
 * Convert a logical address, in SECSIZE units, into a hardware
 * address, in DEV_BSIZE units.
 * XXX SECSIZE is too small, while CLUSTERSIZE is too big.
 * XXX We should probably use something like MIN(CLUSTERSIZE, MAXPHYS),
 * XXX and convert units when necessary.
 */
int
exfatfs_bmap_shared(struct vnode *vp, daddr_t targetlbn, struct vnode **vpp,
		    daddr_t *bnp, int *runp)
{
	struct xfinode *xip;
	struct exfatfs *fs;
	struct buf *bp;
	daddr_t pcn, targetcn;
	uint32_t lcn;
	int error, run = 0;
	unsigned c;

	DPRINTF(("BMAP(%p, %lu, ., ., .)\n",
		 vp, (unsigned long)targetlbn));

	assert(vp != NULL);
	assert(targetlbn >= 0);
	/* assert(vpp != NULL); */
	assert(bnp != NULL);
	xip = VTOXI(vp);
	assert(xip != NULL);
	fs = xip->xi_fs;
	assert(fs != NULL);
	exfatfs_check_fence(fs);
	assert(xip->xi_direntp != NULL);
	assert(xip->xi_direntp[0] != NULL);
	assert(xip->xi_direntp[1] != NULL);
	
	if (vpp != NULL)
		*vpp = xip->xi_devvp;
	if (bnp == NULL) {
		exfatfs_check_fence(fs);
		return (0);
	}

	/* If we can't find it, return -1 */
	*bnp = (daddr_t)-1;

	if (EXFATFS_FSSEC2CLUSTER(fs, (unsigned long)targetlbn) >
	    EXFATFS_BYTES2CLUSTER(fs, GET_DSE_DATALENGTH(xip))) {
		exfatfs_check_fence(fs);
		return 0;
	}

	/*
	 * If this file has "ignore FAT" set, calculate the physical
	 * address using dead reckoning from the first cluster.
	 */
	if (IS_DSE_NOFATCHAIN(xip)) {
		*bnp = EXFATFS_CLUSTER2HWADDR(fs, GET_DSE_FIRSTCLUSTER(xip))
			+ EXFATFS_FSSEC2DEVBSIZE(fs, targetlbn);
		if (runp) {
			*runp = EXFATFS_BYTES2FSSEC(fs, GET_DSE_DATALENGTH(xip))
				- targetlbn;
		}
		return 0;
	}
	    
	/*
	 * Walk the FAT chain until we reach the address of interest
	 * By default we start from zero
	 */
	targetcn = EXFATFS_FSSEC2CLUSTER(fs, targetlbn);
	assert(targetcn < fs->xf_ClusterCount);
	DPRINTF(("BMAP targetlbn=%lu targetcn=%lu\n",
		 (unsigned long)targetlbn,
		 (unsigned long)targetcn));
	pcn = GET_DSE_FIRSTCLUSTER(xip);
	lcn = 0;

	/* If we have no allocation, no block lives anywhere */
	if (pcn == 0) {
		*bnp = -1;
		if (runp)
			*runp = 0;
		exfatfs_check_fence(fs);
		return 0;
	}
			
	if (!(pcn >= 2 && pcn < fs->xf_ClusterCount + 2)) {
		printf("bmap: ino 0x%lx data length %lu, first cluster 0x%lx\n",
		       (unsigned long)INUM(xip),
		       (unsigned long)GET_DSE_DATALENGTH(xip),
		       (unsigned long)pcn);
	}
	assert(pcn >= 2 && pcn < fs->xf_ClusterCount + 2);

	DPRINTF(("BMAP lcn #0 at pcn %lu\n",
		 (unsigned long)GET_DSE_FIRSTCLUSTER(xip)));
	
	/* If we have a value cached, start there instead */
#ifdef USE_FATCACHE
	if (xip->xi_fatcache_lc > 0 && xip->xi_fatcache_lc < targetcn) {
		lcn = xip->xi_fatcache_lc;
		pcn = xip->xi_fatcache_pc;
		assert(pcn >= 2 && pcn < fs->xf_ClusterCount + 2);
		DPRINTF(("BMAP cached lclust #%d at pclust %lu\n",
			 lcn, pcn));
	}
#endif /* USE_FATCACHE */
	
	assert(fs->xf_devvp != NULL);
	assert(pcn >= 2 && pcn < fs->xf_ClusterCount + 2);
	c = 0;
	while (targetcn != lcn) {
		if (pcn < 2 || pcn >= fs->xf_ClusterCount + 2) {
			printf("Cluster 0x%x out of bounds!\n", (unsigned)pcn);
		}
		assert(pcn >= 2 && pcn < fs->xf_ClusterCount + 2);
		/* Read the FAT to find the next cluster */
		if ((error = bread(fs->xf_devvp, EXFATFS_FATBLK(fs, pcn), SECSIZE(fs), 0, &bp)) != 0) {
			printf("failed to read FAT pcn %u block 0x%x\n",
			       (unsigned)pcn, (unsigned)EXFATFS_FATBLK(fs, pcn));
			goto errout;
		}
		pcn = ((uint32_t *)bp->b_data)[EXFATFS_FATOFF(pcn)];
		brelse(bp, 0);
		++lcn;
		DPRINTF(("BMAP read lclust #%d at pcn %lu\n", lcn, pcn));
		if (pcn == 0xFFFFFFFF)
			break;
		assert(++c <= fs->xf_ClusterCount);
	}

	if (targetcn != lcn) {
		/* Not found */
		*bnp = -1;
		if (runp)
			*runp = 0;
		exfatfs_check_fence(fs);
		return 0;
	}
	
	if (pcn < 2 || pcn >= fs->xf_ClusterCount + 2) {
		printf("pcn 0x%x out of bounds in bmap(ino 0x%lx, lbn %lu)\n",
		       (unsigned)pcn, (unsigned long)INUM(xip),
		       (unsigned long)targetlbn);
		exfatfs_check_fence(fs);
		return EIO;
	}
	xip->xi_fatcache_lc = targetcn;
	xip->xi_fatcache_pc = pcn;
	*bnp = EXFATFS_CLUSTER2HWADDR(fs, pcn)
		+ EXFATFS_FSSEC2DEVBSIZE(fs, (targetlbn
					      & ((1 << fs->xf_SectorsPerClusterShift) - 1)));

	/* If we found it, hint the rest of the cluster. */
	run = EXFATFS_CLUSTER2FSSEC(fs, targetcn + 1) - targetlbn - 1;
	if (runp && *bnp != (daddr_t)-1) {
		*runp = run;
	}

	DPRINTF(("BMAP return lcn %d at pcn 0x%lx -> lbn %d..%d at bn 0x%lx..0x%lx\n",
		 (int)lcn, (unsigned long)pcn,
		 (int)targetlbn, (int)(targetlbn + run),
		 (unsigned long)*bnp,
		 (unsigned long)(*bnp + EXFATFS_FSSEC2DEVBSIZE(fs, run))));
errout:
	exfatfs_check_fence(fs);
	return error;
}

int exfatfs_mountfs_shared(struct vnode *devvp, struct exfatfs_mount *xmp, unsigned secsize, struct exfatfs **fsp)
{
	struct exfatfs *fs = NULL;
	struct buf *bp;
	int	error;
	unsigned secshift;
	const char *errstr;
	int boot_offset;
	uint8_t boot_ignore[3] = { 106, 107, 112 };
	int bn;
	uint32_t sum, badsb;

	DPRINTF(("exfatfs_mountfs_shared(%p, %u, %p)\n",
		 devvp, secsize, fsp));

	if (secsize < DEV_BSIZE) {
		DPRINTF(("Invalid block secsize (%d < DEV_BSIZE)\n", secsize));
		return EINVAL;
	}
	secshift = DEV_BSHIFT;
	while ((1U << secshift) < secsize)
		++secshift;

	/*
	 * Set up our exfat-specific mount structure
	 */
	if (secsize < sizeof(fs->xf_exfatdfs)) {
		DPRINTF(("sector size %u smaller than required %u\n",
			 (unsigned)secsize, (unsigned)sizeof(fs->xf_exfatdfs)));
		return EINVAL;
	}

	/*
	 * Check both boot blocks and checksums to find a valid one
	 */
	for (boot_offset = 0; boot_offset <= 12; boot_offset += 12) {
		/* Innocent until proven guilty */
		badsb = 0;
		
		/* Compute the checksum of the first 11 blocks */
		sum = 0;
		for (bn = 0; bn < 11; ++bn) {
			if ((error = bread(devvp,
					   (bn + boot_offset)
					   << (secshift - DEV_BSHIFT),
					   secsize, 0, &bp)) < 0) {
				DPRINTF(("bread (., %lu, %lu, ., .) errno %d\n",
					 (unsigned long)((bn + boot_offset)
							 << (secshift - DEV_BSHIFT)),
					 (unsigned long)secsize, error));
				continue;
			}
			/*
			 * The checksum of the first sector ignores
			 * certain fields, per the specification.
			 */
			sum = exfatfs_cksum32(sum, (uint8_t *)bp->b_data,
					      secsize,
					      boot_ignore, (bn == 0 ? 3 : 0));

			/*
			 * If this is the boot block, save the data;
			 * this is our superblock if checksums match.
			 */
			if (bn == 0 && fs == NULL) {
				fs = malloc(sizeof(*fs)
#ifdef _KERNEL
					    , M_EXFATFSBOOT, M_WAITOK
#endif /* _KERNEL */
					);
				memset(fs, 0, sizeof(*fs));
				memcpy(fs, bp->b_data, sizeof(fs->xf_exfatdfs));
#ifdef DEBUG_VERBOSE
				DPRINTF(("fs = %p\n", fs));
#endif
			}
			brelse(bp, 0);
		}
		if (fs == NULL)
			continue;

		/* Now read the recorded checksum and compare */
		if ((error = bread(devvp, (11 + boot_offset) << (secshift - DEV_BSHIFT),
				   secsize, 0, &bp)) != 0)
			continue;
		if (sum != *(u_int32_t *)(bp->b_data)) {
			DPRINTF(("Checksum mismatch at offset %u\n", boot_offset));
			badsb = 1;
		}
		brelse(bp, 0);
		bp = NULL;

		/* Check other aspects of the boot block */
		if (!badsb && (errstr = exfatfs_check_bootblock(fs)) != NULL) {
			DPRINTF(("%s\n", errstr));
			badsb = 1;
		}

		if (!badsb)
			break;

		/* Not an error on the first one, just try the other */
		free(fs
#ifdef _KERNEL
		     , M_EXFATFSBOOT
#endif /* _KERNEL */
			);
		fs = NULL;
	}
	
	if (fs == NULL) {
		DPRINTF(("Neither boot block was valid\n"));
		return EINVAL;
	}

	fs->xf_devvp = devvp;
	fs->xf_mp = xmp;
	if (xmp != NULL)
		xmp->xm_fs = fs;
	LIST_INIT(&fs->xf_newxip);
	
	exfatfs_finish_mountfs(fs);

	/*
	 * Read root directory to establish bitmapvp and upcasevp
	 */
	read_rootdir(fs);
	exfatfs_check_fence(fs);
	
	/*
	 * Initialize data structure for finding free clusters.
	 */
	if ((error = exfatfs_bitmap_init(fs, 1)) != 0) {
		DPRINTF(("Bitmap init failed with %d\n", error));
		goto error_exit;
	}

	*fsp = fs;
	exfatfs_check_fence(fs);
	return 0;

error_exit:
	exfatfs_check_fence(fs);
	if (fs)
		free(fs
#ifdef _KERNEL
		     , M_EXFATFSBOOT
#endif /* _KERNEL */
			);
	return (error);
}

struct genfs_ops;
extern struct genfs_ops exfatfs_genfsops;

/*
 * A more specialized version of loadvnode when we already have
 * the first block address and don't need anything else.
 */
static int
exfatfs_system_loadvnode(struct exfatfs *fs,
			 uint32_t dirclust, uint32_t diroffset,
			 uint32_t dataStart, uint64_t dataLength,
			 struct vnode **vpp)
{
	int error;
	struct xfinode *xip;
	struct vnode *vp;

	exfatfs_check_fence(fs);

	xip = exfatfs_newxfinode(fs, dirclust, diroffset);

	/*
	 * These do not have a File entry, but their entry is
	 * compatible with a Stream Extension entry.
	 * We fake it here since these vnodes will never be written.
	 */
	xip->xi_direntp[0] = exfatfs_newdirent();
	xip->xi_direntp[1] = exfatfs_newdirent();
		
	/* Fill in the few interesting fields */
	SET_DSE_FIRSTCLUSTER(xip, dataStart);
	SET_DSE_VALIDDATALENGTH(xip, dataLength);
	SET_DSE_DATALENGTH(xip, dataLength);

	DPRINTF(("exfatfs_system_loadvnode(%p, %u, %u, %u, %lu)\n",
		 fs, dirclust, diroffset, dataStart, dataLength));
	if ((error = exfatfs_getnewvnode(fs, NULL, dirclust, diroffset, VREG,
					 xip, &vp)) != 0) {
		exfatfs_check_fence(fs);
		return error;
	}
	DPRINTF(("exfatfs_system_loadvnode now vp=%p\n", vp));
	DPRINTF(("                            xip=%p\n", VTOXI(vp)));

	/* The xfinode is zeroed except for xi_key and xi_fs. */
	xip = VTOXI(vp);
#ifdef EXFATFS_FENCE
	assert(checkzero(xip->xi_fence1, EXFATFS_FENCE));
	assert(checkzero(xip->xi_fence2, EXFATFS_FENCE));
#endif
	/*
	 * Fill in a few fields of the vnode and finish filling in the
	 * xfinode.
	 */
	vp->v_data = xip;
	xip->xi_vnode = vp;
#ifdef _KERNEL
	vp->v_type = VREG; /* XXX */
	vref(xip->xi_devvp); /* XXX */
	vp->v_tag = VT_EXFATFS;
	/* genfs_node_init(vp, &exfatfs_genfsops); */ /* Not for VV_SYSTEM */
	uvm_vnp_setsize(vp, GET_DSE_VALIDDATALENGTH(VTOXI(vp)));
	vp->v_vflag |= VV_SYSTEM;
#ifdef EXFATFS_EXTERN_DEBUG
	vprint("system_loadvnode", vp);
#endif
#endif /* _KERNEL */

	*vpp = vp;
	exfatfs_check_fence(fs);
	return 0;
}

static int
read_rootdir (struct exfatfs *fs)
{
	uint8_t *data, *dp;
	struct exfatfs_dirent_allocation_bitmap *xdab;
	struct exfatfs_dirent_upcase_table *xdut;
	int i;
	struct buf *bp;
	uint32_t clust = fs->xf_FirstClusterOfRootDirectory;
	daddr_t daddr = EXFATFS_CLUSTER2HWADDR(fs, clust);
	
	exfatfs_check_fence(fs);
	/* Print root directory contents */
	bread(fs->xf_devvp, daddr, SECSIZE(fs), 0, &bp);
	data = (uint8_t *)bp->b_data;
	
	for (i = 0; i * 32 < SECSIZE(fs); i++) {
		dp = data + i * 32;

		switch (dp[0]) {
		case XD_ENTRYTYPE_EOD:
			goto done;

		case XD_ENTRYTYPE_ALLOC_BITMAP:
		case XD_ENTRYTYPE_ALLOC_BITMAP & ~XD_ENTRYTYPE_INUSE_MASK:
			xdab = (struct exfatfs_dirent_allocation_bitmap *)dp;
			/*
			 * Release and reacquire bp to avoid deadlock
			 * with loadvnode.  XXX Yuck.  We should instead
			 * inline the necessary parts of loadvnode
			 * so we don't have to re-read the file entry.
			 */
			brelse(bp, 0);
			exfatfs_system_loadvnode(fs, clust, i,
						 xdab->xd_firstCluster,
						 xdab->xd_dataLength,
						 &fs->xf_bitmapvp);
			bread(fs->xf_devvp, daddr, SECSIZE(fs), 0, &bp);
			data = (uint8_t *)bp->b_data;
			break;

		case XD_ENTRYTYPE_UPCASE_TABLE:
		case XD_ENTRYTYPE_UPCASE_TABLE & ~XD_ENTRYTYPE_INUSE_MASK:
			xdut = (struct exfatfs_dirent_upcase_table *)dp;
			brelse(bp, 0);
			exfatfs_system_loadvnode(fs, clust, i,
						 xdut->xd_firstCluster,
						 xdut->xd_dataLength,
						 &fs->xf_upcasevp);
			bread(fs->xf_devvp, daddr, SECSIZE(fs), 0, &bp);
			data = (uint8_t *)bp->b_data;
			break;

		case XD_ENTRYTYPE_VOLUME_LABEL:
		case XD_ENTRYTYPE_VOLUME_LABEL & ~XD_ENTRYTYPE_INUSE_MASK:
		case XD_ENTRYTYPE_VOLUME_GUID:
		case XD_ENTRYTYPE_VOLUME_GUID & ~XD_ENTRYTYPE_INUSE_MASK:
		case XD_ENTRYTYPE_TEXFAT_PADDING:
		case XD_ENTRYTYPE_TEXFAT_PADDING & ~XD_ENTRYTYPE_INUSE_MASK:
		default:
			/* Ignore files and everything else */
			break;
		}
	}
done:
	brelse(bp, 0);

	exfatfs_check_fence(fs);
	return 0;
}

static const char * exfatfs_check_bootblock(struct exfatfs *fs)
{
	unsigned i;

	DPRINTF(("JumpBoot field is: 0x%2.2x 0x%2.2x 0x%2.2x\n",
		 fs->xf_JumpBoot[0],
		 fs->xf_JumpBoot[1],
		 fs->xf_JumpBoot[2]));
	
	/*
	 * Sanity checks, from the specification
	 */
	if (memcmp(fs->xf_JumpBoot, EXFAT_JUMPBOOT, sizeof(fs->xf_JumpBoot)))
		return "JumpBoot field incorrect";
	if (memcmp(fs->xf_FileSystemName, EXFAT_FSNAME, sizeof(EXFAT_FSNAME)))
		return "FileSystemName field incorrect";
	for (i = 0; i < sizeof(fs->xf_MustBeZero); ++i) {
		if (fs->xf_MustBeZero[i] != 0) {
			return "MustBeZero is non-zero";
		}
	}
	if (fs->xf_VolumeLength < (1U << (20U - fs->xf_BytesPerSectorShift)))
		return "VolumeLength is too small";
	if (fs->xf_FatOffset < 24 || fs->xf_FatOffset
	    > fs->xf_ClusterHeapOffset
	    - (fs->xf_FatLength * fs->xf_NumberOfFats))
		return "FatOffset invalid";
	if (fs->xf_FatLength < ((fs->xf_ClusterCount + 2) * 4)
	    >> fs->xf_BytesPerSectorShift)
		return "FatLength %lu is too small";
	if (fs->xf_FatLength > (fs->xf_ClusterHeapOffset - fs->xf_FatOffset)
	    / fs->xf_NumberOfFats)
		return "FatLength is too large";
	if (fs->xf_ClusterHeapOffset < fs->xf_FatOffset
	    + fs->xf_FatLength * fs->xf_NumberOfFats)
		return "ClusterHeapOffset is too small";
	if (fs->xf_ClusterHeapOffset > (MIN(~(u_int32_t)0,
					   fs->xf_VolumeLength)
					- fs->xf_ClusterCount)
	    << fs->xf_SectorsPerClusterShift)
		return "ClusterHeapOffset is too large";
	if (fs->xf_ClusterCount < (fs->xf_VolumeLength
				   - fs->xf_ClusterHeapOffset)
	    >> fs->xf_SectorsPerClusterShift)
		return "ClusterCount is too small";
	if (fs->xf_ClusterCount > ~(u_int32_t)0 - 10)
		return "ClusterCount is too small";
	if (fs->xf_FirstClusterOfRootDirectory < 2
	    || fs->xf_FirstClusterOfRootDirectory > fs->xf_ClusterCount + 1)
		return "FirstClusterOfRootDriectory is out of bounds";
	if ((fs->xf_FileSystemRevision >> 8) > 99
	    || (fs->xf_FileSystemRevision & 0xFF) > 99)
		return "FileSystemRevision out of bounds";
	if ((fs->xf_FileSystemRevision >> 8) > EXFAT_GREATEST_VERSION)
		return "FileSystemRevision unsupported";
	if (fs->xf_BytesPerSectorShift < 9 || fs->xf_BytesPerSectorShift > 12)
		return "BytesPerSectorShift invalid";
	if (fs->xf_SectorsPerClusterShift == 0
	    || fs->xf_SectorsPerClusterShift >
	    25 - fs->xf_BytesPerSectorShift)
		return "BytesPerSectorShift invalid";
	if (fs->xf_NumberOfFats < 1 || fs->xf_NumberOfFats > 2)
		return "NumberOfFats %u invalid";
	if (fs->xf_PercentInUse > 100 && fs->xf_PercentInUse != 0xFF)
		return "PercentInUse out of bounds";
	if (fs->xf_BootSignature != 0xAA55)
		return "BootSignature invalid";

	return NULL;
}

struct xfinode *
exfatfs_newxfinode(struct exfatfs *fs, uint32_t dirclust, uint32_t diroffset)
{
	struct xfinode *xip;
	
#ifdef _KERNEL
	xip = pool_get(&exfatfs_xfinode_pool, PR_WAITOK);
#else /* ! _KERNEL */
	xip = malloc(sizeof(*xip));
	assert(xip != NULL);
#endif /* ! _KERNEL */

	memset(xip, 0, sizeof(*xip));
	/* xip->xi_flag = 0; */
	/* xip->xi_lockf = 0; */
	xip->xi_fs = fs;
	xip->xi_dirclust = dirclust;
	xip->xi_diroffset = diroffset;
	/* xip->xi_dirgen = 0; */
	xip->xi_devvp = fs->xf_devvp;
	xip->xi_refcnt = 1;
	xip->xi_serial = ++serial;
	//printf("alloc xfniode serial %lu as %p\n", xip->xi_serial, xip);
	
#ifdef _KERNEL
	rw_init(&xip->xi_rwlock);
#endif

	return xip;
}

void exfatfs_freexfinode(struct xfinode *xip)
{
	int i;

	assert(checkzero(xip->xi_fence1, EXFATFS_FENCE));
	assert(checkzero(xip->xi_fence2, EXFATFS_FENCE));

	for (i = 0; i < EXFATFS_MAXDIRENT; i++) {
		if (xip->xi_direntp[i] != NULL) {
			exfatfs_freedirent(xip->xi_direntp[i]);
			xip->xi_direntp[i] = NULL;
			xip->xi_dirent_blk[i] = 0;
			xip->xi_dirent_off[i] = 0;
		}
	}
	//printf("free xfniode serial %lu from %p\n", xip->xi_serial, xip);
	xip->xi_serial = -1;
#ifdef _KERNEL
	rw_destroy(&xip->xi_rwlock);
	memset(xip, 0xFF, sizeof(*xip));
	pool_put(&exfatfs_xfinode_pool, xip);
#else /* ! _KERNEL */
	free(xip);
#endif /* ! _KERNEL */
}

struct exfatfs_dirent *
exfatfs_newdirent(void)
{
#ifdef _KERNEL
	return pool_get(&exfatfs_dirent_pool, PR_WAITOK);
#else /* ! _KERNEL */
	struct exfatfs_dirent_plus *r;
	r = malloc(sizeof(struct exfatfs_dirent_plus));
	r->serial = ++serial;
	//printf("alloc dirent %lu as %p\n", r->serial, r);
	assert(r != NULL);
	return (struct exfatfs_dirent *)r;
#endif /* ! _KERNEL */
}

void
exfatfs_freedirent(struct exfatfs_dirent *dp)
{
#ifdef _KERNEL
	memset(dp, 0xFF, sizeof(*dp));
	pool_put(&exfatfs_dirent_pool, dp);
#else /* ! _KERNEL */
	//struct exfatfs_dirent_plus *r = (struct exfatfs_dirent_plus *)dp;
	//printf("free dirent %lu as %p\n", r->serial, r);
	free(dp);
#endif /* ! _KERNEL */
}

#ifdef EXFATFS_FENCE
static int checkzero(void *p, int len) {
	int i;

	for (i = 0; i < len; i++)
		if (((char *)p)[i] != 0)
			return 0;
	return 1;
}

void
exfatfs_check_fence(struct exfatfs *fs) {
	
	if (fs->xf_bitmapvp != NULL) {
		assert(checkzero(VTOXI(fs->xf_bitmapvp)->xi_fence1, EXFATFS_FENCE));
		assert(checkzero(VTOXI(fs->xf_bitmapvp)->xi_fence2, EXFATFS_FENCE));
	}
	if (fs->xf_upcasevp != NULL) {
		assert(checkzero(VTOXI(fs->xf_upcasevp)->xi_fence1, EXFATFS_FENCE));
		assert(checkzero(VTOXI(fs->xf_upcasevp)->xi_fence2, EXFATFS_FENCE));
	}
}
#endif /* EXFATFS_FENCE */

/*
 * Scan a directory looking for empty space, file entries, or
 * a particular file entry.  Used in readdir(), alloc(), lookup(),
 * and rmdir().  Could also be used in remove()/inactive() to
 * shrink directories if their tails become empty.
 *
 * emptyfunc is called with the beginning and end of the empty
 * space found.  If the return value includes SCANDIR_STOP,
 * the scan terminates.
 *
 * validfunc is called with a valid xfinode constructed from the
 * directory entry.  If the return value includes SCANDIR_DONTFREE,
 * exfatfs_scandir does not free the xfinode: the caller has
 * claimed it and takes responsibility for freeing it.
 *
 * Returns 0, or non-zero on error (passed from bread).
 */
int
exfatfs_scandir(struct vnode *dvp,
		off_t startoff, /* start offset in bytes */
		off_t *endoff,  /* ending offset in bytes */
		unsigned (*emptyfunc)(void *, off_t, off_t),
		unsigned (*validfunc)(void *, struct xfinode *, off_t),
		void *arg)
{
	struct exfatfs *fs;
	struct exfatfs_dirent *dentp0, *dentp, *endp;
	daddr_t lbn, maxlbn, blkno;
	int i, valid, error = 0;
	int entryno;
	struct buf *bp = NULL;
	char *data;
	struct xfinode *dxip, *xip = NULL;
	uint16_t cksum;
	off_t off, invstart;
	unsigned flags;
	int have_primary;
	unsigned long dserial;
	static uint8_t PRIMARY_IGNORE[2] = { 2, 3 };
	static uint8_t PRIMARY_IGNORE_LEN = 2;

	assert(dvp != NULL);
	assert(emptyfunc != NULL || validfunc != NULL);
	dxip = VTOXI(dvp);
	assert(dxip != NULL);
	dserial = dxip->xi_serial;
	assert(dserial > 0);
	fs = dxip->xi_fs;
	assert(fs != NULL);
	assert(fs->xf_devvp != NULL);
	assert(startoff == 0 || emptyfunc == NULL);
	assert(dxip->xi_direntp[0] != NULL);
	assert(dxip->xi_direntp[1] != NULL);
	
	xip = exfatfs_newxfinode(fs, 0, 0);
	assert(xip != NULL);
	assert(xip != dxip);
	assert(dxip->xi_serial == dserial);
	assert(dxip->xi_direntp[1] != NULL);

	cksum = 0;
	entryno = 0;
	invstart = -1;
	have_primary = 0;
	off = EXFATFS_BYTES2DIRENT(fs, startoff);
	lbn = EXFATFS_BYTES2FSSEC(fs, startoff);
	maxlbn = howmany(GET_DSE_VALIDDATALENGTH(dxip), SECSIZE(fs));
	while(lbn < maxlbn) {
		exfatfs_bmap_shared(dvp, lbn, NULL, &blkno, NULL);
		error = bread(fs->xf_devvp, blkno, SECSIZE(fs), 0, &bp);
		if (error)
			goto out;

		assert(dxip->xi_serial == dserial);
		data = bp->b_data;
		dentp0 = (struct exfatfs_dirent *)data;
		dentp = dentp0 + (off & (EXFATFS_FSSEC2DIRENT(fs, 1) - 1));
		endp = dentp0 + EXFATFS_FSSEC2DIRENT(fs, 1);
		while (dentp < endp) {
			if (ISPRIMARY(dentp))
				have_primary = 1;
			if (ISPRIMARY(dentp) || !ISINUSE(dentp)) {
				cksum = 0;
				entryno = 0;
				for (i = 0; i < EXFATFS_MAXDIRENT; i++) {
					if (xip->xi_direntp[i] != NULL) {
						DPRINTF((" free i=%d => %p\n", i,
							 xip->xi_direntp[i]));
						exfatfs_freedirent(xip->xi_direntp[i]);
						xip->xi_direntp[i] = NULL;
					}
					xip->xi_dirent_blk[i] = 0;
					xip->xi_dirent_off[i] = 0;
					assert(dxip->xi_serial == dserial);
				}
				if (invstart >= 0) {
					/* End of empty space */
					if (emptyfunc != NULL) {
						flags =	(*emptyfunc)(arg,
								     invstart,
								     off);
						if (flags & SCANDIR_STOP)
							goto out;
					}
					invstart = -1;
				}
			}
			if (ISINUSE(dentp))
				DPRINTF((" entry type 0x%x\n",
					 dentp->xd_entryType));
			else {
				have_primary = 0;
				if (invstart < 0)
					invstart = off;
				++off;
				++dentp;
				continue;
			}
			
			/* Store and checksum data as it comes */
			assert(xip->xi_direntp[entryno] == NULL);
			xip->xi_direntp[entryno] = exfatfs_newdirent();
			DPRINTF((" store i=%d => %p\n", entryno, xip->xi_direntp[entryno]));
			/* XXX letoh, depending on type? Or on access? */
			memcpy(xip->xi_direntp[entryno], dentp, sizeof(*dentp));
			xip->xi_dirent_blk[entryno] = blkno;
			xip->xi_dirent_off[entryno] = dentp - dentp0;
			DPRINTF((" cksum\n"));
			cksum = exfatfs_cksum16(cksum, (uint8_t *)
						dentp,
						sizeof(*dentp),
						PRIMARY_IGNORE,
						ISPRIMARY(dentp) ?
						PRIMARY_IGNORE_LEN : 0);

				       

			assert(dxip->xi_serial == dserial);
			DPRINTF((" switch\n"));
			switch (dentp->xd_entryType) {
			case XD_ENTRYTYPE_EOD:
				brelse(bp, 0);
				bp = NULL;
				lbn = maxlbn; /* super-break */
				DPRINTF(("  EOD\n"));
				break;
				
			case XD_ENTRYTYPE_FILE:
				/* Construct key from buffer and offset */
				memset(&xip->xi_key, 0, sizeof(xip->xi_key));
				xip->xi_dirclust = EXFATFS_HWADDR2CLUSTER(fs, xip->xi_dirent_blk[0]);
				xip->xi_diroffset = EXFATFS_HWADDR2DIRENT(fs, xip->xi_dirent_blk[0]) +
					xip->xi_dirent_off[0];
				break;
				
			case XD_ENTRYTYPE_STREAM_EXTENSION:
			case XD_ENTRYTYPE_FILE_NAME:
			default:
				break;
			}


			/*
			 * If this is the last entry in the set, compare
			 * checksums.  If they are different, this set
			 * does not constitute a valid entry.
			 */
			valid = 0;
			DPRINTF((" %d/%d\n", entryno,
				 GET_DFE_SECONDARY_COUNT(xip)));
			if (entryno > 0 && GET_DFE_SECONDARY_COUNT(xip) == entryno) {
				valid = (cksum == GET_DFE_SET_CHECKSUM(xip));
				DPRINTF((" %svalid: cksum computed 0x%x expected 0x%x\n",
					 (valid ? "" : "in"),
					 cksum, GET_DFE_SET_CHECKSUM(xip)));
				/*
				 * We have found a complete entry.
				 * Take action depending on whether
				 * we have found a valid file or empty space.
				 */
#if 0 && defined TRACE_INUM
				if (INUM(dxip) == TRACE_INUM) {
					uint16_t ucs2[NAME_MAX];
					uint8_t utf8[NAME_MAX];
					int namlen;
					exfatfs_get_file_name(xip, ucs2, &namlen, sizeof(ucs2));
					namlen = exfatfs_ucs2utf8str(ucs2, namlen, utf8, sizeof(utf8));
					printf("trace process inum 0x%lx primary 0x%hhx valid %d len %d name %.*s\n",
					       INUM(xip), xip->xi_direntp[0]->xd_entryType, valid, namlen, namlen, utf8);
				}
#endif /* TRACE_INUM */
					       
				if (valid) {
					assert(entryno > 1);
					assert(xip->xi_direntp[0] != NULL);
					assert(xip->xi_direntp[1] != NULL);
					assert(dxip->xi_serial == dserial);
					if (validfunc != NULL) {
						flags = (*validfunc)(arg, xip, EXFATFS_DIRENT2BYTES(fs, off));
						assert(dxip->xi_serial == dserial);
						if (flags & SCANDIR_DONTFREE) {
							/*
							 * Caller will free
							 * the xfinode.
							 * Get ourselves a
							 * new one.
							 */
							xip = exfatfs_newxfinode(fs, 0, 0);
						}
						assert(dxip->xi_serial == dserial);
						if (flags & SCANDIR_STOP) {
							DPRINTF(("  SCANDIR_STOP\n"));
							goto out;
						}
					}
					assert(dxip->xi_serial == dserial);
				} else {
#ifdef notyet
					/* That was empty space */
					if (invstart < 0)
						invstart = xip->xi_diroffset;
#endif /* notyet */
				}
				have_primary = 0;
				entryno = 0;
			}

			if (have_primary)
				++entryno;
			++dentp;
			++off;
		}
		brelse(bp, 0);
		bp = NULL;
		++lbn;
	}
	assert(dxip->xi_serial == dserial);

out:
	if (xip != NULL)
		exfatfs_freexfinode(xip);
	assert(dxip->xi_serial == dserial);
	if (bp != NULL)
		brelse(bp, 0);
	if (endoff != NULL)
		*endoff = EXFATFS_DIRENT2BYTES(fs, off);
	return error;
}

/*
 * Assemble the file name from its various components and
 * store it in "name", with its length in "namelenp".
 */
int exfatfs_get_file_name(struct xfinode *xip, uint16_t *name,
			  int *namelenp, int resid)
{
	int entryno, i;
	struct exfatfs_dfn *dfn;
	
	*namelenp = GET_DSE_NAMELENGTH(xip);
	for (i = 0, entryno = 2; entryno < EXFATFS_MAXDIRENT
		     && xip->xi_direntp[entryno] != NULL; ++entryno) {
		dfn = (struct exfatfs_dfn *)xip->xi_direntp[entryno];
		if (dfn->xd_entryType == XD_ENTRYTYPE_FILE_NAME) {
			memcpy(name + i, dfn->xd_fileName,
			       MIN(EXFATFS_NAME_CHUNKSIZE * sizeof(uint16_t),
				   (size_t)resid));
			i += EXFATFS_NAME_CHUNKSIZE;
			resid -= EXFATFS_NAME_CHUNKSIZE * sizeof(uint16_t);
		}
	}
	return 0;
}

/*
 * Replace the existing File Name directory entries with ones
 * that store the given filename.  This is complicated with the need
 * to preserve other, non-filename-related, secondary entries that may
 * follow the File Name entries.
 */
int exfatfs_set_file_name(struct xfinode *xip, uint16_t *name,
			  int namelen)
{
	struct exfatfs_dirent *oldp[EXFATFS_MAXDIRENT];
	struct exfatfs_dfn *dfnp;
	int i, j, off;

	/* Save a copy and set original to NULL */
	for (i = 0; i < EXFATFS_MAXDIRENT; i++) {
		oldp[i] = xip->xi_direntp[i];
		if (i >= 2)
			xip->xi_direntp[i] = NULL;
	}

	/* Generate new entries and copy them in */
	for (i = 2, off = 0; off < namelen; ++i, off += EXFATFS_NAME_CHUNKSIZE) {
		xip->xi_direntp[i] = exfatfs_newdirent();
		dfnp = (struct exfatfs_dfn *)xip->xi_direntp[i];
		memset(dfnp, 0, sizeof(*dfnp));
		dfnp->xd_entryType = XD_ENTRYTYPE_FILE_NAME;
		memcpy(dfnp->xd_fileName, name + off,
		       MIN(namelen - off, EXFATFS_NAME_CHUNKSIZE)
		       * sizeof(uint16_t));
	}
	DPRINTF((" namelen set to %d\n", namelen));
	SET_DSE_NAMELENGTH(xip, namelen);
	
	/* Copy any secondaries that followed the File Name entries */
	for (j = 2; oldp[j] != NULL && j < EXFATFS_MAXDIRENT
		     && i < EXFATFS_MAXDIRENT; j++) {
		if (oldp[j]->xd_entryType == XD_ENTRYTYPE_FILE_NAME)
			exfatfs_freedirent(oldp[j]);
		else
			xip->xi_direntp[i++] = oldp[j];
	}

	/* Recompute name hash */
	SET_DSE_NAMEHASH(xip, exfatfs_namehash(xip->xi_fs, 0, name, namelen));
	
	/* Note how many secondaries we now have */
	SET_DFE_SECONDARY_COUNT(xip, i - 1);

	return 0;
}
