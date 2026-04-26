/*	$NetBSD: fpu.c,v 1.6 2026/04/26 10:52:15 thorpej Exp $	*/

/*-
 * Copyright (c) 1996, 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Gordon W. Ross.
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
 * Floating Point Unit (MC68881/882/040/060)
 * Probe for the FPU at early bootstrap.
 */

#include "opt_m68k_arch.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: fpu.c,v 1.6 2026/04/26 10:52:15 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/lwp.h>

#include <machine/pcb.h>
#include <machine/psl.h>
#include <machine/cpu.h>
#include <machine/frame.h>
#include <machine/vectors.h>

/*
 * This is the default, but may be overridden as we determine
 * the machine / cpu type before we ever get to fpu_init().
 *
 * For systems that lack a coprocessor interface, we just default
 * to NONE.
 */
#ifdef M68K_FPCOPROC
int	fputype = FPU_UNKNOWN;
#else
int	fputype = FPU_NONE;
#endif

void
fpu_init(void)
{
#ifdef M68K_FPCOPROC
	/*
	 * A 68881 idle frame is 28 bytes and a 68882's is 60 bytes.
	 * We, of course, need to have enough room for either.
	 */
	struct pcb *pcb = lwp_getpcb(&lwp0);
	struct fpframe fpframe;
	label_t faultbuf;

	/* Make sure lwp0 has a NULL state frame. */
	memset(&pcb->pcb_fpregs, 0, sizeof(pcb->pcb_fpregs));

	if (fputype != FPU_UNKNOWN) {
		goto done;
	}

	nofault = &faultbuf;
	if (setjmp(&faultbuf)) {
		nofault = NULL;
		fputype = FPU_NONE;
		return;
	}

	/*
	 * Synchronize FPU or cause a fault.
	 * This should leave the 881/882 in the IDLE state,
	 * state, so we can determine which we have by
	 * examining the size of the FP state frame
	 */
	__asm volatile("fnop");

	nofault = NULL;

	/*
	 * Presumably, if we're an 040/060 and did not take exception
	 * above, we have an FPU.  Don't bother probing.
	 */
	if (cputype == CPU_68060) {
		fputype = FPU_68060;
	} else if (cputype == CPU_68040) {
		fputype = FPU_68040;
	}

	if (fputype == FPU_UNKNOWN) {
		/*
		 * fpu_doprobe() will have saved an FP state frame for us,
		 * and can consult it to intuit the FP type.
		 *
		 * The size of a 68881 IDLE frame is 0x18
		 *         and a 68882 frame is 0x38
		 */
		__asm volatile("fsave %0@" : : "a" (&fpframe) : "memory");
		if (fpframe.fpf_fsize == 0x18) {
			fputype = FPU_68881;
		} else if (fpframe.fpf_fsize == 0x38) {
			fputype = FPU_68882;
		}
		/*
		 * If it's not one of the above, we have no clue
		 * what it is.
		 */
	}

 done:
	/* Avoid crazy values. */
	if (fputype < 0 || fputype > FPU_UNKNOWN) {
		fputype = FPU_UNKNOWN;
	}
	if (fputype != FPU_UNKNOWN) {
		m68k_make_fpu_idle_frame();
		vec_init_fp();
	}
#else
	fputype = FPU_NONE;
#endif
}
