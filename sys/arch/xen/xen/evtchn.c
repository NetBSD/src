/*	$NetBSD: evtchn.c,v 1.99 2022/05/25 14:35:15 bouyer Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: evtchn.c,v 1.99 2022/05/25 14:35:15 bouyer Exp $");

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
#include <sys/interrupt.h>
#include <sys/xcall.h>

#include <uvm/uvm.h>

#include <xen/intr.h>

#include <xen/xen.h>
#include <xen/hypervisor.h>
#include <xen/evtchn.h>
#include <xen/xenfunc.h>

/* maximum number of (v)CPUs supported */
#ifdef XENPV
#define NBSD_XEN_MAX_VCPUS XEN_LEGACY_MAX_VCPUS
#else
#include <xen/include/public/hvm/hvm_info_table.h>
#define NBSD_XEN_MAX_VCPUS HVM_MAX_VCPUS
#endif

#define	NR_PIRQS	NR_EVENT_CHANNELS

/*
 * This lock protects updates to the following mapping and reference-count
 * arrays. The lock does not need to be acquired to read the mapping tables.
 */
static kmutex_t evtchn_lock;

/* event handlers */
struct evtsource *evtsource[NR_EVENT_CHANNELS];

/* Reference counts for bindings to event channels XXX: redo for SMP */
static uint8_t evtch_bindcount[NR_EVENT_CHANNELS];

/* event-channel <-> VCPU mapping for IPIs. XXX: redo for SMP. */
static evtchn_port_t vcpu_ipi_to_evtch[NBSD_XEN_MAX_VCPUS];

/* event-channel <-> VCPU mapping for VIRQ_TIMER.  XXX: redo for SMP. */
static int virq_timer_to_evtch[NBSD_XEN_MAX_VCPUS];

/* event-channel <-> VIRQ mapping. */
static int virq_to_evtch[NR_VIRQS];


#if defined(XENPV) && (NPCI > 0 || NISA > 0)
/* event-channel <-> PIRQ mapping */
static int pirq_to_evtch[NR_PIRQS];
/* PIRQ needing notify */
static int evtch_to_pirq_eoi[NR_EVENT_CHANNELS];
int pirq_interrupt(void *);
#endif /* defined(XENPV) && (NPCI > 0 || NISA > 0) */

static void xen_evtchn_mask(struct pic *, int);
static void xen_evtchn_unmask(struct pic *, int);
static void xen_evtchn_addroute(struct pic *, struct cpu_info *, int, int, int);
static void xen_evtchn_delroute(struct pic *, struct cpu_info *, int, int, int);
static bool xen_evtchn_trymask(struct pic *, int);
static void xen_intr_get_devname(const char *, char *, size_t);
static void xen_intr_get_assigned(const char *, kcpuset_t *);
static uint64_t xen_intr_get_count(const char *, u_int);

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
	.pic_intr_get_devname = xen_intr_get_devname,
	.pic_intr_get_assigned = xen_intr_get_assigned,
	.pic_intr_get_count = xen_intr_get_count,
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

/* #define IRQ_DEBUG 4 */

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
	for (i = 0; i < NBSD_XEN_MAX_VCPUS; i++)
		vcpu_ipi_to_evtch[i] = -1;

	/* No VIRQ_TIMER -> event mappings. */
	for (i = 0; i < NBSD_XEN_MAX_VCPUS; i++)
		virq_timer_to_evtch[i] = -1;

	/* No VIRQ -> event mappings. */
	for (i = 0; i < NR_VIRQS; i++)
		virq_to_evtch[i] = -1;

#if defined(XENPV) && (NPCI > 0 || NISA > 0)
	/* No PIRQ -> event mappings. */
	for (i = 0; i < NR_PIRQS; i++)
		pirq_to_evtch[i] = -1;
	for (i = 0; i < NR_EVENT_CHANNELS; i++)
		evtch_to_pirq_eoi[i] = -1;
#endif /* defined(XENPV) && (NPCI > 0 || NISA > 0) */

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

	(void)events_resume();
}

bool
events_resume(void)
{
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
	hypervisor_unmask_event(debug_port);
	x86_enable_intr();		/* at long last... */

	return true;
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

unsigned int
evtchn_do_event(int evtch, struct intrframe *regs)
{
	struct cpu_info *ci;
	int ilevel;
	struct intrhand *ih;
	int	(*ih_fun)(void *, void *);
	uint32_t iplmask;

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
		hypervisor_unmask_event(debug_port);
		return 0;
	}

	KASSERTMSG(evtsource[evtch] != NULL, "unknown event %d", evtch);

	if (evtsource[evtch]->ev_cpu != ci)
		return 0;

	ci->ci_data.cpu_nintr++;
	evtsource[evtch]->ev_evcnt.ev_count++;
	ilevel = ci->ci_ilevel;

	if (evtsource[evtch]->ev_maxlevel <= ilevel) {
#ifdef IRQ_DEBUG
		if (evtch == IRQ_DEBUG)
		    printf("evtsource[%d]->ev_maxlevel %d <= ilevel %d\n",
		    evtch, evtsource[evtch]->ev_maxlevel, ilevel);
#endif
		hypervisor_set_ipending(evtsource[evtch]->ev_imask,
					evtch >> LONG_SHIFT,
					evtch & LONG_MASK);
		ih = evtsource[evtch]->ev_handlers;
		while (ih != NULL) {
			ih->ih_pending++;
			ih = ih->ih_evt_next;
		}

		/* leave masked */

		return 0;
	}
	ci->ci_ilevel = evtsource[evtch]->ev_maxlevel;
	iplmask = evtsource[evtch]->ev_imask;
	KASSERT(ci->ci_ilevel >= IPL_VM);
	KASSERT(cpu_intr_p());
	x86_enable_intr();
	ih = evtsource[evtch]->ev_handlers;
	while (ih != NULL) {
		KASSERT(ih->ih_cpu == ci);
#if 0
		if (ih->ih_cpu != ci) {
			hypervisor_send_event(ih->ih_cpu, evtch);
			iplmask &= ~(1 << XEN_IPL2SIR(ih->ih_level));
			ih = ih->ih_evt_next;
			continue;
		}
#endif
		if (ih->ih_level <= ilevel) {
#ifdef IRQ_DEBUG
		if (evtch == IRQ_DEBUG)
		    printf("ih->ih_level %d <= ilevel %d\n", ih->ih_level, ilevel);
#endif
			x86_disable_intr();
			hypervisor_set_ipending(iplmask,
			    evtch >> LONG_SHIFT, evtch & LONG_MASK);
			/* leave masked */
			while (ih != NULL) {
				ih->ih_pending++;
				ih = ih->ih_evt_next;
			}
			goto splx;
		}
		iplmask &= ~(1 << XEN_IPL2SIR(ih->ih_level));
		ci->ci_ilevel = ih->ih_level;
		ih->ih_pending = 0;
		ih_fun = (void *)ih->ih_fun;
		ih_fun(ih->ih_arg, regs);
		ih = ih->ih_evt_next;
	}
	x86_disable_intr();
	hypervisor_unmask_event(evtch);
#if defined(XENPV) && (NPCI > 0 || NISA > 0)
	hypervisor_ack_pirq_event(evtch);
#endif /* defined(XENPV) && (NPCI > 0 || NISA > 0) */

splx:
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
		evtchn = virq_timer_to_evtch[ci->ci_vcpuid];
	} else {
		evtchn = virq_to_evtch[virq];
	}

	/* Allocate a channel if there is none already allocated */
	if (evtchn == -1) {
		op.cmd = EVTCHNOP_bind_virq;
		op.u.bind_virq.virq = virq;
		op.u.bind_virq.vcpu = ci->ci_vcpuid;
		if (HYPERVISOR_event_channel_op(&op) != 0)
			panic("Failed to bind virtual IRQ %d\n", virq);
		evtchn = op.u.bind_virq.port;
	}

	/* Set event channel */
	if (virq == VIRQ_TIMER) {
		virq_timer_to_evtch[ci->ci_vcpuid] = evtchn;
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
		evtchn = virq_timer_to_evtch[ci->ci_vcpuid];
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
			virq_timer_to_evtch[ci->ci_vcpuid] = -1;
		} else {
			virq_to_evtch[virq] = -1;
		}
	}

	mutex_spin_exit(&evtchn_lock);

	return evtchn;
}

#if defined(XENPV) && (NPCI > 0 || NISA > 0) 
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
    const char *intrname, const char *xname, bool known_mpsafe)
{
	struct pintrhand *ih;

	ih = kmem_zalloc(sizeof(struct pintrhand),
	    cold ? KM_NOSLEEP : KM_SLEEP);
	if (ih == NULL) {
		printf("pirq_establish: can't allocate handler info\n");
		return NULL;
	}

	KASSERT(evtch > 0);

	ih->pirq = pirq;
	ih->evtch = evtch;
	ih->func = func;
	ih->arg = arg;

	if (event_set_handler(evtch, pirq_interrupt, ih, level, intrname,
	    xname, known_mpsafe, NULL) == NULL) {
		kmem_free(ih, sizeof(struct pintrhand));
		return NULL;
	}

	hypervisor_prime_pirq_event(pirq, evtch);
	hypervisor_unmask_event(evtch);
	hypervisor_ack_pirq_event(evtch);
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

#endif /* defined(XENPV) && (NPCI > 0 || NISA > 0) */


/*
 * Recalculate the interrupt from scratch for an event source.
 */
static void
intr_calculatemasks(struct evtsource *evts, int evtch, struct cpu_info *ci)
{
	struct intrhand *ih;
	int cpu_receive = 0;

	evts->ev_maxlevel = IPL_NONE;
	evts->ev_imask = 0;
	for (ih = evts->ev_handlers; ih != NULL; ih = ih->ih_evt_next) {
		KASSERT(ih->ih_cpu == curcpu());
		if (ih->ih_level > evts->ev_maxlevel)
			evts->ev_maxlevel = ih->ih_level;
		evts->ev_imask |= (1 << XEN_IPL2SIR(ih->ih_level));
		if (ih->ih_cpu == ci)
			cpu_receive = 1;
	}
	if (cpu_receive)
		xen_atomic_set_bit(&curcpu()->ci_evtmask[0], evtch);
	else
		xen_atomic_clear_bit(&curcpu()->ci_evtmask[0], evtch);
}


struct event_set_handler_args {
	struct intrhand *ih;
	struct intrsource *ipls;
	struct evtsource *evts;
	int evtch;
};

/*
 * Called on bound CPU to handle event_set_handler()
 * caller (on initiating CPU) holds cpu_lock on our behalf
 * arg1: struct event_set_handler_args *
 * arg2: NULL
 */

static void
event_set_handler_xcall(void *arg1, void *arg2)
{
	struct event_set_handler_args *esh_args = arg1;
	struct intrhand **ihp, *ih = esh_args->ih;
	struct evtsource *evts = esh_args->evts;

	const u_long psl = x86_read_psl();
	x86_disable_intr();
	/* sort by IPL order, higher first */
	for (ihp = &evts->ev_handlers; *ihp != NULL;
	    ihp = &((*ihp)->ih_evt_next)) {
		if ((*ihp)->ih_level < ih->ih_level)
			break;
	}
	/* insert before *ihp */
	ih->ih_evt_next = *ihp;
	*ihp = ih;
#ifndef XENPV
	evts->ev_isl->is_handlers = evts->ev_handlers;
#endif
	/* register per-cpu handler for spllower() */
	struct cpu_info *ci = ih->ih_cpu;
	int sir = XEN_IPL2SIR(ih->ih_level);
	KASSERT(sir >= SIR_XENIPL_VM && sir <= SIR_XENIPL_HIGH);

	KASSERT(ci == curcpu());
	if (ci->ci_isources[sir] == NULL) {
		KASSERT(esh_args->ipls != NULL);
		ci->ci_isources[sir] = esh_args->ipls;
	}
	struct intrsource *ipls = ci->ci_isources[sir];
	ih->ih_next = ipls->is_handlers;
	ipls->is_handlers = ih;
	x86_intr_calculatemasks(ci);

	intr_calculatemasks(evts, esh_args->evtch, ci);
	x86_write_psl(psl);
}

struct intrhand *
event_set_handler(int evtch, int (*func)(void *), void *arg, int level,
    const char *intrname, const char *xname, bool mpsafe, struct cpu_info *ci)
{
	struct event_set_handler_args esh_args;
	char intrstr_buf[INTRIDBUF];
	bool bind = false;

	memset(&esh_args, 0, sizeof(esh_args));

	/*
	 * if ci is not specified, we bind to the current cpu.
	 * if ci has been proviced by the called, we assume 
	 * he will do the EVTCHNOP_bind_vcpu if needed.
	 */
	if (ci == NULL) {
		ci = curcpu();
		bind = true;
	}
		

#ifdef IRQ_DEBUG
	printf("event_set_handler IRQ %d handler %p\n", evtch, func);
#endif

	KASSERTMSG(evtch >= 0, "negative evtch: %d", evtch);
	KASSERTMSG(evtch < NR_EVENT_CHANNELS,
	    "evtch number %d > NR_EVENT_CHANNELS", evtch);
	KASSERT(xname != NULL);

#if 0
	printf("event_set_handler evtch %d handler %p level %d\n", evtch,
	       handler, level);
#endif
	esh_args.ih = kmem_zalloc(sizeof (struct intrhand), KM_NOSLEEP);
	if (esh_args.ih == NULL)
		panic("can't allocate fixed interrupt source");


	esh_args.ih->ih_pic = &xen_pic;
	esh_args.ih->ih_level = level;
	esh_args.ih->ih_fun = esh_args.ih->ih_realfun = func;
	esh_args.ih->ih_arg = esh_args.ih->ih_realarg = arg;
	esh_args.ih->ih_evt_next = NULL;
	esh_args.ih->ih_next = NULL;
	esh_args.ih->ih_pending = 0;
	esh_args.ih->ih_cpu = ci;
	esh_args.ih->ih_pin = evtch;
#ifdef MULTIPROCESSOR
	if (!mpsafe) {
		esh_args.ih->ih_fun = xen_intr_biglock_wrapper;
		esh_args.ih->ih_arg = esh_args.ih;
	}
#endif /* MULTIPROCESSOR */
	KASSERT(mpsafe || level < IPL_HIGH);

	mutex_enter(&cpu_lock);
	/* allocate IPL source if needed */
	int sir = XEN_IPL2SIR(level);
	if (ci->ci_isources[sir] == NULL) {
		struct intrsource *ipls;
		ipls = kmem_zalloc(sizeof (struct intrsource), KM_NOSLEEP);
		if (ipls == NULL)
			panic("can't allocate fixed interrupt source");
		ipls->is_recurse = xenev_stubs[level - IPL_VM].ist_recurse;
		ipls->is_resume = xenev_stubs[level - IPL_VM].ist_resume;
		ipls->is_pic = &xen_pic;
		esh_args.ipls = ipls;
		/*
		 * note that we can't set ci_isources here, as
		 * the assembly can't handle is_handlers being NULL
		 */
	}
	/* register handler for event channel */
	if (evtsource[evtch] == NULL) {
		struct evtsource *evts;
		evtchn_op_t op;
		if (intrname == NULL)
			intrname = intr_create_intrid(-1, &xen_pic, evtch,
			    intrstr_buf, sizeof(intrstr_buf));
		evts = kmem_zalloc(sizeof (struct evtsource),
		    KM_NOSLEEP);
		if (evts == NULL)
			panic("can't allocate fixed interrupt source");

		evts->ev_cpu = ci;
		strlcpy(evts->ev_intrname, intrname, sizeof(evts->ev_intrname));

		evcnt_attach_dynamic(&evts->ev_evcnt, EVCNT_TYPE_INTR, NULL,
		    device_xname(ci->ci_dev), evts->ev_intrname);
		if (bind) {
			op.cmd = EVTCHNOP_bind_vcpu;
			op.u.bind_vcpu.port = evtch;
			op.u.bind_vcpu.vcpu = ci->ci_vcpuid;
			if (HYPERVISOR_event_channel_op(&op) != 0) {
				panic("Failed to bind event %d to VCPU  %s %d",
				    evtch, device_xname(ci->ci_dev),
				    ci->ci_vcpuid);
			}
		}
#ifndef XENPV
		evts->ev_isl = intr_allocate_io_intrsource(intrname);
		evts->ev_isl->is_pic = &xen_pic;
#endif
		evtsource[evtch] = evts;
	}
	esh_args.evts = evtsource[evtch];

	// append device name
	if (esh_args.evts->ev_xname[0] != '\0') {
		strlcat(esh_args.evts->ev_xname, ", ",
		    sizeof(esh_args.evts->ev_xname));
	}
	strlcat(esh_args.evts->ev_xname, xname,
	    sizeof(esh_args.evts->ev_xname));

	esh_args.evtch = evtch;

	if (ci == curcpu() || !mp_online) {
		event_set_handler_xcall(&esh_args, NULL);
	} else {
		uint64_t where = xc_unicast(0, event_set_handler_xcall,
		    &esh_args, NULL, ci);
		xc_wait(where);
	}

	mutex_exit(&cpu_lock);
	return esh_args.ih;
}

/*
 * Called on bound CPU to handle event_remove_handler()
 * caller (on initiating CPU) holds cpu_lock on our behalf
 * arg1: evtch
 * arg2: struct intrhand *ih
 */

static void
event_remove_handler_xcall(void *arg1, void *arg2)
{
	struct intrsource *ipls;
	struct evtsource *evts;
	struct intrhand **ihp;
	struct cpu_info *ci;
	struct intrhand *ih = arg2;
	int evtch = (intptr_t)(arg1);

	evts = evtsource[evtch];
	KASSERT(evts != NULL);
	KASSERT(ih != NULL);
	ci = ih->ih_cpu;
	KASSERT(ci == curcpu());

	const u_long psl = x86_read_psl();
	x86_disable_intr();
	
	for (ihp = &evts->ev_handlers; *ihp != NULL;
	    ihp = &(*ihp)->ih_evt_next) {
		if ((*ihp) == ih)
			break;
	}
	if (*(ihp) == NULL) {
		panic("event_remove_handler_xcall: not in ev_handlers");
	}

	*ihp = ih->ih_evt_next;

	int sir = XEN_IPL2SIR(ih->ih_level);
	KASSERT(sir >= SIR_XENIPL_VM && sir <= SIR_XENIPL_HIGH);
	ipls = ci->ci_isources[sir];
	for (ihp = &ipls->is_handlers; *ihp != NULL; ihp = &(*ihp)->ih_next) {
		if (*ihp == ih)
			break;
	}
	if (*ihp == NULL)
		panic("event_remove_handler_xcall: not in is_handlers");
	*ihp = ih->ih_next;
	intr_calculatemasks(evts, evtch, ci);
#ifndef XENPV
	evts->ev_isl->is_handlers = evts->ev_handlers;
#endif
	if (evts->ev_handlers == NULL)
		xen_atomic_clear_bit(&ci->ci_evtmask[0], evtch);

	x86_write_psl(psl);
}

int
event_remove_handler(int evtch, int (*func)(void *), void *arg)
{
	struct intrhand *ih;
	struct cpu_info *ci;
	struct evtsource *evts;

	mutex_enter(&cpu_lock);
	evts = evtsource[evtch];
	if (evts == NULL)
		return ENOENT;

	for (ih = evts->ev_handlers; ih != NULL; ih = ih->ih_evt_next) {
		if (ih->ih_realfun == func && ih->ih_realarg == arg)
			break;
	}
	if (ih == NULL) {
		mutex_exit(&cpu_lock);
		return ENOENT;
	}
	ci = ih->ih_cpu;

	if (ci == curcpu() || !mp_online) {
		event_remove_handler_xcall((void *)(intptr_t)evtch, ih);
	} else {
		uint64_t where = xc_unicast(0, event_remove_handler_xcall,
		    (void *)(intptr_t)evtch, ih, ci);
		xc_wait(where);
	}

	kmem_free(ih, sizeof (struct intrhand));
	if (evts->ev_handlers == NULL) {
#ifndef XENPV
		KASSERT(evts->ev_isl->is_handlers == NULL);
		intr_free_io_intrsource(evts->ev_intrname);
#endif
		evcnt_detach(&evts->ev_evcnt);
		kmem_free(evts, sizeof (struct evtsource));
		evtsource[evtch] = NULL;
	}
	mutex_exit(&cpu_lock);
	return 0;
}

#if defined(XENPV) && (NPCI > 0 || NISA > 0)
void
hypervisor_prime_pirq_event(int pirq, unsigned int evtch)
{
	struct physdev_irq_status_query irq_status;
	irq_status.irq = pirq;
	if (HYPERVISOR_physdev_op(PHYSDEVOP_irq_status_query, &irq_status) < 0)
		panic("HYPERVISOR_physdev_op(PHYSDEVOP_IRQ_STATUS_QUERY)");
	if (irq_status.flags & XENIRQSTAT_needs_eoi) {
		evtch_to_pirq_eoi[evtch] = pirq;
#ifdef IRQ_DEBUG
		printf("pirq %d needs notify\n", pirq);
#endif
	}
}

void
hypervisor_ack_pirq_event(unsigned int evtch)
{
#ifdef IRQ_DEBUG
	if (evtch == IRQ_DEBUG)
		printf("%s: evtch %d\n", __func__, evtch);
#endif

	if (evtch_to_pirq_eoi[evtch] > 0) {
		struct physdev_eoi eoi;
		eoi.irq = evtch_to_pirq_eoi[evtch];
#ifdef  IRQ_DEBUG
		if (evtch == IRQ_DEBUG)
		    printf("pirq_notify(%d)\n", evtch);
#endif
		(void)HYPERVISOR_physdev_op(PHYSDEVOP_eoi, &eoi);
	}
}
#endif /* defined(XENPV) && (NPCI > 0 || NISA > 0) */

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

static struct evtsource *
event_get_handler(const char *intrid)
{
	for (int i = 0; i < NR_EVENT_CHANNELS; i++) {
		if (evtsource[i] == NULL || i == debug_port)
			continue;

		struct evtsource *evp = evtsource[i];

		if (strcmp(evp->ev_intrname, intrid) == 0)
			return evp;
	}

	return NULL;
}

static uint64_t
xen_intr_get_count(const char *intrid, u_int cpu_idx)
{
	int count = 0;
	struct evtsource *evp;

	mutex_spin_enter(&evtchn_lock);

	evp = event_get_handler(intrid);
	if (evp != NULL && cpu_idx == cpu_index(evp->ev_cpu))
		count = evp->ev_evcnt.ev_count;

	mutex_spin_exit(&evtchn_lock);

	return count;
}

static void
xen_intr_get_assigned(const char *intrid, kcpuset_t *cpuset)
{
	struct evtsource *evp;

	kcpuset_zero(cpuset);

	mutex_spin_enter(&evtchn_lock);

	evp = event_get_handler(intrid);
	if (evp != NULL)
		kcpuset_set(cpuset, cpu_index(evp->ev_cpu));

	mutex_spin_exit(&evtchn_lock);
}

static void
xen_intr_get_devname(const char *intrid, char *buf, size_t len)
{
	struct evtsource *evp;

	mutex_spin_enter(&evtchn_lock);

	evp = event_get_handler(intrid);
	strlcpy(buf, evp ? evp->ev_xname : "unknown", len);

	mutex_spin_exit(&evtchn_lock);
}

#ifdef XENPV
/*
 * MI interface for subr_interrupt.
 */
struct intrids_handler *
interrupt_construct_intrids(const kcpuset_t *cpuset)
{
	struct intrids_handler *ii_handler;
	intrid_t *ids;
	int i, count, off;
	struct evtsource *evp;

	if (kcpuset_iszero(cpuset))
		return 0;

	/*
	 * Count the number of interrupts which affinity to any cpu of "cpuset".
	 */
	count = 0;
	for (i = 0; i < NR_EVENT_CHANNELS; i++) {
		evp = evtsource[i];

		if (evp == NULL || i == debug_port)
			continue;

		if (!kcpuset_isset(cpuset, cpu_index(evp->ev_cpu)))
			continue;

		count++;
	}

	ii_handler = kmem_zalloc(sizeof(int) + sizeof(intrid_t) * count,
	    KM_SLEEP);
	if (ii_handler == NULL)
		return NULL;
	ii_handler->iih_nids = count;
	if (count == 0)
		return ii_handler;

	ids = ii_handler->iih_intrids;
	mutex_spin_enter(&evtchn_lock);
	for (i = 0, off = 0; i < NR_EVENT_CHANNELS && off < count; i++) {
		evp = evtsource[i];

		if (evp == NULL || i == debug_port)
			continue;

		if (!kcpuset_isset(cpuset, cpu_index(evp->ev_cpu)))
			continue;

		snprintf(ids[off], sizeof(intrid_t), "%s", evp->ev_intrname);
		off++;
	}
	mutex_spin_exit(&evtchn_lock);
	return ii_handler;
}
__strong_alias(interrupt_get_count, xen_intr_get_count);
__strong_alias(interrupt_get_assigned, xen_intr_get_assigned);
__strong_alias(interrupt_get_devname, xen_intr_get_devname);
__strong_alias(x86_intr_get_count, xen_intr_get_count);
__strong_alias(x86_intr_get_assigned, xen_intr_get_assigned);
__strong_alias(x86_intr_get_devname, xen_intr_get_devname);
#endif /* XENPV */
