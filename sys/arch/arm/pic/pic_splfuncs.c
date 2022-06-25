/*	$NetBSD: pic_splfuncs.c,v 1.23 2022/06/25 12:39:46 jmcneill Exp $	*/
/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas.
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

#include "opt_modular.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pic_splfuncs.c,v 1.23 2022/06/25 12:39:46 jmcneill Exp $");

#define _INTR_PRIVATE
#include <sys/param.h>
#include <sys/atomic.h>
#include <sys/evcnt.h>
#include <sys/lwp.h>
#include <sys/kernel.h>

#include <dev/cons.h>

#include <arm/armreg.h>
#include <arm/cpu.h>
#include <arm/cpufunc.h>
#include <arm/locore.h>	/* for compat aarch64 */

#include <arm/pic/picvar.h>

static int	pic_default_splraise(int);
static int	pic_default_spllower(int);
static void	pic_default_splx(int);

int (*pic_splraise)(int) = pic_default_splraise;
int (*pic_spllower)(int) = pic_default_spllower;
void (*pic_splx)(int) = pic_default_splx;

static int
pic_default_splraise(int newipl)
{
	struct cpu_info * const ci = curcpu();
	const int oldipl = ci->ci_cpl;
	KASSERT(newipl < NIPL);
	if (newipl > ci->ci_cpl) {
		pic_set_priority(ci, newipl);
	}
	return oldipl;
}

static int
pic_default_spllower(int newipl)
{
	struct cpu_info * const ci = curcpu();
	const int oldipl = ci->ci_cpl;
	KASSERT(panicstr || newipl <= ci->ci_cpl);
	if (newipl < ci->ci_cpl) {
		register_t psw = DISABLE_INTERRUPT_SAVE();
		ci->ci_intr_depth++;
		pic_do_pending_ints(psw, newipl, NULL);
		ci->ci_intr_depth--;
		if ((psw & I32_bit) == 0 || newipl == IPL_NONE)
			ENABLE_INTERRUPT();
		cpu_dosoftints();
	}
	return oldipl;
}

static void
pic_default_splx(int savedipl)
{
	struct cpu_info * const ci = curcpu();
	KASSERT(savedipl < NIPL);

	if (__predict_false(savedipl == ci->ci_cpl)) {
		return;
	}

	if (ci->ci_cpl >= IPL_VM) {
		register_t psw = DISABLE_INTERRUPT_SAVE();
		KASSERTMSG(panicstr != NULL || savedipl < ci->ci_cpl,
		    "splx(%d) to a higher ipl than %d", savedipl, ci->ci_cpl);

		ci->ci_intr_depth++;
		pic_do_pending_ints(psw, savedipl, NULL);
		ci->ci_intr_depth--;
		KASSERTMSG(ci->ci_cpl == savedipl, "cpl %d savedipl %d",
		    ci->ci_cpl, savedipl);
		if ((psw & I32_bit) == 0)
			ENABLE_INTERRUPT();
	} else {
		pic_set_priority(ci, savedipl);
	}

	cpu_dosoftints();
	KASSERTMSG(ci->ci_cpl == savedipl, "cpl %d savedipl %d",
	    ci->ci_cpl, savedipl);
}

#ifdef MODULAR
#ifdef _spllower
#undef _spllower
#endif
int _spllower(int);

#ifdef _splraise
#undef _splraise
#endif
int _splraise(int);

#ifdef splx
#undef splx
#endif
void splx(int);

int
_spllower(int newipl)
{
	return pic_spllower(newipl);
}

int
_splraise(int newipl)
{
	return pic_splraise(newipl);
}

void
splx(int savedipl)
{
	pic_splx(savedipl);
}
#endif /* !MODULAR */
