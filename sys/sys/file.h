/*	$NetBSD: file.h,v 1.37 2003/02/23 16:38:01 martin Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1989, 1993
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
 *	@(#)file.h	8.3 (Berkeley) 1/9/95
 */

#ifndef _SYS_FILE_H_
#define	_SYS_FILE_H_

#include <sys/fcntl.h>
#include <sys/lock.h>
#include <sys/unistd.h>

#ifdef _KERNEL
#include <sys/mallocvar.h>
#include <sys/queue.h>

MALLOC_DECLARE(M_FILE);
MALLOC_DECLARE(M_IOCTLOPS);

struct proc;
struct uio;
struct iovec;
struct stat;
struct knote;

/*
 * Kernel descriptor table.
 * One entry for each open kernel vnode and socket.
 */
struct file {
	LIST_ENTRY(file) f_list;	/* list of active files */
	int		f_flag;		/* see fcntl.h */
	int		f_iflags;	/* internal flags */
#define	DTYPE_VNODE	1		/* file */
#define	DTYPE_SOCKET	2		/* communications endpoint */
#define	DTYPE_PIPE	3		/* pipe */
#define	DTYPE_KQUEUE	4		/* event queue */
#define	DTYPE_MISC	5		/* misc file descriptor type */
	int		f_type;		/* descriptor type */
	u_int		f_count;	/* reference count */
	u_int		f_msgcount;	/* references from message queue */
	int		f_usecount;	/* number active users */
	struct ucred	*f_cred;	/* creds associated with descriptor */
	struct fileops {
		int	(*fo_read)	(struct file *fp, off_t *offset,
					    struct uio *uio,
					    struct ucred *cred, int flags);
		int	(*fo_write)	(struct file *fp, off_t *offset,
					    struct uio *uio,
					    struct ucred *cred, int flags);
		int	(*fo_ioctl)	(struct file *fp, u_long com,
					    caddr_t data, struct proc *p);
		int	(*fo_fcntl)	(struct file *fp, u_int com,
					    caddr_t data, struct proc *p);
		int	(*fo_poll)	(struct file *fp, int events,
					    struct proc *p);
		int	(*fo_stat)	(struct file *fp, struct stat *sp,
					    struct proc *p);
		int	(*fo_close)	(struct file *fp, struct proc *p);
		int	(*fo_kqfilter)	(struct file *fp, struct knote *kn);
	} *f_ops;
	off_t		f_offset;
	caddr_t		f_data;		/* descriptor data, e.g. vnode/socket */
	struct simplelock f_slock;
};

#define	FIF_WANTCLOSE		0x01	/* a close is waiting for usecount */
#define	FIF_LARVAL		0x02	/* not fully constructed; don't use */

#define	FILE_IS_USABLE(fp)	(((fp)->f_iflags &			\
				  (FIF_WANTCLOSE|FIF_LARVAL)) == 0)

#define	FILE_SET_MATURE(fp)						\
do {									\
	(fp)->f_iflags &= ~FIF_LARVAL;					\
} while (/*CONSTCOND*/0)

#ifdef DIAGNOSTIC
#define	FILE_USE_CHECK(fp, str)						\
do {									\
	if ((fp)->f_usecount < 0)					\
		panic(str);						\
} while (/* CONSTCOND */ 0)
#else
#define	FILE_USE_CHECK(fp, str)		/* nothing */
#endif

/*
 * FILE_USE() must be called with the file lock held.
 * (Typical usage is: `fp = fd_getfile(..); FILE_USE(fp);'
 * and fd_getfile() returns the file locked)
 */
#define	FILE_USE(fp)							\
do {									\
	(fp)->f_usecount++;						\
	FILE_USE_CHECK((fp), "f_usecount overflow");			\
	simple_unlock(&(fp)->f_slock);					\
} while (/* CONSTCOND */ 0)

#define	FILE_UNUSE(fp, p)						\
do {									\
	simple_lock(&(fp)->f_slock);					\
	if ((fp)->f_iflags & FIF_WANTCLOSE) {				\
		simple_unlock(&(fp)->f_slock);				\
		/* Will drop usecount */				\
		(void) closef((fp), (p));				\
		break;							\
	} else {							\
		(fp)->f_usecount--;					\
		FILE_USE_CHECK((fp), "f_usecount underflow");		\
	}								\
	simple_unlock(&(fp)->f_slock);					\
} while (/* CONSTCOND */ 0)

/*
 * Flags for fo_read and fo_write.
 */
#define	FOF_UPDATE_OFFSET	0x01	/* update the file offset */

LIST_HEAD(filelist, file);
extern struct filelist	filehead;	/* head of list of open files */
extern int		maxfiles;	/* kernel limit on # of open files */
extern int		nfiles;		/* actual number of open files */

extern struct fileops	vnops;		/* vnode operations for files */

int	dofileread(struct proc *, int, struct file *, void *, size_t,
	    off_t *, int, register_t *);
int	dofilewrite(struct proc *, int, struct file *, const void *,
	    size_t, off_t *, int, register_t *);

int	dofilereadv(struct proc *, int, struct file *,
	    const struct iovec *, int, off_t *, int, register_t *);
int	dofilewritev(struct proc *, int, struct file *,
	    const struct iovec *, int, off_t *, int, register_t *);

void	finit(void);

#endif /* _KERNEL */

#endif /* _SYS_FILE_H_ */
