/*	$NetBSD: ptrace.h,v 1.39.16.1 2008/01/09 01:58:15 matt Exp $	*/

/*-
 * Copyright (c) 1984, 1993
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
 *	@(#)ptrace.h	8.2 (Berkeley) 1/4/94
 */

#ifndef	_SYS_PTRACE_H_
#define	_SYS_PTRACE_H_

#define	PT_TRACE_ME	0	/* child declares it's being traced */
#define	PT_READ_I	1	/* read word in child's I space */
#define	PT_READ_D	2	/* read word in child's D space */
#define	PT_WRITE_I	4	/* write word in child's I space */
#define	PT_WRITE_D	5	/* write word in child's D space */
#define	PT_CONTINUE	7	/* continue the child */
#define	PT_KILL		8	/* kill the child process */
#define	PT_ATTACH	9	/* attach to running process */
#define	PT_DETACH	10	/* detach from running process */
#define	PT_IO		11	/* do I/O to/from the stopped process */
#define	PT_DUMPCORE	12	/* make the child generate a core dump */
#define	PT_LWPINFO	13	/* get info about the LWP */
#define	PT_SYSCALL	14	/* stop on syscall entry/exit */
#define	PT_FIRSTMACH	32	/* for machine-specific requests */
#include <machine/ptrace.h>	/* machine-specific requests, if any */

/*
 * Argument structure for PT_IO.
 */
struct ptrace_io_desc {
	int	piod_op;	/* I/O operation (see below) */
	void	*piod_offs;	/* child offset */
	void	*piod_addr;	/* parent offset */
	size_t	piod_len;	/* request length (in)/actual count (out) */
};

/* piod_op */
#define	PIOD_READ_D	1	/* read from D space */
#define	PIOD_WRITE_D	2	/* write to D spcae */
#define	PIOD_READ_I	3	/* read from I space */
#define	PIOD_WRITE_I	4	/* write to I space */

/*
 * Argument structure for PT_LWPINFO.
 */
struct ptrace_lwpinfo {
	lwpid_t	pl_lwpid;	/* LWP described */
	int	pl_event;	/* Event that stopped the LWP */
	/* Add fields at the end */
};

#define PL_EVENT_NONE	0
#define PL_EVENT_SIGNAL	1

#ifdef _KERNEL

#if defined(PT_GETREGS) || defined(PT_SETREGS)
struct reg;
#ifndef process_reg32
#define process_reg32 struct reg
#endif
#ifndef process_reg64
#define process_reg64 struct reg
#endif
#endif
#if defined(PT_GETFPREGS) || defined(PT_SETFPREGS)
struct fpreg;
#ifndef process_fpreg32
#define process_fpreg32 struct fpreg
#endif
#ifndef process_fpreg64
#define process_fpreg64 struct fpreg
#endif
#endif

int	process_doregs(struct lwp *, struct lwp *, struct uio *);
int	process_validregs(struct lwp *);

int	process_dofpregs(struct lwp *, struct lwp *, struct uio *);
int	process_validfpregs(struct lwp *);

int	process_domem(struct lwp *, struct lwp *, struct uio *);

void	process_stoptrace(void);

void	proc_reparent(struct proc *, struct proc *);
#ifdef PT_GETFPREGS
int	process_read_fpregs(struct lwp *, struct fpreg *);
#ifndef process_read_fpregs32
#define process_read_fpregs32	process_read_fpregs
#endif
#ifndef process_read_fpregs64
#define process_read_fpregs64	process_read_fpregs
#endif
#endif
#ifdef PT_GETREGS
int	process_read_regs(struct lwp *, struct reg *);
#ifndef process_read_regs32
#define process_read_regs32	process_read_regs
#endif
#ifndef process_read_regs64
#define process_read_regs64	process_read_regs
#endif
#endif
int	process_set_pc(struct lwp *, void *);
int	process_sstep(struct lwp *, int);
#ifdef PT_SETFPREGS
int	process_write_fpregs(struct lwp *, const struct fpreg *);
#endif
#ifdef PT_SETREGS
int	process_write_regs(struct lwp *, const struct reg *);
#endif

#ifdef __HAVE_PROCFS_MACHDEP
int	ptrace_machdep_dorequest(struct lwp *, struct lwp *, int,
	    void *, int);
#endif

#ifndef FIX_SSTEP
#define FIX_SSTEP(p)
#endif

#else /* !_KERNEL */

#include <sys/cdefs.h>

__BEGIN_DECLS
int	ptrace(int _request, pid_t _pid, void *_addr, int _data);
__END_DECLS

#endif /* !_KERNEL */

#endif	/* !_SYS_PTRACE_H_ */
