/*	$NetBSD: evtchn.c,v 1.25.4.1 2007/12/13 05:05:24 yamt Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: evtchn.c,v 1.25.4.1 2007/12/13 05:05:24 yamt Exp $");

#include "opt_xen.h"
#include "isa.h"
#include "pci.h"

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/reboot.h>

#include <uvm/uvm.h>

#include <machine/intrdefs.h>

#include <xen/xen.h>
#include <xen/hypervisor.h>
#include <xen/evtchn.h>
#ifndef XEN3
#include <xen/ctrl_if.h>
#endif
#include <xen/xenfunc.h>

/*
 * This lock protects updates to the following mapping and reference-count
 * arrays. The lock does not need to be acquired to read the mapping tables.
 */
static struct simplelock irq_mapping_update_lock = SIMPLELOCK_INITIALIZER;

/* event handlers */
struct evtsource *evtsource[NR_EVENT_CHANNELS];

/* Reference counts for bindings to event channels */
static u_int8_t evtch_bindcount[NR_EVENT_CHANNELS];

/* event-channel <-> VIRQ mapping. */
static int virq_to_evtch[NR_VIRQS];


#if NPCI > 0 || NISA > 0
/* event-channel <-> PIRQ mapping */
static int pirq_to_evtch[NR_PIRQS];
/* PIRQ needing notify */
static u_int32_t pirq_needs_unmask_notify[NR_EVENT_CHANNELS / 32];
int pirq_interrupt(void *);
physdev_op_t physdev_op_notify = {
	.cmd = PHYSDEVOP_IRQ_UNMASK_NOTIFY,
};
#endif

int debug_port;
#ifndef XEN3
static int xen_misdirect_handler(void *);
#endif

// #define IRQ_DEBUG 4

void
events_default_setup()
{
	int i;

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
init_events()
{
#ifndef XEN3
	int evtch;

	/* no debug port, it doesn't work any more for some reason ... */
	debug_port = -1;

	evtch = bind_virq_to_evtch(VIRQ_MISDIRECT);
	aprint_verbose("misdirect virtual interrupt using event channel %d\n",
	    evtch);
	event_set_handler(evtch, &xen_misdirect_handler, NULL, IPL_HIGH,
	    "misdirev");
	hypervisor_enable_event(evtch);

	/* This needs to be done early, but after the IRQ subsystem is
	 * alive. */
	ctrl_if_init();
#endif
	debug_port = bind_virq_to_evtch(VIRQ_DEBUG);
	aprint_verbose("debug virtual interrupt using event channel %d\n",
	    debug_port);
	/*
	 * Don't call event_set_handler(), we'll use a shortcut. Just set
	 * evtsource[] to a non-NULL value so that evtchn_do_event will
	 * be called.
	 */
	evtsource[debug_port] = (void *)-1;
	hypervisor_enable_event(debug_port);

	x86_enable_intr();		/* at long last... */
}

unsigned int
evtchn_do_event(int evtch, struct intrframe *regs)
{
	struct cpu_info *ci;
	int ilevel;
	struct intrhand *ih;
	int	(*ih_fun)(void *, void *);
	extern struct uvmexp uvmexp;
	u_int32_t iplmask;

#ifdef DIAGNOSTIC
	if (evtch >= NR_EVENT_CHANNELS) {
		printf("event number %d > NR_IRQS\n", evtch);
		panic("evtchn_do_event");
	}
#endif

#ifdef IRQ_DEBUG
	if (evtch == IRQ_DEBUG)
		printf("evtchn_do_event: evtch %d\n", evtch);
#endif
	ci = &cpu_info_primary;

	/*
	 * Shortcut for the debug handler, we want it to always run,
	 * regardless of the IPL level.
	 */
	if (evtch == debug_port) {
		xen_debug_handler(NULL);
		hypervisor_enable_event(evtch);
		return 0;
	}

#ifdef DIAGNOSTIC
	if (evtsource[evtch] == NULL) {
		panic("evtchn_do_event: unknown event");
	}
#endif
	uvmexp.intrs++;
	evtsource[evtch]->ev_evcnt.ev_count++;
	ilevel = ci->ci_ilevel;
	if (evtsource[evtch]->ev_maxlevel <= ilevel) {
#ifdef IRQ_DEBUG
		if (evtch == IRQ_DEBUG)
		    printf("evtsource[%d]->ev_maxlevel %d <= ilevel %d\n",
		    evtch, evtsource[evtch]->ev_maxlevel, ilevel);
#endif
		hypervisor_set_ipending(evtsource[evtch]->ev_imask,
		    evtch / 32, evtch % 32);
		/* leave masked */
		return 0;
	}
	ci->ci_ilevel = evtsource[evtch]->ev_maxlevel;
	iplmask = evtsource[evtch]->ev_imask;
	sti();
#ifdef MULTIPROCESSOR
	x86_intlock(regs);
#endif
	ih = evtsource[evtch]->ev_handlers;
	while (ih != NULL) {
		if (ih->ih_level <= ilevel) {
#ifdef IRQ_DEBUG
		if (evtch == IRQ_DEBUG)
		    printf("ih->ih_level %d <= ilevel %d\n", ih->ih_level, ilevel);
#endif
#ifdef MULTIPROCESSOR
			x86_intunlock(regs);
#endif
			cli();
			hypervisor_set_ipending(iplmask,
			    evtch / 32, evtch % 32);
			/* leave masked */
			splx(ilevel);
			return 0;
		}
		iplmask &= ~IUNMASK(ci, ih->ih_level);
		ci->ci_ilevel = ih->ih_level;
		ih_fun = (void *)ih->ih_fun;
		ih_fun(ih->ih_arg, regs);
		ih = ih->ih_evt_next;
	}
	cli();
#ifdef MULTIPROCESSOR
	x86_intunlock(regs);
#endif
	hypervisor_enable_event(evtch);
	splx(ilevel);

	return 0;
}

int
bind_virq_to_evtch(int virq)
{
	evtchn_op_t op;
	int evtchn, s;

	s = splhigh();
	simple_lock(&irq_mapping_update_lock);

	evtchn = virq_to_evtch[virq];
	if (evtchn == -1) {
		op.cmd = EVTCHNOP_bind_virq;
		op.u.bind_virq.virq = virq;
#ifdef XEN3
		op.u.bind_virq.vcpu = 0;
#endif
		if (HYPERVISOR_event_channel_op(&op) != 0)
			panic("Failed to bind virtual IRQ %d\n", virq);
		evtchn = op.u.bind_virq.port;

		virq_to_evtch[virq] = evtchn;
	}

	evtch_bindcount[evtchn]++;

	simple_unlock(&irq_mapping_update_lock);
	splx(s);
    
	return evtchn;
}

void
unbind_virq_from_evtch(int virq)
{
	evtchn_op_t op;
	int evtchn = virq_to_evtch[virq];
	int s = splhigh();

	simple_lock(&irq_mapping_update_lock);

	evtch_bindcount[evtchn]--;
	if (evtch_bindcount[evtchn] == 0) {
		op.cmd = EVTCHNOP_close;
#ifndef XEN3
		op.u.close.dom = DOMID_SELF;
#endif
		op.u.close.port = evtchn;
		if (HYPERVISOR_event_channel_op(&op) != 0)
			panic("Failed to unbind virtual IRQ %d\n", virq);

		virq_to_evtch[virq] = -1;
	}

	simple_unlock(&irq_mapping_update_lock);
	splx(s);
}

#if NPCI > 0 || NISA > 0
int
bind_pirq_to_evtch(int pirq)
{
	evtchn_op_t op;
	int evtchn, s;

	if (pirq >= NR_PIRQS) {
		panic("pirq %d out of bound, increase NR_PIRQS", pirq);
	}

	s = splhigh();
	simple_lock(&irq_mapping_update_lock);

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

	simple_unlock(&irq_mapping_update_lock);
	splx(s);
    
	return evtchn;
}

void
unbind_pirq_from_evtch(int pirq)
{
	evtchn_op_t op;
	int evtchn = pirq_to_evtch[pirq];
	int s = splhigh();

	simple_lock(&irq_mapping_update_lock);

	evtch_bindcount[evtchn]--;
	if (evtch_bindcount[evtchn] == 0) {
		op.cmd = EVTCHNOP_close;
#ifndef XEN3
		op.u.close.dom = DOMID_SELF;
#endif
		op.u.close.port = evtchn;
		if (HYPERVISOR_event_channel_op(&op) != 0)
			panic("Failed to unbind physical IRQ %d\n", pirq);

		pirq_to_evtch[pirq] = -1;
	}

	simple_unlock(&irq_mapping_update_lock);
	splx(s);
}

struct pintrhand *
pirq_establish(int pirq, int evtch, int (*func)(void *), void *arg, int level,
    const char *evname)
{
	struct pintrhand *ih;
	physdev_op_t physdev_op;

	ih = malloc(sizeof *ih, M_DEVBUF, cold ? M_NOWAIT : M_WAITOK);
	if (ih == NULL) {
		printf("pirq_establish: can't malloc handler info\n");
		return NULL;
	}
	if (event_set_handler(evtch, pirq_interrupt, ih, level, evname) != 0) {
		free(ih, M_DEVBUF);
		return NULL;
	}
	ih->pirq = pirq;
	ih->evtch = evtch;
	ih->func = func;
	ih->arg = arg;

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

int
event_set_handler(int evtch, int (*func)(void *), void *arg, int level,
    const char *evname)
{
	struct iplsource *ipls;
	struct evtsource *evts;
	struct intrhand *ih, **ihp;
	struct cpu_info *ci;
	int s;

#ifdef IRQ_DEBUG
	printf("event_set_handler IRQ %d handler %p\n", evtch, func);
#endif

#ifdef DIAGNOSTIC
	if (evtch >= NR_EVENT_CHANNELS) {
		printf("evtch number %d > NR_EVENT_CHANNELS\n", evtch);
		panic("event_set_handler");
	}
#endif

#if 0
	printf("event_set_handler evtch %d handler %p level %d\n", evtch,
	       handler, level);
#endif
	MALLOC(ih, struct intrhand *, sizeof (struct intrhand), M_DEVBUF,
	    M_WAITOK|M_ZERO);
	if (ih == NULL)
		panic("can't allocate fixed interrupt source");


	ih->ih_level = level;
	ih->ih_fun = func;
	ih->ih_arg = arg;
	ih->ih_evt_next = NULL;
	ih->ih_ipl_next = NULL;

	ci = &cpu_info_primary;
	s = splhigh();
	if (ci->ci_isources[level] == NULL) {
		MALLOC(ipls, struct iplsource *, sizeof (struct iplsource),
		    M_DEVBUF, M_WAITOK|M_ZERO);
		if (ipls == NULL)
			panic("can't allocate fixed interrupt source");
		ipls->ipl_recurse = xenev_stubs[level].ist_recurse;
		ipls->ipl_resume = xenev_stubs[level].ist_resume;
		ipls->ipl_handlers = ih;
		ci->ci_isources[level] = ipls;
	} else {
		ipls = ci->ci_isources[level];
		ih->ih_ipl_next = ipls->ipl_handlers;
		ipls->ipl_handlers = ih;
	}
	if (evtsource[evtch] == NULL) {
		MALLOC(evts, struct evtsource *, sizeof (struct evtsource),
		    M_DEVBUF, M_WAITOK|M_ZERO);
		if (evts == NULL)
			panic("can't allocate fixed interrupt source");
		evts->ev_handlers = ih;
		evtsource[evtch] = evts;
		if (evname)
			strncpy(evts->ev_evname, evname,
			    sizeof(evts->ev_evname));
		else
			snprintf(evts->ev_evname, sizeof(evts->ev_evname),
			    "evt%d", evtch);
		evcnt_attach_dynamic(&evts->ev_evcnt, EVCNT_TYPE_INTR, NULL,
		    ci->ci_dev->dv_xname, evts->ev_evname);
	} else {
		evts = evtsource[evtch];
		/* sort by IPL order, higher first */
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
	}

	intr_calculatemasks(evts);
	splx(s);

	return 0;
}

int
event_remove_handler(int evtch, int (*func)(void *), void *arg)
{
	struct iplsource *ipls;
	struct evtsource *evts;
	struct intrhand *ih;
	struct intrhand **ihp;
	struct cpu_info *ci = &cpu_info_primary;

	evts = evtsource[evtch];
	if (evts == NULL)
		return ENOENT;

	for (ihp = &evts->ev_handlers, ih = evts->ev_handlers;
	    ih != NULL;
	    ihp = &ih->ih_evt_next, ih = ih->ih_evt_next) {
		if (ih->ih_fun == func && ih->ih_arg == arg)
			break;
	}
	if (ih == NULL)
		return ENOENT;
	*ihp = ih->ih_evt_next;

	ipls = ci->ci_isources[ih->ih_level];
	for (ihp = &ipls->ipl_handlers, ih = ipls->ipl_handlers;
	    ih != NULL;
	    ihp = &ih->ih_ipl_next, ih = ih->ih_ipl_next) {
		if (ih->ih_fun == func && ih->ih_arg == arg)
			break;
	}
	if (ih == NULL)
		panic("event_remove_handler");
	*ihp = ih->ih_ipl_next;
	FREE(ih, M_DEVBUF);
	if (evts->ev_handlers == NULL) {
		evcnt_detach(&evts->ev_evcnt);
		FREE(evts, M_DEVBUF);
		evtsource[evtch] = NULL;
	} else {
		intr_calculatemasks(evts);
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
	u_long upcall_pending =
	    HYPERVISOR_shared_info->vcpu_info[0].evtchn_upcall_pending;
	u_long upcall_mask =
	    HYPERVISOR_shared_info->vcpu_info[0].evtchn_upcall_mask;
#ifdef XEN3
	u_long pending_sel = HYPERVISOR_shared_info->vcpu_info[0].evtchn_pending_sel;
#else
	u_long pending_sel = HYPERVISOR_shared_info->evtchn_pending_sel;
#endif
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
	for (i = 0 ; i < 32; i++)
		printf(" %lx", (u_long)evtchn_mask[i]);
	printf("\n");
	printf("evtchn_pending");
	for (i = 0 ; i < 32; i++)
		printf(" %lx", (u_long)evtchn_pending[i]);
	printf("\n");
	return 0;
}

#ifndef XEN3
static int
xen_misdirect_handler(void *arg)
{
#if 0
	char *msg = "misdirect\n";
	(void)HYPERVISOR_console_io(CONSOLEIO_write, strlen(msg), msg);
#endif
	return 0;
}
#endif
