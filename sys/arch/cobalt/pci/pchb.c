/*	$NetBSD: pchb.c,v 1.7.6.1 2006/04/22 11:37:21 simonb Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: pchb.c,v 1.7.6.1 2006/04/22 11:37:21 simonb Exp $");

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

CFATTACH_DECL(pchb, sizeof(struct device),
    pchb_match, pchb_attach, NULL, NULL);

static int
pchb_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct pci_attach_args *pa = aux;

	if ((PCI_VENDOR(pa->pa_id) == PCI_VENDOR_GALILEO) &&
	    (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_GALILEO_GT64011))
		return 1;

	return 0;
}

static void
pchb_attach(struct device *parent, struct device *self, void *aux)
{
	struct pci_attach_args *pa = aux;
	int major, minor;

	major = PCI_REVISION(pa->pa_class) >> 4;
	minor = PCI_REVISION(pa->pa_class) & 0x0f;

	if (major == 0)
		printf(": Galileo GT-64011 System Controller, rev %d\n", minor);
	else
		printf(": Galileo GT-64111 System Controller, rev %d\n", minor);
}
