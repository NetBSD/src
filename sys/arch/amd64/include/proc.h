/*	$NetBSD: proc.h,v 1.11.8.2 2009/11/01 13:58:54 jym Exp $	*/

/*
 * Copyright (c) 1991 Regents of the University of California.
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
 *	@(#)proc.h	7.1 (Berkeley) 5/15/91
 */

#ifndef _AMD64_PROC_H
#define _AMD64_PROC_H

#ifdef __x86_64__

#include <sys/user.h> /* for sizeof(struct user) */
#include <machine/frame.h>

/*
 * Machine-dependent part of the lwp structure for amd64.
 */
struct pmap;
struct vm_page;

struct mdlwp {
	struct	trapframe *md_regs;	/* registers on current frame */
	struct pmap *md_gc_pmap;	/* pmap being garbage collected */
	struct vm_page *md_gc_ptp;	/* pages from pmap g/c */
	int	md_flags;		/* machine-dependent flags */
	volatile int md_astpending;
};

struct mdproc {
	int	md_flags;
					/* Syscall handling function */
	void	(*md_syscall)(struct trapframe *);
};

/* md_flags */
#define	MDP_USEDFPU	0x0001	/* has used the FPU */
#define MDP_COMPAT	0x0002	/* x86 compatibility process */
#define MDP_SYSCALL	0x0004	/* entered kernel via syscall ins */
#define MDP_USEDMTRR	0x0008	/* has set volatile MTRRs */
#define MDP_IRET	0x0010	/* return via iret, not sysret */

#define	UAREA_USER_OFFSET	(USPACE - ALIGN(sizeof(struct user)))
#define	KSTACK_LOWEST_ADDR(l)	((void *)USER_TO_UAREA((l)->l_addr))
#define	KSTACK_SIZE		UAREA_USER_OFFSET

#else	/*	__x86_64__	*/

#include <i386/proc.h>

#endif	/*	__x86_64__	*/

#endif /* _AMD64_PROC_H */
