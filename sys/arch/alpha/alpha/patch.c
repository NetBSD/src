/*	$NetBSD: patch.c,v 1.7 2021/07/13 01:59:10 thorpej Exp $	*/

/*-
 * Copyright (c) 2007, 2021 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran and Jason R. Thorpe.
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

/*
 * Patch kernel code at boot time, depending on available CPU features
 * and configuration.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: patch.c,v 1.7 2021/07/13 01:59:10 thorpej Exp $");

#include "opt_multiprocessor.h"

#include <sys/types.h>
#include <sys/systm.h>
#include <sys/lwp.h>

#include <machine/cpu.h>
#include <machine/alpha.h>
#include <machine/intr.h>

#include <alpha/alpha/db_instruction.h>

void	_membar_producer(void);
void	_membar_producer_end(void);
void	_membar_producer_mp(void);
void	_membar_producer_mp_end(void);

void	_membar_sync(void);
void	_membar_sync_end(void);
void	_membar_sync_mp(void);
void	_membar_sync_mp_end(void);

extern char alpha_copystr_bwx[], alpha_copystr_bwx_end[];
extern char alpha_copystr[], alpha_copystr_end[];

#if defined(MULTIPROCESSOR)
extern uint32_t *lock_stub_patch_table[];

static void
patch_lock_stubs(void)
{
	alpha_instruction insn = {
		.mem_format = {
			.opcode = op_special,
			.displacement = op_mb,
		},
	};
	int i, s;

	s = splhigh();

	for (i = 0; lock_stub_patch_table[i] != NULL; i++) {
		*lock_stub_patch_table[i] = insn.bits;
	}
	alpha_pal_imb();

	splx(s);
}
#endif /* MULTIPROCESSOR */

static void
patchfunc(void *from_s, void *from_e, void *to_s, void *to_e)
{
	int s;

	s = splhigh();

	if ((uintptr_t)from_e - (uintptr_t)from_s !=
	    (uintptr_t)to_e - (uintptr_t)to_s)
	    	panic("patchfunc: sizes do not match (from=%p)", from_s);
	
	memcpy(to_s, from_s, (uintptr_t)to_e - (uintptr_t)to_s);
	alpha_pal_imb();

	splx(s);
}

void
alpha_patch(bool is_mp)
{

	/*
	 * We allow this function to be called multiple times
	 * (there is no harm in doing so), so long as other
	 * CPUs have not yet actually hatched to start running
	 * kernel code.
	 */

	if (cpu_amask & ALPHA_AMASK_BWX) {
		patchfunc(alpha_copystr_bwx, alpha_copystr_bwx_end,
		    alpha_copystr, alpha_copystr_end);
	}

#if defined(MULTIPROCESSOR)
	if (is_mp) {
		KASSERT(curcpu()->ci_flags & CPUF_PRIMARY);
		KASSERT((cpus_running & ~(1UL << cpu_number())) == 0);

		patch_lock_stubs();

		patchfunc(_membar_producer_mp, _membar_producer_mp_end,
			  _membar_producer, _membar_producer_end);
		patchfunc(_membar_sync_mp, _membar_sync_mp_end,
			  _membar_sync, _membar_sync_end);
	}
#else
	KASSERT(is_mp == false);
#endif /* MULTIPROCESSOR */
}
