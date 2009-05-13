/*	$NetBSD: if_mcvar.h,v 1.11.58.1 2009/05/13 17:18:01 jym Exp $	*/

/*-
 * Copyright (c) 1997 David Huang <khym@bga.com>
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

#include <macppc/dev/dbdma.h>

#if 1
#define integrate
#define hide
#else
#define integrate	static /*inline*/
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
#define	MC_RXDMABUFS	2
#endif
#if (MC_RXDMABUFS < 2)
#error Must have at least two buffers for DMA!
#endif

#define	MC_NPAGES	((MC_RXDMABUFS * 0x800 + PAGE_SIZE - 1) / PAGE_SIZE)

struct mc_rxframe {
	uint8_t		rx_rcvcnt;
	uint8_t		rx_rcvsts;
	uint8_t		rx_rntpc;
	uint8_t		rx_rcvcc;
	uint8_t		*rx_frame;
};

struct mc_softc {
	struct device	sc_dev;		/* base device glue */
	struct ethercom	sc_ethercom;	/* Ethernet common part */
#define	sc_if		sc_ethercom.ec_if
	struct ifmedia	sc_media;

	struct mc_rxframe	sc_rxframe;
	u_int8_t	sc_biucc;
	u_int8_t	sc_fifocc;
	u_int8_t	sc_plscc;
	u_int8_t	sc_enaddr[6];
	u_int8_t	sc_pad[2];
	int		sc_havecarrier; /* carrier status */
	void		(*sc_bus_init)(struct mc_softc *);
	void		(*sc_putpacket)(struct mc_softc *, u_int);
	int		(*sc_mediachange)(struct mc_softc *);
	void		(*sc_mediastatus)(struct mc_softc *,
				struct ifmediareq *);

	bus_space_tag_t	sc_regt;
	bus_space_handle_t sc_regh;

	u_char		*sc_txbuf, *sc_rxbuf;
	int		sc_txbuf_phys, sc_rxbuf_phys;
	int		sc_tail;

	int		sc_node;
	dbdma_regmap_t	*sc_txdma;
	dbdma_command_t	*sc_txdmacmd;
	dbdma_regmap_t	*sc_rxdma;
	dbdma_command_t	*sc_rxdmacmd;
};

int	mcsetup(struct mc_softc *, u_int8_t *);
int	mcintr(void *arg);
void	mc_rint(struct mc_softc *sc);
