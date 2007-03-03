/*	$NetBSD: if_re_pci.c,v 1.8.2.5 2007/03/03 23:30:25 bouyer Exp $	*/

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

#include <machine/bus.h>

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

static int	re_pci_match(struct device *, struct cfdata *, void *);
static void	re_pci_attach(struct device *, struct device *, void *);

/*
 * Various supported device vendors/types and their names.
 */
static const struct rtk_type re_devs[] = {
	{ PCI_VENDOR_REALTEK, PCI_PRODUCT_REALTEK_RT8139,
	    RTK_HWREV_8139CPLUS,
	    "RealTek 8139C+ 10/100BaseTX" },
	{ PCI_VENDOR_REALTEK, PCI_PRODUCT_REALTEK_RT8101E,
	    RTK_HWREV_8100E,
	    "RealTek 8100E PCIe 10/100BaseTX" },
	{ PCI_VENDOR_REALTEK, PCI_PRODUCT_REALTEK_RT8101E,
	    RTK_HWREV_8101E,
	    "RealTek 8101E PCIe 10/100BaseTX" },
	{ PCI_VENDOR_REALTEK, PCI_PRODUCT_REALTEK_RT8168,
	    RTK_HWREV_8168_SPIN1,
	    "RealTek 8168B/8111B PCIe Gigabit Ethernet" },
	{ PCI_VENDOR_REALTEK, PCI_PRODUCT_REALTEK_RT8168,
	    RTK_HWREV_8168_SPIN2,
	    "RealTek 8168B/8111B PCIe Gigabit Ethernet" },
	{ PCI_VENDOR_REALTEK, PCI_PRODUCT_REALTEK_RT8169,
	    RTK_HWREV_8169,
	    "RealTek 8169 Gigabit Ethernet" },
	{ PCI_VENDOR_REALTEK, PCI_PRODUCT_REALTEK_RT8169,
	    RTK_HWREV_8169S,
	    "RealTek 8169S Single-chip Gigabit Ethernet" },
	{ PCI_VENDOR_REALTEK, PCI_PRODUCT_REALTEK_RT8169,
	    RTK_HWREV_8110S,
	    "RealTek 8110S Single-chip Gigabit Ethernet" },
	{ PCI_VENDOR_REALTEK, PCI_PRODUCT_REALTEK_RT8169,
	    RTK_HWREV_8169_8110SB,
	    "RealTek 8169SB/8110SB Single-chip Gigabit Ethernet" },
	{ PCI_VENDOR_REALTEK, PCI_PRODUCT_REALTEK_RT8169,
	    RTK_HWREV_8169_8110SC,
	    "RealTek 8169SC/8110SC Single-chip Gigabit Ethernet" },
	{ PCI_VENDOR_REALTEK, PCI_PRODUCT_REALTEK_RT8169SC,
	    RTK_HWREV_8169_8110SC,
	    "RealTek 8169SC/8110SC Single-chip Gigabit Ethernet" },
	{ PCI_VENDOR_COREGA, PCI_PRODUCT_COREGA_LAPCIGT,
	    RTK_HWREV_8169S,
	    "Corega CG-LAPCIGT Gigabit Ethernet" },
	{ PCI_VENDOR_DLINK, PCI_PRODUCT_DLINK_DGE528T,
	    RTK_HWREV_8169S,
	    "D-Link DGE-528T Gigabit Ethernet" },
	{ PCI_VENDOR_USR2, PCI_PRODUCT_USR2_USR997902,
	    RTK_HWREV_8169S,
	    "US Robotics (3Com) USR997902 Gigabit Ethernet" },
	{ PCI_VENDOR_LINKSYS, PCI_PRODUCT_LINKSYS_EG1032,
	    RTK_HWREV_8169S,
	    "Linksys EG1032 rev. 3 Gigabit Ethernet" },
	{ 0, 0, 0, NULL }
};

static const struct rtk_hwrev re_hwrevs[] = {
	{ RTK_HWREV_8139, RTK_8139,  "" },
	{ RTK_HWREV_8139A, RTK_8139, "A" },
	{ RTK_HWREV_8139AG, RTK_8139, "A-G" },
	{ RTK_HWREV_8139B, RTK_8139, "B" },
	{ RTK_HWREV_8130, RTK_8139, "8130" },
	{ RTK_HWREV_8139C, RTK_8139, "C" },
	{ RTK_HWREV_8139D, RTK_8139, "8139D/8100B/8100C" },
	{ RTK_HWREV_8139CPLUS, RTK_8139CPLUS, "C+"},
	{ RTK_HWREV_8168_SPIN1, RTK_8169, "8168B/8111B"},
	{ RTK_HWREV_8168_SPIN2, RTK_8169, "8168B/8111B"},
	{ RTK_HWREV_8169, RTK_8169, "8169"},
	{ RTK_HWREV_8169S, RTK_8169, "8169S"},
	{ RTK_HWREV_8110S, RTK_8169, "8110S"},
	{ RTK_HWREV_8169_8110SB, RTK_8169, "8169SB"},
	{ RTK_HWREV_8169_8110SC, RTK_8169, "8169SC"},
	{ RTK_HWREV_8100E, RTK_8169, "8100E"},
	{ RTK_HWREV_8101E, RTK_8169, "8101E"},
	{ RTK_HWREV_8100, RTK_8139, "8100"},
	{ RTK_HWREV_8101, RTK_8139, "8101"},
	{ 0, 0, NULL }
};

#define RE_LINKSYS_EG1032_SUBID	0x00241737

CFATTACH_DECL(re_pci, sizeof(struct re_pci_softc), re_pci_match, re_pci_attach,
	      NULL, NULL);

/*
 * Probe for a RealTek 8139C+/8169/8110 chip. Check the PCI vendor and device
 * IDs against our list and return a device name if we find a match.
 */
static int
re_pci_match(struct device *parent, struct cfdata *match, void *aux)
{
	const struct rtk_type	*t;
	struct pci_attach_args	*pa = aux;
	bus_space_tag_t		iot, memt, bst;
	bus_space_handle_t	ioh, memh, bsh;
	bus_size_t		memsize, iosize, bsize;
	u_int32_t		hwrev;
	pcireg_t subid;
	boolean_t ioh_valid, memh_valid;

	subid = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_SUBSYS_ID_REG);

	/* special-case Linksys EG1032, since rev 2 uses sk(4) */
	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_LINKSYS &&
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_LINKSYS_EG1032 &&
	    subid == RE_LINKSYS_EG1032_SUBID)
		return 1;

	t = re_devs;

	while (t->rtk_name != NULL) {
		if ((PCI_VENDOR(pa->pa_id) == t->rtk_vid) &&
		    (PCI_PRODUCT(pa->pa_id) == t->rtk_did)) {

			/*
			 * Temporarily map the I/O space
			 * so we can read the chip ID register.
			 */
			ioh_valid = (pci_mapreg_map(pa, RTK_PCI_LOIO,
			    PCI_MAPREG_TYPE_IO, 0, &iot, &ioh,
			    NULL, &iosize) == 0);
			memh_valid = (pci_mapreg_map(pa, RTK_PCI_LOMEM,
			    PCI_MAPREG_TYPE_MEM, 0, &memt, &memh,
			    NULL, &memsize) == 0);
			if (ioh_valid) {
				bst = iot;
				bsh = ioh;
				bsize = iosize;
			} else if (memh_valid) {
				bst = memt;
				bsh = memh;
				bsize = memsize;
			} else
				return 0;
			hwrev = bus_space_read_4(bst, bsh, RTK_TXCFG) &
			    RTK_TXCFG_HWREV;
			bus_space_unmap(bst, bsh, bsize);
			if (t->rtk_basetype == hwrev)
				return 2;	/* defeat rtk(4) */
		}
		t++;
	}

	return 0;
}

static void
re_pci_attach(struct device *parent, struct device *self, void *aux)
{
	struct re_pci_softc	*psc = (void *)self;
	struct rtk_softc	*sc = &psc->sc_rtk;
	struct pci_attach_args 	*pa = aux;
	pci_chipset_tag_t pc = pa->pa_pc;
	pci_intr_handle_t ih;
	const char *intrstr = NULL;
	const struct rtk_type	*t;
	const struct rtk_hwrev	*hw_rev;
	uint32_t		hwrev;
	int			error = 0;
	int			pmreg;
	boolean_t		ioh_valid, memh_valid;
	pcireg_t		command;
	bus_space_tag_t		iot, memt;
	bus_space_handle_t	ioh, memh;
	bus_size_t		iosize, memsize, bsize;


	/*
	 * Handle power management nonsense.
	 */
	if (pci_get_capability(pc, pa->pa_tag, PCI_CAP_PWRMGMT, &pmreg, 0)) {
		command = pci_conf_read(pc, pa->pa_tag, pmreg + PCI_PMCSR);
		if (command & PCI_PMCSR_STATE_MASK) {
			u_int32_t		iobase, membase, irq;

			/* Save important PCI config data. */
			iobase = pci_conf_read(pc, pa->pa_tag, RTK_PCI_LOIO);
			membase = pci_conf_read(pc, pa->pa_tag, RTK_PCI_LOMEM);
			irq = pci_conf_read(pc, pa->pa_tag, PCI_INTERRUPT_REG);

			/* Reset the power state. */
			aprint_normal("%s: chip is is in D%d power mode "
		    	    "-- setting to D0\n", sc->sc_dev.dv_xname,
		    	    command & PCI_PMCSR_STATE_MASK);

			command &= ~PCI_PMCSR_STATE_MASK;
			pci_conf_write(pc, pa->pa_tag,
			    pmreg + PCI_PMCSR, command);

			/* Restore PCI config data. */
			pci_conf_write(pc, pa->pa_tag, RTK_PCI_LOIO, iobase);
			pci_conf_write(pc, pa->pa_tag, RTK_PCI_LOMEM, membase);
			pci_conf_write(pc, pa->pa_tag, PCI_INTERRUPT_REG, irq);
		}
	}

	/*
	 * Map control/status registers.
	 */
	command = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG);
	command |= PCI_COMMAND_MASTER_ENABLE;
	pci_conf_write(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG, command);

	ioh_valid = (pci_mapreg_map(pa, RTK_PCI_LOIO, PCI_MAPREG_TYPE_IO, 0,
	    &iot, &ioh, NULL, &iosize) == 0);
	memh_valid = (pci_mapreg_map(pa, RTK_PCI_LOMEM, PCI_MAPREG_TYPE_MEM, 0,
	    &memt, &memh, NULL, &memsize) == 0);
	if (ioh_valid) {
		sc->rtk_btag = iot;
		sc->rtk_bhandle = ioh;
		bsize = iosize;
	} else if (memh_valid) {
		sc->rtk_btag = memt;
		sc->rtk_bhandle = memh;
		bsize = memsize;
	} else {
		aprint_error("%s: can't map registers\n", sc->sc_dev.dv_xname);
		return;
	}

	t = re_devs;
	hwrev = CSR_READ_4(sc, RTK_TXCFG) & RTK_TXCFG_HWREV;

	while (t->rtk_name != NULL) {
		if ((PCI_VENDOR(pa->pa_id) == t->rtk_vid) &&
		    (PCI_PRODUCT(pa->pa_id) == t->rtk_did)) {

			if (t->rtk_basetype == hwrev)
				break;
		}
		t++;
	}
	aprint_normal(": %s\n", t->rtk_name);

	hw_rev = re_hwrevs;
	hwrev = CSR_READ_4(sc, RTK_TXCFG) & RTK_TXCFG_HWREV;
	while (hw_rev->rtk_desc != NULL) {
		if (hw_rev->rtk_rev == hwrev) {
			sc->rtk_type = hw_rev->rtk_type;
			break;
		}
		hw_rev++;
	}

	sc->sc_dmat = pa->pa_dmat;

	/*
	 * No power/enable/disable machinery for PCI attac;
	 * mark the card enabled now.
	 */
	sc->sc_flags |= RTK_ENABLED;

	/* Hook interrupt last to avoid having to lock softc */
	/* Allocate interrupt */
	if (pci_intr_map(pa, &ih)) {
		aprint_error("%s: couldn't map interrupt\n",
		    sc->sc_dev.dv_xname);
		return;
	}
	intrstr = pci_intr_string(pc, ih);
	psc->sc_ih = pci_intr_establish(pc, ih, IPL_NET, re_intr, sc);
	if (psc->sc_ih == NULL) {
		aprint_error("%s: couldn't establish interrupt",
		    sc->sc_dev.dv_xname);
		if (intrstr != NULL)
			aprint_error(" at %s", intrstr);
		aprint_error("\n");
		return;
	}
	aprint_normal("%s: interrupting at %s\n", sc->sc_dev.dv_xname, intrstr);

	re_attach(sc);

	/*
	 * Perform hardware diagnostic on the original RTL8169.
	 * Some 32-bit cards were incorrectly wired and would
	 * malfunction if plugged into a 64-bit slot.
	 */
	if (hwrev == RTK_HWREV_8169) {
		error = re_diag(sc);
		if (error) {
			aprint_error(
			    "%s: attach aborted due to hardware diag failure\n",
			    sc->sc_dev.dv_xname);

			re_detach(sc);

			if (psc->sc_ih != NULL) {
				pci_intr_disestablish(pc, psc->sc_ih);
				psc->sc_ih = NULL;
			}

			if (bsize)
				bus_space_unmap(sc->rtk_btag, sc->rtk_bhandle,
				    bsize);
		}
	}
}
