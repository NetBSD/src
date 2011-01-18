/*	$NetBSD: proc.h,v 1.10 2011/01/18 01:02:54 matt Exp $	*/

/*-
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _POWERPC_PROC_H_
#define _POWERPC_PROC_H_

/*
 * Machine-dependent part of the lwp structure
 */
struct mdlwp {
	volatile int md_flags;
	struct cpu_info * volatile md_fpucpu;	/* last cpu FP was used on */
	struct cpu_info * volatile md_veccpu;	/* last cpu VEC was used on */
	struct trapframe *md_utf;		/* user trampframe */
};
#define MDLWP_USEDFPU	0x0001	/* this thread has used the FPU */
#define MDLWP_OWNFPU	0x0002	/* this thread is using the FPU */
#define	MDLWP_USEDVEC	0x0010	/* this thread has used the VEC */
#define	MDLWP_OWNVEC	0x0020	/* this thread is using the VEC */

struct trapframe;

struct mdproc {
	void (*md_syscall)(struct trapframe *);
};

#ifdef _KERNEL
#define	LWP0_CPU_INFO	&cpu_info[0]
#define	LWP0_MD_INITIALIZER {	\
		.md_flags = 0, \
		.md_fpucpu = LWP0_CPU_INFO, \
		.md_veccpu = LWP0_CPU_INFO, \
		.md_utf = (void *)0xdeadbeef, \
	}
#endif /* _KERNEL */

#endif /* _POWERPC_PROC_H_ */
