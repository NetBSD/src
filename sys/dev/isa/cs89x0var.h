/*	$NetBSD: cs89x0var.h,v 1.9 2000/09/28 10:37:21 tsutsui Exp $	*/

/*
 * Copyright 1997
 * Digital Equipment Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and
 * copied only in accordance with the following terms and conditions.
 * Subject to these conditions, you may download, copy, install,
 * use, modify and distribute this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce
 *    and retain this copyright notice and list of conditions as
 *    they appear in the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Digital Equipment Corporation. Neither the "Digital Equipment
 *    Corporation" name nor any trademark or logo of Digital Equipment
 *    Corporation may be used to endorse or promote products derived
 *    from this software without the prior written permission of
 *    Digital Equipment Corporation.
 *
 * 3) This software is provided "AS-IS" and any express or implied
 *    warranties, including but not limited to, any implied warranties
 *    of merchantability, fitness for a particular purpose, or
 *    non-infringement are disclaimed. In no event shall DIGITAL be
 *    liable for any damages whatsoever, and in particular, DIGITAL
 *    shall not be liable for special, indirect, consequential, or
 *    incidental damages or damages for lost profits, loss of
 *    revenue or loss of use, whether such damages arise in contract,
 *    negligence, tort, under statute, in equity, at law or otherwise,
 *    even if advised of the possibility of such damage.
 */

/*
**++
**  FACILITY  Crystal CS8900 Ethernet driver header file
**
**  ABSTRACT
**
**     This module provides CS8900 driver softc and related definitions
**
**  AUTHORS
**
**     Peter Dettori   SEA - Software Engineering.
**
**  CREATION DATE:
**
**       13-Feb-1997.
**
**  MODIFICATION HISTORY:
**
**--
*/

#ifndef _DEV_ISA_CS89X0VAR_H_
#define	_DEV_ISA_CS89X0VAR_H_

/*
 * Ethernet software status per interface.
 *
 * Each interface is referenced by a network interface structure,
 * arpcom.ac_if, which the routing code uses to locate the interface.
 * This structure contains the output queue for the interface, 
 * its address, ...
 */
struct cs_softc {
	struct device sc_dev;		/* base device glue */
	struct ethercom sc_ethercom;	/* Ethernet common */
	struct ifmedia sc_media;	/* media control structures */

	void	*sc_ih;			/* interupt handler */
	void 	*sc_sh;			/* shutdown hook */

	bus_space_tag_t sc_iot;		/* bus space tag for IO */
	bus_space_tag_t sc_memt;	/* bus space tag for memory mode */
	bus_space_handle_t sc_ioh;	/* bus space handles */
	bus_space_handle_t sc_memh;

	isa_chipset_tag_t sc_ic;	/* ISA chipset */

	int	sc_irq;			/* IRQ line */
	int	sc_drq;			/* DRQ line */

	int	sc_prodid;		/* saved product ID */
	int	sc_prodrev;		/* saved product rev */

	bus_addr_t sc_pktpgaddr;	/* PacketPage bus memory address */

	bus_size_t sc_dmasize;		/* DMA size (16k or 64k) */
	caddr_t	sc_dmabase;		/* base DMA address (KVA) */
	caddr_t	sc_dmacur;		/* current DMA address (KVA) */

	int	sc_cfgflags;		/* software configuration flags */

	int	sc_memorymode;		/* are we in memory mode? */
	int	sc_txbusy;		/* transmit in progress */
	int	sc_resetting;		/* reset in progress */

	int	sc_xe_ent;		/* current early-xmit table entry */
	int	sc_xe_togo;		/* # of packets to go at this ent */

	int	sc_carrier;		/* has carrier */

	u_int8_t sc_enaddr[ETHER_ADDR_LEN];	/* MAC address */

#if NRND > 0
	rndsource_element_t rnd_source; /* random source */
#endif
};

/*
 * Inlines for reading/writing the packet page area.
 */
static __inline__ u_int16_t CS_READ_PACKET_PAGE_IO __P((bus_space_tag_t,
	bus_space_handle_t, int));

static __inline__ u_int16_t
CS_READ_PACKET_PAGE_IO(iot, ioh, offset)
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	int offset;
{

	bus_space_write_2(iot, ioh, PORT_PKTPG_PTR, offset);
	return (bus_space_read_2(iot, ioh, PORT_PKTPG_DATA));
}

#define	CS_READ_PACKET_PAGE_MEM(memt, memh, offset)			\
	bus_space_read_2((memt), (memh), (offset))

#define	CS_READ_PACKET_PAGE(sc, offset)					\
	((sc)->sc_memorymode ? CS_READ_PACKET_PAGE_MEM((sc)->sc_memt,	\
			       (sc)->sc_memh, (offset)) :		\
	 CS_READ_PACKET_PAGE_IO((sc)->sc_iot, (sc)->sc_ioh, (offset)))

#define	CS_WRITE_PACKET_PAGE_IO(iot, ioh, offset, val)			\
do {									\
	bus_space_write_2((iot), (ioh), PORT_PKTPG_PTR, (offset));	\
	bus_space_write_2((iot), (ioh), PORT_PKTPG_DATA, (val));	\
} while (0)

#define	CS_WRITE_PACKET_PAGE_MEM(memt, memh, offset, val)		\
	bus_space_write_2((memt), (memh), (offset), (val))

#define	CS_WRITE_PACKET_PAGE(sc, offset, val)				\
do {									\
	if ((sc)->sc_memorymode)					\
		CS_WRITE_PACKET_PAGE_MEM((sc)->sc_memt, (sc)->sc_memh,	\
		    (offset), (val));					\
	else								\
		CS_WRITE_PACKET_PAGE_IO((sc)->sc_iot, (sc)->sc_ioh,	\
		    (offset), (val));					\
} while (0)

/* Return Status */
#define	CS_ERROR   -1
#define CS_OK       1
		
/* Config Flags in cs_softc */

#define CFGFLG_MEM_MODE     0x0001
#define CFGFLG_USE_SA       0x0002
#define CFGFLG_IOCHRDY      0x0004
#define CFGFLG_DCDC_POL     0x0008
#define CFGFLG_DMA_MODE	    0x0020
#define CFGFLG_NOT_EEPROM   0x8000


/* Media Type in cs_softc */

#define MEDIA_AUI           0x0001
#define MEDIA_10BASE2       0x0002
#define MEDIA_10BASET       0x0003


/* Miscellaneous definitions */

#define MAXLOOP            0x8888

void	cs_attach __P((struct cs_softc *sc, u_int8_t *enaddr,
	    int *media, int nmedia, int defmedia));
int	cs_verify_eeprom __P((bus_space_tag_t, bus_space_handle_t));
int	cs_read_eeprom __P((bus_space_tag_t, bus_space_handle_t, int,
	    u_int16_t *));
int	cs_intr __P((void *));

#endif /* _DEV_ISA_CS89X0VAR_H_ */
