/* $NetBSD: cpuvar.h,v 1.4 1999/02/23 03:20:01 thorpej Exp $ */

/*-
 * Copyright (c) 1998, 1999 The NetBSD Foundation, Inc.
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

#ifndef _ALPHA_ALPHA_CPUVAR_H_
#define	_ALPHA_ALPHA_CPUVAR_H_

#include <sys/lock.h>

#define	CPUF_PRIMARY	0x00000001	/* CPU is primary CPU */

struct cpu_info {
	struct proc *ci_curproc;	/* current owner of the processor */
	struct proc *ci_fpcurproc;	/* current owner of the FPU */
	paddr_t ci_curpcb;		/* PA of current HW context */
	struct simplelock ci_slock;	/* spin lock */
	u_long ci_cpuid;		/* CPU ID */
	struct proc *ci_idle_thread;	/* our idle thread */
	u_long ci_flags;		/* flags; see below */
	u_long ci_ipis;			/* interprocessor interrupts pending */
	struct device *ci_dev;		/* pointer to our device */
};

#ifdef _KERNEL

#ifndef _LKM
#include "opt_multiprocessor.h"
#endif

#ifdef MULTIPROCESSOR
extern	unsigned long cpus_running;
extern	struct cpu_info cpu_info[];

/*
 * Map per-cpu variables to the cpu_info structure.
 * XXX alpha_pal_whami() might be expensive, here!
 */
#define	curproc		cpu_info[alpha_pal_whami()].ci_curproc
#define	fpcurproc	cpu_info[alpha_pal_whami()].ci_fpcurproc
#define	curpcb		cpu_info[alpha_pal_whami()].ci_curpcb
#else
extern	struct proc *fpcurproc;		/* current owner of FPU */
extern	paddr_t curpcb;			/* PA of current HW context */
#endif /* MULTIPROCESSOR */

#endif /* _KERNEL */

#endif /* _ALPHA_ALPHA_CPUVAR_H_ */
