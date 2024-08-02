/* $NetBSD: exfatfs.h,v 1.1.2.6 2024/08/02 00:16:55 perseant Exp $ */

/*-
 * Copyright (c) 2022, 2024 The NetBSD Foundation, Inc.
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

#ifndef FS_EXFATFS_EXFATFS_H_
#define FS_EXFATFS_EXFATFS_H_

#include <sys/types.h>
#include <sys/queue.h>
#include <sys/param.h>

/* #define TRACE_INUM 0x8b54e1b */

#define USE_FATCACHE

#define EXFATFS_BOOTBLOCK_PRIMARY_FSSEC   0
#define EXFATFS_BOOTBLOCK_SECONDARY_FSSEC 12

/*
 * A data structure representing the boot sector (superblock).
 * From section 3 of the standatd.
 */
struct exfatdfs {
	uint8_t xdf_JumpBoot[3]; /* 0xEB 0x76 0x90 */
#define EXFAT_JUMPBOOT "\xEB\x76\x90"
	uint8_t xdf_FileSystemName[8];
#define EXFAT_FSNAME "EXFAT   "
	uint8_t xdf_MustBeZero[53]; /* 0x00 */
	uint64_t xdf_PartitionOffset;
	uint64_t xdf_VolumeLength;
	uint32_t xdf_FatOffset;
	uint32_t xdf_FatLength;
	uint32_t xdf_ClusterHeapOffset;
	uint32_t xdf_ClusterCount;
	uint32_t xdf_FirstClusterOfRootDirectory;
	uint32_t xdf_VolumeSerialNumber;
	uint16_t xdf_FileSystemRevision;
#define EXFAT_GREATEST_VERSION 1
	uint16_t xdf_VolumeFlags;
#define EXFATFS_VOLUME_ACTIVE_FAT    0x0001
#define EXFATFS_VOLUME_DIRTY         0x0002
#define EXFATFS_VOLUME_MEDIA_FAILURE 0x0004
#define EXFATFS_VOLUME_CLEAR_TO_ZERO 0x0008
	uint8_t  xdf_BytesPerSectorShift;
	uint8_t  xdf_SectorsPerClusterShift;
	uint8_t  xdf_NumberOfFats;
	uint8_t  xdf_DriveSelect;
	uint8_t  xdf_PercentInUse;
	uint8_t  xdf_Reserved[7];
#define EXFATFS_BOOTCODE_SIZE 390
	uint8_t  xdf_BootCode[EXFATFS_BOOTCODE_SIZE];
	uint16_t xdf_BootSignature;
#define EXFAT_BOOT_SIGNATURE 0xAA55
#define EXFAT_EXTENDED_BOOT_SIGNATURE 0xAA550000
};

struct exfatfs {
	struct exfatdfs xf_exfatdfs;
#define xf_JumpBoot xf_exfatdfs.xdf_JumpBoot
#define xf_FileSystemName xf_exfatdfs.xdf_FileSystemName
#define xf_MustBeZero xf_exfatdfs.xdf_MustBeZero
#define xf_PartitionOffset xf_exfatdfs.xdf_PartitionOffset
#define xf_VolumeLength xf_exfatdfs.xdf_VolumeLength
#define xf_FatOffset xf_exfatdfs.xdf_FatOffset
#define xf_FatLength xf_exfatdfs.xdf_FatLength
#define xf_ClusterHeapOffset xf_exfatdfs.xdf_ClusterHeapOffset
#define xf_ClusterCount xf_exfatdfs.xdf_ClusterCount
#define xf_FirstClusterOfRootDirectory \
		xf_exfatdfs.xdf_FirstClusterOfRootDirectory
#define xf_VolumeSerialNumber xf_exfatdfs.xdf_VolumeSerialNumber
#define xf_FileSystemRevision xf_exfatdfs.xdf_FileSystemRevision
#define EXFATFS_MAJOR(fs) (((fs)->xf_FileSystemRevision & 0xFF00) >> 8)
#define EXFATFS_MINOR(fs) ((fs)->xf_FileSystemRevision & 0xFF)
#define xf_VolumeFlags xf_exfatdfs.xdf_VolumeFlags
#define xf_BytesPerSectorShift xf_exfatdfs.xdf_BytesPerSectorShift
#define xf_SectorsPerClusterShift xf_exfatdfs.xdf_SectorsPerClusterShift
#define xf_NumberOfFats xf_exfatdfs.xdf_NumberOfFats
#define xf_DriveSelect xf_exfatdfs.xdf_DriveSelect
#define xf_PercentInUse xf_exfatdfs.xdf_PercentInUse
#define xf_Reserved xf_exfatdfs.xdf_Reserved
#define xf_BootCode xf_exfatdfs.xdf_BootCode
#define xf_BootSignature xf_exfatdfs.xdf_BootSignature

	/* Not on disk */
	struct exfatfs_mount *xf_mp;
	struct vnode *xf_devvp;
	struct vnode *xf_rootvp;
	struct vnode *xf_bitmapvp;
	struct vnode *xf_upcasevp; /* XXX not needed? */
	STAILQ_HEAD(, exfatfs_upcase_range_offset) xf_eurolist;
	LIST_HEAD(, xfinode) xf_newxip;
	int xf_bitmap_toplevel;
	uint32_t xf_FreeClusterCount;
	uint32_t xf_dirmask;
	uint32_t xf_gid;
	uint8_t  xf_gmtoff;
	uint32_t xf_mask;
	uint32_t xf_uid;
	int xf_nparent;
#ifdef _KERNEL
	kmutex_t xf_lock;
#endif /* _KERNEL */
#ifndef ALLOC_SEQUENTIAL
	uint32_t xf_next_cluster;
#endif
};

/*
 * Find the (absolute) hardware sector address and offset of a FAT entry.
 * DEV_BSIZE is 512 bytes and cluster numbers are 4 bytes,
 * so there are 128 = 2**7 entries per DEV_BSIZE.
 */
#define EXFATFS_FATBLK(fs, clust) (((fs)->xf_FatOffset >> 		\
	((fs)->xf_BytesPerSectorShift - DEV_BSHIFT)) + ((clust) >> 7))
#define EXFATFS_FATOFF(clust) ((clust) & 0x7F)

#if 0
#define EXFATFS_CLUSTER2PHYS(fs, clust) \
	(((fs)->xf_ClusterHeapOffset					\
	  + ((clust) << (fs)->xf_SectorsPerClusterShift))		\
	 << ((fs)->xf_BytesPerSectorShift - DEV_BSHIFT))
#endif

/*
 *  Arguments to mount EXFAT filesystems.
 */
struct exfatfs_args {
        char    *fspec;         /* blocks special holding the fs to mount */
        uid_t   uid;            /* uid that owns msdosfs files */
        gid_t   gid;            /* gid that owns msdosfs files */
        mode_t  mask;           /* mask to be applied for msdosfs perms */
        int     flags;          /* see below */

        /* Following items added after versioning support */
        int     version;        /* version of the struct */
#define EXFATFSMNT_VERSION      1
        mode_t  dirmask;        /* mask to be applied for msdosfs perms */
        int     gmtoff;         /* offset from UTC in seconds */
};

#define EXFATFS_LABELMAX 11
#define EXFATFS_NAMEMAX 255

/* A shift corresponding to MAXPHYS */
#define MAXPSHIFT 16
#define MAXPSIZE (1 << MAXPSHIFT)

/*
 * Units conversions between clusters, logical blocks, filesystem sectors
 * and DEV_BSIZE.
 */

/* Convert between bytes and the basic three types */
#define EXFATFS_CSHIFT(fs) ((fs)->xf_BytesPerSectorShift			\
			+ (fs)->xf_SectorsPerClusterShift)
#define EXFATFS_CSIZE(fs) (1 << EXFATFS_CSHIFT(fs))
#define EXFATFS_CMASK(fs) (EXFATFS_CSIZE(fs) - 1)

#define EXFATFS_LSHIFT(fs) MIN(MAXPSHIFT, EXFATFS_CSHIFT(fs))
#define EXFATFS_LSIZE(fs)  (1 << EXFATFS_LSHIFT(fs))
#define EXFATFS_LMASK(fs)  (EXFATFS_LSIZE(fs) - 1)

#define EXFATFS_SSHIFT(fs) ((fs)->xf_BytesPerSectorShift)
#define EXFATFS_SSIZE(fs) (1 << EXFATFS_SSHIFT(fs))
#define EXFATFS_SMASK(fs) (EXFATFS_SSIZE(fs) - 1)

/* Base shift from bytes to sizeof(struct dirent) */
#define EXFATFS_DIRENT_SHIFT 5

/* Convert from bytes to each */
#define EXFATFS_B2C(fs, n)      ((n) >> EXFATFS_CSHIFT(fs))
#define EXFATFS_B2L(fs, n)      ((n) >> EXFATFS_LSHIFT(fs))
#define EXFATFS_B2S(fs, n)      ((n) >> EXFATFS_SSHIFT(fs))
#define EXFATFS_B2D(fs, n)      ((n) >> DEV_BSHIFT)
#define EXFATFS_B2DIRENT(fs, n) ((n) >> EXFATFS_DIRENT_SHIFT)

/* Convert from sizeof(dirent) to each */
#define EXFATFS_DIRENT2C(fs, n) ((n) >> (EXFATFS_CSHIFT(fs) - EXFATFS_DIRENT_SHIFT))
#define EXFATFS_DIRENT2L(fs, n) ((n) >> (EXFATFS_LSHIFT(fs) - EXFATFS_DIRENT_SHIFT))
#define EXFATFS_DIRENT2S(fs, n) ((n) >> (EXFATFS_SSHIFT(fs) - EXFATFS_DIRENT_SHIFT))
#define EXFATFS_DIRENT2D(fs, n) ((n) >> (DEV_BSHIFT - EXFATFS_DIRENT_SHIFT))
#define EXFATFS_DIRENT2B(fs, n) ((n) << EXFATFS_DIRENT_SHIFT)

/* Convert from DEV_BSIZE to each */
#define EXFATFS_D2C(fs, n)      ((n) >> (EXFATFS_CSHIFT(fs) - DEV_BSHIFT))
#define EXFATFS_D2L(fs, n)      ((n) >> (EXFATFS_LSHIFT(fs) - DEV_BSHIFT))
#define EXFATFS_D2S(fs, n)      ((n) >> (EXFATFS_SSHIFT(fs) - DEV_BSHIFT))
#define EXFATFS_D2DIRENT(fs, n) ((n) << (DEV_BSHIFT - EXFATFS_DIRENT_SHIFT))
#define EXFATFS_D2B(fs, n)      ((n) << DEV_BSHIFT)

/* Convert from filesystem sectors to each */
#define EXFATFS_S2C(fs, n)      ((n) >> (EXFATFS_CSHIFT(fs) - EXFATFS_SSHIFT(fs)))
#define EXFATFS_S2L(fs, n)      ((n) >> (EXFATFS_LSHIFT(fs) - EXFATFS_SSHIFT(fs)))
#define EXFATFS_S2D(fs, n)      ((n) << (EXFATFS_SSHIFT(fs) - DEV_BSHIFT))
#define EXFATFS_S2DIRENT(fs, n) ((n) << (EXFATFS_SSHIFT(fs) - EXFATFS_DIRENT_SHIFT))
#define EXFATFS_S2B(fs, n)      ((n) << EXFATFS_SSHIFT(fs))

/* Convert from logical blocks to each */
#define EXFATFS_L2C(fs, n)      ((n) >> (EXFATFS_CSHIFT(fs) - EXFATFS_LSHIFT(fs)))
#define EXFATFS_L2S(fs, n)      ((n) << (EXFATFS_LSHIFT(fs) - EXFATFS_SSHIFT(fs)))
#define EXFATFS_L2D(fs, n)      ((n) << (EXFATFS_LSHIFT(fs) - DEV_BSHIFT))
#define EXFATFS_L2DIRENT(fs, n) ((n) << (EXFATFS_LSHIFT(fs) - EXFATFS_DIRENT_SHIFT))
#define EXFATFS_L2B(fs, n)      ((n) << EXFATFS_LSHIFT(fs))

/* Convert between clusters and each */
#define EXFATFS_C2L(fs, n)      ((n) << (EXFATFS_CSHIFT(fs) - EXFATFS_LSHIFT(fs)))
#define EXFATFS_C2S(fs, n)      ((n) << (EXFATFS_CSHIFT(fs) - EXFATFS_SSHIFT(fs)))
#define EXFATFS_C2D(fs, n)      ((n) << (EXFATFS_CSHIFT(fs) - DEV_BSHIFT))
#define EXFATFS_C2DIRENT(fs, n) ((n) << (EXFATFS_CSHIFT(fs) - EXFATFS_DIRENT_SHIFT))
#define EXFATFS_C2B(fs, n)      ((n) << EXFATFS_CSHIFT(fs))

/* Cluster / DEV_BSIZE mask */
#define EXFATFS_DCMASK(fs) (EXFATFS_C2D((fs), 1) - 1)

/* Size of boot sectors */
#define BSSHIFT(fs) EXFATFS_SSHIFT(fs)
#define BSSIZE(fs)  EXFATFS_SSIZE(fs)
#define BSMASK(fs)  EXFATFS_SMASK(fs)

/* Size of FAT blocks */
#define FATBSHIFT(fs) EXFATFS_SSHIFT(fs)
#define FATBSIZE(fs)  EXFATFS_SSIZE(fs)
#define FATBMASK(fs)  EXFATFS_SMASK(fs)

/* Inode numbers are coded as (cluster, entry #) pairs */
#define INO2CLUST(ino) EXFATFS_DIRENT2C(fs, ino)
#define INO2ENTRY(ino) ((ino) & ((EXFATFS_C2DIRENT(fs, 1)) - 1))
#define CE2INO(fs, c, e)   (EXFATFS_C2DIRENT(fs, (c)) | (e))

#define ROOTDIRCLUST 1
#define ROOTDIRENTRY 1
#define ROOTINO(fs) CE2INO((fs), ROOTDIRCLUST, ROOTDIRENTRY)

/*
 * Convert between a logical cluster number and its physical location on disk,
 * in DEV_BSIZE units.
 */
#define EXFATFS_LC2D(fs, c) (EXFATFS_C2D((fs), (c) - 2) + (fs)->xf_ClusterHeapOffset) /* EXFATFS_CLUSTER2HWADDR */
#define EXFATFS_D2LC(fs, bn) (EXFATFS_D2C((fs), (bn) - (fs)->xf_ClusterHeapOffset) + 2) /* EXFATFS_HWADDR2CLUSTER */
/* The offset of this DEV_BSIZE block relative to start of cluster, in dirent units */
#define EXFATFS_DCLOFF_DIRENT(fs, bn) EXFATFS_D2DIRENT((fs), ((bn) - EXFATFS_C2D((fs), EXFATFS_D2C((fs), (bn))))) /* EXFATFS_HWADDR2DIRENT */

#endif /* FS_EXFATFS_EXFATFS_H_ */
