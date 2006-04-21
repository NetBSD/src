/*	$NetBSD: pcib.c,v 1.13 2006/04/21 15:46:07 tsutsui Exp $	*/

/*
 * Copyright (c) 2000 Soren S. Jorvang.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pcib.c,v 1.13 2006/04/21 15:46:07 tsutsui Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/autoconf.h>
#include <machine/intr.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#include <dev/isa/isareg.h>

#define PCIB_BASE	0x10000000	/* XXX */

static int	pcib_match(struct device *, struct cfdata *, void *);
static void	pcib_attach(struct device *, struct device *, void *);
static int	icu_intr(void *);

CFATTACH_DECL(pcib, sizeof(struct device),
    pcib_match, pcib_attach, NULL, NULL);

static struct cobalt_intrhand icu[IO_ICUSIZE];

static int
pcib_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct pci_attach_args *pa = aux;

	if ((PCI_VENDOR(pa->pa_id) == PCI_VENDOR_VIATECH) &&
	    (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_VIATECH_VT82C586_ISA))
		return 1;

	return 0;
}

static void
pcib_attach(struct device *parent, struct device *self, void *aux)
{
	struct pci_attach_args *pa = aux;
	char devinfo[256];

	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo, sizeof(devinfo));
	printf("\n%s: %s, rev %d\n", self->dv_xname, devinfo,
	    PCI_REVISION(pa->pa_class));

	/*
	 * Initialize ICU. Since we block all these interrupts with
	 * splbio(), we can just enable all of them all the time here.
	 */
	*(volatile uint8_t *)MIPS_PHYS_TO_KSEG1(PCIB_BASE + IO_ICU1) = 0x10;
	*(volatile uint8_t *)MIPS_PHYS_TO_KSEG1(PCIB_BASE + IO_ICU1 + 1) = 0xff;
	*(volatile uint8_t *)MIPS_PHYS_TO_KSEG1(PCIB_BASE + IO_ICU2) = 0x10;
	*(volatile uint8_t *)MIPS_PHYS_TO_KSEG1(PCIB_BASE + IO_ICU2 + 1) = 0xff;
	wbflush();

	cpu_intr_establish(4, IPL_NONE, icu_intr, NULL);
}

void *
icu_intr_establish(int irq, int type, int level, int (*func)(void *),
    void *arg)
{
	int i;

	for (i = 0; i < IO_ICUSIZE; i++) {
		if (icu[i].ih_func == NULL) {
			icu[i].cookie_type = COBALT_COOKIE_TYPE_ICU;
			icu[i].ih_func = func;
			icu[i].ih_arg = arg;
			return &icu[i];
		}
	}

	panic("too many IRQs");
}

void
icu_intr_disestablish(void *cookie)
{
	struct cobalt_intrhand *ih = cookie;

	if (ih->cookie_type == COBALT_COOKIE_TYPE_ICU) {
		ih->ih_func = NULL;
		ih->ih_arg = NULL;
	}
}

int
icu_intr(void *arg)
{
	int i;

	for (i = 0; i < IO_ICUSIZE; i++) {
		if (icu[i].ih_func == NULL)
			return 0;

		(*icu[i].ih_func)(icu[i].ih_arg);
	}

	return 0;
}
