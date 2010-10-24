/*	$NetBSD: xenfunc.c,v 1.7.12.2 2010/10/24 22:48:22 jym Exp $	*/

/*
 *
 * Copyright (c) 2004 Christian Limpach.
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: xenfunc.c,v 1.7.12.2 2010/10/24 22:48:22 jym Exp $");

#include <sys/param.h>

#include <uvm/uvm_extern.h>

#include <machine/intr.h>
#include <machine/vmparam.h>
#include <machine/pmap.h>
#include <xen/xen.h>
#include <xen/hypervisor.h>
//#include <xen/evtchn.h>
#include <xen/xenpmap.h>
#include <machine/pte.h>

#ifdef XENDEBUG_LOW
#define	__PRINTK(x) printk x
#else
#define	__PRINTK(x)
#endif

void xen_set_ldt(vaddr_t, uint32_t);

void 
invlpg(vaddr_t addr)
{
	int s = splvm();
	xpq_queue_invlpg(addr);
	splx(s);
}  

void
lldt(u_short sel)
{
#ifndef __x86_64__
	struct cpu_info *ci;

	ci = curcpu();

	if (ci->ci_curldt == sel)
		return;
	/* __PRINTK(("ldt %x\n", IDXSELN(sel))); */
	if (sel == GSEL(GLDT_SEL, SEL_KPL))
		xen_set_ldt((vaddr_t)ldt, NLDT);
	else
		xen_set_ldt(ci->ci_gdt[IDXSELN(sel)].ld.ld_base,
		    ci->ci_gdt[IDXSELN(sel)].ld.ld_entries);
	ci->ci_curldt = sel;
#endif
}

void
ltr(u_short sel)
{
	__PRINTK(("XXX ltr not supported\n"));
}

void
lcr0(u_long val)
{
	__PRINTK(("XXX lcr0 not supported\n"));
}

u_long
rcr0(void)
{
	__PRINTK(("XXX rcr0 not supported\n"));
	return 0;
}

#ifndef __x86_64__
void
lcr3(vaddr_t val)
{
	int s = splvm();
	xpq_queue_pt_switch(xpmap_ptom_masked(val));
	splx(s);
}
#endif

void
tlbflush(void)
{
	int s = splvm();
	xpq_queue_tlb_flush();
	splx(s);
}

void
tlbflushg(void)
{
	tlbflush();
}

vaddr_t
rdr6(void)
{
	u_int val;

	val = HYPERVISOR_get_debugreg(6);
	return val;
}

void
ldr6(vaddr_t val)
{

	HYPERVISOR_set_debugreg(6, val);
}

void
wbinvd(void)
{

	xpq_flush_cache();
}

vaddr_t
rcr2(void)
{
	return curcpu()->ci_vcpu->arch.cr2;
}
