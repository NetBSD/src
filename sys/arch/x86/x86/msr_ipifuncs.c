/* $NetBSD: msr_ipifuncs.c,v 1.1 2007/03/20 21:07:39 xtraeme Exp $ */

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Juan Romero Pardines.
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
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
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

/*
 * Generic IPI handlers to make reads and writes to a MSR on x86, in
 * all CPUs available in the system.
 *
 * Thanks to Andrew Doran, Michael Van Elst and Quentin Garnier for
 * help and information provided.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: msr_ipifuncs.c,v 1.1 2007/03/20 21:07:39 xtraeme Exp $");

#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/mutex.h>
#include <sys/lock.h>

#include <x86/cpu_msr.h>

#include <machine/cpu.h>
#include <machine/intrdefs.h>

/* #define MSR_CPU_BROADCAST_DEBUG */

#ifdef MSR_CPU_BROADCAST_DEBUG
#define DPRINTF(x)	do { printf x; } while (/* CONSTCOND */ 0)
#else
#define DPRINTF(x)
#endif

static kmutex_t msr_mtx;
static volatile uint64_t msr_setvalue;
static volatile uint64_t msr_setmask;
static volatile int msr_type;
static volatile int msr_runcount;

static void msr_cpu_broadcast_main(struct msr_cpu_broadcast *, int);

/*
 * This function will read the value of the MSR defined in msr_type
 * and will return it in ci_msr_rvalue (per-cpu).
 */
void
msr_read_ipi(struct cpu_info *ci)
{
	/* Assign current value for the MSR msr_type. */
	ci->ci_msr_rvalue = rdmsr(msr_type);

	/* 
	 * Increement the counter atomically, this cpu has
	 * finished the operation correctly.
	 */
	__asm volatile ("lock; incl (%0)" :: "r" (&msr_runcount));
}

/*
 * This function will write the value of msr_setvalue in the MSR msr_type
 * and if a mask is provided, the value will be masked with msr_setmask,
 * also will update ci->ci_msr_rvalue with the returned value of the MSR,
 * to avoid another run of msr_read_ipi in the main function.
 */
void
msr_write_ipi(struct cpu_info *ci)
{
	uint64_t msr;

	/* Read the MSR requested and apply the mask if defined. */
	msr = rdmsr(msr_type);
	if (msr_setmask)
		msr &= ~msr_setmask;

	/* Ok, assign value now.*/
	msr |= msr_setvalue;

	/* Write it now */
	wrmsr(msr_type, msr);

	/* Update our per-cpu variable with the current value of the MSR. */
	ci->ci_msr_rvalue = rdmsr(msr_type);

	/* This cpu has finished making all tasks, update the counter. */
	__asm volatile ("lock; incl (%0)" :: "r" (&msr_runcount));
}

/*
 * Main function. Assigns values provided by the driver into the global
 * variables, necessary for the IPI handlers. If mode is true, a write
 * operation will be performed, otherwise a read operation. 
 */
static void
msr_cpu_broadcast_main(struct msr_cpu_broadcast *mcb, int mode)
{
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;

	DPRINTF(("\n%s: ---- START ----\n", __func__));

	/* We only want one CPU at a time sending these IPIs out. */
	mutex_enter(&msr_mtx);

	/* Initialize counter, the task has not run in any cpu yet. */
	msr_runcount = msr_setmask = msr_setvalue = 0;

	/* Assign requested MSR type, value and mask. */
	msr_type = mcb->msr_type;

	if (mode) {
		msr_setvalue = mcb->msr_value;
		msr_setmask = mcb->msr_mask;
	}

	/* 
	 * Issue a full memory barrier, to make sure the operations
	 * are done in a serialized way.
	 */
	mb_memory();

	if (mode) {	/* Write mode */

		DPRINTF(("%s: before write\n", __func__));

		/* Run the IPI write handler in the CPUs. */
		msr_write_ipi(curcpu());

#ifdef MULTIPROCESSOR
		if (ncpu > 1)
			x86_broadcast_ipi(X86_IPI_WRITE_MSR);
#endif

		DPRINTF(("%s: after write\n", __func__));

	} else {	/* Read mode */

		DPRINTF(("%s: before read\n", __func__));

		/* Run the IPI read handler in the CPUs. */
		msr_read_ipi(curcpu());

#ifdef MULTIPROCESSOR
		if (ncpu > 1)
			x86_broadcast_ipi(X86_IPI_READ_MSR);
#endif

		DPRINTF(("%s: after read\n", __func__));
	}

	DPRINTF(("%s: before pause\n", __func__));
	DPRINTF(("%s: msr_runcount=%d ncpu=%d\n", __func__,
	    msr_runcount, ncpu));

	/* Spin until every CPU has run the handler. */
	while (msr_runcount < ncpu)
		x86_pause();

	DPRINTF(("%s: after pause\n", __func__));

	for (CPU_INFO_FOREACH(cii, ci)) {
		DPRINTF(("%s: ci_msr_rvalue=0x%" PRIu64 "\n",
		    ci->ci_dev->dv_xname, ci->ci_msr_rvalue));
	}

	/* We're done, so unlock. */
	mutex_exit(&msr_mtx);

	DPRINTF(("%s: ----- END -----\n\n", __func__));
}

/* 
 * Initializes the mutex very early in the boot process
 * for future access.
 */
void
msr_cpu_broadcast_initmtx(void)
{
	mutex_init(&msr_mtx, MUTEX_DRIVER, IPL_NONE);
}

/*
 * This is the function used by the drivers using this framework.
 * The driver is reponsible to set apropiately its members:
 *
 * 	mcb->msr_type	type of the MSR requested by the driver,
 * 			e.g MSR_PERF_STATUS.
 *
 * 	mcb->msr_value	value that will be set in the MSR, when a
 * 			write operation is requested by the driver.
 *
 * 	mcb->msr_mask	value that will be used to mask the returned
 * 			value of the rdmsr(msr_type) call in the write
 * 			operation.
 *
 * msr_type needs to be defined, otherwise EINVAL is returned.
 *
 * When calling this function, a mode must be selected. Currently
 * two modes are implemented:
 *
 * 	MSR_CPU_BROADCAST_READ	Performs a read operation.
 *
 * 	MSR_CPU_BROADCAST_WRITE	Performs a write operation.
 *
 * If an invalid mode is specified, EINVAL is returned.
 */
int
msr_cpu_broadcast(struct msr_cpu_broadcast *mcb, int mode)
{
	if (!mcb->msr_type) {
		DPRINTF(("%s: msr_type is empty\n", __func__));
		return EINVAL;
	}

	DPRINTF(("%s: mcb->msr_value=%" PRIu64 " mcb->msr_type=%d "
	    "mcb->msr_mask=%" PRIu64 "\n", __func__,
	    mcb->msr_value, mcb->msr_type, mcb->msr_mask));

	switch (mode) {
	case MSR_CPU_BROADCAST_READ:
		msr_cpu_broadcast_main(mcb, false);
		break;
	case MSR_CPU_BROADCAST_WRITE:
		msr_cpu_broadcast_main(mcb, true);
		break;
	default:
		return EINVAL;
	}

	return 0;
}
