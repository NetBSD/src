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
__KERNEL_RCSID(0, "$NetBSD: intr.c,v 1.17.10.3 2009/06/20 07:20:13 yamt Exp $");

#include "opt_multiprocessor.h"
#include "opt_xen.h"
#include "isa.h"
#include "pci.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/cpu.h>

#include <uvm/uvm_extern.h>

#include <machine/i8259.h>
#include <machine/pio.h>
#include <xen/evtchn.h>

#ifdef XEN3
#include "acpi.h"
#include "ioapic.h"
#include "opt_mpbios.h"
/* for x86/i8259.c */
struct intrstub i8259_stubs[NUM_LEGACY_IRQS] = {{0,0}};
#if NIOAPIC > 0
/* for x86/ioapic.c */
struct intrstub ioapic_edge_stubs[MAX_INTR_SOURCES] = {{0,0}};
struct intrstub ioapic_level_stubs[MAX_INTR_SOURCES] = {{0,0}};

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

/*
 * Initialize all handlers that aren't dynamically allocated, and exist
 * for each CPU. Also init ci_iunmask[].
 */
void
cpu_intr_init(struct cpu_info *ci)
{
	int i;
#if defined(INTRSTACKSIZE)
	char *cp;
#endif

	ci->ci_iunmask[0] = 0xfffffffe;
	for (i = 1; i < NIPL; i++)
		ci->ci_iunmask[i] = ci->ci_iunmask[i - 1] & ~(1 << i);

#if defined(INTRSTACKSIZE)
	cp = (char *)uvm_km_alloc(kernel_map, INTRSTACKSIZE, 0, UVM_KMF_WIRED);
	ci->ci_intrstack = cp + INTRSTACKSIZE - sizeof(register_t);
#endif
	ci->ci_idepth = -1;
}

#if NPCI > 0 || NISA > 0
void *
intr_establish(int legacy_irq, struct pic *pic, int pin,
    int type, int level, int (*handler)(void *) , void *arg,
    bool known_mpsafe)
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
	 *
	 * Xen 3.5 also rejects 200. Try out all values until Xen accepts
	 * or none is available.
	 */
	static int xen_next_irq = 200;
	struct ioapic_softc *ioapic = ioapic_find(APIC_IRQ_APIC(*pirq));
	struct pic *pic = &ioapic->sc_pic;
	int pin = APIC_IRQ_PIN(*pirq);
	physdev_op_t op;

	if (*pirq & APIC_INT_VIA_APIC) {
		irq = vect2irq[ioapic->sc_pins[pin].ip_vector];
		if (ioapic->sc_pins[pin].ip_vector == 0 || irq == 0) {
			/* allocate IRQ */
			irq = APIC_IRQ_LEGACY_IRQ(*pirq);
			if (irq <= 0 || irq > 15)
				irq = xen_next_irq--;
retry:
			/* allocate vector and route interrupt */
			op.cmd = PHYSDEVOP_ASSIGN_VECTOR;
			op.u.irq_op.irq = irq;
			if (HYPERVISOR_physdev_op(&op) < 0) {
				irq = xen_next_irq--;
				if (xen_next_irq == 15)
					panic("PHYSDEVOP_ASSIGN_VECTOR irq %d", irq);
				goto retry;
			}
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
	struct ioapic_softc *pic;

	pic = ioapic_find_bybase(num);
	if (pic != NULL)
		return &pic->sc_pic;
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
		printf("%s: interrupt masks:\n", device_xname(ci->ci_dev));
		for (i = 0; i < NIPL; i++)
			printf("IPL %d mask %lx unmask %lx\n", i,
			    (u_long)ci->ci_imask[i], (u_long)ci->ci_iunmask[i]);
		for (i = 0; i < MAX_INTR_SOURCES; i++) {
			isp = ci->ci_isources[i];
			if (isp == NULL)
				continue;
			printf("%s source %d is pin %d from pic %s maxlevel %d\n",
			    device_xname(ci->ci_dev), i, isp->is_pin,
			    isp->is_pic->pic_name, isp->is_maxlevel);
			for (ih = isp->is_handlers; ih != NULL;
			     ih = ih->ih_next)
				printf("\thandler %p level %d\n",
				    ih->ih_fun, ih->ih_level);

		}
	}
}
#endif

void
cpu_intr_redistribute(void)
{

	/* XXX nothing */
}

u_int
cpu_intr_count(struct cpu_info *ci)
{

	KASSERT(ci->ci_nintrhand >= 0);

	return ci->ci_nintrhand;
}
