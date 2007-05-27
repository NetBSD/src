/* $NetBSD: msr_ipifuncs.c,v 1.8.2.3 2007/05/27 14:27:06 ad Exp $ */

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
 * Generic IPI handler to make writes to a MSR on x86, in all
 * CPUs available in the system.
 *
 * Thanks to Andrew Doran, Michael Van Elst and Quentin Garnier for
 * help and information provided.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: msr_ipifuncs.c,v 1.8.2.3 2007/05/27 14:27:06 ad Exp $");

#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/mutex.h>
#include <sys/lock.h>

#include <x86/cpu_msr.h>

#include <machine/cpu.h>
#include <machine/intrdefs.h>

static kmutex_t msr_mtx;
static volatile uint64_t msr_setvalue, msr_setmask;
static volatile int msr_type, msr_runcount, msr_read;


/*
 * This function will write the value of msr_setvalue in the MSR msr_type
 * and if a mask is provided, the value will be masked with msr_setmask.
 */
void
msr_write_ipi(struct cpu_info *ci)
{
	uint64_t msr = 0;

	/* Read the MSR requested and apply the mask if defined. */
	if (msr_read)
		msr = rdmsr(msr_type);

	if (msr_setmask)
		msr &= ~msr_setmask;

	/* Ok, assign value now.*/
	if (msr_read)
		msr |= msr_setvalue;
	else
		msr = msr_setvalue;

	/* Write it now */
	wrmsr(msr_type, msr);

	/* This cpu has finished making all tasks, update the counter. */
	__asm volatile ("lock; incl (%0)" :: "r" (&msr_runcount));
}

/*
 * Main function. Assigns values provided by the driver into the global
 * variables, necessary for the IPI handler.
 */
void
msr_cpu_broadcast(struct msr_cpu_broadcast *mcb)
{

	if (!mcb->msr_type)
		panic("msr_type not defined");

	/* We only want one CPU at a time sending these IPIs out. */
	mutex_enter(&msr_mtx);

	/* Initialize counter, the task has not run in any cpu yet. */
	msr_runcount = 0;

	/* Assign requested MSR type, value and mask. */
	msr_type = mcb->msr_type;
	msr_setvalue = mcb->msr_value;
	msr_setmask = mcb->msr_mask;
	msr_read = mcb->msr_read;

	/* 
	 * Issue a full memory barrier, to make sure the operations
	 * are done in a serialized way.
	 */
	mb_memory();

	/* Run the IPI write handler in the CPUs. */
	msr_write_ipi(curcpu());

#ifdef MULTIPROCESSOR
	if (ncpu > 1)
		x86_broadcast_ipi(X86_IPI_WRITE_MSR);
#endif

	while (msr_runcount < ncpu)
		x86_pause();

	/* We're done, so unlock. */
	mutex_exit(&msr_mtx);
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
