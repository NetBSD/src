/*	$NetBSD: armadaxp_splfuncs.c,v 1.2 2013/05/29 23:50:35 rkujawa Exp $	*/
/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

Developed by Semihalf

********************************************************************************
Marvell BSD License

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File under the following licensing terms.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    *   Redistributions of source code must retain the above copyright notice,
            this list of conditions and the following disclaimer.

    *   Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

    *   Neither the name of Marvell nor the names of its contributors may be
        used to endorse or promote products derived from this software without
        specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: armadaxp_splfuncs.c,v 1.2 2013/05/29 23:50:35 rkujawa Exp $");

#define _INTR_PRIVATE

#include "opt_mvsoc.h"

#include <sys/param.h>
#include <sys/bus.h>

#include <machine/intr.h>

#include <arm/pic/picvar.h>

#include <arm/armreg.h>
#include <arm/cpu.h>
#include <arm/cpufunc.h>

#include <arm/marvell/mvsocreg.h>
#include <arm/marvell/mvsocvar.h>
#include <evbarm/armadaxp/armadaxpreg.h>

#include <evbarm/marvell/marvellreg.h>
#include <dev/marvell/marvellreg.h>

extern bus_space_handle_t mpic_cpu_handle;

#define	MPIC_WRITE(reg, val)		(bus_space_write_4(&mvsoc_bs_tag, \
					    mpic_handle, reg, val))
#define	MPIC_CPU_WRITE(reg, val)	(bus_space_write_4(&mvsoc_bs_tag, \
					    mpic_cpu_handle, reg, val))

#define	MPIC_READ(reg)			(bus_space_read_4(&mvsoc_bs_tag, \
					    mpic_handle, reg))
#define	MPIC_CPU_READ(reg)		(bus_space_read_4(&mvsoc_bs_tag, \
					    mpic_cpu_handle, reg))

int
_splraise(int newipl)
{
	struct cpu_info * const ci = curcpu();
	const int oldipl = ci->ci_cpl;
	int ctp;

	/*
	 * Disable interrupts in order to avoid disrupt 
	 * while changing the priority level that may cause
	 * mismatch between CTP and ci_cpl values.
	 */
	register_t psw = disable_interrupts(I32_bit);
	KASSERT(newipl < NIPL);
	if (newipl > ci->ci_cpl) {
		ctp = MPIC_CPU_READ(ARMADAXP_MLMB_MPIC_CTP);
		ctp &= ~(0xf << MPIC_CTP_SHIFT);
		ctp |= (newipl << MPIC_CTP_SHIFT);
		MPIC_CPU_WRITE(ARMADAXP_MLMB_MPIC_CTP, ctp);
		ci->ci_cpl = newipl;
	}
	restore_interrupts(psw);

	return oldipl;
}

int
_spllower(int newipl)
{
	struct cpu_info * const ci = curcpu();
	const int oldipl = ci->ci_cpl;
	int ctp;

	/*
	 * Disable interrupts in order to avoid disrupt 
	 * while changing the priority level that may cause
	 * mismatch between CTP and ci_cpl values.
	 */
	register_t psw = disable_interrupts(I32_bit);
	KASSERT(newipl <= ci->ci_cpl);
	if (newipl < ci->ci_cpl) {
		ctp = MPIC_CPU_READ(ARMADAXP_MLMB_MPIC_CTP);
		ctp &= ~(0xf << MPIC_CTP_SHIFT);
		ctp |= (newipl << MPIC_CTP_SHIFT);
		MPIC_CPU_WRITE(ARMADAXP_MLMB_MPIC_CTP, ctp);
		ci->ci_cpl = newipl;
	}
	restore_interrupts(psw);

#ifdef __HAVE_FAST_SOFTINTS
	cpu_dosoftints();
#endif
	return oldipl;
}

void
splx(int savedipl)
{
	struct cpu_info * const ci = curcpu();
	int ctp;

	/*
	 * Disable interrupts in order to avoid disrupt 
	 * while changing the priority level that may cause
	 * mismatch between CTP and ci_cpl values.
	 */
	register_t psw = disable_interrupts(I32_bit);
	KASSERT(savedipl < NIPL);
	if (savedipl != ci->ci_cpl) {
		ctp = MPIC_CPU_READ(ARMADAXP_MLMB_MPIC_CTP);
		ctp &= ~(0xf << MPIC_CTP_SHIFT);
		ctp |= (savedipl << MPIC_CTP_SHIFT);
		MPIC_CPU_WRITE(ARMADAXP_MLMB_MPIC_CTP, ctp);
		ci->ci_cpl = savedipl;
	}
	restore_interrupts(psw);

#ifdef __HAVE_FAST_SOFTINTS
	cpu_dosoftints();
#endif
}

