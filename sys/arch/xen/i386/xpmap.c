/*	$NetBSD: xpmap.c,v 1.2.12.1 2007/12/11 23:03:01 bouyer Exp $	*/

/*
 * Copyright (c) 2006 Manuel Bouyer.
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
 *	This product includes software developed by Manuel Bouyer.
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
 *
 */

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


#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: xpmap.c,v 1.2.12.1 2007/12/11 23:03:01 bouyer Exp $");

#include "opt_xen.h"

#include <sys/param.h>
#include <sys/systm.h>

#include <uvm/uvm.h>

#include <machine/gdt.h>
#include <machine/xenfunc.h>

#undef	XENDEBUG
/* #define XENDEBUG_SYNC */
/* #define	XENDEBUG_LOW */

#ifdef XENDEBUG
#define	XENPRINTF(x) printf x
#define	XENPRINTK(x) printk x
#define	XENPRINTK2(x) /* printk x */

static char XBUF[256];
#else
#define	XENPRINTF(x)
#define	XENPRINTK(x)
#define	XENPRINTK2(x)
#endif
#define	PRINTF(x) printf x
#define	PRINTK(x) printk x

#ifdef XEN3
#define HYPERVISOR_mmu_update_self(req, count, success_count) \
	HYPERVISOR_mmu_update((req), (count), (success_count), DOMID_SELF)
#else
#define HYPERVISOR_mmu_update_self(req, count, success_count) \
	HYPERVISOR_mmu_update((req), (count), (success_count))
#endif

static pd_entry_t
xpmap_get_bootpde(paddr_t va)
{

	return ((pd_entry_t *)xen_start_info.pt_base)[va >> L2_SHIFT];
}

static pd_entry_t
xpmap_get_vbootpde(paddr_t va)
{
	pd_entry_t pde;

	pde = xpmap_get_bootpde(va);
	if ((pde & PG_V) == 0)
		return (pde & ~PG_FRAME);
	return (pde & ~PG_FRAME) |
		(xpmap_mtop(pde & PG_FRAME) + KERNBASE);
}

static pt_entry_t *
xpmap_get_bootptep(paddr_t va)
{
	pd_entry_t pde;

	pde = xpmap_get_vbootpde(va);
	if ((pde & PG_V) == 0)
		return (void *)-1;
	return &(((pt_entry_t *)(pde & PG_FRAME))[(va & L1_MASK) >> PAGE_SHIFT]);
}

static pt_entry_t
xpmap_get_bootpte(paddr_t va)
{

	return xpmap_get_bootptep(va)[0];
}

#if defined(XENDEBUG)
static void
xpmap_dump_pt(pt_entry_t *ptp, int p)
{
	pt_entry_t pte;
	int j;
	int bufpos;

	pte = xpmap_ptom((uint32_t)ptp - KERNBASE);
	PRINTK(("%03x: %p(%p) %08x\n", p, ptp, (void *)pte, p << L2_SHIFT));

	bufpos = 0;
	for (j = 0; j < NPTEPG; j++) {
		if ((ptp[j] & PG_V) == 0)
			continue;
		pte = ptp[j] /* & PG_FRAME */;
		bufpos += sprintf(XBUF + bufpos, "%x:%03x:%08x ",
		    p, j, pte);
		if (bufpos > 70) {
			int k;
			sprintf(XBUF + bufpos, "\n");
			PRINTK((XBUF));
			bufpos = 0;
			for (k = 0; k < 1000000; k++);
		}
	}
	if (bufpos) {
		PRINTK((XBUF));
		PRINTK(("\n"));
		bufpos = 0;
	}
}
#endif

void
xpmap_init(void)
{
	pd_entry_t *xen_pdp;
	pt_entry_t *ptp, *sysptp;
	pt_entry_t pte;
	uint32_t i, j;
	int bufpos;
#if defined(XENDEBUG_LOW)
	extern char kernel_text, _etext, __bss_start, end, *esym;
#endif

	xpmap_phys_to_machine_mapping = (void *)xen_start_info.mfn_list;

	xen_pdp = (pd_entry_t *)xen_start_info.pt_base;

	/*XENPRINTK(("text %p data %p bss %p end %p esym %p\n", &kernel_text,
		   &_etext, &__bss_start, &end, esym));*/
	XENPRINTK(("xpmap_init PDP %p nkptp[1] %d upages %d xen_PDP %p p2m-map %p\n",
		   (void *)PDPpaddr, nkptp[1], UPAGES, xen_pdp,
		   xpmap_phys_to_machine_mapping));

	bufpos = 0;

	XENPRINTK(("shared_inf %08x\n", (paddr_t)xen_start_info.shared_info));
	XENPRINTK(("c0100000: %08x\n",
	    xpmap_get_bootpte(0xc0100000)));

	/* Map kernel. */

	/* Map kernel data/bss/tables. */

	/* Map ISA I/O memory. */
	
	/* Map kernel PDEs. */

	/* Install a PDE recursively mapping page directory as a page table! */

	sysptp = (pt_entry_t *)(PDPpaddr + ((1 + UPAGES) << PAGE_SHIFT));

	/* make xen's PDE and PTE pages read-only in our pagetable */
	for (i = 0; i < xen_start_info.nr_pt_frames; i++) {
		/* mark PTE page read-only in our table */
		sysptp[((xen_start_info.pt_base +
			    (i << PAGE_SHIFT) - KERNBASE) & 
			   (L2_MASK | L1_MASK)) >> PAGE_SHIFT] &= ~PG_RW;
	}

	xpq_flush_queue();

	for (i = 0; i < 1 + UPAGES + nkptp[1]; i++) {
		/* mark PTE page read-only in xen's table */
		ptp = xpmap_get_bootptep(PDPpaddr + (i << PAGE_SHIFT));
		xpq_queue_pte_update(
		    (void *)xpmap_ptom((unsigned long)ptp - KERNBASE), *ptp & ~PG_RW);
		XENPRINTK(("%03x: %p(%p) -> %08x\n", i, ptp,
			      (unsigned long)ptp - KERNTEXTOFF, *ptp));

		/* mark PTE page read-only in our table */
		sysptp[((PDPpaddr + (i << PAGE_SHIFT) - KERNBASE) & 
			   (L2_MASK | L1_MASK)) >> PAGE_SHIFT] &= ~PG_RW;

		/* update our pte's */
		ptp = (pt_entry_t *)(PDPpaddr + (i << PAGE_SHIFT));
#if 0
		pte = xpmap_ptom((uint32_t)ptp - KERNBASE);
		XENPRINTK(("%03x: %p(%p) %08x\n", i, ptp, pte, i << L2_SHIFT));
#endif
		for (j = 0; j < NPTEPG; j++) {
			if ((ptp[j] & PG_V) == 0)
				continue;
			if (ptp[j] >= KERNTEXTOFF) {
				pte = ptp[j];
				ptp[j] = (pte & ~PG_FRAME) |
					(xpmap_get_bootpte(pte & PG_FRAME) &
					    PG_FRAME);
			}
#if defined(XENDEBUG) && 0
			pte = ptp[j] /* & PG_FRAME */;
			bufpos += sprintf(XBUF + bufpos, "%x:%03x:%08x ",
			    i, j, pte);
			if (bufpos > 70) {
				int k;
				sprintf(XBUF + bufpos, "\n");
				XENPRINTK((XBUF));
				bufpos = 0;
				for (k = 0; k < 1000000; k++);
			}
		}
		if (bufpos) {
			XENPRINTK((XBUF));
			bufpos = 0;
#endif
		}
		if (i == 0)
			i = 1 + UPAGES - 1;
	}

#if 0
	for (i = 0x300; i < 0x305; i++)
		if (((pt_entry_t *)xen_start_info.pt_base)[i] & PG_V)
			xpmap_dump_pt((pt_entry_t *)
			    (xpmap_mtop(((pt_entry_t *)xen_start_info.pt_base)[i] &
				PG_FRAME) + KERNBASE), i);
	xpmap_dump_pt((pt_entry_t *)xen_start_info.pt_base, 0);
#endif

	XENPRINTK(("switching pdp: %p, %08lx, %p, %p, %p\n", (void *)PDPpaddr,
		      PDPpaddr - KERNBASE,
		      (void *)xpmap_ptom(PDPpaddr - KERNBASE),
		      (void *)xpmap_get_bootpte(PDPpaddr),
		      (void *)xpmap_mtop(xpmap_ptom(PDPpaddr - KERNBASE))));

#if defined(XENDEBUG)
	xpmap_dump_pt((pt_entry_t *)PDPpaddr, 0);
#endif

	xpq_flush_queue();

	xpq_queue_pin_table(xpmap_get_bootpte(PDPpaddr) & PG_FRAME);
	xpq_queue_pt_switch(xpmap_get_bootpte(PDPpaddr) & PG_FRAME);
	xpq_queue_unpin_table(
		xpmap_get_bootpte(xen_start_info.pt_base) & PG_FRAME);

	/* make xen's PDE and PTE pages writable in our pagetable */
	for (i = 0; i < xen_start_info.nr_pt_frames; i++) {
		/* mark PTE page writable in our table */
		ptp = &sysptp[((xen_start_info.pt_base +
				   (i << PAGE_SHIFT) - KERNBASE) & 
				  (L2_MASK | L1_MASK)) >> PAGE_SHIFT];
		xpq_queue_pte_update(
		    (void *)xpmap_ptom((unsigned long)ptp - KERNBASE), *ptp |
		    PG_RW);
	}

	xpq_flush_queue();
	XENPRINTK(("pt_switch done!\n"));
}

/*
 * Do a binary search to find out where physical memory ends on the
 * real hardware.  Xen will fail our updates if they are beyond the
 * last available page (max_page in xen/common/memory.c).
 */
paddr_t
find_pmap_mem_end(vaddr_t va)
{
	mmu_update_t r;
	int start, end, ok;
	pt_entry_t old;

	start = xen_start_info.nr_pages;
	end = HYPERVISOR_VIRT_START >> PAGE_SHIFT;

	r.ptr = (unsigned long)&PTE_BASE[x86_btop(va)];
	old = PTE_BASE[x86_btop(va)];

	while (start + 1 < end) {
		r.val = (((start + end) / 2) << PAGE_SHIFT) | PG_V;

		if (HYPERVISOR_mmu_update_self(&r, 1, &ok) < 0)
			end = (start + end) / 2;
		else
			start = (start + end) / 2;
	}
	r.val = old;
	if (HYPERVISOR_mmu_update_self(&r, 1, &ok) < 0)
		printf("pmap_mem_end find: old update failed %08x\n",
		    old);

	return end << PAGE_SHIFT;
}


#if 0
void xpmap_find_memory(paddr_t);
void
xpmap_find_memory(paddr_t first_avail)
{
	char buf[256];
	uint32_t i;
	int bufpos;
	paddr_t p;

	bufpos = 0;
	for (i = ((first_avail - KERNTEXTOFF) >> PAGE_SHIFT);
	     i < xen_start_info.nr_pages; i++) {
		/* if (xpmap_phys_to_machine_mapping[i] */
		bufpos += sprintf(buf + bufpos, "%03x:%08x:%08x ",
		    i, (uint32_t)xpmap_phys_to_machine_mapping[i],
		    (uint32_t)xpmap_mtop(xpmap_phys_to_machine_mapping[i] <<
			PAGE_SHIFT));
		p = xpmap_phys_to_machine_mapping[i];
		uvm_page_physload(p, p + 1, p, p + 1, VM_FREELIST_DEFAULT);
		    
		if (bufpos > 70) {
			int k;
			sprintf(buf + bufpos, "\n");
			XENPRINTK((buf));
			bufpos = 0;
			for (k = 0; k < 1000000; k++);
		}
	}
	if (bufpos) {
		XENPRINTK((buf));
		bufpos = 0;
	}
}
#endif
