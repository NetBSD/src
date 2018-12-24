/*	$NetBSD: xen_intr.c,v 1.10 2018/12/24 14:55:42 cherry Exp $	*/

/*-
 * Copyright (c) 1998, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum, and by Jason R. Thorpe.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: xen_intr.c,v 1.10 2018/12/24 14:55:42 cherry Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/kmem.h>

#include <xen/evtchn.h>

#include <machine/cpu.h>
#include <machine/intr.h>

/*
 * Add a mask to cpl, and return the old value of cpl.
 */
int
splraise(int nlevel)
{
	int olevel;
	struct cpu_info *ci = curcpu();

	olevel = ci->ci_ilevel;
	if (nlevel > olevel)
		ci->ci_ilevel = nlevel;
	__insn_barrier();
	return (olevel);
}

/*
 * Restore a value to cpl (unmasking interrupts).  If any unmasked
 * interrupts are pending, call Xspllower() to process them.
 */
void
spllower(int nlevel)
{
	struct cpu_info *ci = curcpu();
	uint32_t imask;
	u_long psl;

	if (ci->ci_ilevel <= nlevel)
		return;

	__insn_barrier();

	imask = IUNMASK(ci, nlevel);
	psl = x86_read_psl();
	x86_disable_intr();
	if (ci->ci_ipending & imask) {
		KASSERT(psl == 0);
		Xspllower(nlevel);
		/* Xspllower does enable_intr() */
	} else {
		ci->ci_ilevel = nlevel;
		x86_write_psl(psl);
	}
}

void
x86_disable_intr(void)
{
	__cli();
}

void
x86_enable_intr(void)
{
	__sti();
}

u_long
x86_read_psl(void)
{

	return (curcpu()->ci_vcpu->evtchn_upcall_mask);
}

void
x86_write_psl(u_long psl)
{
	struct cpu_info *ci = curcpu();

	ci->ci_vcpu->evtchn_upcall_mask = psl;
	xen_rmb();
	if (ci->ci_vcpu->evtchn_upcall_pending && psl == 0) {
	    	hypervisor_force_callback();
	}
}

void *
xen_intr_establish(int legacy_irq, struct pic *pic, int pin,
    int type, int level, int (*handler)(void *), void *arg,
    bool known_mpsafe)
{

	return xen_intr_establish_xname(legacy_irq, pic, pin, type, level,
	    handler, arg, known_mpsafe, "XEN");
}

void *
xen_intr_establish_xname(int legacy_irq, struct pic *pic, int pin,
    int type, int level, int (*handler)(void *), void *arg,
    bool known_mpsafe, const char *xname)
{
	const char *intrstr;
	char intrstr_buf[INTRIDBUF];

	if (pic->pic_type == PIC_XEN) {
		struct intrhand *rih;

		/*
		 * event_set_handler interprets `level != IPL_VM' to
		 * mean MP-safe, so we require the caller to match that
		 * for the moment.
		 */
		KASSERT(known_mpsafe == (level != IPL_VM));

		intrstr = intr_create_intrid(legacy_irq, pic, pin, intrstr_buf,
		    sizeof(intrstr_buf));

		event_set_handler(pin, handler, arg, level, intrstr, xname);

		rih = kmem_zalloc(sizeof(*rih), cold ? KM_NOSLEEP : KM_SLEEP);
		if (rih == NULL) {
			printf("%s: can't allocate handler info\n", __func__);
			return NULL;
		}

		/*
		 * XXX:
		 * This is just a copy for API conformance.
		 * The real ih is lost in the innards of
		 * event_set_handler(); where the details of
		 * biglock_wrapper etc are taken care of.
		 * All that goes away when we nuke event_set_handler()
		 * et. al. and unify with x86/intr.c
		 */
		rih->ih_pin = pin; /* port */
		rih->ih_fun = rih->ih_realfun = handler;
		rih->ih_arg = rih->ih_realarg = arg;
		rih->pic_type = pic->pic_type;
		return rih;
	} 	/* Else we assume pintr */

#if NPCI > 0 || NISA > 0
	struct pintrhand *pih;
	int gsi;
	int vector, evtchn;

	KASSERTMSG(legacy_irq == -1 || (0 <= legacy_irq && legacy_irq < NUM_XEN_IRQS),
	    "bad legacy IRQ value: %d", legacy_irq);
	KASSERTMSG(!(legacy_irq == -1 && pic == &i8259_pic),
	    "non-legacy IRQon i8259 ");

	gsi = xen_pic_to_gsi(pic, pin);

	intrstr = intr_create_intrid(gsi, pic, pin, intrstr_buf,
	    sizeof(intrstr_buf));

	vector = xen_vec_alloc(gsi);

	if (irq2port[gsi] == 0) {
		extern struct cpu_info phycpu_info_primary; /* XXX */
		struct cpu_info *ci = &phycpu_info_primary;

		pic->pic_addroute(pic, ci, pin, vector, type);

		evtchn = bind_pirq_to_evtch(gsi);
		KASSERT(evtchn > 0);
		KASSERT(evtchn < NR_EVENT_CHANNELS);
		irq2port[gsi] = evtchn + 1;
		xen_atomic_set_bit(&ci->ci_evtmask[0], evtchn);
	} else {
		/*
		 * Shared interrupt - we can't rebind.
		 * The port is shared instead.
		 */
		evtchn = irq2port[gsi] - 1;
	}

	pih = pirq_establish(gsi, evtchn, handler, arg, level,
			     intrstr, xname);
	pih->pic_type = pic->pic_type;
	return pih;
#endif /* NPCI > 0 || NISA > 0 */

	/* FALLTHROUGH */
	return NULL;
}

/*
 * Deregister an interrupt handler.
 */
void
xen_intr_disestablish(struct intrhand *ih)
{

	if (ih->pic_type == PIC_XEN) {
		event_remove_handler(ih->ih_pin, ih->ih_realfun,
		    ih->ih_realarg);
		kmem_free(ih, sizeof(*ih));
		return;
	}
#if defined(DOM0OPS)
	/* 
	 * Cache state, to prevent a use after free situation with
	 * ih.
	 */

	struct pintrhand *pih = (struct pintrhand *)ih;

	int pirq = pih->pirq;
	int port = pih->evtch;
	KASSERT(irq2port[pirq] != 0);

	pirq_disestablish(pih);

	if (evtsource[port] == NULL) {
			/*
			 * Last handler was removed by
			 * event_remove_handler().
			 *
			 * We can safely unbind the pirq now.
			 */

			port = unbind_pirq_from_evtch(pirq);
			KASSERT(port == pih->evtch);
			irq2port[pirq] = 0;
	}
#endif
	return;
}

__weak_alias(intr_establish, xen_intr_establish);
__weak_alias(intr_establish_xname, xen_intr_establish_xname);
__weak_alias(intr_disestablish, xen_intr_disestablish);
