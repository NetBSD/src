/*	$NetBSD: pchb.c,v 1.1 2000/03/19 23:07:48 soren Exp $	*/

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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>

#include <dev/pci/pcidevs.h>

static int	pchb_match(struct device *, struct cfdata *, void *);
static void	pchb_attach(struct device *, struct device *, void *);

struct cfattach pchb_ca = {
	sizeof(struct device), pchb_match, pchb_attach
};

static int
pchb_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pci_attach_args *pa = aux;

	if ((PCI_VENDOR(pa->pa_id) == PCI_VENDOR_GALILEO) &&
	    (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_GALILEO_GT64011))
		return 1;

	return 0;
}

static void
pchb_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct pci_attach_args *pa = aux;
	char devinfo[256];
	pcireg_t bhlc;

	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo);
	printf("\n%s: %s, rev %d\n", self->dv_xname, devinfo,
					PCI_REVISION(pa->pa_class));

	/*
	 * XXX Still isn't quite right or enough.
	 */

	bhlc = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_BHLC_REG);

	bhlc &= ~(PCI_LATTIMER_MASK << PCI_LATTIMER_SHIFT);
	bhlc |= (0x40 << PCI_LATTIMER_SHIFT);

	bhlc &= ~(PCI_CACHELINE_MASK << PCI_CACHELINE_SHIFT);
	bhlc |= (7 << PCI_CACHELINE_SHIFT);

	pci_conf_write(pa->pa_pc, pa->pa_tag, PCI_BHLC_REG, bhlc);
}
