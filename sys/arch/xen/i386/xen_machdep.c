/*	$NetBSD: xen_machdep.c,v 1.15 2006/01/15 22:09:51 bouyer Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: xen_machdep.c,v 1.15 2006/01/15 22:09:51 bouyer Exp $");

#include "opt_xen.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/boot_flag.h>
#include <sys/mount.h>
#include <sys/reboot.h>

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

volatile shared_info_t *HYPERVISOR_shared_info;
union start_info_union start_info_union;

void xen_failsafe_handler(void);

#ifdef XEN3
#define HYPERVISOR_mmu_update_self(req, count, success_count) \
	HYPERVISOR_mmu_update((req), (count), (success_count), DOMID_SELF)
#else
#define HYPERVISOR_mmu_update_self(req, count, success_count) \
	HYPERVISOR_mmu_update((req), (count), (success_count))
#endif

void
xen_failsafe_handler(void)
{

	panic("xen_failsafe_handler called!\n");
}


void
xen_update_descriptor(union descriptor *table, union descriptor *entry)
{
	paddr_t pa;
	pt_entry_t *ptp;

	ptp = kvtopte((vaddr_t)table);
	pa = (*ptp & PG_FRAME) | ((vaddr_t)table & ~PG_FRAME);
	if (HYPERVISOR_update_descriptor(pa, entry->raw[0], entry->raw[1]))
		panic("HYPERVISOR_update_descriptor failed\n");
}

void
xen_set_ldt(vaddr_t base, uint32_t entries)
{
	vaddr_t va;
	pt_entry_t *ptp, *maptp;
	int s;

	for (va = base; va < base + entries * sizeof(union descriptor);
	     va += PAGE_SIZE) {
		KASSERT(va >= VM_MIN_KERNEL_ADDRESS);
		ptp = kvtopte(va);
		maptp = (pt_entry_t *)vtomach((vaddr_t)ptp);
		XENPRINTF(("xen_set_ldt %p %d %p %p\n", (void *)base,
			      entries, ptp, maptp));
		PTE_CLEARBITS(ptp, maptp, PG_RW);
	}
	s = splvm();
	PTE_UPDATES_FLUSH();

	xpq_queue_set_ldt(base, entries);
	xpq_flush_queue();
	splx(s);
}

void
lgdt(struct region_descriptor *rdp)
{

	panic("lgdt %p %08x\n", (void *)rdp->rd_base, rdp->rd_limit);
}

void
xen_parse_cmdline(int what, union xen_cmdline_parseinfo *xcp)
{
	char _cmd_line[128], *cmd_line, *opt, *s;
	int b, i, ipidx = 0;
	uint32_t xi_ip[5];

	cmd_line = strncpy(_cmd_line, xen_start_info.cmd_line, 128);
	cmd_line[127] = '\0';

	switch (what) {
	case XEN_PARSE_BOOTDEV:
		xcp->xcp_bootdev[0] = 0;
		break;
	case XEN_PARSE_CONSOLE:
		xcp->xcp_console[0] = 0;
		break;
	}

	while (cmd_line && *cmd_line) {
		opt = cmd_line;
		cmd_line = strchr(opt, ' ');
		if (cmd_line)
			*cmd_line = 0;

		switch (what) {
		case XEN_PARSE_BOOTDEV:
			if (strncasecmp(opt, "bootdev=", 8) == 0)
				strncpy(xcp->xcp_bootdev, opt + 8,
				    sizeof(xcp->xcp_console));
			break;

		case XEN_PARSE_NETINFO:
			if (xcp->xcp_netinfo.xi_root &&
			    strncasecmp(opt, "nfsroot=", 8) == 0)
				strncpy(xcp->xcp_netinfo.xi_root, opt + 8,
				    MNAMELEN);

			if (strncasecmp(opt, "ip=", 3) == 0) {
				memset(xi_ip, 0, sizeof(xi_ip));
				opt += 3;
				ipidx = 0;
				while (opt && *opt) {
					s = opt;
					opt = strchr(opt, ':');
					if (opt)
						*opt = 0;

					switch (ipidx) {
					case 0:	/* ip */
					case 1:	/* nfs server */
					case 2:	/* gw */
					case 3:	/* mask */
					case 4:	/* host */
						if (*s == 0)
							break;
						for (i = 0; i < 4; i++) {
							b = strtoul(s, &s, 10);
							xi_ip[ipidx] = b + 256
								* xi_ip[ipidx];
							if (*s != '.')
								break;
							s++;
						}
						if (i < 3)
							xi_ip[ipidx] = 0;
						break;
					case 5:	/* interface */
						if (!strncmp(s, "xennet", 6))
							s += 6;
						else if (!strncmp(s, "eth", 3))
							s += 3;
						else
							break;
						if (xcp->xcp_netinfo.xi_ifno
						    == strtoul(s, NULL, 10))
							memcpy(xcp->
							    xcp_netinfo.xi_ip,
							    xi_ip,
							    sizeof(xi_ip));
						break;
					}
					ipidx++;

					if (opt)
						*opt++ = ':';
				}
			}
			break;

		case XEN_PARSE_CONSOLE:
			if (strncasecmp(opt, "console=", 8) == 0)
				strncpy(xcp->xcp_console, opt + 8,
				    sizeof(xcp->xcp_console));
			break;

		case XEN_PARSE_BOOTFLAGS:
			if (*opt == '-') {
				opt++;
				while(*opt != '\0') {
					BOOT_FLAG(*opt, boothowto);
					opt++;
				}
			}
			break;
		}

		if (cmd_line)
			*cmd_line++ = ' ';
	}
}



static pd_entry_t
xpmap_get_bootpde(paddr_t va)
{

	return ((pd_entry_t *)xen_start_info.pt_base)[va >> PDSHIFT];
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
	return &(((pt_entry_t *)(pde & PG_FRAME))[(va & PT_MASK) >> PAGE_SHIFT]);
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
	PRINTK(("%03x: %p(%p) %08x\n", p, ptp, (void *)pte, p << PDSHIFT));

	bufpos = 0;
	for (j = 0; j < PTES_PER_PTP; j++) {
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
	XENPRINTK(("xpmap_init PDP %p nkpde %d upages %d xen_PDP %p p2m-map %p\n",
		   (void *)PDPpaddr, nkpde, UPAGES, xen_pdp,
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
			   (PD_MASK | PT_MASK)) >> PAGE_SHIFT] &= ~PG_RW;
	}

	xpq_flush_queue();

	for (i = 0; i < 1 + UPAGES + nkpde; i++) {
		/* mark PTE page read-only in xen's table */
		ptp = xpmap_get_bootptep(PDPpaddr + (i << PAGE_SHIFT));
		xpq_queue_pte_update(
		    (void *)xpmap_ptom((unsigned long)ptp - KERNBASE), *ptp & ~PG_RW);
		XENPRINTK(("%03x: %p(%p) -> %08x\n", i, ptp,
			      (unsigned long)ptp - KERNTEXTOFF, *ptp));

		/* mark PTE page read-only in our table */
		sysptp[((PDPpaddr + (i << PAGE_SHIFT) - KERNBASE) & 
			   (PD_MASK | PT_MASK)) >> PAGE_SHIFT] &= ~PG_RW;

		/* update our pte's */
		ptp = (pt_entry_t *)(PDPpaddr + (i << PAGE_SHIFT));
#if 0
		pte = xpmap_ptom((uint32_t)ptp - KERNBASE);
		XENPRINTK(("%03x: %p(%p) %08x\n", i, ptp, pte, i << PDSHIFT));
#endif
		for (j = 0; j < PTES_PER_PTP; j++) {
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

	xpq_queue_pin_table(xpmap_get_bootpte(PDPpaddr) & PG_FRAME,
	    XPQ_PIN_L2_TABLE);
	xpq_queue_pt_switch(xpmap_get_bootpte(PDPpaddr) & PG_FRAME);
	xpq_queue_unpin_table(
		xpmap_get_bootpte(xen_start_info.pt_base) & PG_FRAME);

	/* make xen's PDE and PTE pages writable in our pagetable */
	for (i = 0; i < xen_start_info.nr_pt_frames; i++) {
		/* mark PTE page writable in our table */
		ptp = &sysptp[((xen_start_info.pt_base +
				   (i << PAGE_SHIFT) - KERNBASE) & 
				  (PD_MASK | PT_MASK)) >> PAGE_SHIFT];
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


#ifdef XENDEBUG
void xpq_debug_dump(void);
#endif

#define XPQUEUE_SIZE 2048
static mmu_update_t xpq_queue[XPQUEUE_SIZE];
static int xpq_idx = 0;

void
xpq_flush_queue()
{
	int i, ok;

	XENPRINTK2(("flush queue %p entries %d\n", xpq_queue, xpq_idx));
	for (i = 0; i < xpq_idx; i++)
		XENPRINTK2(("%d: %p %08x\n", i, (u_int)xpq_queue[i].ptr,
		    (u_int)xpq_queue[i].val));
	if (xpq_idx != 0 &&
	    HYPERVISOR_mmu_update_self(xpq_queue, xpq_idx, &ok) < 0)
		panic("HYPERVISOR_mmu_update failed\n");
	xpq_idx = 0;
}

static inline void
xpq_increment_idx(void)
{

	xpq_idx++;
	if (__predict_false(xpq_idx == XPQUEUE_SIZE))
		xpq_flush_queue();
}

void
xpq_queue_machphys_update(paddr_t ma, paddr_t pa)
{
	XENPRINTK2(("xpq_queue_machphys_update ma=%p pa=%p\n", (void *)ma, (void *)pa));
	xpq_queue[xpq_idx].ptr = ma | MMU_MACHPHYS_UPDATE;
	xpq_queue[xpq_idx].val = (pa - XPMAP_OFFSET) >> PAGE_SHIFT;
	xpq_increment_idx();
#ifdef XENDEBUG_SYNC
	xpq_flush_queue();
#endif
}

void
xpq_queue_pde_update(pd_entry_t *ptr, pd_entry_t val)
{

	xpq_queue[xpq_idx].ptr = (paddr_t)ptr;
	xpq_queue[xpq_idx].val = val;
	xpq_increment_idx();
#ifdef XENDEBUG_SYNC
	xpq_flush_queue();
#endif
}

void
xpq_queue_pte_update(pt_entry_t *ptr, pt_entry_t val)
{

	xpq_queue[xpq_idx].ptr = (paddr_t)ptr;
	xpq_queue[xpq_idx].val = val;
	xpq_increment_idx();
#ifdef XENDEBUG_SYNC
	xpq_flush_queue();
#endif
}

void
xpq_queue_unchecked_pte_update(pt_entry_t *ptr, pt_entry_t val)
{

	xpq_queue[xpq_idx].ptr = (paddr_t)ptr | MMU_NORMAL_PT_UPDATE;
	/* XXXcl UNCHECKED_PT_UPDATE */
	xpq_queue[xpq_idx].val = val;
	xpq_increment_idx();
#ifdef XENDEBUG_SYNC
	xpq_flush_queue();
#endif
}

#ifdef XEN3
void
xpq_queue_pt_switch(paddr_t pa)
{
	struct mmuext_op op;
	xpq_flush_queue();

	XENPRINTK2(("xpq_queue_pt_switch: %p %p\n", (void *)pa, (void *)pa));
	op.cmd = MMUEXT_NEW_BASEPTR;
	op.arg1.mfn = pa >> PAGE_SHIFT;
	if (HYPERVISOR_mmuext_op(&op, 1, NULL, DOMID_SELF) < 0)
		panic("xpq_queue_pt_switch");
}

void
xpq_queue_pin_table(paddr_t pa, int type)
{
	struct mmuext_op op;
	xpq_flush_queue();

	XENPRINTK2(("xpq_queue_pin_table: %p %p\n", (void *)pa, (void *)pa));
	op.arg1.mfn = pa >> PAGE_SHIFT;

	switch (type) {
	case XPQ_PIN_L1_TABLE:
		op.cmd = MMUEXT_PIN_L1_TABLE;
		break;
	case XPQ_PIN_L2_TABLE:
		op.cmd = MMUEXT_PIN_L2_TABLE;
		break;
	}
	if (HYPERVISOR_mmuext_op(&op, 1, NULL, DOMID_SELF) < 0)
		panic("xpq_queue_pin_table");
}

void
xpq_queue_unpin_table(paddr_t pa)
{
	struct mmuext_op op;
	xpq_flush_queue();

	XENPRINTK2(("xpq_queue_unpin_table: %p %p\n", (void *)pa, (void *)pa));
	op.arg1.mfn = pa >> PAGE_SHIFT;
	op.cmd = MMUEXT_UNPIN_TABLE;
	if (HYPERVISOR_mmuext_op(&op, 1, NULL, DOMID_SELF) < 0)
		panic("xpq_queue_unpin_table");
}

void
xpq_queue_set_ldt(vaddr_t va, uint32_t entries)
{
	struct mmuext_op op;
	xpq_flush_queue();

	XENPRINTK2(("xpq_queue_set_ldt\n"));
	KASSERT(va == (va & PG_FRAME));
	op.cmd = MMUEXT_SET_LDT;
	op.arg1.linear_addr = va;
	op.arg2.nr_ents = entries;
	if (HYPERVISOR_mmuext_op(&op, 1, NULL, DOMID_SELF) < 0)
		panic("xpq_queue_set_ldt");
}

void
xpq_queue_tlb_flush()
{
	struct mmuext_op op;
	xpq_flush_queue();

	XENPRINTK2(("xpq_queue_tlb_flush\n"));
	op.cmd = MMUEXT_TLB_FLUSH_LOCAL;
	if (HYPERVISOR_mmuext_op(&op, 1, NULL, DOMID_SELF) < 0)
		panic("xpq_queue_tlb_flush");
}

void
xpq_flush_cache()
{
	struct mmuext_op op;
	int s = splvm();
	xpq_flush_queue();

	XENPRINTK2(("xpq_queue_flush_cache\n"));
	op.cmd = MMUEXT_FLUSH_CACHE;
	if (HYPERVISOR_mmuext_op(&op, 1, NULL, DOMID_SELF) < 0)
		panic("xpq_flush_cache");
	splx(s);
}

void
xpq_queue_invlpg(vaddr_t va)
{
	struct mmuext_op op;
	xpq_flush_queue();

	XENPRINTK2(("xpq_queue_invlpg %p\n", (void *)va));
	op.cmd = MMUEXT_INVLPG_LOCAL;
	op.arg1.linear_addr = (va & PG_FRAME);
	if (HYPERVISOR_mmuext_op(&op, 1, NULL, DOMID_SELF) < 0)
		panic("xpq_queue_invlpg");
}

int
xpq_update_foreign(pt_entry_t *ptr, pt_entry_t val, int dom)
{
	mmu_update_t op;
	int ok;
	xpq_flush_queue();

	op.ptr = (uint32_t)ptr;
	op.val = val;
	if (HYPERVISOR_mmu_update(&op, 1, &ok, dom) < 0)
		return EFAULT;
	return (0);
}
#else /* XEN3 */
void
xpq_queue_pt_switch(paddr_t pa)
{

	XENPRINTK2(("xpq_queue_pt_switch: %p %p\n", (void *)pa, (void *)pa));
	xpq_queue[xpq_idx].ptr = pa | MMU_EXTENDED_COMMAND;
	xpq_queue[xpq_idx].val = MMUEXT_NEW_BASEPTR;
	xpq_increment_idx();
}

void
xpq_queue_pin_table(paddr_t pa, int type)
{

	XENPRINTK2(("xpq_queue_pin_table: %p %p\n", (void *)pa, (void *)pa));
	xpq_queue[xpq_idx].ptr = pa | MMU_EXTENDED_COMMAND;
	switch (type) {
	case XPQ_PIN_L1_TABLE:
		xpq_queue[xpq_idx].val = MMUEXT_PIN_L1_TABLE;
		break;
	case XPQ_PIN_L2_TABLE:
		xpq_queue[xpq_idx].val = MMUEXT_PIN_L2_TABLE;
		break;
	}
	xpq_increment_idx();
}

void
xpq_queue_unpin_table(paddr_t pa)
{

	XENPRINTK2(("xpq_queue_unpin_table: %p %p\n", (void *)pa, (void *)pa));
	xpq_queue[xpq_idx].ptr = pa | MMU_EXTENDED_COMMAND;
	xpq_queue[xpq_idx].val = MMUEXT_UNPIN_TABLE;
	xpq_increment_idx();
}

void
xpq_queue_set_ldt(vaddr_t va, uint32_t entries)
{

	XENPRINTK2(("xpq_queue_set_ldt\n"));
	KASSERT(va == (va & PG_FRAME));
	xpq_queue[xpq_idx].ptr = MMU_EXTENDED_COMMAND | va;
	xpq_queue[xpq_idx].val = MMUEXT_SET_LDT | (entries << MMUEXT_CMD_SHIFT);
	xpq_increment_idx();
}

void
xpq_queue_tlb_flush()
{

	XENPRINTK2(("xpq_queue_tlb_flush\n"));
	xpq_queue[xpq_idx].ptr = MMU_EXTENDED_COMMAND;
	xpq_queue[xpq_idx].val = MMUEXT_TLB_FLUSH;
	xpq_increment_idx();
}

void
xpq_flush_cache()
{
	int s = splvm();

	XENPRINTK2(("xpq_queue_flush_cache\n"));
	xpq_queue[xpq_idx].ptr = MMU_EXTENDED_COMMAND;
	xpq_queue[xpq_idx].val = MMUEXT_FLUSH_CACHE;
	xpq_increment_idx();
	xpq_flush_queue();
	splx(s);
}

void
xpq_queue_invlpg(vaddr_t va)
{

	XENPRINTK2(("xpq_queue_invlpg %p\n", (void *)va));
	xpq_queue[xpq_idx].ptr = (va & PG_FRAME) | MMU_EXTENDED_COMMAND;
	xpq_queue[xpq_idx].val = MMUEXT_INVLPG;
	xpq_increment_idx();
}

int
xpq_update_foreign(pt_entry_t *ptr, pt_entry_t val, int dom)
{
	mmu_update_t xpq_up[3];

	xpq_up[0].ptr = MMU_EXTENDED_COMMAND;
	xpq_up[0].val = MMUEXT_SET_FOREIGNDOM | (dom << 16);
	xpq_up[1].ptr = (paddr_t)ptr;
	xpq_up[1].val = val;
	if (HYPERVISOR_mmu_update_self(xpq_up, 2, NULL) < 0)
		return EFAULT;
	return (0);
}
#endif /* XEN3 */

#ifdef XENDEBUG
void
xpq_debug_dump()
{
	int i;

	XENPRINTK2(("idx: %d\n", xpq_idx));
	for (i = 0; i < xpq_idx; i++) {
		sprintf(XBUF, "%x %08x ", (u_int)xpq_queue[i].ptr,
		    (u_int)xpq_queue[i].val);
		if (++i < xpq_idx)
			sprintf(XBUF + strlen(XBUF), "%x %08x ",
			    (u_int)xpq_queue[i].ptr, (u_int)xpq_queue[i].val);
		if (++i < xpq_idx)
			sprintf(XBUF + strlen(XBUF), "%x %08x ",
			    (u_int)xpq_queue[i].ptr, (u_int)xpq_queue[i].val);
		if (++i < xpq_idx)
			sprintf(XBUF + strlen(XBUF), "%x %08x ",
			    (u_int)xpq_queue[i].ptr, (u_int)xpq_queue[i].val);
		XENPRINTK2(("%d: %s\n", xpq_idx, XBUF));
	}
}
#endif
