/*	$NetBSD: xenfunc.c,v 1.2.16.2 2008/01/09 01:50:17 matt Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Christian Limpach.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
void xen_update_descriptor(union descriptor *, union descriptor *);

void 
invlpg(vaddr_t addr)
{
	int s = splvm();
	xpq_queue_invlpg(addr);
	xpq_flush_queue();
	splx(s);
}  

#ifndef __x86_64__
void
lldt(u_short sel)
{

	/* __PRINTK(("ldt %x\n", IDXSELN(sel))); */
	if (sel == GSEL(GLDT_SEL, SEL_KPL))
		xen_set_ldt((vaddr_t)ldt, NLDT);
	else
		xen_set_ldt(cpu_info_primary.ci_gdt[IDXSELN(sel)].ld.ld_base,
		    cpu_info_primary.ci_gdt[IDXSELN(sel)].ld.ld_entries);
}
#endif

void
ltr(u_short sel)
{
	__PRINTK(("XXX ltr not supported\n"));
}

void
lcr0(u_int val)
{
	__PRINTK(("XXX lcr0 not supported\n"));
}

u_int
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
	xpq_flush_queue();
	splx(s);
}
#endif

void
tlbflush(void)
{
	int s = splvm();
	xpq_queue_tlb_flush();
	xpq_flush_queue();
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
#ifdef XEN3
	return HYPERVISOR_shared_info->vcpu_info[0].arch.cr2; /* XXX curcpu */
#else
	return 0;
#endif
}
