/*	$NetBSD: if_tlp_cardbus.c,v 1.25 2001/01/16 18:55:00 thorpej Exp $	*/

/*-
 * Copyright (c) 1999, 2000 The NetBSD Foundation, Inc.
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
 * CardBus bus front-end for the Digital Semiconductor ``Tulip'' (21x4x)
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

#include <machine/endian.h>
 
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
#include <dev/mii/mii_bitbang.h>

#include <dev/ic/tulipreg.h>
#include <dev/ic/tulipvar.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#include <dev/cardbus/cardbusvar.h>
#include <dev/cardbus/cardbusdevs.h>

/*
 * PCI configuration space registers used by the Tulip.
 */
#define	TULIP_PCI_IOBA		0x10	/* i/o mapped base */
#define	TULIP_PCI_MMBA		0x14	/* memory mapped base */
#define	TULIP_PCI_CFDA		0x40	/* configuration driver area */

#define	CFDA_SLEEP		0x80000000	/* sleep mode */
#define	CFDA_SNOOZE		0x40000000	/* snooze mode */

struct tulip_cardbus_softc {
	struct tulip_softc sc_tulip;	/* real Tulip softc */

	/* CardBus-specific goo. */
	void	*sc_ih;			/* interrupt handle */
	cardbus_devfunc_t sc_ct;	/* our CardBus devfuncs */
	cardbustag_t sc_tag;		/* our CardBus tag */
	int	sc_csr;			/* CSR bits */
	bus_size_t sc_mapsize;		/* the size of mapped bus space
					   region */

	int	sc_cben;		/* CardBus enables */
	int	sc_bar_reg;		/* which BAR to use */
	pcireg_t sc_bar_val;		/* value of the BAR */

	int	sc_intrline;		/* interrupt line */
};

int	tlp_cardbus_match __P((struct device *, struct cfdata *, void *));
void	tlp_cardbus_attach __P((struct device *, struct device *, void *));
int	tlp_cardbus_detach __P((struct device *, int));

struct cfattach tlp_cardbus_ca = {
	sizeof(struct tulip_cardbus_softc),
	    tlp_cardbus_match, tlp_cardbus_attach,
		tlp_cardbus_detach, tlp_activate,
};

const struct tulip_cardbus_product {
	u_int32_t	tcp_vendor;	/* PCI vendor ID */
	u_int32_t	tcp_product;	/* PCI product ID */
	tulip_chip_t	tcp_chip;	/* base Tulip chip type */
} tlp_cardbus_products[] = {
	{ PCI_VENDOR_DEC,		PCI_PRODUCT_DEC_21142,
	  TULIP_CHIP_21142 },

	{ PCI_VENDOR_XIRCOM,		PCI_PRODUCT_XIRCOM_X3201_3_21143,
	  TULIP_CHIP_X3201_3 },

	{ CARDBUS_VENDOR_ACCTON,	CARDBUS_PRODUCT_ACCTON_EN2242,
	  TULIP_CHIP_AN985 },

	{ CARDBUS_VENDOR_ABOCOM,	CARDBUS_PRODUCT_ABOCOM_FE2500,
	  TULIP_CHIP_AN985 },

	{ 0,				0,
	  TULIP_CHIP_INVALID },
};

void	tlp_cardbus_setup __P((struct tulip_cardbus_softc *));

int	tlp_cardbus_enable __P((struct tulip_softc *));
void	tlp_cardbus_disable __P((struct tulip_softc *));
void	tlp_cardbus_power __P((struct tulip_softc *, int));

void	tlp_cardbus_x3201_reset __P((struct tulip_softc *));

const struct tulip_cardbus_product *tlp_cardbus_lookup
    __P((const struct cardbus_attach_args *));

const struct tulip_cardbus_product *
tlp_cardbus_lookup(ca)
	const struct cardbus_attach_args *ca;
{
	const struct tulip_cardbus_product *tcp;

	for (tcp = tlp_cardbus_products;
	     tlp_chip_names[tcp->tcp_chip] != NULL;
	     tcp++) {
		if (PCI_VENDOR(ca->ca_id) == tcp->tcp_vendor &&
		    PCI_PRODUCT(ca->ca_id) == tcp->tcp_product)
			return (tcp);
	}
	return (NULL);
}

int
tlp_cardbus_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct cardbus_attach_args *ca = aux;

	if (tlp_cardbus_lookup(ca) != NULL)
		return (1);

	return (0);
}

void
tlp_cardbus_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct tulip_cardbus_softc *csc = (void *)self;
	struct tulip_softc *sc = &csc->sc_tulip;
	struct cardbus_attach_args *ca = aux;
	cardbus_devfunc_t ct = ca->ca_ct;
	const struct tulip_cardbus_product *tcp;
	u_int8_t enaddr[ETHER_ADDR_LEN];
	bus_addr_t adr;
	pcireg_t reg;

	sc->sc_devno = ca->ca_device;
	sc->sc_dmat = ca->ca_dmat;
	csc->sc_ct = ct;
	csc->sc_tag = ca->ca_tag;

	tcp = tlp_cardbus_lookup(ca);
	if (tcp == NULL) {
		printf("\n");
		panic("tlp_cardbus_attach: impossible");
	}
	sc->sc_chip = tcp->tcp_chip;

	/*
	 * By default, Tulip registers are 8 bytes long (4 bytes
	 * followed by a 4 byte pad).
	 */
	sc->sc_regshift = 3;

	/*
	 * Power management hooks.
	 */
	sc->sc_enable = tlp_cardbus_enable;
	sc->sc_disable = tlp_cardbus_disable;
	sc->sc_power = tlp_cardbus_power;

	/*
	 * Get revision info, and set some chip-specific variables.
	 */
	sc->sc_rev = PCI_REVISION(ca->ca_class);
	switch (sc->sc_chip) {
	case TULIP_CHIP_21142:
		if (sc->sc_rev >= 0x20)
			sc->sc_chip = TULIP_CHIP_21143;
		break;

	case TULIP_CHIP_AN985:
		/*
		 * The AN983 and AN985 are very similar, and are
		 * differentiated by a "signature" register that
		 * is like, but not identical, to a PCI ID register.
		 */
		reg = cardbus_conf_read(ct->ct_cc, ct->ct_cf, csc->sc_tag,
		    0x80);
		switch (reg) {
		case 0x09811317:
			sc->sc_chip = TULIP_CHIP_AN985;
			break;

		case 0x09851317:
			sc->sc_chip = TULIP_CHIP_AN983;
			break;

		default:
			/* Unknown -- use default. */
		}
		break;

	default:
		/* Nothing. */
	}

	printf(": %s Ethernet, pass %d.%d\n",
	    tlp_chip_names[sc->sc_chip],
	    (sc->sc_rev >> 4) & 0xf, sc->sc_rev & 0xf);

	/*
	 * Map the device.
	 */
	csc->sc_csr = PCI_COMMAND_MASTER_ENABLE;
	if (Cardbus_mapreg_map(ct, TULIP_PCI_IOBA,
	    PCI_MAPREG_TYPE_IO, 0, &sc->sc_st, &sc->sc_sh, &adr,
	    &csc->sc_mapsize) == 0) {
#if rbus
#else
		(*ct->ct_cf->cardbus_io_open)(cc, 0, adr, adr+csc->sc_mapsize);
#endif
		csc->sc_cben = CARDBUS_IO_ENABLE;
		csc->sc_csr |= PCI_COMMAND_IO_ENABLE;
		csc->sc_bar_reg = TULIP_PCI_IOBA;
		csc->sc_bar_val = adr | PCI_MAPREG_TYPE_IO;
	} else if (Cardbus_mapreg_map(ct, TULIP_PCI_MMBA,
	    PCI_MAPREG_TYPE_MEM|PCI_MAPREG_MEM_TYPE_32BIT, 0,
	    &sc->sc_st, &sc->sc_sh, &adr, &csc->sc_mapsize) == 0) {
#if rbus
#else
		(*ct->ct_cf->cardbus_mem_open)(cc, 0, adr, adr+csc->sc_mapsize);
#endif
		csc->sc_cben = CARDBUS_MEM_ENABLE;
		csc->sc_csr |= PCI_COMMAND_MEM_ENABLE;
		csc->sc_bar_reg = TULIP_PCI_MMBA;
		csc->sc_bar_val = adr | PCI_MAPREG_TYPE_MEM;
	} else {
		printf("%s: unable to map device registers\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	/*
	 * Bring the chip out of powersave mode and initialize the
	 * configuration registers.
	 */
	tlp_cardbus_setup(csc);

	/*
	 * Read the contents of the Ethernet Address ROM/SROM.
	 */
	switch (sc->sc_chip) {
	case TULIP_CHIP_X3201_3:
		/*
		 * No SROM on this chip.
		 */
		break;

	default:
		if (tlp_read_srom(sc) == 0)
			goto cant_cope;
		break;
	}

	/*
	 * Deal with chip/board quirks.  This includes setting up
	 * the mediasw, and extracting the Ethernet address from
	 * the rombuf.
	 */
	switch (sc->sc_chip) {
	case TULIP_CHIP_21142:
	case TULIP_CHIP_21143:
		/* Check for new format SROM. */
		if (tlp_isv_srom_enaddr(sc, enaddr) != 0) {
			/*
			 * We start out with the 2114x ISV media switch.
			 * When we search for quirks, we may change to
			 * a different switch.
			 */
			sc->sc_mediasw = &tlp_2114x_isv_mediasw;
		} else if (tlp_parse_old_srom(sc, enaddr) == 0) {
			/*
			 * Not an ISV SROM, and not in old DEC Address
			 * ROM format.  Try to snarf it out of the CIS.
			 */
			if (ca->ca_cis.funce.network.netid_present == 0)
				goto cant_cope;

			/* Grab the MAC address from the CIS. */
			memcpy(enaddr, ca->ca_cis.funce.network.netid,
			    sizeof(enaddr));
		}

		/* XXX XXX XXX QUIRKS XXX XXX XXX */

		/*
		 * If we don't already have a media switch, default to
		 * MII-over-SIO, with no special reset routine.
		 */
		if (sc->sc_mediasw == NULL) {
			printf("%s: defaulting to MII-over-SIO; no bets...\n",
			    sc->sc_dev.dv_xname);
			sc->sc_mediasw = &tlp_sio_mii_mediasw;
		}
		break;

	case TULIP_CHIP_AN983:
	case TULIP_CHIP_AN985:
		/*
		 * The ADMtek AN985's Ethernet address is located
		 * at offset 8 of its EEPROM.
		 */
		memcpy(enaddr, &sc->sc_srom[8], ETHER_ADDR_LEN);

		/*
		 * The ADMtek AN985 can be configured in Single-Chip
		 * mode or MAC-only mode.  Single-Chip uses the built-in
		 * PHY, MAC-only has an external PHY (usually HomePNA).
		 * The selection is based on an EEPROM setting, and both
		 * PHYs are access via MII attached to SIO.
		 *
		 * The AN985 "ghosts" the internal PHY onto all
		 * MII addresses, so we have to use a media init
		 * routine that limits the search.
		 * XXX How does this work with MAC-only mode?
		 */
		sc->sc_mediasw = &tlp_an985_mediasw;
		break;

	case TULIP_CHIP_X3201_3:
		/*
		 * The X3201 doesn't have an SROM.  Lift the MAC address
		 * from the CIS.  Also, we have a special media switch:
		 * MII-on-SIO, plus some special GPIO setup.
		 */
		memcpy(enaddr, ca->ca_cis.funce.network.netid, sizeof(enaddr));
		sc->sc_reset = tlp_cardbus_x3201_reset;
		sc->sc_mediasw = &tlp_sio_mii_mediasw;
		break;

	default:
 cant_cope:
		printf("%s: sorry, unable to handle your board\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	/* Remember which interrupt line. */
	csc->sc_intrline = ca->ca_intrline;

	/*
	 * The CardBus cards will make it to store-and-forward mode as
	 * soon as you put them under any kind of load, so just start
	 * out there.
	 */
	sc->sc_txthresh = TXTH_SF;

	/*
	 * Finish off the attach.
	 */
	tlp_attach(sc, enaddr);

	/*
	 * Power down the socket.
	 */
	Cardbus_function_disable(csc->sc_ct);
}

int
tlp_cardbus_detach(self, flags)
	struct device *self;
	int flags;
{
	struct tulip_cardbus_softc *csc = (void *)self;
	struct tulip_softc *sc = &csc->sc_tulip;
	struct cardbus_devfunc *ct = csc->sc_ct;
	int rv;

#if defined(DIAGNOSTIC)
	if (ct == NULL)
		panic("%s: data structure lacks\n", sc->sc_dev.dv_xname);
#endif

	rv = tlp_detach(sc);
	if (rv)
		return (rv);

	/*
	 * Unhook the interrupt handler.
	 */
	if (csc->sc_ih != NULL)
		cardbus_intr_disestablish(ct->ct_cc, ct->ct_cf, csc->sc_ih);

	/*
	 * Release bus space and close window.
	 */
	if (csc->sc_bar_reg != 0)
		Cardbus_mapreg_unmap(ct, csc->sc_bar_reg,
		    sc->sc_st, sc->sc_sh, csc->sc_mapsize);

	return (0);
}

int
tlp_cardbus_enable(sc)
	struct tulip_softc *sc;
{
	struct tulip_cardbus_softc *csc = (void *) sc;
	cardbus_devfunc_t ct = csc->sc_ct;
	cardbus_chipset_tag_t cc = ct->ct_cc;
	cardbus_function_tag_t cf = ct->ct_cf;

	/*
	 * Power on the socket.
	 */
	Cardbus_function_enable(ct);

	/*
	 * Set up the PCI configuration registers.
	 */
	tlp_cardbus_setup(csc);

	/*
	 * Map and establish the interrupt.
	 */
	csc->sc_ih = cardbus_intr_establish(cc, cf, csc->sc_intrline, IPL_NET,
	    tlp_intr, sc);
	if (csc->sc_ih == NULL) {
		printf("%s: unable to establish interrupt at %d\n",
		    sc->sc_dev.dv_xname, csc->sc_intrline);
		Cardbus_function_disable(csc->sc_ct);
		return (1);
	}
	printf("%s: interrupting at %d\n", sc->sc_dev.dv_xname,
	    csc->sc_intrline);

	return (0);
}

void
tlp_cardbus_disable(sc)
	struct tulip_softc *sc;
{
	struct tulip_cardbus_softc *csc = (void *) sc;
	cardbus_devfunc_t ct = csc->sc_ct;
	cardbus_chipset_tag_t cc = ct->ct_cc;
	cardbus_function_tag_t cf = ct->ct_cf;

	/* Unhook the interrupt handler. */
	cardbus_intr_disestablish(cc, cf, csc->sc_ih);
	csc->sc_ih = NULL;

	/* Power down the socket. */
	Cardbus_function_disable(ct);
}

void
tlp_cardbus_power(sc, why)
	struct tulip_softc *sc;
	int why;
{
	struct tulip_cardbus_softc *csc = (void *) sc;

	if (why == PWR_RESUME) {
		/*
		 * Give the PCI configuration registers a kick
		 * in the head.
		 */
#ifdef DIAGNOSTIC
		if (TULIP_IS_ENABLED(sc) == 0)
			panic("tlp_cardbus_power");
#endif
		tlp_cardbus_setup(csc);
	}
}

void
tlp_cardbus_setup(csc)
	struct tulip_cardbus_softc *csc;
{
	struct tulip_softc *sc = &csc->sc_tulip;
	cardbus_devfunc_t ct = csc->sc_ct;
	cardbus_chipset_tag_t cc = ct->ct_cc;
	cardbus_function_tag_t cf = ct->ct_cf;
	pcireg_t reg;
	int pmreg;

	/*
	 * Check to see if the device is in power-save mode, and
	 * bring it out if necessary.
	 */
	switch (sc->sc_chip) {
	case TULIP_CHIP_21142:
	case TULIP_CHIP_21143:
	case TULIP_CHIP_X3201_3:
		/*
		 * Clear the "sleep mode" bit in the CFDA register.
		 */
		reg = cardbus_conf_read(cc, cf, csc->sc_tag, TULIP_PCI_CFDA);
		if (reg & (CFDA_SLEEP|CFDA_SNOOZE))
			cardbus_conf_write(cc, cf, csc->sc_tag, TULIP_PCI_CFDA,
			    reg & ~(CFDA_SLEEP|CFDA_SNOOZE));
		break;

	default:
		/* Nothing. */
	}

	if (cardbus_get_capability(cc, cf, csc->sc_tag,
	    PCI_CAP_PWRMGMT, &pmreg, 0)) {
		reg = cardbus_conf_read(cc, cf, csc->sc_tag, pmreg + 4) & 0x03;
#if 1 /* XXX Probably not right for CardBus. */
		if (reg == 3) {
			/*
			 * The card has lost all configuration data in
			 * this state, so punt.
			 */
			printf("%s: unable to wake up from power state D3\n",
			    sc->sc_dev.dv_xname);
			return;
		}
#endif
		if (reg != 0) {
			printf("%s: waking up from power state D%d\n",
			    sc->sc_dev.dv_xname, reg);
			cardbus_conf_write(cc, cf, csc->sc_tag,
			    pmreg + 4, 0);
		}
	}

	/* Make sure the right access type is on the CardBus bridge. */
	(*ct->ct_cf->cardbus_ctrl)(cc, csc->sc_cben);
	(*ct->ct_cf->cardbus_ctrl)(cc, CARDBUS_BM_ENABLE);

	/* Program the BAR. */
	cardbus_conf_write(cc, cf, csc->sc_tag, csc->sc_bar_reg,
	    csc->sc_bar_val);

	/* Enable the appropriate bits in the PCI CSR. */
	reg = cardbus_conf_read(cc, cf, csc->sc_tag, PCI_COMMAND_STATUS_REG);
	reg &= ~(PCI_COMMAND_IO_ENABLE|PCI_COMMAND_MEM_ENABLE);
	reg |= csc->sc_csr;
	cardbus_conf_write(cc, cf, csc->sc_tag, PCI_COMMAND_STATUS_REG, reg);

	/*
	 * Make sure the latency timer is set to some reasonable
	 * value.
	 */
	reg = cardbus_conf_read(cc, cf, csc->sc_tag, PCI_BHLC_REG);
	if (PCI_LATTIMER(reg) < 0x20) {
		reg &= ~(PCI_LATTIMER_MASK << PCI_LATTIMER_SHIFT);
		reg |= (0x20 << PCI_LATTIMER_SHIFT);
		cardbus_conf_write(cc, cf, csc->sc_tag, PCI_BHLC_REG, reg);
	}
}

void
tlp_cardbus_x3201_reset(sc)
	struct tulip_softc *sc;
{
	u_int32_t reg;

	reg = TULIP_READ(sc, CSR_SIAGEN);

	/* make GP[2,0] outputs */
	TULIP_WRITE(sc, CSR_SIAGEN, (reg & ~SIAGEN_MD) | SIAGEN_CWE |
	    0x00050000);
	TULIP_WRITE(sc, CSR_SIAGEN, (reg & ~SIAGEN_CWE) | SIAGEN_MD);
}
