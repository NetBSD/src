/*	$NetBSD: filedesc.h,v 1.23 2001/06/14 20:32:49 thorpej Exp $	*/

/*
 * Copyright (c) 1990, 1993
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
 *	@(#)filedesc.h	8.1 (Berkeley) 6/2/93
 */

#ifndef _SYS_FILEDESC_H_
#define	_SYS_FILEDESC_H_

/*
 * This structure is used for the management of descriptors.  It may be
 * shared by multiple processes.
 *
 * A process is initially started out with NDFILE descriptors stored within
 * this structure, selected to be enough for typical applications based on
 * the historical limit of 20 open files (and the usage of descriptors by
 * shells).  If these descriptors are exhausted, a larger descriptor table
 * may be allocated, up to a process' resource limit; the internal arrays
 * are then unused.  The initial expansion is set to NDEXTENT; each time
 * it runs out, it is doubled until the resource limit is reached. NDEXTENT
 * should be selected to be the biggest multiple of OFILESIZE (see below)
 * that will fit in a power-of-two sized piece of memory.
 */
#define	NDFILE		20
#define	NDEXTENT	50		/* 250 bytes in 256-byte alloc */

struct filedesc {
	struct file	**fd_ofiles;	/* file structures for open files */
	char		*fd_ofileflags;	/* per-process open file flags */
	int		fd_nfiles;	/* number of open files allocated */
	int		fd_lastfile;	/* high-water mark of fd_ofiles */
	int		fd_freefile;	/* approx. next free file */
	int		fd_refcnt;	/* reference count */
};

struct cwdinfo {
	struct vnode	*cwdi_cdir;	/* current directory */
	struct vnode	*cwdi_rdir;	/* root directory */
	u_short		cwdi_cmask;	/* mask for file creation */
	u_short		cwdi_refcnt;	/* reference count */
};


/*
 * Basic allocation of descriptors:
 * one of the above, plus arrays for NDFILE descriptors.
 */
struct filedesc0 {
	struct filedesc	fd_fd;
	/*
	 * These arrays are used when the number of open files is
	 * <= NDFILE, and are then pointed to by the pointers above.
	 */
	struct file	*fd_dfiles[NDFILE];
	char		fd_dfileflags[NDFILE];
};

/*
 * Per-process open flags.
 */
#define	UF_EXCLOSE 	0x01		/* auto-close on exec */

/*
 * Storage required per open file descriptor.
 */
#define	OFILESIZE (sizeof(struct file *) + sizeof(char))

#ifdef _KERNEL
/*
 * Kernel global variables and routines.
 */
int	dupfdopen(struct proc *p, int indx, int dfd, int mode, int error);
int	fdalloc(struct proc *p, int want, int *result);
void	fdexpand(struct proc *p);
int	fdavail(struct proc *p, int n);
int	falloc(struct proc *p, struct file **resultfp, int *resultfd);
void	ffree(struct file *);
struct filedesc *fdcopy(struct proc *p);
struct filedesc *fdinit(struct proc *p);
void	fdshare(struct proc *p1, struct proc *p2);
void	fdunshare(struct proc *p);
void	fdinit1(struct filedesc0 *newfdp);
void	fdclear(struct proc *p);
void	fdfree(struct proc *p);
void	fdremove(struct filedesc *, int);
int	fdrelease(struct proc *p, int);
void	fdcloseexec(struct proc *);

struct file *fd_getfile(struct filedesc *, int);

struct cwdinfo *cwdinit(struct proc *);
void	cwdshare(struct proc *, struct proc *);
void	cwdunshare(struct proc *);
void	cwdfree(struct proc *);

int	closef(struct file *, struct proc *);
int	getsock(struct filedesc *, int, struct file **);
#endif /* _KERNEL */

#endif /* !_SYS_FILEDESC_H_ */
