/*	$NetBSD: if_ath_pci.c,v 1.14 2006/06/20 14:38:34 perry Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: if_ath_pci.c,v 1.14 2006/06/20 14:38:34 perry Exp $");
#endif

/*
 * PCI/Cardbus front-end for the Atheros Wireless LAN controller driver.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/lock.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/errno.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <net/if.h>
#include <net/if_media.h>
#include <net/if_ether.h>
#include <net/if_llc.h>
#include <net/if_arp.h>

#include <net80211/ieee80211_netbsd.h>
#include <net80211/ieee80211_var.h>

#ifdef INET
#include <netinet/in.h>
#endif

#include <dev/ic/ath_netbsd.h>
#include <dev/ic/athvar.h>
#include <contrib/dev/ath/ah.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#include <sys/device.h>

/*
 * PCI glue.
 */

struct ath_pci_softc {
	struct ath_softc	sc_sc;
	pci_chipset_tag_t	sc_pc;
        pcitag_t 		sc_pcitag; 
        struct pci_conf_state 	sc_pciconf;
	void			*sc_ih;		/* interrupt handler */
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
};

#define	BS_BAR	0x10
#define	PCIR_RETRY_TIMEOUT_REG		0x40
#define	PCIR_RETRY_TIMEOUT_MASK		0x0000ff00

static int ath_pci_match(struct device *, struct cfdata *, void *);
static void ath_pci_attach(struct device *, struct device *, void *);
static void ath_pci_shutdown(void *);
static void ath_pci_powerhook(int, void *);
static int ath_pci_detach(struct device *, int);

CFATTACH_DECL(ath_pci,
    sizeof(struct ath_pci_softc),
    ath_pci_match,
    ath_pci_attach,
    ath_pci_detach,
    NULL);

static int
ath_pci_match(struct device *parent, struct cfdata *match, void *aux)
{
	const char* devname;
	struct pci_attach_args *pa = aux;

	devname = ath_hal_probe(PCI_VENDOR(pa->pa_id), PCI_PRODUCT(pa->pa_id));
	if (devname != NULL)
		return 1;
	return 0;
}

static int
ath_pci_setup(struct pci_attach_args *pa)
{
	pci_chipset_tag_t pc = pa->pa_pc;
	uint32_t res;
	/*
	 * Enable memory mapping and bus mastering.
	 */
	pci_conf_write(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG,
	    pci_conf_read(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG) |
	        PCI_COMMAND_MASTER_ENABLE | PCI_COMMAND_MEM_ENABLE);
	res = pci_conf_read(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG);

	if ((res & PCI_COMMAND_MEM_ENABLE) == 0) {
		aprint_error("couldn't enable memory mapping\n");
		return 0;
	}
	if ((res & PCI_COMMAND_MASTER_ENABLE) == 0) {
		aprint_error("couldn't enable bus mastering\n");
		return 0;
	}

	/*
	 * Disable retry timeout to keep PCI Tx retries from
	 * interfering with C3 CPU state.
	 */
	pci_conf_write(pc, pa->pa_tag, PCIR_RETRY_TIMEOUT_REG,
	    pci_conf_read(pc, pa->pa_tag, PCIR_RETRY_TIMEOUT_REG) &
	    ~PCIR_RETRY_TIMEOUT_MASK);

	return 1;
}

static void
ath_pci_attach(struct device *parent, struct device *self, void *aux)
{
	struct ath_pci_softc *psc = (struct ath_pci_softc *)self;
	struct ath_softc *sc = &psc->sc_sc;
	struct pci_attach_args *pa = aux;
	pci_chipset_tag_t pc = pa->pa_pc;
	pci_intr_handle_t ih;
	void *shook;
	void *phook;
	const char *intrstr = NULL;

	psc->sc_pc = pc;

	psc->sc_pcitag = pa->pa_tag;

	if (!ath_pci_setup(pa))
		goto bad;

	/*
	 * Setup memory-mapping of PCI registers.
	 */
	if (pci_mapreg_map(pa, BS_BAR, PCI_MAPREG_TYPE_MEM, 0, &psc->sc_iot,
		&psc->sc_ioh, NULL, NULL)) {
		aprint_error("cannot map register space\n");
		goto bad;
	}

	sc->sc_st = HALTAG(psc->sc_iot);
	sc->sc_sh = HALHANDLE(psc->sc_ioh);

	sc->sc_invalid = 1;

	/*
	 * Arrange interrupt line.
	 */
	if (pci_intr_map(pa, &ih)) {
		aprint_error("couldn't map interrupt\n");
		goto bad1;
	}

	intrstr = pci_intr_string(pc, ih); 
	psc->sc_ih = pci_intr_establish(pc, ih, IPL_NET, ath_intr, sc);
	if (psc->sc_ih == NULL) {
		aprint_error("couldn't map interrupt\n");
		goto bad2;
	}

	printf("\n");
	printf("%s: interrupting at %s\n", sc->sc_dev.dv_xname, intrstr);

	sc->sc_dmat = pa->pa_dmat;

	shook = shutdownhook_establish(ath_pci_shutdown, psc);
	if (shook == NULL) {
		aprint_error("couldn't make shutdown hook\n");
		goto bad3;
	}

	phook = powerhook_establish(ath_pci_powerhook, psc);
	if (phook == NULL) {
		aprint_error("couldn't make power hook\n");
		goto bad3;
	}

	if (ath_attach(PCI_PRODUCT(pa->pa_id), sc) == 0)
		return;

	shutdownhook_disestablish(shook);
	powerhook_disestablish(phook);

bad3:	pci_intr_disestablish(pc, psc->sc_ih);
bad2:	/* XXX */
bad1:	/* XXX */
bad:
	return;
}

static int
ath_pci_detach(struct device *self, int flags)
{
	struct ath_pci_softc *psc = (struct ath_pci_softc *)self;

	ath_detach(&psc->sc_sc);
	pci_intr_disestablish(psc->sc_pc, psc->sc_ih);

	return (0);
}

static void
ath_pci_shutdown(void *self)
{
	struct ath_pci_softc *psc = (struct ath_pci_softc *)self;

	ath_shutdown(&psc->sc_sc);
}

static void
ath_pci_powerhook(int why, void *arg)
{
	struct ath_pci_softc *sc = arg;
	pci_chipset_tag_t pc = sc->sc_pc;
	pcitag_t tag = sc->sc_pcitag;

	switch (why) {
	case PWR_SOFTSUSPEND:
		ath_pci_shutdown(sc);
		break;
	case PWR_SUSPEND:
		pci_conf_capture(pc, tag, &sc->sc_pciconf);
		break;
	case PWR_RESUME:
		pci_conf_restore(pc, tag, &sc->sc_pciconf);
		break;
	}

	return;
}
