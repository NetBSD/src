/*	$NetBSD: clmpccvar.h,v 1.1 1999/02/13 17:05:20 scw Exp $ */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Steve C. Woodford.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#ifndef __clmpccvar_h
#define __clmpccvar_h


/* Buffer size for character buffer */
#define	CLMPCC_RING_SIZE	512

/* How many channels per chip */
#define CLMPCC_NUM_CHANS	4

/* Reasons for calling the MD code's iack hook function */
#define CLMPCC_IACK_MODEM	0
#define CLMPCC_IACK_RX		1
#define CLMPCC_IACK_TX		2


struct clmpcc_softc;

/*
 * Each channel is represented by one of the following structures
 */
struct clmpcc_chan {
	struct tty	*ch_tty;	/* This channel's tty structure */
	struct clmpcc_softc *ch_sc;	/* Pointer to chip's softc structure */
	int		ch_car;		/* Channel number (CD2400_REG_CAR) */
	u_char		ch_openflags;	/* Persistant TIOC flags */
	u_char		ch_flags;	/* Various channel-specific flags */
#define	CLMPCC_FLG_IS_CONSOLE	0x01	/* Channel is system console */
#define CLMPCC_FLG_CARRIER_CHNG	0x02
#define CLMPCC_FLG_START_BREAK 	0x04
#define CLMPCC_FLG_END_BREAK 	0x08
#define CLMPCC_FLG_START 	0x10
#define CLMPCC_FLG_STOP 	0x20
#define CLMPCC_FLG_SEND_NULL 	0x40
#define CLMPCC_FLG_FIFO_CLEAR	0x80

	u_char		ch_fifo;	/* Current Rx Fifo threshold */
	u_char		ch_control;

	u_int8_t	*ch_ibuf;	/* Start of input ring buffer */
	u_int8_t	*ch_ibuf_end;	/* End of input ring buffer */
	u_int8_t	*ch_ibuf_rd;	/* Input buffer tail (reader) */
	u_int8_t	*ch_ibuf_wr;	/* Input buffer head (writer) */
};


struct clmpcc_softc {
	struct device	sc_dev;

	/*
	 * The bus/MD-specific attachment code must initialise the
	 * following four fields before calling 'clmpcc_attach_subr()'.
	 */
	bus_space_tag_t	sc_iot;		/* Tag for parent bus */
	bus_space_handle_t sc_ioh;	/* Handle for chip's regs */

	void		*sc_data;	/* MD-specific data */
	int		sc_clk;		/* Clock-rate, in Hz */
	u_char		sc_vector_base;	/* Vector base reg, or 0 for auto */
	u_char		sc_rpilr;	/* Receive Priority Interupt Level */
	u_char		sc_tpilr;	/* Transmit Priority Interupt Level */
	u_char		sc_mpilr;	/* Modem Priority Interupt Level */
	int		sc_swaprtsdtr;	/* Non-zero if RTS and DTR swapped */
	u_int		sc_byteswap;	/* One of the following ... */
#define CLMPCC_BYTESWAP_LOW	0x00	/* *byteswap pin is low */
#define CLMPCC_BYTESWAP_HIGH	0x03	/* *byteswap pin is high */

	/* Called to request a soft interrupt callback to clmpcc_softintr */
	void		(*sc_softhook) __P((struct clmpcc_softc *));

	/* Called when an interrupt has to be acknowledged in polled mode. */
	void		(*sc_iackhook) __P((struct clmpcc_softc *, int));

	/*
	 * No user-serviceable parts below
	 */
	int		sc_soft_running;
	struct clmpcc_chan sc_chans[CLMPCC_NUM_CHANS];
};

extern void	clmpcc_attach	__P((struct clmpcc_softc *));
extern int	clmpcc_cnattach	__P((struct clmpcc_softc *, int, int));
extern int	clmpcc_rxintr	__P((void *));
extern int	clmpcc_txintr	__P((void *));
extern int	clmpcc_mdintr	__P((void *));
extern int 	clmpcc_softintr	__P((void *));

#endif	/* __clmpccvar_h */
