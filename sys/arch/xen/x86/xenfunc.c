/*	$NetBSD: xenfunc.c,v 1.17.2.5 2018/12/26 14:01:46 pgoyette Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: xenfunc.c,v 1.17.2.5 2018/12/26 14:01:46 pgoyette Exp $");

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

#define MAX_XEN_IDT 128

void xen_set_ldt(vaddr_t, uint32_t);

/*
 * We don't need to export these declarations, since they are used via
 * linker aliasing. They should always be accessed via the
 * corresponding wrapper function names defined in
 * x86/include/cpufunc.h and exported as __weak_alias()
 *
 * We use this rather roundabout method so that a runtime wrapper
 * function may be made available for PVHVM, which could override both
 * native and PV aliases and decide which to invoke at run time.
 */

void xen_invlpg(vaddr_t);
void xen_lidt(struct region_descriptor *);
void xen_lldt(u_short);
void xen_ltr(u_short);
void xen_lcr0(u_long);
u_long xen_rcr0(void);
void xen_tlbflush(void);
void xen_tlbflushg(void);
register_t xen_rdr0(void);
void xen_ldr0(register_t);
register_t xen_rdr1(void);
void xen_ldr1(register_t);
register_t xen_rdr2(void);
void xen_ldr2(register_t);
register_t xen_rdr3(void);
void xen_ldr3(register_t);
register_t xen_rdr6(void);
void xen_ldr6(register_t);
register_t xen_rdr7(void);
void xen_ldr7(register_t);
void xen_wbinvd(void);
vaddr_t xen_rcr2(void);

__weak_alias(invlpg, xen_invlpg);
__weak_alias(lidt, xen_lidt);
__weak_alias(lldt, xen_lldt);
__weak_alias(ltr, xen_ltr);
__weak_alias(lcr0, xen_lcr0);
__weak_alias(rcr0, xen_rcr0);
__weak_alias(tlbflush, xen_tlbflush);
__weak_alias(tlbflushg, xen_tlbflushg);
__weak_alias(rdr0, xen_rdr0);
__weak_alias(ldr0, xen_ldr0);
__weak_alias(rdr1, xen_rdr1);
__weak_alias(ldr1, xen_ldr1);
__weak_alias(rdr2, xen_rdr2);
__weak_alias(ldr2, xen_ldr2);
__weak_alias(rdr3, xen_rdr3);
__weak_alias(ldr3, xen_ldr3);
__weak_alias(rdr6, xen_rdr6);
__weak_alias(ldr6, xen_ldr6);
__weak_alias(rdr7, xen_rdr7);
__weak_alias(ldr7, xen_ldr7);
__weak_alias(wbinvd, xen_wbinvd);
__weak_alias(rcr2, xen_rcr2);

#ifdef __x86_64__
void xen_setusergs(int);
__weak_alias(setusergs, xen_setusergs);
#else
void xen_lcr3(vaddr_t);
__weak_alias(lcr3, xen_lcr3);

#endif

void 
xen_invlpg(vaddr_t addr)
{
	int s = splvm(); /* XXXSMP */
	xpq_queue_invlpg(addr);
	splx(s);
}  

void
xen_lidt(struct region_descriptor *rd)
{
	/* 
	 * We need to do this because we can't assume kmem_alloc(9)
	 * will be available at the boot stage when this is called.
	 */
	static char xen_idt_page[PAGE_SIZE] __attribute__((__aligned__ (PAGE_SIZE)));
	memset(xen_idt_page, 0, PAGE_SIZE);
	
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
		if (idd[i].address == 0) /* Skip gap */
			continue;
		KASSERT(xen_idt_idx < MAX_XEN_IDT);
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
xen_lldt(u_short sel)
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
xen_ltr(u_short sel)
{
	panic("XXX ltr not supported\n");
}

void
xen_lcr0(u_long val)
{
	panic("XXX lcr0 not supported\n");
}

u_long
xen_rcr0(void)
{
	/* XXX: handle X86_CR0_TS ? */
	return 0;
}

#ifndef __x86_64__
void
xen_lcr3(vaddr_t val)
{
	int s = splvm(); /* XXXSMP */
	xpq_queue_pt_switch(xpmap_ptom_masked(val));
	splx(s);
}
#endif

void
xen_tlbflush(void)
{
	int s = splvm(); /* XXXSMP */
	xpq_queue_tlb_flush();
	splx(s);
}

void
xen_tlbflushg(void)
{
	tlbflush();
}

register_t
xen_rdr0(void)
{

	return HYPERVISOR_get_debugreg(0);
}

void
xen_ldr0(register_t val)
{

	HYPERVISOR_set_debugreg(0, val);
}

register_t
xen_rdr1(void)
{

	return HYPERVISOR_get_debugreg(1);
}

void
xen_ldr1(register_t val)
{

	HYPERVISOR_set_debugreg(1, val);
}

register_t
xen_rdr2(void)
{

	return HYPERVISOR_get_debugreg(2);
}

void
xen_ldr2(register_t val)
{

	HYPERVISOR_set_debugreg(2, val);
}

register_t
xen_rdr3(void)
{

	return HYPERVISOR_get_debugreg(3);
}

void
xen_ldr3(register_t val)
{

	HYPERVISOR_set_debugreg(3, val);
}
register_t
xen_rdr6(void)
{

	return HYPERVISOR_get_debugreg(6);
}

void
xen_ldr6(register_t val)
{

	HYPERVISOR_set_debugreg(6, val);
}

register_t
xen_rdr7(void)
{

	return HYPERVISOR_get_debugreg(7);
}

void
xen_ldr7(register_t val)
{

	HYPERVISOR_set_debugreg(7, val);
}

void
xen_wbinvd(void)
{

	xpq_flush_cache();
}

vaddr_t
xen_rcr2(void)
{
	return curcpu()->ci_vcpu->arch.cr2;
}

#ifdef __x86_64__
void
xen_setusergs(int gssel)
{
	HYPERVISOR_set_segment_base(SEGBASE_GS_USER_SEL, gssel);
}
#endif
