/*	$NetBSD: rmixl_cpu.c,v 1.1.2.3 2010/01/24 05:39:57 cliff Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Simon Burge for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "locators.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rmixl_cpu.c,v 1.1.2.3 2010/01/24 05:39:57 cliff Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/cpu.h>
#include <sys/lock.h>
#include <uvm/uvm_pglist.h>
#include <uvm/uvm_extern.h>
#include <mips/rmi/rmixlreg.h>
#include <mips/rmi/rmixlvar.h>
#include <mips/rmi/rmixl_cpucorevar.h>

static int	cpu_rmixl_match(device_t, cfdata_t, void *);
static void	cpu_rmixl_attach(device_t, device_t, void *);

#ifdef MULTIPROCESSOR
void		cpu_trampoline_park(void);
static void	cpu_setup_trampoline(struct device *, struct cpu_info *);
#endif

CFATTACH_DECL_NEW(cpu_rmixl, 0, cpu_rmixl_match, cpu_rmixl_attach, NULL, NULL);

static int
cpu_rmixl_match(device_t parent, cfdata_t cf, void *aux)
{
	struct cpucore_attach_args *ca = aux;
	int thread = cf->cf_loc[CPUCORECF_THREAD];

	if (!cpu_rmixl(mips_options.mips_cpu))
		return 0;

	if (strncmp(ca->ca_name, cf->cf_name, strlen(cf->cf_name)) == 0
#ifndef MULTIPROCESSOR
	    && ca->ca_thread == 0
#endif
	    && (thread == CPUCORECF_THREAD_DEFAULT || thread == ca->ca_thread))
			return 1;

	return 0;
}

static void
cpu_rmixl_attach(device_t parent, device_t self, void *aux)
{
	struct cpucore_attach_args *ca = aux;

	if (ca->ca_thread == 0 && ca->ca_core == 0) {
		struct cpu_info * const ci = curcpu();
		ci->ci_dev = self;
		self->dv_private = ci;
#ifdef MULTIPROCESSOR
	} else {
		struct pglist pglist;
		int error;

		/*
		 * Grab a page from the first 256MB to use to store
		 * exception vectors and cpu_info for this cpu.
		 */
		error = uvm_pglistalloc(PAGE_SIZE,
		    0, 0x10000000,
		    PAGE_SIZE, PAGE_SIZE, &pglist, 1, false);
		if (error) {
			aprint_error(": failed to allocte exception vectors\n");
			return;
		}

		const paddr_t pa = VM_PAGE_TO_PHYS(TAILQ_FIRST(&pglist));
		const vaddr_t va = MIPS_PHYS_TO_KSEG0(pa);
		struct cpu_info * const ci = (void *) (va + 0x400);
		memset((void *)va, 0, PAGE_SIZE);
		ci->ci_ebase = va;
		ci->ci_ebase_pa = pa;
		ci->ci_dev = self;
		KASSERT(ca->ca_core < 8);
		KASSERT(ca->ca_thread < 4);
		ci->ci_cpuid = (ca->ca_core << 2) | ca->ca_thread;
		self->dv_private = ci;

		cpu_setup_trampoline(self, ci);

#endif
	}
	aprint_normal("\n");
}

#ifdef MULTIPROCESSOR
void
cpu_trampoline_park(void)
{
}

#if 0
static void
cpu_setup_trampoline(struct device *self, struct cpu_info *ci)
{
	volatile struct rmixlfw_cpu_wakeup_info *wip;
	u_int cpu, core, thread;
	uint32_t ipi;
	int32_t addr;
	uint64_t gp;
	uint64_t sp;
	uint32_t mask;
	volatile uint32_t *maskp;
	__cpu_simple_lock_t *llk;
	volatile uint32_t *xflag;			/* ??? */
	extern void cpu_wakeup_trampoline(void);

	cpu = ci->ci_cpuid;
	core = cpu >> 2;
	thread = cpu & __BITS(1,0);
printf("\n%s: cpu %d, core %d, thread %d\n", __func__, cpu, core, thread);

	wip = &rmixl_configuration.rc_cpu_wakeup_info[cpu];
printf("%s: wip %p\n", __func__, wip);

	llk = (__cpu_simple_lock_t *)(intptr_t)wip->loader_lock;
printf("%s: llk %p\n", __func__, llk);

	/* XXX WTF */
	xflag = (volatile uint32_t *)(intptr_t)(wip->loader_lock + 0x2c);
printf("%s: xflag %p, %#x\n", __func__, xflag, *xflag);

	ipi = (thread << RMIXL_PIC_IPIBASE_ID_THREAD_SHIFT)
	    | (core << RMIXL_PIC_IPIBASE_ID_CORE_SHIFT)
	    | RMIXLFW_IPI_WAKEUP;
printf("%s: ipi %#x\n", __func__, ipi);

	/* entry addr must be uncached, use KSEG1 */
	addr = (int32_t)MIPS_PHYS_TO_KSEG1(
			MIPS_KSEG0_TO_PHYS(cpu_wakeup_trampoline));
printf("%s: addr %#x\n", __func__, addr);

	__asm__ volatile("move	%0, $gp\n" : "=r"(gp));
printf("%s: gp %#"PRIx64"\n", __func__, gp);

	sp = (256 * 1024) - 32;			/* XXX TMP FIXME */
	sp = MIPS_PHYS_TO_KSEG1(sp);
printf("%s: sp %#"PRIx64"\n", __func__, sp);

	maskp = (uint32_t *)(intptr_t)wip->global_wakeup_mask;
printf("%s: maskp %p\n", __func__, maskp);

	__cpu_simple_lock(llk);

	wip->entry.addr = addr;
	wip->entry.args = 0;
if (0) {
	wip->entry.sp = sp;
	wip->entry.gp = gp;
}

	mask = *maskp;
	mask |= 1 << cpu;
	*maskp = mask;

#if 0
	*xflag = mask;	/* XXX */
#endif

	RMIXL_IOREG_WRITE(RMIXL_PIC_IPIBASE, ipi);

	__cpu_simple_unlock(llk);

	Debugger();
}
#else
static uint64_t argv[4] = { 0x1234,  0x2345, 0x3456, 0x4567 };
static void
cpu_setup_trampoline(struct device *self, struct cpu_info *ci)
{
	void (*wakeup_cpu)(void *, void *, unsigned int);
	extern void cpu_wakeup_trampoline(void);
	extern void rmixlfw_wakeup_cpu(void *, void *, u_int64_t, void *);

	wakeup_cpu = (void *)rmixl_configuration.rc_psb_info.wakeup;

	rmixlfw_wakeup_cpu(cpu_wakeup_trampoline, argv,
		1 << ci->ci_cpuid, wakeup_cpu);
}
#endif	/* 0 */
#endif	/* MULTIPROCESSOR */
