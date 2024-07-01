/* $NetBSD: exfatfs.h,v 1.1.2.2 2024/07/01 22:15:21 perseant Exp $ */

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

struct xf_bitmap_node;

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
	uint8_t  xdf_BootCode[390];
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
	struct xf_bitmap_node *xf_bitmap_trie;
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

/* Base shift from bytes to sizeof(struct dirent) */
#define EXFATFS_DIRENT_BASESHIFT 5
#define EXFATFS_BYTES2DIRENT(fs, e) ((e) >> EXFATFS_DIRENT_BASESHIFT)
#define EXFATFS_DIRENT2BYTES(fs, e) ((e) << EXFATFS_DIRENT_BASESHIFT)

/* Convert from sizeof(dirent) to cluster */
#define EXFATFS_DIRENT_SHIFT(fs) ((fs)->xf_BytesPerSectorShift + \
	(fs)->xf_SectorsPerClusterShift - EXFATFS_DIRENT_BASESHIFT)

#define EXFATFS_DIRENT2DEVBSIZE(fs, e) ((e) >> (DEV_BSHIFT - \
					EXFATFS_DIRENT_BASESHIFT))
#define EXFATFS_DEVBSIZE2DIRENT(fs, e) ((e) << (DEV_BSHIFT \
						- EXFATFS_DIRENT_BASESHIFT))
#define EXFATFS_DIRENT2FSSEC(fs, e) ((e) >> (fs->xf_BytesPerSectorShift \
						- EXFATFS_DIRENT_BASESHIFT))
#define EXFATFS_FSSEC2DIRENT(fs, sec) ((sec) << (fs->xf_BytesPerSectorShift \
						- EXFATFS_DIRENT_BASESHIFT))

/* If we have an entry number we may need to convert it to lbn and offset */
#define EXFATFS_DIRENT2ENTRY(fs, e) EXFATFS_BYTES2DIRENT((fs), \
			(EXFATFS_DIRENT2BYTES((fs), (e)) & SECMASK(fs)))

#define EXFATFS_CLUST_ENTRY2INO(fs, clust, entry) ((((uint64_t)clust) \
				<< EXFATFS_DIRENT_SHIFT(fs)) | (entry))
#define EXFATFS_HWADDR_ENTRY2INO(fs, bn, entry) ((EXFATFS_HWADDR2CLUSTER((fs),\
			 (bn)) << EXFATFS_DIRENT_SHIFT(fs)) | (entry))
#define INO2CLUST(ino) ((ino) >> EXFATFS_DIRENT_SHIFT(fs))
#define INO2ENTRY(ino) ((ino) & ((1 << EXFATFS_DIRENT_SHIFT(fs)) - 1))
#define ROOTDIRCLUST 1
#define ROOTDIRENTRY 1
#define ROOTINO(fs) EXFATFS_CLUST_ENTRY2INO((fs), ROOTDIRCLUST, ROOTDIRENTRY)

/*
 * Units conversions between clusters, filesystem sectors and DEV_BSIZE.
 */
#define EXFATFS_FSSEC2DEVBSIZE(fs, bn) ((bn) << ((fs)->xf_BytesPerSectorShift \
							- DEV_BSHIFT))
#define EXFATFS_DEVBSIZE2FSSEC(fs, bn) ((bn) >> ((fs)->xf_BytesPerSectorShift \
							- DEV_BSHIFT))
#define EXFATFS_BYTES2CLUSTER(fs, n) ((n) >> ((fs)->xf_BytesPerSectorShift \
					+ (fs)->xf_SectorsPerClusterShift))
#define EXFATFS_BYTES2FSSEC(fs, n) ((n) >> ((fs)->xf_BytesPerSectorShift))
#define EXFATFS_FSSEC2BYTES(fs, n) ((n) << ((fs)->xf_BytesPerSectorShift))
#define EXFATFS_CLUSTER2BYTES(fs, cn) ((cn) << ((fs)->xf_BytesPerSectorShift \
					+ (fs)->xf_SectorsPerClusterShift))
#define EXFATFS_CLUSTER2DEVBSIZE(fs, cn) ((cn) << ((fs)->xf_BytesPerSectorShift\
			 + (fs)->xf_SectorsPerClusterShift - DEV_BSHIFT))
#define EXFATFS_DEVBSIZE2CLUSTER(fs, bn) ((bn) >> ((fs)->xf_BytesPerSectorShift\
			 + (fs)->xf_SectorsPerClusterShift - DEV_BSHIFT))
#define EXFATFS_CLUSTER2FSSEC(fs, clust) ((clust) << 			\
			(fs)->xf_SectorsPerClusterShift)
#define EXFATFS_FSSEC2CLUSTER(fs, lbn)   ((lbn) >> 			\
			(fs)->xf_SectorsPerClusterShift)
#define SECSIZE(fs) (1 << (fs)->xf_BytesPerSectorShift)
#define SECMASK(fs) (SECSIZE(fs) - 1)
#define CLUSTERSIZE(fs) (1 << ((fs)->xf_BytesPerSectorShift + 		\
				(fs)->xf_SectorsPerClusterShift))
#define CLUSTERMASK(fs) (CLUSTERSIZE(fs) - 1)
/* The unit in which I/O is performed */
#define IOSIZE(fs) MIN(CLUSTERSIZE(fs), MAXPHYS)
#define IOMASK(fs) (IOSIZE(fs) - 1)

#define EXFATFS_CLUSTER2HWADDR(fs, clust) (EXFATFS_CLUSTER2FSSEC((fs),	\
				(clust) - 2) + (fs)->xf_ClusterHeapOffset)
#define EXFATFS_HWADDR2CLUSTER(fs, bn) (EXFATFS_FSSEC2CLUSTER((fs), (bn) \
				- (fs)->xf_ClusterHeapOffset) + 2)
/* The offset of this block relative to start of cluster, in dirent units */
#define EXFATFS_HWADDR2DIRENT(fs, bn) EXFATFS_DEVBSIZE2DIRENT((fs),	\
	((bn) - EXFATFS_CLUSTER2HWADDR((fs), 				\
		EXFATFS_HWADDR2CLUSTER((fs), (bn)))))

#endif /* FS_EXFATFS_EXFATFS_H_ */
