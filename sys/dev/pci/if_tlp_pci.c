/*	$NetBSD: if_tlp_pci.c,v 1.20.4.1 1999/11/15 00:41:02 fvdl Exp $	*/

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
#define	TULIP_PCI_CFDA		0x40	/* configuration driver area */

#define	CFDA_SLEEP		0x80000000	/* sleep mode */

struct tulip_pci_softc {
	struct tulip_softc sc_tulip;	/* real Tulip softc */

	/* PCI-specific goo. */
	void	*sc_ih;			/* interrupt handle */

	pci_chipset_tag_t sc_pc;	/* our PCI chipset */
	pcitag_t sc_pcitag;		/* our PCI tag */

	int	sc_flags;		/* flags; see below */

	LIST_HEAD(, tulip_pci_softc) sc_intrslaves;
	LIST_ENTRY(tulip_pci_softc) sc_intrq;

	/* Our {ROM,interrupt} master. */
	struct tulip_pci_softc *sc_master;
};

/* sc_flags */
#define	TULIP_PCI_SHAREDINTR	0x01	/* interrupt is shared */
#define	TULIP_PCI_SLAVEINTR	0x02	/* interrupt is slave */
#define	TULIP_PCI_SHAREDROM	0x04	/* ROM is shared */
#define	TULIP_PCI_SLAVEROM	0x08	/* slave of shared ROM */

int	tlp_pci_match __P((struct device *, struct cfdata *, void *));
void	tlp_pci_attach __P((struct device *, struct device *, void *));

struct cfattach tlp_pci_ca = {
	sizeof(struct tulip_pci_softc), tlp_pci_match, tlp_pci_attach,
};

const struct tulip_pci_product {
	u_int32_t	tpp_vendor;	/* PCI vendor ID */
	u_int32_t	tpp_product;	/* PCI product ID */
	tulip_chip_t	tpp_chip;	/* base Tulip chip type */
	int		tpp_pmreg;	/* power management register offset */
} tlp_pci_products[] = {
#ifdef TLP_MATCH_21040
	{ PCI_VENDOR_DEC,		PCI_PRODUCT_DEC_21040,
	  TULIP_CHIP_21040,		0 },
#endif
#ifdef TLP_MATCH_21041
	{ PCI_VENDOR_DEC,		PCI_PRODUCT_DEC_21041,
	  TULIP_CHIP_21041,		0 },
#endif
#ifdef TLP_MATCH_21140
	{ PCI_VENDOR_DEC,		PCI_PRODUCT_DEC_21140,
	  TULIP_CHIP_21140,		0 },
#endif
#ifdef TLP_MATCH_21142
	{ PCI_VENDOR_DEC,		PCI_PRODUCT_DEC_21142,
	  TULIP_CHIP_21142,		0 },
#endif

	{ PCI_VENDOR_LITEON,		PCI_PRODUCT_LITEON_82C168,
	  TULIP_CHIP_82C168,		0 },

	/*
	 * Note: This is like a MX98725 with Wake-On-LAN and a
	 * 128-bit multicast hash table.
	 */
	{ PCI_VENDOR_LITEON,		PCI_PRODUCT_LITEON_82C115,
	  TULIP_CHIP_82C115,		0x48 },

	{ PCI_VENDOR_MACRONIX,		PCI_PRODUCT_MACRONIX_MX98713,
	  TULIP_CHIP_MX98713,		0 },
	{ PCI_VENDOR_MACRONIX,		PCI_PRODUCT_MACRONIX_MX987x5,
	  TULIP_CHIP_MX98715,		0x48 },

	{ PCI_VENDOR_COMPEX,		PCI_PRODUCT_COMPEX_RL100TX,
	  TULIP_CHIP_MX98713,		0 },

	{ PCI_VENDOR_WINBOND,		PCI_PRODUCT_WINBOND_W89C840F,
	  TULIP_CHIP_WB89C840F,		0 },
	{ PCI_VENDOR_COMPEX,		PCI_PRODUCT_COMPEX_RL100ATX,
	  TULIP_CHIP_WB89C840F,		0 },

#if 0
	{ PCI_VENDOR_DAVICOM,		PCI_PRODUCT_DAVICOM_DM9102,
	  TULIP_CHIP_DM9102,		0 },
#endif

	{ PCI_VENDOR_ADMTEK,		PCI_PRODUCT_ADMTEK_AL981,
	  TULIP_CHIP_AL981,		0xc4 },

#if 0
	{ PCI_VENDOR_ASIX,		PCI_PRODUCT_ASIX_AX88140A,
	  TULIP_CHIP_AX88140,		0 },
#endif

	{ 0,				0,
	  TULIP_CHIP_INVALID,		0 },
};

struct tlp_pci_quirks {
	void		(*tpq_func) __P((struct tulip_pci_softc *,
			    const u_int8_t *));
	u_int8_t	tpq_oui[3];
};

void	tlp_pci_dec_quirks __P((struct tulip_pci_softc *,
	    const u_int8_t *));

void	tlp_pci_znyx_21040_quirks __P((struct tulip_pci_softc *,
	    const u_int8_t *));
void	tlp_pci_smc_21040_quirks __P((struct tulip_pci_softc *,
	    const u_int8_t *));
void	tlp_pci_cogent_21040_quirks __P((struct tulip_pci_softc *,
	    const u_int8_t *));
void	tlp_pci_accton_21040_quirks __P((struct tulip_pci_softc *,
	    const u_int8_t *));

const struct tlp_pci_quirks tlp_pci_21040_quirks[] = {
	{ tlp_pci_znyx_21040_quirks,	{ 0x00, 0xc0, 0x95 } },
	{ tlp_pci_smc_21040_quirks,	{ 0x00, 0x00, 0xc0 } },
	{ tlp_pci_cogent_21040_quirks,	{ 0x00, 0x00, 0x92 } },
	{ tlp_pci_accton_21040_quirks,	{ 0x00, 0x00, 0xe8 } },
	{ NULL,				{ 0, 0, 0 } }
};

const struct tlp_pci_quirks tlp_pci_21041_quirks[] = {
	{ tlp_pci_dec_quirks,		{ 0x08, 0x00, 0x2b } },
	{ tlp_pci_dec_quirks,		{ 0x00, 0x00, 0xf8 } },
	{ NULL,				{ 0, 0, 0 } }
};

void	tlp_pci_asante_21140_quirks __P((struct tulip_pci_softc *,
	    const u_int8_t *));

const struct tlp_pci_quirks tlp_pci_21140_quirks[] = {
	{ tlp_pci_dec_quirks,		{ 0x08, 0x00, 0x2b } },
	{ tlp_pci_dec_quirks,		{ 0x00, 0x00, 0xf8 } },
	{ tlp_pci_asante_21140_quirks,	{ 0x00, 0x00, 0x94 } },
	{ NULL,				{ 0, 0, 0 } }
};

const struct tlp_pci_quirks tlp_pci_21142_quirks[] = {
	{ tlp_pci_dec_quirks,		{ 0x08, 0x00, 0x2b } },
	{ tlp_pci_dec_quirks,		{ 0x00, 0x00, 0xf8 } },
	{ NULL,				{ 0, 0, 0 } }
};

const char *tlp_pci_chip_names[] = TULIP_CHIP_NAMES;

int	tlp_pci_shared_intr __P((void *));

const struct tulip_pci_product *tlp_pci_lookup
    __P((const struct pci_attach_args *));
void tlp_pci_get_quirks __P((struct tulip_pci_softc *, const u_int8_t *,
    const struct tlp_pci_quirks *));
void tlp_pci_check_slaved __P((struct tulip_pci_softc *, int, int));

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

void
tlp_pci_get_quirks(psc, enaddr, tpq)
	struct tulip_pci_softc *psc;
	const u_int8_t *enaddr;
	const struct tlp_pci_quirks *tpq;
{

	for (; tpq->tpq_func != NULL; tpq++) {
		if (tpq->tpq_oui[0] == enaddr[0] &&
		    tpq->tpq_oui[1] == enaddr[1] &&
		    tpq->tpq_oui[2] == enaddr[2]) {
			(*tpq->tpq_func)(psc, enaddr);
			return;
		}
	}
}

void
tlp_pci_check_slaved(psc, shared, slaved)
	struct tulip_pci_softc *psc;
	int shared, slaved;
{
	extern struct cfdriver tlp_cd;
	struct tulip_pci_softc *cur, *best = NULL;
	struct tulip_softc *sc = &psc->sc_tulip;
	int i;

	/*
	 * First of all, find the lowest pcidev numbered device on our
	 * bus marked as shared.  That should be our master.
	 */
	for (i = 0; i < tlp_cd.cd_ndevs; i++) {
		if ((cur = tlp_cd.cd_devs[i]) == NULL)
			continue;
		if (cur->sc_tulip.sc_dev.dv_parent != sc->sc_dev.dv_parent)
			continue;
		if ((cur->sc_flags & shared) == 0)
			continue;
		if (cur == psc)
			continue;
		if (best == NULL ||
		    best->sc_tulip.sc_devno > cur->sc_tulip.sc_devno)
			best = cur;
	}

	if (best != NULL) {
		psc->sc_master = best;
		psc->sc_flags |= (shared | slaved);
	}
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
	u_int8_t enaddr[ETHER_ADDR_LEN];
	u_int32_t val;
	pcireg_t reg;

	sc->sc_devno = pa->pa_device;
	psc->sc_pc = pa->pa_pc;
	psc->sc_pcitag = pa->pa_tag;

	LIST_INIT(&psc->sc_intrslaves);

	tpp = tlp_pci_lookup(pa);
	if (tpp == NULL) {
		printf("\n");
		panic("tlp_pci_attach: impossible");
	}
	sc->sc_chip = tpp->tpp_chip;

	/*
	 * By default, Tulip registers are 8 bytes long (4 bytes
	 * followed by a 4 byte pad).
	 */
	sc->sc_regshift = 3;

	/*
	 * Get revision info, and set some chip-specific variables.
	 */
	sc->sc_rev = PCI_REVISION(pa->pa_class);
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
		if (sc->sc_rev >= 0x20)
			sc->sc_chip = TULIP_CHIP_MX98715A;
		if (sc->sc_rev >= 0x30)
			sc->sc_chip = TULIP_CHIP_MX98725;
		break;

	case TULIP_CHIP_WB89C840F:
		sc->sc_regshift = 2;
		break;

	case TULIP_CHIP_AX88140:
		if (sc->sc_rev >= 0x10)
			sc->sc_chip = TULIP_CHIP_AX88141;
		break;

	default:
		/* Nothing. */
	}

	printf(": %s Ethernet, pass %d.%d\n",
	    tlp_pci_chip_names[sc->sc_chip],
	    (sc->sc_rev >> 4) & 0xf, sc->sc_rev & 0xf);

	switch (sc->sc_chip) {
	case TULIP_CHIP_21040:
		if (sc->sc_rev < 0x20) {
			printf("%s: 21040 must be at least pass 2.0\n",
			    sc->sc_dev.dv_xname);
			return;
		}
		break;

	case TULIP_CHIP_21140:
		if (sc->sc_rev < 0x11) {
			printf("%s: 21140 must be at least pass 1.1\n",
			    sc->sc_dev.dv_xname);
			return;
		}
		break;

	default:
		/* Nothing. */
	}

	/*
	 * Check to see if the device is in power-save mode, and
	 * being it out if necessary.
	 */
	switch (sc->sc_chip) {
	case TULIP_CHIP_21140:
	case TULIP_CHIP_21140A:
	case TULIP_CHIP_MX98713A:
	case TULIP_CHIP_MX98715:
	case TULIP_CHIP_MX98715A:
	case TULIP_CHIP_MX98725:
		/*
		 * Clear the "sleep mode" bit in the CFDA register.
		 */
		reg = pci_conf_read(pc, pa->pa_tag, TULIP_PCI_CFDA);
		if (reg & CFDA_SLEEP)
			pci_conf_write(pc, pa->pa_tag, TULIP_PCI_CFDA,
			    reg & ~CFDA_SLEEP);
		break;

	default:
		/* Nothing. */
	}

	if (pci_get_capability(pc, pa->pa_tag, PCI_CAP_PWRMGMT, 0, 0)) {
		if (tpp->tpp_pmreg == 0) {
			printf("%s: don't know location of PMCSR for this "
			    "chip\n", sc->sc_dev.dv_xname);
			return;
		}
		reg = pci_conf_read(pc, pa->pa_tag, tpp->tpp_pmreg) & 0x3;
		if (reg == 3) {
			/*
			 * The card has lost all configuration data in
			 * this state, so punt.
			 */
			printf("%s: unable to wake up from power state D3\n",
			    sc->sc_dev.dv_xname);
			return;
		}
		if (reg != 0) {
			printf("%s: waking up from power state D%d\n",
			    sc->sc_dev.dv_xname, reg);
			pci_conf_write(pc, pa->pa_tag, tpp->tpp_pmreg, 0);
		}
	}

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
	 * Get PCI data moving command info.
	 */
	if (pa->pa_flags & PCI_FLAGS_MRL_OKAY)
		sc->sc_flags |= TULIPF_MRL;
	if (pa->pa_flags & PCI_FLAGS_MRM_OKAY)
		sc->sc_flags |= TULIPF_MRM;
	if (pa->pa_flags & PCI_FLAGS_MWI_OKAY)
		sc->sc_flags |= TULIPF_MWI;

	/*
	 * Read the contents of the Ethernet Address ROM/SROM.
	 */
	memset(sc->sc_srom, 0, sizeof(sc->sc_srom));
	switch (sc->sc_chip) {
	case TULIP_CHIP_21040:
		TULIP_WRITE(sc, CSR_MIIROM, MIIROM_SROMCS);
		for (i = 0; i < sizeof(sc->sc_srom); i++) {
			for (j = 0; j < 10000; j++) {
				val = TULIP_READ(sc, CSR_MIIROM);
				if ((val & MIIROM_DN) == 0)
					break;
			}
			sc->sc_srom[i] = val & MIIROM_DATA;
		}
		break;

	case TULIP_CHIP_82C168:
	case TULIP_CHIP_82C169:
	    {
		u_int16_t *rombuf = (u_int16_t *)sc->sc_srom;

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
	    }

	default:
		tlp_read_srom(sc, 0, sizeof(sc->sc_srom) >> 1, sc->sc_srom);
#if 0
		printf("SROM CONTENTS:");
		for (i = 0; i < sizeof(sc->sc_srom); i++) {
			if ((i % 8) == 0)
				printf("\n\t");
			printf("0x%02x ", sc->sc_srom[i]);
		}
		printf("\n");
#endif
	}

	/*
	 * Deal with chip/board quirks.  This includes setting up
	 * the mediasw, and extracting the Ethernet address from
	 * the rombuf.
	 */
	switch (sc->sc_chip) {
	case TULIP_CHIP_21040:
		/* Check for a slaved ROM on a multi-port board. */
		tlp_pci_check_slaved(psc, TULIP_PCI_SHAREDROM,
		    TULIP_PCI_SLAVEROM);
		if (psc->sc_flags & TULIP_PCI_SLAVEROM)
			memcpy(sc->sc_srom, psc->sc_master->sc_tulip.sc_srom,
			    sizeof(sc->sc_srom));

		/*
		 * Parse the Ethernet Address ROM.
		 */
		if (tlp_parse_old_srom(sc, enaddr) == 0) {
			printf("%s: unable to decode Ethernet Address ROM\n",
			    sc->sc_dev.dv_xname);
			return;
		}

		/*
		 * If we have a slaved ROM, adjust the Ethernet address.
		 */
		if (psc->sc_flags & TULIP_PCI_SLAVEROM)
			enaddr[5] +=
			    sc->sc_devno - psc->sc_master->sc_tulip.sc_devno;

		/*
		 * All 21040 boards start out with the same
		 * media switch.
		 */
		sc->sc_mediasw = &tlp_21040_mediasw;

		/*
		 * Deal with any quirks this board might have.
		 */
		tlp_pci_get_quirks(psc, enaddr, tlp_pci_21040_quirks);
		break;

	case TULIP_CHIP_21041:
		/* Check for a slaved ROM on a multi-port board. */
		tlp_pci_check_slaved(psc, TULIP_PCI_SHAREDROM,
		    TULIP_PCI_SLAVEROM);
		if (psc->sc_flags & TULIP_PCI_SLAVEROM)
			memcpy(sc->sc_srom, psc->sc_master->sc_tulip.sc_srom,
			    sizeof(sc->sc_srom));

		/* Check for new format SROM. */
		if (tlp_isv_srom_enaddr(sc, enaddr) == 0) {
			/*
			 * Not an ISV SROM; try the old DEC Ethernet Address
			 * ROM format.
			 */
			if (tlp_parse_old_srom(sc, enaddr) == 0) {
				printf("%s: unable to decode Ethernet "
				    "Address ROM\n", sc->sc_dev.dv_xname);
				return;
			}
		}

		/*
		 * All 21041 boards use the same media switch; they all
		 * work basically the same!  Yippee!
		 */
		sc->sc_mediasw = &tlp_21041_mediasw;

		/*
		 * Deal with any quirks this board might have.
		 */
		tlp_pci_get_quirks(psc, enaddr, tlp_pci_21041_quirks);
		break;

	case TULIP_CHIP_21140:
	case TULIP_CHIP_21140A:
		/* Check for new format SROM. */
		if (tlp_isv_srom_enaddr(sc, enaddr) == 0) {
			/*
			 * Not an ISV SROM; try the old DEC Ethernet Address
			 * ROM format.
			 */
			if (tlp_parse_old_srom(sc, enaddr) == 0) {
				printf("%s: unable to decode Ethernet "
				    "Address ROM\n", sc->sc_dev.dv_xname);
				return;
			}
		} else {
			/*
			 * We start out with the 2114x ISV media switch.
			 * When we search for quirks, we may change to
			 * a different switch.
			 */
			sc->sc_mediasw = &tlp_2114x_isv_mediasw;
		}

		/*
		 * Deal with any quirks this board might have.
		 */
		tlp_pci_get_quirks(psc, enaddr, tlp_pci_21140_quirks);

		/*
		 * Bail out now if we can't deal with this board.
		 */
		if (sc->sc_mediasw == NULL)
			goto cant_cope;
		break;

	case TULIP_CHIP_21142:
	case TULIP_CHIP_21143:
		/* Check for new format SROM. */
		if (tlp_isv_srom_enaddr(sc, enaddr) == 0) {
			/*
			 * Not an ISV SROM; can't cope, for now.
			 */
			goto cant_cope;
		} else {
			/*
			 * We start out with the 2114x ISV media switch.
			 * When we search for quirks, we may change to
			 * a different switch.
			 */
			sc->sc_mediasw = &tlp_2114x_isv_mediasw;
		}

		/*
		 * Deal with any quirks this board might have.
		 */
		tlp_pci_get_quirks(psc, enaddr, tlp_pci_21142_quirks);

		/*
		 * Bail out now if we can't deal with this board.
		 */
		if (sc->sc_mediasw == NULL)
			goto cant_cope;
		break;

	case TULIP_CHIP_82C168:
	case TULIP_CHIP_82C169:
		/*
		 * Lite-On PNIC's Ethernet address is the first 6
		 * bytes of its EEPROM.
		 */
		memcpy(enaddr, sc->sc_srom, ETHER_ADDR_LEN);

		/*
		 * Lite-On PNICs always use the same mediasw; we
		 * select MII vs. internal NWAY automatically.
		 */
		sc->sc_mediasw = &tlp_pnic_mediasw;
		break;

	case TULIP_CHIP_MX98713:
		/*
		 * The Macronix MX98713 has an MII and GPIO, but no
		 * internal Nway block.  This chip is basically a
		 * perfect 21140A clone, with the exception of the
		 * a magic register frobbing in order to make the
		 * interface function.
		 */
		if (tlp_isv_srom_enaddr(sc, enaddr)) {
			sc->sc_mediasw = &tlp_2114x_isv_mediasw;
			break;
		}
		/* FALLTHROUGH */

	case TULIP_CHIP_82C115:
		/*
		 * Yippee!  The Lite-On 82C115 is a clone of
		 * the MX98725 (the data sheet even says `MXIC'
		 * on it)!  Imagine that, a clone of a clone.
		 *
		 * The differences are really minimal:
		 *
		 *	- Wake-On-LAN support
		 *	- 128-bit multicast hash table, rather than
		 *	  the standard 512-bit hash table
		 */
		/* FALLTHROUGH */

	case TULIP_CHIP_MX98713A:
	case TULIP_CHIP_MX98715A:
	case TULIP_CHIP_MX98725:
		/*
		 * The MX98713A has an MII as well as an internal Nway block,
		 * but no GPIO.  The MX98715 and MX98725 have an internal
		 * Nway block only.
		 *
		 * The internal Nway block, unlike the Lite-On PNIC's, does
		 * just that - performs Nway.  Once autonegotiation completes,
		 * we must program the GPR media information into the chip.
		 *
		 * The byte offset of the Ethernet address is stored at
		 * offset 0x70.
		 */
		memcpy(enaddr, &sc->sc_srom[sc->sc_srom[0x70]], ETHER_ADDR_LEN);
		sc->sc_mediasw = &tlp_pmac_mediasw;
		break;

	case TULIP_CHIP_WB89C840F:
		/*
		 * Winbond 89C840F's Ethernet address is the first
		 * 6 bytes of its EEPROM.
		 */
		memcpy(enaddr, sc->sc_srom, ETHER_ADDR_LEN);

		/*
		 * Winbond 89C840F has an MII attached to the SIO.
		 */
		sc->sc_mediasw = &tlp_sio_mii_mediasw;
		break;

	case TULIP_CHIP_AL981:
		/*
		 * The ADMtek AL981's Ethernet address is located
		 * at offset 8 of its EEPROM.
		 */
		memcpy(enaddr, &sc->sc_srom[8], ETHER_ADDR_LEN);

		/*
		 * ADMtek AL981 has a built-in PHY accessed through
		 * special registers.
		 */
		sc->sc_mediasw = &tlp_al981_mediasw;
		break;

	default:
 cant_cope:
		printf("%s: sorry, unable to handle your board\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	/*
	 * Handle shared interrupts.
	 */
	if (psc->sc_flags & TULIP_PCI_SHAREDINTR) {
		if (psc->sc_master)
			psc->sc_flags |= TULIP_PCI_SLAVEINTR;
		else {
			tlp_pci_check_slaved(psc, TULIP_PCI_SHAREDINTR,
			    TULIP_PCI_SLAVEINTR);
			if (psc->sc_master == NULL)
				psc->sc_master = psc;
		}
		LIST_INSERT_HEAD(&psc->sc_master->sc_intrslaves,
		    psc, sc_intrq);
	}

	if (psc->sc_flags & TULIP_PCI_SLAVEINTR) {
		printf("%s: sharing interrupt with %s\n",
		    sc->sc_dev.dv_xname,
		    psc->sc_master->sc_tulip.sc_dev.dv_xname);
	} else {
		/*
		 * Map and establish our interrupt.
		 */
		if (pci_intr_map(pc, pa->pa_intrtag, pa->pa_intrpin,
		    pa->pa_intrline, &ih)) {
			printf("%s: unable to map interrupt\n",
			    sc->sc_dev.dv_xname);
			return;
		}
		intrstr = pci_intr_string(pc, ih); 
		psc->sc_ih = pci_intr_establish(pc, ih, IPL_NET,
		    (psc->sc_flags & TULIP_PCI_SHAREDINTR) ?
		    tlp_pci_shared_intr : tlp_intr, sc);
		if (psc->sc_ih == NULL) {
			printf("%s: unable to establish interrupt",
			    sc->sc_dev.dv_xname);
			if (intrstr != NULL)
				printf(" at %s", intrstr);
			printf("\n");
			return;
		}
		printf("%s: interrupting at %s\n", sc->sc_dev.dv_xname,
		    intrstr);
	}

	/*
	 * Finish off the attach.
	 */
	tlp_attach(sc, enaddr);
}

int
tlp_pci_shared_intr(arg)
	void *arg;
{
	struct tulip_pci_softc *master = arg, *slave;
	int rv = 0;

	for (slave = LIST_FIRST(&master->sc_intrslaves);
	     slave != NULL;
	     slave = LIST_NEXT(slave, sc_intrq))
		rv |= tlp_intr(&slave->sc_tulip);
	
	return (rv);
}

void
tlp_pci_dec_quirks(psc, enaddr)
	struct tulip_pci_softc *psc;
	const u_int8_t *enaddr;
{
	struct tulip_softc *sc = &psc->sc_tulip;

	/*
	 * This isn't really a quirk-gathering device, really.  We
	 * just want to get the spiffy DEC board name from the SROM.
	 */
	strcpy(sc->sc_name, "DEC ");

	if (memcmp(&sc->sc_srom[29], "DE500", 5) == 0 ||
	    memcmp(&sc->sc_srom[29], "DE450", 5) == 0)
		memcpy(&sc->sc_name[4], &sc->sc_srom[29], 8);
}

void
tlp_pci_znyx_21040_quirks(psc, enaddr)
	struct tulip_pci_softc *psc;
	const u_int8_t *enaddr;
{
	struct tulip_softc *sc = &psc->sc_tulip;
	u_int16_t id = 0;

	/*
	 * If we have a slaved ROM, just copy the bits from the master.
	 * This is in case we fail the ROM ID check (older boards) and
	 * need to fall back on Ethernet address model checking; that
	 * will fail for slave chips.
	 */
	if (psc->sc_flags & TULIP_PCI_SLAVEROM) {
		strcpy(sc->sc_name, psc->sc_master->sc_tulip.sc_name);
		sc->sc_mediasw = psc->sc_master->sc_tulip.sc_mediasw;
		psc->sc_flags |=
		    psc->sc_master->sc_flags & TULIP_PCI_SHAREDINTR;
		return;
	}

	if (sc->sc_srom[32] == 0x4a && sc->sc_srom[33] == 0x52) {
		id = sc->sc_srom[37] | (sc->sc_srom[36] << 8);
		switch (id) {
 zx312:
		case 0x0602:	/* ZX312 */
			strcpy(sc->sc_name, "ZNYX ZX312");
			return;

		case 0x0622:	/* ZX312T */
			strcpy(sc->sc_name, "ZNYX ZX312T");
			sc->sc_mediasw = &tlp_21040_tp_mediasw;
			return;

 zx314_inta:
		case 0x0701:	/* ZX314 INTA */
			psc->sc_flags |= TULIP_PCI_SHAREDINTR;
			/* FALLTHROUGH */
		case 0x0711:	/* ZX314 */
			strcpy(sc->sc_name, "ZNYX ZX314");
			psc->sc_flags |= TULIP_PCI_SHAREDROM;
			sc->sc_mediasw = &tlp_21040_tp_mediasw;
			return;

 zx315_inta:
		case 0x0801:	/* ZX315 INTA */
			psc->sc_flags |= TULIP_PCI_SHAREDINTR;
			/* FALLTHROUGH */
		case 0x0811:	/* ZX315 */
			strcpy(sc->sc_name, "ZNYX ZX315");
			psc->sc_flags |= TULIP_PCI_SHAREDROM;
			return;

		default:
			id = 0;
		}
	}

	/*
	 * Deal with boards that have broken ROMs.
	 */
	if (id == 0) {
		if ((enaddr[3] & ~3) == 0xf0 && (enaddr[5] & 3) == 0x00)
			goto zx314_inta;
		if ((enaddr[3] & ~3) == 0xf4 && (enaddr[5] & 1) == 0x00)
			goto zx315_inta;
		if ((enaddr[3] & ~3) == 0xec)
			goto zx312;
	}

	strcpy(sc->sc_name, "ZNYX ZX31x");
}

void
tlp_pci_smc_21040_quirks(psc, enaddr)
	struct tulip_pci_softc *psc;
	const u_int8_t *enaddr;
{
	struct tulip_softc *sc = &psc->sc_tulip;
	u_int16_t id1, id2, ei;
	int auibnc = 0, utp = 0;
	char *cp;

	id1 = sc->sc_srom[0x60] | (sc->sc_srom[0x61] << 8);
	id2 = sc->sc_srom[0x62] | (sc->sc_srom[0x63] << 8);
	ei  = sc->sc_srom[0x66] | (sc->sc_srom[0x67] << 8);

	strcpy(sc->sc_name, "SMC 8432");
	cp = &sc->sc_name[8];

	if ((id1 & 1) == 0) {
		*cp++ = 'B';
		auibnc = 1;
	}
	if ((id1 & 0xff) > 0x32) {
		*cp++ = 'T';
		utp = 1;
	}
	if ((id1 & 0x4000) == 0) {
		*cp++ = 'A';
		auibnc = 1;
	}
	if (id2 == 0x15) {
		sc->sc_name[7] = '4';
		*cp++ = '-';
		*cp++ = 'C';
		*cp++ = 'H';
		*cp++ = ei ? '2' : '1';
	}
	*cp = '\0';

	if (utp != 0 && auibnc == 0)
		sc->sc_mediasw = &tlp_21040_tp_mediasw;
	else if (utp == 0 && auibnc != 0)
		sc->sc_mediasw = &tlp_21040_auibnc_mediasw;
}

void
tlp_pci_cogent_21040_quirks(psc, enaddr)
	struct tulip_pci_softc *psc;
	const u_int8_t *enaddr;
{

	strcpy(psc->sc_tulip.sc_name, "Cogent multi-port");
	psc->sc_flags |= TULIP_PCI_SHAREDINTR|TULIP_PCI_SHAREDROM;
}

void
tlp_pci_accton_21040_quirks(psc, enaddr)
	struct tulip_pci_softc *psc;
	const u_int8_t *enaddr;
{

	strcpy(psc->sc_tulip.sc_name, "ACCTON EN1203");
}

void	tlp_pci_asante_21140_reset __P((struct tulip_softc *));

void
tlp_pci_asante_21140_quirks(psc, enaddr)
	struct tulip_pci_softc *psc;
	const u_int8_t *enaddr;
{
	struct tulip_softc *sc = &psc->sc_tulip;

	/*
	 * Some Asante boards don't use the ISV SROM format.  For
	 * those that don't, we initialize the GPIO direction bits,
	 * and provide our own reset hook, which resets the MII.
	 *
	 * All of these boards use SIO-attached-MII media.
	 */
	if (sc->sc_mediasw == &tlp_2114x_isv_mediasw)
		return;

	strcpy(sc->sc_name, "Asante");

	sc->sc_gp_dir = 0xbf;
	sc->sc_reset = tlp_pci_asante_21140_reset;
	sc->sc_mediasw = &tlp_sio_mii_mediasw;
}

void
tlp_pci_asante_21140_reset(sc)
	struct tulip_softc *sc;
{

	TULIP_WRITE(sc, CSR_GPP, GPP_GPC | sc->sc_gp_dir);
	TULIP_WRITE(sc, CSR_GPP, 0x8);
	delay(100);
	TULIP_WRITE(sc, CSR_GPP, 0);
}
