/*	$NetBSD: nappi_nppb.c,v 1.1.2.2 2002/07/21 13:00:35 gehenna Exp $ */
/*
 * Copyright (c) 2002
 *	Ichiro FUKUHARA <ichiro@ichiro.org>.
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
 *	This product includes software developed by Ichiro FUKUHARA.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ICHIRO FUKUHARA ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ICHIRO FUKUHARA OR THE VOICES IN HIS HEAD BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "pci.h"
#include "opt_pci.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/malloc.h>

#include <machine/bus.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pciconf.h>

static int	nppbmatch(struct device *, struct cfdata *, void *);
static void	nppbattach(struct device *, struct device *, void *);

struct cfattach nppb_ca = {
	sizeof(struct device), nppbmatch, nppbattach
};

static int
nppbmatch(struct device *parent, struct cfdata *cf, void *aux)
{
	struct pci_attach_args *pa = aux;
	u_int32_t class, id;

	class = pa->pa_class;
	id = pa->pa_id;

	if (PCI_CLASS(class) == PCI_CLASS_BRIDGE &&
	    PCI_SUBCLASS(class) == PCI_SUBCLASS_BRIDGE_MISC) {
#ifdef PCI_DEBUG
	printf("pci vendor = 0x%08x\n", PCI_VENDOR(id));
	printf("pci class = 0x%08x\n", PCI_CLASS(class));
	printf("pci subclass = 0x%08x\n", PCI_SUBCLASS(class));
#endif
		switch (PCI_VENDOR(id)) {
		case PCI_VENDOR_INTEL:
			switch (PCI_PRODUCT(id)) {
			case PCI_PRODUCT_INTEL_21555:
			    return(1);
			}
			break;
		}
	}
	return(0);
}

static void
nppbattach(struct device *parent, struct device *self, void *aux)
{
	struct pci_attach_args *pa = aux;
	char devinfo[256];

	printf("\n");

	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo);
	printf("%s: %s (rev. 0x%02x)\n", self->dv_xname, devinfo,
		PCI_REVISION(pa->pa_class));
}
