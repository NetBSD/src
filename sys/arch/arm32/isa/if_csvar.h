/*	$NetBSD: if_csvar.h,v 1.5 1998/07/21 00:20:35 thorpej Exp $	*/

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

#ifndef __IF_CSVAR_H__
#define __IF_CSVAR_H__

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

	void	*sc_ih;			/* interupt handler */
	void 	*sc_sh;			/* shutdown hook */
	bus_space_tag_t sc_iot;		/* bus space tag for IO */
	bus_space_tag_t sc_memt;	/* bus space tag for memory mode */
	bus_space_handle_t sc_ioh;	/* bus space handles */
	bus_space_handle_t sc_memh;
	isa_chipset_tag_t sc_ic;	/* ISA chipset */
	u_int16_t sc_iobase;		/* base IO port address */
   	u_int16_t  sc_int;		/* interrupt level */ 
	int pPacketPagePhys;		/* physical io address */
   	u_int16_t  mediaType;		/* 10BaseT, thin wire */
   	u_int16_t  configFlags;		/* chip configuration software flag */
   	int    inMemoryMode;		/* status */
   	int    txInProgress;		/* flags  */
   	int    resetting;		/* chip in reset mode flag */
	uint dmaMemSize;		/* 16K or 64K if DMA being used */
	char *dmaBase;			/* DMA memory base */
	char *dma_offset;		/* offset into DMA space */
	int sc_drq;			/* DMA request channel : 5,6,7 */

	int	sc_xe_ent;		/* current early-xmit table entry */
	int	sc_xe_togo;		/* # of packets to go at this ent */

	u_int8_t sc_enaddr[6];		/* MAC address */
};

/* Debug Status */
/*#define CS_DEBUG 0*/

#ifdef CS_DEBUG
#define dbg_print(x) printf x
#else
#define dbg_print(x) 
#endif

/* Return Status */
#define	CS_ERROR   -1
#define CS_OK       1
		
/* Config Flags in cs_softc */

#define CFGFLG_MEM_MODE     0x0001
#define CFGFLG_USE_SA       0x0002
#define CFGFLG_IOCHRDY      0x0004
#define CFGFLG_DCDC_POL     0x0008
#define CFGFLG_FDX          0x0010
#define CFGFLG_DMA_MODE	    0x0020
#define CFGFLG_NOT_EEPROM   0x8000


/* Media Type in cs_softc */

#define MEDIA_AUI           0x0001
#define MEDIA_10BASE2       0x0002
#define MEDIA_10BASET       0x0003


/* Miscellaneous definitions */

#define MAXLOOP            0x8888
#define CS_DMA_FRAME_HEADER_SIZE   (sizeof(u_int16_t) * 2)
#define RXBUFCOUNT         16
#define MC_LOANED          5

#endif /* __IF_CSVAR_H__ */
