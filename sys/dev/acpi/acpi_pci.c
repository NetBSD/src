/* $NetBSD $ */

/*
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
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
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * ACPI PCI Bus
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: acpi_pci.c,v 1.1 2009/12/03 21:04:29 cegger Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <sys/bus.h>
#include <sys/kmem.h>
#include <dev/pci/pcivar.h>

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_pci.h>

struct acpi_pcidev;

static TAILQ_HEAD(, acpi_pcidev) acpi_pcidevlist =
    TAILQ_HEAD_INITIALIZER(acpi_pcidevlist);

struct acpi_pcidev {
	struct acpi_devnode *ap_node;
	uint32_t ap_pciseg;
	uint32_t ap_pcibus;
	uint32_t ap_pcidev;
	uint32_t ap_pcifunc;
	bool ap_pcihost;
	TAILQ_ENTRY(acpi_pcidev) ap_list;
};


static const char * const acpi_pcidev_ids[] = {
	"PNP0A??",	/* ACPI PCI host controllers */
	NULL
};

static bool
acpi_pcidev_add(struct acpi_softc *sc, struct acpi_devnode *ad)
{
	struct acpi_pcidev *ap;
	ACPI_STATUS rv;
	ACPI_INTEGER seg, bus, addr;

	/*
	 * ACPI spec: "The _BBN object is located under a
	 * PCI host bridge and must be unique for every
	 * host bridge within a segment since it is the PCI bus number."
	 */
	rv = acpi_eval_integer(ad->ad_handle, "_BBN", &bus);
	if (ACPI_FAILURE(rv))
		return false;
	/*
         * The ACPI address (_ADR) is equal to: (device << 16) | function.
	 */
	rv = acpi_eval_integer(ad->ad_handle, "_ADR", &addr);
	if (ACPI_FAILURE(rv))
		return false;

	/*
	 * ACPI spec: "The optional _SEG object is located under a PCI host
	 * bridge and evaluates to an integer that describes the
	 * PCI Segment Group (see PCI Firmware Specification v3.0)."
	 *
	 * "PCI Segment Group supports more than 256 buses
	 * in a system by allowing the reuse of the PCI bus numbers.
	 * Within each PCI Segment Group, the bus numbers for the PCI
	 * buses must be unique. PCI buses in different PCI Segment
	 * Group are permitted to have the same bus number."
	 */
	rv = acpi_eval_integer(ad->ad_handle, "_SEG", &seg);
	if (ACPI_FAILURE(rv)) {
		/*
		 * ACPI spec: "If _SEG does not exist, OSPM assumes that all
		 * PCI bus segments are in PCI Segment Group 0."
		 */
		seg = 0;
	}

	ap = kmem_alloc(sizeof(*ap), KM_SLEEP);
	if (ap == NULL) {
		aprint_error("%s: kmem_alloc failed\n", __func__);
		return false;
	}

	if (acpi_match_hid(ad->ad_devinfo, acpi_pcidev_ids))
		ap->ap_pcihost = true;
	else
		ap->ap_pcihost = false;

	ap->ap_node = ad;
	ap->ap_pciseg = seg;
	ap->ap_pcibus = bus;
	ap->ap_pcidev = addr >> 16;
	ap->ap_pcifunc = addr & 0xffff;

	TAILQ_INSERT_TAIL(&acpi_pcidevlist, ap, ap_list);

	return true;
}

static void
acpi_pcidev_print(struct acpi_pcidev *ap)
{
	aprint_debug(" %s", ap->ap_node->ad_name);
}

int
acpi_pcidev_scan(struct acpi_softc *sc)
{
	struct acpi_scope *as;
	struct acpi_devnode *ad;
	struct acpi_pcidev *ap;
	ACPI_DEVICE_INFO *di;
	int count = 0;

#define ACPI_STA_DEV_VALID      \
	(ACPI_STA_DEV_PRESENT|ACPI_STA_DEV_ENABLED|ACPI_STA_DEV_OK)

	TAILQ_FOREACH(as, &sc->sc_scopes, as_list) {
		TAILQ_FOREACH(ad, &as->as_devnodes, ad_list) {
			di = ad->ad_devinfo;
			if (di->Type != ACPI_TYPE_DEVICE)
				continue;
			if ((di->Valid & ACPI_VALID_STA) != 0 &&
			    (di->CurrentStatus & ACPI_STA_DEV_VALID) !=
			     ACPI_STA_DEV_VALID)
				continue;
			if (acpi_pcidev_add(sc, ad) == true)
				++count;
		}
	}

#undef ACPI_STA_DEV_VALID

	if (count == 0)
		return 0;

	aprint_debug_dev(sc->sc_dev, "pci devices:");
	TAILQ_FOREACH(ap, &acpi_pcidevlist, ap_list)
		acpi_pcidev_print(ap);
	aprint_debug("\n");

	return count;
}

/*
 * acpi_pcidev_find:
 *
 *      Finds a PCI device in the ACPI name space.
 *      The return status is either:
 *      - AE_NOT_FOUND if no such device was found.
 *      - AE_OK if one and only one such device was found.
 */
ACPI_STATUS
acpi_pcidev_find(u_int segment, u_int bus, u_int device, u_int function,
    ACPI_HANDLE *handlep)
{
	struct acpi_pcidev *ap;
	ACPI_HANDLE hdl;

	hdl = NULL;
	TAILQ_FOREACH(ap, &acpi_pcidevlist, ap_list) {
		if (ap->ap_pciseg != segment)
			continue;
		if (ap->ap_pcibus != bus)
			continue;
		if (ap->ap_pcidev != device)
			continue;
		if (ap->ap_pcifunc != function)
			continue;

		hdl = ap->ap_node->ad_handle;
		break;
	}

	*handlep = hdl;
	return (hdl != NULL) ? AE_OK : AE_NOT_FOUND;
}
