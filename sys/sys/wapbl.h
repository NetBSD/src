/*	$NetBSD: wapbl.h,v 1.2.4.2 2008/09/28 10:41:05 mjf Exp $	*/

/*-
 * Copyright (c) 2003,2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Wasabi Systems, Inc.
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

#ifndef _SYS_WAPBL_H
#define	_SYS_WAPBL_H

#include <sys/mutex.h>

#include <miscfs/specfs/specdev.h>

/* This header file describes the api and data structures for
 * write ahead physical block logging (WAPBL) support.
 */

#if defined(_KERNEL_OPT)
#include "opt_wapbl.h"
#endif

#ifdef WAPBL_DEBUG
#ifndef WAPBL_DEBUG_PRINT
#define	WAPBL_DEBUG_PRINT (WAPBL_PRINT_REPLAY | WAPBL_PRINT_OPEN)
#endif

#if 0
#define	WAPBL_DEBUG_BUFBYTES
#define	WAPBL_DEBUG_SERIALIZE
#endif

#endif

#ifdef WAPBL_DEBUG_PRINT

enum {
	WAPBL_PRINT_OPEN = 0x1,
	WAPBL_PRINT_FLUSH = 0x2,
	WAPBL_PRINT_TRUNCATE = 0x4,
	WAPBL_PRINT_TRANSACTION = 0x8,
	WAPBL_PRINT_BUFFER = 0x10,
	WAPBL_PRINT_BUFFER2 = 0x20,
	WAPBL_PRINT_ALLOC = 0x40,
	WAPBL_PRINT_INODE = 0x80,
	WAPBL_PRINT_WRITE = 0x100,
	WAPBL_PRINT_IO = 0x200,
	WAPBL_PRINT_REPLAY = 0x400,
	WAPBL_PRINT_ERROR = 0x800,
	WAPBL_PRINT_DISCARD = 0x1000,
	WAPBL_PRINT_BIODONE = 0x2000,
};

#define	WAPBL_PRINTF(mask, a) if (wapbl_debug_print & (mask)) printf  a
extern int wapbl_debug_print;
#else
#define	WAPBL_PRINTF(mask, a)
#endif

/****************************************************************/

/* The WAPBL journal layout.
 * 
 * The journal consists of a header followed by a circular buffer
 * region.  The circular data area is described by the header
 * wc_circ_off, wc_circ_size, wc_head and wc_tail fields as bytes
 * from the start of the journal header.  New records are inserted
 * at wc_head and the oldest valid record can be found at wc_tail.
 * When ((wc_head == wc_tail) && (wc_head == 0)), the journal is empty.
 * The condition of ((wc_head == wc_tail) && (wc_head != 0))
 * indicates a full journal, although this condition is rare.
 *
 * The journal header as well as its records are marked by a 32bit
 * type tag and length for ease of parsing.  Journal records are
 * padded so as to fall on journal device block boundaries.
 * (XXX i think there is currently a bug wrt WC_BLOCKS not ending
 * correctly on a journal device block boundary. this would need
 * to be fixed if the journal blocksize does not match filesystem.)
 */

/*
 * The following are the 4 record types used by the journal:
 * Each tag indicates journal data organized by one of the
 * structures used below.
 */
enum {
	WAPBL_WC_HEADER = 0x5741424c,	/* "WABL", struct wapbl_wc_header */
	WAPBL_WC_INODES,		/* struct wapbl_wc_inodelist */
	WAPBL_WC_REVOCATIONS,		/* struct wapbl_wc_blocklist */
	WAPBL_WC_BLOCKS,		/* struct wapbl_wc_blocklist */
};

/* null entry (on disk) */
/* This structure isn't used directly, but shares its header
 * layout with all the other log structures for the purpose
 * of reading a log structure and determining its type
 */
struct wapbl_wc_null {
	uint32_t	wc_type;	/* WAPBL_WC_* */
	int32_t		wc_len;
	uint8_t		wc_spare[0];	/* actually longer */
};

/* journal header (on-disk)
 * This record is found at the start of the
 * journal, but not within the circular buffer region.  As well as
 * describing the journal parameters and matching filesystem, it
 * additionally serves as the atomic update record for journal
 * updates.
 */
struct wapbl_wc_header {
	uint32_t	wc_type;	/* WAPBL_WC_HEADER log magic number */
	int32_t		wc_len;		/* length of this journal entry */
	uint32_t	wc_checksum;
	uint32_t	wc_generation;
	int32_t		wc_fsid[2];
	uint64_t	wc_time;
	uint32_t	wc_timensec;
	uint32_t	wc_version;
	uint32_t	wc_log_dev_bshift;
	uint32_t	wc_fs_dev_bshift;
	int64_t		wc_head;
	int64_t		wc_tail;
	int64_t		wc_circ_off;	/* offset of of circ buffer region */
	int64_t		wc_circ_size;	/* size of circular buffer region */
	uint8_t		wc_spare[0];	/* actually longer */
};

/* list of blocks (on disk)
 * This record is used to describe a set of filesystem blocks,
 * and is used with two type tags, WAPBL_WC_BLOCKS and
 * WAPBL_WC_REVOCATIONS.
 * 
 * For WAPBL_WC_BLOCKS, a copy of each listed block can be found
 * starting at the next log device blocksize boundary.  starting at
 * one log device block since the start of the record.  This contains
 * the bulk of the filesystem journal data which is written using
 * these records before being written into the filesystem.
 *
 * The WAPBL_WC_REVOCATIONS record is used to indicate that any
 * previously listed blocks should not be written into the filesystem.
 * This is important so that deallocated and reallocated data blocks
 * do not get overwritten with stale data from the journal.  The
 * revocation records to not contain a copy of any actual block data.
 */
struct wapbl_wc_blocklist {
	uint32_t	wc_type; /* WAPBL_WC_{REVOCATIONS,BLOCKS} */
	int32_t		wc_len;
	int32_t		wc_blkcount;
	int32_t		wc_unused;
	struct {
		int64_t	wc_daddr;
		int32_t	wc_unused;
		int32_t	wc_dlen;
	} wc_blocks[0];			/* actually longer */
};

/* list of inodes (on disk)
 * This record is used to describe the set of inodes which
 * may be allocated but are unlinked.  Inodes end up listed here
 * while they are in the process of being initialized and
 * deinitialized.  Inodes unlinked while in use by a process
 * will be listed here and the actual deletion must be completed
 * on journal replay.
 */
struct wapbl_wc_inodelist {
	uint32_t	wc_type; /* WAPBL_WC_INODES */
	int32_t		wc_len;
	int32_t		wc_inocnt;
	int32_t		wc_clear;	/* set if previously listed inodes 
					   hould be ignored */
	struct {
		uint32_t wc_inumber;
		uint32_t wc_imode;
	} wc_inodes[0];		/* actually longer */
};

/****************************************************************/

#include <sys/queue.h>
#include <sys/vnode.h>
#include <sys/buf.h>

typedef void (*wapbl_flush_fn_t)(struct mount *, daddr_t *, int *, int);

#ifdef _KERNEL

struct wapbl_entry;
struct wapbl_wc_header;
struct wapbl_replay;
struct wapbl;

/*
 * This structure holds per transaction log information
 */
struct wapbl_entry {
	struct wapbl *we_wapbl;
	SIMPLEQ_ENTRY(wapbl_entry) we_entries;
	size_t we_bufcount;		/* Count of unsynced buffers */
	size_t we_reclaimable_bytes;	/* Number on disk bytes for this
					   transaction */
	int	we_error;
#ifdef WAPBL_DEBUG_BUFBYTES
	size_t we_unsynced_bufbytes;	/* Byte count of unsynced buffers */
#endif
};

void	wapbl_init(void);

/* Start using a log */
int	wapbl_start(struct wapbl **, struct mount *, struct vnode *, daddr_t,
		    size_t, size_t, struct wapbl_replay *,
		    wapbl_flush_fn_t, wapbl_flush_fn_t);

/* Discard the current transaction, potentially dangerous */
void	wapbl_discard(struct wapbl *);

/* stop using a log */
int	wapbl_stop(struct wapbl *, int);

/*
 * Begin a new transaction or increment transaction recursion
 * level if called while a transaction is already in progress
 * by the current process.
 */
int	wapbl_begin(struct wapbl *, const char *, int);


/* End a transaction or decrement the transaction recursion level */
void	wapbl_end(struct wapbl *);

/*
 * Add a new buffer to the current transaction.  The buffers
 * data will be copied to the current transaction log and the
 * buffer will be marked B_LOCKED so that it will not be
 * flushed to disk by the syncer or reallocated.
 */
void	wapbl_add_buf(struct wapbl *, struct buf *);

/* Remove a buffer from the current transaction. */
void	wapbl_remove_buf(struct wapbl *, struct buf *);

void	wapbl_resize_buf(struct wapbl *, struct buf *, long, long);

/*
 * This will flush all completed transactions to disk and
 * start asynchronous writes on the associated buffers
 */
int	wapbl_flush(struct wapbl *, int);

/*
 * Inodes that are allocated but have zero link count
 * must be registered with the current transaction
 * so they may be recorded in the log and cleaned up later.
 * registration/unregistration of ino numbers already registered is ok.
 */
void	wapbl_register_inode(struct wapbl *, ino_t, mode_t);
void	wapbl_unregister_inode(struct wapbl *, ino_t, mode_t);

/*
 * Metadata block deallocations must be registered so
 * that revocations records can be written and to prevent
 * the corresponding blocks from being reused as data
 * blocks until the log is on disk.
 */
void	wapbl_register_deallocation(struct wapbl *, daddr_t, int);

void	wapbl_jlock_assert(struct wapbl *wl);
void	wapbl_junlock_assert(struct wapbl *wl);

void	wapbl_print(struct wapbl *wl, int full, void (*pr)(const char *, ...));

#if defined(WAPBL_DEBUG) || defined(DDB)
void	wapbl_dump(struct wapbl *);
#endif

void	wapbl_biodone(struct buf *);

extern struct wapbl_ops wapbl_ops;

static __inline struct mount *
wapbl_vptomp(struct vnode *vp)
{
	struct mount *mp;

	mp = NULL;
	if (vp != NULL) {
		if (vp->v_type == VBLK)
			mp = vp->v_specmountpoint;
		else
			mp = vp->v_mount;
	}

	return mp;
}

static __inline bool
wapbl_vphaswapbl(struct vnode *vp)
{
	struct mount *mp;

	if (vp == NULL)
		return false;

	mp = wapbl_vptomp(vp);
	if (mp && mp->mnt_wapbl)
		return true;
	else
		return false;
}

#endif /* _KERNEL */

/****************************************************************/
/* Replay support */

struct wapbl_replay {
	struct vnode *wr_logvp;
	struct vnode *wr_devvp;
	daddr_t wr_logpbn;

	struct wapbl_wc_header wr_wc_header;
	void *wr_scratch;

	LIST_HEAD(wapbl_blk_head, wapbl_blk) *wr_blkhash;
	u_long wr_blkhashmask;
	int wr_blkhashcnt;

	off_t wr_inodeshead;
	off_t wr_inodestail;
	int wr_inodescnt;
	struct {
		uint32_t wr_inumber;
		uint32_t wr_imode;
	} *wr_inodes;
};

#define	wapbl_replay_isopen(wr) ((wr)->wr_scratch != 0)

int	wapbl_replay_isopen1(struct wapbl_replay *);
int	wapbl_replay_start(struct wapbl_replay **, struct vnode *,
	daddr_t, size_t, size_t);
void	wapbl_replay_stop(struct wapbl_replay *);
void	wapbl_replay_free(struct wapbl_replay *);
int	wapbl_replay_verify(struct wapbl_replay *, struct vnode *);
int	wapbl_replay_write(struct wapbl_replay *, struct vnode *);
int	wapbl_replay_read(struct wapbl_replay *, void *, daddr_t, long);

/****************************************************************/

/* Supply this to provide i/o support */
int wapbl_write(void *, size_t, struct vnode *, daddr_t);
int wapbl_read(void *, size_t, struct vnode *, daddr_t);

/****************************************************************/

#endif /* !_SYS_WAPBL_H */
