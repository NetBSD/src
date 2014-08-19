/*      $NetBSD: loongson_intr.c,v 1.1.12.2 2014/08/20 00:02:58 tls Exp $      */

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
__KERNEL_RCSID(0, "$NetBSD: loongson_intr.c,v 1.1.12.2 2014/08/20 00:02:58 tls Exp $");

#define __INTR_PRIVATE

#include <sys/param.h>
#include <sys/device.h>
#include <sys/cpu.h>
#include <sys/intr.h>
#include <sys/bus.h>
#include <sys/malloc.h>

#include <mips/mips3_clock.h>
#include <machine/locore.h>

#include <evbmips/loongson/autoconf.h>
#include <evbmips/loongson/loongson_intr.h>

#include <mips/locore.h>

#include <mips/bonito/bonitoreg.h>
#include <mips/bonito/bonitovar.h>

#include <dev/pci/pciidereg.h>
#include <dev/isa/isavar.h>

#include "isa.h"

#ifdef INTR_DEBUG
#define DPRINTF(x) printf x
#else
#define DPRINTF(x)
#endif

struct bonito_intrhead bonito_intrhead[BONITO_NINTS];

/*
 * This is a mask of bits to clear in the SR when we go to a
 * given hardware interrupt priority level.
 */
static const struct ipl_sr_map loongson_ipl_sr_map = {
    .sr_bits = {
	[IPL_NONE] =		0,
	[IPL_SOFTCLOCK] =	MIPS_SOFT_INT_MASK_0,
	[IPL_SOFTNET] =		MIPS_SOFT_INT_MASK_0 | MIPS_SOFT_INT_MASK_1,
	[IPL_VM] =
	    MIPS_SOFT_INT_MASK_0 | MIPS_SOFT_INT_MASK_1 |
	    MIPS_INT_MASK_0 |
	    MIPS_INT_MASK_1 |
	    MIPS_INT_MASK_2 |
	    MIPS_INT_MASK_3 |
	    MIPS_INT_MASK_4,
	[IPL_SCHED] =
	    MIPS_SOFT_INT_MASK_0 | MIPS_SOFT_INT_MASK_1 |
	    MIPS_INT_MASK_0 |
	    MIPS_INT_MASK_1 |
	    MIPS_INT_MASK_2 |
	    MIPS_INT_MASK_3 |
	    MIPS_INT_MASK_4 |
	    MIPS_INT_MASK_5,
	[IPL_DDB] =		MIPS_INT_MASK,
	[IPL_HIGH] =            MIPS_INT_MASK,
    },
};


void
evbmips_intr_init(void)
{
	const struct bonito_config * const bc = sys_platform->bonito_config;
	const struct bonito_irqmap *irqmap;
	int i;

	ipl_sr_map = loongson_ipl_sr_map;

	for (i = 0; i < BONITO_NDIRECT; i++) {
		irqmap = &sys_platform->irq_map[i];
		if (irqmap->name == NULL)
			continue;
		DPRINTF(("attach %d %s\n", i, irqmap->name));
		evcnt_attach_dynamic(&bonito_intrhead[i].intr_count,
		    EVCNT_TYPE_INTR, NULL, "bonito", irqmap->name);
		LIST_INIT(&bonito_intrhead[i].intrhand_head);
	}

	REGVAL(BONITO_GPIOIE) = bc->bc_gpioIE;
	REGVAL(BONITO_INTEDGE) = bc->bc_intEdge;
	/* REGVAL(BONITO_INTSTEER) = bc->bc_intSteer; XXX */
	REGVAL(BONITO_INTPOL) = bc->bc_intPol;
	REGVAL(BONITO_INTENCLR) = 0xffffffff;
	(void)REGVAL(BONITO_INTENCLR);

	if (sys_platform->isa_chipset != NULL) {
		int irq;
		static char irqstr[8];
		for (irq = 0; irq < BONITO_NISA; irq++) {
			i = BONITO_ISA_IRQ(irq);
			snprintf(irqstr, sizeof(irqstr), "irq %d", irq);
			DPRINTF(("attach %d %d %s\n", i, irq, irqstr));
			evcnt_attach_dynamic(&bonito_intrhead[i].intr_count,
			    EVCNT_TYPE_INTR, NULL, "isa", irqstr);
			LIST_INIT(&bonito_intrhead[i].intrhand_head);
		}
	}
}

void
evbmips_iointr(int ppl, vaddr_t pc, uint32_t ipending)
{
	struct evbmips_intrhand *ih;
	int irq;
	uint32_t isr0, isr, imr;

	/*
	 * Read the interrupt pending registers, mask them with the
	 * ones we have enabled, and service them in order of decreasing
	 * priority.
	 */
	isr0 = REGVAL(BONITO_INTISR);
	imr = REGVAL(BONITO_INTEN);

	if (ipending & sys_platform->bonito_mips_intr) {
		isr = isr0 & imr & LOONGSON_INTRMASK_LVL4;

		REGVAL(BONITO_INTENCLR) = isr;
		(void)REGVAL(BONITO_INTENCLR);

		for (irq = 0; irq < BONITO_NINTS; irq++) {
			if ((isr & (1 << irq)) == 0)
				continue;
			bonito_intrhead[irq].intr_count.ev_count++;
			LIST_FOREACH (ih,
			    &bonito_intrhead[irq].intrhand_head, ih_q) {
				(*ih->ih_func)(ih->ih_arg);
			}
		}
		REGVAL(BONITO_INTENSET) = isr;
		(void)REGVAL(BONITO_INTENSET);
	}
	if (isr0 & LOONGSON_INTRMASK_INT0)
		sys_platform->isa_intr(ppl, pc, ipending);
}

void *
loongson_pciide_compat_intr_establish(void *v, device_t dev,
    const struct pci_attach_args *pa, int chan, int (*func)(void *), void *arg)
{
	pci_chipset_tag_t pc = pa->pa_pc; 
	void *cookie;
	int bus, irq;
	char buf[PCI_INTRSTR_LEN];

	pci_decompose_tag(pc, pa->pa_tag, &bus, NULL, NULL);

	/*
	 * If this isn't PCI bus #0, all bets are off.
	 */
	if (bus != 0)
		return (NULL);

	irq = PCIIDE_COMPAT_IRQ(chan);
#if NISA > 0
	if (sys_platform->isa_chipset != NULL)
		cookie = isa_intr_establish(sys_platform->isa_chipset, irq,
		    IST_EDGE, IPL_BIO, func, arg);
	else
#endif
		cookie = NULL;
	if (cookie == NULL)
		return (NULL);
	printf("%s: %s channel interrupting at %s\n", device_xname(dev),
	    PCIIDE_CHANNEL_NAME(chan),
	    isa_intr_string(sys_platform->isa_chipset, irq, buf, sizeof(buf)));
	return (cookie);
}

int
loongson_pci_intr_map(const struct pci_attach_args *pa,
    pci_intr_handle_t *ihp)
{
	pcitag_t bustag = pa->pa_intrtag;
	int buspin = pa->pa_intrpin;
	pci_chipset_tag_t pc = pa->pa_pc;
	int device, function;

	if (buspin == 0) {
		/* No IRQ used. */
		return (1);
	}

	if (buspin > 4) {
		printf("loongson_pci_intr_map: bad interrupt pin %d\n",
		    buspin);
		return (1);
	}

	pci_decompose_tag(pc, bustag, NULL, &device, &function);
	return (sys_platform->p_pci_intr_map(device, function, buspin, ihp));
}

const char *
loongson_pci_intr_string(void *v, pci_intr_handle_t ih, char *buf, size_t len)
{
	
	const struct bonito_config *bc = v;
	return loongson_intr_string(bc, ih, buf, len);
}

const struct evcnt *
loongson_pci_intr_evcnt(void *v, pci_intr_handle_t ih)
{

	return &bonito_intrhead[ih].intr_count;
}

void *
loongson_pci_intr_establish(void *v, pci_intr_handle_t ih, int level,
    int (*func)(void *), void *arg)
{
	if (BONITO_IRQ_IS_ISA(ih)) {
		if (sys_platform->isa_chipset == NULL)
			panic("ISA interrupt on non-ISA platform");
		return sys_platform->isa_chipset->ic_intr_establish(v,
		    BONITO_IRQ_TO_ISA(ih), IST_EDGE, level, func, arg);
	}
	return evbmips_intr_establish(ih, func, arg);
}

void
loongson_pci_intr_disestablish(void *v, void *cookie)
{
	struct evbmips_intrhand *ih = cookie;
	if (BONITO_IRQ_IS_ISA(ih->ih_irq)) {
		if (sys_platform->isa_chipset == NULL)
			panic("ISA interrupt on non-ISA platform");
		sys_platform->isa_chipset->ic_intr_disestablish(v, ih);
		return;
	}
	return (evbmips_intr_disestablish(cookie));
}

void
loongson_pci_conf_interrupt(void *v, int bus, int dev, int pin, int swiz,
    int *iline)
{

	/*
	 * We actually don't need to do anything; everything is handled
	 * in pci_intr_map().
	 */
	*iline = 0;
}


void *
evbmips_intr_establish(int irq, int (*func)(void *), void *arg)
{
	struct evbmips_intrhand *ih;
	int s;

	KASSERT(irq < BONITO_NINTS);
	DPRINTF(("loongson_intr_establish %d %p", irq, func));

	ih = malloc(sizeof(*ih), M_DEVBUF, M_NOWAIT|M_ZERO);
	if (ih == NULL)
		return NULL;

	ih->ih_func = func;
	ih->ih_arg = arg;
	ih->ih_irq = irq;
	DPRINTF((" ih %p", ih));

	/* link it into tables */
	s = splhigh();
	LIST_INSERT_HEAD(&bonito_intrhead[irq].intrhand_head, ih, ih_q);
	/* and enable it */
	DPRINTF((" inten 0x%x", REGVAL(BONITO_INTEN)));
	if (bonito_intrhead[irq].refcnt++ == 0 && !BONITO_IRQ_IS_ISA(irq))
		REGVAL(BONITO_INTENSET) = (1 << ih->ih_irq);
	DPRINTF((" now 0x%x\n", REGVAL(BONITO_INTEN)));
	splx(s);

	return (ih);
}

void
evbmips_intr_disestablish(void *cookie)
{
	struct evbmips_intrhand *ih = cookie;
	int s;

	s = splhigh();
	LIST_REMOVE(ih, ih_q);
	bonito_intrhead[ih->ih_irq].refcnt--;
	if (bonito_intrhead[ih->ih_irq].refcnt == 0 &&
	    !BONITO_IRQ_IS_ISA(ih->ih_irq))
		REGVAL(BONITO_INTENCLR) = (1 << ih->ih_irq);
	splx(s);
	free(ih, M_DEVBUF);
}

const char *
loongson_intr_string(const struct bonito_config *bc, int irq, char *buf, size_t len)
{
	if (BONITO_IRQ_IS_ISA(irq))
		snprintf(buf, len, "isa irq %d", BONITO_IRQ_TO_ISA(irq));
	else
		strlcpy(buf, sys_platform->irq_map[irq].name, len);
	return buf;
}
