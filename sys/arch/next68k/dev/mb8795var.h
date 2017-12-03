/*	$NetBSD: mb8795var.h,v 1.13.6.2 2017/12/03 11:36:33 jdolecek Exp $	*/
/*
 * Copyright (c) 1998 Darrin B. Jewell
 * All rights reserved.
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
 */

#include <sys/rndsource.h>

#define MB8795_NRXBUFS (32)

struct mb8795_softc;

/*
 * Function switch used as glue to MD code.
 */
struct mb8795_glue {
	/* Mandatory entry points. */
	u_char	(*gl_read_reg)(struct mb8795_softc *, int);
	void	(*gl_write_reg)(struct mb8795_softc *, int, u_char);
	void	(*gl_dma_reset)(struct mb8795_softc *);
	void	(*gl_dma_rx_setup)(struct mb8795_softc *);
	void	(*gl_dma_rx_go)(struct mb8795_softc *);
	struct mbuf *	(*gl_dma_rx_mbuf)(struct mb8795_softc *);
	void	(*gl_dma_tx_setup)(struct mb8795_softc *);
	void	(*gl_dma_tx_go)(struct mb8795_softc *);
	int	(*gl_dma_tx_mbuf)(struct mb8795_softc *, struct mbuf *);
	int	(*gl_dma_tx_isactive)(struct mb8795_softc *);
	/* Optional entry points. */
};

struct mb8795_softc {
	device_t		sc_dev;		/* base device glue */
	struct ethercom		sc_ethercom;	/* Ethernet common part */

	struct	mb8795_glue 	*sc_glue;	/* glue to MD code */

	void	*sc_sh;		/* shutdownhook cookie */

	int sc_debug;

	bus_space_tag_t sc_bmap_bst;    /* bus space tag */

	bus_space_handle_t sc_bmap_bsh; /* bus space handle */

	u_int8_t sc_enaddr[6];

	struct ifaltq sc_tx_snd;

	struct ifmedia sc_media;

	krndsource_t     rnd_source;

};

/*
 * Macros to read and write the chip's registers.
 */
#define	MB_READ_REG(sc, reg)		\
	(*(sc)->sc_glue->gl_read_reg)((sc), (reg))
#define	MB_WRITE_REG(sc, reg, val)	\
	(*(sc)->sc_glue->gl_write_reg)((sc), (reg), (val))

/*
 * DMA macros for mb8795
 */
#define	MBDMA_RESET(sc)	(*(sc)->sc_glue->gl_dma_reset)((sc))
#define	MBDMA_SETUP(sc, addr, len, datain, dmasize)	\
     (*(sc)->sc_glue->gl_dma_setup)((sc), (addr), (len), (datain), (dmasize))
#define	MBDMA_GO(sc)		(*(sc)->sc_glue->gl_dma_go)((sc))
#define	MBDMA_ISACTIVE(sc)	(*(sc)->sc_glue->gl_dma_isactive)((sc))
#define MBDMA_RX_SETUP(sc)	(*(sc)->sc_glue->gl_dma_rx_setup) ((sc))
#define MBDMA_RX_GO(sc)		(*(sc)->sc_glue->gl_dma_rx_go) ((sc))
#define MBDMA_RX_MBUF(sc)	(*(sc)->sc_glue->gl_dma_rx_mbuf) ((sc))
#define MBDMA_TX_SETUP(sc)	(*(sc)->sc_glue->gl_dma_tx_setup) ((sc))
#define MBDMA_TX_GO(sc)		(*(sc)->sc_glue->gl_dma_tx_go) ((sc))
#define MBDMA_TX_MBUF(sc,m)	(*(sc)->sc_glue->gl_dma_tx_mbuf) ((sc), (m))
#define	MBDMA_TX_ISACTIVE(sc)	(*(sc)->sc_glue->gl_dma_tx_isactive)((sc))

void mb8795_config(struct mb8795_softc *, int *, int, int);
void mb8795_init(struct mb8795_softc *);
int mb8795_ioctl(struct ifnet *, u_long, void *);
void mb8795_reset(struct mb8795_softc *);
void mb8795_start(struct ifnet *);
void mb8795_stop(struct mb8795_softc *);
void mb8795_watchdog(struct ifnet *);

void mb8795_rint(struct mb8795_softc *);
void mb8795_tint(struct mb8795_softc *);
