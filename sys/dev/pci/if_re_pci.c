/*	$NetBSD: if_re_pci.c,v 1.35.8.1 2009/05/13 17:20:26 jym Exp $	*/

/*
 * Copyright (c) 1997, 1998-2003
 *	Bill Paul <wpaul@windriver.com>.  All rights reserved.
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
 */

/* $FreeBSD: /repoman/r/ncvs/src/sys/dev/re/if_re.c,v 1.20 2004/04/11 20:34:08 ru Exp $ */

/*
 * RealTek 8139C+/8169/8169S/8110S PCI NIC driver
 *
 * Written by Bill Paul <wpaul@windriver.com>
 * Senior Networking Software Engineer
 * Wind River Systems
 *
 * NetBSD bus-specific frontends for written by
 * Jonathan Stone <jonathan@netbsd.org>
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_re_pci.c,v 1.35.8.1 2009/05/13 17:20:26 jym Exp $");

#include "bpfilter.h"
#include "vlan.h"

#include <sys/types.h>

#include <sys/param.h>
#include <sys/endian.h>
#include <sys/systm.h>
#include <sys/sockio.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_arp.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_media.h>
#include <net/if_vlanvar.h>

#include <sys/bus.h>

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/ic/rtl81x9reg.h>
#include <dev/ic/rtl81x9var.h>
#include <dev/ic/rtl8169var.h>

struct re_pci_softc {
	struct rtk_softc sc_rtk;

	void *sc_ih;
	pci_chipset_tag_t sc_pc;
	pcitag_t sc_pcitag;
};

static int	re_pci_match(device_t, cfdata_t, void *);
static void	re_pci_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(re_pci, sizeof(struct re_pci_softc),
    re_pci_match, re_pci_attach, NULL, NULL);

/*
 * Various supported device vendors/types and their names.
 */
static const struct rtk_type re_devs[] = {
	{ PCI_VENDOR_REALTEK, PCI_PRODUCT_REALTEK_RT8139,
	    RTK_8139CPLUS,
	    "RealTek 8139C+ 10/100BaseTX" },
	{ PCI_VENDOR_REALTEK, PCI_PRODUCT_REALTEK_RT8101E,
	    RTK_8101E,
	    "RealTek 8100E/8101E/8102E/8102EL PCIe 10/100BaseTX" },
	{ PCI_VENDOR_REALTEK, PCI_PRODUCT_REALTEK_RT8168,
	    RTK_8168,
	    "RealTek 8168/8111 PCIe Gigabit Ethernet" },
	{ PCI_VENDOR_REALTEK, PCI_PRODUCT_REALTEK_RT8169,
	    RTK_8169,
	    "RealTek 8169/8110 Gigabit Ethernet" },
	{ PCI_VENDOR_REALTEK, PCI_PRODUCT_REALTEK_RT8169SC,
	    RTK_8169,
	    "RealTek 8169SC/8110SC Single-chip Gigabit Ethernet" },
	{ PCI_VENDOR_COREGA, PCI_PRODUCT_COREGA_LAPCIGT,
	    RTK_8169,
	    "Corega CG-LAPCIGT Gigabit Ethernet" },
	{ PCI_VENDOR_DLINK, PCI_PRODUCT_DLINK_DGE528T,
	    RTK_8169,
	    "D-Link DGE-528T Gigabit Ethernet" },
	{ PCI_VENDOR_USR2, PCI_PRODUCT_USR2_USR997902,
	    RTK_8169,
	    "US Robotics (3Com) USR997902 Gigabit Ethernet" },
	{ PCI_VENDOR_LINKSYS, PCI_PRODUCT_LINKSYS_EG1032,
	    RTK_8169,
	    "Linksys EG1032 rev. 3 Gigabit Ethernet" },
	{ 0, 0, 0, NULL }
};

#define RE_LINKSYS_EG1032_SUBID	0x00241737

/*
 * Probe for a RealTek 8139C+/8169/8110 chip. Check the PCI vendor and device
 * IDs against our list and return a device name if we find a match.
 */
static int
re_pci_match(device_t parent, cfdata_t cf, void *aux)
{
	const struct rtk_type *t;
	struct pci_attach_args *pa = aux;
	pcireg_t subid;

	subid = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_SUBSYS_ID_REG);

	/* special-case Linksys EG1032, since rev 2 uses sk(4) */
	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_LINKSYS &&
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_LINKSYS_EG1032) {
		if (subid != RE_LINKSYS_EG1032_SUBID)
			return 0;
	}
	/* Don't match 8139 other than C-PLUS */
	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_REALTEK &&
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_REALTEK_RT8139) {
		if (PCI_REVISION(pa->pa_class) != 0x20)
			return 0;
	}

	t = re_devs;

	while (t->rtk_name != NULL) {
		if ((PCI_VENDOR(pa->pa_id) == t->rtk_vid) &&
		    (PCI_PRODUCT(pa->pa_id) == t->rtk_did)) {
			return 2;	/* defect rtk(4) */
		}
		t++;
	}

	return 0;
}

static void
re_pci_attach(device_t parent, device_t self, void *aux)
{
	struct re_pci_softc *psc = device_private(self);
	struct rtk_softc *sc = &psc->sc_rtk;
	struct pci_attach_args *pa = aux;
	pci_chipset_tag_t pc = pa->pa_pc;
	pci_intr_handle_t ih;
	const char *intrstr = NULL;
	const struct rtk_type *t;
	uint32_t hwrev;
	int error = 0;
	pcireg_t command, memtype;
	bool ioh_valid, memh_valid;
	bus_space_tag_t iot, memt;
	bus_space_handle_t ioh, memh;
	bus_size_t iosize, memsize, bsize;

	sc->sc_dev = self;

	/*
	 * Map control/status registers.
	 */
	command = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG);
	command |= PCI_COMMAND_MASTER_ENABLE;
	pci_conf_write(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG, command);

	ioh_valid = (pci_mapreg_map(pa, RTK_PCI_LOIO, PCI_MAPREG_TYPE_IO, 0,
	    &iot, &ioh, NULL, &iosize) == 0);
	memtype = pci_mapreg_type(pa->pa_pc, pa->pa_tag, RTK_PCI_LOMEM);
	switch (memtype) {
	case PCI_MAPREG_TYPE_MEM | PCI_MAPREG_MEM_TYPE_32BIT:
	case PCI_MAPREG_TYPE_MEM | PCI_MAPREG_MEM_TYPE_64BIT:
		memh_valid = (pci_mapreg_map(pa, RTK_PCI_LOMEM,
		    memtype, 0, &memt, &memh, NULL, &memsize) == 0);
		break;
	default:
		memh_valid = 0;
		break;
	}

	if (ioh_valid) {
		sc->rtk_btag = iot;
		sc->rtk_bhandle = ioh;
		bsize = iosize;
	} else if (memh_valid) {
		sc->rtk_btag = memt;
		sc->rtk_bhandle = memh;
		bsize = memsize;
	} else {
		aprint_error(": can't map registers\n");
		return;
	}

	t = re_devs;
	hwrev = CSR_READ_4(sc, RTK_TXCFG) & RTK_TXCFG_HWREV;

	while (t->rtk_name != NULL) {
		if ((PCI_VENDOR(pa->pa_id) == t->rtk_vid) &&
		    (PCI_PRODUCT(pa->pa_id) == t->rtk_did)) {
			break;
		}
		t++;
	}

	aprint_normal(": %s (rev. 0x%02x)\n",
	    t->rtk_name, PCI_REVISION(pa->pa_class));

	if (t->rtk_basetype == RTK_8139CPLUS)
		sc->sc_quirk |= RTKQ_8139CPLUS;

	if (t->rtk_basetype == RTK_8168 ||
	    t->rtk_basetype == RTK_8101E)
		sc->sc_quirk |= RTKQ_PCIE;

	sc->sc_dmat = pa->pa_dmat;

	/*
	 * No power/enable/disable machinery for PCI attach;
	 * mark the card enabled now.
	 */
	sc->sc_flags |= RTK_ENABLED;

	/* Hook interrupt last to avoid having to lock softc */
	/* Allocate interrupt */
	if (pci_intr_map(pa, &ih)) {
		aprint_error_dev(self, "couldn't map interrupt\n");
		return;
	}
	intrstr = pci_intr_string(pc, ih);
	psc->sc_ih = pci_intr_establish(pc, ih, IPL_NET, re_intr, sc);
	if (psc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt");
		if (intrstr != NULL)
			aprint_error(" at %s", intrstr);
		aprint_error("\n");
		return;
	}
	aprint_normal_dev(self, "interrupting at %s\n", intrstr);

	re_attach(sc);

	/*
	 * Perform hardware diagnostic on the original RTL8169.
	 * Some 32-bit cards were incorrectly wired and would
	 * malfunction if plugged into a 64-bit slot.
	 */
	if (hwrev == RTK_HWREV_8169) {
		error = re_diag(sc);
		if (error) {
			aprint_error_dev(self,
			    "attach aborted due to hardware diag failure\n");

			re_detach(sc);

			if (psc->sc_ih != NULL) {
				pci_intr_disestablish(pc, psc->sc_ih);
				psc->sc_ih = NULL;
			}

			if (ioh_valid)
				bus_space_unmap(iot, ioh, iosize);
			if (memh_valid)
				bus_space_unmap(memt, memh, memsize);
		}
	}

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");
	else
		pmf_class_network_register(self, &sc->ethercom.ec_if);
}
