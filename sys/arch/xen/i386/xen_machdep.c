/*	$NetBSD: xen_machdep.c,v 1.5 2004/05/02 20:38:35 cl Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: xen_machdep.c,v 1.5 2004/05/02 20:38:35 cl Exp $");

#include "opt_xen.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mount.h>

#include <uvm/uvm.h>

#include <machine/gdt.h>
#include <machine/xenfunc.h>
#include <machine/xenpmap.h>

/* #define	XENDEBUG */
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
void printk(char *, ...);
#define	PRINTF(x) printf x
#define	PRINTK(x) printk x

shared_info_t *HYPERVISOR_shared_info;
union start_info_union start_info_union;


void xen_failsafe_handler(void);

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
	pt_entry_t *ptp;

	for (va = base; va < base + entries * sizeof(union descriptor);
	     va += PAGE_SIZE) {
		KASSERT(va >= VM_MIN_KERNEL_ADDRESS);
		ptp = kvtopte(va);
		XENPRINTF(("xen_set_ldt %p %d %p\n", (void *)base,
			      entries, ptp));
		PTE_CLEARBITS(ptp, PG_RW);
	}
	PTE_UPDATES_FLUSH();

	xpq_queue_set_ldt(base, entries);
	xpq_flush_queue();
}

void
lgdt(struct region_descriptor *rdp)
{

	panic("lgdt %p %08x\n", (void *)rdp->rd_base, rdp->rd_limit);
}

void
xen_parse_cmdline(int what, union xen_cmdline_parseinfo *xcp)
{
	char *cmd_line, *opt, *s;
	int b, i, ipidx = 0;
	uint32_t xi_ip[5];

	cmd_line = xen_start_info.cmd_line;

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

		}

		if (cmd_line)
			*cmd_line++ = ' ';
	}
}





#define XEN_PAGE_OFFSET 0xC0100000

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
		bufpos = 0;
	}
}
#endif

void
xpmap_init(void)
{
	pd_entry_t *xen_pdp;
	pd_entry_t pde;
	pt_entry_t *ptp, *sysptp;
	pt_entry_t pte;
	uint32_t i, j;
	int bufpos;
#if defined(XENDEBUG_LOW)
	extern char kernel_text, _etext, __bss_start, end, *esym;
#endif

	XENPRINTK(("text %p data %p bss %p end %p esym %p\n", &kernel_text,
		   &_etext, &__bss_start, &end, esym));
	XENPRINTK(("xpmap_init PTD %p nkpde %d upages %d xen_PTD %p p2m-map %p\n",
		      (void *)PTDpaddr, nkpde, UPAGES,
		      (pd_entry_t *)xen_start_info.pt_base,
		      xpmap_phys_to_machine_mapping));

	xen_pdp = (pd_entry_t *)xen_start_info.pt_base;

	for (i = 0; i < xen_start_info.nr_pages; i++) {
		pde = xpmap_get_vbootpde((i << PAGE_SHIFT) + XEN_PAGE_OFFSET) &
			PG_FRAME;
		pte = ((pt_entry_t *)pde)[(i + (XEN_PAGE_OFFSET >> PAGE_SHIFT)) &
		    (PT_MASK >> PAGE_SHIFT)];
		xpmap_phys_to_machine_mapping[i] = pte >> PAGE_SHIFT;
	}

	bufpos = 0;

	XENPRINTK(("shared_inf %08x\n", (paddr_t)xen_start_info.shared_info));
	XENPRINTK(("c0100000: %08x\n",
	    xpmap_get_bootpte(0xc0100000)));

	/* Map kernel. */

	/* Map kernel data/bss/tables. */

	/* Map ISA I/O memory. */
	
	/* Map kernel PDEs. */

	/* Install a PDE recursively mapping page directory as a page table! */

	sysptp = (pt_entry_t *)(PTDpaddr + ((1 + UPAGES) << PAGE_SHIFT));
	for (i = 0; i < 1 + UPAGES + nkpde; i++) {
		/* mark PTE page read-only in xen's table */
		ptp = xpmap_get_bootptep(PTDpaddr + (i << PAGE_SHIFT));
		xpq_queue_pte_update(ptp, *ptp & ~PG_RW);

		/* mark PTE page read-only in our table */
		sysptp[((PTDpaddr + (i << PAGE_SHIFT) - KERNBASE_LOCORE) & 
			   (PD_MASK | PT_MASK)) >> PAGE_SHIFT] &= ~PG_RW;

		/* update our pte's */
		ptp = (pt_entry_t *)(PTDpaddr + (i << PAGE_SHIFT));
#if 0
		pte = xpmap_ptom((uint32_t)ptp - KERNBASE);
		XENPRINTK(("%03x: %p(%p) %08x\n", i, ptp, pte, i << PDSHIFT));
#endif
		for (j = 0; j < PTES_PER_PTP; j++) {
			if ((ptp[j] & PG_V) == 0)
				continue;
			if (ptp[j] == 0xffffffff)
				ptp[j] = xen_start_info.shared_info |
					(PG_V|PG_RW);
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
	xpq_flush_queue();

#if 0
	for (i = 0x300; i < 0x305; i++)
		xpmap_dump_pt((pt_entry_t *)
		    (xpmap_mtop(((pt_entry_t *)xen_start_info.pt_base)[i] &
			PG_FRAME) + KERNBASE), i);
	xpmap_dump_pt((pt_entry_t *)xen_start_info.pt_base, 0);
#endif

	XENPRINTK(("switching pdp: %p, %08lx, %p, %p, %p\n", (void *)PTDpaddr,
		      PTDpaddr - KERNBASE,
		      (void *)xpmap_ptom(PTDpaddr - KERNBASE),
		      (void *)xpmap_get_bootpte(PTDpaddr),
		      (void *)xpmap_mtop(xpmap_ptom(PTDpaddr - KERNBASE))));

#if defined(XENDEBUG)
	xpmap_dump_pt((pt_entry_t *)PTDpaddr, 0);
#endif

	xpq_queue_pin_table(xpmap_get_bootpte(PTDpaddr) & PG_FRAME,
	    XPQ_PIN_L2_TABLE);
	xpq_queue_pt_switch(xpmap_get_bootpte(PTDpaddr) & PG_FRAME);
	xpq_queue_unpin_table(
		xpmap_get_bootpte(xen_start_info.pt_base) & PG_FRAME);

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
	int start, end;
	pt_entry_t old;

	start = xen_start_info.nr_pages;
	end = HYPERVISOR_VIRT_START >> PAGE_SHIFT;

	r.ptr = (unsigned long)&PTE_BASE[x86_btop(va)];
	old = PTE_BASE[x86_btop(va)];

	while (start + 1 < end) {
		r.val = (((start + end) / 2) << PAGE_SHIFT) | PG_V;

		if (HYPERVISOR_mmu_update_fail_ok(&r, 1) < 0)
			end = (start + end) / 2;
		else
			start = (start + end) / 2;
	}
	r.val = old;
	if (HYPERVISOR_mmu_update_fail_ok(&r, 1) < 0)
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
typedef union xpq_queue {
	struct {
		pd_entry_t *ptr;
		pd_entry_t val;
	} pde;
	struct {
		pt_entry_t *ptr;
		pt_entry_t val;
	} pte;
	struct {
		paddr_t ptr;
		uint32_t val;
	} pa;
} xpq_queue_t;
static xpq_queue_t xpq_queue[XPQUEUE_SIZE];
static int xpq_idx = 0;

void
xpq_flush_queue()
{
	int i;

	XENPRINTK2(("flush queue %p entries %d\n", xpq_queue, xpq_idx));
	for (i = 0; i < xpq_idx; i++)
		XENPRINTK2(("%d: %p %08x\n", i, xpq_queue[i].pde.ptr,
		    xpq_queue[i].pde.val));
	HYPERVISOR_mmu_update((mmu_update_t *)xpq_queue, xpq_idx);
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
xpq_queue_invlpg(vaddr_t va)
{

	XENPRINTK2(("xpq_queue_invlpg %p\n", (void *)va));
	xpq_queue[xpq_idx].pa.ptr = MMU_EXTENDED_COMMAND;
	xpq_queue[xpq_idx].pa.val = (va & ~PAGE_MASK) | MMUEXT_INVLPG;
	xpq_increment_idx();
}

void
xpq_queue_pde_update(pd_entry_t *ptr, pd_entry_t val)
{

	xpq_queue[xpq_idx].pde.ptr = ptr;
	xpq_queue[xpq_idx].pde.val = val;
	xpq_increment_idx();
}

void
xpq_queue_pte_update(pt_entry_t *ptr, pt_entry_t val)
{

	xpq_queue[xpq_idx].pte.ptr = ptr;
	xpq_queue[xpq_idx].pte.val = val;
	xpq_increment_idx();
}

void
xpq_queue_unchecked_pte_update(pt_entry_t *ptr, pt_entry_t val)
{

	xpq_queue[xpq_idx].pa.ptr = (paddr_t)ptr | MMU_UNCHECKED_PT_UPDATE;
	xpq_queue[xpq_idx].pa.val = val;
	xpq_increment_idx();
}

void
xpq_queue_pt_switch(paddr_t pa)
{

	XENPRINTK2(("xpq_queue_pt_switch: %p %p\n", (void *)pa, (void *)pa));
	xpq_queue[xpq_idx].pa.ptr = pa | MMU_EXTENDED_COMMAND;
	xpq_queue[xpq_idx].pa.val = MMUEXT_NEW_BASEPTR;
	xpq_increment_idx();
}

void
xpq_queue_pin_table(paddr_t pa, int type)
{

	XENPRINTK2(("xpq_queue_pin_table: %p %p\n", (void *)pa, (void *)pa));
	xpq_queue[xpq_idx].pa.ptr = pa | MMU_EXTENDED_COMMAND;
	switch (type) {
	case XPQ_PIN_L1_TABLE:
		xpq_queue[xpq_idx].pa.val = MMUEXT_PIN_L1_TABLE;
		break;
	case XPQ_PIN_L2_TABLE:
		xpq_queue[xpq_idx].pa.val = MMUEXT_PIN_L2_TABLE;
		break;
	}
	xpq_increment_idx();
}

void
xpq_queue_unpin_table(paddr_t pa)
{

	XENPRINTK2(("xpq_queue_unpin_table: %p %p\n", (void *)pa, (void *)pa));
	xpq_queue[xpq_idx].pa.ptr = pa | MMU_EXTENDED_COMMAND;
	xpq_queue[xpq_idx].pa.val = MMUEXT_UNPIN_TABLE;
	xpq_increment_idx();
}

void
xpq_queue_set_ldt(vaddr_t va, uint32_t entries)
{

	XENPRINTK2(("xpq_queue_set_ldt\n"));
	KASSERT(va == (va & PG_FRAME));
	xpq_queue[xpq_idx].pa.ptr = MMU_EXTENDED_COMMAND | va;
	xpq_queue[xpq_idx].pa.val = MMUEXT_SET_LDT |
		(entries << MMUEXT_CMD_SHIFT);
	xpq_increment_idx();
}

void
xpq_queue_tlb_flush()
{

	XENPRINTK2(("xpq_queue_tlb_flush\n"));
	xpq_queue[xpq_idx].pa.ptr = MMU_EXTENDED_COMMAND;
	xpq_queue[xpq_idx].pa.val = MMUEXT_TLB_FLUSH;
	xpq_increment_idx();
}

#ifdef XENDEBUG
void
xpq_debug_dump()
{
	int i;

	XENPRINTK2(("idx: %d\n", xpq_idx));
	for (i = 0; i < xpq_idx; i++) {
		sprintf(XBUF, "%p %08x ", xpq_queue[i].pte.ptr,
		    xpq_queue[i].pte.val);
		if (++i < xpq_idx)
			sprintf(XBUF + strlen(XBUF), "%p %08x ",
			    xpq_queue[i].pte.ptr, xpq_queue[i].pte.val);
		if (++i < xpq_idx)
			sprintf(XBUF + strlen(XBUF), "%p %08x ",
			    xpq_queue[i].pte.ptr, xpq_queue[i].pte.val);
		if (++i < xpq_idx)
			sprintf(XBUF + strlen(XBUF), "%p %08x ",
			    xpq_queue[i].pte.ptr, xpq_queue[i].pte.val);
		XENPRINTK2(("%d: %s\n", xpq_idx, XBUF));
	}
}
#endif
