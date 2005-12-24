/*	$NetBSD: xenfunc.h,v 1.9 2005/12/24 20:07:48 perry Exp $	*/

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


#ifndef _XEN_XENFUNC_H_
#define _XEN_XENFUNC_H_

#include <machine/xen.h>
#include <machine/hypervisor.h>
#include <machine/evtchn.h>
#include <machine/xenpmap.h>
#include <machine/pte.h>

#ifdef XENDEBUG_LOW
#define	__PRINTK(x) printk x
#else
#define	__PRINTK(x)
#endif

void xen_set_ldt(vaddr_t, uint32_t);
void xen_update_descriptor(union descriptor *, union descriptor *);

static inline void 
invlpg(u_int addr)
{
	int s = splvm();
	xpq_queue_invlpg(addr);
	xpq_flush_queue();
	splx(s);
}  

static inline void
lldt(u_short sel)
{

	/* __PRINTK(("ldt %x\n", IDXSELN(sel))); */
	if (sel == GSEL(GLDT_SEL, SEL_KPL))
		xen_set_ldt((vaddr_t)ldt, NLDT);
	else
		xen_set_ldt(cpu_info_primary.ci_gdt[IDXSELN(sel)].ld.ld_base,
		    cpu_info_primary.ci_gdt[IDXSELN(sel)].ld.ld_entries);
}

static inline void
ltr(u_short sel)
{
	__PRINTK(("XXX ltr not supported\n"));
}

static inline void
lcr0(u_int val)
{
	__PRINTK(("XXX lcr0 not supported\n"));
}

static inline u_int
rcr0(void)
{
	__PRINTK(("XXX rcr0 not supported\n"));
	return 0;
}

#define lcr3(_v) _lcr3((_v), __FILE__, __LINE__)
static inline void
_lcr3(u_int val, const char *file, int line)
{
	int s = splvm();
/* 	__PRINTK(("lcr3 %08x at %s:%d\n", val, file, line)); */
	xpq_queue_pt_switch(xpmap_ptom(val) & PG_FRAME);
	xpq_flush_queue();
	splx(s);
}

static inline void
tlbflush(void)
{
	int s = splvm();
	xpq_queue_tlb_flush();
	xpq_flush_queue();
	splx(s);
}

#define	tlbflushg()	tlbflush()	/* we don't use PGE */

static inline u_int
rdr6(void)
{
	u_int val;

	val = HYPERVISOR_get_debugreg(6);
	return val;
}

static inline void
ldr6(u_int val)
{

	HYPERVISOR_set_debugreg(6, val);
}

#endif /* _XEN_XENFUNC_H_ */
