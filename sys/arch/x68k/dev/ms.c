/*	$NetBSD: ms.c,v 1.10.8.1 2001/09/09 19:14:39 thorpej Exp $ */

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
 *	@(#)ms.c	8.1 (Berkeley) 6/11/93
 */

/*
 * X68k mouse driver.
 */

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/syslog.h>
#include <sys/systm.h>
#include <sys/tty.h>
#include <sys/device.h>
#include <sys/signalvar.h>

#include <dev/ic/z8530reg.h>
#include <machine/z8530var.h>

#include <arch/x68k/dev/event_var.h>
#include <machine/vuid_event.h>
#include <arch/x68k/dev/mfp.h>

#include "locators.h"

/*
 * How many input characters we can buffer.
 * The port-specific var.h may override this.
 * Note: must be a power of two!
 */
#define	MS_RX_RING_SIZE	256
#define MS_RX_RING_MASK (MS_RX_RING_SIZE-1)
/*
 * Output buffer.  Only need a few chars.
 */
#define	MS_TX_RING_SIZE	16
#define MS_TX_RING_MASK (MS_TX_RING_SIZE-1)
/*
 * Mouse serial line is fixed at 4800 bps.
 */
#define MS_BPS 4800

/*
 * Mouse state.  A SHARP X1/X680x0 mouse is a fairly simple device,
 * producing three-byte blobs of the form:
 *
 *	b dx dy
 *
 * where b is the button state, encoded as 0x80|(buttons)---there are
 * two buttons (2=left, 1=right)---and dx,dy are X and Y delta values.
 *
 * It needs a trigger for the transmission.  When zs RTS negated, the
 * mouse begins the sequence.  RTS assertion has no effect.
 */
struct ms_softc {
	struct	device ms_dev;		/* required first: base device */
	struct	zs_chanstate *ms_cs;

	struct callout ms_modem_ch;

	/* Flags to communicate with ms_softintr() */
	volatile int ms_intr_flags;
#define	INTR_RX_OVERRUN 1
#define INTR_TX_EMPTY   2
#define INTR_ST_CHECK   4

	/*
	 * The receive ring buffer.
	 */
	u_int	ms_rbget;	/* ring buffer `get' index */
	volatile u_int	ms_rbput;	/* ring buffer `put' index */
	u_short	ms_rbuf[MS_RX_RING_SIZE]; /* rr1, data pairs */

	/*
	 * State of input translator
	 */
	short	ms_byteno;		/* input byte number, for decode */
	char	ms_mb;			/* mouse button state */
	char	ms_ub;			/* user button state */
	int	ms_dx;			/* delta-x */
	int	ms_dy;			/* delta-y */
	int	ms_rts;			/* MSCTRL */
	int	ms_nodata;

	/*
	 * State of upper interface.
	 */
	volatile int ms_ready;		/* event queue is ready */
	struct	evvar ms_events;	/* event queue state */
} ms_softc;

cdev_decl(ms);

static int ms_match __P((struct device*, struct cfdata*, void*));
static void ms_attach __P((struct device*, struct device*, void*));
static void ms_trigger __P((struct zs_chanstate*, int));
void ms_modem __P((void *));

struct cfattach ms_ca = {
	sizeof(struct ms_softc), ms_match, ms_attach
};

extern struct zsops zsops_ms;
extern struct cfdriver ms_cd;

/*
 * ms_match: how is this zs channel configured?
 */
int 
ms_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void   *aux;
{
	struct zsc_attach_args *args = aux;
	struct zsc_softc *zsc = (void*) parent;

	/* Exact match required for the mouse. */
	if (cf->cf_loc[ZSCCF_CHANNEL] != args->channel)
		return 0;
	if (args->channel != 1)
		return 0;
	if (&zsc->zsc_addr->zs_chan_b != (struct zschan *) ZSMS_PHYSADDR)
		return 0;

	return 2;
}

void 
ms_attach(parent, self, aux)
	struct device *parent, *self;
	void   *aux;

{
	struct zsc_softc *zsc = (void *) parent;
	struct ms_softc *ms = (void *) self;
	struct zs_chanstate *cs;
	struct cfdata *cf;
	int reset, s;

	callout_init(&ms->ms_modem_ch);

	cf = ms->ms_dev.dv_cfdata;
	cs = zsc->zsc_cs[1];
	cs->cs_private = ms;
	cs->cs_ops = &zsops_ms;
	ms->ms_cs = cs;

	/* Initialize the speed, etc. */
	s = splzs();
	/* May need reset... */
	reset = ZSWR9_B_RESET;
	zs_write_reg(cs, 9, reset);
	/* We don't care about status or tx interrupts. */
	cs->cs_preg[1] = ZSWR1_RIE;
	cs->cs_preg[4] = ZSWR4_CLK_X16 | ZSWR4_TWOSB;
	(void) zs_set_speed(cs, MS_BPS);
	zs_loadchannelregs(cs);
	splx(s);

	/* Initialize translator. */
	ms->ms_ready = 0;

	printf ("\n");
}

/****************************************************************
 *  Entry points for /dev/mouse
 *  (open,close,read,write,...)
 ****************************************************************/

int
msopen(dev, flags, mode, p)
	dev_t dev;
	int flags, mode;
	struct proc *p;
{
	struct ms_softc *ms;
	int unit;
	int s;

	unit = minor(dev);
	if (unit >= ms_cd.cd_ndevs)
		return (ENXIO);
	ms = ms_cd.cd_devs[unit];
	if (ms == NULL)
		return (ENXIO);

	/* This is an exclusive open device. */
	if (ms->ms_events.ev_io)
		return (EBUSY);
	ms->ms_events.ev_io = p;
	ev_init(&ms->ms_events);	/* may cause sleep */

	ms->ms_ready = 1;		/* start accepting events */
	ms->ms_rts = 1;
	ms->ms_byteno = -1;
	ms->ms_nodata = 0;

	/* start sequencer */
	ms_modem(ms);

	return (0);
}

int
msclose(dev, flags, mode, p)
	dev_t dev;
	int flags, mode;
	struct proc *p;
{
	struct ms_softc *ms;

	ms = ms_cd.cd_devs[minor(dev)];
	ms->ms_ready = 0;		/* stop accepting events */
	callout_stop(&ms->ms_modem_ch);
	ev_fini(&ms->ms_events);

	ms->ms_events.ev_io = NULL;
	return (0);
}

int
msread(dev, uio, flags)
	dev_t dev;
	struct uio *uio;
	int flags;
{
	struct ms_softc *ms;

	ms = ms_cd.cd_devs[minor(dev)];
	return (ev_read(&ms->ms_events, uio, flags));
}

/* this routine should not exist, but is convenient to write here for now */
int
mswrite(dev, uio, flags)
	dev_t dev;
	struct uio *uio;
	int flags;
{

	return (EOPNOTSUPP);
}

int
msioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	register caddr_t data;
	int flag;
	struct proc *p;
{
	struct ms_softc *ms;

	ms = ms_cd.cd_devs[minor(dev)];

	switch (cmd) {

	case FIONBIO:		/* we will remove this someday (soon???) */
		return (0);

	case FIOASYNC:
		ms->ms_events.ev_async = *(int *)data != 0;
		return (0);

	case TIOCSPGRP:
		if (*(int *)data != ms->ms_events.ev_io->p_pgid)
			return (EPERM);
		return (0);

	case VUIDGFORMAT:
		/* we only do firm_events */
		*(int *)data = VUID_FIRM_EVENT;
		return (0);

	case VUIDSFORMAT:
		if (*(int *)data != VUID_FIRM_EVENT)
			return (EINVAL);
		return (0);
	}
	return (ENOTTY);
}

int
mspoll(dev, events, p)
	dev_t dev;
	int events;
	struct proc *p;
{
	struct ms_softc *ms;

	ms = ms_cd.cd_devs[minor(dev)];
	return (ev_poll(&ms->ms_events, events, p));
}

int
mskqfilter(dev_t dev, struct knote *kn)
{
	struct ms_softc *ms;

	ms = ms_cd.cd_devs[minor(dev)];
	return (ev_kqfilter(&ms->ms_events, kn));
}

/****************************************************************
 * Middle layer (translator)
 ****************************************************************/

static void ms_input __P((struct ms_softc *, int c));


/*
 * Called by our ms_softint() routine on input.
 */
static void
ms_input(ms, c)
	register struct ms_softc *ms;
	register int c;
{
	register struct firm_event *fe;
	register int mb, ub, d, get, put, any;
	static const char to_one[] = { 1, 2, 3 };
	static const int to_id[] = { MS_LEFT, MS_RIGHT, MS_MIDDLE };

	/*
	 * Discard input if not ready.  Drop sync on parity or framing
	 * error; gain sync on button byte.
	 */
	if (ms->ms_ready == 0)
		return;

	ms->ms_nodata = 0;
	/*
	 * Run the decode loop, adding to the current information.
	 * We add, rather than replace, deltas, so that if the event queue
	 * fills, we accumulate data for when it opens up again.
	 */
	switch (ms->ms_byteno) {

	case -1:
		return;

	case 0:
		/* buttons */
		ms->ms_byteno = 1;
		ms->ms_mb = c & 0x3;
		return;

	case 1:
		/* delta-x */
		ms->ms_byteno = 2;
		ms->ms_dx += (char)c;
		return;

	case 2:
		/* delta-y */
		ms->ms_byteno = -1;
		ms->ms_dy += (char)c;
		break;

	default:
		panic("ms_input");
		/* NOTREACHED */
	}

	/*
	 * We have at least one event (mouse button, delta-X, or
	 * delta-Y; possibly all three, and possibly three separate
	 * button events).  Deliver these events until we are out
	 * of changes or out of room.  As events get delivered,
	 * mark them `unchanged'.
	 */
	any = 0;
	get = ms->ms_events.ev_get;
	put = ms->ms_events.ev_put;
	fe = &ms->ms_events.ev_q[put];

	/* NEXT prepares to put the next event, backing off if necessary */
#define	NEXT \
	if ((++put) % EV_QSIZE == get) { \
		put--; \
		goto out; \
	}
	/* ADVANCE completes the `put' of the event */
#define	ADVANCE \
	fe++; \
	if (put >= EV_QSIZE) { \
		put = 0; \
		fe = &ms->ms_events.ev_q[0]; \
	} \

	mb = ms->ms_mb;
	ub = ms->ms_ub;
	while ((d = mb ^ ub) != 0) {
		/*
		 * Mouse button change.  Convert up to three changes
		 * to the `first' change, and drop it into the event queue.
		 */
		NEXT;
		d = to_one[d - 1];		/* from 1..7 to {1,2,4} */
		fe->id = to_id[d - 1];		/* from {1,2,4} to ID */
		fe->value = mb & d ? VKEY_DOWN : VKEY_UP;
		fe->time = time;
		ADVANCE;
		ub ^= d;
		any++;
	}
	if (ms->ms_dx) {
		NEXT;
		fe->id = LOC_X_DELTA;
		fe->value = ms->ms_dx;
		fe->time = time;
		ADVANCE;
		ms->ms_dx = 0;
		any++;
	}
	if (ms->ms_dy) {
		NEXT;
		fe->id = LOC_Y_DELTA;
		fe->value = -ms->ms_dy;	/* XXX? */
		fe->time = time;
		ADVANCE;
		ms->ms_dy = 0;
		any++;
	}
out:
	if (any) {
		ms->ms_ub = ub;
		ms->ms_events.ev_put = put;
		EV_WAKEUP(&ms->ms_events);
	}
}

/****************************************************************
 * Interface to the lower layer (zscc)
 ****************************************************************/

static void ms_rxint __P((struct zs_chanstate *));
static void ms_stint __P((struct zs_chanstate *, int));
static void ms_txint __P((struct zs_chanstate *));
static void ms_softint __P((struct zs_chanstate *));

static void
ms_rxint(cs)
	register struct zs_chanstate *cs;
{
	register struct ms_softc *ms;
	register int put, put_next;
	register u_char c, rr1;

	ms = cs->cs_private;
	put = ms->ms_rbput;

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

	ms->ms_rbuf[put] = (c << 8) | rr1;
	put_next = (put + 1) & MS_RX_RING_MASK;

	/* Would overrun if increment makes (put==get). */
	if (put_next == ms->ms_rbget) {
		ms->ms_intr_flags |= INTR_RX_OVERRUN;
	} else {
		/* OK, really increment. */
		put = put_next;
	}

	/* Done reading. */
	ms->ms_rbput = put;

	/* Ask for softint() call. */
	cs->cs_softreq = 1;
}


static void
ms_txint(cs)
	register struct zs_chanstate *cs;
{
	register struct ms_softc *ms;

	ms = cs->cs_private;
	zs_write_csr(cs, ZSWR0_RESET_TXINT);
	ms->ms_intr_flags |= INTR_TX_EMPTY;
	/* Ask for softint() call. */
	cs->cs_softreq = 1;
}


static void
ms_stint(cs, force)
	register struct zs_chanstate *cs;
	int force;
{
	register struct ms_softc *ms;
	register int rr0;

	ms = cs->cs_private;

	rr0 = zs_read_csr(cs);
	zs_write_csr(cs, ZSWR0_RESET_STATUS);

	/*
	 * We have to accumulate status line changes here.
	 * Otherwise, if we get multiple status interrupts
	 * before the softint runs, we could fail to notice
	 * some status line changes in the softint routine.
	 * Fix from Bill Studenmund, October 1996.
	 */
	cs->cs_rr0_delta |= (cs->cs_rr0 ^ rr0);
	cs->cs_rr0 = rr0;
	ms->ms_intr_flags |= INTR_ST_CHECK;

	/* Ask for softint() call. */
	cs->cs_softreq = 1;
}


static void
ms_softint(cs)
	struct zs_chanstate *cs;
{
	register struct ms_softc *ms;
	register int get, c, s;
	int intr_flags;
	register u_short ring_data;

	ms = cs->cs_private;

	/* Atomically get and clear flags. */
	s = splzs();
	intr_flags = ms->ms_intr_flags;
	ms->ms_intr_flags = 0;

	/* Now lower to spltty for the rest. */
	(void) spltty();

	/*
	 * Copy data from the receive ring to the event layer.
	 */
	get = ms->ms_rbget;
	while (get != ms->ms_rbput) {
		ring_data = ms->ms_rbuf[get];
		get = (get + 1) & MS_RX_RING_MASK;

		/* low byte of ring_data is rr1 */
		c = (ring_data >> 8) & 0xff;

		if (ring_data & ZSRR1_DO)
			intr_flags |= INTR_RX_OVERRUN;
		if (ring_data & (ZSRR1_FE | ZSRR1_PE)) {
			log(LOG_ERR, "%s: input error (0x%x)\n",
				ms->ms_dev.dv_xname, ring_data);
			c = -1;	/* signal input error */
		}

		/* Pass this up to the "middle" layer. */
		ms_input(ms, c);
	}
	if (intr_flags & INTR_RX_OVERRUN) {
		log(LOG_ERR, "%s: input overrun\n",
		    ms->ms_dev.dv_xname);
	}
	ms->ms_rbget = get;

	if (intr_flags & INTR_TX_EMPTY) {
		/*
		 * Transmit done.  (Not expected.)
		 */
		log(LOG_ERR, "%s: transmit interrupt?\n",
		    ms->ms_dev.dv_xname);
	}

	if (intr_flags & INTR_ST_CHECK) {
		/*
		 * Status line change.  (Not expected.)
		 */
		log(LOG_ERR, "%s: status interrupt?\n",
		    ms->ms_dev.dv_xname);
		cs->cs_rr0_delta = 0;
	}

	splx(s);
}

struct zsops zsops_ms = {
	ms_rxint,	/* receive char available */
	ms_stint,	/* external/status */
	ms_txint,	/* xmit buffer empty */
	ms_softint,	/* process software interrupt */
};


static void
ms_trigger (cs, onoff)
	struct zs_chanstate *cs;
	int onoff;
{
	/* for front connected one */
	if (onoff)
		cs->cs_preg[5] |= ZSWR5_RTS;
	else
		cs->cs_preg[5] &= ~ZSWR5_RTS;
	cs->cs_creg[5] = cs->cs_preg[5];
	zs_write_reg(cs, 5, cs->cs_preg[5]);

	/* for keyborad connected one */
	mfp_send_usart (onoff | 0x40);
}

/*
 * mouse timer interrupt.
 * called after system tick interrupt is done.
 */
void
ms_modem(arg)
	void *arg;
{
	struct ms_softc *ms = arg;
	int s;

	if (!ms->ms_ready)
		return;

	s = splzs();

	if (ms->ms_nodata++ > 250) { /* XXX */
		log(LOG_ERR, "%s: no data for 5 secs. resetting.\n",
		    ms->ms_dev.dv_xname);
		ms->ms_byteno = -1;
		ms->ms_nodata = 0;
		ms->ms_rts = 0;
	}

	if (ms->ms_rts) {
		if (ms->ms_byteno == -1) {
			/* start next sequence */
			ms->ms_rts = 0;
			ms_trigger(ms->ms_cs, ms->ms_rts);
			ms->ms_byteno = 0;
		}
	} else {
		ms->ms_rts = 1;
		ms_trigger(ms->ms_cs, ms->ms_rts);
	}

	(void) splx(s);
	callout_reset(&ms->ms_modem_ch, 2, ms_modem, ms);
}
