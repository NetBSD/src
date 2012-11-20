/*	$NetBSD: if_mcvar.h,v 1.15.88.1 2012/11/20 03:01:30 tls Exp $	*/

/*-
 * Copyright (c) 1997 David Huang <khym@azeotrope.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/* XXX this file depends on opt_ddb.h */
#ifdef DDB
#define	integrate
#define hide
#else
#define	integrate	static inline
#define hide		static
#endif

#define	MC_REGSPACING	16
#define	MC_REGSIZE	MACE_NREGS * MC_REGSPACING
#define	MACE_REG(x)	((x)*MC_REGSPACING)

#define	NIC_GET(sc, reg)	(bus_space_read_1((sc)->sc_regt,	\
				    (sc)->sc_regh, MACE_REG(reg)))

#define	NIC_PUT(sc, reg, val)	(bus_space_write_1((sc)->sc_regt,	\
				    (sc)->sc_regh, MACE_REG(reg), (val)))

#ifndef	MC_RXDMABUFS
#define	MC_RXDMABUFS	4
#endif
#if (MC_RXDMABUFS < 2)
#error Must have at least two buffers for DMA!
#endif

#define	MC_NPAGES	((MC_RXDMABUFS * 0x800 + PAGE_SIZE - 1) / PAGE_SIZE)

struct mc_rxframe {
	u_int8_t	rx_rcvcnt;
	u_int8_t	rx_rcvsts;
	u_int8_t	rx_rntpc;
	u_int8_t	rx_rcvcc;
	u_char		*rx_frame;
};

struct mc_softc {
	device_t	sc_dev;		/* base device glue */
	struct ethercom	sc_ethercom;	/* Ethernet common part */
#define	sc_if		sc_ethercom.ec_if

	struct mc_rxframe	sc_rxframe;
	u_int8_t	sc_biucc;
	u_int8_t	sc_fifocc;
	u_int8_t	sc_plscc;
	u_int8_t	sc_enaddr[6];
	u_int8_t	sc_pad[2];
	int		sc_havecarrier; /* carrier status */
	void		(*sc_bus_init)(struct mc_softc *);
	void		(*sc_putpacket)(struct mc_softc *, u_int);

	bus_space_tag_t		sc_regt;
	bus_space_handle_t	sc_regh;
	bus_dma_tag_t	sc_dmat;
	bus_dmamap_t	sc_dmam_tx, sc_dmam_rx;
	bus_dma_segment_t	sc_dmasegs_tx, sc_dmasegs_rx;

	uint8_t		*sc_txbuf, *sc_rxbuf;
	bus_addr_t	sc_txbuf_phys, sc_rxbuf_phys;
	int		sc_tail;
	int		sc_rxset;
	int		sc_txset, sc_txseti;
};

int	mcsetup(struct mc_softc *, u_int8_t *);
void	mcintr(void *arg);
void	mc_rint(struct mc_softc *);
u_char	mc_get_enaddr(bus_space_tag_t, bus_space_handle_t, bus_size_t,
		      u_char *);
