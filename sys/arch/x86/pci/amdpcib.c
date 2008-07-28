/* $NetBSD: amdpcib.c,v 1.2.8.1 2008/07/28 14:37:26 simonb Exp $ */

/*
 * Copyright (c) 2006 Nicolas Joly
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS
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
__KERNEL_RCSID(0, "$NetBSD: amdpcib.c,v 1.2.8.1 2008/07/28 14:37:26 simonb Exp $");

#include <sys/systm.h>
#include <sys/device.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>
#include "pcibvar.h"

struct amdpcib_softc {
	/* we are calling pcibattach(), which assumes this starts like this: */
	struct pcib_softc	sc_pcib;
	bus_space_tag_t		sc_memt;
	bus_space_handle_t	sc_memh;
};

static int	amdpcib_match(device_t, cfdata_t, void *);
static void	amdpcib_attach(device_t, device_t, void *);
static int	amdpcib_search(device_t, cfdata_t, const int *, void *);

CFATTACH_DECL_NEW(amdpcib, sizeof(struct amdpcib_softc), amdpcib_match,
    amdpcib_attach, NULL, NULL);

static int
amdpcib_match(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa = aux;

	if ((PCI_VENDOR(pa->pa_id) == PCI_VENDOR_AMD) &&
	    (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_AMD_PBC8111_LPC))
		return 2;

	return 0;
}

static void
amdpcib_attach(device_t parent, device_t self, void *aux)
{
	struct pci_attach_args *pa = aux;

	pcibattach(parent, self, aux);
	config_search_loc(amdpcib_search, self, "amdpcib", NULL, pa);
}

static int
amdpcib_search(device_t parent, cfdata_t cf, const int *locs, void *aux)
{

	if (config_match(parent, cf, aux))
		config_attach_loc(parent, cf, locs, aux, NULL);

	return 0;
}
