/*	$NetBSD: intr.c,v 1.116 2018/01/04 13:36:30 maxv Exp $	*/

/*-
 * Copyright (c) 2007, 2008, 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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
 *	@(#)isa.c	7.2 (Berkeley) 5/13/91
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
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
 *	@(#)isa.c	7.2 (Berkeley) 5/13/91
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: intr.c,v 1.116 2018/01/04 13:36:30 maxv Exp $");

#include "opt_intrdebug.h"
#include "opt_multiprocessor.h"
#include "opt_acpi.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/intr.h>
#include <sys/cpu.h>
#include <sys/atomic.h>
#include <sys/xcall.h>
#include <sys/interrupt.h>
#include <sys/reboot.h> /* for AB_VERBOSE */

#include <sys/kauth.h>
#include <sys/conf.h>

#include <uvm/uvm_extern.h>

#include <machine/i8259.h>
#include <machine/pio.h>

#include "ioapic.h"
#include "lapic.h"
#include "pci.h"
#include "acpica.h"

#if NIOAPIC > 0 || NACPICA > 0
#include <machine/i82093var.h> 
#include <machine/mpbiosvar.h>
#include <machine/mpacpi.h>
#endif

#if NLAPIC > 0
#include <machine/i82489var.h>
#endif

#if NPCI > 0
#include <dev/pci/ppbreg.h>
#endif

#include <x86/pci/msipic.h>
#include <x86/pci/pci_msi_machdep.h>

#if NPCI == 0
#define msipic_is_msi_pic(PIC)	(false)
#endif

#if defined(XEN) /* XXX: Cleanup */
#include <xen/xen.h>
#include <xen/hypervisor.h>
#include <xen/evtchn.h>
#include <xen/xenfunc.h>
#endif /* XEN */

#ifdef DDB
#include <ddb/db_output.h>
#endif

#ifdef INTRDEBUG
#define DPRINTF(msg) printf msg
#else
#define DPRINTF(msg)
#endif

struct pic softintr_pic = {
	.pic_name = "softintr_fakepic",
	.pic_type = PIC_SOFT,
	.pic_vecbase = 0,
	.pic_apicid = 0,
	.pic_lock = __SIMPLELOCK_UNLOCKED,
};

static void intr_calculatemasks(struct cpu_info *);

static SIMPLEQ_HEAD(, intrsource) io_interrupt_sources =
	SIMPLEQ_HEAD_INITIALIZER(io_interrupt_sources);

static kmutex_t intr_distribute_lock;

#if NIOAPIC > 0 || NACPICA > 0
static int intr_scan_bus(int, int, intr_handle_t *);
#if NPCI > 0
static int intr_find_pcibridge(int, pcitag_t *, pci_chipset_tag_t *);
#endif
#endif

#if !defined(XEN)
static int intr_allocate_slot_cpu(struct cpu_info *, struct pic *, int, int *,
				  struct intrsource *);
static int __noinline intr_allocate_slot(struct pic *, int, int,
					 struct cpu_info **, int *, int *,
					 struct intrsource *);

static void intr_source_free(struct cpu_info *, int, struct pic *, int);

static void intr_establish_xcall(void *, void *);
static void intr_disestablish_xcall(void *, void *);

static const char *legacy_intr_string(int, char *, size_t, struct pic *);
#if defined(XEN) /* XXX: nuke conditional after integration */
static const char *xen_intr_string(int, char *, size_t, struct pic *);
#endif /* XXX: XEN */
#endif

static inline bool redzone_const_or_false(bool);
static inline int redzone_const_or_zero(int);

static void intr_redistribute_xc_t(void *, void *);
static void intr_redistribute_xc_s1(void *, void *);
static void intr_redistribute_xc_s2(void *, void *);
static bool intr_redistribute(struct cpu_info *);

static struct intrsource *intr_get_io_intrsource(const char *);
static void intr_free_io_intrsource_direct(struct intrsource *);
#if !defined(XEN)
static int intr_num_handlers(struct intrsource *);

static int intr_find_unused_slot(struct cpu_info *, int *);
static void intr_activate_xcall(void *, void *);
static void intr_deactivate_xcall(void *, void *);
static void intr_get_affinity(struct intrsource *, kcpuset_t *);
static int intr_set_affinity(struct intrsource *, const kcpuset_t *);
#endif /* XEN */

/*
 * Fill in default interrupt table (in case of spurious interrupt
 * during configuration of kernel), setup interrupt control unit
 */
void
intr_default_setup(void)
{
#if !defined(XEN)
	int i;

	/* icu vectors */
	for (i = 0; i < NUM_LEGACY_IRQS; i++) {
		idt_vec_reserve(ICU_OFFSET + i);
		idt_vec_set(ICU_OFFSET + i, i8259_stubs[i].ist_entry);
	}

	/*
	 * Eventually might want to check if it's actually there.
	 */
	i8259_default_setup();

#else
	events_default_setup();
#endif /* !XEN */
	mutex_init(&intr_distribute_lock, MUTEX_DEFAULT, IPL_NONE);
}

/*
 * Handle a NMI, possibly a machine check.
 * return true to panic system, false to ignore.
 */
void
x86_nmi(void)
{

	log(LOG_CRIT, "NMI port 61 %x, port 70 %x\n", inb(0x61), inb(0x70));
}

/*
 * Recalculate the interrupt masks from scratch.
 * During early boot, anything goes and we are always called on the BP.
 * When the system is up and running:
 *
 * => called with ci == curcpu()
 * => cpu_lock held by the initiator
 * => interrupts disabled on-chip (PSL_I)
 *
 * Do not call printf(), kmem_free() or other "heavyweight" routines
 * from here.  This routine must be quick and must not block.
 */
static void
intr_calculatemasks(struct cpu_info *ci)
{
	int irq, level, unusedirqs, intrlevel[MAX_INTR_SOURCES];
	struct intrhand *q;

	/* First, figure out which levels each IRQ uses. */
	unusedirqs = 0xffffffff;
	for (irq = 0; irq < MAX_INTR_SOURCES; irq++) {
		int levels = 0;

		if (ci->ci_isources[irq] == NULL) {
			intrlevel[irq] = 0;
			continue;
		}
		for (q = ci->ci_isources[irq]->is_handlers; q; q = q->ih_next)
			levels |= 1 << q->ih_level;
		intrlevel[irq] = levels;
		if (levels)
			unusedirqs &= ~(1 << irq);
	}

	/* Then figure out which IRQs use each level. */
	for (level = 0; level < NIPL; level++) {
		int irqs = 0;
		for (irq = 0; irq < MAX_INTR_SOURCES; irq++)
			if (intrlevel[irq] & (1 << level))
				irqs |= 1 << irq;
		ci->ci_imask[level] = irqs | unusedirqs;
	}

	for (level = 0; level<(NIPL-1); level++)
		ci->ci_imask[level+1] |= ci->ci_imask[level];

	for (irq = 0; irq < MAX_INTR_SOURCES; irq++) {
		int maxlevel = IPL_NONE;
		int minlevel = IPL_HIGH;

		if (ci->ci_isources[irq] == NULL)
			continue;
		for (q = ci->ci_isources[irq]->is_handlers; q;
		     q = q->ih_next) {
			if (q->ih_level < minlevel)
				minlevel = q->ih_level;
			if (q->ih_level > maxlevel)
				maxlevel = q->ih_level;
		}
		ci->ci_isources[irq]->is_maxlevel = maxlevel;
		ci->ci_isources[irq]->is_minlevel = minlevel;
	}

	for (level = 0; level < NIPL; level++)
		ci->ci_iunmask[level] = ~ci->ci_imask[level];
}

/*
 * List to keep track of PCI buses that are probed but not known
 * to the firmware. Used to 
 *
 * XXX should maintain one list, not an array and a linked list.
 */
#if (NPCI > 0) && ((NIOAPIC > 0) || NACPICA > 0)
struct intr_extra_bus {
	int bus;
	pcitag_t *pci_bridge_tag;
	pci_chipset_tag_t pci_chipset_tag;
	LIST_ENTRY(intr_extra_bus) list;
};

LIST_HEAD(, intr_extra_bus) intr_extra_buses =
    LIST_HEAD_INITIALIZER(intr_extra_buses);


void
intr_add_pcibus(struct pcibus_attach_args *pba)
{
	struct intr_extra_bus *iebp;

	iebp = kmem_alloc(sizeof(*iebp), KM_SLEEP);
	iebp->bus = pba->pba_bus;
	iebp->pci_chipset_tag = pba->pba_pc;
	iebp->pci_bridge_tag = pba->pba_bridgetag;
	LIST_INSERT_HEAD(&intr_extra_buses, iebp, list);
}

static int
intr_find_pcibridge(int bus, pcitag_t *pci_bridge_tag,
		    pci_chipset_tag_t *pc)
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
		*pc = mpb->mb_pci_chipset_tag;
		return 0;
	}

	LIST_FOREACH(iebp, &intr_extra_buses, list) {
		if (iebp->bus == bus) {
			if (iebp->pci_bridge_tag == NULL)
				return ENOENT;
			*pci_bridge_tag = *iebp->pci_bridge_tag;
			*pc = iebp->pci_chipset_tag;
			return 0;
		}
	}
	return ENOENT;
}
#endif

#if NIOAPIC > 0 || NACPICA > 0
/*
 * 'pin' argument pci bus_pin encoding of a device/pin combination.
 */
int
intr_find_mpmapping(int bus, int pin, intr_handle_t *handle)
{

#if NPCI > 0
	while (intr_scan_bus(bus, pin, handle) != 0) {
		int dev, func;
		pcitag_t pci_bridge_tag;
		pci_chipset_tag_t pc;

		if (intr_find_pcibridge(bus, &pci_bridge_tag, &pc) != 0)
			return ENOENT;
		dev = pin >> 2;
		pin = pin & 3;
		pin = PPB_INTERRUPT_SWIZZLE(pin + 1, dev) - 1;
		pci_decompose_tag(pc, pci_bridge_tag, &bus, &dev, &func);
		pin |= (dev << 2);
	}
	return 0;
#else
	return intr_scan_bus(bus, pin, handle);
#endif
}

static int
intr_scan_bus(int bus, int pin, intr_handle_t *handle)
{
	struct mp_intr_map *mip, *intrs;

	if (bus < 0 || bus >= mp_nbus)
		return ENOENT;

	intrs = mp_busses[bus].mb_intrs;
	if (intrs == NULL)
		return ENOENT;

	for (mip = intrs; mip != NULL; mip = mip->next) {
		if (mip->bus_pin == pin) {
#if NACPICA > 0
			if (mip->linkdev != NULL)
				if (mpacpi_findintr_linkdev(mip) != 0)
					continue;
#endif
			*handle = mip->ioapic_ih;
			return 0;
		}
	}
	return ENOENT;
}
#endif

#if !defined(XEN)
/*
 * Create an interrupt id such as "ioapic0 pin 9". This interrupt id is used
 * by MI code and intrctl(8).
 */
const char *
intr_create_intrid(int legacy_irq, struct pic *pic, int pin, char *buf, size_t len)
{
	int ih = 0;

#if NPCI > 0
	if ((pic->pic_type == PIC_MSI) || (pic->pic_type == PIC_MSIX)) {
		uint64_t pih;
		int dev, vec;

		dev = msipic_get_devid(pic);
		vec = pin;
		pih = __SHIFTIN((uint64_t)dev, MSI_INT_DEV_MASK)
			| __SHIFTIN((uint64_t)vec, MSI_INT_VEC_MASK)
			| APIC_INT_VIA_MSI;
		if (pic->pic_type == PIC_MSI)
			MSI_INT_MAKE_MSI(pih);
		else if (pic->pic_type == PIC_MSIX)
			MSI_INT_MAKE_MSIX(pih);

		return x86_pci_msi_string(NULL, pih, buf, len);
	}
#endif

#if defined(XEN)
	evtchn_port_t port = pin; /* Port number */

	if (pic->pic_type == PIC_XEN) {
		ih = pin;
		return xen_intr_string(port, buf, len, pic);
	}
#endif

	/*
	 * If the device is pci, "legacy_irq" is alway -1. Least 8 bit of "ih"
	 * is only used in intr_string() to show the irq number.
	 * If the device is "legacy"(such as floppy), it should not use
	 * intr_string().
	 */
	if (pic->pic_type == PIC_I8259) {
		ih = legacy_irq;
		return legacy_intr_string(ih, buf, len, pic);
	}

#if NIOAPIC > 0 || NACPICA > 0
	ih = ((pic->pic_apicid << APIC_INT_APIC_SHIFT) & APIC_INT_APIC_MASK)
	    | ((pin << APIC_INT_PIN_SHIFT) & APIC_INT_PIN_MASK);
	if (pic->pic_type == PIC_IOAPIC) {
		ih |= APIC_INT_VIA_APIC;
	}
	ih |= pin;
	return intr_string(ih, buf, len);
#endif

	return NULL; /* No pic found! */
}

#endif /* XEN */

/*
 * Find intrsource from io_interrupt_sources list.
 */
static struct intrsource *
intr_get_io_intrsource(const char *intrid)
{
	struct intrsource *isp;

	KASSERT(mutex_owned(&cpu_lock));

	SIMPLEQ_FOREACH(isp, &io_interrupt_sources, is_list) {
		KASSERT(isp->is_intrid != NULL);
		if (strncmp(intrid, isp->is_intrid, INTRIDBUF - 1) == 0)
			return isp;
	}
	return NULL;
}

/*
 * Allocate intrsource and add to io_interrupt_sources list.
 */
struct intrsource *
intr_allocate_io_intrsource(const char *intrid)
{
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;
	struct intrsource *isp;
	struct percpu_evcnt *pep;

	KASSERT(mutex_owned(&cpu_lock));

	if (intrid == NULL)
		return NULL;

	isp = kmem_zalloc(sizeof(*isp), KM_SLEEP);
	pep = kmem_zalloc(sizeof(*pep) * ncpu, KM_SLEEP);
	isp->is_saved_evcnt = pep;
	for (CPU_INFO_FOREACH(cii, ci)) {
		pep->cpuid = ci->ci_cpuid;
		pep++;
	}
	strlcpy(isp->is_intrid, intrid, sizeof(isp->is_intrid));

	SIMPLEQ_INSERT_TAIL(&io_interrupt_sources, isp, is_list);

	return isp;
}

/*
 * Remove from io_interrupt_sources list and free by the intrsource pointer.
 */
static void
intr_free_io_intrsource_direct(struct intrsource *isp)
{
	KASSERT(mutex_owned(&cpu_lock));

	SIMPLEQ_REMOVE(&io_interrupt_sources, isp, intrsource, is_list);

	/* Is this interrupt established? */
	if (isp->is_evname[0] != '\0')
		evcnt_detach(&isp->is_evcnt);

	kmem_free(isp->is_saved_evcnt,
	    sizeof(*(isp->is_saved_evcnt)) * ncpu);

	kmem_free(isp, sizeof(*isp));
}

/*
 * Remove from io_interrupt_sources list and free by the interrupt id.
 * This function can be used by MI code.
 */
void
intr_free_io_intrsource(const char *intrid)
{
	struct intrsource *isp;

	KASSERT(mutex_owned(&cpu_lock));

	if (intrid == NULL)
		return;

	if ((isp = intr_get_io_intrsource(intrid)) == NULL) {
		return;
	}

	/* If the interrupt uses shared IRQ, don't free yet. */
	if (isp->is_handlers != NULL) {
		return;
	}

	intr_free_io_intrsource_direct(isp);
}

#if !defined(XEN)
static int
intr_allocate_slot_cpu(struct cpu_info *ci, struct pic *pic, int pin,
		       int *index, struct intrsource *chained)
{
	int slot, i;
	struct intrsource *isp;

	KASSERT(mutex_owned(&cpu_lock));

	if (pic == &i8259_pic) {
		KASSERT(CPU_IS_PRIMARY(ci));
		slot = pin;
	} else {
		int start = 0;
		slot = -1;

		/* avoid reserved slots for legacy interrupts. */
		if (CPU_IS_PRIMARY(ci) && msipic_is_msi_pic(pic))
			start = NUM_LEGACY_IRQS;
		/*
		 * intr_allocate_slot has checked for an existing mapping.
		 * Now look for a free slot.
		 */
		for (i = start; i < MAX_INTR_SOURCES ; i++) {
			if (ci->ci_isources[i] == NULL) {
				slot = i;
				break;
			}
		}
		if (slot == -1) {
			return EBUSY;
		}
	}

	isp = ci->ci_isources[slot];
	if (isp == NULL) {
		const char *via;

		isp = chained;
		KASSERT(isp != NULL);
		if (pic->pic_type == PIC_MSI || pic->pic_type == PIC_MSIX)
			via = "vec";
		else
			via = "pin";
		snprintf(isp->is_evname, sizeof (isp->is_evname),
		    "%s %d", via, pin);
		evcnt_attach_dynamic(&isp->is_evcnt, EVCNT_TYPE_INTR, NULL,
		    pic->pic_name, isp->is_evname);
		isp->is_active_cpu = ci->ci_cpuid;
		ci->ci_isources[slot] = isp;
	}

	*index = slot;
	return 0;
}

/*
 * A simple round-robin allocator to assign interrupts to CPUs.
 */
static int __noinline
intr_allocate_slot(struct pic *pic, int pin, int level,
		   struct cpu_info **cip, int *index, int *idt_slot,
		   struct intrsource *chained)
{
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci, *lci;
	struct intrsource *isp;
	int slot = 0, idtvec, error;

	KASSERT(mutex_owned(&cpu_lock));

	/* First check if this pin is already used by an interrupt vector. */
	for (CPU_INFO_FOREACH(cii, ci)) {
		for (slot = 0 ; slot < MAX_INTR_SOURCES ; slot++) {
			if ((isp = ci->ci_isources[slot]) == NULL) {
				continue;
			}
			if (isp->is_pic == pic &&
			    pin != -1 && isp->is_pin == pin) {
				*idt_slot = isp->is_idtvec;
				*index = slot;
				*cip = ci;
				return 0;
			}
		}
	}

	/*
	 * The pic/pin combination doesn't have an existing mapping.
	 * Find a slot for a new interrupt source.  For the i8259 case,
	 * we always use reserved slots of the primary CPU.  Otherwise,
	 * we make an attempt to balance the interrupt load.
	 *
	 * PIC and APIC usage are essentially exclusive, so the reservation
	 * of the ISA slots is ignored when assigning IOAPIC slots.
	 */
	if (pic == &i8259_pic) {
		/*
		 * Must be directed to BP.
		 */
		ci = &cpu_info_primary;
		error = intr_allocate_slot_cpu(ci, pic, pin, &slot, chained);
	} else {
		/*
		 * Find least loaded AP/BP and try to allocate there.
		 */
		ci = NULL;
		for (CPU_INFO_FOREACH(cii, lci)) {
			if ((lci->ci_schedstate.spc_flags & SPCF_NOINTR) != 0) {
				continue;
			}
#if 0
			if (ci == NULL ||
			    ci->ci_nintrhand > lci->ci_nintrhand) {
			    	ci = lci;
			}
#else
			ci = &cpu_info_primary;
#endif
		}
		KASSERT(ci != NULL);
		error = intr_allocate_slot_cpu(ci, pic, pin, &slot, chained);

		/*
		 * If that did not work, allocate anywhere.
		 */
		if (error != 0) {
			for (CPU_INFO_FOREACH(cii, ci)) {
				if ((ci->ci_schedstate.spc_flags &
				    SPCF_NOINTR) != 0) {
					continue;
				}
				error = intr_allocate_slot_cpu(ci, pic,
				    pin, &slot, chained);
				if (error == 0) {
					break;
				}
			}
		}
	}
	if (error != 0) {
		return error;
	}
	KASSERT(ci != NULL);

	/* 
	 * Now allocate an IDT vector.
	 * For the 8259 these are reserved up front.
	 */
	if (pic == &i8259_pic) {
		idtvec = ICU_OFFSET + pin;
	} else {
		idtvec = idt_vec_alloc(APIC_LEVEL(level), IDT_INTR_HIGH);
	}
	if (idtvec == 0) {
		evcnt_detach(&ci->ci_isources[slot]->is_evcnt);
		ci->ci_isources[slot] = NULL;
		return EBUSY;
	}
	ci->ci_isources[slot]->is_idtvec = idtvec;
	*idt_slot = idtvec;
	*index = slot;
	*cip = ci;
	return 0;
}

static void
intr_source_free(struct cpu_info *ci, int slot, struct pic *pic, int idtvec)
{
	struct intrsource *isp;

	isp = ci->ci_isources[slot];

	if (isp->is_handlers != NULL)
		return;
	ci->ci_isources[slot] = NULL;
	if (pic != &i8259_pic)
		idt_vec_free(idtvec);
}

#ifdef MULTIPROCESSOR
static int intr_biglock_wrapper(void *);

/*
 * intr_biglock_wrapper: grab biglock and call a real interrupt handler.
 */

static int
intr_biglock_wrapper(void *vp)
{
	struct intrhand *ih = vp;
	int ret;

	KERNEL_LOCK(1, NULL);

	ret = (*ih->ih_realfun)(ih->ih_realarg);

	KERNEL_UNLOCK_ONE(NULL);

	return ret;
}
#endif /* MULTIPROCESSOR */
#endif /* XEN */

#if defined(DOM0OPS) || !defined(XEN)
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
#if !defined(XEN)
/*
 * Append device name to intrsource. If device A and device B share IRQ number,
 * the device name of the interrupt id is "device A, device B".
 */
static void
intr_append_intrsource_xname(struct intrsource *isp, const char *xname)
{

	if (isp->is_xname[0] != '\0')
		strlcat(isp->is_xname, ", ", sizeof(isp->is_xname));
	strlcat(isp->is_xname, xname, sizeof(isp->is_xname));
}

/*
 * Handle per-CPU component of interrupt establish.
 *
 * => caller (on initiating CPU) holds cpu_lock on our behalf
 * => arg1: struct intrhand *ih
 * => arg2: int idt_vec
 */
static void
intr_establish_xcall(void *arg1, void *arg2)
{
	struct intrsource *source;
	struct intrstub *stubp;
	struct intrhand *ih;
	struct cpu_info *ci;
	int idt_vec;
	u_long psl;

	ih = arg1;

	KASSERT(ih->ih_cpu == curcpu() || !mp_online);

	ci = ih->ih_cpu;
	source = ci->ci_isources[ih->ih_slot];
	idt_vec = (int)(intptr_t)arg2;

	/* Disable interrupts locally. */
	psl = x86_read_psl();
	x86_disable_intr();

	/* Link in the handler and re-calculate masks. */
	*(ih->ih_prevp) = ih;
	intr_calculatemasks(ci);

	/* Hook in new IDT vector and SPL state. */
	if (source->is_resume == NULL || source->is_idtvec != idt_vec) {
		if (source->is_idtvec != 0 && source->is_idtvec != idt_vec)
			idt_vec_free(source->is_idtvec);
		source->is_idtvec = idt_vec;
		if (source->is_type == IST_LEVEL) {
			stubp = &source->is_pic->pic_level_stubs[ih->ih_slot];
		} else {
			stubp = &source->is_pic->pic_edge_stubs[ih->ih_slot];
		}
		source->is_resume = stubp->ist_resume;
		source->is_recurse = stubp->ist_recurse;
		idt_vec_set(idt_vec, stubp->ist_entry);
	}

	/* Re-enable interrupts locally. */
	x86_write_psl(psl);
}

void *
intr_establish_xname(int legacy_irq, struct pic *pic, int pin, int type,
		     int level, int (*handler)(void *), void *arg,
		     bool known_mpsafe, const char *xname)
{
	struct intrhand **p, *q, *ih;
	struct cpu_info *ci;
	int slot, error, idt_vec;
	struct intrsource *chained, *source;
#ifdef MULTIPROCESSOR
	bool mpsafe = (known_mpsafe || level != IPL_VM);
#endif /* MULTIPROCESSOR */
	uint64_t where;
	const char *intrstr;
	char intrstr_buf[INTRIDBUF];

	KASSERTMSG((legacy_irq == -1 || (0 <= legacy_irq && legacy_irq < 16)),
	    "bad legacy IRQ value: %d", legacy_irq);
	KASSERTMSG((legacy_irq != -1 || pic != &i8259_pic),
	    "non-legacy IRQ on i8259");

	ih = kmem_alloc(sizeof(*ih), KM_SLEEP);
	intrstr = intr_create_intrid(legacy_irq, pic, pin, intrstr_buf,
	    sizeof(intrstr_buf));
	KASSERT(intrstr != NULL);

	mutex_enter(&cpu_lock);

	/* allocate intrsource pool, if not yet. */
	chained = intr_get_io_intrsource(intrstr);
	if (chained == NULL) {
		if (msipic_is_msi_pic(pic)) {
			mutex_exit(&cpu_lock);
			kmem_free(ih, sizeof(*ih));
			printf("%s: %s has no intrsource\n", __func__, intrstr);
			return NULL;
		}
		chained = intr_allocate_io_intrsource(intrstr);
		if (chained == NULL) {
			mutex_exit(&cpu_lock);
			kmem_free(ih, sizeof(*ih));
			printf("%s: can't allocate io_intersource\n", __func__);
			return NULL;
		}
	}

	error = intr_allocate_slot(pic, pin, level, &ci, &slot, &idt_vec,
	    chained);
	if (error != 0) {
		intr_free_io_intrsource_direct(chained);
		mutex_exit(&cpu_lock);
		kmem_free(ih, sizeof(*ih));
		printf("failed to allocate interrupt slot for PIC %s pin %d\n",
		    pic->pic_name, pin);
		return NULL;
	}

	source = ci->ci_isources[slot];

	if (source->is_handlers != NULL &&
	    source->is_pic->pic_type != pic->pic_type) {
		intr_free_io_intrsource_direct(chained);
		mutex_exit(&cpu_lock);
		kmem_free(ih, sizeof(*ih));
		printf("%s: can't share intr source between "
		       "different PIC types (legacy_irq %d pin %d slot %d)\n",
		    __func__, legacy_irq, pin, slot);
		return NULL;
	}

	source->is_pin = pin;
	source->is_pic = pic;
	intr_append_intrsource_xname(source, xname);
	switch (source->is_type) {
	case IST_NONE:
		source->is_type = type;
		break;
	case IST_EDGE:
	case IST_LEVEL:
		if (source->is_type == type)
			break;
		/* FALLTHROUGH */
	case IST_PULSE:
		if (type != IST_NONE) {
			intr_source_free(ci, slot, pic, idt_vec);
			intr_free_io_intrsource_direct(chained);
			mutex_exit(&cpu_lock);
			kmem_free(ih, sizeof(*ih));
			printf("%s: pic %s pin %d: can't share "
			       "type %d with %d\n",
				__func__, pic->pic_name, pin,
				source->is_type, type);
			return NULL;
		}
		break;
	default:
		panic("%s: bad intr type %d for pic %s pin %d\n",
		    __func__, source->is_type, pic->pic_name, pin);
		/* NOTREACHED */
	}

        /*
	 * If the establishing interrupt uses shared IRQ, the interrupt uses
	 * "ci->ci_isources[slot]" instead of allocated by the establishing
	 * device's pci_intr_alloc() or this function.
	 */
	if (source->is_handlers != NULL) {
		struct intrsource *isp, *nisp;

		SIMPLEQ_FOREACH_SAFE(isp, &io_interrupt_sources,
		    is_list, nisp) {
			if (strncmp(intrstr, isp->is_intrid, INTRIDBUF - 1) == 0
			    && isp->is_handlers == NULL)
				intr_free_io_intrsource_direct(isp);
		}
	}

	/*
	 * We're now committed.  Mask the interrupt in hardware and
	 * count it for load distribution.
	 */
	(*pic->pic_hwmask)(pic, pin);
	(ci->ci_nintrhand)++;

	/*
	 * Figure out where to put the handler.
	 * This is O(N^2), but we want to preserve the order, and N is
	 * generally small.
	 */
	for (p = &ci->ci_isources[slot]->is_handlers;
	     (q = *p) != NULL && q->ih_level > level;
	     p = &q->ih_next) {
		/* nothing */;
	}

	ih->ih_fun = ih->ih_realfun = handler;
	ih->ih_arg = ih->ih_realarg = arg;
	ih->ih_prevp = p;
	ih->ih_next = *p;
	ih->ih_level = level;
	ih->ih_pin = pin;
	ih->ih_cpu = ci;
	ih->ih_slot = slot;
#ifdef MULTIPROCESSOR
	if (!mpsafe) {
		ih->ih_fun = intr_biglock_wrapper;
		ih->ih_arg = ih;
	}
#endif /* MULTIPROCESSOR */

	/*
	 * Call out to the remote CPU to update its interrupt state.
	 * Only make RPCs if the APs are up and running.
	 */
	if (ci == curcpu() || !mp_online) {
		intr_establish_xcall(ih, (void *)(intptr_t)idt_vec);
	} else {
		where = xc_unicast(0, intr_establish_xcall, ih,
		    (void *)(intptr_t)idt_vec, ci);
		xc_wait(where);
	}

	/* All set up, so add a route for the interrupt and unmask it. */
	(*pic->pic_addroute)(pic, ci, pin, idt_vec, type);
	(*pic->pic_hwunmask)(pic, pin);
	mutex_exit(&cpu_lock);

	if (bootverbose || cpu_index(ci) != 0)
		aprint_verbose("allocated pic %s type %s pin %d level %d to %s slot %d "
		    "idt entry %d\n",
		    pic->pic_name, type == IST_EDGE ? "edge" : "level", pin, level,
		    device_xname(ci->ci_dev), slot, idt_vec);

	return (ih);
}

void *
intr_establish(int legacy_irq, struct pic *pic, int pin, int type, int level,
	       int (*handler)(void *), void *arg, bool known_mpsafe)
{

	return intr_establish_xname(legacy_irq, pic, pin, type,
	    level, handler, arg, known_mpsafe, "unknown");
}

/*
 * Called on bound CPU to handle intr_disestablish().
 *
 * => caller (on initiating CPU) holds cpu_lock on our behalf
 * => arg1: struct intrhand *ih
 * => arg2: unused
 */
static void
intr_disestablish_xcall(void *arg1, void *arg2)
{
	struct intrhand **p, *q;
	struct cpu_info *ci;
	struct pic *pic;
	struct intrsource *source;
	struct intrhand *ih;
	u_long psl;
	int idtvec;

	ih = arg1;
	ci = ih->ih_cpu;

	KASSERT(ci == curcpu() || !mp_online);

	/* Disable interrupts locally. */
	psl = x86_read_psl();
	x86_disable_intr();

	pic = ci->ci_isources[ih->ih_slot]->is_pic;
	source = ci->ci_isources[ih->ih_slot];
	idtvec = source->is_idtvec;

	(*pic->pic_hwmask)(pic, ih->ih_pin);	
	atomic_and_32(&ci->ci_ipending, ~(1 << ih->ih_slot));

	/*
	 * Remove the handler from the chain.
	 */
	for (p = &source->is_handlers; (q = *p) != NULL && q != ih;
	     p = &q->ih_next)
		;
	if (q == NULL) {
		x86_write_psl(psl);
		panic("%s: handler not registered", __func__);
		/* NOTREACHED */
	}

	*p = q->ih_next;

	intr_calculatemasks(ci);
	/*
	 * If there is no any handler, 1) do delroute because it has no
	 * any source and 2) dont' hwunmask to prevent spurious interrupt.
	 *
	 * If there is any handler, 1) don't delroute because it has source
	 * and 2) do hwunmask to be able to get interrupt again.
	 *
	 */
	if (source->is_handlers == NULL)
		(*pic->pic_delroute)(pic, ci, ih->ih_pin, idtvec, source->is_type);
	else
		(*pic->pic_hwunmask)(pic, ih->ih_pin);

	/* Re-enable interrupts. */
	x86_write_psl(psl);

	/* If the source is free we can drop it now. */
	intr_source_free(ci, ih->ih_slot, pic, idtvec);

	DPRINTF(("%s: remove slot %d (pic %s pin %d vec %d)\n",
	    device_xname(ci->ci_dev), ih->ih_slot, pic->pic_name,
	    ih->ih_pin, idtvec));
}

static int
intr_num_handlers(struct intrsource *isp)
{
	struct intrhand *ih;
	int num;

	num = 0;
	for (ih = isp->is_handlers; ih != NULL; ih = ih->ih_next)
		num++;

	return num;
}

#else /* XEN */
void *
intr_establish(int legacy_irq, struct pic *pic, int pin,
    int type, int level, int (*handler)(void *), void *arg,
    bool known_mpsafe)
{

	return intr_establish_xname(legacy_irq, pic, pin, type, level,
	    handler, arg, known_mpsafe, "XEN");
}

void *
intr_establish_xname(int legacy_irq, struct pic *pic, int pin,
    int type, int level, int (*handler)(void *), void *arg,
    bool known_mpsafe, const char *xname)
{
	if (pic->pic_type == PIC_XEN) {
		struct intrhand *rih;

		/*
		 * event_set_handler interprets `level != IPL_VM' to
		 * mean MP-safe, so we require the caller to match that
		 * for the moment.
		 */
		KASSERT(known_mpsafe == (level != IPL_VM));

		event_set_handler(pin, handler, arg, level, xname);

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
		rih->ih_fun = handler;
		rih->ih_arg = arg;
		rih->pic_type = pic->pic_type;
		return rih;
	} 	/* Else we assume pintr */

#if NPCI > 0 || NISA > 0
	struct pintrhand *pih;
	intr_handle_t irq;
	int evtchn;
	char evname[16];

	KASSERTMSG(legacy_irq == -1 || (0 <= legacy_irq && legacy_irq < 16),
	    "bad legacy IRQ value: %d", legacy_irq);
	KASSERTMSG(!(legacy_irq == -1 && pic == &i8259_pic),
	    "non-legacy IRQon i8259 ");

	if (pic->pic_type != PIC_I8259) {
#if NIOAPIC > 0
		/* will do interrupts via I/O APIC */
		irq = APIC_INT_VIA_APIC;
		irq |= pic->pic_apicid << APIC_INT_APIC_SHIFT;
		irq |= pin << APIC_INT_PIN_SHIFT;
		snprintf(evname, sizeof(evname), "%s pin %d",
		    pic->pic_name, pin);
#else /* NIOAPIC */
		return NULL;
#endif /* NIOAPIC */
	} else {
		snprintf(evname, sizeof(evname), "irq%d", legacy_irq);
		irq = legacy_irq;
	}

	evtchn = xen_pirq_alloc(&irq, type);
	pih = pirq_establish(irq & 0xff, evtchn, handler, arg, level,
	    evname);
	pih->pic_type = pic->pic_type;
	return pih;
#endif /* NPCI > 0 || NISA > 0 */

	/* FALLTHROUGH */
	return NULL;
}

#endif /* XEN */

/*
 * Deregister an interrupt handler.
 */
void
intr_disestablish(struct intrhand *ih)
{
#if !defined(XEN)
	struct cpu_info *ci;
	struct intrsource *isp;
	uint64_t where;

	/*
	 * Count the removal for load balancing.
	 * Call out to the remote CPU to update its interrupt state.
	 * Only make RPCs if the APs are up and running.
	 */
	mutex_enter(&cpu_lock);
	ci = ih->ih_cpu;
	(ci->ci_nintrhand)--;
	KASSERT(ci->ci_nintrhand >= 0);
	isp = ci->ci_isources[ih->ih_slot];
	if (ci == curcpu() || !mp_online) {
		intr_disestablish_xcall(ih, NULL);
	} else {
		where = xc_unicast(0, intr_disestablish_xcall, ih, NULL, ci);
		xc_wait(where);
	}
	if (!msipic_is_msi_pic(isp->is_pic) && intr_num_handlers(isp) < 1) {
		intr_free_io_intrsource_direct(isp);
	}
	mutex_exit(&cpu_lock);
	kmem_free(ih, sizeof(*ih));
#else /* XEN */
	if (ih->pic_type == PIC_XEN) {
		event_remove_handler(ih->ih_pin, ih->ih_realfun,
		    ih->ih_realarg);
		kmem_free(ih, sizeof(*ih));
		return;
	}
#if defined(DOM0OPS)
	pirq_disestablish((struct pintrhand *)ih);
#endif
	return;
#endif /* XEN */
}

#if !defined(XEN)
#if defined(XEN) /* nuke conditional post integration */
static const char *
xen_intr_string(int port, char *buf, size_t len, struct pic *pic)
{
	KASSERT(pic->pic_type == PIC_XEN);

	KASSERT(port >= 0);
	KASSERT(port < NR_EVENT_CHANNELS);

	snprintf(buf, len, "%s channel %d", pic->pic_name, port);

	return buf;
}
#endif /* XXX: XEN */

static const char *
legacy_intr_string(int ih, char *buf, size_t len, struct pic *pic)
{
	int legacy_irq;

	KASSERT(pic->pic_type == PIC_I8259);
#if NLAPIC > 0
	KASSERT(APIC_IRQ_ISLEGACY(ih));

	legacy_irq = APIC_IRQ_LEGACY_IRQ(ih);
#else
	legacy_irq = ih;
#endif
	KASSERT(legacy_irq >= 0 && legacy_irq < 16);

	snprintf(buf, len, "%s pin %d", pic->pic_name, legacy_irq);

	return buf;
}
#endif

const char *
intr_string(intr_handle_t ih, char *buf, size_t len)
{
#if NIOAPIC > 0
	struct ioapic_softc *pic;
#endif

	if (ih == 0)
		panic("%s: bogus handle 0x%" PRIx64, __func__, ih);

#if NIOAPIC > 0
	if (ih & APIC_INT_VIA_APIC) {
		pic = ioapic_find(APIC_IRQ_APIC(ih));
		if (pic != NULL) {
			snprintf(buf, len, "%s pin %d",
			    device_xname(pic->sc_dev), APIC_IRQ_PIN(ih));
		} else {
			snprintf(buf, len,
			    "apic %d int %d (irq %d)",
			    APIC_IRQ_APIC(ih),
			    APIC_IRQ_PIN(ih),
			    APIC_IRQ_LEGACY_IRQ(ih));
		}
	} else
		snprintf(buf, len, "irq %d", APIC_IRQ_LEGACY_IRQ(ih));

#elif NLAPIC > 0
	snprintf(buf, len, "irq %d" APIC_IRQ_LEGACY_IRQ(ih));
#else
	snprintf(buf, len, "irq %d", (int) ih);
#endif
	return buf;

}

/*
 * Fake interrupt handler structures for the benefit of symmetry with
 * other interrupt sources, and the benefit of intr_calculatemasks()
 */
struct intrhand fake_softclock_intrhand;
struct intrhand fake_softnet_intrhand;
struct intrhand fake_softserial_intrhand;
struct intrhand fake_softbio_intrhand;
struct intrhand fake_timer_intrhand;
struct intrhand fake_ipi_intrhand;
struct intrhand fake_preempt_intrhand;

#if NLAPIC > 0 && defined(MULTIPROCESSOR)
static const char *x86_ipi_names[X86_NIPI] = X86_IPI_NAMES;
#endif

static inline bool
redzone_const_or_false(bool x)
{
#ifdef DIAGNOSTIC
	return x;
#else
	return false;
#endif /* !DIAGNOSTIC */
}

static inline int
redzone_const_or_zero(int x)
{
	return redzone_const_or_false(true) ? x : 0;
}

/*
 * Initialize all handlers that aren't dynamically allocated, and exist
 * for each CPU.
 */
void
cpu_intr_init(struct cpu_info *ci)
{
#if !defined(XEN)
	struct intrsource *isp;
#if NLAPIC > 0 && defined(MULTIPROCESSOR)
	int i;
	static int first = 1;
#endif
#ifdef INTRSTACKSIZE
	vaddr_t istack;
#endif

#if NLAPIC > 0
	isp = kmem_zalloc(sizeof(*isp), KM_SLEEP);
	isp->is_recurse = Xrecurse_lapic_ltimer;
	isp->is_resume = Xresume_lapic_ltimer;
	fake_timer_intrhand.ih_level = IPL_CLOCK;
	isp->is_handlers = &fake_timer_intrhand;
	isp->is_pic = &local_pic;
	ci->ci_isources[LIR_TIMER] = isp;
	evcnt_attach_dynamic(&isp->is_evcnt,
	    first ? EVCNT_TYPE_INTR : EVCNT_TYPE_MISC, NULL,
	    device_xname(ci->ci_dev), "timer");
	first = 0;

#ifdef MULTIPROCESSOR
	isp = kmem_zalloc(sizeof(*isp), KM_SLEEP);
	isp->is_recurse = Xrecurse_lapic_ipi;
	isp->is_resume = Xresume_lapic_ipi;
	fake_ipi_intrhand.ih_level = IPL_HIGH;
	isp->is_handlers = &fake_ipi_intrhand;
	isp->is_pic = &local_pic;
	ci->ci_isources[LIR_IPI] = isp;

	for (i = 0; i < X86_NIPI; i++)
		evcnt_attach_dynamic(&ci->ci_ipi_events[i], EVCNT_TYPE_MISC,
		    NULL, device_xname(ci->ci_dev), x86_ipi_names[i]);
#endif
#endif

#if defined(__HAVE_PREEMPTION)
	isp = kmem_zalloc(sizeof(*isp), KM_SLEEP);
	isp->is_recurse = Xpreemptrecurse;
	isp->is_resume = Xpreemptresume;
	fake_preempt_intrhand.ih_level = IPL_PREEMPT;
	isp->is_handlers = &fake_preempt_intrhand;
	isp->is_pic = &softintr_pic;
	ci->ci_isources[SIR_PREEMPT] = isp;

#endif
	intr_calculatemasks(ci);

#else /* XEN */
	int i; /* XXX: duplicate */
	vaddr_t istack; /* XXX: duplicate */
		ci->ci_iunmask[0] = 0xfffffffe;
	for (i = 1; i < NIPL; i++)
		ci->ci_iunmask[i] = ci->ci_iunmask[i - 1] & ~(1 << i);
#endif /* XEN */

#if defined(INTRSTACKSIZE)
	/*
	 * If the red zone is activated, protect both the top and
	 * the bottom of the stack with an unmapped page.
	 */
	istack = uvm_km_alloc(kernel_map,
	    INTRSTACKSIZE + redzone_const_or_zero(2 * PAGE_SIZE), 0,
	    UVM_KMF_WIRED);
	if (redzone_const_or_false(true)) {
		pmap_kremove(istack, PAGE_SIZE);
		pmap_kremove(istack + INTRSTACKSIZE + PAGE_SIZE, PAGE_SIZE);
		pmap_update(pmap_kernel());
	}
	/* 33 used to be 1.  Arbitrarily reserve 32 more register_t's
	 * of space for ddb(4) to examine some subroutine arguments
	 * and to hunt for the next stack frame.
	 */
	ci->ci_intrstack = (char *)istack + redzone_const_or_zero(PAGE_SIZE) +
	    INTRSTACKSIZE - 33 * sizeof(register_t);
#if defined(__x86_64__)
	ci->ci_tss->tss.tss_ist[0] = (uintptr_t)ci->ci_intrstack & ~0xf;
#endif /* defined(__x86_64__) */
#endif /* defined(INTRSTACKSIZE) */
	ci->ci_idepth = -1;
}

#if defined(INTRDEBUG) || defined(DDB)

void
intr_printconfig(void)
{
	int i;
	struct intrhand *ih;
	struct intrsource *isp;
	struct cpu_info *ci;
	CPU_INFO_ITERATOR cii;
	void (*pr)(const char *, ...);

	pr = printf;
#ifdef DDB
	extern int db_active;
	if (db_active) {
		pr = db_printf;
	}
#endif

	for (CPU_INFO_FOREACH(cii, ci)) {
		(*pr)("%s: interrupt masks:\n", device_xname(ci->ci_dev));
		for (i = 0; i < NIPL; i++)
			(*pr)("IPL %d mask %08lx unmask %08lx\n", i,
			    (u_long)ci->ci_imask[i], (u_long)ci->ci_iunmask[i]);
		for (i = 0; i < MAX_INTR_SOURCES; i++) {
			isp = ci->ci_isources[i];
			if (isp == NULL)
				continue;
			(*pr)("%s source %d is pin %d from pic %s type %d maxlevel %d\n",
			    device_xname(ci->ci_dev), i, isp->is_pin,
			    isp->is_pic->pic_name, isp->is_type, isp->is_maxlevel);
			for (ih = isp->is_handlers; ih != NULL;
			     ih = ih->ih_next)
				(*pr)("\thandler %p level %d\n",
				    ih->ih_fun, ih->ih_level);
#if NIOAPIC > 0
			if (isp->is_pic->pic_type == PIC_IOAPIC) {
				struct ioapic_softc *sc;
				sc = isp->is_pic->pic_ioapic;
				(*pr)("\tioapic redir 0x%x\n",
				    sc->sc_pins[isp->is_pin].ip_map->redir);
			}
#endif

		}
	}
}

#endif

#if defined(__HAVE_FAST_SOFTINTS)
void
softint_init_md(lwp_t *l, u_int level, uintptr_t *machdep)
{
	struct intrsource *isp;
	struct cpu_info *ci;
	u_int sir;

	ci = l->l_cpu;

	isp = kmem_zalloc(sizeof(*isp), KM_SLEEP);
	isp->is_recurse = Xsoftintr;
	isp->is_resume = Xsoftintr;
	isp->is_pic = &softintr_pic;

	switch (level) {
	case SOFTINT_BIO:
		sir = SIR_BIO;
		fake_softbio_intrhand.ih_level = IPL_SOFTBIO;
		isp->is_handlers = &fake_softbio_intrhand;
		break;
	case SOFTINT_NET:
		sir = SIR_NET;
		fake_softnet_intrhand.ih_level = IPL_SOFTNET;
		isp->is_handlers = &fake_softnet_intrhand;
		break;
	case SOFTINT_SERIAL:
		sir = SIR_SERIAL;
		fake_softserial_intrhand.ih_level = IPL_SOFTSERIAL;
		isp->is_handlers = &fake_softserial_intrhand;
		break;
	case SOFTINT_CLOCK:
		sir = SIR_CLOCK;
		fake_softclock_intrhand.ih_level = IPL_SOFTCLOCK;
		isp->is_handlers = &fake_softclock_intrhand;
		break;
	default:
		panic("softint_init_md");
	}

	KASSERT(ci->ci_isources[sir] == NULL);

	*machdep = (1 << sir);
	ci->ci_isources[sir] = isp;
	ci->ci_isources[sir]->is_lwp = l;

	intr_calculatemasks(ci);
}
#endif /* __HAVE_FAST_SOFTINTS */
/*
 * Save current affinitied cpu's interrupt count.
 */
static void
intr_save_evcnt(struct intrsource *source, cpuid_t cpuid)
{
	struct percpu_evcnt *pep;
	uint64_t curcnt;
	int i;

	curcnt = source->is_evcnt.ev_count;
	pep = source->is_saved_evcnt;

	for (i = 0; i < ncpu; i++) {
		if (pep[i].cpuid == cpuid) {
			pep[i].count = curcnt;
			break;
		}
	}
}

/*
 * Restore current affinitied cpu's interrupt count.
 */
static void
intr_restore_evcnt(struct intrsource *source, cpuid_t cpuid)
{
	struct percpu_evcnt *pep;
	int i;

	pep = source->is_saved_evcnt;

	for (i = 0; i < ncpu; i++) {
		if (pep[i].cpuid == cpuid) {
			source->is_evcnt.ev_count = pep[i].count;
			break;
		}
	}
}

static void
intr_redistribute_xc_t(void *arg1, void *arg2)
{
	struct cpu_info *ci;
	struct intrsource *isp;
	int slot;
	u_long psl;

	ci = curcpu();
	isp = arg1;
	slot = (int)(intptr_t)arg2;

	/* Disable interrupts locally. */
	psl = x86_read_psl();
	x86_disable_intr();

	/* Hook it in and re-calculate masks. */
	ci->ci_isources[slot] = isp;
	intr_calculatemasks(curcpu());

	/* Re-enable interrupts locally. */
	x86_write_psl(psl);
}

static void
intr_redistribute_xc_s1(void *arg1, void *arg2)
{
	struct pic *pic;
	struct intrsource *isp;
	struct cpu_info *nci;
	u_long psl;

	isp = arg1;
	nci = arg2;

	/*
	 * Disable interrupts on-chip and mask the pin.  Back out
	 * and let the interrupt be processed if one is pending.
	 */
	pic = isp->is_pic;
	for (;;) {
		psl = x86_read_psl();
		x86_disable_intr();
		if ((*pic->pic_trymask)(pic, isp->is_pin)) {
			break;
		}
		x86_write_psl(psl);
		DELAY(1000);
	}

	/* pic_addroute will unmask the interrupt. */
	(*pic->pic_addroute)(pic, nci, isp->is_pin, isp->is_idtvec,
	    isp->is_type);
	x86_write_psl(psl);
}

static void
intr_redistribute_xc_s2(void *arg1, void *arg2)
{
	struct cpu_info *ci;
	u_long psl;
	int slot;

	ci = curcpu();
	slot = (int)(uintptr_t)arg1;

	/* Disable interrupts locally. */
	psl = x86_read_psl();
	x86_disable_intr();

	/* Patch out the source and re-calculate masks. */
	ci->ci_isources[slot] = NULL;
	intr_calculatemasks(ci);

	/* Re-enable interrupts locally. */
	x86_write_psl(psl);
}

static bool
intr_redistribute(struct cpu_info *oci)
{
	struct intrsource *isp;
	struct intrhand *ih;
	CPU_INFO_ITERATOR cii;
	struct cpu_info *nci, *ici;
	int oslot, nslot;
	uint64_t where;

	KASSERT(mutex_owned(&cpu_lock));

	/* Look for an interrupt source that we can migrate. */
	for (oslot = 0; oslot < MAX_INTR_SOURCES; oslot++) {
		if ((isp = oci->ci_isources[oslot]) == NULL) {
			continue;
		}
		if (isp->is_pic->pic_type == PIC_IOAPIC) {
			break;
		}
	}
	if (oslot == MAX_INTR_SOURCES) {
		return false;
	}

	/* Find least loaded CPU and try to move there. */
	nci = NULL;
	for (CPU_INFO_FOREACH(cii, ici)) {
		if ((ici->ci_schedstate.spc_flags & SPCF_NOINTR) != 0) {
			continue;
		}
		KASSERT(ici != oci);
		if (nci == NULL || nci->ci_nintrhand > ici->ci_nintrhand) {
			nci = ici;
		}
	}
	if (nci == NULL) {
		return false;
	}
	for (nslot = 0; nslot < MAX_INTR_SOURCES; nslot++) {
		if (nci->ci_isources[nslot] == NULL) {
			break;
		}
	}

	/* If that did not work, allocate anywhere. */
	if (nslot == MAX_INTR_SOURCES) {
		for (CPU_INFO_FOREACH(cii, nci)) {
			if ((nci->ci_schedstate.spc_flags & SPCF_NOINTR) != 0) {
				continue;
			}
			KASSERT(nci != oci);
			for (nslot = 0; nslot < MAX_INTR_SOURCES; nslot++) {
				if (nci->ci_isources[nslot] == NULL) {
					break;
				}
			}
			if (nslot != MAX_INTR_SOURCES) {
				break;
			}
		}
	}
	if (nslot == MAX_INTR_SOURCES) {
		return false;
	}

	/*
	 * Now we have new CPU and new slot.  Run a cross-call to set up
	 * the new vector on the target CPU.
	 */
	where = xc_unicast(0, intr_redistribute_xc_t, isp,
	    (void *)(intptr_t)nslot, nci);
	xc_wait(where);
	
	/*
	 * We're ready to go on the target CPU.  Run a cross call to
	 * reroute the interrupt away from the source CPU.
	 */
	where = xc_unicast(0, intr_redistribute_xc_s1, isp, nci, oci);
	xc_wait(where);

	/* Sleep for (at least) 10ms to allow the change to take hold. */
	(void)kpause("intrdist", false, mstohz(10), NULL);

	/* Complete removal from the source CPU. */
	where = xc_unicast(0, intr_redistribute_xc_s2,
	    (void *)(uintptr_t)oslot, NULL, oci);
	xc_wait(where);

	/* Finally, take care of book-keeping. */
	for (ih = isp->is_handlers; ih != NULL; ih = ih->ih_next) {
		oci->ci_nintrhand--;
		nci->ci_nintrhand++;
		ih->ih_cpu = nci;
	}
	intr_save_evcnt(isp, oci->ci_cpuid);
	intr_restore_evcnt(isp, nci->ci_cpuid);
	isp->is_active_cpu = nci->ci_cpuid;

	return true;
}

void
cpu_intr_redistribute(void)
{
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;

	KASSERT(mutex_owned(&cpu_lock));
	KASSERT(mp_online);

#if defined(XEN) /* XXX: remove */
	return;
#endif
	/* Direct interrupts away from shielded CPUs. */
	for (CPU_INFO_FOREACH(cii, ci)) {
		if ((ci->ci_schedstate.spc_flags & SPCF_NOINTR) == 0) {
			continue;
		}
		while (intr_redistribute(ci)) {
			/* nothing */
		}
	}

	/* XXX should now re-balance */
}

u_int
cpu_intr_count(struct cpu_info *ci)
{

	KASSERT(ci->ci_nintrhand >= 0);

	return ci->ci_nintrhand;
}

#if !defined(XEN)
static int
intr_find_unused_slot(struct cpu_info *ci, int *index)
{
	int slot, i;

	KASSERT(mutex_owned(&cpu_lock));

	slot = -1;
	for (i = 0; i < MAX_INTR_SOURCES ; i++) {
		if (ci->ci_isources[i] == NULL) {
			slot = i;
			break;
		}
	}
	if (slot == -1) {
		DPRINTF(("cannot allocate ci_isources\n"));
		return EBUSY;
	}

	*index = slot;
	return 0;
}

/*
 * Let cpu_info ready to accept the interrupt.
 */
static void
intr_activate_xcall(void *arg1, void *arg2)
{
	struct cpu_info *ci;
	struct intrsource *source;
	struct intrstub *stubp;
	struct intrhand *ih;
	u_long psl;
	int idt_vec;
	int slot;

	ih = arg1;

	kpreempt_disable();

	KASSERT(ih->ih_cpu == curcpu() || !mp_online);

	ci = ih->ih_cpu;
	slot = ih->ih_slot;
	source = ci->ci_isources[slot];
	idt_vec = source->is_idtvec;

	psl = x86_read_psl();
	x86_disable_intr();

	intr_calculatemasks(ci);

	if (source->is_type == IST_LEVEL) {
		stubp = &source->is_pic->pic_level_stubs[slot];
	} else {
		stubp = &source->is_pic->pic_edge_stubs[slot];
	}
	source->is_resume = stubp->ist_resume;
	source->is_recurse = stubp->ist_recurse;
	idt_vec_set(idt_vec, stubp->ist_entry);

	x86_write_psl(psl);

	kpreempt_enable();
}

/*
 * Let cpu_info not accept the interrupt.
 */
static void
intr_deactivate_xcall(void *arg1, void *arg2)
{
	struct cpu_info *ci;
	struct intrhand *ih, *lih;
	u_long psl;
	int slot;

	ih = arg1;

	kpreempt_disable();

	KASSERT(ih->ih_cpu == curcpu() || !mp_online);

	ci = ih->ih_cpu;
	slot = ih->ih_slot;

	psl = x86_read_psl();
	x86_disable_intr();

	/* Move all devices sharing IRQ number. */
	ci->ci_isources[slot] = NULL;
	for (lih = ih; lih != NULL; lih = lih->ih_next) {
		ci->ci_nintrhand--;
	}

	intr_calculatemasks(ci);

	/*
	 * Skip unsetgate(), because the same itd[] entry is overwritten in
	 * intr_activate_xcall().
	 */

	x86_write_psl(psl);

	kpreempt_enable();
}

static void
intr_get_affinity(struct intrsource *isp, kcpuset_t *cpuset)
{
	struct cpu_info *ci;

	KASSERT(mutex_owned(&cpu_lock));

	if (isp == NULL) {
		kcpuset_zero(cpuset);
		return;
	}

	ci = isp->is_handlers->ih_cpu;
	if (ci == NULL) {
		kcpuset_zero(cpuset);
		return;
	}

	kcpuset_set(cpuset, cpu_index(ci));
	return;
}

static int
intr_set_affinity(struct intrsource *isp, const kcpuset_t *cpuset)
{
	struct cpu_info *oldci, *newci;
	struct intrhand *ih, *lih;
	struct pic *pic;
	u_int cpu_idx;
	int idt_vec;
	int oldslot, newslot;
	int err;
	int pin;

	KASSERT(mutex_owned(&intr_distribute_lock));
	KASSERT(mutex_owned(&cpu_lock));

	/* XXX
	 * logical destination mode is not supported, use lowest index cpu.
	 */
	cpu_idx = kcpuset_ffs(cpuset) - 1;
	newci = cpu_lookup(cpu_idx);
	if (newci == NULL) {
		DPRINTF(("invalid cpu index: %u\n", cpu_idx));
		return EINVAL;
	}
	if ((newci->ci_schedstate.spc_flags & SPCF_NOINTR) != 0) {
		DPRINTF(("the cpu is set nointr shield. index:%u\n", cpu_idx));
		return EINVAL;
	}

	if (isp == NULL) {
		DPRINTF(("invalid intrctl handler\n"));
		return EINVAL;
	}

	/* i8259_pic supports only primary cpu, see i8259.c. */
	pic = isp->is_pic;
	if (pic == &i8259_pic) {
		DPRINTF(("i8259 pic does not support set_affinity\n"));
		return ENOTSUP;
	}

	ih = isp->is_handlers;
	oldci = ih->ih_cpu;
	if (newci == oldci) /* nothing to do */
		return 0;

	oldslot = ih->ih_slot;
	idt_vec = isp->is_idtvec;

	err = intr_find_unused_slot(newci, &newslot);
	if (err) {
		DPRINTF(("failed to allocate interrupt slot for PIC %s intrid %s\n",
			isp->is_pic->pic_name, isp->is_intrid));
		return err;
	}

	pin = isp->is_pin;
	(*pic->pic_hwmask)(pic, pin); /* for ci_ipending check */
	while(oldci->ci_ipending & (1 << oldslot))
		(void)kpause("intrdist", false, 1, &cpu_lock);

	kpreempt_disable();

	/* deactivate old interrupt setting */
	if (oldci == curcpu() || !mp_online) {
		intr_deactivate_xcall(ih, NULL);
	} else {
		uint64_t where;
		where = xc_unicast(0, intr_deactivate_xcall, ih,
				   NULL, oldci);
		xc_wait(where);
	}
	intr_save_evcnt(isp, oldci->ci_cpuid);
	(*pic->pic_delroute)(pic, oldci, pin, idt_vec, isp->is_type);

	/* activate new interrupt setting */
	newci->ci_isources[newslot] = isp;
	for (lih = ih; lih != NULL; lih = lih->ih_next) {
		newci->ci_nintrhand++;
		lih->ih_cpu = newci;
		lih->ih_slot = newslot;
	}
	if (newci == curcpu() || !mp_online) {
		intr_activate_xcall(ih, NULL);
	} else {
		uint64_t where;
		where = xc_unicast(0, intr_activate_xcall, ih,
				   NULL, newci);
		xc_wait(where);
	}
	intr_restore_evcnt(isp, newci->ci_cpuid);
	isp->is_active_cpu = newci->ci_cpuid;
	(*pic->pic_addroute)(pic, newci, pin, idt_vec, isp->is_type);

	kpreempt_enable();

	(*pic->pic_hwunmask)(pic, pin);

	return err;
}
#endif /* XEN */
static bool
intr_is_affinity_intrsource(struct intrsource *isp, const kcpuset_t *cpuset)
{
	struct cpu_info *ci;

	KASSERT(mutex_owned(&cpu_lock));

	ci = isp->is_handlers->ih_cpu;
	KASSERT(ci != NULL);

	return kcpuset_isset(cpuset, cpu_index(ci));
}

static struct intrhand *
intr_get_handler(const char *intrid)
{
	struct intrsource *isp;

	KASSERT(mutex_owned(&cpu_lock));

	isp = intr_get_io_intrsource(intrid);
	if (isp == NULL)
		return NULL;

	return isp->is_handlers;
}

#if !defined(XEN)
/*
 * MI interface for subr_interrupt.c
 */
uint64_t
interrupt_get_count(const char *intrid, u_int cpu_idx)
{
	struct cpu_info *ci;
	struct intrsource *isp;
	struct intrhand *ih;
	struct percpu_evcnt pep;
	cpuid_t cpuid;
	int i, slot;
	uint64_t count = 0;

	ci = cpu_lookup(cpu_idx);
	cpuid = ci->ci_cpuid;

	mutex_enter(&cpu_lock);

	ih = intr_get_handler(intrid);
	if (ih == NULL) {
		count = 0;
		goto out;
	}
	slot = ih->ih_slot;
	isp = ih->ih_cpu->ci_isources[slot];

	for (i = 0; i < ncpu; i++) {
		pep = isp->is_saved_evcnt[i];
		if (cpuid == pep.cpuid) {
			if (isp->is_active_cpu == pep.cpuid) {
				count = isp->is_evcnt.ev_count;
				goto out;
			} else {
				count = pep.count;
				goto out;
			}
		}
	}

 out:
	mutex_exit(&cpu_lock);
	return count;
}

#endif /* XEN */

/*
 * MI interface for subr_interrupt.c
 */
void
interrupt_get_assigned(const char *intrid, kcpuset_t *cpuset)
{
	struct cpu_info *ci;
	struct intrhand *ih;

	kcpuset_zero(cpuset);

	mutex_enter(&cpu_lock);

	ih = intr_get_handler(intrid);
	if (ih == NULL)
		goto out;

	ci = ih->ih_cpu;
	kcpuset_set(cpuset, cpu_index(ci));

 out:
	mutex_exit(&cpu_lock);
}

#if !defined(XEN)

/*
 * MI interface for subr_interrupt.c
 */
void
interrupt_get_available(kcpuset_t *cpuset)
{
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;

	kcpuset_zero(cpuset);

	mutex_enter(&cpu_lock);
	for (CPU_INFO_FOREACH(cii, ci)) {
		if ((ci->ci_schedstate.spc_flags & SPCF_NOINTR) == 0) {
			kcpuset_set(cpuset, cpu_index(ci));
		}
	}
	mutex_exit(&cpu_lock);
}

/*
 * MI interface for subr_interrupt.c
 */
void
interrupt_get_devname(const char *intrid, char *buf, size_t len)
{
	struct intrsource *isp;
	struct intrhand *ih;
	int slot;

	mutex_enter(&cpu_lock);

	ih = intr_get_handler(intrid);
	if (ih == NULL) {
		buf[0] = '\0';
		goto out;
	}
	slot = ih->ih_slot;
	isp = ih->ih_cpu->ci_isources[slot];
	strlcpy(buf, isp->is_xname, len);

 out:
	mutex_exit(&cpu_lock);
}

static int
intr_distribute_locked(struct intrhand *ih, const kcpuset_t *newset,
    kcpuset_t *oldset)
{
	struct intrsource *isp;
	int slot;

	KASSERT(mutex_owned(&intr_distribute_lock));
	KASSERT(mutex_owned(&cpu_lock));

	if (ih == NULL)
		return EINVAL;

	slot = ih->ih_slot;
	isp = ih->ih_cpu->ci_isources[slot];
	KASSERT(isp != NULL);

	if (oldset != NULL)
		intr_get_affinity(isp, oldset);

	return intr_set_affinity(isp, newset);
}

/*
 * MI interface for subr_interrupt.c
 */
int
interrupt_distribute(void *cookie, const kcpuset_t *newset, kcpuset_t *oldset)
{
	int error;
	struct intrhand *ih = cookie;

	mutex_enter(&intr_distribute_lock);
	mutex_enter(&cpu_lock);
	error = intr_distribute_locked(ih, newset, oldset);
	mutex_exit(&cpu_lock);
	mutex_exit(&intr_distribute_lock);

	return error;
}

/*
 * MI interface for subr_interrupt.c
 */
int
interrupt_distribute_handler(const char *intrid, const kcpuset_t *newset,
    kcpuset_t *oldset)
{
	int error;
	struct intrhand *ih;

	mutex_enter(&intr_distribute_lock);
	mutex_enter(&cpu_lock);

	ih = intr_get_handler(intrid);
	if (ih == NULL) {
		error = ENOENT;
		goto out;
	}
	error = intr_distribute_locked(ih, newset, oldset);

 out:
	mutex_exit(&cpu_lock);
	mutex_exit(&intr_distribute_lock);
	return error;
}
#endif

/*
 * MI interface for subr_interrupt.c
 */
struct intrids_handler *
interrupt_construct_intrids(const kcpuset_t *cpuset)
{
	struct intrsource *isp;
	struct intrids_handler *ii_handler;
	intrid_t *ids;
	int i, count;

	if (kcpuset_iszero(cpuset))
		return 0;

	/*
	 * Count the number of interrupts which affinity to any cpu of "cpuset".
	 */
	count = 0;
	mutex_enter(&cpu_lock);
	SIMPLEQ_FOREACH(isp, &io_interrupt_sources, is_list) {
		if (intr_is_affinity_intrsource(isp, cpuset))
			count++;
	}
	mutex_exit(&cpu_lock);

	ii_handler = kmem_zalloc(sizeof(int) + sizeof(intrid_t) * count,
	    KM_SLEEP);
	if (ii_handler == NULL)
		return NULL;
	ii_handler->iih_nids = count;
	if (count == 0)
		return ii_handler;

	ids = ii_handler->iih_intrids;
	i = 0;
	mutex_enter(&cpu_lock);
	SIMPLEQ_FOREACH(isp, &io_interrupt_sources, is_list) {
		/* Ignore devices attached after counting "count". */
		if (i >= count) {
			DPRINTF(("New devices are attached after counting.\n"));
			break;
		}

		if (!intr_is_affinity_intrsource(isp, cpuset))
			continue;

		strncpy(ids[i], isp->is_intrid, sizeof(intrid_t));
		i++;
	}
	mutex_exit(&cpu_lock);

	return ii_handler;
}

/*
 * MI interface for subr_interrupt.c
 */
void
interrupt_destruct_intrids(struct intrids_handler *ii_handler)
{
	size_t iih_size;

	if (ii_handler == NULL)
		return;

	iih_size = sizeof(int) + sizeof(intrid_t) * ii_handler->iih_nids;
	kmem_free(ii_handler, iih_size);
}
