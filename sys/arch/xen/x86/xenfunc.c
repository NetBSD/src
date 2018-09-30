/*	$NetBSD: xenfunc.c,v 1.17.2.3 2018/09/30 01:45:48 pgoyette Exp $	*/

/*
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
__KERNEL_RCSID(0, "$NetBSD: xenfunc.c,v 1.17.2.3 2018/09/30 01:45:48 pgoyette Exp $");

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

void xen_set_ldt(vaddr_t, uint32_t);

void 
invlpg(vaddr_t addr)
{
	int s = splvm(); /* XXXSMP */
	xpq_queue_invlpg(addr);
	splx(s);
}  

void
lidt(struct region_descriptor *rd)
{
	/* 
	 * We need to do this because we can't assume kmem_alloc(9)
	 * will be available at the boot stage when this is called.
	 */
	static char xen_idt_page[PAGE_SIZE] __attribute__((__aligned__ (PAGE_SIZE)));

	struct trap_info *xen_idt = (void * )xen_idt_page;
	int xen_idt_idx = 0;
	
	struct trap_info * idd = (void *) rd->rd_base;
	const int nidt = rd->rd_limit / (sizeof *idd); 

	int i;

	/*
	 * Sweep in all initialised entries, consolidate them back to
	 * back in the requestor array.
	 */
	for (i = 0; i < nidt; i++) {
		if (idd->address == 0) /* Skip gap */
			continue;

		/* Copy over entry */
		xen_idt[xen_idt_idx++] = idd[i];
	}

#if defined(__x86_64__)
	/* page needs to be r/o */
	pmap_changeprot_local((vaddr_t) xen_idt, VM_PROT_READ);
#endif /* __x86_64 */

	/* Hook it up in the hypervisor */
	if (HYPERVISOR_set_trap_table(xen_idt))
		panic("HYPERVISOR_set_trap_table() failed");

#if defined(__x86_64__)
	/* reset */
	pmap_changeprot_local((vaddr_t) xen_idt, VM_PROT_READ|VM_PROT_WRITE);
#endif /* __x86_64 */
}

void
lldt(u_short sel)
{
#ifndef __x86_64__
	struct cpu_info *ci;

	ci = curcpu();

	if (ci->ci_curldt == sel)
		return;
	if (sel == GSEL(GLDT_SEL, SEL_KPL))
		xen_set_ldt((vaddr_t)ldtstore, NLDT);
	else
		xen_set_ldt(ci->ci_gdt[IDXSELN(sel)].ld.ld_base,
		    ci->ci_gdt[IDXSELN(sel)].ld.ld_entries);
	ci->ci_curldt = sel;
#endif
}

void
ltr(u_short sel)
{
	panic("XXX ltr not supported\n");
}

void
lcr0(u_long val)
{
	panic("XXX lcr0 not supported\n");
}

u_long
rcr0(void)
{
	/* XXX: handle X86_CR0_TS ? */
	return 0;
}

#ifndef __x86_64__
void
lcr3(vaddr_t val)
{
	int s = splvm(); /* XXXSMP */
	xpq_queue_pt_switch(xpmap_ptom_masked(val));
	splx(s);
}
#endif

void
tlbflush(void)
{
	int s = splvm(); /* XXXSMP */
	xpq_queue_tlb_flush();
	splx(s);
}

void
tlbflushg(void)
{
	tlbflush();
}

register_t
rdr0(void)
{

	return HYPERVISOR_get_debugreg(0);
}

void
ldr0(register_t val)
{

	HYPERVISOR_set_debugreg(0, val);
}

register_t
rdr1(void)
{

	return HYPERVISOR_get_debugreg(1);
}

void
ldr1(register_t val)
{

	HYPERVISOR_set_debugreg(1, val);
}

register_t
rdr2(void)
{

	return HYPERVISOR_get_debugreg(2);
}

void
ldr2(register_t val)
{

	HYPERVISOR_set_debugreg(2, val);
}

register_t
rdr3(void)
{

	return HYPERVISOR_get_debugreg(3);
}

void
ldr3(register_t val)
{

	HYPERVISOR_set_debugreg(3, val);
}
register_t
rdr6(void)
{

	return HYPERVISOR_get_debugreg(6);
}

void
ldr6(register_t val)
{

	HYPERVISOR_set_debugreg(6, val);
}

register_t
rdr7(void)
{

	return HYPERVISOR_get_debugreg(7);
}

void
ldr7(register_t val)
{

	HYPERVISOR_set_debugreg(7, val);
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

#ifdef __x86_64__
void
setusergs(int gssel)
{
	HYPERVISOR_set_segment_base(SEGBASE_GS_USER_SEL, gssel);
}
#endif
