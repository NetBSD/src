/*	$NetBSD: mb8795var.h,v 1.3.2.1 2002/06/23 17:38:57 jdolecek Exp $	*/
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Darrin B. Jewell
 * 4. The name of the author may not be used to endorse or promote products
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
 */

#include "rnd.h"                /* is random device-driver configured? */
#if NRND > 0
#include <sys/rnd.h>
#endif  /* NRND */

#define MB8795_NRXBUFS (32)

struct mb8795_softc {
	struct	device sc_dev;		/* base device glue */
	struct	ethercom sc_ethercom;	/* Ethernet common part */

	void	*sc_sh;		/* shutdownhook cookie */

	int sc_debug;

	bus_space_tag_t sc_bst;    /* bus space tag */

	bus_space_handle_t sc_bsh; /* bus space handle */

	u_int8_t sc_enaddr[6];

	bus_dma_tag_t sc_tx_dmat;
	bus_dmamap_t sc_tx_dmamap; /* should we have multiple of these? */
	struct mbuf *sc_tx_mb_head;   /* pointer to data for this command */
	int sc_tx_loaded;

	u_char *sc_txbuf; 	/* to solve alignment problems, we
				 * copy the mbuf into this buffer before
				 * trying to dma it */

	struct ifaltq sc_tx_snd;

	bus_dma_tag_t sc_rx_dmat;
	bus_dmamap_t sc_rx_dmamap[MB8795_NRXBUFS];
	struct mbuf *sc_rx_mb_head[MB8795_NRXBUFS];
	int sc_rx_loaded_idx;
	int sc_rx_completed_idx;
	int sc_rx_handled_idx;

	struct nextdma_config *sc_tx_nd;		/* @@@ this shouldn't be here */
	struct nextdma_config *sc_rx_nd;		/* @@@ this shouldn't be here */

#if NRND > 0
	rndsource_element_t     rnd_source;
#endif /* NRND */

};

void mb8795_config __P((struct mb8795_softc *));
void mb8795_init __P((struct mb8795_softc *));
int mb8795_ioctl __P((struct ifnet *, u_long, caddr_t));
void mb8795_reset __P((struct mb8795_softc *));
void mb8795_start __P((struct ifnet *));
void mb8795_stop __P((struct mb8795_softc *));
void mb8795_watchdog __P((struct ifnet *));

void mb8795_rint __P((struct mb8795_softc *));
void mb8795_tint __P((struct mb8795_softc *));
