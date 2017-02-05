/*	$NetBSD: proc.h,v 1.19.6.1 2017/02/05 13:40:02 skrll Exp $	*/

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

#include <machine/frame.h>
#include <machine/pcb.h>
#include <x86/dbregs.h>

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
	struct	x86_hw_watchpoint md_watchpoint[X86_HW_WATCHPOINTS];
};

#define	MDL_COMPAT32		0x0008	/* i386, always return via iret */
#define	MDL_IRET		0x0010	/* force return via iret, not sysret */
#define	MDL_X86_HW_WATCHPOINTS	0x0020	/* has hardware watchpoints */

struct mdproc {
	int	md_flags;
					/* Syscall handling function */
	void	(*md_syscall)(struct trapframe *);
};

/* md_flags */
#define MDP_USEDMTRR	0x0008	/* has set volatile MTRRs */

#else	/*	__x86_64__	*/

#include <i386/proc.h>

#endif	/*	__x86_64__	*/

#endif /* _AMD64_PROC_H */
