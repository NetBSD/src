/*	$NetBSD: if_tlp_pci.c,v 1.3 1999/09/08 21:42:44 thorpej Exp $	*/

/*-
 * Copyright (c) 1998, 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 * PCI bus front-end for the Digital Semiconductor ``Tulip'' (21x4x)
 * Ethernet controller family driver.
 */

#include "opt_inet.h"
#include "opt_ns.h"
#include "bpfilter.h"

#include <sys/param.h>
#include <sys/systm.h> 
#include <sys/mbuf.h>   
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/device.h>
 
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_ether.h>

#if NBPFILTER > 0 
#include <net/bpf.h>
#endif 

#ifdef INET
#include <netinet/in.h> 
#include <netinet/if_inarp.h>
#endif

#ifdef NS
#include <netns/ns.h>
#include <netns/ns_if.h>
#endif

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/mii/miivar.h>

#include <dev/ic/tulipreg.h>
#include <dev/ic/tulipvar.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

/*
 * PCI configuration space registers used by the Tulip.
 */
#define	TULIP_PCI_IOBA		0x10	/* i/o mapped base */
#define	TULIP_PCI_MMBA		0x14	/* memory mapped base */

struct tulip_pci_softc {
	struct tulip_softc sc_tulip;	/* real Tulip softc */

	/* PCI-specific goo. */
	void	*sc_ih;			/* interrupt handle */
};

int	tlp_pci_match __P((struct device *, struct cfdata *, void *));
void	tlp_pci_attach __P((struct device *, struct device *, void *));

struct cfattach tlp_pci_ca = {
	sizeof(struct tulip_pci_softc), tlp_pci_match, tlp_pci_attach,
};

const struct tulip_pci_product {
	u_int32_t	tpp_vendor;	/* PCI vendor ID */
	u_int32_t	tpp_product;	/* PCI product ID */
	tulip_chip_t	tpp_chip;	/* base Tulip chip type */
} tlp_pci_products[] = {
#if 0
	{ PCI_VENDOR_DEC,		PCI_PRODUCT_DEC_21040,
	  TULIP_CHIP_21040 },
	{ PCI_VENDOR_DEC,		PCI_PRODUCT_DEC_21041,
	  TULIP_CHIP_21041 },
	{ PCI_VENDOR_DEC,		PCI_PRODUCT_DEC_21140,
	  TULIP_CHIP_21140 },
	{ PCI_VENDOR_DEC,		PCI_PRODUCT_DEC_21142,
	  TULIP_CHIP_21142 },
#endif

	{ PCI_VENDOR_LITEON,		PCI_PRODUCT_LITEON_82C168,
	  TULIP_CHIP_82C168 },

#if 0
	{ PCI_VENDOR_MACRONIX,		PCI_PRODUCT_MACRONIX_MX98713,
	  TULIP_CHIP_MX98713 },
	{ PCI_VENDOR_MACRONIX,		PCI_PRODUCT_MACRONIX_MX987x5,
	  TULIP_CHIP_MX98715 },
#endif

	{ PCI_VENDOR_WINBOND,		PCI_PRODUCT_WINBOND_W89C840F,
	  TULIP_CHIP_WB89C840F },

	{ 0,				0,
	  TULIP_CHIP_INVALID },
};

const char *tlp_pci_chip_names[] = TULIP_CHIP_NAMES;

const struct tulip_pci_product *tlp_pci_lookup
    __P((const struct pci_attach_args *));

const struct tulip_pci_product *
tlp_pci_lookup(pa)
	const struct pci_attach_args *pa;
{
	const struct tulip_pci_product *tpp;

	for (tpp = tlp_pci_products;
	     tlp_pci_chip_names[tpp->tpp_chip] != NULL;
	     tpp++) {
		if (PCI_VENDOR(pa->pa_id) == tpp->tpp_vendor &&
		    PCI_PRODUCT(pa->pa_id) == tpp->tpp_product)
			return (tpp);
	}
	return (NULL);
}

int
tlp_pci_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pci_attach_args *pa = aux;

	if (tlp_pci_lookup(pa) != NULL)
		return (10);	/* beat if_de.c */

	return (0);
}

void
tlp_pci_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct tulip_pci_softc *psc = (void *) self;
	struct tulip_softc *sc = &psc->sc_tulip;
	struct pci_attach_args *pa = aux;
	pci_chipset_tag_t pc = pa->pa_pc;
	pci_intr_handle_t ih;
	const char *intrstr = NULL;
	bus_space_tag_t iot, memt;
	bus_space_handle_t ioh, memh;
	int ioh_valid, memh_valid, i, j;
	const struct tulip_pci_product *tpp;
	u_int8_t enaddr[ETHER_ADDR_LEN], *romdata;
	u_int16_t rombuf[TULIP_MAX_ROM_SIZE >> 1];
	u_int32_t val;
	const char *name = NULL;

	/*
	 * Map the device.
	 */
	ioh_valid = (pci_mapreg_map(pa, TULIP_PCI_IOBA,
	    PCI_MAPREG_TYPE_IO, 0,
	    &iot, &ioh, NULL, NULL) == 0);
	memh_valid = (pci_mapreg_map(pa, TULIP_PCI_MMBA,
	    PCI_MAPREG_TYPE_MEM|PCI_MAPREG_MEM_TYPE_32BIT, 0,
	    &memt, &memh, NULL, NULL) == 0);

	if (memh_valid) {
		sc->sc_st = memt;
		sc->sc_sh = memh;
	} else if (ioh_valid) {
		sc->sc_st = iot;
		sc->sc_sh = ioh;
	} else {
		printf(": unable to map device registers\n");
		return;
	}

	tpp = tlp_pci_lookup(pa);
	if (tpp == NULL) {
		printf("\n");
		panic("tlp_pci_attach: impossible");
	}
	sc->sc_chip = tpp->tpp_chip;

	/*
	 * Get revision info, and set some chip-specific variables.
	 */
	sc->sc_rev = PCI_REVISION(pa->pa_class);
	sc->sc_regshift = 0;
	switch (sc->sc_chip) {
	case TULIP_CHIP_21140:
		if (sc->sc_rev >= 0x20)
			sc->sc_chip = TULIP_CHIP_21140A;
		break;

	case TULIP_CHIP_21142:
		if (sc->sc_rev >= 0x20)
			sc->sc_chip = TULIP_CHIP_21143;
		break;

	case TULIP_CHIP_82C168:
		if (sc->sc_rev >= 0x20)
			sc->sc_chip = TULIP_CHIP_82C169;
		break;

	case TULIP_CHIP_MX98713:
		if (sc->sc_rev >= 0x10)
			sc->sc_chip = TULIP_CHIP_MX98713A;
		break;

	case TULIP_CHIP_MX98715:
		if (sc->sc_rev >= 0x30)
			sc->sc_chip = TULIP_CHIP_MX98725;
		break;

	case TULIP_CHIP_WB89C840F:
		sc->sc_regshift = 1;
		break;

	default:
		/* Nothing. */
	}

	printf(": %s Ethernet, pass %d.%d\n",
	    tlp_pci_chip_names[sc->sc_chip],
	    (sc->sc_rev >> 4) & 0xf, sc->sc_rev & 0xf);

	sc->sc_dmat = pa->pa_dmat;

	/*
	 * Make sure bus mastering is enabled.
	 */
	pci_conf_write(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG,
	    pci_conf_read(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG) |
	    PCI_COMMAND_MASTER_ENABLE);

	/*
	 * Get the cacheline size.
	 */
	sc->sc_cacheline = PCI_CACHELINE(pci_conf_read(pc, pa->pa_tag,
	    PCI_BHLC_REG));

	/*
	 * Read the contents of the Ethernet Address ROM/SROM.
	 */
	memset(rombuf, 0, sizeof(rombuf));
	romdata = (u_int8_t *) rombuf;
	switch (sc->sc_chip) {
	case TULIP_CHIP_21040:
		TULIP_WRITE(sc, CSR_MIIROM, MIIROM_SROMCS);
		for (i = 0; i < sizeof(rombuf); i++) {
			for (j = 0; j < 10000; j++) {
				val = TULIP_READ(sc, CSR_MIIROM);
				if ((val & MIIROM_DN) == 0)
					break;
			}
			romdata[i] = val & MIIROM_DATA;
		}
		break;

	case TULIP_CHIP_82C168:
	case TULIP_CHIP_82C169:
		/*
		 * The Lite-On PNIC stores the Ethernet address in
		 * the first 3 words of the EEPROM.  EEPROM access
		 * is not like the other Tulip chips.
		 */
		for (i = 0; i < 3; i++) {
			TULIP_WRITE(sc, CSR_PNIC_SROMCTL,
			    PNIC_SROMCTL_READ | i);
			for (j = 0; j < 500; j++) {
				delay(2);
				val = TULIP_READ(sc, CSR_MIIROM);
				if ((val & PNIC_MIIROM_BUSY) == 0)
					break;
			}
			if (val & PNIC_MIIROM_BUSY) {
				printf("%s: EEPROM timed out\n",
				    sc->sc_dev.dv_xname);
				return;
			}
			rombuf[i] = bswap16(val & PNIC_MIIROM_DATA);
		}
		break;

	default:
		tlp_read_srom(sc, 0, sizeof(rombuf) >> 1, rombuf);
	}

	/*
	 * Deal with chip/board quirks.  This includes setting up
	 * the mediasw, and extracting the Ethernet address from
	 * the rombuf.
	 *
	 * XXX Eventually, handle master/slave interrupts on the
	 * XXX multi-port boards which require that.
	 */
	switch (sc->sc_chip) {
	case TULIP_CHIP_82C168:
	case TULIP_CHIP_82C169:
		/*
		 * Lite-On PNIC's Ethernet address is the first 6
		 * bytes of its EEPROM.
		 */
		memcpy(enaddr, romdata, ETHER_ADDR_LEN);

		/*
		 * Lite-On PNICs always use the same mediasw; we
		 * select MII vs. internal NWAY automatically.
		 */
		sc->sc_mediasw = &tlp_pnic_mediasw;
		break;

	case TULIP_CHIP_WB89C840F:
		/*
		 * Winbond 89C840F's Ethernet address is the first
		 * 6 bytes of its EEPROM.
		 */
		memcpy(enaddr, romdata, ETHER_ADDR_LEN);

		/*
		 * Winbond 89C840F has an MII attached to the SIO.
		 */
		sc->sc_mediasw = &tlp_sio_mii_mediasw;
		break;

	default:
		printf("%s: sorry, unable to handle your board\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	/*
	 * Map and establish our interrupt.
	 */
	if (pci_intr_map(pc, pa->pa_intrtag, pa->pa_intrpin,
	    pa->pa_intrline, &ih)) {
		printf("%s: unable to map interrupt\n", sc->sc_dev.dv_xname);
		return;
	}
	intrstr = pci_intr_string(pc, ih); 
	psc->sc_ih = pci_intr_establish(pc, ih, IPL_NET, tlp_intr, sc);
	if (psc->sc_ih == NULL) {
		printf("%s: unable to establish interrupt",
		    sc->sc_dev.dv_xname);
		if (intrstr != NULL)
			printf(" at %s", intrstr);
		printf("\n");
		return;
	}
	printf("%s: interrupting at %s\n", sc->sc_dev.dv_xname, intrstr);

	/*
	 * Finish off the attach.
	 */
	tlp_attach(sc, name, enaddr);
}
