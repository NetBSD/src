/*	$NetBSD: lockf.h,v 1.15 2004/04/25 16:42:43 simonb Exp $	*/

/*
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Scooter Morris at Genentech Inc.
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
 *	@(#)lockf.h	8.2 (Berkeley) 10/26/94
 */

#ifndef _SYS_LOCKF_H_
#define _SYS_LOCKF_H_

/*
 * The lockf structure is a kernel structure which contains the information
 * associated with a byte range lock.  The lockf structures are linked into
 * the inode structure. Locks are sorted by the starting byte of the lock for
 * efficiency.
 *
 * lf_next is used for two purposes, depending on whether the lock is
 * being held, or is in conflict with an existing lock.  If this lock
 * is held, it indicates the next lock on the same vnode.
 * For pending locks, if lock->lf_next is non-NULL, then lock->lf_block
 * must be queued on the lf_blkhd TAILQ of lock->lf_next.
 */

TAILQ_HEAD(locklist, lockf);

struct lockf {
	short	lf_flags;	 /* Lock semantics: F_POSIX, F_FLOCK, F_WAIT */
	short	lf_type;	 /* Lock type: F_RDLCK, F_WRLCK */
	off_t	lf_start;	 /* The byte # of the start of the lock */
	off_t	lf_end;		 /* The byte # of the end of the lock (-1=EOF)*/
	void	*lf_id;		 /* process or file description holding lock */
	struct	lwp *lf_lwp;	 /* LWP waiting for lock */
	struct	lockf **lf_head; /* Back pointer to the head of lockf list */
	struct	lockf *lf_next;	 /* Next lock on this vnode, or blocking lock */
	struct  locklist lf_blkhd; /* List of requests blocked on this lock */
	TAILQ_ENTRY(lockf) lf_block;/* A request waiting for a lock */
};

/* Maximum length of sleep chains to traverse to try and detect deadlock. */
#define MAXDEPTH 50

#ifdef _KERNEL
int lf_advlock(struct vop_advlock_args *, struct lockf **, off_t);
#endif /* _KERNEL */

#endif /* !_SYS_LOCKF_H_ */
