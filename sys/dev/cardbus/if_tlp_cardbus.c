/*	$NetBSD: if_tlp_cardbus.c,v 1.5 1999/12/11 00:32:59 thorpej Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
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
 *
 * TODO:
 *
 *	- power management
 *	- add support for the Xircom clone
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
	int	sc_intrline;		/* our interrupt line */
	cardbustag_t sc_tag;

	int	sc_cbenable;		/* what CardBus access type to enable */
	int	sc_csr;			/* CSR bits */
};

int	tlp_cardbus_match __P((struct device *, struct cfdata *, void *));
void	tlp_cardbus_attach __P((struct device *, struct device *, void *));

struct cfattach tlp_cardbus_ca = {
	sizeof(struct tulip_cardbus_softc),
	    tlp_cardbus_match, tlp_cardbus_attach,
};

const struct tulip_cardbus_product {
	u_int32_t	tcp_vendor;	/* PCI vendor ID */
	u_int32_t	tcp_product;	/* PCI product ID */
	tulip_chip_t	tcp_chip;	/* base Tulip chip type */
	int		tcp_pmreg;	/* power management register offset */
} tlp_cardbus_products[] = {
	{ PCI_VENDOR_DEC,		PCI_PRODUCT_DEC_21142,
	  TULIP_CHIP_21142,		0xe0 },

	{ 0,				0,
	  TULIP_CHIP_INVALID,		0 },
};

const char *tlp_cardbus_chip_names[] = TULIP_CHIP_NAMES;

const struct tulip_cardbus_product *tlp_cardbus_lookup
    __P((const struct cardbus_attach_args *));

const struct tulip_cardbus_product *
tlp_cardbus_lookup(ca)
	const struct cardbus_attach_args *ca;
{
	const struct tulip_cardbus_product *tcp;

	for (tcp = tlp_cardbus_products;
	     tlp_cardbus_chip_names[tcp->tcp_chip] != NULL;
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
	struct tulip_cardbus_softc *csc = (void *) self;
	struct tulip_softc *sc = &csc->sc_tulip;
	struct cardbus_attach_args *ca = aux;
	cardbus_devfunc_t ct = ca->ca_ct;
	cardbus_chipset_tag_t cc = ct->ct_cc;
	cardbus_function_tag_t cf = ct->ct_cf;
	const struct tulip_cardbus_product *tcp;
	u_int8_t enaddr[ETHER_ADDR_LEN];
	pcireg_t reg;

	sc->sc_devno = ca->ca_device;
	sc->sc_dmat = ca->ca_dmat;
	csc->sc_ct = ct;
	csc->sc_tag = ca->ca_tag;
	csc->sc_intrline = ca->ca_intrline;

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
	 * Get revision info, and set some chip-specific variables.
	 */
	sc->sc_rev = PCI_REVISION(ca->ca_class);
	switch (sc->sc_chip) {
	case TULIP_CHIP_21142:
		if (sc->sc_rev >= 0x20)
			sc->sc_chip = TULIP_CHIP_21143;
		break;

	default:
		/* Nothing. */
	}

	printf(": %s Ethernet, pass %d.%d\n",
	    tlp_cardbus_chip_names[sc->sc_chip],
	    (sc->sc_rev >> 4) & 0xf, sc->sc_rev & 0xf);

	/*
	 * Check to see if the device is in power-save mode, and
	 * bring it out if necessary.
	 */
	switch (sc->sc_chip) {
	case TULIP_CHIP_21142:
	case TULIP_CHIP_21143:
		/*
		 * Clear the "sleep mode" bit in the CFDA register.
		 */
		reg = cardbus_conf_read(cc, cf, ca->ca_tag, TULIP_PCI_CFDA);
		if (reg & (CFDA_SLEEP|CFDA_SNOOZE))
			cardbus_conf_write(cc, cf, ca->ca_tag, TULIP_PCI_CFDA,
			    reg & ~(CFDA_SLEEP|CFDA_SNOOZE));
		break;

	default:
		/* Nothing. */
	}

	/*
	 * Map the device.
	 */
	csc->sc_csr = PCI_COMMAND_MASTER_ENABLE;
	if (Cardbus_mapreg_map(csc->sc_ct, TULIP_PCI_MMBA,
	    PCI_MAPREG_TYPE_MEM|PCI_MAPREG_MEM_TYPE_32BIT, 0,
	    &sc->sc_st, &sc->sc_sh, NULL, NULL) == 0) {
		csc->sc_cbenable = CARDBUS_MEM_ENABLE;
		csc->sc_csr |= PCI_COMMAND_MEM_ENABLE;
	} else if (Cardbus_mapreg_map(csc->sc_ct, TULIP_PCI_IOBA,
	    PCI_MAPREG_TYPE_IO, 0, &sc->sc_st, &sc->sc_sh, NULL, NULL) == 0) {
		csc->sc_cbenable = CARDBUS_IO_ENABLE;
		csc->sc_csr |= PCI_COMMAND_IO_ENABLE;
	} else {
		printf("%s: unable to map device registers\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	/* Make sure the right access type is on the CardBus bridge. */
	(*ct->ct_cf->cardbus_ctrl)(cc, csc->sc_cbenable);

	/* Enable the appropriate bits in the PCI CSR. */
	reg = cardbus_conf_read(cc, cf, ca->ca_tag, PCI_COMMAND_STATUS_REG);
	reg = (reg & ~(PCI_COMMAND_IO_ENABLE|PCI_COMMAND_MEM_ENABLE)) |
	    csc->sc_csr;
	cardbus_conf_write(cc, cf, ca->ca_tag, PCI_COMMAND_STATUS_REG, reg);

	/*
	 * Read the contents of the Ethernet Address ROM/SROM.  Some
	 * chips have a 128 byte SROM (6 address bits), and some
	 * have a 512 byte SROM (8 address bits).
	 */
	sc->sc_srom_addrbits = 6;
 try_again:
	memset(sc->sc_srom, 0, sizeof(sc->sc_srom));
	tlp_read_srom(sc, 0, TULIP_ROM_SIZE(sc->sc_srom_addrbits) >> 1,
	    sc->sc_srom);
#if 0
	{
		int i;

		printf("SROM CONTENTS:");
		for (i = 0; i < TULIP_ROM_SIZE(sc->sc_srom_addrbits); i++) {
			if ((i % 8) == 0)
				printf("\n\t");
			printf("0x%02x ", sc->sc_srom[i]);
		}
		printf("\n");
	}
#endif

	/*
	 * Deal with chip/board quirks.  This includes setting up
	 * the mediasw, and extracting the Ethernet address from
	 * the rombuf.
	 */
	switch (sc->sc_chip) {
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
		 * Bail out now if we can't deal with this board.
		 */
		if (sc->sc_mediasw == NULL)
			goto cant_cope;
		break;

	default:
 cant_cope:
		/*
		 * Try reading it again, with larger SROM
		 * size, if we haven't already.
		 */
		if (sc->sc_srom_addrbits != 8) {
			sc->sc_srom_addrbits = 8;
			goto try_again;
		}

		printf("%s: sorry, unable to handle your board\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	/*
	 * Map and establish the interrupt.
	 */
	csc->sc_ih = cardbus_intr_establish(cc, cf, ca->ca_intrline, IPL_NET,
	    tlp_intr, sc);
	if (csc->sc_ih == NULL) {
		printf("%s: unable to establish interrupt at %d\n",
		    sc->sc_dev.dv_xname, ca->ca_intrline);
		return;
	}
	printf("%s: interrupting at %d\n", sc->sc_dev.dv_xname,
	    ca->ca_intrline);

	/*
	 * Finish off the attach.
	 */
	tlp_attach(sc, enaddr);
}
