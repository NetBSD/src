/*	$NetBSD: if_ne_pbus.c,v 1.16.2.1 2012/05/23 10:07:38 yamt Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_ne_pbus.c,v 1.16.2.1 2012/05/23 10:07:38 yamt Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/socket.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/bus.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <machine/intr.h>
#include <machine/io.h>
#include <dev/ic/dp8390reg.h>
#include <dev/ic/dp8390var.h>
#include <dev/ic/ne2000reg.h>
#include <dev/ic/ne2000var.h>
#include <dev/ic/dp83905reg.h>
#include <dev/ic/dp83905var.h>
#include <dev/ic/mx98905var.h>

#include <arch/acorn32/podulebus/podulebus.h>
#include <arch/acorn32/podulebus/if_ne_pbusreg.h>

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
static int  ne_pbus_probe	(device_t, cfdata_t , void *);
static void ne_pbus_attach	(device_t, device_t, void *);

CFATTACH_DECL_NEW(ne_pbus, sizeof(struct ne_pbus_softc),
    ne_pbus_probe, ne_pbus_attach, NULL, NULL);

/*
 * Prototypes for interface specific routines
 */
static uint8_t *em_ea		(struct ne_pbus_softc *sc, uint8_t *buffer);
static void em_postattach	(struct ne_pbus_softc *sc);
static void eh600_postattach	(struct ne_pbus_softc *sc);
static void eh600_preattach	(struct ne_pbus_softc *sc);
static uint8_t *eh600_ea	(struct ne_pbus_softc *sc, uint8_t *buffer);

void	eh600_init_media        (struct dp8390_softc *);

void	en_postattach		(struct ne_pbus_softc *);
void	en_init_media           (struct dp8390_softc *);

/*
 * Define a structure to hold all the information required on an NE2000
 * clone interface.
 * We create an array of these structures to describe all the interfaces
 * that we can handle via the MI NE2000 driver.
 */
struct ne_clone {
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
	uint8_t *	(*getea)	/* do this to get the MAC */
			    (struct ne_pbus_softc *sc, uint8_t *buffer);
	void		(*preattach)	/* do this before attach */
			    (struct ne_pbus_softc *sc);
	void		(*postattach)	/* do this after attach */
			    (struct ne_pbus_softc *sc);
        int          	(*mediachange)  /* media change */
                            (struct dp8390_softc *);
        void          	(*mediastatus)  /* media status */
                            (struct dp8390_softc *, struct ifmediareq *);
        void          	(*init_card)    /* media init card */
                            (struct dp8390_softc *);
        void          	(*init_media)   /* media init */
                            (struct dp8390_softc *);
} ne_clones[] = {
	/* ANT EtherM netslot interface */
	{
	  PODULE_ETHERM, EM_REGSHIFT,
	  EM_NIC_OFFSET, EM_NIC_SIZE, EM_ASIC_OFFSET, EM_ASIC_SIZE,
	  0,0, NE_SPACE_FAST,
	  NE_SPACE_FAST, NE_SPACE_FAST, 0,
	  "EtherM", em_ea, NULL, em_postattach,
	  NULL,NULL,NULL,NULL
	},
	/* ICubed EtherLan EtherH netslot interface */
	{
	  PODULE_ETHERLAN600, EH600_REGSHIFT,
	  EH600_NIC_OFFSET, EH600_NIC_SIZE, EH600_ASIC_OFFSET, EH600_ASIC_SIZE,
	  EH600_CONTROL_OFFSET, EH600_CONTROL_SIZE, NE_SPACE_FAST,
	  NE_SPACE_FAST, NE_SPACE_FAST, 0,
	  "EtherLan 600", eh600_ea, eh600_preattach, eh600_postattach,
	  dp83905_mediachange, dp83905_mediastatus, dp83905_init_card,
	  eh600_init_media
	},
	/* Acorn EtherLan EtherH netslot interface */
	{
	  PODULE_ETHERLAN600AEH, EH600_REGSHIFT,
	  EH600_NIC_OFFSET, EH600_NIC_SIZE, EH600_ASIC_OFFSET, EH600_ASIC_SIZE,
	  EH600_CONTROL_OFFSET, EH600_CONTROL_SIZE, NE_SPACE_FAST,
	  NE_SPACE_FAST, NE_SPACE_FAST, 0,
	  "EtherLan 600A", eh600_ea , eh600_preattach, eh600_postattach,
	  dp83905_mediachange, dp83905_mediastatus, dp83905_init_card,
	  eh600_init_media
	},
	/* Irlam EtherN podule. (supplied with NC) */
	{
	  PODULE_ETHERN, EN_REGSHIFT,
	  EN_NIC_OFFSET, EN_NIC_SIZE, EN_ASIC_OFFSET, EN_ASIC_SIZE,
	  0,0, NE_SPACE_EASI,
	  NE_SPACE_EASI, NE_SPACE_EASI, 0,
	  "EtherN", em_ea, NULL, en_postattach,
	  dp83905_mediachange, dp83905_mediastatus, dp83905_init_card,
	  en_init_media
	},
	/* Acorn EtherI podule. (supplied with NC) */
	{
	  PODULE_ETHERI, EN_REGSHIFT,
	  EN_NIC_OFFSET, EN_NIC_SIZE, EN_ASIC_OFFSET, EN_ASIC_SIZE,
	  0,0, NE_SPACE_EASI,
	  NE_SPACE_EASI, NE_SPACE_EASI, 0,
	  "EtherI", em_ea, NULL, en_postattach,
	  dp83905_mediachange, dp83905_mediastatus, dp83905_init_card,
	  en_init_media
	},
};

/*
 * Determine if the device is present.
 */
static int
ne_pbus_probe(device_t parent, cfdata_t cf, void *aux)
{
	struct podule_attach_args *pa = (void *) aux;
	int loop;

	/* Scan the list of known interfaces looking for a match */
	for (loop = 0; loop < sizeof(ne_clones) / sizeof(struct ne_clone);
	    ++loop) {
		if (pa->pa_product == ne_clones[loop].product)
			return(1);
	}
	return(0);
}

/*
 * Install interface into kernel networking data structures.
 */
static void
ne_pbus_attach(device_t parent, device_t self, void *aux)
{
	struct podule_attach_args *pa = (void *)aux;
	struct ne_pbus_softc *npsc = device_private(self);
	struct ne2000_softc *nsc = &npsc->sc_ne2000;
	struct dp8390_softc *dsc = &nsc->sc_dp8390;

 	int *media, nmedia, defmedia;
	struct ne_clone *ne = NULL;
	uint8_t buffer[6];
	uint8_t *myea;
	int loop;

	dsc->sc_dev = self;
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
		if (pa->pa_product == ne_clones[loop].product) {
			ne = &ne_clones[loop];
			break;
		}
	}

#ifdef	DIAGNOSTIC
	/* This should never fail as we must have matched at probe time */
	if (ne == NULL)
		panic("Podule has vanished");
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
	aprint_normal(": %s ethernet\n", ne->name);

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
		aprint_error_dev(self, "cannot map i/o space\n");
		return;
	}
	/* Map the I/O space for the ASIC */
	if (bus_space_map(nsc->sc_asict, ne->asicbase, ne->asicsize,
	    0, &nsc->sc_asich)) {
		aprint_error_dev(self, "cannot map i/o space\n");
		return;
	}
	/* Map any extra register space required by the card */
	if (ne->extrasize > 0) {
		if (bus_space_map(&npsc->sc_tag, ne->extrabase, ne->extrasize,
				  0, &npsc->sc_extrah)) {
			aprint_error_dev(self, "cannot map extra space\n");
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
	aprint_normal_dev(self, "");
	switch (nsc->sc_type) {
	case NE2000_TYPE_NE1000:
		aprint_normal("NE1000");
		break;
	case NE2000_TYPE_NE2000:
		aprint_normal("NE2000");
		break;
	case NE2000_TYPE_AX88190:
		aprint_normal("AX88190");
		break;
	case NE2000_TYPE_DL10019:
		aprint_normal("DL10019");
		break;
        case NE2000_TYPE_DL10022:
		aprint_normal("DL10022");
		break;
	default:
		printf("??");
	};
	aprint_normal(" chipset, %d Kb memory\n", dsc->mem_start/1024);

	/* Does the interface need a postattach call ? */
	if (ne->postattach)
		ne->postattach(npsc);

	/* Install an interrupt handler */
	evcnt_attach_dynamic(&npsc->sc_intrcnt, EVCNT_TYPE_INTR, NULL,
	    device_xname(self), "intr");
	npsc->sc_ih = podulebus_irq_establish(pa->pa_ih, IPL_NET, dp8390_intr,
	    dsc, &npsc->sc_intrcnt);
	if (npsc->sc_ih == NULL)
		panic("%s: Cannot install interrupt handler",
		   device_xname(self));
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
static uint8_t *
em_ea(struct ne_pbus_softc *sc, uint8_t *buffer)
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
em_postattach(struct ne_pbus_softc *sc)
{
	int dsr;

	/*
	 * Report information from the Diagnostic Status Register for
	 * the EtherM card
	 */
	aprint_normal_dev(sc->sc_ne2000.sc_dp8390.sc_dev,
	    "16KB buffer memory");

	/* Get the Diagnostic Status Register */
	dsr = bus_space_read_1(sc->sc_ne2000.sc_asict,
	    sc->sc_ne2000.sc_asich, EM_DSR_REG);

	/* Check for bits that indicate a fault */
	if (!(dsr & EM_DSR_20M))
		aprint_normal(", VCO faulty");
	if (!(dsr & EM_DSR_TCOK))
		aprint_normal(", TxClk faulty");

	/* Report status of card */
	if (dsr & EM_DSR_POL)
		aprint_normal(", UTP reverse polarity");
	if (dsr & EM_DSR_JAB)
		aprint_normal(", jabber");
	if (dsr & EM_DSR_LNK)
		aprint_normal(", link OK");
	if (dsr & EM_DSR_LBK)
		aprint_normal(", loopback");
	if (dsr & EM_DSR_UTP)
		aprint_normal(", UTP");
	aprint_normal("\n");
}


/*
 * eh600_preattach()
 *
 * pre-initialise the AT/Lantic chipset so that the card probes and 
 * detects properly. 
 */
static void
eh600_preattach(struct ne_pbus_softc *sc)
{
	u_char tmp;
	struct ne2000_softc *nsc = &sc->sc_ne2000;
	struct dp8390_softc *dsc = &nsc->sc_dp8390;
	bus_space_tag_t nict = dsc->sc_regt;
	bus_space_handle_t nich = dsc->sc_regh;
	
	/* initialise EH600 config register */
	bus_space_read_1(nict, nich, DP83905_MCRA);
	bus_space_write_1(nict, nich, DP83905_MCRA, DP83905_MCRA_INT3);

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
eh600_postattach(struct ne_pbus_softc *sc)
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
	aprint_normal_dev(dsc->sc_dev, "32KB buffer memory\n");
}
/*
 * EtherLan 600 media.
 */
void eh600_init_media(struct dp8390_softc *sc)
{
	static int eh600_media[] = {
		IFM_ETHER|IFM_AUTO,
		IFM_ETHER|IFM_10_T,
		IFM_ETHER|IFM_10_2,
	};
	int i, defmedia = IFM_ETHER|IFM_AUTO;
	static const int eh600_nmedia =
	    sizeof(eh600_media) / sizeof(eh600_media[0]);

	aprint_normal_dev(sc->sc_dev,
	    "10base2, 10baseT, auto, default auto\n");

	ifmedia_init(&sc->sc_media, 0, dp8390_mediachange, dp8390_mediastatus);
	for (i = 0; i < eh600_nmedia; i++)
		ifmedia_add(&sc->sc_media, eh600_media[i], 0, NULL);
	ifmedia_set(&sc->sc_media, defmedia);

}


void
en_postattach(struct ne_pbus_softc *sc)
{

	mx98905_attach(&sc->sc_ne2000.sc_dp8390);
}

/*
 * EtherN media.
 */
void
en_init_media(struct dp8390_softc *sc)
{
	static int en_media[] = {
		IFM_ETHER|IFM_10_T
	};
	aprint_normal_dev(sc->sc_dev, "10baseT, default 10baseT\n");

	ifmedia_init(&sc->sc_media, 0, dp8390_mediachange, dp8390_mediastatus);
	ifmedia_add(&sc->sc_media, en_media[0], 0, NULL);
	ifmedia_set(&sc->sc_media, en_media[0]);
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

static uint8_t *
eh600_ea(struct ne_pbus_softc *sc, uint8_t *buffer)
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
				uint8_t tmp;
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
