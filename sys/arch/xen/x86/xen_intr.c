/*	$NetBSD: xen_intr.c,v 1.7.2.1 2008/07/03 18:37:57 simonb Exp $	*/

/*-
 * Copyright (c) 1998, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum, and by Jason R. Thorpe.
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
__KERNEL_RCSID(0, "$NetBSD: xen_intr.c,v 1.7.2.1 2008/07/03 18:37:57 simonb Exp $");

#include <sys/param.h>

#include <machine/cpu.h>
#include <machine/intr.h>

/*
 * Add a mask to cpl, and return the old value of cpl.
 */
int
splraise(int nlevel)
{
	int olevel;
	struct cpu_info *ci = curcpu();

	olevel = ci->ci_ilevel;
	if (nlevel > olevel)
		ci->ci_ilevel = nlevel;
	__insn_barrier();
	return (olevel);
}

/*
 * Restore a value to cpl (unmasking interrupts).  If any unmasked
 * interrupts are pending, call Xspllower() to process them.
 */
void
spllower(int nlevel)
{
	struct cpu_info *ci = curcpu();
	uint32_t imask;
	u_long psl;

	if (ci->ci_ilevel <= nlevel)
		return;

	__insn_barrier();

	imask = IUNMASK(ci, nlevel);
	psl = x86_read_psl();
	x86_disable_intr();
	if (ci->ci_ipending & imask) {
		KASSERT(psl == 0);
		Xspllower(nlevel);
		/* Xspllower does enable_intr() */
	} else {
		ci->ci_ilevel = nlevel;
		x86_write_psl(psl);
	}
}

void
x86_disable_intr(void)
{
	__cli();
}

void
x86_enable_intr(void)
{
	__sti();
}

u_long
x86_read_psl(void)
{

	return (curcpu()->ci_vcpu->evtchn_upcall_mask);
}

void
x86_write_psl(u_long psl)
{
	struct cpu_info *ci = curcpu();

	ci->ci_vcpu->evtchn_upcall_mask = psl;
	x86_lfence();
	if (ci->ci_vcpu->evtchn_upcall_pending && psl == 0) {
	    	hypervisor_force_callback();
	}
}
