/*	$NetBSD: lfs_accessors.h,v 1.3 2015/08/02 17:57:27 dholland Exp $	*/

/*  from NetBSD: lfs.h,v 1.165 2015/07/24 06:59:32 dholland Exp  */
/*  from NetBSD: dinode.h,v 1.22 2013/01/22 09:39:18 dholland Exp  */
/*  from NetBSD: dir.h,v 1.21 2009/07/22 04:49:19 dholland Exp  */

/*-
 * Copyright (c) 1999, 2000, 2001, 2002, 2003 The NetBSD Foundation, Inc.
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
 *
 *	@(#)lfs.h	8.9 (Berkeley) 5/8/95
 */
/*
 * Copyright (c) 2002 Networks Associates Technology, Inc.
 * All rights reserved.
 *
 * This software was developed for the FreeBSD Project by Marshall
 * Kirk McKusick and Network Associates Laboratories, the Security
 * Research Division of Network Associates, Inc. under DARPA/SPAWAR
 * contract N66001-01-C-8035 ("CBOSS"), as part of the DARPA CHATS
 * research program
 *
 * Copyright (c) 1982, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *
 *	@(#)dinode.h	8.9 (Berkeley) 3/29/95
 */
/*
 * Copyright (c) 1982, 1986, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *
 *	@(#)dir.h	8.5 (Berkeley) 4/27/95
 */

#ifndef _UFS_LFS_LFS_ACCESSORS_H_
#define _UFS_LFS_LFS_ACCESSORS_H_

/*
 * Maximum length of a symlink that can be stored within the inode.
 */
#define ULFS1_MAXSYMLINKLEN	((ULFS_NDADDR + ULFS_NIADDR) * sizeof(int32_t))
#define ULFS2_MAXSYMLINKLEN	((ULFS_NDADDR + ULFS_NIADDR) * sizeof(int64_t))

#define ULFS_MAXSYMLINKLEN(ip) \
	((ip)->i_ump->um_fstype == ULFS1) ? \
	ULFS1_MAXSYMLINKLEN : ULFS2_MAXSYMLINKLEN

/*
 * "struct buf" associated definitions
 */

# define LFS_LOCK_BUF(bp) do {						\
	if (((bp)->b_flags & B_LOCKED) == 0 && bp->b_iodone == NULL) {	\
		mutex_enter(&lfs_lock);					\
		++locked_queue_count;					\
		locked_queue_bytes += bp->b_bufsize;			\
		mutex_exit(&lfs_lock);					\
	}								\
	(bp)->b_flags |= B_LOCKED;					\
} while (0)

# define LFS_UNLOCK_BUF(bp) do {					\
	if (((bp)->b_flags & B_LOCKED) != 0 && bp->b_iodone == NULL) {	\
		mutex_enter(&lfs_lock);					\
		--locked_queue_count;					\
		locked_queue_bytes -= bp->b_bufsize;			\
		if (locked_queue_count < LFS_WAIT_BUFS &&		\
		    locked_queue_bytes < LFS_WAIT_BYTES)		\
			cv_broadcast(&locked_queue_cv);			\
		mutex_exit(&lfs_lock);					\
	}								\
	(bp)->b_flags &= ~B_LOCKED;					\
} while (0)

/*
 * "struct inode" associated definitions
 */

#define LFS_SET_UINO(ip, flags) do {					\
	if (((flags) & IN_ACCESSED) && !((ip)->i_flag & IN_ACCESSED))	\
		lfs_sb_adduinodes((ip)->i_lfs, 1);			\
	if (((flags) & IN_CLEANING) && !((ip)->i_flag & IN_CLEANING))	\
		lfs_sb_adduinodes((ip)->i_lfs, 1);			\
	if (((flags) & IN_MODIFIED) && !((ip)->i_flag & IN_MODIFIED))	\
		lfs_sb_adduinodes((ip)->i_lfs, 1);			\
	(ip)->i_flag |= (flags);					\
} while (0)

#define LFS_CLR_UINO(ip, flags) do {					\
	if (((flags) & IN_ACCESSED) && ((ip)->i_flag & IN_ACCESSED))	\
		lfs_sb_subuinodes((ip)->i_lfs, 1);			\
	if (((flags) & IN_CLEANING) && ((ip)->i_flag & IN_CLEANING))	\
		lfs_sb_subuinodes((ip)->i_lfs, 1);			\
	if (((flags) & IN_MODIFIED) && ((ip)->i_flag & IN_MODIFIED))	\
		lfs_sb_subuinodes((ip)->i_lfs, 1);			\
	(ip)->i_flag &= ~(flags);					\
	if (lfs_sb_getuinodes((ip)->i_lfs) < 0) {			\
		panic("lfs_uinodes < 0");				\
	}								\
} while (0)

#define LFS_ITIMES(ip, acc, mod, cre) \
	while ((ip)->i_flag & (IN_ACCESS | IN_CHANGE | IN_UPDATE | IN_MODIFY)) \
		lfs_itimes(ip, acc, mod, cre)

/*
 * On-disk and in-memory checkpoint segment usage structure.
 */

#define	SEGUPB(fs)	(lfs_sb_getsepb(fs))
#define	SEGTABSIZE_SU(fs)						\
	((lfs_sb_getnseg(fs) + SEGUPB(fs) - 1) / lfs_sb_getsepb(fs))

#ifdef _KERNEL
# define SHARE_IFLOCK(F) 						\
  do {									\
	rw_enter(&(F)->lfs_iflock, RW_READER);				\
  } while(0)
# define UNSHARE_IFLOCK(F)						\
  do {									\
	rw_exit(&(F)->lfs_iflock);					\
  } while(0)
#else /* ! _KERNEL */
# define SHARE_IFLOCK(F)
# define UNSHARE_IFLOCK(F)
#endif /* ! _KERNEL */

/* Read in the block with a specific segment usage entry from the ifile. */
#define	LFS_SEGENTRY(SP, F, IN, BP) do {				\
	int _e;								\
	SHARE_IFLOCK(F);						\
	VTOI((F)->lfs_ivnode)->i_flag |= IN_ACCESS;			\
	if ((_e = bread((F)->lfs_ivnode,				\
	    ((IN) / lfs_sb_getsepb(F)) + lfs_sb_getcleansz(F),		\
	    lfs_sb_getbsize(F), 0, &(BP))) != 0)			\
		panic("lfs: ifile read: %d", _e);			\
	if ((F)->lfs_version == 1)					\
		(SP) = (SEGUSE *)((SEGUSE_V1 *)(BP)->b_data +		\
			((IN) & (lfs_sb_getsepb(F) - 1)));		\
	else								\
		(SP) = (SEGUSE *)(BP)->b_data + ((IN) % lfs_sb_getsepb(F)); \
	UNSHARE_IFLOCK(F);						\
} while (0)

#define LFS_WRITESEGENTRY(SP, F, IN, BP) do {				\
	if ((SP)->su_nbytes == 0)					\
		(SP)->su_flags |= SEGUSE_EMPTY;				\
	else								\
		(SP)->su_flags &= ~SEGUSE_EMPTY;			\
	(F)->lfs_suflags[(F)->lfs_activesb][(IN)] = (SP)->su_flags;	\
	LFS_BWRITE_LOG(BP);						\
} while (0)

/*
 * Index file inode entries.
 */

/*
 * LFSv1 compatibility code is not allowed to touch if_atime, since it
 * may not be mapped!
 */
/* Read in the block with a specific inode from the ifile. */
#define	LFS_IENTRY(IP, F, IN, BP) do {					\
	int _e;								\
	SHARE_IFLOCK(F);						\
	VTOI((F)->lfs_ivnode)->i_flag |= IN_ACCESS;			\
	if ((_e = bread((F)->lfs_ivnode,				\
	(IN) / lfs_sb_getifpb(F) + lfs_sb_getcleansz(F) + lfs_sb_getsegtabsz(F), \
	lfs_sb_getbsize(F), 0, &(BP))) != 0)				\
		panic("lfs: ifile ino %d read %d", (int)(IN), _e);	\
	if ((F)->lfs_version == 1)					\
		(IP) = (IFILE *)((IFILE_V1 *)(BP)->b_data +		\
				 (IN) % lfs_sb_getifpb(F));		\
	else								\
		(IP) = (IFILE *)(BP)->b_data + (IN) % lfs_sb_getifpb(F); \
	UNSHARE_IFLOCK(F);						\
} while (0)

/*
 * Cleaner information structure.  This resides in the ifile and is used
 * to pass information from the kernel to the cleaner.
 */

#define	CLEANSIZE_SU(fs)						\
	((sizeof(CLEANERINFO) + lfs_sb_getbsize(fs) - 1) >> lfs_sb_getbshift(fs))

/* Read in the block with the cleaner info from the ifile. */
#define LFS_CLEANERINFO(CP, F, BP) do {					\
	SHARE_IFLOCK(F);						\
	VTOI((F)->lfs_ivnode)->i_flag |= IN_ACCESS;			\
	if (bread((F)->lfs_ivnode,					\
	    (daddr_t)0, lfs_sb_getbsize(F), 0, &(BP)))			\
		panic("lfs: ifile read");				\
	(CP) = (CLEANERINFO *)(BP)->b_data;				\
	UNSHARE_IFLOCK(F);						\
} while (0)

/*
 * Synchronize the Ifile cleaner info with current avail and bfree.
 */
#define LFS_SYNC_CLEANERINFO(cip, fs, bp, w) do {		 	\
    mutex_enter(&lfs_lock);						\
    if ((w) || (cip)->bfree != lfs_sb_getbfree(fs) ||		 	\
	(cip)->avail != lfs_sb_getavail(fs) - fs->lfs_ravail -	 	\
	fs->lfs_favail) {	 					\
	(cip)->bfree = lfs_sb_getbfree(fs);			 	\
	(cip)->avail = lfs_sb_getavail(fs) - fs->lfs_ravail -		\
		fs->lfs_favail;					 	\
	if (((bp)->b_flags & B_GATHERED) == 0) {		 	\
		fs->lfs_flags |= LFS_IFDIRTY;				\
	}								\
	mutex_exit(&lfs_lock);						\
	(void) LFS_BWRITE_LOG(bp); /* Ifile */			 	\
    } else {							 	\
	mutex_exit(&lfs_lock);						\
	brelse(bp, 0);						 	\
    }									\
} while (0)

/*
 * Get the head of the inode free list.
 * Always called with the segment lock held.
 */
#define LFS_GET_HEADFREE(FS, CIP, BP, FREEP) do {			\
	if ((FS)->lfs_version > 1) {					\
		LFS_CLEANERINFO((CIP), (FS), (BP));			\
		lfs_sb_setfreehd(FS, (CIP)->free_head);			\
		brelse(BP, 0);						\
	}								\
	*(FREEP) = lfs_sb_getfreehd(FS);				\
} while (0)

#define LFS_PUT_HEADFREE(FS, CIP, BP, VAL) do {				\
	lfs_sb_setfreehd(FS, VAL);					\
	if ((FS)->lfs_version > 1) {					\
		LFS_CLEANERINFO((CIP), (FS), (BP));			\
		(CIP)->free_head = (VAL);				\
		LFS_BWRITE_LOG(BP);					\
		mutex_enter(&lfs_lock);					\
		(FS)->lfs_flags |= LFS_IFDIRTY;				\
		mutex_exit(&lfs_lock);					\
	}								\
} while (0)

#define LFS_GET_TAILFREE(FS, CIP, BP, FREEP) do {			\
	LFS_CLEANERINFO((CIP), (FS), (BP));				\
	*(FREEP) = (CIP)->free_tail;					\
	brelse(BP, 0);							\
} while (0)

#define LFS_PUT_TAILFREE(FS, CIP, BP, VAL) do {				\
	LFS_CLEANERINFO((CIP), (FS), (BP));				\
	(CIP)->free_tail = (VAL);					\
	LFS_BWRITE_LOG(BP);						\
	mutex_enter(&lfs_lock);						\
	(FS)->lfs_flags |= LFS_IFDIRTY;					\
	mutex_exit(&lfs_lock);						\
} while (0)

/*
 * On-disk segment summary information
 */

#define SEGSUM_SIZE(fs) ((fs)->lfs_version == 1 ? sizeof(SEGSUM_V1) : sizeof(SEGSUM))

/*
 * Super block.
 */

/*
 * Generate accessors for the on-disk superblock fields with cpp.
 *
 * STRUCT_LFS is used by the libsa code to get accessors that work
 * with struct salfs instead of struct lfs.
 */

#ifndef STRUCT_LFS
#define STRUCT_LFS struct lfs
#endif

#define LFS_DEF_SB_ACCESSOR_FULL(type, type32, field) \
	static __unused inline type				\
	lfs_sb_get##field(STRUCT_LFS *fs)			\
	{							\
		return fs->lfs_dlfs.dlfs_##field;		\
	}							\
	static __unused inline void				\
	lfs_sb_set##field(STRUCT_LFS *fs, type val)		\
	{							\
		fs->lfs_dlfs.dlfs_##field = val;		\
	}							\
	static __unused inline void				\
	lfs_sb_add##field(STRUCT_LFS *fs, type val)		\
	{							\
		type32 *p = &fs->lfs_dlfs.dlfs_##field;		\
		*p += val;					\
	}							\
	static __unused inline void				\
	lfs_sb_sub##field(STRUCT_LFS *fs, type val)		\
	{							\
		type32 *p = &fs->lfs_dlfs.dlfs_##field;		\
		*p -= val;					\
	}

#define LFS_DEF_SB_ACCESSOR(t, f) LFS_DEF_SB_ACCESSOR_FULL(t, t, f)

#define lfs_magic lfs_dlfs.dlfs_magic
#define lfs_version lfs_dlfs.dlfs_version
LFS_DEF_SB_ACCESSOR_FULL(u_int64_t, u_int32_t, size);
LFS_DEF_SB_ACCESSOR(u_int32_t, ssize);
LFS_DEF_SB_ACCESSOR(u_int32_t, dsize);
LFS_DEF_SB_ACCESSOR(u_int32_t, bsize);
LFS_DEF_SB_ACCESSOR(u_int32_t, fsize);
LFS_DEF_SB_ACCESSOR(u_int32_t, frag);
LFS_DEF_SB_ACCESSOR(u_int32_t, freehd);
LFS_DEF_SB_ACCESSOR(int32_t, bfree);
LFS_DEF_SB_ACCESSOR(u_int32_t, nfiles);
LFS_DEF_SB_ACCESSOR(int32_t, avail);
LFS_DEF_SB_ACCESSOR(int32_t, uinodes);
LFS_DEF_SB_ACCESSOR(int32_t, idaddr);
LFS_DEF_SB_ACCESSOR(u_int32_t, ifile);
LFS_DEF_SB_ACCESSOR(int32_t, lastseg);
LFS_DEF_SB_ACCESSOR(int32_t, nextseg);
LFS_DEF_SB_ACCESSOR(int32_t, curseg);
LFS_DEF_SB_ACCESSOR(int32_t, offset);
LFS_DEF_SB_ACCESSOR(int32_t, lastpseg);
LFS_DEF_SB_ACCESSOR(u_int32_t, inopf);
LFS_DEF_SB_ACCESSOR(u_int32_t, minfree);
LFS_DEF_SB_ACCESSOR(uint64_t, maxfilesize);
LFS_DEF_SB_ACCESSOR(u_int32_t, fsbpseg);
LFS_DEF_SB_ACCESSOR(u_int32_t, inopb);
LFS_DEF_SB_ACCESSOR(u_int32_t, ifpb);
LFS_DEF_SB_ACCESSOR(u_int32_t, sepb);
LFS_DEF_SB_ACCESSOR(u_int32_t, nindir);
LFS_DEF_SB_ACCESSOR(u_int32_t, nseg);
LFS_DEF_SB_ACCESSOR(u_int32_t, nspf);
LFS_DEF_SB_ACCESSOR(u_int32_t, cleansz);
LFS_DEF_SB_ACCESSOR(u_int32_t, segtabsz);
LFS_DEF_SB_ACCESSOR(u_int32_t, segmask);
LFS_DEF_SB_ACCESSOR(u_int32_t, segshift);
LFS_DEF_SB_ACCESSOR(u_int64_t, bmask);
LFS_DEF_SB_ACCESSOR(u_int32_t, bshift);
LFS_DEF_SB_ACCESSOR(u_int64_t, ffmask);
LFS_DEF_SB_ACCESSOR(u_int32_t, ffshift);
LFS_DEF_SB_ACCESSOR(u_int64_t, fbmask);
LFS_DEF_SB_ACCESSOR(u_int32_t, fbshift);
LFS_DEF_SB_ACCESSOR(u_int32_t, blktodb);
LFS_DEF_SB_ACCESSOR(u_int32_t, fsbtodb);
LFS_DEF_SB_ACCESSOR(u_int32_t, sushift);
LFS_DEF_SB_ACCESSOR(int32_t, maxsymlinklen);
LFS_DEF_SB_ACCESSOR(u_int32_t, cksum);
LFS_DEF_SB_ACCESSOR(u_int16_t, pflags);
LFS_DEF_SB_ACCESSOR(u_int32_t, nclean);
LFS_DEF_SB_ACCESSOR(int32_t, dmeta);
LFS_DEF_SB_ACCESSOR(u_int32_t, minfreeseg);
LFS_DEF_SB_ACCESSOR(u_int32_t, sumsize);
LFS_DEF_SB_ACCESSOR(u_int64_t, serial);
LFS_DEF_SB_ACCESSOR(u_int32_t, ibsize);
LFS_DEF_SB_ACCESSOR(int32_t, s0addr);
LFS_DEF_SB_ACCESSOR(u_int64_t, tstamp);
LFS_DEF_SB_ACCESSOR(u_int32_t, inodefmt);
LFS_DEF_SB_ACCESSOR(u_int32_t, interleave);
LFS_DEF_SB_ACCESSOR(u_int32_t, ident);
LFS_DEF_SB_ACCESSOR(u_int32_t, resvseg);

/* special-case accessors */

/*
 * the v1 otstamp field lives in what's now dlfs_inopf
 */
#define lfs_sb_getotstamp(fs) lfs_sb_getinopf(fs)
#define lfs_sb_setotstamp(fs, val) lfs_sb_setinopf(fs, val)

/*
 * lfs_sboffs is an array
 */
static __unused inline int32_t
lfs_sb_getsboff(STRUCT_LFS *fs, unsigned n)
{
#ifdef KASSERT /* ugh */
	KASSERT(n < LFS_MAXNUMSB);
#endif
	return fs->lfs_dlfs.dlfs_sboffs[n];
}
static __unused inline void
lfs_sb_setsboff(STRUCT_LFS *fs, unsigned n, int32_t val)
{
#ifdef KASSERT /* ugh */
	KASSERT(n < LFS_MAXNUMSB);
#endif
	fs->lfs_dlfs.dlfs_sboffs[n] = val;
}

/*
 * lfs_fsmnt is a string
 */
static __unused inline const char *
lfs_sb_getfsmnt(STRUCT_LFS *fs)
{
	return fs->lfs_dlfs.dlfs_fsmnt;
}

/* LFS_NINDIR is the number of indirects in a file system block. */
#define	LFS_NINDIR(fs)	(lfs_sb_getnindir(fs))

/* LFS_INOPB is the number of inodes in a secondary storage block. */
#define	LFS_INOPB(fs)	(lfs_sb_getinopb(fs))
/* LFS_INOPF is the number of inodes in a fragment. */
#define LFS_INOPF(fs)	(lfs_sb_getinopf(fs))

#define	lfs_blksize(fs, ip, lbn) \
	(((lbn) >= ULFS_NDADDR || (ip)->i_ffs1_size >= ((lbn) + 1) << lfs_sb_getbshift(fs)) \
	    ? lfs_sb_getbsize(fs) \
	    : (lfs_fragroundup(fs, lfs_blkoff(fs, (ip)->i_ffs1_size))))
#define	lfs_blkoff(fs, loc)	((int)((loc) & lfs_sb_getbmask(fs)))
#define lfs_fragoff(fs, loc)    /* calculates (loc % fs->lfs_fsize) */ \
    ((int)((loc) & lfs_sb_getffmask(fs)))

#if defined(_KERNEL)
#define	LFS_FSBTODB(fs, b)	((b) << (lfs_sb_getffshift(fs) - DEV_BSHIFT))
#define	LFS_DBTOFSB(fs, b)	((b) >> (lfs_sb_getffshift(fs) - DEV_BSHIFT))
#else
#define	LFS_FSBTODB(fs, b)	((b) << lfs_sb_getfsbtodb(fs))
#define	LFS_DBTOFSB(fs, b)	((b) >> lfs_sb_getfsbtodb(fs))
#endif

#define	lfs_lblkno(fs, loc)	((loc) >> lfs_sb_getbshift(fs))
#define	lfs_lblktosize(fs, blk)	((blk) << lfs_sb_getbshift(fs))

#define lfs_fsbtob(fs, b)	((b) << lfs_sb_getffshift(fs))
#define lfs_btofsb(fs, b)	((b) >> lfs_sb_getffshift(fs))

#define lfs_numfrags(fs, loc)	/* calculates (loc / fs->lfs_fsize) */	\
	((loc) >> lfs_sb_getffshift(fs))
#define lfs_blkroundup(fs, size)/* calculates roundup(size, lfs_sb_getbsize(fs)) */ \
	((off_t)(((size) + lfs_sb_getbmask(fs)) & (~lfs_sb_getbmask(fs))))
#define lfs_fragroundup(fs, size)/* calculates roundup(size, fs->lfs_fsize) */ \
	((off_t)(((size) + lfs_sb_getffmask(fs)) & (~lfs_sb_getffmask(fs))))
#define lfs_fragstoblks(fs, frags)/* calculates (frags / fs->fs_frag) */ \
	((frags) >> lfs_sb_getfbshift(fs))
#define lfs_blkstofrags(fs, blks)/* calculates (blks * fs->fs_frag) */ \
	((blks) << lfs_sb_getfbshift(fs))
#define lfs_fragnum(fs, fsb)	/* calculates (fsb % fs->lfs_frag) */	\
	((fsb) & ((fs)->lfs_frag - 1))
#define lfs_blknum(fs, fsb)	/* calculates rounddown(fsb, fs->lfs_frag) */ \
	((fsb) &~ ((fs)->lfs_frag - 1))
#define lfs_dblksize(fs, dp, lbn) \
	(((lbn) >= ULFS_NDADDR || (dp)->di_size >= ((lbn) + 1) << lfs_sb_getbshift(fs)) \
	    ? lfs_sb_getbsize(fs) \
	    : (lfs_fragroundup(fs, lfs_blkoff(fs, (dp)->di_size))))

#define	lfs_segsize(fs)	((fs)->lfs_version == 1 ?	     		\
			   lfs_lblktosize((fs), lfs_sb_getssize(fs)) :	\
			   lfs_sb_getssize(fs))
#define lfs_segtod(fs, seg) (((fs)->lfs_version == 1     ?	    	\
			   lfs_sb_getssize(fs) << lfs_sb_getblktodb(fs) : \
			   lfs_btofsb((fs), lfs_sb_getssize(fs))) * (seg))
#define	lfs_dtosn(fs, daddr)	/* block address to segment number */	\
	((uint32_t)(((daddr) - lfs_sb_gets0addr(fs)) / lfs_segtod((fs), 1)))
#define lfs_sntod(fs, sn)	/* segment number to disk address */	\
	((daddr_t)(lfs_segtod((fs), (sn)) + lfs_sb_gets0addr(fs)))

/*
 * Macros for determining free space on the disk, with the variable metadata
 * of segment summaries and inode blocks taken into account.
 */
/*
 * Estimate number of clean blocks not available for writing because
 * they will contain metadata or overhead.  This is calculated as
 *
 *		E = ((C * M / D) * D + (0) * (T - D)) / T
 * or more simply
 *		E = (C * M) / T
 *
 * where
 * C is the clean space,
 * D is the dirty space,
 * M is the dirty metadata, and
 * T = C + D is the total space on disk.
 *
 * This approximates the old formula of E = C * M / D when D is close to T,
 * but avoids falsely reporting "disk full" when the sample size (D) is small.
 */
#define LFS_EST_CMETA(F) (int32_t)((					\
	(lfs_sb_getdmeta(F) * (int64_t)lfs_sb_getnclean(F)) / 		\
	(lfs_sb_getnseg(F))))

/* Estimate total size of the disk not including metadata */
#define LFS_EST_NONMETA(F) (lfs_sb_getdsize(F) - lfs_sb_getdmeta(F) - LFS_EST_CMETA(F))

/* Estimate number of blocks actually available for writing */
#define LFS_EST_BFREE(F) (lfs_sb_getbfree(F) > LFS_EST_CMETA(F) ?	     \
			  lfs_sb_getbfree(F) - LFS_EST_CMETA(F) : 0)

/* Amount of non-meta space not available to mortal man */
#define LFS_EST_RSVD(F) (int32_t)((LFS_EST_NONMETA(F) *			     \
				   (u_int64_t)lfs_sb_getminfree(F)) /	     \
				  100)

/* Can credential C write BB blocks */
#define ISSPACE(F, BB, C)						\
	((((C) == NOCRED || kauth_cred_geteuid(C) == 0) &&		\
	  LFS_EST_BFREE(F) >= (BB)) ||					\
	 (kauth_cred_geteuid(C) != 0 && IS_FREESPACE(F, BB)))

/* Can an ordinary user write BB blocks */
#define IS_FREESPACE(F, BB)						\
	  (LFS_EST_BFREE(F) >= (BB) + LFS_EST_RSVD(F))

/*
 * The minimum number of blocks to create a new inode.  This is:
 * directory direct block (1) + ULFS_NIADDR indirect blocks + inode block (1) +
 * ifile direct block (1) + ULFS_NIADDR indirect blocks = 3 + 2 * ULFS_NIADDR blocks.
 */
#define LFS_NRESERVE(F) (lfs_btofsb((F), (2 * ULFS_NIADDR + 3) << lfs_sb_getbshift(F)))



#endif /* _UFS_LFS_LFS_ACCESSORS_H_ */
