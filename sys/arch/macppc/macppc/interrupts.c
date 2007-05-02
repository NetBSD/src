/*	$NetBSD: interrupts.c,v 1.1.2.1 2007/05/02 03:02:34 macallan Exp $ */

/*-
 * Copyright (c) 2007 Michael Lorenz
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
 * 3. Neither the name of The NetBSD Foundation nor the names of its
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: interrupts.c,v 1.1.2.1 2007/05/02 03:02:34 macallan Exp $");

#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/kernel.h>

#include <machine/autoconf.h>
#include <arch/powerpc/pic/picvar.h>
#include <dev/ofw/openfirm.h>

#include "opt_interrupt.h"

void
init_interrupt(void)
{

	pic_init();
#ifdef PIC_OHARE
	if (init_ohare())
		goto done;
#endif
#ifdef PIC_GRANDCENTRAL
	if (init_grandcentral())
		goto done;
#endif
#ifdef PIC_HEATHROW
	if (init_heathrow())
		goto done;
#endif
#ifdef PIC_OPENPIC
	if (init_openpic())
		goto done;
#endif
	panic("%s: no supported interrupt controller found", __func__);
done:
	oea_install_extint(pic_ext_intr);
}

int
splraise(int ncpl)
{
	struct cpu_info *ci = curcpu();
	int ocpl;

	__asm volatile("sync; eieio");	/* don't reorder.... */
	ocpl = ci->ci_cpl;
	ci->ci_cpl = ocpl | ncpl;
	__asm volatile("sync; eieio");	/* reorder protect */
	return ocpl;
}

void
splx(int ncpl)
{

	struct cpu_info *ci = curcpu();
	__asm volatile("sync; eieio");	/* reorder protect */
	ci->ci_cpl = ncpl;
	if (ci->ci_ipending & ~ncpl)
		pic_do_pending_int();
	__asm volatile("sync; eieio");	/* reorder protect */
}

int
spllower(int ncpl)
{
	struct cpu_info *ci = curcpu();
	int ocpl;

	__asm volatile("sync; eieio");	/* reorder protect */
	ocpl = ci->ci_cpl;
	ci->ci_cpl = ncpl;
	if (ci->ci_ipending & ~ncpl)
		pic_do_pending_int();
	__asm volatile("sync; eieio");	/* reorder protect */
	return ocpl;
}

/* Following code should be implemented with lwarx/stwcx to avoid
 * the disable/enable. i need to read the manual once more.... */
void
softintr(int ipl)
{
	int msrsave;

	msrsave = mfmsr();
	mtmsr(msrsave & ~PSL_EE);
	curcpu()->ci_ipending |= 1 << ipl;
	mtmsr(msrsave);
}
