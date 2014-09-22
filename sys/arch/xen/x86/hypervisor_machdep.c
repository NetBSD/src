/*	$NetBSD: hypervisor_machdep.c,v 1.27.12.1 2014/09/22 11:15:57 martin Exp $	*/

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

/******************************************************************************
 * hypervisor.c
 * 
 * Communication to/from hypervisor.
 * 
 * Copyright (c) 2002-2004, K A Fraser
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 */


#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: hypervisor_machdep.c,v 1.27.12.1 2014/09/22 11:15:57 martin Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kmem.h>

#include <uvm/uvm_extern.h>

#include <machine/vmparam.h>
#include <machine/pmap.h>

#include <xen/xen.h>
#include <xen/hypervisor.h>
#include <xen/evtchn.h>
#include <xen/xenpmap.h>

#include "opt_xen.h"

/*
 * arch-dependent p2m frame lists list (L3 and L2)
 * used by Xen for save/restore mappings
 */
static unsigned long * l3_p2m_page;
static unsigned long * l2_p2m_page;
static int l2_p2m_page_size; /* size of L2 page, in pages */

static void build_p2m_frame_list_list(void);
static void update_p2m_frame_list_list(void);

// #define PORT_DEBUG 4
// #define EARLY_DEBUG_EVENT

/* callback function type */
typedef void (*iterate_func_t)(unsigned int, unsigned int,
			       unsigned int, void *);

static inline void
evt_iterate_bits(volatile unsigned long *pendingl1,
		 volatile unsigned long *pendingl2, 
		 volatile unsigned long *mask,
		 iterate_func_t iterate_pending, void *iterate_args)
{

	KASSERT(pendingl1 != NULL);
	KASSERT(pendingl2 != NULL);
	
	unsigned long l1, l2;
	unsigned int l1i, l2i, port;

	l1 = xen_atomic_xchg(pendingl1, 0);
	while ((l1i = xen_ffs(l1)) != 0) {
		l1i--;
		l1 &= ~(1UL << l1i);

		l2 = pendingl2[l1i] & (mask != NULL ? ~mask[l1i] : -1UL);
		l2 &= curcpu()->ci_evtmask[l1i];

		if (mask != NULL) xen_atomic_setbits_l(&mask[l1i], l2);
		xen_atomic_clearbits_l(&pendingl2[l1i], l2);

		while ((l2i = xen_ffs(l2)) != 0) {
			l2i--;
			l2 &= ~(1UL << l2i);

			port = (l1i << LONG_SHIFT) + l2i;

			iterate_pending(port, l1i, l2i, iterate_args);
		}
	}
}

/*
 * Set per-cpu "pending" information for outstanding events that
 * cannot be processed now.
 */
   
static inline void
evt_set_pending(unsigned int port, unsigned int l1i,
		unsigned int l2i, void *args)
{

	KASSERT(args != NULL);

	int *ret = args;

	if (evtsource[port]) {
		hypervisor_set_ipending(evtsource[port]->ev_imask, l1i, l2i);
		evtsource[port]->ev_evcnt.ev_count++;
		if (*ret == 0 && curcpu()->ci_ilevel <
		    evtsource[port]->ev_maxlevel)
			*ret = 1;
	}
#ifdef DOM0OPS
	else  {
		/* set pending event */
		xenevt_setipending(l1i, l2i);
	}
#endif
}

int stipending(void);
int
stipending(void)
{
	volatile shared_info_t *s = HYPERVISOR_shared_info;
	struct cpu_info *ci;
	volatile struct vcpu_info *vci;
	int ret;

	ret = 0;
	ci = curcpu();
	vci = ci->ci_vcpu;

#if 0
	if (HYPERVISOR_shared_info->events)
		printf("stipending events %08lx mask %08lx ilevel %d\n",
		    HYPERVISOR_shared_info->events,
		    HYPERVISOR_shared_info->events_mask, ci->ci_ilevel);
#endif

#ifdef EARLY_DEBUG_EVENT
	if (xen_atomic_test_bit(&s->evtchn_pending[0], debug_port)) {
		xen_debug_handler(NULL);
		xen_atomic_clear_bit(&s->evtchn_pending[0], debug_port);
	}
#endif

	/*
	 * we're only called after STIC, so we know that we'll have to
	 * STI at the end
	 */

	while (vci->evtchn_upcall_pending) {
		cli();

		vci->evtchn_upcall_pending = 0;

		evt_iterate_bits(&vci->evtchn_pending_sel,
		    s->evtchn_pending, s->evtchn_mask,
		    evt_set_pending, &ret);

		sti();
	}

#if 0
	if (ci->ci_ipending & 0x1)
		printf("stipending events %08lx mask %08lx ilevel %d ipending %08x\n",
		    HYPERVISOR_shared_info->events,
		    HYPERVISOR_shared_info->events_mask, ci->ci_ilevel,
		    ci->ci_ipending);
#endif

	return (ret);
}

/* Iterate through pending events and call the event handler */

static inline void
evt_do_hypervisor_callback(unsigned int port, unsigned int l1i,
			   unsigned int l2i, void *args)
{
	KASSERT(args != NULL);

	struct cpu_info *ci = curcpu();
	struct intrframe *regs = args;

#ifdef PORT_DEBUG
	if (port == PORT_DEBUG)
		printf("do_hypervisor_callback event %d\n", port);
#endif
	if (evtsource[port]) {
		ci->ci_idepth++;
		evtchn_do_event(port, regs);
		ci->ci_idepth--;
	}
#ifdef DOM0OPS
	else  {
		if (ci->ci_ilevel < IPL_HIGH) {
			/* fast path */
			int oipl = ci->ci_ilevel;
			ci->ci_ilevel = IPL_HIGH;
			ci->ci_idepth++;			
			xenevt_event(port);
			ci->ci_idepth--;
			ci->ci_ilevel = oipl;
		} else {
			/* set pending event */
			xenevt_setipending(l1i, l2i);
		}
	}
#endif
}

void
do_hypervisor_callback(struct intrframe *regs)
{
	volatile shared_info_t *s = HYPERVISOR_shared_info;
	struct cpu_info *ci;
	volatile struct vcpu_info *vci;
	int level __diagused;

	ci = curcpu();
	vci = ci->ci_vcpu;
	level = ci->ci_ilevel;

	// DDD printf("do_hypervisor_callback\n");

#ifdef EARLY_DEBUG_EVENT
	if (xen_atomic_test_bit(&s->evtchn_pending[0], debug_port)) {
		xen_debug_handler(NULL);
		xen_atomic_clear_bit(&s->evtchn_pending[0], debug_port);
	}
#endif

	while (vci->evtchn_upcall_pending) {
		vci->evtchn_upcall_pending = 0;

		evt_iterate_bits(&vci->evtchn_pending_sel,
		    s->evtchn_pending, s->evtchn_mask,
		    evt_do_hypervisor_callback, regs);
	}

#ifdef DIAGNOSTIC
	if (level != ci->ci_ilevel)
		printf("hypervisor done %08x level %d/%d ipending %08x\n",
		    (uint)vci->evtchn_pending_sel,
		    level, ci->ci_ilevel, ci->ci_ipending);
#endif
}

void
hypervisor_send_event(struct cpu_info *ci, unsigned int ev)
{
	KASSERT(ci != NULL);

	volatile shared_info_t *s = HYPERVISOR_shared_info;
	volatile struct vcpu_info *vci = ci->ci_vcpu;

#ifdef PORT_DEBUG
	if (ev == PORT_DEBUG)
		printf("hypervisor_send_event %d\n", ev);
#endif

	xen_atomic_set_bit(&s->evtchn_pending[0], ev);

	if (__predict_false(ci == curcpu())) {
		xen_atomic_set_bit(&vci->evtchn_pending_sel,
		    ev >> LONG_SHIFT);
		xen_atomic_set_bit(&vci->evtchn_upcall_pending, 0);
	}

	xen_atomic_clear_bit(&s->evtchn_mask[0], ev);

	if (__predict_true(ci == curcpu())) {
		hypervisor_force_callback();
	} else {
		if (__predict_false(xen_send_ipi(ci, XEN_IPI_HVCB))) {
			panic("xen_send_ipi(cpu%d, XEN_IPI_HVCB) failed\n",
			    (int) ci->ci_cpuid);
		}
	}
}

void
hypervisor_unmask_event(unsigned int ev)
{
	volatile shared_info_t *s = HYPERVISOR_shared_info;
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;
	volatile struct vcpu_info *vci;

#ifdef PORT_DEBUG
	if (ev == PORT_DEBUG)
		printf("hypervisor_unmask_event %d\n", ev);
#endif

	xen_atomic_clear_bit(&s->evtchn_mask[0], ev);
	/*
	 * The following is basically the equivalent of
	 * 'hw_resend_irq'. Just like a real IO-APIC we 'lose the
	 * interrupt edge' if the channel is masked.
	 */
	if (!xen_atomic_test_bit(&s->evtchn_pending[0], ev))
		return;

	for (CPU_INFO_FOREACH(cii, ci)) {
		if (!xen_atomic_test_bit(&ci->ci_evtmask[0], ev))
			continue;
		vci = ci->ci_vcpu;
		if (__predict_true(ci == curcpu())) {
			if (!xen_atomic_test_and_set_bit(&vci->evtchn_pending_sel,
				ev>>LONG_SHIFT))
				xen_atomic_set_bit(&vci->evtchn_upcall_pending, 0);
		}
		if (!vci->evtchn_upcall_mask) {
			if (__predict_true(ci == curcpu())) {
				hypervisor_force_callback();
			} else {
				if (__predict_false(
				    xen_send_ipi(ci, XEN_IPI_HVCB))) {
					panic("xen_send_ipi(cpu%d, "
					    "XEN_IPI_HVCB) failed\n",
					    (int) ci->ci_cpuid);
				}
			}
		}
	}
}

void
hypervisor_mask_event(unsigned int ev)
{
	volatile shared_info_t *s = HYPERVISOR_shared_info;
#ifdef PORT_DEBUG
	if (ev == PORT_DEBUG)
		printf("hypervisor_mask_event %d\n", ev);
#endif

	xen_atomic_set_bit(&s->evtchn_mask[0], ev);
}

void
hypervisor_clear_event(unsigned int ev)
{
	volatile shared_info_t *s = HYPERVISOR_shared_info;
#ifdef PORT_DEBUG
	if (ev == PORT_DEBUG)
		printf("hypervisor_clear_event %d\n", ev);
#endif

	xen_atomic_clear_bit(&s->evtchn_pending[0], ev);
}

static inline void
evt_enable_event(unsigned int port, unsigned int l1i,
		 unsigned int l2i, void *args)
{
	KASSERT(args == NULL);
	hypervisor_enable_event(port);
}

void
hypervisor_enable_ipl(unsigned int ipl)
{
	struct cpu_info *ci = curcpu();

	/*
	 * enable all events for ipl. As we only set an event in ipl_evt_mask
	 * for its lowest IPL, and pending IPLs are processed high to low,
	 * we know that all callback for this event have been processed.
	 */

	evt_iterate_bits(&ci->ci_isources[ipl]->ipl_evt_mask1,
	    ci->ci_isources[ipl]->ipl_evt_mask2, NULL, 
	    evt_enable_event, NULL);

}

void
hypervisor_set_ipending(uint32_t iplmask, int l1, int l2)
{

	/* This function is not re-entrant */
	KASSERT(x86_read_psl() != 0);

	int ipl;
	struct cpu_info *ci = curcpu();

	/* set pending bit for the appropriate IPLs */	
	ci->ci_ipending |= iplmask;

	/*
	 * And set event pending bit for the lowest IPL. As IPL are handled
	 * from high to low, this ensure that all callbacks will have been
	 * called when we ack the event
	 */
	ipl = ffs(iplmask);
	KASSERT(ipl > 0);
	ipl--;
	KASSERT(ipl < NIPL);
	KASSERT(ci->ci_isources[ipl] != NULL);
	ci->ci_isources[ipl]->ipl_evt_mask1 |= 1UL << l1;
	ci->ci_isources[ipl]->ipl_evt_mask2[l1] |= 1UL << l2;
	if (__predict_false(ci != curcpu())) {
		if (xen_send_ipi(ci, XEN_IPI_HVCB)) {
			panic("hypervisor_set_ipending: "
			    "xen_send_ipi(cpu%d, XEN_IPI_HVCB) failed\n",
			    (int) ci->ci_cpuid);
		}
	}
}

void
hypervisor_machdep_attach(void)
{
 	/* dom0 does not require the arch-dependent P2M translation table */
	if (!xendomain_is_dom0()) {
		build_p2m_frame_list_list();
		sysctl_xen_suspend_setup();
	}
}

void
hypervisor_machdep_resume(void)
{
	/* dom0 does not require the arch-dependent P2M translation table */
	if (!xendomain_is_dom0())
		update_p2m_frame_list_list();
}

/*
 * Generate the p2m_frame_list_list table,
 * needed for guest save/restore
 */
static void
build_p2m_frame_list_list(void)
{
        int fpp; /* number of page (frame) pointer per page */
        unsigned long max_pfn;
        /*
         * The p2m list is composed of three levels of indirection,
         * each layer containing MFNs pointing to lower level pages
         * The indirection is used to convert a given PFN to its MFN
         * Each N level page can point to @fpp (N-1) level pages
         * For example, for x86 32bit, we have:
         * - PAGE_SIZE: 4096 bytes
         * - fpp: 1024 (one L3 page can address 1024 L2 pages)
         * A L1 page contains the list of MFN we are looking for
         */
        max_pfn = xen_start_info.nr_pages;
        fpp = PAGE_SIZE / sizeof(xen_pfn_t);

        /* we only need one L3 page */
        l3_p2m_page = (vaddr_t *)uvm_km_alloc(kernel_map, PAGE_SIZE,
	    PAGE_SIZE, UVM_KMF_WIRED | UVM_KMF_NOWAIT);
        if (l3_p2m_page == NULL)
                panic("could not allocate memory for l3_p2m_page");

        /*
         * Determine how many L2 pages we need for the mapping
         * Each L2 can map a total of @fpp L1 pages
         */
        l2_p2m_page_size = howmany(max_pfn, fpp);

        l2_p2m_page = (vaddr_t *)uvm_km_alloc(kernel_map,
	    l2_p2m_page_size * PAGE_SIZE,
	    PAGE_SIZE, UVM_KMF_WIRED | UVM_KMF_NOWAIT);
        if (l2_p2m_page == NULL)
                panic("could not allocate memory for l2_p2m_page");

        /* We now have L3 and L2 pages ready, update L1 mapping */
        update_p2m_frame_list_list();

}

/*
 * Update the L1 p2m_frame_list_list mapping (during guest boot or resume)
 */
static void
update_p2m_frame_list_list(void)
{
        int i;
        int fpp; /* number of page (frame) pointer per page */
        unsigned long max_pfn;

        max_pfn = xen_start_info.nr_pages;
        fpp = PAGE_SIZE / sizeof(xen_pfn_t);

        for (i = 0; i < l2_p2m_page_size; i++) {
                /*
                 * Each time we start a new L2 page,
                 * store its MFN in the L3 page
                 */
                if ((i % fpp) == 0) {
                        l3_p2m_page[i/fpp] = vtomfn(
                                (vaddr_t)&l2_p2m_page[i]);
                }
                /*
                 * we use a shortcut
                 * since @xpmap_phys_to_machine_mapping array
                 * already contains PFN to MFN mapping, we just
                 * set the l2_p2m_page MFN pointer to the MFN of the
                 * according frame of @xpmap_phys_to_machine_mapping
                 */
                l2_p2m_page[i] = vtomfn((vaddr_t)
                        &xpmap_phys_to_machine_mapping[i*fpp]);
        }

        HYPERVISOR_shared_info->arch.pfn_to_mfn_frame_list_list =
                                        vtomfn((vaddr_t)l3_p2m_page);
        HYPERVISOR_shared_info->arch.max_pfn = max_pfn;

}
