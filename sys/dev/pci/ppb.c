/*	$NetBSD: ppb.c,v 1.34.22.5 2007/10/01 05:37:55 joerg Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: ppb.c,v 1.34.22.5 2007/10/01 05:37:55 joerg Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/ppbreg.h>
#include <dev/pci/pcidevs.h>

struct ppb_softc {
	struct device sc_dev;		/* generic device glue */
	pci_chipset_tag_t sc_pc;	/* our PCI chipset... */
	pcitag_t sc_tag;		/* ...and tag. */

	pcireg_t sc_pciconfext[48];
};

static void		ppb_resume(device_t);
static void		ppb_suspend(device_t);

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
ich_disable_sci(struct device *self, pci_chipset_tag_t pc, pcitag_t tag)
{
	pcireg_t val;

	/*
	 * Intel I/O Controller Hub 6 Family Datasheet
	 * Section 19.1.44
	 *
	 * Intel I/O Controller Hub 7 Family Datasheet
	 * Section 18.1.44
	 *
	 * Intel I/O Controller Hub 8 Family Datasheet
	 * Section 20.1.45
	 *
	 * Intel I/O Controller Hub 9 Family Datasheet
	 * Section 20.1.46
	 *
	 * Address Offset: D8
	 *
	 * Bit 31: Power Management SCI Enable
	 * Bit 30: Hot Plug SCI Enable
	 *
	 * Disable both as NetBSD currently can't deal with the interrupts.
	 */

	val = pci_conf_read(pc, tag, 0xd8);
	if ((val & 0xc000000) != 0) {
		aprint_normal("%s: disabling unsupported PM and Hot Plug SCI\n",
		    self->dv_xname);

		val &= ~(0xc000000);
		pci_conf_write(pc, tag, 0xd8, val);
	}
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
	pnp_status_t pnp_status;

	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo, sizeof(devinfo));
	aprint_normal(": %s (rev. 0x%02x)\n", devinfo,
	    PCI_REVISION(pa->pa_class));
	aprint_naive("\n");

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_INTEL &&
	    (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_INTEL_82801FB_EXP_0 ||
	     PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_INTEL_82801FB_EXP_1 ||
	     PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_INTEL_82801FB_EXP_2 ||
	     PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_INTEL_82801G_EXP_1 ||
	     PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_INTEL_82801G_EXP_2 ||
	     PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_INTEL_82801G_EXP_3 ||
	     PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_INTEL_82801G_EXP_4 ||
	     PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_INTEL_82801G_EXP_5 ||
	     PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_INTEL_82801G_EXP_6 ||
	     PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_INTEL_82801H_EXP_1 ||
	     PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_INTEL_82801H_EXP_2 ||
	     PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_INTEL_82801H_EXP_3 ||
	     PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_INTEL_82801H_EXP_4 ||
	     PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_INTEL_82801H_EXP_5 ||
	     PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_INTEL_82801H_EXP_6 ||
	     PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_INTEL_82801I_EXP_1 ||
	     PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_INTEL_82801I_EXP_2 ||
	     PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_INTEL_82801I_EXP_3 ||
	     PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_INTEL_82801I_EXP_4 ||
	     PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_INTEL_82801I_EXP_5 ||
	     PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_INTEL_82801I_EXP_6))
		ich_disable_sci(self, pc, pa->pa_tag);

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

	pnp_status = pci_generic_power_register(self, pa->pa_pc, pa->pa_tag,
	    ppb_suspend, ppb_resume);
	if (pnp_status != PNP_STATUS_SUCCESS) {
		aprint_error("%s: couldn't establish power handler\n",
		    device_xname(self));
	}

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

static void
ppb_resume(device_t dv)
{
	struct ppb_softc *sc = device_private(dv);
	int off;
	pcireg_t val;

        for (off = 0x40; off <= 0xff; off += 4) {
		val = pci_conf_read(sc->sc_pc, sc->sc_tag, off);
		if (val != sc->sc_pciconfext[(off - 0x40) / 4])
			pci_conf_write(sc->sc_pc, sc->sc_tag, off,
			    sc->sc_pciconfext[(off - 0x40)/4]);
	}
}

static void
ppb_suspend(device_t dv)
{
	struct ppb_softc *sc = device_private(dv);
	int off;

	for (off = 0x40; off <= 0xff; off += 4)
		sc->sc_pciconfext[(off - 0x40) / 4] =
		    pci_conf_read(sc->sc_pc, sc->sc_tag, off);
}

CFATTACH_DECL(ppb, sizeof(struct ppb_softc),
    ppbmatch, ppbattach, NULL, NULL);
