/* $NetBSD: gic_splfuncs.c,v 1.5 2021/10/30 18:44:24 jmcneill Exp $ */

/*-
 * Copyright (c) 2021 Jared McNeill <jmcneill@invisible.ca>
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gic_splfuncs.c,v 1.5 2021/10/30 18:44:24 jmcneill Exp $");

#include <sys/param.h>
#include <sys/atomic.h>
#include <sys/kernel.h>
#include <sys/lwp.h>

#include <arm/pic/picvar.h>
#include <arm/cpu.h>
#include <arm/cpufunc.h>
#include <arm/locore.h>

#include <arm/cortex/gic_splfuncs.h>

/* Prototypes for functions in gic_splfuncs_<arch>.S */
int	gic_splraise(int);
void	gic_splx(int);

/* Local functions */
void	Xgic_splx(int);	

static int
gic_spllower(int newipl)
{
	struct cpu_info * const ci = curcpu();
	const int oldipl = ci->ci_cpl;
	KASSERT(panicstr || newipl <= ci->ci_cpl);
	if (newipl < ci->ci_cpl) {
		register_t psw = DISABLE_INTERRUPT_SAVE();
		ci->ci_intr_depth++;
		pic_do_pending_ints(psw, newipl, NULL);
		ci->ci_intr_depth--;
		if ((psw & I32_bit) == 0 || newipl == IPL_NONE) {
			ENABLE_INTERRUPT();
		}
		cpu_dosoftints();
	}

	return oldipl;
}

void
Xgic_splx(int newipl)
{
	struct cpu_info *ci = curcpu();
	register_t psw;

	if (newipl >= ci->ci_cpl) {
		return;
	}

	psw = DISABLE_INTERRUPT_SAVE();
	ci->ci_intr_depth++;
	pic_do_pending_ints(psw, newipl, NULL);
	ci->ci_intr_depth--;
	if ((psw & I32_bit) == 0) {
		ENABLE_INTERRUPT();
	}

	cpu_dosoftints();
}

void
gic_spl_init(void)
{
	pic_splraise = gic_splraise;
	pic_spllower = gic_spllower;
	pic_splx = gic_splx;
}
