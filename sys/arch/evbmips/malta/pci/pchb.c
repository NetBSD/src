/*	$NetBSD: pchb.c,v 1.1 2002/03/07 14:44:05 simonb Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Simon Burge for Wasabi Systems, Inc.
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

#include <sys/types.h>
#include <sys/device.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

static int	pchb_match(struct device *, struct cfdata *, void *);
static void	pchb_attach(struct device *, struct device *, void *);

struct cfattach pchb_ca = {
	sizeof(struct device), pchb_match, pchb_attach
};

static int pcifound = 0;

static int
pchb_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct pci_attach_args *pa = aux;

	/*
	 * Match all known PCI host chipsets.
	 */
	if (PCI_CLASS(pa->pa_class) == PCI_CLASS_BRIDGE &&
	    PCI_SUBCLASS(pa->pa_class) == PCI_SUBCLASS_BRIDGE_HOST) {
		switch (PCI_VENDOR(pa->pa_id)) {
		case PCI_VENDOR_GALILEO:
			switch (PCI_PRODUCT(pa->pa_id)) {
			case PCI_PRODUCT_GALILEO_GT64120:
				return (!pcifound);
			}
			break;
		}
	}
	return (0);
}

static void
pchb_attach(struct device *parent, struct device *self, void *aux)
{
	struct pci_attach_args *pa = aux;
	char devinfo[256];

	printf("\n");
	pcifound++;

	/*
	 * All we do is print out a description.  Eventually, we
	 * might want to add code that does something that's
	 * possibly chipset-specific.
	 */
	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_GALILEO &&
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_GALILEO_GT64120) {
		/* Bah, same product ID... */

		/*
		 * XXX:	Is the >= 0x10 test correct?  The '120 doco
		 *	lists rev == 0x02 and the '120A doco lists
		 *	rev == 0x10.
		 */
		snprintf(devinfo, sizeof(devinfo),
		    "Galileo Technology GT-64120%s System Controller",
		    PCI_REVISION(pa->pa_class) >= 0x10 ? "A" : "");
	} else {
		pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo);
	}
	printf("%s: %s (rev. 0x%02x)\n", self->dv_xname, devinfo,
	    PCI_REVISION(pa->pa_class));
}
