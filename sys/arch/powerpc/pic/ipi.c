/* $NetBSD: ipi.c,v 1.3.4.1 2008/05/16 02:23:02 yamt Exp $ */
/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tim Rightnour
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
__KERNEL_RCSID(0, "$NetBSD: ipi.c,v 1.3.4.1 2008/05/16 02:23:02 yamt Exp $");

#include "opt_multiprocessor.h"
#include "opt_pic.h"
#include "opt_interrupt.h"
#include "opt_altivec.h"

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/kernel.h>

#include <powerpc/atomic.h>
#include <powerpc/fpu.h>
#include <powerpc/altivec.h>

#include <arch/powerpc/pic/picvar.h>
#include <arch/powerpc/pic/ipivar.h>
#include "opt_ipi.h"

#ifdef MULTIPROCESSOR

struct ipi_ops ipiops;
volatile u_long IPI[CPU_MAXNUM];

/* Process an actual IPI */

int
ppcipi_intr(void *v)
{
	int cpu_id = curcpu()->ci_index;
	int msr;
	u_long ipi;

	curcpu()->ci_ev_ipi.ev_count++;
	ipi = atomic_loadlatch_ulong(&IPI[cpu_id], 0);

	if (ipi == PPC_IPI_NOMESG)
		return 1;

	if (ipi & PPC_IPI_FLUSH_FPU)
		save_fpu_cpu();

#ifdef ALTIVEC
	if (ipi & PPC_IPI_FLUSH_VEC)
		save_vec_cpu();
#endif

	if (ipi & PPC_IPI_HALT) {
		aprint_normal("halting CPU %d\n", cpu_id);
		msr = (mfmsr() & ~PSL_EE) | PSL_POW;
		for (;;) {
			__asm volatile ("sync; isync");
			mtmsr(msr);
		}
	}
	return 1;
}

#endif /*MULTIPROCESSOR*/
