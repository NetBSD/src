/*	$NetBSD: if_levar.h,v 1.6 1998/07/21 17:30:27 drochner Exp $	*/

/*
 * LANCE Ethernet driver header file
 *
 * Copyright (c) 1994, 1995 Charles M. Hannum.  All rights reserved.
 *
 * Copyright (C) 1993, Paul Richards. This software may be used, modified,
 *   copied, distributed, and sold, in both source and binary form provided
 *   that the above copyright and these terms are retained. Under no
 *   circumstances is the author responsible for the proper functioning
 *   of this software, nor does the author assume any responsibility
 *   for damages incurred with its use.
 */

#define	PCNET_PCI_RDP	0x10
#define	PCNET_PCI_RAP	0x12
#define	PCNET_PCI_BDP	0x16

/*
 * Ethernet software status per interface.
 *
 * Each interface is referenced by a network interface structure,
 * ethercom.ec_if, which the routing code uses to locate the interface.
 * This structure contains the output queue for the interface, its address, ...
 */
struct le_softc {
	struct	am79900_softc sc_am79900;	/* glue to MI code */

	void	*sc_ih;
	bus_space_tag_t sc_iot;		/* space cookie */
	bus_space_handle_t sc_ioh;	/* bus space handle */
	bus_dma_tag_t	sc_dmat;	/* bus dma tag */
	bus_dmamap_t	sc_dmam;	/* bus dma map */
	int	sc_rap, sc_rdp;		/* offsets to LANCE registers */
};
