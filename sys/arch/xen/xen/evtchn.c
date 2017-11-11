/*	$NetBSD: evtchn.c,v 1.77 2017/11/11 08:23:50 riastradh Exp $	*/

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
 * Copyright (c) 2004, K A Fraser.
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
__KERNEL_RCSID(0, "$NetBSD: evtchn.c,v 1.77 2017/11/11 08:23:50 riastradh Exp $");

#include "opt_xen.h"
#include "isa.h"
#include "pci.h"

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/kmem.h>
#include <sys/reboot.h>
#include <sys/mutex.h>

#include <uvm/uvm.h>

#include <machine/intrdefs.h>

#include <xen/xen.h>
#include <xen/hypervisor.h>
#include <xen/evtchn.h>
#include <xen/xenfunc.h>

/*
 * This lock protects updates to the following mapping and reference-count
 * arrays. The lock does not need to be acquired to read the mapping tables.
 */
static kmutex_t evtchn_lock;

/* event handlers */
struct evtsource *evtsource[NR_EVENT_CHANNELS];

/* channel locks */
static kmutex_t evtlock[NR_EVENT_CHANNELS];

/* Reference counts for bindings to event channels XXX: redo for SMP */
static uint8_t evtch_bindcount[NR_EVENT_CHANNELS];

/* event-channel <-> VCPU mapping for IPIs. XXX: redo for SMP. */
static evtchn_port_t vcpu_ipi_to_evtch[XEN_LEGACY_MAX_VCPUS];

/* event-channel <-> VCPU mapping for VIRQ_TIMER.  XXX: redo for SMP. */
static int virq_timer_to_evtch[XEN_LEGACY_MAX_VCPUS];

/* event-channel <-> VIRQ mapping. */
static int virq_to_evtch[NR_VIRQS];


#if NPCI > 0 || NISA > 0
/* event-channel <-> PIRQ mapping */
static int pirq_to_evtch[NR_PIRQS];
/* PIRQ needing notify */
static uint32_t pirq_needs_unmask_notify[NR_EVENT_CHANNELS / 32];
int pirq_interrupt(void *);
physdev_op_t physdev_op_notify = {
	.cmd = PHYSDEVOP_IRQ_UNMASK_NOTIFY,
};
#endif

static void xen_evtchn_mask(struct pic *, int);
static void xen_evtchn_unmask(struct pic *, int);
static void xen_evtchn_addroute(struct pic *, struct cpu_info *, int, int, int);
static void xen_evtchn_delroute(struct pic *, struct cpu_info *, int, int, int);
static bool xen_evtchn_trymask(struct pic *, int);


struct pic xen_pic = {
	.pic_name = "xenev0",
	.pic_type = PIC_XEN,
	.pic_vecbase = 0,
	.pic_apicid = 0,
	.pic_lock = __SIMPLELOCK_UNLOCKED,
	.pic_hwmask = xen_evtchn_mask,
	.pic_hwunmask = xen_evtchn_unmask,
	.pic_addroute = xen_evtchn_addroute,
	.pic_delroute = xen_evtchn_delroute,
	.pic_trymask = xen_evtchn_trymask,
	.pic_level_stubs = xenev_stubs,
	.pic_edge_stubs = xenev_stubs,
};
	
/*
 * We try to stick to the traditional x86 PIC semantics wrt Xen
 * events.
 *
 * PIC pins exist in a global namespace which may be hierarchical, and
 * are mapped to a cpu bus concept called 'IRQ' numbers, which are
 * also global, but linear. Thus a PIC, pin tuple will always map to
 * an IRQ number. These tuples can alias to the same IRQ number, thus
 * causing IRQ "sharing". IRQ numbers can be bound to specific CPUs,
 * and to specific callback vector indices on the CPU called idt_vec,
 * which are aliases to handlers meant to run on destination
 * CPUs. This binding can also happen at interrupt time and resolved
 * 'round-robin' between all CPUs, depending on the lapic setup. In
 * this case, all CPUs need to have identical idt_vec->handler
 * mappings.
 *
 * The job of pic_addroute() is to setup the 'wiring' between the
 * source pin, and the destination CPU handler, ideally on a specific
 * CPU in MP systems (or 'round-robin').
 *
 * On Xen, a global namespace of 'events' exist, which are initially
 * bound to nothing. This is similar to the relationship between
 * realworld realworld IRQ numbers wrt PIC pins, since before routing,
 * IRQ numbers by themselves have no causal connection setup with the
 * real world. (Except for the hardwired cases on the PC Architecture,
 * which we ignore for the purpose of this description). However the
 * really important routing is from pin to idt_vec. On PIC_XEN, all
 * three (pic, irq, idt_vec) belong to the same namespace and are
 * identical. Further, the mapping between idt_vec and the actual
 * callback handler is setup via calls to the evtchn.h api - this
 * last bit is analogous to x86/idt.c:idt_vec_set() on real h/w
 *
 * For now we handle two cases:
 * - IPC style events - eg: timer, PV devices, etc.
 * - dom0 physical irq bound events.
 *
 * In the case of IPC style events, we currently externalise the
 * event binding by using evtchn.h functions. From the POV of
 * PIC_XEN ,  'pin' , 'irq' and 'idt_vec' are all identical to the
 * port number of the event.
 *
 * In the case of dom0 physical irq bound events, we currently
 * event binding by exporting evtchn.h functions. From the POV of
 * PIC_LAPIC/PIC_IOAPIC, the 'pin' is the hardware pin, the 'irq' is
 * the x86 global irq number  - the port number is extracted out of a
 * global array (this is currently kludgy and breaks API abstraction)
 * and the binding happens during pic_addroute() of the ioapic.
 *
 * Later when we integrate more tightly with x86/intr.c, we will be
 * able to conform better to (PIC_LAPIC/PIC_IOAPIC)->PIC_XEN
 * cascading model.
 */

int debug_port = -1;

// #define IRQ_DEBUG 4

/* http://mail-index.netbsd.org/port-amd64/2004/02/22/0000.html */
#ifdef MULTIPROCESSOR

/*
 * intr_biglock_wrapper: grab biglock and call a real interrupt handler.
 */

int
xen_intr_biglock_wrapper(void *vp)
{
	struct intrhand *ih = vp;
	int ret;

	KERNEL_LOCK(1, NULL);

	ret = (*ih->ih_realfun)(ih->ih_realarg);

	KERNEL_UNLOCK_ONE(NULL);

	return ret;
}
#endif /* MULTIPROCESSOR */

void
events_default_setup(void)
{
	int i;

	/* No VCPU -> event mappings. */
	for (i = 0; i < XEN_LEGACY_MAX_VCPUS; i++)
		vcpu_ipi_to_evtch[i] = -1;

	/* No VIRQ_TIMER -> event mappings. */
	for (i = 0; i < XEN_LEGACY_MAX_VCPUS; i++)
		virq_timer_to_evtch[i] = -1;

	/* No VIRQ -> event mappings. */
	for (i = 0; i < NR_VIRQS; i++)
		virq_to_evtch[i] = -1;

#if NPCI > 0 || NISA > 0
	/* No PIRQ -> event mappings. */
	for (i = 0; i < NR_PIRQS; i++)
		pirq_to_evtch[i] = -1;
	for (i = 0; i < NR_EVENT_CHANNELS / 32; i++)
		pirq_needs_unmask_notify[i] = 0;
#endif

	/* No event-channel are 'live' right now. */
	for (i = 0; i < NR_EVENT_CHANNELS; i++) {
		evtsource[i] = NULL;
		evtch_bindcount[i] = 0;
		hypervisor_mask_event(i);
	}

}

void
events_init(void)
{
	mutex_init(&evtchn_lock, MUTEX_DEFAULT, IPL_NONE);
	debug_port = bind_virq_to_evtch(VIRQ_DEBUG);

	KASSERT(debug_port != -1);

	aprint_verbose("VIRQ_DEBUG interrupt using event channel %d\n",
	    debug_port);
	/*
	 * Don't call event_set_handler(), we'll use a shortcut. Just set
	 * evtsource[] to a non-NULL value so that evtchn_do_event will
	 * be called.
	 */
	evtsource[debug_port] = (void *)-1;
	xen_atomic_set_bit(&curcpu()->ci_evtmask[0], debug_port);
	hypervisor_enable_event(debug_port);

	x86_enable_intr();		/* at long last... */
}

bool
events_suspend(void)
{
	int evtch;

	x86_disable_intr();

	/* VIRQ_DEBUG is the last interrupt to remove */
	evtch = unbind_virq_from_evtch(VIRQ_DEBUG);

	KASSERT(evtch != -1);

	hypervisor_mask_event(evtch);
	/* Remove the non-NULL value set in events_init() */
	evtsource[evtch] = NULL;
	aprint_verbose("VIRQ_DEBUG interrupt disabled, "
	    "event channel %d removed\n", evtch);

	return true;
}

bool
events_resume (void)
{
	events_init();

	return true;
}


unsigned int
evtchn_do_event(int evtch, struct intrframe *regs)
{
	struct cpu_info *ci;
	int ilevel;
	struct intrhand *ih;
	int	(*ih_fun)(void *, void *);
	uint32_t iplmask;
	int i;
	uint32_t iplbit;

	KASSERTMSG(evtch >= 0, "negative evtch: %d", evtch);
	KASSERTMSG(evtch < NR_EVENT_CHANNELS,
	    "evtch number %d > NR_EVENT_CHANNELS", evtch);

#ifdef IRQ_DEBUG
	if (evtch == IRQ_DEBUG)
		printf("evtchn_do_event: evtch %d\n", evtch);
#endif
	ci = curcpu();

	/*
	 * Shortcut for the debug handler, we want it to always run,
	 * regardless of the IPL level.
	 */
	if (__predict_false(evtch == debug_port)) {
		xen_debug_handler(NULL);
		hypervisor_enable_event(evtch);
		return 0;
	}

	KASSERTMSG(evtsource[evtch] != NULL, "unknown event %d", evtch);
	ci->ci_data.cpu_nintr++;
	evtsource[evtch]->ev_evcnt.ev_count++;
	ilevel = ci->ci_ilevel;

	if (evtsource[evtch]->ev_cpu != ci /* XXX: get stats */) {
		hypervisor_send_event(evtsource[evtch]->ev_cpu, evtch);
		return 0;
	}

	if (evtsource[evtch]->ev_maxlevel <= ilevel) {
#ifdef IRQ_DEBUG
		if (evtch == IRQ_DEBUG)
		    printf("evtsource[%d]->ev_maxlevel %d <= ilevel %d\n",
		    evtch, evtsource[evtch]->ev_maxlevel, ilevel);
#endif
		hypervisor_set_ipending(evtsource[evtch]->ev_imask,
					evtch >> LONG_SHIFT,
					evtch & LONG_MASK);

		/* leave masked */

		return 0;
	}
	ci->ci_ilevel = evtsource[evtch]->ev_maxlevel;
	iplmask = evtsource[evtch]->ev_imask;
	sti();
	mutex_spin_enter(&evtlock[evtch]);
	ih = evtsource[evtch]->ev_handlers;
	while (ih != NULL) {
		if (ih->ih_cpu != ci) {
			hypervisor_send_event(ih->ih_cpu, evtch);
			iplmask &= ~IUNMASK(ci, ih->ih_level);
			ih = ih->ih_evt_next;
			continue;
		}
		if (ih->ih_level <= ilevel) {
#ifdef IRQ_DEBUG
		if (evtch == IRQ_DEBUG)
		    printf("ih->ih_level %d <= ilevel %d\n", ih->ih_level, ilevel);
#endif
			cli();
			hypervisor_set_ipending(iplmask,
			    evtch >> LONG_SHIFT, evtch & LONG_MASK);
			/* leave masked */
			mutex_spin_exit(&evtlock[evtch]);
			goto splx;
		}
		iplmask &= ~IUNMASK(ci, ih->ih_level);
		ci->ci_ilevel = ih->ih_level;
		ih_fun = (void *)ih->ih_fun;
		ih_fun(ih->ih_arg, regs);
		KASSERTMSG(ci->ci_ilevel == ih->ih_level,
		    "event handler %p for evtsource[%d] (%s) changed ipl:"
		    " %d != %d",
		    ih->ih_realfun, evtch, evtsource[evtch]->ev_evname,
		    ci->ci_ilevel, ih->ih_level);
		ih = ih->ih_evt_next;
	}
	mutex_spin_exit(&evtlock[evtch]);
	cli();
	hypervisor_enable_event(evtch);
splx:
	/*
	 * C version of spllower(). ASTs will be checked when
	 * hypevisor_callback() exits, so no need to check here.
	 */
	iplmask = (IUNMASK(ci, ilevel) & ci->ci_ipending);
	while (iplmask != 0) {
		iplbit = 1 << (NIPL - 1);
		i = (NIPL - 1);
		while (iplmask != 0 && i > ilevel) {
			while (iplmask & iplbit) {
				ci->ci_ipending &= ~iplbit;
				ci->ci_ilevel = i;
				for (ih = ci->ci_isources[i]->is_handlers;
				    ih != NULL; ih = ih->ih_next) {
					KASSERT(ih->ih_cpu == ci);
					sti();
					ih_fun = (void *)ih->ih_fun;
					ih_fun(ih->ih_arg, regs);
					KASSERTMSG(ci->ci_ilevel == i,
					    "interrupt handler %p"
					    " for interrupt source %s"
					    " changed ipl: %d != %d",
					    ih->ih_realfun,
					    ci->ci_isources[i]->is_xname,
					    ci->ci_ilevel, i);
					cli();
				}
				hypervisor_enable_ipl(i);
				/* more pending IPLs may have been registered */
				iplmask =
				    (IUNMASK(ci, ilevel) & ci->ci_ipending);
			}
			i--;
			iplbit >>= 1;
		}
	}
	ci->ci_ilevel = ilevel;
	return 0;
}

#define PRIuCPUID	"lu" /* XXX: move this somewhere more appropriate */

/* PIC callbacks */
/* pic "pin"s are conceptually mapped to event port numbers */
static void
xen_evtchn_mask(struct pic *pic, int pin)
{
	evtchn_port_t evtchn = pin;

	KASSERT(pic->pic_type == PIC_XEN);
	KASSERT(evtchn < NR_EVENT_CHANNELS);

	hypervisor_mask_event(evtchn);
	
}

static void
xen_evtchn_unmask(struct pic *pic, int pin)
{
	evtchn_port_t evtchn = pin;

	KASSERT(pic->pic_type == PIC_XEN);
	KASSERT(evtchn < NR_EVENT_CHANNELS);

	hypervisor_unmask_event(evtchn);
	
}


static void
xen_evtchn_addroute(struct pic *pic, struct cpu_info *ci, int pin, int idt_vec, int type)
{

	evtchn_port_t evtchn = pin;

	/* Events are simulated as level triggered interrupts */
	KASSERT(type == IST_LEVEL); 

	KASSERT(evtchn < NR_EVENT_CHANNELS);
#if notyet
	evtchn_port_t boundport = idt_vec;
#endif
	
	KASSERT(pic->pic_type == PIC_XEN);

	xen_atomic_set_bit(&ci->ci_evtmask[0], evtchn);

}

static void
xen_evtchn_delroute(struct pic *pic, struct cpu_info *ci, int pin, int idt_vec, int type)
{
	/*
	 * XXX: In the future, this is a great place to
	 * 'unbind' events to underlying events and cpus.
	 * For now, just disable interrupt servicing on this cpu for
	 * this pin aka cpu.
	 */
	evtchn_port_t evtchn = pin;

	/* Events are simulated as level triggered interrupts */
	KASSERT(type == IST_LEVEL); 

	KASSERT(evtchn < NR_EVENT_CHANNELS);
#if notyet
	evtchn_port_t boundport = idt_vec;
#endif
	
	KASSERT(pic->pic_type == PIC_XEN);

	xen_atomic_clear_bit(&ci->ci_evtmask[0], evtchn);
}

/*
 * xen_evtchn_trymask(pic, pin)
 *
 *	If there are interrupts pending on the bus-shared pic, return
 *	false.  Otherwise, mask interrupts on the bus-shared pic and
 *	return true.
 */
static bool
xen_evtchn_trymask(struct pic *pic, int pin)
{
	volatile struct shared_info *s = HYPERVISOR_shared_info;
	unsigned long masked __diagused;

	/* Mask it.  */
	masked = xen_atomic_test_and_set_bit(&s->evtchn_mask[0], pin);

	/*
	 * Caller is responsible for calling trymask only when the
	 * interrupt pin is not masked, and for serializing calls to
	 * trymask.
	 */
	KASSERT(!masked);

	/*
	 * Check whether there were any interrupts pending when we
	 * masked it.  If there were, unmask and abort.
	 */
	if (xen_atomic_test_bit(&s->evtchn_pending[0], pin)) {
		xen_atomic_clear_bit(&s->evtchn_mask[0], pin);
		return false;
	}

	/* Success: masked, not pending.  */
	return true;
}

evtchn_port_t
bind_vcpu_to_evtch(cpuid_t vcpu)
{
	evtchn_op_t op;
	evtchn_port_t evtchn;

	mutex_spin_enter(&evtchn_lock);

	evtchn = vcpu_ipi_to_evtch[vcpu];
	if (evtchn == -1) {
		op.cmd = EVTCHNOP_bind_ipi;
		op.u.bind_ipi.vcpu = (uint32_t) vcpu;
		if (HYPERVISOR_event_channel_op(&op) != 0)
			panic("Failed to bind ipi to VCPU %"PRIuCPUID"\n", vcpu);
		evtchn = op.u.bind_ipi.port;

		vcpu_ipi_to_evtch[vcpu] = evtchn;
	}

	evtch_bindcount[evtchn]++;

	mutex_spin_exit(&evtchn_lock);

	return evtchn;
}

int
bind_virq_to_evtch(int virq)
{
	evtchn_op_t op;
	int evtchn;

	mutex_spin_enter(&evtchn_lock);

	/*
	 * XXX: The only per-cpu VIRQ we currently use is VIRQ_TIMER.
	 * Please re-visit this implementation when others are used.
	 * Note: VIRQ_DEBUG is special-cased, and not used or bound on APs.
	 * XXX: event->virq/ipi can be unified in a linked-list
	 * implementation.
	 */
	struct cpu_info *ci = curcpu();

	if (virq == VIRQ_DEBUG && ci != &cpu_info_primary) {
		mutex_spin_exit(&evtchn_lock);
		return -1;
	}

	if (virq == VIRQ_TIMER) {
		evtchn = virq_timer_to_evtch[ci->ci_cpuid];
	} else {
		evtchn = virq_to_evtch[virq];
	}

	/* Allocate a channel if there is none already allocated */
	if (evtchn == -1) {
		op.cmd = EVTCHNOP_bind_virq;
		op.u.bind_virq.virq = virq;
		op.u.bind_virq.vcpu = ci->ci_cpuid;
		if (HYPERVISOR_event_channel_op(&op) != 0)
			panic("Failed to bind virtual IRQ %d\n", virq);
		evtchn = op.u.bind_virq.port;
	}

	/* Set event channel */
	if (virq == VIRQ_TIMER) {
		virq_timer_to_evtch[ci->ci_cpuid] = evtchn;
	} else {
		virq_to_evtch[virq] = evtchn;
	}

	/* Increase ref counter */
	evtch_bindcount[evtchn]++;

	mutex_spin_exit(&evtchn_lock);

	return evtchn;
}

int
unbind_virq_from_evtch(int virq)
{
	evtchn_op_t op;
	int evtchn;

	struct cpu_info *ci = curcpu();

	if (virq == VIRQ_TIMER) {
		evtchn = virq_timer_to_evtch[ci->ci_cpuid];
	}
	else {
		evtchn = virq_to_evtch[virq];
	}

	if (evtchn == -1) {
		return -1;
	}

	mutex_spin_enter(&evtchn_lock);

	evtch_bindcount[evtchn]--;
	if (evtch_bindcount[evtchn] == 0) {
		op.cmd = EVTCHNOP_close;
		op.u.close.port = evtchn;
		if (HYPERVISOR_event_channel_op(&op) != 0)
			panic("Failed to unbind virtual IRQ %d\n", virq);

		if (virq == VIRQ_TIMER) {
			virq_timer_to_evtch[ci->ci_cpuid] = -1;
		} else {
			virq_to_evtch[virq] = -1;
		}
	}

	mutex_spin_exit(&evtchn_lock);

	return evtchn;
}

#if NPCI > 0 || NISA > 0
int
get_pirq_to_evtch(int pirq)
{
	int evtchn;

	if (pirq == -1) /* Match previous behaviour */
		return -1;
	
	if (pirq >= NR_PIRQS) {
		panic("pirq %d out of bound, increase NR_PIRQS", pirq);
	}
	mutex_spin_enter(&evtchn_lock);

	evtchn = pirq_to_evtch[pirq];

	mutex_spin_exit(&evtchn_lock);

	return evtchn;
}

int
bind_pirq_to_evtch(int pirq)
{
	evtchn_op_t op;
	int evtchn;

	if (pirq >= NR_PIRQS) {
		panic("pirq %d out of bound, increase NR_PIRQS", pirq);
	}

	mutex_spin_enter(&evtchn_lock);

	evtchn = pirq_to_evtch[pirq];
	if (evtchn == -1) {
		op.cmd = EVTCHNOP_bind_pirq;
		op.u.bind_pirq.pirq = pirq;
		op.u.bind_pirq.flags = BIND_PIRQ__WILL_SHARE;
		if (HYPERVISOR_event_channel_op(&op) != 0)
			panic("Failed to bind physical IRQ %d\n", pirq);
		evtchn = op.u.bind_pirq.port;

#ifdef IRQ_DEBUG
		printf("pirq %d evtchn %d\n", pirq, evtchn);
#endif
		pirq_to_evtch[pirq] = evtchn;
	}

	evtch_bindcount[evtchn]++;

	mutex_spin_exit(&evtchn_lock);

	return evtchn;
}

int
unbind_pirq_from_evtch(int pirq)
{
	evtchn_op_t op;
	int evtchn = pirq_to_evtch[pirq];

	mutex_spin_enter(&evtchn_lock);

	evtch_bindcount[evtchn]--;
	if (evtch_bindcount[evtchn] == 0) {
		op.cmd = EVTCHNOP_close;
		op.u.close.port = evtchn;
		if (HYPERVISOR_event_channel_op(&op) != 0)
			panic("Failed to unbind physical IRQ %d\n", pirq);

		pirq_to_evtch[pirq] = -1;
	}

	mutex_spin_exit(&evtchn_lock);

	return evtchn;
}

struct pintrhand *
pirq_establish(int pirq, int evtch, int (*func)(void *), void *arg, int level,
    const char *evname)
{
	struct pintrhand *ih;
	physdev_op_t physdev_op;

	ih = kmem_zalloc(sizeof(struct pintrhand),
	    cold ? KM_NOSLEEP : KM_SLEEP);
	if (ih == NULL) {
		printf("pirq_establish: can't allocate handler info\n");
		return NULL;
	}

	ih->pirq = pirq;
	ih->evtch = evtch;
	ih->func = func;
	ih->arg = arg;

	if (event_set_handler(evtch, pirq_interrupt, ih, level, evname) != 0) {
		kmem_free(ih, sizeof(struct pintrhand));
		return NULL;
	}

	physdev_op.cmd = PHYSDEVOP_IRQ_STATUS_QUERY;
	physdev_op.u.irq_status_query.irq = pirq;
	if (HYPERVISOR_physdev_op(&physdev_op) < 0)
		panic("HYPERVISOR_physdev_op(PHYSDEVOP_IRQ_STATUS_QUERY)");
	if (physdev_op.u.irq_status_query.flags &
	    PHYSDEVOP_IRQ_NEEDS_UNMASK_NOTIFY) {
		pirq_needs_unmask_notify[evtch >> 5] |= (1 << (evtch & 0x1f));
#ifdef IRQ_DEBUG
		printf("pirq %d needs notify\n", pirq);
#endif
	}
	hypervisor_enable_event(evtch);
	return ih;
}

void
pirq_disestablish(struct pintrhand *ih)
{
	int error = event_remove_handler(ih->evtch, pirq_interrupt, ih);
	if (error) {
		printf("pirq_disestablish(%p): %d\n", ih, error);
		return;
	}
	kmem_free(ih, sizeof(struct pintrhand));
}

int
pirq_interrupt(void *arg)
{
	struct pintrhand *ih = arg;
	int ret;


	ret = ih->func(ih->arg);
#ifdef IRQ_DEBUG
	if (ih->evtch == IRQ_DEBUG)
	    printf("pirq_interrupt irq %d ret %d\n", ih->pirq, ret);
#endif
	return ret;
}

#endif /* NPCI > 0 || NISA > 0 */


/*
 * Recalculate the interrupt from scratch for an event source.
 */
static void
intr_calculatemasks(struct evtsource *evts, int evtch, struct cpu_info *ci)
{
	struct intrhand *ih;
	int cpu_receive = 0;

#ifdef MULTIPROCESSOR
	KASSERT(!mutex_owned(&evtlock[evtch]));
#endif
	mutex_spin_enter(&evtlock[evtch]);
	evts->ev_maxlevel = IPL_NONE;
	evts->ev_imask = 0;
	for (ih = evts->ev_handlers; ih != NULL; ih = ih->ih_evt_next) {
		if (ih->ih_level > evts->ev_maxlevel)
			evts->ev_maxlevel = ih->ih_level;
		evts->ev_imask |= (1 << ih->ih_level);
		if (ih->ih_cpu == ci)
			cpu_receive = 1;
	}
	if (cpu_receive)
		xen_atomic_set_bit(&curcpu()->ci_evtmask[0], evtch);
	else
		xen_atomic_clear_bit(&curcpu()->ci_evtmask[0], evtch);
	mutex_spin_exit(&evtlock[evtch]);
}

int
event_set_handler(int evtch, int (*func)(void *), void *arg, int level,
    const char *evname)
{
	struct cpu_info *ci = curcpu(); /* XXX: pass in ci ? */
	struct evtsource *evts;
	struct intrhand *ih, **ihp;
	int s;
#ifdef MULTIPROCESSOR
	bool mpsafe = (level != IPL_VM);
#endif /* MULTIPROCESSOR */

#ifdef IRQ_DEBUG
	printf("event_set_handler IRQ %d handler %p\n", evtch, func);
#endif

	KASSERTMSG(evtch >= 0, "negative evtch: %d", evtch);
	KASSERTMSG(evtch < NR_EVENT_CHANNELS,
	    "evtch number %d > NR_EVENT_CHANNELS", evtch);

#if 0
	printf("event_set_handler evtch %d handler %p level %d\n", evtch,
	       handler, level);
#endif
	ih = kmem_zalloc(sizeof (struct intrhand), KM_NOSLEEP);
	if (ih == NULL)
		panic("can't allocate fixed interrupt source");


	ih->ih_level = level;
	ih->ih_fun = ih->ih_realfun = func;
	ih->ih_arg = ih->ih_realarg = arg;
	ih->ih_evt_next = NULL;
	ih->ih_next = NULL;
	ih->ih_cpu = ci;
#ifdef MULTIPROCESSOR
	if (!mpsafe) {
		ih->ih_fun = xen_intr_biglock_wrapper;
		ih->ih_arg = ih;
	}
#endif /* MULTIPROCESSOR */

	s = splhigh();

	/* register per-cpu handler for spllower() */
	event_set_iplhandler(ci, ih, level);

	/* register handler for event channel */
	if (evtsource[evtch] == NULL) {
		evts = kmem_zalloc(sizeof (struct evtsource),
		    KM_NOSLEEP);
		if (evts == NULL)
			panic("can't allocate fixed interrupt source");

		evts->ev_handlers = ih;
		/*
		 * XXX: We're assuming here that ci is the same cpu as
		 * the one on which this event/port is bound on. The
		 * api needs to be reshuffled so that this assumption
		 * is more explicitly implemented.
		 */
		evts->ev_cpu = ci;
		mutex_init(&evtlock[evtch], MUTEX_DEFAULT, IPL_HIGH);
		evtsource[evtch] = evts;
		if (evname)
			strncpy(evts->ev_evname, evname,
			    sizeof(evts->ev_evname));
		else
			snprintf(evts->ev_evname, sizeof(evts->ev_evname),
			    "evt%d", evtch);
		evcnt_attach_dynamic(&evts->ev_evcnt, EVCNT_TYPE_INTR, NULL,
		    device_xname(ci->ci_dev), evts->ev_evname);
	} else {
		evts = evtsource[evtch];
		/* sort by IPL order, higher first */
		mutex_spin_enter(&evtlock[evtch]);
		for (ihp = &evts->ev_handlers; ; ihp = &((*ihp)->ih_evt_next)) {
			if ((*ihp)->ih_level < ih->ih_level) {
				/* insert before *ihp */
				ih->ih_evt_next = *ihp;
				*ihp = ih;
				break;
			}
			if ((*ihp)->ih_evt_next == NULL) {
				(*ihp)->ih_evt_next = ih;
				break;
			}
		}
		mutex_spin_exit(&evtlock[evtch]);
	}

	intr_calculatemasks(evts, evtch, ci);
	splx(s);

	return 0;
}

void
event_set_iplhandler(struct cpu_info *ci,
		     struct intrhand *ih,
		     int level)
{
	struct intrsource *ipls;

	KASSERT(ci == ih->ih_cpu);
	if (ci->ci_isources[level] == NULL) {
		ipls = kmem_zalloc(sizeof (struct intrsource),
		    KM_NOSLEEP);
		if (ipls == NULL)
			panic("can't allocate fixed interrupt source");
		ipls->is_recurse = xenev_stubs[level].ist_recurse;
		ipls->is_resume = xenev_stubs[level].ist_resume;
		ipls->is_handlers = ih;
		ci->ci_isources[level] = ipls;
	} else {
		ipls = ci->ci_isources[level];
		ih->ih_next = ipls->is_handlers;
		ipls->is_handlers = ih;
	}
}

int
event_remove_handler(int evtch, int (*func)(void *), void *arg)
{
	struct intrsource *ipls;
	struct evtsource *evts;
	struct intrhand *ih;
	struct intrhand **ihp;
	struct cpu_info *ci;

	evts = evtsource[evtch];
	if (evts == NULL)
		return ENOENT;

	mutex_spin_enter(&evtlock[evtch]);
	for (ihp = &evts->ev_handlers, ih = evts->ev_handlers;
	    ih != NULL;
	    ihp = &ih->ih_evt_next, ih = ih->ih_evt_next) {
		if (ih->ih_realfun == func && ih->ih_realarg == arg)
			break;
	}
	if (ih == NULL) {
		mutex_spin_exit(&evtlock[evtch]);
		return ENOENT;
	}
	ci = ih->ih_cpu;
	*ihp = ih->ih_evt_next;

	ipls = ci->ci_isources[ih->ih_level];
	for (ihp = &ipls->is_handlers, ih = ipls->is_handlers;
	    ih != NULL;
	    ihp = &ih->ih_next, ih = ih->ih_next) {
		if (ih->ih_realfun == func && ih->ih_realarg == arg)
			break;
	}
	if (ih == NULL)
		panic("event_remove_handler");
	*ihp = ih->ih_next;
	mutex_spin_exit(&evtlock[evtch]);
	kmem_free(ih, sizeof (struct intrhand));
	if (evts->ev_handlers == NULL) {
		xen_atomic_clear_bit(&ci->ci_evtmask[0], evtch);
		evcnt_detach(&evts->ev_evcnt);
		kmem_free(evts, sizeof (struct evtsource));
		evtsource[evtch] = NULL;
	} else {
		intr_calculatemasks(evts, evtch, ci);
	}
	return 0;
}

void
hypervisor_enable_event(unsigned int evtch)
{
#ifdef IRQ_DEBUG
	if (evtch == IRQ_DEBUG)
		printf("hypervisor_enable_evtch: evtch %d\n", evtch);
#endif

	hypervisor_unmask_event(evtch);
#if NPCI > 0 || NISA > 0
	if (pirq_needs_unmask_notify[evtch >> 5] & (1 << (evtch & 0x1f))) {
#ifdef  IRQ_DEBUG
		if (evtch == IRQ_DEBUG)
		    printf("pirq_notify(%d)\n", evtch);
#endif
		(void)HYPERVISOR_physdev_op(&physdev_op_notify);
	}
#endif /* NPCI > 0 || NISA > 0 */
}

int
xen_debug_handler(void *arg)
{
	struct cpu_info *ci = curcpu();
	int i;
	int xci_ilevel = ci->ci_ilevel;
	int xci_ipending = ci->ci_ipending;
	int xci_idepth = ci->ci_idepth;
	u_long upcall_pending = ci->ci_vcpu->evtchn_upcall_pending;
	u_long upcall_mask = ci->ci_vcpu->evtchn_upcall_mask;
	u_long pending_sel = ci->ci_vcpu->evtchn_pending_sel;
	unsigned long evtchn_mask[sizeof(unsigned long) * 8];
	unsigned long evtchn_pending[sizeof(unsigned long) * 8];

	u_long p;

	p = (u_long)&HYPERVISOR_shared_info->evtchn_mask[0];
	memcpy(evtchn_mask, (void *)p, sizeof(evtchn_mask));
	p = (u_long)&HYPERVISOR_shared_info->evtchn_pending[0];
	memcpy(evtchn_pending, (void *)p, sizeof(evtchn_pending));

	__insn_barrier();
	printf("debug event\n");
	printf("ci_ilevel 0x%x ci_ipending 0x%x ci_idepth %d\n",
	    xci_ilevel, xci_ipending, xci_idepth);
	printf("evtchn_upcall_pending %ld evtchn_upcall_mask %ld"
	    " evtchn_pending_sel 0x%lx\n",
		upcall_pending, upcall_mask, pending_sel);
	printf("evtchn_mask");
	for (i = 0 ; i <= LONG_MASK; i++)
		printf(" %lx", (u_long)evtchn_mask[i]);
	printf("\n");
	printf("evtchn_pending");
	for (i = 0 ; i <= LONG_MASK; i++)
		printf(" %lx", (u_long)evtchn_pending[i]);
	printf("\n");
	return 0;
}
