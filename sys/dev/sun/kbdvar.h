/*	$NetBSD: kbdvar.h,v 1.2 2000/03/19 12:50:43 pk Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)kbd.c	8.2 (Berkeley) 10/30/93
 */

/*
 * How many input characters we can buffer.
 * The port-specific var.h may override this.
 * Note: must be a power of two!
 */
#define	KBD_RX_RING_SIZE	256
#define KBD_RX_RING_MASK (KBD_RX_RING_SIZE-1)
/*
 * Output buffer.  Only need a few chars.
 */
#define	KBD_TX_RING_SIZE	16
#define KBD_TX_RING_MASK (KBD_TX_RING_SIZE-1)
/*
 * Keyboard serial line speed is fixed at 1200 bps.
 */
#define KBD_BPS 1200
#define KBD_RESET_TIMO 1000 /* mS. */

/*
 * XXX - Historical comment - no longer quite right...
 * Keyboard driver state.  The ascii and kbd links go up and down and
 * we just sit in the middle doing translation.  Note that it is possible
 * to get just one of the two links, in which case /dev/kbd is unavailable.
 * The downlink supplies us with `internal' open and close routines which
 * will enable dataflow across the downlink.  We promise to call open when
 * we are willing to take keystrokes, and to call close when we are not.
 * If /dev/kbd is not the console tty input source, we do this whenever
 * /dev/kbd is in use; otherwise we just leave it open forever.
 */
struct zs_chanstate;
struct ucom_softc;

struct kbd_softc {
	struct	device k_dev;		/* required first: base device */

	/* Stuff our parent setup */
	union {
		struct	zs_chanstate *ku_zcs;
		struct	ucom_softc *ku_usc;
	} k_cs_u;
#define	k_cs k_cs_u.ku_zcs
#define	k_usc k_cs_u.ku_usc
	void	(*k_write_data) __P((struct kbd_softc *, int));

	/* Flags to communicate with kbd_softint() */
	volatile int k_intr_flags;
#define	INTR_RX_OVERRUN 1
#define INTR_TX_EMPTY   2
#define INTR_ST_CHECK   4

	/* Transmit state */
	volatile int k_txflags;
#define	K_TXBUSY 1
#define K_TXWANT 2

	/*
	 * State of upper interface.
	 */
	int	k_isopen;		/* set if open has been done */
	int	k_evmode;		/* set if we should produce events */
	struct	evvar k_events;		/* event queue state */

	/*
	 * ACSI translation state
	 */
	int k_repeat_start; 	/* initial delay */
	int k_repeat_step;  	/* inter-char delay */
	int	k_repeatsym;		/* repeating symbol */
	int	k_repeating;		/* we've called timeout() */
	struct	kbd_state k_state;	/* ASCII translation state */

	/* Console hooks */
	char k_isconsole;
	struct cons_channel *k_cc;

	/*
	 * Magic sequence stuff (L1-A)
	 */
	char k_magic1_down;
	u_char k_magic1;	/* L1 */
	u_char k_magic2;	/* A */


	/*
	 * The transmit ring buffer.
	 */
	volatile u_int	k_tbget;	/* transmit buffer `get' index */
	volatile u_int	k_tbput;	/* transmit buffer `put' index */
	u_char	k_tbuf[KBD_TX_RING_SIZE]; /* data */

	/*
	 * The receive ring buffer.
	 */
	u_int	k_rbget;	/* ring buffer `get' index */
	volatile u_int	k_rbput;	/* ring buffer `put' index */
	u_short	k_rbuf[KBD_RX_RING_SIZE]; /* rr1, data pairs */

};

/* functions used between backend/frontend keyboard drivers */
void	kbd_input_raw __P((struct kbd_softc *k, int));
void	kbd_output(struct kbd_softc *k, int c);
void	kbd_start_tx(struct kbd_softc *k);

/*
 * kbd console input channel interface.
 * XXX - does not belong in this header; but for now, kbd is the only user..
 */
struct cons_channel {
	/* Provided by lower driver */
	void	*cc_dev;			/* Lower device private data */
	int	(*cc_iopen)			/* Open lower device */
			__P((struct cons_channel *));
	int	(*cc_iclose)			/* Close lower device */
			__P((struct cons_channel *));

	/* Provided by upper driver */
	void	(*cc_upstream)__P((int));	/* Send a character upstream */
};

/* Special hook to attach the keyboard driver to the console */
void	cons_attach_input __P((struct cons_channel *));

int	kbd_iopen __P((struct cons_channel *));
int	kbd_iclose __P((struct cons_channel *));
