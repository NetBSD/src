/*	$NetBSD: ppb.c,v 1.34.22.1 2007/08/03 22:17:21 jmcneill Exp $	*/

/*
 * Copyright (c) 1996, 1998 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ppb.c,v 1.34.22.1 2007/08/03 22:17:21 jmcneill Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/ppbreg.h>

struct ppb_softc {
	struct device sc_dev;		/* generic device glue */
	pci_chipset_tag_t sc_pc;	/* our PCI chipset... */
	pcitag_t sc_tag;		/* ...and tag. */

	struct pci_conf_state sc_pciconf;
};

static pnp_status_t	ppb_power(device_t, pnp_request_t, void *);

static int
ppbmatch(struct device *parent, struct cfdata *match,
    void *aux)
{
	struct pci_attach_args *pa = aux;

	/*
	 * Check the ID register to see that it's a PCI bridge.
	 * If it is, we assume that we can deal with it; it _should_
	 * work in a standardized way...
	 */
	if (PCI_CLASS(pa->pa_class) == PCI_CLASS_BRIDGE &&
	    PCI_SUBCLASS(pa->pa_class) == PCI_SUBCLASS_BRIDGE_PCI)
		return (1);

	return (0);
}

static void
ppbattach(struct device *parent, struct device *self, void *aux)
{
	struct ppb_softc *sc = (void *) self;
	struct pci_attach_args *pa = aux;
	pci_chipset_tag_t pc = pa->pa_pc;
	struct pcibus_attach_args pba;
	pcireg_t busdata;
	char devinfo[256];

	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo, sizeof(devinfo));
	aprint_normal(": %s (rev. 0x%02x)\n", devinfo,
	    PCI_REVISION(pa->pa_class));
	aprint_naive("\n");

	sc->sc_pc = pc;
	sc->sc_tag = pa->pa_tag;

	busdata = pci_conf_read(pc, pa->pa_tag, PPB_REG_BUSINFO);

	if (PPB_BUSINFO_SECONDARY(busdata) == 0) {
		aprint_normal("%s: not configured by system firmware\n",
		    self->dv_xname);
		return;
	}

#if 0
	/*
	 * XXX can't do this, because we're not given our bus number
	 * (we shouldn't need it), and because we've no way to
	 * decompose our tag.
	 */
	/* sanity check. */
	if (pa->pa_bus != PPB_BUSINFO_PRIMARY(busdata))
		panic("ppbattach: bus in tag (%d) != bus in reg (%d)",
		    pa->pa_bus, PPB_BUSINFO_PRIMARY(busdata));
#endif

	if (pnp_register(self, ppb_power) != PNP_STATUS_SUCCESS)
		aprint_error("%s: couldn't establish power handler\n",
		    device_xname(self));

	/*
	 * Attach the PCI bus than hangs off of it.
	 *
	 * XXX Don't pass-through Memory Read Multiple.  Should we?
	 * XXX Consult the spec...
	 */
	pba.pba_iot = pa->pa_iot;
	pba.pba_memt = pa->pa_memt;
	pba.pba_dmat = pa->pa_dmat;
	pba.pba_dmat64 = pa->pa_dmat64;
	pba.pba_pc = pc;
	pba.pba_flags = pa->pa_flags & ~PCI_FLAGS_MRM_OKAY;
	pba.pba_bus = PPB_BUSINFO_SECONDARY(busdata);
	pba.pba_bridgetag = &sc->sc_tag;
	pba.pba_intrswiz = pa->pa_intrswiz;
	pba.pba_intrtag = pa->pa_intrtag;

	config_found_ia(self, "pcibus", &pba, pcibusprint);
}

static pnp_status_t
ppb_power(device_t dv, pnp_request_t req, void *opaque)
{
	struct ppb_softc *sc;
	pnp_capabilities_t *pcaps;
	pnp_state_t *pstate;

	sc = (struct ppb_softc *)dv;
	switch (req) {
	case PNP_REQUEST_GET_CAPABILITIES:
		pcaps = opaque;
		pcaps->state = PNP_STATE_D0 | PNP_STATE_D3;
		break;
	case PNP_REQUEST_GET_STATE:
		pstate = opaque;
		*pstate = PNP_STATE_D0; /* XXX */
		break;
	case PNP_REQUEST_SET_STATE:
		pstate = opaque;
		switch (*pstate) {
		case PNP_STATE_D0:
			pci_conf_restore(sc->sc_pc, sc->sc_tag,
			    &sc->sc_pciconf);
			break;
		case PNP_STATE_D3:
			pci_conf_capture(sc->sc_pc, sc->sc_tag,
			    &sc->sc_pciconf);
			break;
		default:
			return PNP_STATUS_UNSUPPORTED;
		}
		break;
	default:
		return PNP_STATUS_UNSUPPORTED;
	}

	return PNP_STATUS_SUCCESS;
}

CFATTACH_DECL(ppb, sizeof(struct ppb_softc),
    ppbmatch, ppbattach, NULL, NULL);
