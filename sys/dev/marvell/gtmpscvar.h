/*	$NetBSD: gtmpscvar.h,v 1.6.6.1 2006/04/22 11:39:09 simonb Exp $	*/

/*
 * Copyright (c) 2002 Allegro Networks, Inc., Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Allegro Networks, Inc., and Wasabi Systems, Inc.
 * 4. The name of Allegro Networks, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 * 5. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ALLEGRO NETWORKS, INC. AND
 * WASABI SYSTEMS, INC. ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL EITHER ALLEGRO NETWORKS, INC. OR WASABI SYSTEMS, INC.
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * gtmpscvar.h - includes for gtmpsctty GT-64260 UART-mode serial driver
 *
 * creation	Mon Apr  9 19:54:33 PDT 2001	cliff
 */
#ifndef _DEV_MARVELL_GTMPSCVAR_H
#define	_DEV_MARVELL_GTMPSCVAR_H

#include "opt_marvell.h"

#ifndef GT_MPSC_DEFAULT_BAUD_RATE
#define	GT_MPSC_DEFAULT_BAUD_RATE	115200
#endif
#define	GTMPSC_CLOCK_DIVIDER            8
#define	GTMPSC_MMCR_HI_TCDV_DEFAULT     GTMPSC_MMCR_HI_TCDV_8X
#define	GTMPSC_MMCR_HI_RCDV_DEFAULT     GTMPSC_MMCR_HI_RCDV_8X
#define	BRG_BCR_CDV_MAX                 0xffff

/*
 * gtmpsc_poll_dmapage_t - used for MPSC getchar/putchar polled console
 *
 *	sdma descriptors must be 16 byte aligned
 *	sdma RX buffer pointers must be 8 byte aligned
 */
#define	GTMPSC_NTXDESC 64
#define	GTMPSC_NRXDESC 64
#define	GTMPSC_TXBUFSZ 16
#define	GTMPSC_RXBUFSZ 16

typedef struct gtmpsc_polltx {
	sdma_desc_t txdesc;
	unsigned char txbuf[GTMPSC_TXBUFSZ];
} gtmpsc_polltx_t;

typedef struct gtmpsc_pollrx {
	sdma_desc_t rxdesc;
	unsigned char rxbuf[GTMPSC_RXBUFSZ];
} gtmpsc_pollrx_t;

typedef struct {
	gtmpsc_polltx_t tx[GTMPSC_NTXDESC];
	gtmpsc_pollrx_t rx[GTMPSC_NRXDESC];
} gtmpsc_poll_sdma_t;

/* Size of the Rx FIFO */
#define	GTMPSC_RXFIFOSZ   (GTMPSC_NRXDESC * GTMPSC_RXBUFSZ * 2)

/* Flags in sc->gtmpsc_flags */
#define	GTMPSCF_KGDB      1

typedef struct gtmpsc_softc {
	struct device gtmpsc_dev;
	bus_space_tag_t gtmpsc_memt;
	bus_space_handle_t gtmpsc_memh;
	bus_dma_tag_t gtmpsc_dmat;
	void *sc_si;				/* softintr cookie */
	struct tty *gtmpsc_tty;			/* our tty */
	int gtmpsc_unit;
	unsigned int gtmpsc_flags;
	unsigned int gtmpsc_baud_rate;
	bus_dma_segment_t gtmpsc_dma_segs[1];
	bus_dmamap_t gtmpsc_dma_map;
	gtmpsc_poll_sdma_t *gtmpsc_poll_sdmapage;
	unsigned int gtmpsc_poll_txix;		/* "current" tx xfer index */
	unsigned int gtmpsc_poll_rxix;		/* "current" rx xfer index */
	unsigned int gtmpsc_cx;			/* data index in gtmpsc_rxbuf */
	unsigned int gtmpsc_nc;			/* data count in gtmpsc_rxbuf */
	unsigned int gtmpsc_chr2;                 /* soft copy of CHR2 */
	unsigned int gtmpsc_chr3;                 /* soft copy of CHR3 */
	unsigned int gtmpsc_brg_bcr;              /* soft copy of BRG_BCR */
	volatile u_char sc_heldchange;          /* new params wait for output */
	volatile u_char sc_tx_busy;
	volatile u_char sc_tx_done;
	volatile u_char sc_tx_stopped;
	u_char  *sc_tba;                        /* Tx buf ptr */
	u_int   sc_tbc;                         /* Tx buf cnt */
	u_int   sc_heldtbc;
	unsigned char gtmpsc_rxbuf[GTMPSC_RXBUFSZ];	/* polling read buffer */
	volatile unsigned int gtmpsc_rxfifo_navail;
						/* available count in fifo */
	unsigned int gtmpsc_rxfifo_putix;		/* put index in fifo */
	unsigned int gtmpsc_rxfifo_getix;		/* get index in fifo */
	unsigned char gtmpsc_rxfifo[GTMPSC_RXFIFOSZ];
	unsigned int cnt_rx_from_sdma;
	unsigned int cnt_rx_to_fifo;
	unsigned int cnt_rx_from_fifo;
	unsigned int cnt_tx_from_ldisc;
	unsigned int cnt_tx_to_sdma;
} gtmpsc_softc_t;

/* Make receiver interrupt 8 times a second */
#define	GTMPSC_MAXIDLE(baudrate)  ((baudrate) / (10 * 8)) /* There are 10 bits
							   in a frame */

int gtmpsccnattach(bus_space_tag_t, bus_space_handle_t, int, int, tcflag_t);
int gtmpsc_is_console(bus_space_tag_t, int);

#endif /* _DEV_MARVELL_GTPSCVAR_H */
