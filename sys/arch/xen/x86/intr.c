/*	NetBSD: intr.c,v 1.15 2004/04/10 14:49:55 kochi Exp 	*/

/*
 * Copyright 2002 (c) Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Frank van der Linden for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*-
 * Copyright (c) 1991 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *     @(#)isa.c       7.2 (Berkeley) 5/13/91
 */
/*-
 * Copyright (c) 1993, 1994 Charles Hannum.
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
 *     This product includes software developed by the University of
 *     California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *     @(#)isa.c       7.2 (Berkeley) 5/13/91
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: intr.c,v 1.11 2006/10/15 13:31:18 yamt Exp $");

#include "opt_multiprocessor.h"
#include "opt_xen.h"
#include "isa.h"
#include "pci.h"

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/errno.h>

#include <machine/atomic.h>
#include <machine/i8259.h>
#include <machine/cpu.h>
#include <machine/pio.h>
#include <machine/evtchn.h>

#ifdef XEN3
#include "acpi.h"
#include "ioapic.h"
#include "opt_mpbios.h"
/* for x86/i8259.c */
struct intrstub i8259_stubs[NUM_LEGACY_IRQS] = {{0}};
#if NIOAPIC > 0
/* for x86/ioapic.c */
struct intrstub ioapic_edge_stubs[MAX_INTR_SOURCES] = {{0}};
struct intrstub ioapic_level_stubs[MAX_INTR_SOURCES] = {{0}};

#include <machine/i82093var.h>
int irq2vect[256] = {0};
int vect2irq[256] = {0};
#endif /* NIOAPIC */
#if NACPI > 0
#include <machine/mpconfig.h>
#include <machine/mpacpi.h>
#endif
#ifdef MPBIOS
#include <machine/mpbiosvar.h>
#endif

#if NPCI > 0
#include <dev/pci/ppbreg.h>
#endif

#endif /* XEN3 */

/*
 * Recalculate the interrupt from scratch for an event source.
 */
void
intr_calculatemasks(struct evtsource *evts)
{
	struct intrhand *ih;

	evts->ev_maxlevel = IPL_NONE;
	evts->ev_imask = 0;
	for (ih = evts->ev_handlers; ih != NULL; ih = ih->ih_evt_next) {
		if (ih->ih_level > evts->ev_maxlevel)
			evts->ev_maxlevel = ih->ih_level;
		evts->ev_imask |= (1 << ih->ih_level);
	}

}

/*
 * Fake interrupt handler structures for the benefit of symmetry with
 * other interrupt sources.
 */
struct intrhand fake_softclock_intrhand;
struct intrhand fake_softnet_intrhand;
struct intrhand fake_softserial_intrhand;
struct intrhand fake_timer_intrhand;
struct intrhand fake_ipi_intrhand;
#if defined(DOM0OPS)
struct intrhand fake_softxenevt_intrhand;

extern void Xsoftxenevt(void);
#endif

/*
 * Event counters for the software interrupts.
 */
struct evcnt softclock_evtcnt;
struct evcnt softnet_evtcnt;
struct evcnt softserial_evtcnt;
#if defined(DOM0OPS)
struct evcnt softxenevt_evtcnt;
#endif

/*
 * Initialize all handlers that aren't dynamically allocated, and exist
 * for each CPU. Also init ci_iunmask[].
 */
void
cpu_intr_init(struct cpu_info *ci)
{
	struct iplsource *ipl;
	int i;

	ci->ci_iunmask[0] = 0xfffffffe;
	for (i = 1; i < NIPL; i++)
		ci->ci_iunmask[i] = ci->ci_iunmask[i - 1] & ~(1 << i);

	MALLOC(ipl, struct iplsource *, sizeof (struct iplsource), M_DEVBUF,
	    M_WAITOK|M_ZERO);
	if (ipl == NULL)
		panic("can't allocate fixed interrupt source");
	ipl->ipl_recurse = Xsoftclock;
	ipl->ipl_resume = Xsoftclock;
	fake_softclock_intrhand.ih_level = IPL_SOFTCLOCK;
	ipl->ipl_handlers = &fake_softclock_intrhand;
	ci->ci_isources[SIR_CLOCK] = ipl;
	evcnt_attach_dynamic(&softclock_evtcnt, EVCNT_TYPE_INTR, NULL,
	    ci->ci_dev->dv_xname, "softclock");

	MALLOC(ipl, struct iplsource *, sizeof (struct iplsource), M_DEVBUF,
	    M_WAITOK|M_ZERO);
	if (ipl == NULL)
		panic("can't allocate fixed interrupt source");
	ipl->ipl_recurse = Xsoftnet;
	ipl->ipl_resume = Xsoftnet;
	fake_softnet_intrhand.ih_level = IPL_SOFTNET;
	ipl->ipl_handlers = &fake_softnet_intrhand;
	ci->ci_isources[SIR_NET] = ipl;
	evcnt_attach_dynamic(&softnet_evtcnt, EVCNT_TYPE_INTR, NULL,
	    ci->ci_dev->dv_xname, "softnet");

	MALLOC(ipl, struct iplsource *, sizeof (struct iplsource), M_DEVBUF,
	    M_WAITOK|M_ZERO);
	if (ipl == NULL)
		panic("can't allocate fixed interrupt source");
	ipl->ipl_recurse = Xsoftserial;
	ipl->ipl_resume = Xsoftserial;
	fake_softserial_intrhand.ih_level = IPL_SOFTSERIAL;
	ipl->ipl_handlers = &fake_softserial_intrhand;
	ci->ci_isources[SIR_SERIAL] = ipl;
	evcnt_attach_dynamic(&softserial_evtcnt, EVCNT_TYPE_INTR, NULL,
	    ci->ci_dev->dv_xname, "softserial");

#if defined(DOM0OPS)
	MALLOC(ipl, struct iplsource *, sizeof (struct iplsource), M_DEVBUF,
	    M_WAITOK|M_ZERO);
	if (ipl == NULL)
		panic("can't allocate fixed interrupt source");
	ipl->ipl_recurse = Xsoftxenevt;
	ipl->ipl_resume = Xsoftxenevt;
	fake_softxenevt_intrhand.ih_level = IPL_SOFTXENEVT;
	ipl->ipl_handlers = &fake_softxenevt_intrhand;
	ci->ci_isources[SIR_XENEVT] = ipl;
	evcnt_attach_dynamic(&softxenevt_evtcnt, EVCNT_TYPE_INTR, NULL,
	    ci->ci_dev->dv_xname, "xenevt");
#endif /* defined(DOM0OPS) */
}

#if NPCI > 0 || NISA > 0
void *
intr_establish(int legacy_irq, struct pic *pic, int pin,
    int type, int level, int (*handler)(void *) , void *arg)
{
	struct pintrhand *ih;
	int evtchn;
	char evname[16];
#ifdef XEN3
#ifdef DIAGNOSTIC
	if (legacy_irq != -1 && (legacy_irq < 0 || legacy_irq > 15))
		panic("intr_establish: bad legacy IRQ value");
	if (legacy_irq == -1 && pic == &i8259_pic)
		panic("intr_establish: non-legacy IRQ on i8259");
#endif /* DIAGNOSTIC */
	if (legacy_irq == -1) {
#if NIOAPIC > 0
		/* will do interrupts via I/O APIC */
		legacy_irq = APIC_INT_VIA_APIC;
		legacy_irq |= pic->pic_apicid << APIC_INT_APIC_SHIFT;
		legacy_irq |= pin << APIC_INT_PIN_SHIFT;
		snprintf(evname, sizeof(evname), "%s pin %d",
		    pic->pic_name, pin);
#else /* NIOAPIC */
		return NULL;
#endif /* NIOAPIC */
	} else
#endif /* XEN3 */
		snprintf(evname, sizeof(evname), "irq%d", legacy_irq);

	evtchn = xen_intr_map(&legacy_irq, type);
	ih = pirq_establish(legacy_irq & 0xff, evtchn, handler, arg, level,
	    evname);
	return ih;
}

int
xen_intr_map(int *pirq, int type)
{
	int irq = *pirq;
#ifdef XEN3
#if NIOAPIC > 0
	extern struct cpu_info phycpu_info_primary; /* XXX */
	/*
	 * The hypervisor has already allocated vectors and IRQs for the
	 * devices. Reusing the same IRQ doesn't work because as we bind
	 * them for each devices, we can't then change the route entry
	 * of the next device if this one used this IRQ. The easiest is
	 * to allocate IRQs top-down, starting with a high number.
	 * 250 and 230 have been tried, but got rejected by Xen.
	 */
	static int xen_next_irq = 200;
	struct ioapic_softc *ioapic = ioapic_find(APIC_IRQ_APIC(*pirq));
	struct pic *pic = (struct pic *)ioapic;
	int pin = APIC_IRQ_PIN(*pirq);
	physdev_op_t op;

	if (*pirq & APIC_INT_VIA_APIC) {
		irq = vect2irq[ioapic->sc_pins[pin].ip_vector];
		if (ioapic->sc_pins[pin].ip_vector == 0 || irq == 0) {
			/* allocate IRQ */
			irq = xen_next_irq--;
			/* allocate vector and route interrupt */
			op.cmd = PHYSDEVOP_ASSIGN_VECTOR;
			op.u.irq_op.irq = irq;
			if (HYPERVISOR_physdev_op(&op) < 0)
				panic("PHYSDEVOP_ASSIGN_VECTOR");
			irq2vect[irq] = op.u.irq_op.vector;
			vect2irq[op.u.irq_op.vector] = irq;
			pic->pic_addroute(pic, &phycpu_info_primary, pin,
			    op.u.irq_op.vector, type);
		}
		*pirq &= ~0xff;
		*pirq |= irq;
	}
#endif /* NIOAPIC */
#endif /* XEN3 */
	return bind_pirq_to_evtch(irq);
}

void
intr_disestablish(struct intrhand *ih)
{
	printf("intr_disestablish irq\n");
}

#if defined(MPBIOS) || NACPI > 0
struct pic *
intr_findpic(int num)
{
#if NIOAPIC > 0
	struct pic *pic;

	pic = (struct pic *)ioapic_find_bybase(num);
	if (pic != NULL)
		return pic;
#endif
	if (num < NUM_LEGACY_IRQS)
		return &i8259_pic;

	return NULL;
}
#endif

#if NIOAPIC > 0 || NACPI > 0
struct intr_extra_bus {
	int bus;
	pcitag_t *pci_bridge_tag;
	pci_chipset_tag_t pci_chipset_tag;
	LIST_ENTRY(intr_extra_bus) list;
};

LIST_HEAD(, intr_extra_bus) intr_extra_buses =
    LIST_HEAD_INITIALIZER(intr_extra_buses);

static int intr_scan_bus(int, int, struct xen_intr_handle *);

void
intr_add_pcibus(struct pcibus_attach_args *pba)
{
	struct intr_extra_bus *iebp;

	iebp = malloc(sizeof(struct intr_extra_bus), M_TEMP, M_WAITOK);
	iebp->bus = pba->pba_bus;
	iebp->pci_chipset_tag = pba->pba_pc;
	iebp->pci_bridge_tag = pba->pba_bridgetag;
	LIST_INSERT_HEAD(&intr_extra_buses, iebp, list);
}

static int
intr_find_pcibridge(int bus, pcitag_t *pci_bridge_tag,
		    pci_chipset_tag_t *pci_chipset_tag)
{
	struct intr_extra_bus *iebp;
	struct mp_bus *mpb;

	if (bus < 0)
		return ENOENT;

	if (bus < mp_nbus) {
		mpb = &mp_busses[bus];
		if (mpb->mb_pci_bridge_tag == NULL)
			return ENOENT;
		*pci_bridge_tag = *mpb->mb_pci_bridge_tag;
		*pci_chipset_tag = mpb->mb_pci_chipset_tag;
		return 0;
	}

	LIST_FOREACH(iebp, &intr_extra_buses, list) {
		if (iebp->bus == bus) {
			if (iebp->pci_bridge_tag == NULL)
				return ENOENT;
			*pci_bridge_tag = *iebp->pci_bridge_tag;
			*pci_chipset_tag = iebp->pci_chipset_tag;
			return 0;
		}
	}
	return ENOENT;
}

int
intr_find_mpmapping(int bus, int pin, struct xen_intr_handle *handle)
{
#if NPCI > 0
	int dev, func;
	pcitag_t pci_bridge_tag;
	pci_chipset_tag_t pci_chipset_tag;
#endif

#if NPCI > 0
	while (intr_scan_bus(bus, pin, handle) != 0) {
		if (intr_find_pcibridge(bus, &pci_bridge_tag,
		    &pci_chipset_tag) != 0)
			return ENOENT;
		dev = pin >> 2;
		pin = pin & 3;
		pin = PPB_INTERRUPT_SWIZZLE(pin + 1, dev) - 1;
		pci_decompose_tag(pci_chipset_tag, pci_bridge_tag, &bus,
		    &dev, &func);
		pin |= (dev << 2);
	}
	return 0;
#else
	return intr_scan_bus(bus, pin, handle);
#endif
}

static int
intr_scan_bus(int bus, int pin, struct xen_intr_handle *handle)
{
	struct mp_intr_map *mip, *intrs;

	if (bus < 0 || bus >= mp_nbus)
		return ENOENT;

	intrs = mp_busses[bus].mb_intrs;
	if (intrs == NULL)
		return ENOENT;

	for (mip = intrs; mip != NULL; mip = mip->next) {
		if (mip->bus_pin == pin) {
#if NACPI > 0
			if (mip->linkdev != NULL)
				if (mpacpi_findintr_linkdev(mip) != 0)
					continue;
#endif
			handle->pirq = mip->ioapic_ih;
			return 0;
		}
	}
	return ENOENT;
}
#endif /* NIOAPIC > 0 || NACPI > 0 */
#endif /* NPCI > 0 || NISA > 0 */


#ifdef INTRDEBUG
void
intr_printconfig(void)
{
	int i;
	struct intrhand *ih;
	struct intrsource *isp;
	struct cpu_info *ci;
	CPU_INFO_ITERATOR cii;

	for (CPU_INFO_FOREACH(cii, ci)) {
		printf("cpu%d: interrupt masks:\n", ci->ci_apicid);
		for (i = 0; i < NIPL; i++)
			printf("IPL %d mask %lx unmask %lx\n", i,
			    (u_long)ci->ci_imask[i], (u_long)ci->ci_iunmask[i]);
		simple_lock(&ci->ci_slock);
		for (i = 0; i < MAX_INTR_SOURCES; i++) {
			isp = ci->ci_isources[i];
			if (isp == NULL)
				continue;
			printf("cpu%u source %d is pin %d from pic %s maxlevel %d\n",
			    ci->ci_apicid, i, isp->is_pin,
			    isp->is_pic->pic_name, isp->is_maxlevel);
			for (ih = isp->is_handlers; ih != NULL;
			     ih = ih->ih_next)
				printf("\thandler %p level %d\n",
				    ih->ih_fun, ih->ih_level);

		}
		simple_unlock(&ci->ci_slock);
	}
}
#endif
