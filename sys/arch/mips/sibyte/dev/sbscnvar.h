/* $NetBSD: sbscnvar.h,v 1.1.10.2 2002/06/23 17:38:08 jdolecek Exp $ */

/*
 * Copyright 2000, 2001
 * Broadcom Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and copied only
 * in accordance with the following terms and conditions.  Subject to these
 * conditions, you may download, copy, install, use, modify and distribute
 * modified or unmodified copies of this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce and
 *    retain this copyright notice and list of conditions as they appear in
 *    the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Broadcom Corporation. Neither the "Broadcom Corporation" name nor any
 *    trademark or logo of Broadcom Corporation may be used to endorse or
 *    promote products derived from this software without the prior written
 *    permission of Broadcom Corporation.
 *
 * 3) THIS SOFTWARE IS PROVIDED "AS-IS" AND ANY EXPRESS OR IMPLIED
 *    WARRANTIES, INCLUDING BUT NOT LIMITED TO, ANY IMPLIED WARRANTIES OF
 *    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR
 *    NON-INFRINGEMENT ARE DISCLAIMED. IN NO EVENT SHALL BROADCOM BE LIABLE
 *    FOR ANY DAMAGES WHATSOEVER, AND IN PARTICULAR, BROADCOM SHALL NOT BE
 *    LIABLE FOR DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *    BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *    OR OTHERWISE), EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* from: $NetBSD: comvar.h,v 1.32 2000/03/23 07:01:30 thorpej Exp */

/*
 * Copyright (c) 1996 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
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

#include "rnd.h"
#if NRND > 0 && defined(RND_SBSCN)
#include <sys/rnd.h>
#endif

#include <sys/callout.h>

#define	SBSCN_CHAN(x)		((minor(x) & 0x00001) >> 0)
#define	SBSCN_UNIT(x)		((minor(x) & 0x7fffe) >> 1)
#define	SBSCN_DIALOUT(x)	((minor(x) & 0x80000) != 0)

#define	SBSCN_TOLERANCE	30	/* baud rate tolerance, in 0.1% units */

int	sbscn_cnattach(u_long addr, int chan, int rate,
	    tcflag_t cflag);
#ifdef KGDB
int	sbscn_kgdb_attach(u_long addr, int chan, int rate,
	    tcflag_t cflag);
#endif
int	sbscn_is_console(u_long addr, int chan);

/* Hardware flag masks */
#define	SBSCN_HW_CONSOLE		0x01
#define	SBSCN_HW_KGDB			0x02
#define	SBSCN_HW_DEV_OK			0x04

/* Buffer size for character buffer */
#define	SBSCN_RING_SIZE		2048

struct sbscn_channel {
	int		ch_num;
	struct sbscn_softc *ch_sc;
	struct tty	*ch_tty;	/* tty struct */
	void		*ch_intrhand;	/* interrupt registration handle */
	void		*ch_si;		/* softintr cookie */
	struct callout ch_diag_callout;	/* callout for diagnostic msgs */

	volatile char	*ch_base;	/* kseg1 addr of channel regs */
	volatile char	*ch_isr_base;	/* kseg1 addr of channel ISR */
	volatile char	*ch_imr_base;	/* kseg1 addr of channel IMR */
#ifdef XXXCGDnotyet
	volatile char	*ch_inchg_base;	/* kseg1 addr of channel inport-chg */
#endif

	u_int		ch_overflows;
	u_int		ch_floods;
	u_int		ch_errors;

	u_int		ch_hwflags;
	u_int		ch_swflags;

	u_int		ch_r_hiwat;
	u_int		ch_r_lowat;

	/* receive ring buffer management */
	u_char *volatile ch_rbget;
	u_char *volatile ch_rbput;
 	volatile u_int	ch_rbavail;
	u_char		*ch_rbuf;
	u_char		*ch_ebuf;

	/* transmit buffer management */
	u_char		*ch_tba;
	u_int		ch_tbc;
	u_int		ch_heldtbc;

	volatile u_int ch_rx_flags;
#define	RX_TTY_BLOCKED		0x01
#define	RX_TTY_OVERFLOWED	0x02
#define	RX_IBUF_BLOCKED		0x04
#define	RX_IBUF_OVERFLOWED	0x08
#define	RX_ANY_BLOCK		0x0f
	volatile u_int	ch_tx_busy;
	volatile u_int	ch_tx_done;
	volatile u_int	ch_tx_stopped;
	volatile u_int	ch_st_check;
	volatile u_int	ch_rx_ready;

	volatile u_char ch_heldchange;

	volatile u_int	ch_brc;
	volatile u_char	ch_imr;
	volatile u_char	ch_iports, ch_iports_delta;
	volatile u_char	ch_oports, ch_oports_active;
	volatile u_char	ch_mode1, ch_mode2;

	u_char		ch_i_dcd, ch_i_cts, ch_i_dsr, ch_i_ri, ch_i_mask;
	u_char		ch_o_dtr, ch_o_rts, ch_o_mask;

	u_char		ch_i_dcd_pin, ch_i_cts_pin, ch_i_dsr_pin, ch_i_ri_pin;
	u_char		ch_o_dtr_pin, ch_o_rts_pin;

#if NRND > 0 && defined(RND_SBSCN)
	rndsource_element_t  ch_rnd_source;
#endif
};

struct sbscn_softc {
	struct device	sc_dev;		/* base device */

	/* shared data structures */
	u_long	sc_addr;	/* phys addr of DUART XXX bus_space */

	struct sbscn_channel sc_channels[2];
};

/* Macros to clear/set/test flags. */
#define	SET(t, f)	(t) |= (f)
#define	CLR(t, f)	(t) &= ~(f)
#define	ISSET(t, f)	((t) & (f))
