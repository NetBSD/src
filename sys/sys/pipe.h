/*
 * Copyright (c) 1996 John S. Dyson
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice immediately at the beginning of the file, without modification,
 *    this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Absolutely no warranty of function or purpose is made by the author
 *    John S. Dyson.
 * 4. This work was done expressly for inclusion into FreeBSD.  Other use
 *    is allowed if this notation is included.
 * 5. Modifications may be freely made to this file if the above conditions
 *    are met.
 *
 * $FreeBSD: src/sys/sys/pipe.h,v 1.18 2002/02/27 07:35:59 alfred Exp $
 */

#ifndef _SYS_PIPE_H_
#define _SYS_PIPE_H_

#ifndef _KERNEL
#ifdef __FreeBSD__
#include <sys/time.h>			/* for struct timeval */
#include <sys/selinfo.h>		/* for struct selinfo */
#include <vm/vm.h>			/* for vm_page_t */
#include <machine/param.h>		/* for PAGE_SIZE */
#endif
#ifdef __NetBSD__
#include <sys/select.h>			/* for struct selinfo */
#endif
#endif

/*
 * Pipe buffer size, keep moderate in value, pipes take kva space.
 */
#ifndef PIPE_SIZE
#define PIPE_SIZE	16384
#endif

#ifndef BIG_PIPE_SIZE
#define BIG_PIPE_SIZE	(4*PIPE_SIZE)
#endif

/*
 * Maximum size of kva for direct write transfer. If the amount
 * of data in buffer is larger, it would be transferred in chunks of this
 * size. This kva memory is freed after use if amount of pipe kva memory
 * is bigger than limitpipekva.
 */
#ifndef PIPE_DIRECT_CHUNK
#define PIPE_DIRECT_CHUNK	(1*1024*1024)
#endif

/*
 * PIPE_MINDIRECT MUST be smaller than PIPE_SIZE and MUST be bigger
 * than PIPE_BUF.
 */
#ifndef PIPE_MINDIRECT
#define PIPE_MINDIRECT	8192
#endif

/*
 * Pipe buffer information.
 * Separate in, out, cnt are used to simplify calculations.
 * Buffered write is active when the buffer.cnt field is set.
 */
struct pipebuf {
	size_t	cnt;		/* number of chars currently in buffer */
	u_int	in;		/* in pointer */
	u_int	out;		/* out pointer */
	size_t	size;		/* size of buffer */
	caddr_t	buffer;		/* kva of buffer */
#ifdef __FreeBSD__
	struct	vm_object *object;	/* VM object containing buffer */
#endif
};

/*
 * Information to support direct transfers between processes for pipes.
 */
#if defined(__FreeBSD__)
#define PIPENPAGES	(BIG_PIPE_SIZE / PAGE_SIZE + 1)
struct pipemapping {
	vm_offset_t	kva;		/* kernel virtual address */
	vm_size_t	cnt;		/* number of chars in buffer */
	vm_size_t	pos;		/* current position of transfer */
	int		npages;		/* number of pages */
	vm_page_t	ms[PIPENPAGES];	/* pages in source process */
};
#elif defined(__NetBSD__)
struct pipemapping {
	vaddr_t		kva;		/* kernel virtual address */
	vsize_t		cnt;		/* number of chars in buffer */
	voff_t		pos;		/* current position within page */
	int		npages;		/* how many pages allocated */
	struct vm_page	**pgs;		/* pointers to the pages */
};
#endif

/*
 * Bits in pipe_state.
 */
#define PIPE_ASYNC	0x004	/* Async? I/O. */
#define PIPE_WANTR	0x008	/* Reader wants some characters. */
#define PIPE_WANTW	0x010	/* Writer wants space to put characters. */
#define PIPE_WANTCLOSE	0x020	/* Pipe is wanted to be run-down. */
#define PIPE_SEL	0x040	/* Pipe has a select active. */
#define PIPE_EOF	0x080	/* Pipe is in EOF condition. */
#define PIPE_LOCKFL	0x100	/* Process has exclusive access to pointers/data. */
#define PIPE_LWANT	0x200	/* Process wants exclusive access to pointers/data. */
#define PIPE_DIRECTW	0x400	/* Pipe direct write active. */
#define PIPE_SIGNALR	0x800	/* Do selwakeup() on read(2) */

/*
 * Per-pipe data structure.
 * Two of these are linked together to produce bi-directional pipes.
 */
struct pipe {
	struct	pipebuf pipe_buffer;	/* data storage */
	struct	pipemapping pipe_map;	/* pipe mapping for direct I/O */
	struct	selinfo pipe_sel;	/* for compat with select */
#ifdef __FreeBSD__
	struct	timespec pipe_atime;	/* time of last access */
	struct	timespec pipe_mtime;	/* time of last modify */
	struct	timespec pipe_ctime;	/* time of status change */
	struct	sigio *pipe_sigio;	/* information for async I/O */
	struct	mtx *pipe_mtxp;		/* shared mutex between both pipes */
#elif defined(__NetBSD__)
	struct	timeval pipe_atime;	/* time of last access */
	struct	timeval pipe_mtime;	/* time of last modify */
	struct	timeval pipe_ctime;	/* time of status change */
	gid_t	pipe_pgid;		/* process group for sigio */
	struct	lock pipe_lock;		/* pipe lock */
#endif
	struct	pipe *pipe_peer;	/* link with other direction */
	u_int	pipe_state;		/* pipe status info */
	int	pipe_busy;		/* busy flag, mostly to handle rundown sanely */
};

#ifdef __FreeBSD__
#define PIPE_MTX(pipe)		(pipe)->pipe_mtxp
#define PIPE_LOCK(pipe)		mtx_lock(PIPE_MTX(pipe))
#define PIPE_UNLOCK(pipe)	mtx_unlock(PIPE_MTX(pipe))
#define PIPE_LOCK_ASSERT(pipe, type)  mtx_assert(PIPE_MTX(pipe), (type))
#endif /* FreeBSD */

#ifdef __NetBSD__
/*
 * KERN_PIPE subtypes
 */
#define	KERN_PIPE_MAXKVASZ		1	/* maximum kva size */
#define	KERN_PIPE_LIMITKVA		2	/* */
#define	KERN_PIPE_MAXBIGPIPES		3	/* maximum # of "big" pipes */
#define	KERN_PIPE_NBIGPIPES		4	/* current number of "big" p. */
#define	KERN_PIPE_KVASIZE		5	/* current pipe kva size */
#define	KERN_PIPE_MAXID			6

#define	CTL_PIPE_NAMES { \
	{ 0, 0 }, \
	{ "maxkvasz", CTLTYPE_INT }, \
	{ "maxloankvasz", CTLTYPE_INT }, \
	{ "maxbigpipes", CTLTYPE_INT }, \
	{ "nbigpipes", CTLTYPE_INT }, \
	{ "kvasize", CTLTYPE_INT }, \
}

#ifdef _KERNEL
void pipe_init __P((void));
int sysctl_dopipe __P((int *, u_int, void *, size_t *, void *, size_t));

/* XXXSMP use spinlock ? clear for now */
#define PIPE_MTX(pipe)		NULL
#define PIPE_LOCK(pipe)	
#define PIPE_UNLOCK(pipe)
#define PIPE_LOCK_ASSERT(pipe, type)

#endif /* _KERNEL */

#endif /* NetBSD */

#endif /* !_SYS_PIPE_H_ */
