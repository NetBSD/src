/*	$NetBSD: userret.h,v 1.13.16.1 2016/12/05 10:54:58 skrll Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department and Ralph Campbell.
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
 * from: Utah Hdr: trap.c 1.32 91/04/06
 *
 *	@(#)trap.c	8.5 (Berkeley) 1/11/94
 */

#ifndef _SH3_USERRET_H_
#define	_SH3_USERRET_H_

#include <sys/userret.h>

#include <sh3/ubcreg.h>
#include "opt_ptrace.h"


static __inline void
userret(struct lwp *l)
{

	/* Invoke MI userret code */
	mi_userret(l);

#ifdef PTRACE_HOOKS
	/* Check if lwp is being PT_STEP'ed */
	if (l->l_md.md_flags & MDL_SSTEP) {
		struct trapframe *tf = l->l_md.md_regs;

		/*
		 * Channel A is set up for single stepping in sh_cpu_init().
		 * Before RTE we write tf_ubc to BBRA and tf_spc to BARA.
		 */
#ifdef SH3
		if (CPU_IS_SH3) {
			tf->tf_ubc = UBC_CYCLE_INSN | UBC_CYCLE_READ
				| SH3_UBC_CYCLE_CPU;
	}
#endif
#ifdef SH4
		if (CPU_IS_SH4) {
			tf->tf_ubc = UBC_CYCLE_INSN | UBC_CYCLE_READ;
		}
#endif
	}
#endif /* PTRACE_HOOKS */
}

#endif /* !_SH3_USERRET_H_ */
