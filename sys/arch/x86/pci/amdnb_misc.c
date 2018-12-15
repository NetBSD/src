/*	$NetBSD: amdnb_misc.c,v 1.2.36.1 2018/12/15 13:38:59 martin Exp $ */
/*
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christoph Egger.
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
__KERNEL_RCSID(0, "$NetBSD: amdnb_misc.c,v 1.2.36.1 2018/12/15 13:38:59 martin Exp $");

#include <sys/param.h>
#include <sys/device.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

static int amdnb_misc_match(device_t, cfdata_t match, void *);
static void amdnb_misc_attach(device_t, device_t, void *);
static int amdnb_misc_detach(device_t, int);
static int amdnb_misc_rescan(device_t, const char *, const int *);
static void amdnb_misc_childdet(device_t, device_t);

struct amdnb_misc_softc {
	struct pci_attach_args sc_pa;
};

CFATTACH_DECL3_NEW(amdnb_misc, sizeof(struct amdnb_misc_softc),
    amdnb_misc_match, amdnb_misc_attach, amdnb_misc_detach, NULL,
    amdnb_misc_rescan, amdnb_misc_childdet, DVF_DETACH_SHUTDOWN);


static int
amdnb_misc_match(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa = aux;

	if (PCI_VENDOR(pa->pa_id) != PCI_VENDOR_AMD)
		return 0;

	switch (PCI_PRODUCT(pa->pa_id)) {
	case PCI_PRODUCT_AMD_AMD64_MISC:
	case PCI_PRODUCT_AMD_AMD64_F10_MISC:
	case PCI_PRODUCT_AMD_AMD64_F11_MISC:
	case PCI_PRODUCT_AMD_F14_NB:		/* Family 12h, too */
	case PCI_PRODUCT_AMD_F15_MISC:
	case PCI_PRODUCT_AMD_F16_NB:
	case PCI_PRODUCT_AMD_F16_30_NB:
		break;
	default:
		return 0;
	}

	return 2;	/* supercede pchb(4) */
}

static int
amdnb_misc_search(device_t parent, cfdata_t cf, const int *locs, void *aux)
{
	device_t dev;
	deviter_t di;
	bool attach;

	if (!config_match(parent, cf, aux))
		return 0;

	attach = true;

	/* Figure out if found child 'cf' is already attached.
	 * No need to attach it twice.
	 */

	/* XXX: I only want to iterate over the children of *this* device.
	 * Can we introduce a
	 * deviter_first_child(&di, parent, DEVITER_F_LEAVES_ONLY)
	 * or even better, can we introduce a query function that returns
	 * if a child is already attached?
	 */
	for (dev = deviter_first(&di, DEVITER_F_LEAVES_FIRST); dev != NULL;
	    dev = deviter_next(&di))
	{
		if (device_parent(dev) != parent)
			continue;
		if (device_is_a(dev, cf->cf_name)) {
			attach = false;
			break;
		}
	}
	deviter_release(&di);

	if (!attach)
		return 0;

	config_attach_loc(parent, cf, locs, aux, NULL);

	return 0;
}

static void
amdnb_misc_attach(device_t parent, device_t self, void *aux)
{
	struct amdnb_misc_softc *sc = device_private(self);
	struct pci_attach_args *pa = aux;

	sc->sc_pa = *pa;

	aprint_naive("\n");
	aprint_normal(": AMD NB Misc Configuration\n");

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");

	config_search_loc(amdnb_misc_search, self, "amdnb_miscbus",
	    NULL, &sc->sc_pa);

	return;
}

static int
amdnb_misc_detach(device_t self, int flags)
{
	int rv;

	rv = config_detach_children(self, flags);
	if (rv != 0)
		return rv;

	pmf_device_deregister(self);
	return rv;
}

static int
amdnb_misc_rescan(device_t self, const char *ifattr, const int *locators)
{
	struct amdnb_misc_softc *sc = device_private(self);

	if (!ifattr_match(ifattr, "amdnb_miscbus"))
		return 0;

	config_search_loc(amdnb_misc_search, self, ifattr,
	    locators, &sc->sc_pa);

	return 0;
}

static void
amdnb_misc_childdet(device_t self, device_t child)
{
	return;
}
