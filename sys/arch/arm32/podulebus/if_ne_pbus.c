/*	$NetBSD: if_ne_pbus.c,v 1.10 2001/09/19 22:40:17 reinoud Exp $	*/

/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Mark Brinicombe of Causality Limited.
 *
 * EtherH code Copyright (c) 1998 Mike Pumford
 * EtherN/EtherI code Copyright (c) 1999 Mike Pumford
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
 * This driver uses the generic ne2000 & dp8390 IC drivers
 *
 * Currently supports:
 *	ANT EtherM network slot cards
 *	ICubed Etherlan 600 (EtherH) network slot cards
 *      Irlam EtherN podules
 *      Acorn EtherI podules (identical hardware to EtherN)
 *
 * Thanks go to Stephen Borrill for providing the EtherN card
 * and information to program it.
 *
 * TO DO List for this Driver.
 *
 * EtherM - Needs proper media support.
 */

#include <sys/param.h>
#include <sys/device.h>
#include <sys/socket.h>
#include <sys/systm.h>
#include <sys/mbuf.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <machine/bus.h>
#include <machine/irqhandler.h>
#include <machine/io.h>
#include <dev/ic/dp8390reg.h>
#include <dev/ic/dp8390var.h>
#include <dev/ic/ne2000reg.h>
#include <dev/ic/ne2000var.h>

#include <arch/arm32/podulebus/podulebus.h>
#include <arch/arm32/podulebus/if_ne_pbusreg.h>

#include <dev/podulebus/podules.h>

/*
 * ne_pbus_softc: ne2000_softc plus podule, interrupt and bs tag info
 */
struct ne_pbus_softc {
	struct	ne2000_softc	sc_ne2000;		/* ne2000 softc */
	int			sc_podule_number;
	podule_t		*sc_podule;
	struct bus_space	sc_tag;			/* Patched tag */
	irqhandler_t		*sc_ih;			/* Interrupt handler */
	struct evcnt		sc_intrcnt;		/* Interrupt count */
	bus_space_handle_t	sc_extrah;		/* Bus handle for any
							   extra registers */
};

/*
 * Attach data and prototypes for driver
 */
static int  ne_pbus_probe	__P((struct device *, struct cfdata *, void *));
static void ne_pbus_attach	__P((struct device *, struct device *, void *));

struct cfattach ne_pbus_ca = {
	sizeof(struct ne_pbus_softc), ne_pbus_probe, ne_pbus_attach
};

/*
 * Prototypes for interface specific routines
 */
static u_int8_t *em_ea		__P((struct ne_pbus_softc *sc, u_int8_t *buffer));
static void em_postattach	__P((struct ne_pbus_softc *sc));
static void eh600_postattach	__P((struct ne_pbus_softc *sc));
static void eh600_preattach	__P((struct ne_pbus_softc *sc));
static u_int8_t *eh600_ea	__P((struct ne_pbus_softc *sc, u_int8_t *buffer));

int	eh600_mediachange       __P((struct dp8390_softc *));
void	eh600_mediastatus       __P((struct dp8390_softc *, struct ifmediareq *));
void	eh600_init_card         __P((struct dp8390_softc *));
void	eh600_init_media        __P((struct dp8390_softc *));

int	en_mediachange          __P((struct dp8390_softc *));
void	en_mediastatus          __P((struct dp8390_softc *, struct ifmediareq *));
void	en_init_card            __P((struct dp8390_softc *));
void	en_init_media           __P((struct dp8390_softc *));

/*
 * Define a structure to hold all the information required on an NE2000
 * clone interface.
 * We create an array of these structures to describe all the interfaces
 * that we can handle via the MI NE2000 driver.
 */
struct ne_clone {
	int		manufacturer;	/* podule manufacturer id */
	int		product;	/* podule product id */
	unsigned int	cookie;		/* podulebus space cookie */
	unsigned int	nicbase;	/* byte offset of NIC */
	unsigned int	nicsize;	/* size of NIC (regs) */
	unsigned int	asicbase;	/* byte offset of ASIC */
	unsigned int	asicsize;	/* size of ASIC (regs) */
	unsigned int    extrabase;      /* extra registers byte offset */
	unsigned int    extrasize;      /* size of extra registers(regs) */
	unsigned char	nicspace;	/* easi,fast or mod space ? */
	unsigned char	asicspace;	/* easi,fast or mod space ? */
	unsigned char   extraspace;     /* easi,fast or mod space ? */
#define NE_SPACE_FAST		0
#define NE_SPACE_MOD		1
#define NE_SPACE_EASI           2
	unsigned char	reserved0;	/* not used (padding) */
	const char	*name;		/* name */
	u_int8_t *	(*getea)	/* do this to get the MAC */
			    __P((struct ne_pbus_softc *sc, u_int8_t *buffer));
	void		(*preattach)	/* do this before attach */
			    __P((struct ne_pbus_softc *sc));
	void		(*postattach)	/* do this after attach */
			    __P((struct ne_pbus_softc *sc));
        int          	(*mediachange)  /* media change */
                            __P((struct dp8390_softc *));
        void          	(*mediastatus)  /* media status */
                            __P((struct dp8390_softc *, struct ifmediareq *));
        void          	(*init_card)    /* media init card */
                            __P((struct dp8390_softc *));
        void          	(*init_media)   /* media init */
                            __P((struct dp8390_softc *));
} ne_clones[] = {
	/* ANT EtherM netslot interface */
	{
	  MANUFACTURER_ANT, PODULE_ANT_ETHERM, EM_REGSHIFT,
	  EM_NIC_OFFSET, EM_NIC_SIZE, EM_ASIC_OFFSET, EM_ASIC_SIZE,
	  0,0, NE_SPACE_FAST,
	  NE_SPACE_FAST, NE_SPACE_FAST, 0,
	  "EtherM", em_ea, NULL, em_postattach,
	  NULL,NULL,NULL,NULL
	},
	/* ICubed EtherLan EtherH netslot interface */
	{
	  MANUFACTURER_ICUBED, PODULE_ICUBED_ETHERLAN600, EH600_REGSHIFT,
	  EH600_NIC_OFFSET, EH600_NIC_SIZE, EH600_ASIC_OFFSET, EH600_ASIC_SIZE,
	  EH600_CONTROL_OFFSET, EH600_CONTROL_SIZE, NE_SPACE_FAST,
	  NE_SPACE_FAST, NE_SPACE_FAST, 0,
	  "EtherLan 600", eh600_ea, eh600_preattach, eh600_postattach,
	  eh600_mediachange, eh600_mediastatus, eh600_init_card,
	  eh600_init_media
	},
	/* Acorn EtherLan EtherH netslot interface */
	{
	  MANUFACTURER_ICUBED, PODULE_ICUBED_ETHERLAN600AEH, EH600_REGSHIFT,
	  EH600_NIC_OFFSET, EH600_NIC_SIZE, EH600_ASIC_OFFSET, EH600_ASIC_SIZE,
	  EH600_CONTROL_OFFSET, EH600_CONTROL_SIZE, NE_SPACE_FAST,
	  NE_SPACE_FAST, NE_SPACE_FAST, 0,
	  "EtherLan 600A", eh600_ea , eh600_preattach, eh600_postattach,
	  eh600_mediachange, eh600_mediastatus, eh600_init_card,
	  eh600_init_media
	},
	/* Irlam EtherN podule. (supplied with NC) */
	{
	  MANUFACTURER_IRLAM ,PODULE_IRLAM_ETHERN ,EN_REGSHIFT,
	  EN_NIC_OFFSET, EN_NIC_SIZE, EN_ASIC_OFFSET, EN_ASIC_SIZE,
	  0,0, NE_SPACE_EASI,
	  NE_SPACE_EASI, NE_SPACE_EASI, 0,
	  "EtherN", em_ea, NULL ,NULL,
	  en_mediachange, en_mediastatus, en_init_card,
	  en_init_media
	},
	/* Acorn EtherI podule. (supplied with NC) */
	{
	  MANUFACTURER_ACORN ,PODULE_ACORN_ETHERI ,EN_REGSHIFT,
	  EN_NIC_OFFSET, EN_NIC_SIZE, EN_ASIC_OFFSET, EN_ASIC_SIZE,
	  0,0, NE_SPACE_EASI,
	  NE_SPACE_EASI, NE_SPACE_EASI, 0,
	  "EtherI", em_ea, NULL ,NULL,
	  en_mediachange, en_mediastatus, en_init_card,
	  en_init_media
	},
};

/*
 * Determine if the device is present.
 */
static int
ne_pbus_probe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct podule_attach_args *pa = (void *) aux;
	int loop;

	/* Scan the list of known interfaces looking for a match */
	for (loop = 0; loop < sizeof(ne_clones) / sizeof(struct ne_clone);
	    ++loop) {
		if (matchpodule(pa, ne_clones[loop].manufacturer,
		    ne_clones[loop].product, 0) != 0)
			return(1);
	}
	return(0);
}

/*
 * Install interface into kernel networking data structures.
 */
static void
ne_pbus_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct podule_attach_args *pa = (void *)aux;
	struct ne_pbus_softc *npsc = (void *)self;
	struct ne2000_softc *nsc = &npsc->sc_ne2000;
	struct dp8390_softc *dsc = &nsc->sc_dp8390;

 	int *media, nmedia, defmedia;
	struct ne_clone *ne = NULL;
	u_int8_t buffer[6];
	u_int8_t *myea;
	int loop;

	media = NULL;
	nmedia = defmedia = 0;
	/* Check a few things about the attach args */

	if (pa->pa_podule_number == -1)
		panic("Podule has disappeared !");

	npsc->sc_podule_number = pa->pa_podule_number;
	npsc->sc_podule = pa->pa_podule;
	podules[npsc->sc_podule_number].attached = 1;		/* XXX */

	/* Scan the list of known interfaces for a match */
	for (loop = 0; loop < sizeof(ne_clones) / sizeof(struct ne_clone);
	    ++loop) {
		if (IS_PODULE(pa, ne_clones[loop].manufacturer,
		    ne_clones[loop].product)) {
			ne = &ne_clones[loop];
			break;
		}
	}

#ifdef	DIAGNOSTIC
	/* This should never fail as we must have matched at probe time */
	if (ne == NULL)
		panic("Podule has vanished\n");
#endif

	/* Update the nic and asic base addresses appropriately */
	switch (ne->nicspace) {
	case NE_SPACE_EASI:
		ne->nicbase += npsc->sc_podule->easi_base;
		break;
	case NE_SPACE_MOD:
		ne->nicbase += npsc->sc_podule->mod_base;
		break;
	case NE_SPACE_FAST:
	default:
		ne->nicbase += npsc->sc_podule->fast_base;
		break;
	}
	switch (ne->asicspace) {
	case NE_SPACE_EASI:
		ne->asicbase += npsc->sc_podule->easi_base;
		break;
	case NE_SPACE_MOD:
		ne->asicbase += npsc->sc_podule->mod_base;
		break;
	case NE_SPACE_FAST:
	default:
		ne->asicbase += npsc->sc_podule->fast_base;
		break;
	}

	switch (ne->extraspace) {
	case NE_SPACE_EASI:
		ne->extrabase += npsc->sc_podule->easi_base;
		break;
	case NE_SPACE_MOD:
		ne->extrabase += npsc->sc_podule->mod_base;
		break;
	case NE_SPACE_FAST:
	default:
		ne->extrabase += npsc->sc_podule->fast_base;
		break;
	}

	/* Report the interface name */
	printf(": %s ethernet\n", ne->name);

	/*
	 * Ok we need our own bus tag as the register spacing
	 * may not the default.
	 *
	 * For the podulebus, the bus tag cookie is the shift
	 * to apply to registers
	 * So duplicate the bus space tag and change the
	 * cookie.
	 */

	npsc->sc_tag = *pa->pa_iot;
	npsc->sc_tag.bs_cookie = (void *) ne->cookie;

	dsc->sc_regt = &npsc->sc_tag;
	nsc->sc_asict = dsc->sc_regt;

	/* Map all the I/O space for the NIC */
	if (bus_space_map(dsc->sc_regt, ne->nicbase, ne->nicsize,
	    0, &dsc->sc_regh)) {
		printf("%s: cannot map i/o space\n", dsc->sc_dev.dv_xname);
		return;
	}
	/* Map the I/O space for the ASIC */
	if (bus_space_map(nsc->sc_asict, ne->asicbase, ne->asicsize,
	    0, &nsc->sc_asich)) {
		printf("%s: cannot map i/o space\n", dsc->sc_dev.dv_xname);
		return;
	}
	/* Map any extra register space required by the card */
	if (ne->extrasize > 0) {
		if (bus_space_map(&npsc->sc_tag, ne->extrabase, ne->extrasize,
				  0, &npsc->sc_extrah)) {
			printf("%s: cannot map extra space\n",
			       dsc->sc_dev.dv_xname);
			return;
		}
	}

	/* This interface is always enabled. */
	dsc->sc_enabled = 1;

	/*
	 * Now get the ethernet address in an interface specific manner if
	 * specified
	 */
	if (ne->getea)
		myea = ne->getea(npsc, buffer);
	else
		myea = NULL;

	/* Does the interface need a preattach call ? */
	if (ne->preattach)
		ne->preattach(npsc);

	/* if the interface has media support initialise it */
	if (ne->init_media) {
		dsc->sc_mediachange = ne->mediachange;
		dsc->sc_mediastatus = ne->mediastatus;
		dsc->init_card = ne->init_card;
		dsc->sc_media_init = ne->init_media;
/*		ne->init_media(dsc,&media,&nmedia,&defmedia); */
	}

	/*
	 * Do generic NE2000 attach.  This will read the station address
	 * from the EEPROM.
	 */
	ne2000_attach(nsc, myea);
	printf("%s: ", dsc->sc_dev.dv_xname);
	switch (nsc->sc_type) {
	case NE2000_TYPE_NE1000:
		printf("NE1000");
		break;
	case NE2000_TYPE_NE2000:
		printf("NE2000");
		break;
	case NE2000_TYPE_AX88190:
		printf("AX88190");
		break;
	case NE2000_TYPE_DL10019:
		printf("DL10019");
		break;
        case NE2000_TYPE_DL10022:
		printf("DL10022");
		break;
	default:
		printf("??");
	};
	printf(" chipset, %d Kb memory\n", dsc->mem_start/1024);

	/* Does the interface need a postattach call ? */
	if (ne->postattach)
		ne->postattach(npsc);

	/* Install an interrupt handler */
	evcnt_attach_dynamic(&npsc->sc_intrcnt, EVCNT_TYPE_INTR, NULL,
	    self->dv_xname, "intr");
	npsc->sc_ih = podulebus_irq_establish(pa->pa_ih, IPL_NET, dp8390_intr,
	    dsc, &npsc->sc_intrcnt);
	if (npsc->sc_ih == NULL)
		panic("%s: Cannot install interrupt handler",
		   dsc->sc_dev.dv_xname);
	/* this feels wrong to do this here */
	npsc->sc_ih->ih_maskaddr = npsc->sc_podule->irq_addr;
	npsc->sc_ih->ih_maskbits = npsc->sc_podule->irq_mask;
}

/*
 * em_ea()
 *
 * return the ethernet address for an EtherM netslot interface.
 * The EtherM interface uses the machines ethernet address so just
 * fill it out
 */
static u_int8_t *
em_ea(sc, buffer)
	struct ne_pbus_softc *sc;
	u_int8_t *buffer;
{
	/*
	 * Use the podulebus netslot_ea() function to get the netslot
	 * ethernet address. This is generated from the machine ID.
	 */

	netslot_ea(buffer);
	return(buffer);
}

/*
 * em_postattach()
 *
 * The EtherM interface has a Diagnostic Status register. After attaching
 * the driver, print out some more information using this register.
 */
static void
em_postattach(sc)
	struct ne_pbus_softc *sc;
{
	int dsr;

	/*
	 * Report information from the Diagnostic Status Register for
	 * the EtherM card
	 */
	printf("%s: 16KB buffer memory",
	    sc->sc_ne2000.sc_dp8390.sc_dev.dv_xname);

	/* Get the Diagnostic Status Register */
	dsr = bus_space_read_1(sc->sc_ne2000.sc_asict,
	    sc->sc_ne2000.sc_asich, EM_DSR_REG);

	/* Check for bits that indicate a fault */
	if (!(dsr & EM_DSR_20M))
		printf(", VCO faulty");
	if (!(dsr & EM_DSR_TCOK))
		printf(", TxClk faulty");

	/* Report status of card */
	if (dsr & EM_DSR_POL)
		printf(", UTP reverse polarity");
	if (dsr & EM_DSR_JAB)
		printf(", jabber");
	if (dsr & EM_DSR_LNK)
		printf(", link OK");
	if (dsr & EM_DSR_LBK)
		printf(", loopback");
	if (dsr & EM_DSR_UTP)
		printf(", UTP");
	printf("\n");
}


/*
 * eh600_preattach()
 *
 * pre-initialise the AT/Lantic chipset so that the card probes and 
 * detects properly. 
 */
static void
eh600_preattach(sc)
	struct ne_pbus_softc *sc;
{
	u_char tmp;
	struct ne2000_softc *nsc = &sc->sc_ne2000;
	struct dp8390_softc *dsc = &nsc->sc_dp8390;
	bus_space_tag_t nict = dsc->sc_regt;
	bus_space_handle_t nich = dsc->sc_regh;
	
	/* initialise EH600 config register */
	bus_space_read_1(nict, nich, EH600_MCRA);
	bus_space_write_1(nict,nich,EH600_MCRA,0x18);

	/* enable interrupts for the card */
	tmp = bus_space_read_1(&sc->sc_tag,sc->sc_extrah,0); 
	tmp |= EH_INTR_MASK;
	bus_space_write_1(&sc->sc_tag,sc->sc_extrah,0,tmp); 
}

/*
 * eh600_postattach()
 *
 * Etherlan 600 has 32k of buffer memory as it runs the AT/Lantic 
 * DP8390 clone in IO non-compatible mode. We need to adjust the memory 
 * description set up by dp8390.c and ne2000.c to reflect this.
 */
static void
eh600_postattach(sc)
	struct ne_pbus_softc *sc;
{
	struct ne2000_softc *nsc = &sc->sc_ne2000;
	struct dp8390_softc *dsc = &nsc->sc_dp8390;
	/* first page is mapped to the PROM. so start at 2nd page */
	dsc->mem_start = EH600_MEM_START;
	dsc->mem_size = EH600_MEM_END - EH600_MEM_START;
	dsc->mem_end = EH600_MEM_END;
	dsc->txb_cnt = 3; /* >16k of ram setup 3 tx buffers */
	/* recompute the mem ring (taken straight from the ne2000 init code) */
	dsc->mem_ring = 
		dsc->mem_start + 
		(((dsc->txb_cnt + 1) * ED_TXBUF_SIZE ) <<
		 ED_PAGE_SHIFT);

	/* recompute the dp8390 register values. (from dp8390 init code) */
	dsc->tx_page_start = dsc->mem_start >> ED_PAGE_SHIFT;

	dsc->rec_page_start = dsc->tx_page_start + 
		(dsc->txb_cnt + 1) * ED_TXBUF_SIZE;

	dsc->rec_page_stop = dsc->tx_page_start + 
		(dsc->mem_size >> ED_PAGE_SHIFT);
	printf("%s: 32KB buffer memory\n", dsc->sc_dev.dv_xname);

}
/*
 * EtherLan 600 media.
 */
void eh600_init_media(sc)
	struct dp8390_softc *sc;
{
	static int eh600_media[] = {
		IFM_ETHER|IFM_AUTO,
		IFM_ETHER|IFM_10_T,
		IFM_ETHER|IFM_10_2,
	};
	int i, defmedia = IFM_ETHER|IFM_AUTO;
	static const int eh600_nmedia =
	    sizeof(eh600_media) / sizeof(eh600_media[0]);

	printf("%s: 10base2, 10baseT, auto, default auto\n",
	    sc->sc_dev.dv_xname);

	ifmedia_init(&sc->sc_media, 0, dp8390_mediachange, dp8390_mediastatus);
	for (i = 0; i < eh600_nmedia; i++)
		ifmedia_add(&sc->sc_media, eh600_media[i], 0, NULL);
	ifmedia_set(&sc->sc_media, defmedia);

}




void eh600_init_card(sc)
     struct dp8390_softc *sc;
{
	struct ifmedia *ifm = &sc->sc_media;
	bus_space_tag_t nict = sc->sc_regt;
	bus_space_handle_t nich = sc->sc_regh;
	u_int8_t reg;


	/* Set basic media type. */
	switch (IFM_SUBTYPE(ifm->ifm_cur->ifm_media)) {
	case IFM_AUTO:
		/* software auto detect the media */
		reg = bus_space_read_1(nict, nich, EH600_MCRB);
		reg = (reg & 0xf8) | EH600_10BTSEL;
		bus_space_write_1(nict, nich, EH600_MCRB, reg);
		reg = bus_space_read_1(nict, nich, EH600_MCRB);
		if ((reg & 0x04) != 0x04) {
			/* No UTP use BNC */
			reg = (reg & 0xf8) | EH600_10B2SEL;
			bus_space_write_1(nict, nich, EH600_MCRB, reg);
		} 

		break;

	case IFM_10_T:
		reg = bus_space_read_1(nict, nich, EH600_MCRB);
		reg = (reg & 0xf8) | EH600_10BTSEL;
		bus_space_write_1(nict, nich, EH600_MCRB, reg);
		/* seems that re-reading config B here is required to 
	         * prevent the interface hanging when manually selecting.
		 */
		bus_space_read_1(nict, nich, EH600_MCRB);
		break;

	case IFM_10_2:
		reg = bus_space_read_1(nict, nich, EH600_MCRB);
		reg = (reg & 0xf8) | EH600_10B2SEL;
		bus_space_write_1(nict, nich, EH600_MCRB,reg);
		/* seems that re-reading config B here is required to 
	         * prevent the interface hanging when manually selecting.
		 */
		bus_space_read_1(nict, nich, EH600_MCRB);
		break;
	}
}

int
eh600_mediachange(dsc)
	struct dp8390_softc *dsc;
{
	/* media is already set up. Interface reset will invoke new
	 * new media setting. */
	dp8390_reset(dsc);
	return (0);
}


void
eh600_mediastatus(sc, ifmr)
	struct dp8390_softc *sc;
	struct ifmediareq *ifmr;
{
	bus_space_tag_t nict = sc->sc_regt;
	bus_space_handle_t nich = sc->sc_regh;
	u_int8_t reg;
	reg = bus_space_read_1(nict, nich, EH600_MCRB);
	if ((reg & 0x3) == 1) {
		ifmr->ifm_active = IFM_ETHER|IFM_10_2;
	}
	else {
		ifmr->ifm_active = IFM_ETHER|IFM_10_T;
	}
}


/*
 * EtherN media.
 */
void
en_init_media(sc)
	struct dp8390_softc *sc;
{
	static int en_media[] = {
		IFM_ETHER|IFM_10_T
	};
	printf("%s: 10baseT, default 10baseT\n",
	    sc->sc_dev.dv_xname);

	ifmedia_init(&sc->sc_media, 0, dp8390_mediachange, dp8390_mediastatus);
	ifmedia_add(&sc->sc_media, en_media[0], 0, NULL);
	ifmedia_set(&sc->sc_media, en_media[0]);
}




void
en_init_card(sc)
     struct dp8390_softc *sc;
{
}

int
en_mediachange(dsc)
	struct dp8390_softc *dsc;
{
	/* media is static so any change is invalid. */
	return (EINVAL);
}


void
en_mediastatus(sc, ifmr)
	struct dp8390_softc *sc;
	struct ifmediareq *ifmr;
{
	ifmr->ifm_active = IFM_ETHER|IFM_10_T;
}


/* 
 * extracts the station address from the Podule description string.
 * The description has to be re-read here since the podule description 
 * string is not always long enough to contain the full address.
 *
 * If for any reason we cannot extract the address this routine will 
 * use netslot_ea() to return the generic address for the network slot.
 */

#define POD_READ(addr) \
	podule->read_rom(podule->sync_base, addr)

static u_int8_t *
eh600_ea(sc, buffer)
	struct ne_pbus_softc *sc;
	u_int8_t *buffer;
{
	podule_t *podule = sc->sc_podule;
	u_int address;
	u_int id;

	address = 0x40;
	memset(buffer, 0, 6);

	/* read chunks from the podule  */
	do {
		id = POD_READ(address);
		/* check for description chunk. */
		if (id == 0xf5) {
			u_int size;
			u_int pod_addr;
			int loop;

			/* read the size */
			size = POD_READ(address + 4);
			size |= (POD_READ(address + 8) << 8);
			size |= (POD_READ(address + 12) << 16);

			/* read address of description */
			pod_addr = POD_READ(address + 16);
			pod_addr |= (POD_READ(address + 20) << 8);
			pod_addr |= (POD_READ(address + 24) << 16);
			pod_addr |= (POD_READ(address + 28) << 24);

			if (pod_addr < 0x800) {
				u_int8_t tmp;
				int addr_index = 0;
				int found_ether = 0;

				/*
				 * start scanning for ethernet address
				 * which starts with a '('
				 */
				for (loop = 0; loop < size; ++loop) {
					if (found_ether) {
					        /* we have found a '(' so start decoding the address */
						tmp = POD_READ((pod_addr + loop) * 4);
						if (tmp >= '0' &&  tmp <= '9') {
							buffer[addr_index >> 1] |= (tmp - '0') << ((addr_index & 1) ? 0 : 4);
							++addr_index;
						}
						else if (tmp >= 'a' &&  tmp <= 'f'){
							buffer[addr_index >> 1] |= (10 + (tmp - 'a')) << ((addr_index & 1) ? 0 : 4);
							++addr_index;
						}
						else if (tmp >= 'A' &&  tmp <= 'F'){
							buffer[addr_index >> 1] |= (10 + (tmp - 'A')) << ((addr_index & 1) ? 0 : 4);
							++addr_index;
						}
						else if (tmp == ')') {
							/* we have read the whole address so we can stop scanning 
							 * the podule description */
							break;
						}
					}
					/*
					 * we have found the start of the ethernet address (decode begins 
					 * on the next run round the loop. */
					if (POD_READ((pod_addr + loop) * 4) == '(') {
						found_ether = 1;
					}
				}
				/*
				 * Failed to find the address so fall back
				 * on the netslot address
				 */
				if (!found_ether)
					netslot_ea(buffer);
				return(buffer);
			}
		}
		address += 32;
	} while (id != 0 && address < 0x8000);

	/*
	 * If we get here we failed to find the address
	 * In this case the best solution is to go with the netslot addrness
	 */
	netslot_ea(buffer);
	return(buffer);
}
