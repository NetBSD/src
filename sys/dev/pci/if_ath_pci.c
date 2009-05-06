/*	$NetBSD: if_ath_pci.c,v 1.33 2009/05/06 09:25:15 cegger Exp $	*/

/*-
 * Copyright (c) 2002-2005 Sam Leffler, Errno Consulting
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce at minimum a disclaimer
 *    similar to the "NO WARRANTY" disclaimer below ("Disclaimer") and any
 *    redistribution must be conditioned upon including a substantially
 *    similar Disclaimer requirement for further binary redistribution.
 * 3. Neither the names of the above-listed copyright holders nor the names
 *    of any contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * NO WARRANTY
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF NONINFRINGEMENT, MERCHANTIBILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGES.
 */

#include <sys/cdefs.h>
#ifdef __FreeBSD__
__FBSDID("$FreeBSD: src/sys/dev/ath/if_ath_pci.c,v 1.11 2005/01/18 18:08:16 sam Exp $");
#endif
#ifdef __NetBSD__
__KERNEL_RCSID(0, "$NetBSD: if_ath_pci.c,v 1.33 2009/05/06 09:25:15 cegger Exp $");
#endif

/*
 * PCI/Cardbus front-end for the Atheros Wireless LAN controller driver.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_media.h>
#include <net/if_ether.h>

#include <net80211/ieee80211_netbsd.h>
#include <net80211/ieee80211_var.h>

#ifdef INET
#include <netinet/in.h>
#endif

#include <external/isc/atheros_hal/dist/ah.h>

#include <dev/ic/ath_netbsd.h>
#include <dev/ic/athvar.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

/*
 * PCI glue.
 */

struct ath_pci_softc {
	struct ath_softc	sc_sc;
	pci_chipset_tag_t	sc_pc;
	pcitag_t		sc_pcitag; 
	pci_intr_handle_t	sc_pih;
	void			*sc_ih;		/* interrupt handler */
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_size_t		sc_mapsz;
};

#define	BS_BAR	0x10

static void ath_pci_attach(device_t, device_t, void *);
static int ath_pci_detach(device_t, int);
static int ath_pci_match(device_t, cfdata_t, void *);
static int ath_pci_detach(device_t, int);

CFATTACH_DECL_NEW(ath_pci,
    sizeof(struct ath_pci_softc),
    ath_pci_match,
    ath_pci_attach,
    ath_pci_detach,
    NULL);

static int
ath_pci_match(device_t parent, cfdata_t match, void *aux)
{
	const char* devname;
	struct pci_attach_args *pa = aux;

	devname = ath_hal_probe(PCI_VENDOR(pa->pa_id), PCI_PRODUCT(pa->pa_id));
	if (devname != NULL)
		return 1;
	return 0;
}

static bool
ath_pci_suspend(device_t self PMF_FN_ARGS)
{
	struct ath_pci_softc *sc = device_private(self);

	ath_suspend(&sc->sc_sc);
	pci_intr_disestablish(sc->sc_pc, sc->sc_ih);
	sc->sc_ih = NULL;

	return true;
}

static bool
ath_pci_resume(device_t self PMF_FN_ARGS)
{
	struct ath_pci_softc *sc = device_private(self);

	sc->sc_ih = pci_intr_establish(sc->sc_pc, sc->sc_pih, IPL_NET, ath_intr,
	    &sc->sc_sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't map interrupt\n");
		return false;
	}
	ath_resume(&sc->sc_sc);

	return true;
}

static int
ath_pci_setup(struct ath_pci_softc *sc)
{
	pcireg_t bhlc, csr, icr, lattimer;
	/*
	 * Enable memory mapping and bus mastering.
	 */
	csr = pci_conf_read(sc->sc_pc, sc->sc_pcitag, PCI_COMMAND_STATUS_REG);
	csr |= PCI_COMMAND_MASTER_ENABLE | PCI_COMMAND_MEM_ENABLE;
	pci_conf_write(sc->sc_pc, sc->sc_pcitag, PCI_COMMAND_STATUS_REG, csr);
	csr = pci_conf_read(sc->sc_pc, sc->sc_pcitag, PCI_COMMAND_STATUS_REG);

	if ((csr & PCI_COMMAND_MEM_ENABLE) == 0) {
		aprint_error("couldn't enable memory mapping\n");
		return 0;
	}
	if ((csr & PCI_COMMAND_MASTER_ENABLE) == 0) {
		aprint_error("couldn't enable bus mastering\n");
		return 0;
	}

	/*
	 * XXX Both this comment and code are replicated in
	 * XXX cardbus_rescan().
	 *
	 * Make sure the latency timer is set to some reasonable
	 * value.
	 *
	 * I will set the initial value of the Latency Timer here.
	 *
	 * While a PCI device owns the bus, its Latency Timer counts
	 * down bus cycles from its initial value to 0.  Minimum
	 * Grant tells for how long the device wants to own the
	 * bus once it gets access, in units of 250ns.
	 *
	 * On a 33 MHz bus, there are 8 cycles per 250ns.  So I
	 * multiply the Minimum Grant by 8 to find out the initial
	 * value of the Latency Timer.
	 *
	 * I never set a Latency Timer less than 0x10, since that
	 * is what the old code did.
	 */
	bhlc = pci_conf_read(sc->sc_pc, sc->sc_pcitag, PCI_BHLC_REG);
	icr = pci_conf_read(sc->sc_pc, sc->sc_pcitag, PCI_INTERRUPT_REG);
	lattimer = MAX(0x10, MIN(0xf8, 8 * PCI_MIN_GNT(icr)));
	if (PCI_LATTIMER(bhlc) < lattimer) {
		bhlc &= ~(PCI_LATTIMER_MASK << PCI_LATTIMER_SHIFT);
		bhlc |= (lattimer << PCI_LATTIMER_SHIFT);
		pci_conf_write(sc->sc_pc, sc->sc_pcitag, PCI_BHLC_REG, bhlc);
	}
	return 1;
}

static void
ath_pci_attach(device_t parent, device_t self, void *aux)
{
	struct ath_pci_softc *psc = device_private(self);
	struct ath_softc *sc = &psc->sc_sc;
	struct pci_attach_args *pa = aux;
	pci_chipset_tag_t pc = pa->pa_pc;
	pcireg_t mem_type;
	const char *intrstr = NULL;

	sc->sc_dev = self;
	psc->sc_pc = pc;

	psc->sc_pcitag = pa->pa_tag;

	if (!ath_pci_setup(psc))
		goto bad;	

	/*
	 * Setup memory-mapping of PCI registers.
	 */
	mem_type = pci_mapreg_type(pc, pa->pa_tag, BS_BAR);
	if (mem_type != PCI_MAPREG_TYPE_MEM &&
	    mem_type != PCI_MAPREG_MEM_TYPE_64BIT) {
		aprint_error("bad pci register type %d\n", (int)mem_type);
		goto bad;
	}
	if (pci_mapreg_map(pa, BS_BAR, mem_type, 0, &psc->sc_iot,
		&psc->sc_ioh, NULL, &psc->sc_mapsz)) {
		aprint_error("cannot map register space\n");
		goto bad;
	}

	sc->sc_st = HALTAG(psc->sc_iot);
	sc->sc_sh = HALHANDLE(psc->sc_ioh);

	/*
	 * Arrange interrupt line.
	 */
	if (pci_intr_map(pa, &psc->sc_pih)) {
		aprint_error("couldn't map interrupt\n");
		goto bad1;
	}

	intrstr = pci_intr_string(pc, psc->sc_pih); 
	psc->sc_ih = pci_intr_establish(pc, psc->sc_pih, IPL_NET, ath_intr, sc);
	if (psc->sc_ih == NULL) {
		aprint_error("couldn't map interrupt\n");
		goto bad2;
	}

	aprint_normal("\n");
	aprint_verbose_dev(self, "interrupting at %s\n", intrstr);

	sc->sc_dmat = pa->pa_dmat;

	ATH_LOCK_INIT(sc);

	if (ath_attach(PCI_PRODUCT(pa->pa_id), sc) != 0)
		goto bad3;

	if (!pmf_device_register(self, ath_pci_suspend, ath_pci_resume))
		aprint_error_dev(self, "couldn't establish power handler\n");
	else {
		pmf_class_network_register(self, &sc->sc_if);
		pmf_device_suspend_self(self);
	}
	return;
bad3:
	ATH_LOCK_DESTROY(sc);

	pci_intr_disestablish(pc, psc->sc_ih);
bad2:	/* XXX */
bad1:
	bus_space_unmap(psc->sc_iot, psc->sc_ioh, psc->sc_mapsz);
bad:	/* XXX */
	return;
}

static int
ath_pci_detach(device_t self, int flags)
{
	struct ath_pci_softc *psc = device_private(self);

	ath_detach(&psc->sc_sc);
	pmf_device_deregister(self);
	if (psc->sc_ih != NULL)
		pci_intr_disestablish(psc->sc_pc, psc->sc_ih);
	bus_space_unmap(psc->sc_iot, psc->sc_ioh, psc->sc_mapsz);

	ATH_LOCK_DESTROY(&psc->sc_sc);

	return (0);
}
