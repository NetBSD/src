/*	$NetBSD: if_rtw_pci.c,v 1.15 2009/09/16 16:34:50 dyoung Exp $	*/

/*-
 * Copyright (c) 1998, 1999, 2000, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center; Charles M. Hannum; and David Young.
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

/*
 * PCI bus front-end for the Realtek RTL8180 802.11 MAC/BBP chip.
 *
 * Derived from the ADMtek ADM8211 PCI bus front-end.
 *
 * Derived from the ``Tulip'' PCI bus front-end.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_rtw_pci.c,v 1.15 2009/09/16 16:34:50 dyoung Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/device.h>

#include <machine/endian.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_ether.h>

#include <net80211/ieee80211_netbsd.h>
#include <net80211/ieee80211_radiotap.h>
#include <net80211/ieee80211_var.h>

#include <sys/bus.h>
#include <sys/intr.h>

#include <dev/ic/rtwreg.h>
#include <dev/ic/sa2400reg.h>
#include <dev/ic/rtwvar.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

/*
 * PCI configuration space registers used by the ADM8211.
 */
#define	RTW_PCI_IOBA		0x10	/* i/o mapped base */
#define	RTW_PCI_MMBA		0x14	/* memory mapped base */

struct rtw_pci_softc {
	struct rtw_softc	psc_rtw;	/* real ADM8211 softc */

	pci_intr_handle_t	psc_ih;		/* interrupt handle */
	void			*psc_intrcookie;

	pci_chipset_tag_t	psc_pc;		/* our PCI chipset */
	pcitag_t		psc_pcitag;	/* our PCI tag */
};

static int	rtw_pci_match(device_t, cfdata_t, void *);
static void	rtw_pci_attach(device_t, device_t, void *);
static int	rtw_pci_detach(device_t, int);

CFATTACH_DECL_NEW(rtw_pci, sizeof(struct rtw_pci_softc),
    rtw_pci_match, rtw_pci_attach, rtw_pci_detach, NULL);

static const struct rtw_pci_product {
	u_int32_t	app_vendor;	/* PCI vendor ID */
	u_int32_t	app_product;	/* PCI product ID */
	const char	*app_product_name;
} rtw_pci_products[] = {
	{ PCI_VENDOR_REALTEK,		PCI_PRODUCT_REALTEK_RT8180,
	  "Realtek RTL8180 802.11 MAC/BBP" },
	{ PCI_VENDOR_BELKIN,		PCI_PRODUCT_BELKIN_F5D6001,
	  "Belkin F5D6001" },

	{ 0,				0,				NULL },
};

static const struct rtw_pci_product *
rtw_pci_lookup(const struct pci_attach_args *pa)
{
	const struct rtw_pci_product *app;

	for (app = rtw_pci_products;
	     app->app_product_name != NULL;
	     app++) {
		if (PCI_VENDOR(pa->pa_id) == app->app_vendor &&
		    PCI_PRODUCT(pa->pa_id) == app->app_product)
			return (app);
	}
	return (NULL);
}

static int
rtw_pci_match(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa = aux;

	if (rtw_pci_lookup(pa) != NULL)
		return (1);

	return (0);
}

static bool
rtw_pci_resume(device_t self PMF_FN_ARGS)
{
	struct rtw_pci_softc *psc = device_private(self);
	struct rtw_softc *sc = &psc->psc_rtw;

	/* Establish the interrupt. */
	psc->psc_intrcookie = pci_intr_establish(psc->psc_pc, psc->psc_ih,
	    IPL_NET, rtw_intr, sc);
	if (psc->psc_intrcookie == NULL) {
		aprint_error_dev(sc->sc_dev, "unable to establish interrupt\n");
		return false;
	}

	return rtw_resume(self PMF_FN_CALL);
}

static bool
rtw_pci_suspend(device_t self PMF_FN_ARGS)
{
	struct rtw_pci_softc *psc = device_private(self);

	if (!rtw_suspend(self PMF_FN_CALL))
		return false;

	/* Unhook the interrupt handler. */
	pci_intr_disestablish(psc->psc_pc, psc->psc_intrcookie);
	psc->psc_intrcookie = NULL;
	return true;
}

static void
rtw_pci_attach(device_t parent, device_t self, void *aux)
{
	struct rtw_pci_softc *psc = device_private(self);
	struct rtw_softc *sc = &psc->psc_rtw;
	struct rtw_regs *regs = &sc->sc_regs;
	struct pci_attach_args *pa = aux;
	pci_chipset_tag_t pc = pa->pa_pc;
	const char *intrstr = NULL;
	const struct rtw_pci_product *app;
	int error;

	sc->sc_dev = self;
	psc->psc_pc = pa->pa_pc;
	psc->psc_pcitag = pa->pa_tag;

	app = rtw_pci_lookup(pa);
	if (app == NULL) {
		printf("\n");
		panic("rtw_pci_attach: impossible");
	}

	/*
	 * Get revision info, and set some chip-specific variables.
	 */
	sc->sc_rev = PCI_REVISION(pa->pa_class);
	aprint_normal(": %s, revision %d.%d\n", app->app_product_name,
	    (sc->sc_rev >> 4) & 0xf, sc->sc_rev & 0xf);

	/* power up chip */
	if ((error = pci_activate(pa->pa_pc, pa->pa_tag, self, NULL)) != 0 &&
	    error != EOPNOTSUPP) {
		aprint_error_dev(self, "cannot activate %d\n", error);
		return;
	}

	/*
	 * Map the device.
	 */
	if (pci_mapreg_map(pa, RTW_PCI_MMBA,
	    PCI_MAPREG_TYPE_MEM|PCI_MAPREG_MEM_TYPE_32BIT, 0,
	    &regs->r_bt, &regs->r_bh, NULL, &regs->r_sz) == 0)
		;
	else if (pci_mapreg_map(pa, RTW_PCI_IOBA, PCI_MAPREG_TYPE_IO, 0,
	    &regs->r_bt, &regs->r_bh, NULL, &regs->r_sz) == 0)
		;
	else {
		aprint_error_dev(self, "unable to map device registers\n");
		return;
	}

	sc->sc_dmat = pa->pa_dmat;

	/*
	 * Make sure bus mastering is enabled.
	 */
	pci_conf_write(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG,
	    pci_conf_read(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG) |
	    PCI_COMMAND_MASTER_ENABLE);

	/*
	 * Map and establish our interrupt.
	 */
	if (pci_intr_map(pa, &psc->psc_ih)) {
		aprint_error_dev(self, "unable to map interrupt\n");
		return;
	}
	intrstr = pci_intr_string(pc, psc->psc_ih);
	psc->psc_intrcookie = pci_intr_establish(pc, psc->psc_ih, IPL_NET,
	    rtw_intr, sc);
	if (psc->psc_intrcookie == NULL) {
		aprint_error_dev(self, "unable to establish interrupt");
		if (intrstr != NULL)
			aprint_error(" at %s", intrstr);
		aprint_error("\n");
		return;
	}

	aprint_normal_dev(self, "interrupting at %s\n", intrstr);

	/*
	 * Finish off the attach.
	 */
	rtw_attach(sc);

	if (pmf_device_register(sc->sc_dev, rtw_pci_suspend, rtw_pci_resume)) {
		pmf_class_network_register(self, &sc->sc_if);
		/*
		 * Power down the socket.
		 */
		pmf_device_suspend(sc->sc_dev, &sc->sc_qual);
	} else
		aprint_error_dev(sc->sc_dev,
		    "couldn't establish power handler\n");
}

static int
rtw_pci_detach(device_t self, int flags)
{
	struct rtw_pci_softc *psc = device_private(self);
	struct rtw_softc *sc = &psc->psc_rtw;
	struct rtw_regs *regs = &sc->sc_regs;
	int rc;

	if ((rc = rtw_detach(sc)) != 0)
		return rc;
	if (psc->psc_intrcookie != NULL)
		pci_intr_disestablish(psc->psc_pc, psc->psc_intrcookie);
	bus_space_unmap(regs->r_bt, regs->r_bh, regs->r_sz);

	return 0;
}
