/*	$NetBSD: kbd_zs.c,v 1.3 2000/03/22 16:08:51 pk Exp $	*/

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
 * Keyboard driver (/dev/kbd -- note that we do not have minor numbers
 * [yet?]).  Translates incoming bytes to ASCII or to `firm_events' and
 * passes them up to the appropriate reader.
 */

/*
 * Zilog Z8530 Dual UART driver (keyboard interface)
 *
 * This is the 8530 portion of the driver that will be attached to
 * the "zsc" driver for a Sun keyboard.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/syslog.h>

#include <dev/ic/z8530reg.h>
#include <machine/z8530var.h>
#include <machine/vuid_event.h>
#include <machine/kbd.h>
#include <dev/sun/event_var.h>
#include <dev/sun/kbd_xlate.h>
#include <dev/sun/kbdvar.h>

/****************************************************************
 * Interface to the lower layer (zscc)
 ****************************************************************/

static void kbd_zs_rxint __P((struct zs_chanstate *));
static void kbd_zs_stint __P((struct zs_chanstate *, int));
static void kbd_zs_txint __P((struct zs_chanstate *));
static void kbd_zs_softint __P((struct zs_chanstate *));

struct zsops zsops_kbd = {
	kbd_zs_rxint,	/* receive char available */
	kbd_zs_stint,	/* external/status */
	kbd_zs_txint,	/* xmit buffer empty */
	kbd_zs_softint,	/* process software interrupt */
};

static int	kbd_zs_match(struct device *, struct cfdata *, void *);
static void	kbd_zs_attach(struct device *, struct device *, void *);
static void	kbd_zs_write_data __P((struct kbd_softc *, int));

struct cfattach kbd_zs_ca = {
	sizeof(struct kbd_softc), kbd_zs_match, kbd_zs_attach
};

/*
 * kbd_zs_match: how is this zs channel configured?
 */
int 
kbd_zs_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void   *aux;
{
	struct zsc_attach_args *args = aux;

	/* Exact match required for keyboard. */
	if (cf->cf_loc[ZSCCF_CHANNEL] == args->channel)
		return 2;

	return 0;
}

void 
kbd_zs_attach(parent, self, aux)
	struct device *parent, *self;
	void   *aux;

{
	struct zsc_softc *zsc = (void *) parent;
	struct kbd_softc *k = (void *) self;
	struct zsc_attach_args *args = aux;
	struct zs_chanstate *cs;
	struct cfdata *cf;
	int channel, kbd_unit;
	int reset, s;

	cf = k->k_dev.dv_cfdata;
	kbd_unit = k->k_dev.dv_unit;
	channel = args->channel;
	cs = zsc->zsc_cs[channel];
	cs->cs_private = k;
	cs->cs_ops = &zsops_kbd;
	k->k_cs = cs;
	k->k_write_data = kbd_zs_write_data;

	if ((args->hwflags & ZS_HWFLAG_CONSOLE_INPUT) != 0) {
		/*
		 * Hookup ourselves as the console input channel
		 */
		struct cons_channel *cc;

		if ((cc = malloc(sizeof *cc, M_DEVBUF, M_NOWAIT)) == NULL)
			return;

		cc->cc_dev = self;
		cc->cc_iopen = kbd_cc_open;
		cc->cc_iclose = kbd_cc_close;
		cc->cc_upstream = NULL;
		cons_attach_input(cc);
		k->k_cc = cc;
		k->k_isconsole = 1;
		printf(" (console input)");
	}
	printf("\n");

	/* Initialize the speed, etc. */
	s = splzs();
	if (k->k_isconsole == 0) {
		/* Not the console; may need reset. */
		reset = (channel == 0) ?
			ZSWR9_A_RESET : ZSWR9_B_RESET;
		zs_write_reg(cs, 9, reset);
	}
	/* These are OK as set by zscc: WR3, WR4, WR5 */
	/* We don't care about status interrupts. */
	cs->cs_preg[1] = ZSWR1_RIE | ZSWR1_TIE;
	(void) zs_set_speed(cs, KBD_BPS);
	zs_loadchannelregs(cs);
	splx(s);

	/* Do this before any calls to kbd_rint(). */
	kbd_xlate_init(&k->k_state);

	/* XXX - Do this in open? */
	k->k_repeat_start = hz/2;
	k->k_repeat_step = hz/20;

	/* Magic sequence. */
	k->k_magic1 = KBD_L1;
	k->k_magic2 = KBD_A;
}

/*
 * used by kbd_start_tx();
 */
void
kbd_zs_write_data(k, c)
	struct kbd_softc *k;
	int c;
{
	int	s;

	/* Need splzs to avoid interruption of the delay. */
	s = splzs();
	zs_write_data(k->k_cs, c);
	splx(s);
}

static void
kbd_zs_rxint(cs)
	register struct zs_chanstate *cs;
{
	register struct kbd_softc *k;
	register int put, put_next;
	register u_char c, rr1;

	k = cs->cs_private;
	put = k->k_rbput;

	/*
	 * First read the status, because reading the received char
	 * destroys the status of this char.
	 */
	rr1 = zs_read_reg(cs, 1);
	c = zs_read_data(cs);

	if (rr1 & (ZSRR1_FE | ZSRR1_DO | ZSRR1_PE)) {
		/* Clear the receive error. */
		zs_write_csr(cs, ZSWR0_RESET_ERRORS);
	}

	/*
	 * Check NOW for a console abort sequence, so that we can
	 * abort even when interrupts are locking up the machine.
	 */
	if (k->k_magic1_down) {
		/* The last keycode was "MAGIC1" down. */
		k->k_magic1_down = 0;
		if (c == k->k_magic2) {
			/* Magic "L1-A" sequence; enter debugger. */
			if (k->k_isconsole) {
				zs_abort(cs);
				/* Debugger done.  Fake L1-up to finish it. */
				c = k->k_magic1 | KBD_UP;
			} else {
				printf("kbd: magic sequence, but not console\n");
			}
		}
	}
	if (c == k->k_magic1) {
		k->k_magic1_down = 1;
	}

	k->k_rbuf[put] = (c << 8) | rr1;
	put_next = (put + 1) & KBD_RX_RING_MASK;

	/* Would overrun if increment makes (put==get). */
	if (put_next == k->k_rbget) {
		k->k_intr_flags |= INTR_RX_OVERRUN;
	} else {
		/* OK, really increment. */
		put = put_next;
	}

	/* Done reading. */
	k->k_rbput = put;

	/* Ask for softint() call. */
	cs->cs_softreq = 1;
}


static void
kbd_zs_txint(cs)
	register struct zs_chanstate *cs;
{
	register struct kbd_softc *k;

	k = cs->cs_private;
	zs_write_csr(cs, ZSWR0_RESET_TXINT);
	k->k_intr_flags |= INTR_TX_EMPTY;
	/* Ask for softint() call. */
	cs->cs_softreq = 1;
}


static void
kbd_zs_stint(cs, force)
	register struct zs_chanstate *cs;
	int force;
{
	register struct kbd_softc *k;
	register int rr0;

	k = cs->cs_private;

	rr0 = zs_read_csr(cs);
	zs_write_csr(cs, ZSWR0_RESET_STATUS);

#if 0
	if (rr0 & ZSRR0_BREAK) {
		/* Keyboard unplugged? */
		zs_abort(cs);
		return (0);
	}
#endif

	/*
	 * We have to accumulate status line changes here.
	 * Otherwise, if we get multiple status interrupts
	 * before the softint runs, we could fail to notice
	 * some status line changes in the softint routine.
	 * Fix from Bill Studenmund, October 1996.
	 */
	cs->cs_rr0_delta |= (cs->cs_rr0 ^ rr0);
	cs->cs_rr0 = rr0;
	k->k_intr_flags |= INTR_ST_CHECK;

	/* Ask for softint() call. */
	cs->cs_softreq = 1;
}

/*
 * Get input from the recieve ring and pass it on.
 * Note: this is called at splsoftclock()
 */
static void
kbd_zs_softint(cs)
	struct zs_chanstate *cs;
{
	register struct kbd_softc *k;
	register int get, c, s;
	int intr_flags;
	register u_short ring_data;

	k = cs->cs_private;

	/* Atomically get and clear flags. */
	s = splzs();
	intr_flags = k->k_intr_flags;
	k->k_intr_flags = 0;

	/* Now lower to spltty for the rest. */
	(void) spltty();

	/*
	 * Copy data from the receive ring to the event layer.
	 */
	get = k->k_rbget;
	while (get != k->k_rbput) {
		ring_data = k->k_rbuf[get];
		get = (get + 1) & KBD_RX_RING_MASK;

		/* low byte of ring_data is rr1 */
		c = (ring_data >> 8) & 0xff;

		if (ring_data & ZSRR1_DO)
			intr_flags |= INTR_RX_OVERRUN;
		if (ring_data & (ZSRR1_FE | ZSRR1_PE)) {
			/*
			 * After garbage, flush pending input, and
			 * send a reset to resync key translation.
			 */
			log(LOG_ERR, "%s: input error (0x%x)\n",
				k->k_dev.dv_xname, ring_data);
			get = k->k_rbput; /* flush */
			goto send_reset;
		}

		/* Pass this up to the "middle" layer. */
		kbd_input_raw(k, c);
	}
	if (intr_flags & INTR_RX_OVERRUN) {
		log(LOG_ERR, "%s: input overrun\n",
		    k->k_dev.dv_xname);
	send_reset:
		/* Send a reset to resync translation. */
		kbd_output(k, KBD_CMD_RESET);
		kbd_start_tx(k);
	}
	k->k_rbget = get;

	if (intr_flags & INTR_TX_EMPTY) {
		/*
		 * Transmit done.  Try to send more, or
		 * clear busy and wakeup drain waiters.
		 */
		k->k_txflags &= ~K_TXBUSY;
		kbd_start_tx(k);
	}

	if (intr_flags & INTR_ST_CHECK) {
		/*
		 * Status line change.  (Not expected.)
		 */
		log(LOG_ERR, "%s: status interrupt?\n",
		    k->k_dev.dv_xname);
		cs->cs_rr0_delta = 0;
	}

	splx(s);
}
