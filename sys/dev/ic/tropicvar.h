/*	$NetBSD: tropicvar.h,v 1.3 1999/04/29 15:47:02 bad Exp $	*/

/* 
 * Mach Operating System
 * Copyright (c) 1991 Carnegie Mellon University
 * Copyright (c) 1991 IBM Corporation 
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation,
 * and that the name IBM not be used in advertising or publicity 
 * pertaining to distribution of the software without specific, written
 * prior permission.
 * 
 * CARNEGIE MELLON AND IBM ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON AND IBM DISCLAIM ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */

/*
 * HISTORY
 * $Log: tropicvar.h,v $
 * Revision 1.3  1999/04/29 15:47:02  bad
 * From Onno van der Linden:
 * Reorganise the driver some what.
 * Rename tr_reset() to the more appropriate tr_stop().
 * Create a common tropic reset routine and use it in the frontends.
 * Move the code in tr_config() which is only used in the card attachment
 * routines into a new tr_attach() function.
 * Take adapter off the ring through tr_shutdown() in a shutdown hook.
 * This simplifies the bus-specific frontend.
 *
 * Revision 1.2  1999/03/22 23:01:36  bad
 * Oops. RcsID police.
 *
 * Revision 1.1  1999/03/22 22:21:26  bad
 * Chipset driver for TROPIC based Token-Ring cards.
 * Frontends for IBM and 3COM ISA cards.
 *
 * By Onno van der Linden <onno@simplex.nl>.
 *
 * Revision 2.2  93/02/04  08:00:33  danner
 * 	Integrate PS2 code from IBM.
 * 	[93/01/18            prithvi]
 * 
 */

/* $Header: /cvsroot/src/sys/dev/ic/tropicvar.h,v 1.3 1999/04/29 15:47:02 bad Exp $ */
/* $ACIS:if_lanvar.h 12.0$ */

#if !defined(lint) && !defined(LOCORE)  && defined(RCS_HDRS)
static char    *rcsidif_lanvar = "$Header: /cvsroot/src/sys/dev/ic/tropicvar.h,v 1.3 1999/04/29 15:47:02 bad Exp $";
#endif

/*
 * This file contains structures used in the "tr" driver for the
 *	IBM TOKEN-RING NETWORK PC ADAPTER
 */

/* Receive buffer control block */
struct rbcb {
	bus_size_t	rbufp;		/* offset of current receive buffer */
	bus_size_t	rbufp_next;	/* offset of next receive buffer */
	bus_size_t	rbuf_datap;	/* offset of data in receive buffer */
	unsigned short  data_len;	/* amount of data in this rec buffer */
};

/*
 *	Token-Ring software status per adapter
 */
struct	tr_softc {
	struct device sc_dev;
	void 	*sc_ih;
	struct ethercom sc_ethercom;
	struct ifmedia	sc_media;
	u_char	sc_xmit_correlator;
	int	sc_xmit_buffers;
#if 1
	int	sc_xmit_head;
	int	sc_xmit_tail;
#endif
	int	sc_minbuf;
	int	sc_nbuf;
	bus_size_t sc_txca;

	bus_space_tag_t sc_piot;
	bus_space_tag_t sc_memt;
	bus_space_handle_t sc_pioh;	/* handle pio area */
	bus_space_handle_t sc_sramh;	/* handle for the shared ram area */
	bus_space_handle_t sc_mmioh;	/* handle for the bios/mmio area */

	int (*sc_mediachange) __P((struct tr_softc *));
	void (*sc_mediastatus) __P((struct tr_softc *, struct ifmediareq *));
	struct rbcb rbc;	/* receiver buffer control block */
	bus_size_t sc_aca;	/* offset of adapter ACA */
	bus_size_t sc_ssb;	/* offset of System Status Block */
	bus_size_t sc_arb;	/* offset of Adapter Request Block */
	bus_size_t sc_srb;	/* offset of System Request Block */
	bus_size_t sc_asb;	/* offset of Adapter Status Block */
	u_int sc_maddr;		/* mapped shared memory address */
	u_int sc_memwinsz;	/* mapped shared memory window size */
	u_int sc_memsize;	/* memory installed on adapter */
	u_int sc_memreserved;	/* reserved memory on adapter */
	int sc_dhb4maxsz;	/* max. dhb size at 4MBIT ring speed */
	int sc_dhb16maxsz;	/* max. dbh size at 16MBIT ring speed */
	int sc_maxmtu;		/* max. MTU supported by adapter */
	unsigned char	sc_init_status;
	caddr_t  tr_sleepevent;     	/* tr event signalled on successful */
					/* open of adapter  */
	unsigned short exsap_station;	/* station assigned by open sap cmd */
};

int tr_config __P((struct tr_softc *));
int tr_attach __P((struct tr_softc *));
int tr_intr __P((void *));
void tr_init __P((void *));
int tr_ioctl __P((struct ifnet *, u_long, caddr_t));
void tr_stop __P((struct tr_softc *));
int tr_reset __P((struct tr_softc *));
void tr_sleep __P((struct tr_softc *));
int tr_setspeed __P((struct tr_softc *, u_int8_t));
