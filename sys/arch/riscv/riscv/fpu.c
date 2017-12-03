/*	$NetBSD: fpu.c,v 1.1.18.2 2017/12/03 11:36:39 jdolecek Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: fpu.c,v 1.1.18.2 2017/12/03 11:36:39 jdolecek Exp $");

#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/mutex.h>
#include <sys/condvar.h>
#include <sys/cpu.h>
#include <sys/proc.h>
#include <sys/lwp.h>
#include <sys/pcu.h>

#include <riscv/locore.h>

static void fpu_state_save(lwp_t *);
static void fpu_state_load(lwp_t *, u_int);
static void fpu_state_release(lwp_t *);

const pcu_ops_t pcu_fpu_ops = {
	.pcu_id = PCU_FPU,
	.pcu_state_save = fpu_state_save,
	.pcu_state_load = fpu_state_load,
	.pcu_state_release = fpu_state_release
};

void
fpu_state_save(lwp_t *l)
{
	struct trapframe * const tf = l->l_md.md_utf;
	struct pcb * const pcb = lwp_getpcb(l);
	struct fpreg * const fp = &pcb->pcb_fpregs;

	KASSERT(l->l_pcu_cpu[PCU_FPU] == curcpu());

	// Don't do anything if the FPU is already off.
	if ((tf->tf_sr & SR_EF) == 0)
		return;

	curcpu()->ci_ev_fpu_saves.ev_count++;

	// Enable FPU to save FP state
	(void) riscvreg_status_set(SR_EF);

	// Save FCSR
	fp->r_fcsr = riscvreg_fcsr_read();

	// Save FP register values.
	__asm(	"fsd	f0, (0*%1)(%0)"
	"\n\t"	"fsd	f1, (1*%1)(%0)"
	"\n\t"	"fsd	f2, (2*%1)(%0)"
	"\n\t"	"fsd	f3, (3*%1)(%0)"
	"\n\t"	"fsd	f4, (4*%1)(%0)"
	"\n\t"	"fsd	f5, (5*%1)(%0)"
	"\n\t"	"fsd	f6, (6*%1)(%0)"
	"\n\t"	"fsd	f7, (7*%1)(%0)"
	"\n\t"	"fsd	f8, (8*%1)(%0)"
	"\n\t"	"fsd	f9, (9*%1)(%0)"
	"\n\t"	"fsd	f10, (10*%1)(%0)"
	"\n\t"	"fsd	f11, (11*%1)(%0)"
	"\n\t"	"fsd	f12, (12*%1)(%0)"
	"\n\t"	"fsd	f13, (13*%1)(%0)"
	"\n\t"	"fsd	f14, (14*%1)(%0)"
	"\n\t"	"fsd	f15, (15*%1)(%0)"
	"\n\t"	"fsd	f16, (16*%1)(%0)"
	"\n\t"	"fsd	f17, (17*%1)(%0)"
	"\n\t"	"fsd	f18, (18*%1)(%0)"
	"\n\t"	"fsd	f19, (19*%1)(%0)"
	"\n\t"	"fsd	f20, (20*%1)(%0)"
	"\n\t"	"fsd	f21, (21*%1)(%0)"
	"\n\t"	"fsd	f22, (22*%1)(%0)"
	"\n\t"	"fsd	f23, (23*%1)(%0)"
	"\n\t"	"fsd	f24, (24*%1)(%0)"
	"\n\t"	"fsd	f25, (25*%1)(%0)"
	"\n\t"	"fsd	f26, (26*%1)(%0)"
	"\n\t"	"fsd	f27, (27*%1)(%0)"
	"\n\t"	"fsd	f28, (28*%1)(%0)"
	"\n\t"	"fsd	f29, (29*%1)(%0)"
	"\n\t"	"fsd	f30, (30*%1)(%0)"
	"\n\t"	"fsd	f31, (31*%1)(%0)"
	   ::	"r"(fp->r_fpreg),
		"i"(sizeof(fp->r_fpreg[0])));

	// Disable the FPU
	riscvreg_status_clear(SR_EF);
}

void
fpu_state_load(lwp_t *l, u_int flags)
{
	struct trapframe * const tf = l->l_md.md_utf;
	struct pcb * const pcb = lwp_getpcb(l);
	struct fpreg * const fp = &pcb->pcb_fpregs;

	KASSERT(l->l_pcu_cpu[PCU_FPU] == curcpu());

	// If this is the first time the state is being loaded, zero it first.
	if (__predict_false((flags & PCU_VALID) == 0)) {
		memset(fp, 0, sizeof(*fp));
	}

	// Enable the FP when this lwp return to userspace.
	tf->tf_sr |= SR_EF;

	// If this is a simple reeanble, set the FPU enable flag and return
	if (flags & PCU_REENABLE) {
		curcpu()->ci_ev_fpu_reenables.ev_count++;
		return;
	}

	curcpu()->ci_ev_fpu_loads.ev_count++;


	// Enabling to load FP state.  Interrupts will remain on.
	(void) riscvreg_status_set(SR_EF);

	// load FP registers and establish processes' FP context.
	__asm(	"fld	f0, (0*%1)(%0)"
	"\n\t"	"fld	f1, (1*%1)(%0)"
	"\n\t"	"fld	f2, (2*%1)(%0)"
	"\n\t"	"fld	f3, (3*%1)(%0)"
	"\n\t"	"fld	f4, (4*%1)(%0)"
	"\n\t"	"fld	f5, (5*%1)(%0)"
	"\n\t"	"fld	f6, (6*%1)(%0)"
	"\n\t"	"fld	f7, (7*%1)(%0)"
	"\n\t"	"fld	f8, (8*%1)(%0)"
	"\n\t"	"fld	f9, (9*%1)(%0)"
	"\n\t"	"fld	f10, (10*%1)(%0)"
	"\n\t"	"fld	f11, (11*%1)(%0)"
	"\n\t"	"fld	f12, (12*%1)(%0)"
	"\n\t"	"fld	f13, (13*%1)(%0)"
	"\n\t"	"fld	f14, (14*%1)(%0)"
	"\n\t"	"fld	f15, (15*%1)(%0)"
	"\n\t"	"fld	f16, (16*%1)(%0)"
	"\n\t"	"fld	f17, (17*%1)(%0)"
	"\n\t"	"fld	f18, (18*%1)(%0)"
	"\n\t"	"fld	f19, (19*%1)(%0)"
	"\n\t"	"fld	f20, (20*%1)(%0)"
	"\n\t"	"fld	f21, (21*%1)(%0)"
	"\n\t"	"fld	f22, (22*%1)(%0)"
	"\n\t"	"fld	f23, (23*%1)(%0)"
	"\n\t"	"fld	f24, (24*%1)(%0)"
	"\n\t"	"fld	f25, (25*%1)(%0)"
	"\n\t"	"fld	f26, (26*%1)(%0)"
	"\n\t"	"fld	f27, (27*%1)(%0)"
	"\n\t"	"fld	f28, (28*%1)(%0)"
	"\n\t"	"fld	f29, (29*%1)(%0)"
	"\n\t"	"fld	f30, (30*%1)(%0)"
	"\n\t"	"fld	f31, (31*%1)(%0)"
	   ::	"r"(fp->r_fpreg),
		"i"(sizeof(fp->r_fpreg[0])));

	// load FPCSR and disable FPU again
	riscvreg_fcsr_write(fp->r_fcsr);
	riscvreg_status_clear(SR_EF);
}

void
fpu_state_release(lwp_t *l)
{
	l->l_md.md_utf->tf_sr &= ~SR_EF;
}
