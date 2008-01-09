/*	$NetBSD: if_rtk_pci.c,v 1.32.10.2 2008/01/09 01:53:49 matt Exp $	*/

/*
 * Copyright (c) 1997, 1998
 *	Bill Paul <wpaul@ctr.columbia.edu>.  All rights reserved.
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
 *	This product includes software developed by Bill Paul.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Bill Paul AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Bill Paul OR THE VOICES IN HIS HEAD
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 *	FreeBSD Id: if_rl.c,v 1.17 1999/06/19 20:17:37 wpaul Exp
 */

/*
 * Realtek 8129/8139 PCI NIC driver
 *
 * Supports several extremely cheap PCI 10/100 adapters based on
 * the Realtek chipset. Datasheets can be obtained from
 * www.realtek.com.tw.
 *
 * Written by Bill Paul <wpaul@ctr.columbia.edu>
 * Electrical Engineering Department
 * Columbia University, New York City
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_rtk_pci.c,v 1.32.10.2 2008/01/09 01:53:49 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/device.h>
#include <sys/sockio.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/socket.h>

#include <net/if.h>
#include <net/if_arp.h>
#include <net/if_ether.h>
#include <net/if_dl.h>
#include <net/if_media.h>

#include <sys/bus.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>

#include <dev/ic/rtl81x9reg.h>
#include <dev/ic/rtl81x9var.h>

struct rtk_pci_softc {
	struct rtk_softc sc_rtk;	/* real rtk softc */

	/* PCI-specific goo.*/
	void *sc_ih;
};

static const struct rtk_type rtk_pci_devs[] = {
	{ PCI_VENDOR_REALTEK, PCI_PRODUCT_REALTEK_RT8129,
		RTK_8129, "Realtek 8129 10/100BaseTX" },
	{ PCI_VENDOR_REALTEK, PCI_PRODUCT_REALTEK_RT8139,
		RTK_8139, "Realtek 8139 10/100BaseTX" },
	{ PCI_VENDOR_ACCTON, PCI_PRODUCT_ACCTON_MPX5030,
		RTK_8139, "Accton MPX 5030/5038 10/100BaseTX" },
	{ PCI_VENDOR_DELTA, PCI_PRODUCT_DELTA_8139,
		RTK_8139, "Delta Electronics 8139 10/100BaseTX" },
	{ PCI_VENDOR_ADDTRON, PCI_PRODUCT_ADDTRON_8139,
		RTK_8139, "Addtron Technology 8139 10/100BaseTX" },
	{ PCI_VENDOR_SEGA, PCI_PRODUCT_SEGA_BROADBAND,
		RTK_8139, "SEGA Broadband Adapter" },
	{ PCI_VENDOR_DLINK, PCI_PRODUCT_DLINK_DFE530TXPLUS,
		RTK_8139, "D-Link Systems DFE 530TX+" },
	{ PCI_VENDOR_NORTEL, PCI_PRODUCT_NORTEL_BAYSTACK_21,
		RTK_8139, "Baystack 21 (MPX EN5038) 10/100BaseTX" },
	{ 0, 0, 0, NULL }
};

static int	rtk_pci_match(device_t, struct cfdata *, void *);
static void	rtk_pci_attach(device_t, device_t, void *);

CFATTACH_DECL(rtk_pci, sizeof(struct rtk_pci_softc),
    rtk_pci_match, rtk_pci_attach, NULL, NULL);

static const struct rtk_type *
rtk_pci_lookup(const struct pci_attach_args *pa)
{
	const struct rtk_type *t;

	for (t = rtk_pci_devs; t->rtk_name != NULL; t++) {
		if (PCI_VENDOR(pa->pa_id) == t->rtk_vid &&
		    PCI_PRODUCT(pa->pa_id) == t->rtk_did) {
			return (t);
		}
	}
	return (NULL);
}

static int
rtk_pci_match(device_t parent, struct cfdata *match,
    void *aux)
{
	struct pci_attach_args *pa = aux;

	if (rtk_pci_lookup(pa) != NULL)
		return (1);

	return (0);
}

/*
 * Attach the interface. Allocate softc structures, do ifmedia
 * setup and ethernet/BPF attach.
 */
static void
rtk_pci_attach(device_t parent, device_t self, void *aux)
{
	struct rtk_pci_softc *psc = device_private(self);
	struct rtk_softc *sc = &psc->sc_rtk;
	struct pci_attach_args *pa = aux;
	pci_chipset_tag_t pc = pa->pa_pc;
	pci_intr_handle_t ih;
	bus_space_tag_t iot, memt;
	bus_space_handle_t ioh, memh;
	const char *intrstr = NULL;
	const struct rtk_type *t;
	int ioh_valid, memh_valid;

	t = rtk_pci_lookup(pa);
	if (t == NULL) {
		printf("\n");
		panic("rtk_pci_attach: impossible");
	}

	aprint_naive("\n");
	aprint_normal(": %s (rev. 0x%02x)\n",
		      t->rtk_name, PCI_REVISION(pa->pa_class));

	/*
	 * Map control/status registers.
	 *
	 * The original FreeBSD's driver has the following comment:
	 *
	 *   Default to using PIO access for this driver. On SMP systems,
	 *   there appear to be problems with memory mapped mode:
	 *   it looks like doing too many memory mapped access
	 *   back to back in rapid succession can hang the bus.
	 *   I'm inclined to blame this on crummy design/construction
	 *   on the part of Realtek. Memory mapped mode does appear to
	 *   work on uniprocessor systems though.
	 *
	 * On NetBSD, some port doesn't support PCI I/O space properly,
	 * so we try to map both and prefer I/O space to mem space.
	 */
	ioh_valid = (pci_mapreg_map(pa, RTK_PCI_LOIO, PCI_MAPREG_TYPE_IO, 0,
	    &iot, &ioh, NULL, NULL) == 0);
	memh_valid = (pci_mapreg_map(pa, RTK_PCI_LOMEM, PCI_MAPREG_TYPE_MEM, 0,
	    &memt, &memh, NULL, NULL) == 0);
	if (ioh_valid) {
		sc->rtk_btag = iot;
		sc->rtk_bhandle = ioh;
	} else if (memh_valid) {
		sc->rtk_btag = memt;
		sc->rtk_bhandle = memh;
	} else {
		aprint_error_dev(self, "can't map registers\n");
		return;
	}

	/* Allocate interrupt */
	if (pci_intr_map(pa, &ih)) {
		aprint_error_dev(self, "couldn't map interrupt\n");
		return;
	}
	intrstr = pci_intr_string(pc, ih);
	psc->sc_ih = pci_intr_establish(pc, ih, IPL_NET, rtk_intr, sc);
	if (psc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt");
		if (intrstr != NULL)
			aprint_normal(" at %s", intrstr);
		aprint_normal("\n");
		return;
	}

	if (t->rtk_basetype == RTK_8129)
		sc->sc_quirk |= RTKQ_8129;

	aprint_normal_dev(self, "interrupting at %s\n", intrstr);

	sc->sc_dmat = pa->pa_dmat;
	sc->sc_flags |= RTK_ENABLED;

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");
	else
		pmf_class_network_register(self, &sc->ethercom.ec_if);

	rtk_attach(sc);
}
