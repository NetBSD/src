/*	$NetBSD: exfatfs_inode.h,v 1.1.2.3 2024/07/03 18:57:42 perseant Exp $	*/

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
/*
 * Some of this code is derived from msdosfs, which bore the following
 * copyright notice:
 */
/*-
 * Copyright (C) 1994, 1995, 1997 Wolfgang Solfrank.
 * Copyright (C) 1994, 1995, 1997 TooLs GmbH.
 * All rights reserved.
 * Original code by Paul Popelka (paulp@uts.amdahl.com) (see below).
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Written by Paul Popelka (paulp@uts.amdahl.com)
 *
 * You can do anything you want with this software, just don't say you wrote
 * it, and don't remove this notice.
 *
 * This software is provided "as is".
 *
 * The author supplies this software to be publicly redistributed on the
 * understanding that the author is not responsible for the correct
 * functioning of this software in any circumstances and is not liable for
 * any damages caused by this software.
 *
 * October 1992
 */
#ifndef EXFATFS_INODE_H_
#define EXFATFS_INODE_H_

#include <sys/rwlock.h>
#include <sys/vnode.h>

#include <fs/exfatfs/exfatfs.h>
#include <fs/exfatfs/exfatfs_dirent.h>
#include <miscfs/genfs/genfs_node.h>

struct exfatfs_lookup_results {
        u_long mlr_fndoffset;   /* offset of found dir entry */
        int mlr_fndcnt;         /* number of slots before de_fndoffset */
};

/* Vnode key; taken from msdosfs */
struct exfatfs_dirent_key {
	/* cluster of the (primary) directory file containing this entry */
        uint32_t dk_dirclust;
	/* offset of this entry in the directory cluster, in dirent units */
        uint32_t dk_diroffset;
	/* non zero and unique for unlinked nodes */
        void *dk_dirgen;
};

#define EXFATFS_FENCE 2
#define EXFATFS_MAXDIRENT 19 /* File + Stream + 17 * File Name */

/*
 * The in-memory structure representing an exFAT file.
 */
struct xfinode {
	struct genfs_node i_gnode; /* Must be first */
#ifdef EXFATFS_FENCE
	char xi_fence1[EXFATFS_FENCE];
#endif /* EXFATFS_FENCE */
	LIST_ENTRY(xfinode) xi_newxip;
	struct exfatfs *xi_fs;
	struct genfs_node xi_gnode;
	struct vnode *xi_vnode;	/* addr of vnode we are part of */
	struct vnode *xi_devvp;	/* vnode of blk dev we live on */
	struct vnode *xi_parentvp; /* vnode of parent, if not root */
	u_long xi_flag;		/* flag bits */
	struct exfatfs_dirent_key xi_key;
#define xi_dirclust xi_key.dk_dirclust
#define xi_diroffset xi_key.dk_diroffset
#define xi_dirgen xi_key.dk_dirgen
	long xi_refcnt;		/* reference count */
	struct exfatfs_mount *xi_xmp;	/* addr of our mount struct */
	struct lockf *xi_lockf;	/* byte level lock list */
	struct exfatfs_lookup_results xi_crap;
	int xi_trace; /* XXX DEBUG */
	
	/* Logical and physical cluster number, cached equivalency */
	uint32_t xi_fatcache_lc;
	uint32_t xi_fatcache_pc;

	/* Locking */
	krwlock_t xi_rwlock;
	
	/*
	 * Sections held on disk
	 */
	struct exfatfs_dirent *xi_direntp[EXFATFS_MAXDIRENT];
	daddr_t xi_dirent_blk[EXFATFS_MAXDIRENT]; /* Location on disk */
	int xi_dirent_off[EXFATFS_MAXDIRENT]; /* Offset within block */
	
	/* File Entry, one of the above pointers */
#define DFE(xip) ((struct exfatfs_dfe*)(xip)->xi_direntp[0])

#define GET_DFE_SET_CHECKSUM(xip) DFE(xip)->xd_setChecksum
#define GET_DFE_SECONDARY_COUNT(xip) DFE(xip)->xd_secondaryCount
#define GET_DFE_FILE_ATTRIBUTES(xip) DFE(xip)->xd_fileAttributes
#define GET_DFE_LAST_MODIFIED(xip) DFE(xip)->xd_lastModifiedTimestamp
#define GET_DFE_LAST_MODIFIED10MS(xip) DFE(xip)->xd_lastModified10msIncrement
#define GET_DFE_LAST_MODIFIED_UTCOFF(xip) DFE(xip)->xd_lastModifiedUtcOffset
#define GET_DFE_LAST_ACCESSED(xip) DFE(xip)->xd_lastAccessedTimestamp
#define GET_DFE_LAST_ACCESSED_UTCOFF(xip) DFE(xip)->xd_lastAccessedUtcOffset
#define GET_DFE_CREATE(xip) DFE(xip)->xd_createTimestamp
#define GET_DFE_CREATE10MS(xip) DFE(xip)->xd_create10msIncrement
#define GET_DFE_CREATE_UTCOFF(xip) DFE(xip)->xd_createUtcOffset

#define SET_DFE_SET_CHECKSUM(xip, v) 					\
do {									\
	DFE(xip)->xd_setChecksum = (v); 				\
} while(0)

#define SET_DFE_SECONDARY_COUNT(xip, v)					\
do {									\
	DFE(xip)->xd_secondaryCount = (v);				\
} while(0)

#define SET_DFE_FILE_ATTRIBUTES(xip, v)					\
do {									\
	DFE(xip)->xd_fileAttributes = (v);				\
} while(0)

#define SET_DFE_LAST_MODIFIED(xip, v)					\
do {									\
	DFE(xip)->xd_lastModifiedTimestamp = (v);			\
} while(0)

#define SET_DFE_LAST_MODIFIED10MS(xip, v)				\
do {									\
	DFE(xip)->xd_lastModified10msIncrement = (v);			\
} while(0)

#define SET_DFE_LAST_MODIFIED_UTCOFF(xip, v)				\
do {									\
	DFE(xip)->xd_lastModifiedUtcOffset = (v);			\
} while(0)

#define SET_DFE_LAST_ACCESSED(xip, v)					\
do {									\
	DFE(xip)->xd_lastAccessedTimestamp = (v);			\
} while(0)

#define SET_DFE_LAST_ACCESSED_UTCOFF(xip, v)				\
do {									\
	DFE(xip)->xd_lastAccessedUtcOffset = (v);			\
} while(0)

#define SET_DFE_CREATE(xip, v)						\
do {									\
	DFE(xip)->xd_createTimestamp = (v);				\
} while(0)

#define SET_DFE_CREATE10MS(xip, v)					\
do {									\
	DFE(xip)->xd_create10msIncrement = (v);				\
} while(0)

#define SET_DFE_CREATE_UTCOFF(xip, v)					\
do {									\
	DFE(xip)->xd_createUtcOffset = (v);				\
} while(0)

#define ISDIRECTORY(xip) (DFE(xip)->xd_fileAttributes & XD_FILEATTR_DIRECTORY)
#define ISREADONLY(xip)  (DFE(xip)->xd_fileAttributes & XD_FILEATTR_READONLY)
#define ISARCHIVE(xip)   (DFE(xip)->xd_fileAttributes & XD_FILEATTR_ARCHIVE)
#define ISSYMLINK(xip)   (DFE(xip)->xd_fileAttributes & XD_FILEATTR_SYMLINK)

	/* Stream extension */
#define DSE(xip) ((struct exfatfs_dse*)(xip)->xi_direntp[1])

#define GET_DSE_NAMELENGTH(xip)      DSE(xip)->xd_nameLength
#define GET_DSE_NAMEHASH(xip)        DSE(xip)->xd_nameHash
#define GET_DSE_VALIDDATALENGTH(xip) DSE(xip)->xd_validDataLength
#define GET_DSE_FIRSTCLUSTER(xip)    DSE(xip)->xd_firstCluster
#define GET_DSE_DATALENGTH(xip)      DSE(xip)->xd_dataLength
#define GET_DSE_DATALENGTH_BLK(xip, fs) roundup2(DSE(xip)->xd_dataLength, \
						CLUSTERSIZE(fs))

#define SET_DSE_NAMELENGTH(xip, v)					\
do {									\
	DSE(xip)->xd_nameLength = (v);					\
} while(0)

#define SET_DSE_NAMEHASH(xip, v)					\
do {									\
	DSE(xip)->xd_nameHash = (v);					\
} while(0)

#define SET_DSE_VALIDDATALENGTH(xip, v)					\
do {									\
	DSE(xip)->xd_validDataLength = (v);				\
} while(0)

#define SET_DSE_FIRSTCLUSTER(xip, v)					\
do {									\
	DSE(xip)->xd_firstCluster = (v);				\
} while(0)

#define SET_DSE_DATALENGTH(xip, v)					\
do {									\
	DSE(xip)->xd_dataLength = (v);					\
} while(0)

#define IS_DSE_ALLOCPOSSIBLE(xip)					\
	(DSE(xip)->xd_generalSecondaryFlags & XD_FLAG_ALLOCPOSSIBLE)
#define SET_DSE_ALLOCPOSSIBLE(xip)					\
do {									\
	DSE(xip)->xd_generalSecondaryFlags |= XD_FLAG_ALLOCPOSSIBLE;	\
} while (0)

#define CLR_DSE_ALLOCPOSSIBLE(xip)					\
do {									\
	DSE(xip)->xd_generalSecondaryFlags &= ~XD_FLAG_ALLOCPOSSIBLE;	\
} while (0)

#define IS_DSE_NOFATCHAIN(xip)						\
	(DSE(xip)->xd_generalSecondaryFlags & XD_FLAG_NOFATCHAIN)
#define SET_DSE_NOFATCHAIN(xip)						\
do {									\
	DSE(xip)->xd_generalSecondaryFlags |= XD_FLAG_NOFATCHAIN;	\
} while (0)

#define CLR_DSE_NOFATCHAIN(xip)						\
do {									\
	DSE(xip)->xd_generalSecondaryFlags &= ~XD_FLAG_NOFATCHAIN;	\
} while (0)

	unsigned long xi_serial; /* Serial number, for debugging */
	
#ifdef EXFATFS_FENCE
	char xi_fence2[EXFATFS_FENCE];
#endif /* EXFATFS_FENCE */
};

#ifdef _KERNEL
#define GETPARENT(xip, dvp) do {					\
	KASSERT(dvp != NULL);						\
	if (xip->xi_parentvp == NULL && dvp != NULL) {			\
		KASSERT(dvp->v_data != xip);				\
		++xip->xi_fs->xf_nparent;				\
		DPRINTF(("getparent %s:%d ino 0x%lx "			\
			 "parent 0x%lx now %d\n",			\
			 __FILE__, __LINE__,				\
			 (unsigned long)INUM(xip),			\
			 (unsigned long)INUM(VTOXI(dvp)),		\
			 xip->xi_fs->xf_nparent));			\
		xip->xi_parentvp = dvp;					\
		vref(dvp);						\
	}								\
} while (0)

#define PUTPARENT(xip) do {						\
	if (xip->xi_parentvp != NULL) {					\
		--xip->xi_fs->xf_nparent;				\
		DPRINTF(("putparent %s:%d ino 0x%lx "			\
			 "parent 0x%lx now %d\n",			\
			 __FILE__, __LINE__,				\
			 (unsigned long)INUM(xip),			\
			 (unsigned long)INUM(VTOXI(xip->xi_parentvp)),	\
			 xip->xi_fs->xf_nparent));			\
		vrele(xip->xi_parentvp);				\
		xip->xi_parentvp = NULL;				\
	}								\
} while (0)
#else
#define GETPARENT(xip, dvp)
#define PUTPARENT(xip)
#endif

#ifdef EXFATFS_FENCE
void exfatfs_check_fence(struct exfatfs *fs);
#else
# define exfatfs_check_fence(fs)
#endif

#define INUM(xip) EXFATFS_CLUST_ENTRY2INO((xip)->xi_fs, (xip)->xi_dirclust, \
					(xip)->xi_diroffset)

/*
 * Values for the xi_flag field of the xfinode.
 */
#define	XI_UPDATE	0x0001	/* Modification time update request. */
#define	XI_CREATE	0x0002	/* Creation time update */
#define	XI_ACCESS	0x0004	/* Access time update */
#define	XI_MODIFIED	0x0008	/* Xfinode has been modified. */
#define	XI_RENAME	0x0010	/* Xfinode is being renamed */

#define	EXFATFS_MAXNAMELEN	255

/* Maximum size of a file on a FAT filesystem */
#define EXFATFS_FILESIZE_MAX	~(uint64_t)0

#define	xi_forw		xi_chain[0]
#define	xi_back		xi_chain[1]

#define	VTOXI(vp)	((struct xfinode *)(vp)->v_data)
#define	XITOV(xi)	((xi)->xi_vnode)

#define	XITIMES(xip, acc, mod, cre) \
	while ((xip)->xi_flag & (XI_UPDATE | XI_CREATE | XI_ACCESS)) \
		exfatfs_itimes(xip, acc, mod, cre)

/*
 * This overlays the fid structure (see fstypes.h)
 */
struct defid {
	u_int16_t defid_len;	/* length of structure */
	u_int16_t defid_pad;	/* force 4-byte alignment */

	u_int32_t defid_dirclust; /* cluster this dir entry came from */
	u_int32_t defid_dirofs;	/* offset of entry within the cluster */
	u_int32_t defid_gen;	/* generation number */
};

#endif /* EXFATFS_INODE_H_ */
