/*	$NetBSD: buf.h,v 1.34.2.2 2000/12/08 09:19:41 bouyer Exp $	*/

/*-
 * Copyright (c) 1999, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	@(#)buf.h	8.9 (Berkeley) 3/30/95
 */

#ifndef _SYS_BUF_H_
#define	_SYS_BUF_H_

#include <sys/pool.h>
#include <sys/queue.h>

#define NOLIST ((struct buf *)0x87654321)

/*
 * To avoid including <ufs/ffs/softdep.h> 
 */   
LIST_HEAD(workhead, worklist);

/*
 * Device driver buffer queue.
 */
struct buf_queue {
	TAILQ_HEAD(bufq_head, buf) bq_head; /* actual list of buffers */
	struct buf *bq_barrier;		    /* last B_ORDERED request */
};

#ifdef _KERNEL
#define	BUFQ_FIRST(bufq)	TAILQ_FIRST(&(bufq)->bq_head)
#define	BUFQ_NEXT(bp)		TAILQ_NEXT((bp), b_actq)

#define	BUFQ_INIT(bufq)							\
do {									\
	TAILQ_INIT(&(bufq)->bq_head);					\
	(bufq)->bq_barrier = NULL;					\
} while (/*CONSTCOND*/0)

#define	BUFQ_INSERT_HEAD(bufq, bp)					\
do {									\
	TAILQ_INSERT_HEAD(&(bufq)->bq_head, (bp), b_actq);		\
	if (((bp)->b_flags & B_ORDERED) != 0 &&				\
	    (bufq)->bq_barrier == NULL)					\
		(bufq)->bq_barrier = (bp);				\
} while (/*CONSTCOND*/0)

#define	BUFQ_INSERT_TAIL(bufq, bp)					\
do {									\
	TAILQ_INSERT_TAIL(&(bufq)->bq_head, (bp), b_actq);		\
	if (((bp)->b_flags & B_ORDERED) != 0)				\
		(bufq)->bq_barrier = (bp);				\
} while (/*CONSTCOND*/0)

#define	BUFQ_INSERT_AFTER(bufq, lbp, bp)				\
do {									\
	KASSERT((bufq)->bq_barrier == NULL);				\
	KASSERT(((bp)->b_flags & B_ORDERED) == 0);			\
	TAILQ_INSERT_AFTER(&(bufq)->bq_head, (lbp), (bp), b_actq);	\
} while (/*CONSTCOND*/0)

#define	BUFQ_INSERT_BEFORE(bufq, lbp, bp)				\
do {									\
	KASSERT((bufq)->bq_barrier == NULL);				\
	KASSERT(((bp)->b_flags & B_ORDERED) == 0);			\
	TAILQ_INSERT_BEFORE((lbp), (bp), b_actq);			\
} while (/*CONSTCOND*/0)

#define	BUFQ_REMOVE(bufq, bp)						\
do {									\
	if ((bufq)->bq_barrier == (bp))					\
		(bufq)->bq_barrier = TAILQ_PREV((bp), bufq_head, b_actq); \
	TAILQ_REMOVE(&(bufq)->bq_head, (bp), b_actq);			\
} while (/*CONSTCOND*/0)
#endif /* _KERNEL */

/*
 * These are currently used only by the soft dependency code, hence
 * are stored once in a global variable. If other subsystems wanted
 * to use these hooks, a pointer to a set of bio_ops could be added
 * to each buffer.
 */
struct buf;
struct mount;
struct vnode;
extern struct bio_ops {
 	void	(*io_start) __P((struct buf *));
 	void	(*io_complete) __P((struct buf *));
 	void	(*io_deallocate) __P((struct buf *));
 	int	(*io_fsync) __P((struct vnode *));
 	int	(*io_sync) __P((struct mount *));
	void	(*io_movedeps) __P((struct buf *, struct buf *));
	int	(*io_countdeps) __P((struct buf *, int));
	void	(*io_pageiodone) __P((struct buf *));
} bioops;

/*
 * The buffer header describes an I/O operation in the kernel.
 */
struct buf {
	LIST_ENTRY(buf) b_hash;		/* Hash chain. */
	LIST_ENTRY(buf) b_vnbufs;	/* Buffer's associated vnode. */
	TAILQ_ENTRY(buf) b_freelist;	/* Free list position if not active. */
	TAILQ_ENTRY(buf) b_actq;	/* Device driver queue when active. */
	struct  proc *b_proc;		/* Associated proc; NULL if kernel. */
	volatile long	b_flags;	/* B_* flags. */
	int	b_error;		/* Errno value. */
	long	b_bufsize;		/* Allocated buffer size. */
	long	b_bcount;		/* Valid bytes in buffer. */
	long	b_resid;		/* Remaining I/O. */
	dev_t	b_dev;			/* Device associated with buffer. */
	struct {
		caddr_t	b_addr;		/* Memory, superblocks, indirect etc. */
	} b_un;
	void	*b_saveaddr;		/* Original b_addr for physio. */
	daddr_t	b_lblkno;		/* Logical block number. */
	daddr_t	b_blkno;		/* Underlying physical block number
					   (partition relative) */
	daddr_t	b_rawblkno;		/* Raw underlying physical block
					   number (not partition relative) */
					/* Function to call upon completion. */
	void	(*b_iodone) __P((struct buf *));
	struct	vnode *b_vp;		/* File vnode. */
	void	*b_private;		/* Private data for owner */
	off_t	b_dcookie;		/* Offset cookie if dir block */
	struct  workhead b_dep;		/* List of filesystem dependencies. */
};

/*
 * For portability with historic industry practice, the cylinder number has
 * to be maintained in the `b_resid' field.
 */
#define	b_cylinder b_resid		/* Cylinder number for disksort(). */

/* Device driver compatibility definitions. */
#define	b_data	 b_un.b_addr		/* b_un.b_addr is not changeable. */

/*
 * These flags are kept in b_flags.
 */
#define	B_AGE		0x00000001	/* Move to age queue when I/O done. */
#define	B_NEEDCOMMIT	0x00000002	/* Needs committing to stable storage */
#define	B_ASYNC		0x00000004	/* Start I/O, do not wait. */
#define	B_BAD		0x00000008	/* Bad block revectoring in progress. */
#define	B_BUSY		0x00000010	/* I/O in progress. */
#define B_SCANNED	0x00000020	/* Block already pushed during sync */
#define	B_CALL		0x00000040	/* Call b_iodone from biodone. */
#define	B_DELWRI	0x00000080	/* Delay I/O until buffer reused. */
#define	B_DIRTY		0x00000100	/* Dirty page to be pushed out async. */
#define	B_DONE		0x00000200	/* I/O completed. */
#define	B_EINTR		0x00000400	/* I/O was interrupted */
#define	B_ERROR		0x00000800	/* I/O error occurred. */
#define	B_GATHERED	0x00001000	/* LFS: already in a segment. */
#define	B_INVAL		0x00002000	/* Does not contain valid info. */
#define	B_LOCKED	0x00004000	/* Locked in core (not reusable). */
#define	B_NOCACHE	0x00008000	/* Do not cache block after use. */
#define	B_ORDERED	0x00010000	/* ordered I/O request */
#define	B_CACHE		0x00020000	/* Bread found us in the cache. */
#define	B_PHYS		0x00040000	/* I/O to user memory. */
#define	B_RAW		0x00080000	/* Set by physio for raw transfers. */
#define	B_READ		0x00100000	/* Read buffer. */
#define	B_TAPE		0x00200000	/* Magnetic tape I/O. */
#define	B_WANTED	0x00800000	/* Process wants this buffer. */
#define	B_WRITE		0x00000000	/* Write buffer (pseudo flag). */
#define	B_XXX		0x02000000	/* Debugging flag. */
#define	B_VFLUSH	0x04000000	/* Buffer is being synced. */
#define	B_PDAEMON	0x10000000	/* I/O initiated by pagedaemon. */

/*
 * This structure describes a clustered I/O.  It is stored in the b_saveaddr
 * field of the buffer on which I/O is done.  At I/O completion, cluster
 * callback uses the structure to parcel I/O's to individual buffers, and
 * then free's this structure.
 */
struct cluster_save {
	long	bs_bcount;		/* Saved b_bcount. */
	long	bs_bufsize;		/* Saved b_bufsize. */
	void	*bs_saveaddr;		/* Saved b_addr. */
	int	bs_nchildren;		/* Number of associated buffers. */
	struct buf **bs_children;	/* List of associated buffers. */
};

/*
 * Zero out the buffer's data area.
 */
#define	clrbuf(bp)							\
do {									\
	memset((bp)->b_data, 0, (u_int)(bp)->b_bcount);			\
	(bp)->b_resid = 0;						\
} while (0)

/* Flags to low-level allocation routines. */
#define B_CLRBUF	0x01	/* Request allocated buffer be cleared. */
#define B_SYNC		0x02	/* Do all allocations synchronously. */

#ifdef _KERNEL

extern	int nbuf;		/* The number of buffer headers */
extern	struct buf *buf;	/* The buffer headers. */
extern	char *buffers;		/* The buffer contents. */
extern	int bufpages;		/* Number of memory pages in the buffer pool. */
extern	int nswbuf;		/* Number of swap I/O buffer headers. */

extern struct pool bufpool;	/* I/O buf pool */

__BEGIN_DECLS
void	allocbuf __P((struct buf *, int));
void	bawrite __P((struct buf *));
void	bowrite __P((struct buf *));
void	bdirty __P((struct buf *));
void	bdwrite __P((struct buf *));
void	biodone __P((struct buf *));
int	biowait __P((struct buf *));
int	bread __P((struct vnode *, daddr_t, int,
		   struct ucred *, struct buf **));
int	breada __P((struct vnode *, daddr_t, int, daddr_t, int,
		    struct ucred *, struct buf **));
int	breadn __P((struct vnode *, daddr_t, int, daddr_t *, int *, int,
		    struct ucred *, struct buf **));
void	brelse __P((struct buf *));
void	bremfree __P((struct buf *));
void	bufinit __P((void));
int	bwrite __P((struct buf *));
void	cluster_callback __P((struct buf *));
int	cluster_read __P((struct vnode *, u_quad_t, daddr_t, long,
			  struct ucred *, struct buf **));
void	cluster_write __P((struct buf *, u_quad_t));
struct buf *getblk __P((struct vnode *, daddr_t, int, int, int));
struct buf *geteblk __P((int));
struct buf *getnewbuf __P((int slpflag, int slptimeo));
struct buf *incore __P((struct vnode *, daddr_t));

void	minphys __P((struct buf *bp));
int	physio __P((void (*strategy)(struct buf *), struct buf *bp, dev_t dev,
		    int flags, void (*minphys)(struct buf *), struct uio *uio));

void  brelvp __P((struct buf *));
void  reassignbuf __P((struct buf *, struct vnode *));
void  bgetvp __P((struct vnode *, struct buf *));
#ifdef DDB
void	vfs_buf_print __P((struct buf *, int, void (*)(const char *, ...)));
#endif
__END_DECLS
#endif
#endif /* !_SYS_BUF_H_ */
