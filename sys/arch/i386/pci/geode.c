/*	$NetBSD: geode.c,v 1.8.42.1 2008/01/08 22:10:05 bouyer Exp $	*/

/*-
 * Copyright (c) 2005 David Young.  All rights reserved.
 *
 * This code was written by David Young and merged with geodecntr code
 * by Frank Kardel.
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
 *	This product includes software developed by David Young.
 * 4. The name of David Young may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY DAVID YOUNG ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL DAVID
 * YOUNG BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 */
/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
 * Device driver for the special devices attached to the GBA (watchdog, high
 * resolution counter, ...) in the AMD Geode SC1100 processor.
 */

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: geode.c,v 1.8.42.1 2008/01/08 22:10:05 bouyer Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/wdog.h>
#include <uvm/uvm_extern.h>
#include <machine/bus.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>
#include <arch/i386/pci/geodevar.h>
#include <arch/i386/pci/geodereg.h>
#include <dev/sysmon/sysmonvar.h>

#ifdef GEODE_DEBUG
#define	GEODE_DPRINTF(__x) printf __x
#else /* GEODE_DEBUG */
#define	GEODE_DPRINTF(__x) /* nothing */
#endif

static int
geode_gcb_match(struct device *parent, struct cfdata *match,
    void *aux)
{
	struct pci_attach_args *pa = aux;

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_NS &&
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_NS_SC1100_XBUS)
		return (10);	/* XXX beat pchb? -dyoung */

	return (0);
}

static void
geode_gcb_attach(struct device *parent, struct device *self, void *aux)
{
	struct geode_gcb_softc *sc = (void *) self;
	struct pci_attach_args *pa = aux;
	uint32_t cba;
	u_int rev;

	cba = pci_conf_read(pa->pa_pc, pa->pa_tag, SC1100_XBUS_CBA_SCRATCHPAD);
	sc->sc_iot = pa->pa_iot;
	if (bus_space_map(sc->sc_iot, (bus_addr_t)cba, SC1100_GCB_SIZE, 0,
	    &sc->sc_ioh) != 0) {
		aprint_error("%s: unable to map registers\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	GEODE_DPRINTF(("%s: mapped %u bytes at %#08" PRIx32 "\n",
	    sc->sc_dev.dv_xname, SC1100_GCB_SIZE, cba));

	rev = bus_space_read_1(sc->sc_iot, sc->sc_ioh, SC1100_GCB_REV_B);

	aprint_normal(": AMD Geode GCB (rev. 0x%02x)\n", rev);

	while (config_found(self, NULL, NULL) != NULL)
		/* empty */;
}
static int
geode_gcb_detach(device_t self, int flags)
{
	int rc;
	struct geode_gcb_softc *sc = device_private(self);

	if ((rc = config_detach_children(self, flags)) != 0)
		return rc;

	bus_space_unmap(sc->sc_iot, sc->sc_ioh, SC1100_GCB_SIZE);
	return 0;
}

static void
geode_gcb_childdetached(device_t parent, device_t child)
{
	/* We hold no references to child devices, so there is nothing
	 * to do.
	 */
}

CFATTACH_DECL2(geodegcb, sizeof(struct geode_gcb_softc),
    geode_gcb_match, geode_gcb_attach, geode_gcb_detach, NULL, NULL,
    geode_gcb_childdetached);
