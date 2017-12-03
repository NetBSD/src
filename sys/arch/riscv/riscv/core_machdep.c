/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#include <sys/cdefs.h>

#ifndef CORENAME
__RCSID("$NetBSD: core_machdep.c,v 1.1.18.2 2017/12/03 11:36:39 jdolecek Exp $");
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/core.h>
#include <sys/exec.h>
#include <sys/cpu.h>

#include <riscv/locore.h>

#ifndef CORENAME
#define	CORENAME(n)	n
#endif
#ifdef COREINC
#include COREINC
#endif

/*
 * Dump the machine specific segment at the start of a core dump.
 */
int
CORENAME(cpu_coredump)(struct lwp *l, struct coredump_iostate *iocookie,
    struct CORENAME(core) *chdr)
{
	int error;
	struct CORENAME(coreseg) cseg;
	struct cpustate {
		struct CORENAME(trapframe) tf;
		struct fpreg fpregs;
	} cpustate;

	if (iocookie == NULL) {
		CORE_SETMAGIC(*chdr, COREMAGIC, MID_MACHINE, 0);
		chdr->c_hdrsize = ALIGN(sizeof(*chdr));
		chdr->c_seghdrsize = ALIGN(sizeof(cseg));
		chdr->c_cpusize = sizeof(struct cpustate);
		chdr->c_nseg++;
		return 0;
	}

	pcu_save_all(l);

	// Can't use structure assignment if this is doing COMPAT_NETBSD32
	const struct trapframe * const tf = l->l_md.md_utf;
	for (size_t i = _X_RA; i <= _X_GP; i++) {
		cpustate.tf.tf_reg[i] = tf->tf_reg[i];
	}
	cpustate.tf.tf_pc = tf->tf_pc;
	cpustate.tf.tf_badaddr = tf->tf_badaddr;
	cpustate.tf.tf_cause = tf->tf_cause;
	cpustate.tf.tf_sr = tf->tf_sr;
	if (fpu_valid_p()) {
		cpustate.fpregs = ((struct pcb *)lwp_getpcb(l))->pcb_fpregs;
	} else {
		memset(&cpustate.fpregs, 0, sizeof(cpustate.fpregs));
	}
	CORE_SETMAGIC(cseg, CORESEGMAGIC, MID_MACHINE, CORE_CPU);
	cseg.c_addr = 0;
	cseg.c_size = chdr->c_cpusize;

	error = coredump_write(iocookie, UIO_SYSSPACE, &cseg,
	    chdr->c_seghdrsize);
	if (error)
		return error;

	return coredump_write(iocookie, UIO_SYSSPACE, &cpustate,
	    chdr->c_cpusize);
}
