/*	$NetBSD: intr.c,v 1.58.2.3 2010/10/24 22:48:18 jym Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: intr.c,v 1.58.2.3 2010/10/24 22:48:18 jym Exp $");

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

struct pic softintr_pic = {
	.pic_name = "softintr_fakepic",
	.pic_type = PIC_SOFT,
	.pic_vecbase = 0,
	.pic_apicid = 0,
	.pic_lock = __SIMPLELOCK_UNLOCKED,
};

#if NIOAPIC > 0 || NACPICA > 0
static int intr_scan_bus(int, int, int *);
#if NPCI > 0
static int intr_find_pcibridge(int, pcitag_t *, pci_chipset_tag_t *);
#endif
#endif

/*
 * Fill in default interrupt table (in case of spurious interrupt
 * during configuration of kernel), setup interrupt control unit
 */
void
intr_default_setup(void)
{
	int i;

	/* icu vectors */
	for (i = 0; i < NUM_LEGACY_IRQS; i++) {
		idt_vec_reserve(ICU_OFFSET + i);
		setgate(&idt[ICU_OFFSET + i],
		    i8259_stubs[i].ist_entry, 0, SDT_SYS386IGT,
		    SEL_KPL, GSEL(GCODE_SEL, SEL_KPL));
	}

	/*
	 * Eventually might want to check if it's actually there.
	 */
	i8259_default_setup();
}

/*
 * Handle a NMI, possibly a machine check.
 * return true to panic system, false to ignore.
 */
int
x86_nmi(void)
{

	log(LOG_CRIT, "NMI port 61 %x, port 70 %x\n", inb(0x61), inb(0x70));
	return(0);
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
int
intr_find_mpmapping(int bus, int pin, int *handle)
{
#if NPCI > 0
	int dev, func;
	pcitag_t pci_bridge_tag;
	pci_chipset_tag_t pc;
#endif

#if NPCI > 0
	while (intr_scan_bus(bus, pin, handle) != 0) {
		if (intr_find_pcibridge(bus, &pci_bridge_tag,
		    &pc) != 0)
			return ENOENT;
		dev = pin >> 2;
		pin = pin & 3;
		pin = PPB_INTERRUPT_SWIZZLE(pin + 1, dev) - 1;
		pci_decompose_tag(pc, pci_bridge_tag, &bus,
		    &dev, &func);
		pin |= (dev << 2);
	}
	return 0;
#else
	return intr_scan_bus(bus, pin, handle);
#endif
}

static int
intr_scan_bus(int bus, int pin, int *handle)
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

static int
intr_allocate_slot_cpu(struct cpu_info *ci, struct pic *pic, int pin,
		       int *index)
{
	int slot, i;
	struct intrsource *isp;

	KASSERT(mutex_owned(&cpu_lock));

	if (pic == &i8259_pic) {
		KASSERT(CPU_IS_PRIMARY(ci));
		slot = pin;
	} else {
		slot = -1;

		/*
		 * intr_allocate_slot has checked for an existing mapping.
		 * Now look for a free slot.
		 */
		for (i = 0; i < MAX_INTR_SOURCES ; i++) {
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
		isp = kmem_zalloc(sizeof(*isp), KM_SLEEP);
		if (isp == NULL) {
			return ENOMEM;
		}
		snprintf(isp->is_evname, sizeof (isp->is_evname),
		    "pin %d", pin);
		evcnt_attach_dynamic(&isp->is_evcnt, EVCNT_TYPE_INTR, NULL,
		    pic->pic_name, isp->is_evname);
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
		   struct cpu_info **cip, int *index, int *idt_slot)
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
			if (isp->is_pic == pic && isp->is_pin == pin) {
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
		error = intr_allocate_slot_cpu(ci, pic, pin, &slot);
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
		error = intr_allocate_slot_cpu(ci, pic, pin, &slot);

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
				    pin, &slot);
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
		kmem_free(ci->ci_isources[slot], sizeof(*(ci->ci_isources[slot])));
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
	evcnt_detach(&isp->is_evcnt);
	kmem_free(isp, sizeof(*isp));
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
		setgate(&idt[idt_vec], stubp->ist_entry, 0, SDT_SYS386IGT,
		    SEL_KPL, GSEL(GCODE_SEL, SEL_KPL));
	}

	/* Re-enable interrupts locally. */
	x86_write_psl(psl);
}

void *
intr_establish(int legacy_irq, struct pic *pic, int pin, int type, int level,
	       int (*handler)(void *), void *arg, bool known_mpsafe)
{
	struct intrhand **p, *q, *ih;
	struct cpu_info *ci;
	int slot, error, idt_vec;
	struct intrsource *source;
#ifdef MULTIPROCESSOR
	bool mpsafe = (known_mpsafe || level != IPL_VM);
#endif /* MULTIPROCESSOR */
	uint64_t where;

#ifdef DIAGNOSTIC
	if (legacy_irq != -1 && (legacy_irq < 0 || legacy_irq > 15))
		panic("intr_establish: bad legacy IRQ value");

	if (legacy_irq == -1 && pic == &i8259_pic)
		panic("intr_establish: non-legacy IRQ on i8259");
#endif

	ih = kmem_alloc(sizeof(*ih), KM_SLEEP);
	if (ih == NULL) {
		printf("intr_establish: can't allocate handler info\n");
		return NULL;
	}

	mutex_enter(&cpu_lock);
	error = intr_allocate_slot(pic, pin, level, &ci, &slot, &idt_vec);
	if (error != 0) {
		mutex_exit(&cpu_lock);
		kmem_free(ih, sizeof(*ih));
		printf("failed to allocate interrupt slot for PIC %s pin %d\n",
		    pic->pic_name, pin);
		return NULL;
	}

	source = ci->ci_isources[slot];

	if (source->is_handlers != NULL &&
	    source->is_pic->pic_type != pic->pic_type) {
		mutex_exit(&cpu_lock);
		kmem_free(ih, sizeof(*ih));
		printf("intr_establish: can't share intr source between "
		       "different PIC types (legacy_irq %d pin %d slot %d)\n",
		    legacy_irq, pin, slot);
		return NULL;
	}

	source->is_pin = pin;
	source->is_pic = pic;

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
			mutex_exit(&cpu_lock);
			kmem_free(ih, sizeof(*ih));
			intr_source_free(ci, slot, pic, idt_vec);
			printf("intr_establish: pic %s pin %d: can't share "
			       "type %d with %d\n", pic->pic_name, pin,
				source->is_type, type);
			return NULL;
		}
		break;
	default:
		panic("intr_establish: bad intr type %d for pic %s pin %d\n",
		    source->is_type, pic->pic_name, pin);
		/* NOTREACHED */
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

#ifdef INTRDEBUG
	printf("allocated pic %s type %s pin %d level %d to %s slot %d "
	    "idt entry %d\n",
	    pic->pic_name, type == IST_EDGE ? "edge" : "level", pin, level,
	    device_xname(ci->ci_dev), slot, idt_vec);
#endif

	return (ih);
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
		panic("intr_disestablish: handler not registered");
		/* NOTREACHED */
	}

	*p = q->ih_next;

	intr_calculatemasks(ci);
	(*pic->pic_delroute)(pic, ci, ih->ih_pin, idtvec, source->is_type);
	(*pic->pic_hwunmask)(pic, ih->ih_pin);

	/* Re-enable interrupts. */
	x86_write_psl(psl);

	/* If the source is free we can drop it now. */
	intr_source_free(ci, ih->ih_slot, pic, idtvec);

#ifdef INTRDEBUG
	printf("%s: remove slot %d (pic %s pin %d vec %d)\n",
	    device_xname(ci->ci_dev), ih->ih_slot, pic->pic_name,
	    ih->ih_pin, idtvec);
#endif
}

/*
 * Deregister an interrupt handler.
 */
void
intr_disestablish(struct intrhand *ih)
{
	struct cpu_info *ci;
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
	if (ci == curcpu() || !mp_online) {
		intr_disestablish_xcall(ih, NULL);
	} else {
		where = xc_unicast(0, intr_disestablish_xcall, ih, NULL, ci);
		xc_wait(where);
	}	
	mutex_exit(&cpu_lock);
	kmem_free(ih, sizeof(*ih));
}

const char *
intr_string(int ih)
{
	static char irqstr[64];
#if NIOAPIC > 0
	struct ioapic_softc *pic;
#endif

	if (ih == 0)
		panic("pci_intr_string: bogus handle 0x%x", ih);


#if NIOAPIC > 0
	if (ih & APIC_INT_VIA_APIC) {
		pic = ioapic_find(APIC_IRQ_APIC(ih));
		if (pic != NULL) {
			snprintf(irqstr, sizeof(irqstr), "%s pin %d",
			    device_xname(pic->sc_dev), APIC_IRQ_PIN(ih));
		} else {
			snprintf(irqstr, sizeof(irqstr),
			    "apic %d int %d (irq %d)",
			    APIC_IRQ_APIC(ih),
			    APIC_IRQ_PIN(ih),
			    ih&0xff);
		}
	} else
		snprintf(irqstr, sizeof(irqstr), "irq %d", ih&0xff);
#else

	snprintf(irqstr, sizeof(irqstr), "irq %d", ih&0xff);
#endif
	return (irqstr);

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
	KASSERT(isp != NULL);
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
	KASSERT(isp != NULL);
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

	isp = kmem_zalloc(sizeof(*isp), KM_SLEEP);
	KASSERT(isp != NULL);
	isp->is_recurse = Xpreemptrecurse;
	isp->is_resume = Xpreemptresume;
	fake_preempt_intrhand.ih_level = IPL_PREEMPT;
	isp->is_handlers = &fake_preempt_intrhand;
	isp->is_pic = &softintr_pic;
	ci->ci_isources[SIR_PREEMPT] = isp;

	intr_calculatemasks(ci);

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
	ci->ci_tss.tss_ist[0] = (uintptr_t)ci->ci_intrstack & ~0xf;
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
softint_init_md(lwp_t *l, u_int level, uintptr_t *machdep)
{
	struct intrsource *isp;
	struct cpu_info *ci;
	u_int sir;

	ci = l->l_cpu;

	isp = kmem_zalloc(sizeof(*isp), KM_SLEEP);
	KASSERT(isp != NULL);
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

	return true;
}

void
cpu_intr_redistribute(void)
{
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;

	KASSERT(mutex_owned(&cpu_lock));
	KASSERT(mp_online);

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
