/*	$NetBSD: zskbd.c,v 1.1.2.1 1998/10/19 19:38:43 drochner Exp $	*/

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
 * This is the "slave" driver that will be attached to
 * the "zsc" driver for a Sun keyboard.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/syslog.h>
#include <sys/malloc.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wskbdvar.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsksymvar.h>
#include <dev/dec/wskbdmap_lk201.h>

#include <dev/ic/z8530reg.h>
#include <machine/z8530var.h>

#include <dev/tc/tcvar.h>
#include <arch/pmax/tc/zs_ioasicvar.h>
#include <arch/pmax/tc/zskbdvar.h>
#include <dev/dec/lk201reg.h>
#include <dev/dec/lk201var.h>

#include "locators.h"

/*
 * How many input characters we can buffer.
 * The port-specific var.h may override this.
 * Note: must be a power of two!
 */
#define	ZSKBD_RX_RING_SIZE	256
#define ZSKBD_RX_RING_MASK (ZSKBD_RX_RING_SIZE-1)
/*
 * Output buffer.  Only need a few chars.
 */
#define	ZSKBD_TX_RING_SIZE	16
#define ZSKBD_TX_RING_MASK (ZSKBD_TX_RING_SIZE-1)

#define ZSKBD_BPS 4800

struct zskbd_internal {
	struct	zs_chanstate *zsi_cs;
	struct lk201_state zsi_ks;
};

struct zskbd_internal zskbd_console_internal;

struct zskbd_softc {
	struct	device zskbd_dev;	/* required first: base device */

	struct zskbd_internal *sc_itl;

	/* Flags to communicate with zskbd_softintr() */
	volatile int zskbd_intr_flags;
#define	INTR_RX_OVERRUN 1
#define INTR_TX_EMPTY   2
#define INTR_ST_CHECK   4

	/*
	 * The receive ring buffer.
	 */
	u_int	zskbd_rbget;	/* ring buffer `get' index */
	volatile u_int	zskbd_rbput;	/* ring buffer `put' index */
	u_short	zskbd_rbuf[ZSKBD_RX_RING_SIZE]; /* rr1, data pairs */

	int sc_enabled;
	int kbd_type;
    
	int leds_state;

	struct device *sc_wskbddev;
};

struct zsops zsops_zskbd;

static void	zskbd_input __P((struct zskbd_softc *, int));
static void	zskbd_lk201_init __P((struct zs_chanstate *));
static void	zskbd_bell __P((void *));
static void	zskbd_complexbell __P((void *, struct wskbd_bell_data *));

static int	zskbd_match __P((struct device *, struct cfdata *, void *));
static void	zskbd_attach __P((struct device *, struct device *, void *));

struct cfattach zskbd_ca = {
	sizeof(struct zskbd_softc), zskbd_match, zskbd_attach,
};

static int	zskbd_enable __P((void *, int));
static void	zskbd_set_leds __P((void *, int));
static int	zskbd_ioctl __P((void *, u_long, caddr_t, int, struct proc *));

const struct wskbd_accessops zskbd_accessops = {
	zskbd_enable,
	zskbd_set_leds,
	zskbd_ioctl,
};

static void	zskbd_cngetc(void *, u_int *, int *);
static void	zskbd_cnpollc(void *, int);

const struct wskbd_consops zskbd_consops = {
	zskbd_cngetc,
	zskbd_cnpollc,
};

const struct wskbd_mapdata zskbd_keymapdata = {
	zskbd_keydesctab,
#ifdef ZSKBD_LAYOUT
	ZSKBD_LAYOUT,
#else
	KB_US | KB_LK401,
#endif
};

/*
 * kbd_match: how is this zs channel configured?
 */
int
zskbd_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct zsc_attach_args *args = aux;

	/* Exact match is better than wildcard. */
	if (cf->cf_loc[ZSCCF_CHANNEL] == args->channel)
		return 2;

	/* This driver accepts wildcard. */
	if (cf->cf_loc[ZSCCF_CHANNEL] == ZSCCF_CHANNEL_DEFAULT)
		return 1;

	return 0;
}

void
zskbd_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct zsc_softc *zsc = (void *) parent;
	struct zskbd_softc *zskbd = (void *) self;
	struct zsc_attach_args *args = aux;
	struct zs_chanstate *cs;
	struct cfdata *cf;
	int channel, zskbd_unit;
	int reset, s, isconsole;
	struct zskbd_internal *zsi;
	struct wskbddev_attach_args a;

	cf = zskbd->zskbd_dev.dv_cfdata;
	zskbd_unit = zskbd->zskbd_dev.dv_unit;
	channel = args->channel;
	cs = zsc->zsc_cs[channel];
	cs->cs_private = zskbd;
	cs->cs_ops = &zsops_zskbd;

	isconsole = (args->hwflags & ZS_HWFLAG_CONSOLE);

	if (isconsole) {
		zsi = &zskbd_console_internal;
	} else {
		zsi = malloc(sizeof(struct zskbd_internal),
				       M_DEVBUF, M_NOWAIT);
		zsi->zsi_cs = cs;
		lk201_init_keystate(&zsi->zsi_ks);
	}
	zskbd->sc_itl = zsi;

	printf("\n");

	/* Initialize the speed, etc. */
	s = splzs();
	/* May need reset... */
	reset = (channel == 0) ?
		ZSWR9_A_RESET : ZSWR9_B_RESET;
	zs_write_reg(cs, 9, reset);
	/* These are OK as set by zscc: WR3, WR4, WR5 */
	/* We don't care about status or tx interrupts. */
	cs->cs_preg[1] = ZSWR1_RIE;
	(void) zs_set_speed(cs, ZSKBD_BPS);
	zs_loadchannelregs(cs);
	splx(s);

	zskbd->sc_enabled = 0;
	zskbd_lk201_init(cs);


	zskbd->kbd_type = WSKBD_TYPE_LK201;

	zskbd->leds_state = 0;

	a.console = isconsole;
	a.keymap = &zskbd_keymapdata;

	a.accessops = &zskbd_accessops;
	a.accesscookie = zskbd;

	zskbd->sc_enabled = 1;

	zskbd->sc_wskbddev = config_found(self, &a, wskbddevprint);
}

int
zskbd_cnattach(cs)
	struct zs_chanstate *cs;
{
	(void) zs_set_speed(cs, ZSKBD_BPS);
	zs_loadchannelregs(cs);

	zskbd_lk201_init(cs);

	lk201_init_keystate(&zskbd_console_internal.zsi_ks);
	zskbd_console_internal.zsi_cs = cs;

	wskbd_cnattach(&zskbd_consops, &zskbd_console_internal,
		       &zskbd_keymapdata);

	return (0);
}

int
zskbd_enable(v, on)
	void *v;
	int on;
{
	struct zskbd_softc *sc = v;

	sc->sc_enabled = on;
	return (0);
}

void
zskbd_cngetc(v, type, data)
	void *v;
	u_int *type;
	int *data;
{
	struct zskbd_internal *zsi = v;
	int c;

	do {
		c = zs_getc(zsi->zsi_cs);
	} while (!lk201_decode(&zsi->zsi_ks, c, type, data));
}

void
zskbd_cnpollc(v, on)
	void *v;
        int on;
{
#if 0
	struct zskbd_internal *zsi = v;
#endif
}

void
zskbd_set_leds(v, leds)
	void *v;
	int leds;
{
  struct zskbd_softc *sc = (struct zskbd_softc *)v;
  struct zs_chanstate *cs;
  int old_state;
  int enable_leds;

  cs = sc->sc_itl->zsi_cs;

  old_state = sc->leds_state;
  
#if 1
  if (((leds & WSKBD_LED_SCROLL) ^ (leds & WSKBD_LED_CAPS)) != 0)
    leds ^= (WSKBD_LED_SCROLL | WSKBD_LED_CAPS);
#endif

  sc->leds_state = 0;  
  if (leds & 0x01)
    sc->leds_state |= WSKBD_LED_SCROLL;
  if (leds & 0x02)
    sc->leds_state |= WSKBD_LED_NUM;
  if (leds & 0x04)
    sc->leds_state |= WSKBD_LED_CAPS;

  old_state ^= sc->leds_state;

  enable_leds = (leds | 0x80);

  zs_write_data(cs, LK_LED_DISABLE);
  DELAY(4000);
  zs_write_data(cs, (LK_LED_ALL ^ (0x0f & enable_leds)));
  DELAY(4000);

  zs_write_data(cs, LK_LED_ENABLE);
  DELAY(4000);
  zs_write_data(cs, enable_leds);
}

void
zskbd_bell(v)
	void *v;
{
  struct zskbd_softc *sc = (struct zskbd_softc *)v;
  struct zs_chanstate *cs;

  cs = sc->sc_itl->zsi_cs;

  zs_write_data(cs, LK_RING_BELL);
}

void
zskbd_complexbell(v, bell)
	void *v;
	struct wskbd_bell_data *bell;
{
  struct zskbd_softc *sc = (struct zskbd_softc *)v;
  struct zs_chanstate *cs;

  cs = sc->sc_itl->zsi_cs;

  zs_write_data(cs, LK_RING_BELL);
}

int
zskbd_ioctl(v, cmd, data, flag, p)
	void *v;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
  struct zskbd_softc *sc = (struct zskbd_softc *)v;

  switch (cmd) {
  case WSKBDIO_GTYPE:
    *(int *)data = sc->kbd_type;
    return 0;
  case WSKBDIO_SETLEDS:
    zskbd_set_leds(sc, *(int *)data);
    return 0;
  case WSKBDIO_GETLEDS:
    *(int *)data = sc->leds_state;
    return 0;
  case WSKBDIO_BELL:
    zskbd_bell(sc);
    return 0;
  case WSKBDIO_COMPLEXBELL:
    zskbd_complexbell(sc, (struct wskbd_bell_data *)data);
    return 0;
  }
  return (-1);
}

void zskbd_input(sc, data)
	struct zskbd_softc *sc;
	int data;
{
	u_int type;
	int val;

	if (sc->sc_enabled == 0)
		return;

	if (lk201_decode(&sc->sc_itl->zsi_ks, data, &type, &val))
		wskbd_input(sc->sc_wskbddev, type, val);
}

void zskbd_lk201_init(struct zs_chanstate *cs)
{
  int i;


  zs_write_data(cs, LK_LED_ENABLE);
  DELAY(4000);
  zs_write_data(cs, LK_LED_ALL);
  DELAY(4000);


#if 1
  zs_write_data(cs, LK_CL_DISABLE);
  DELAY(4000);
#else
  zs_write_data(cs, LK_CL_ENABLE);
  DELAY(4000);
  zs_write_data(cs, 0x87);
  DELAY(4000);
#endif

  for (i=1 ; i<=14 ; i++)
    {
      zs_write_data(cs, LK_CMD_MODE(LK_UPDOWN, i));
      DELAY(4000);
    }

  zs_write_data(cs, LK_BELL_ENABLE);
  DELAY(4000);
  zs_write_data(cs, 0x87);
  DELAY(4000);

  zs_write_data(cs, LK_LED_DISABLE);
  DELAY(4000);
  zs_write_data(cs, LK_LED_ALL);
  DELAY(4000);

  zs_write_data(cs, LK_KBD_ENABLE);
}


/****************************************************************
 * Interface to the lower layer (zscc)
 ****************************************************************/

static void zskbd_rxint __P((struct zs_chanstate *));
static void zskbd_txint __P((struct zs_chanstate *));
static void zskbd_stint __P((struct zs_chanstate *));
static void zskbd_softint __P((struct zs_chanstate *));

static void
zskbd_rxint(cs)
	register struct zs_chanstate *cs;
{
	register struct zskbd_softc *zskbd;
	register int put, put_next;
	register u_char c, rr1;

	zskbd = cs->cs_private;
	put = zskbd->zskbd_rbput;

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

	zskbd->zskbd_rbuf[put] = (c << 8) | rr1;
	put_next = (put + 1) & ZSKBD_RX_RING_MASK;

	/* Would overrun if increment makes (put==get). */
	if (put_next == zskbd->zskbd_rbget) {
		zskbd->zskbd_intr_flags |= INTR_RX_OVERRUN;
	} else {
		/* OK, really increment. */
		put = put_next;
	}

	/* Done reading. */
	zskbd->zskbd_rbput = put;

	/* Ask for softint() call. */
	cs->cs_softreq = 1;
}


static void
zskbd_txint(cs)
	register struct zs_chanstate *cs;
{
	register struct zskbd_softc *zskbd;

	zskbd = cs->cs_private;
	zs_write_csr(cs, ZSWR0_RESET_TXINT);
	zskbd->zskbd_intr_flags |= INTR_TX_EMPTY;
	/* Ask for softint() call. */
	cs->cs_softreq = 1;
}


static void
zskbd_stint(cs)
	register struct zs_chanstate *cs;
{
	register struct zskbd_softc *zskbd;
	register int rr0;

	zskbd = cs->cs_private;

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
	zskbd->zskbd_intr_flags |= INTR_ST_CHECK;

	/* Ask for softint() call. */
	cs->cs_softreq = 1;
}


static void
zskbd_softint(cs)
	struct zs_chanstate *cs;
{
	register struct zskbd_softc *zskbd;
	register int get, c, s;
	int intr_flags;
	register u_short ring_data;

	zskbd = cs->cs_private;

	/* Atomically get and clear flags. */
	s = splzs();
	intr_flags = zskbd->zskbd_intr_flags;
	zskbd->zskbd_intr_flags = 0;

	/* Now lower to spltty for the rest. */
	(void) spltty();

	/*
	 * Copy data from the receive ring to the event layer.
	 */
	get = zskbd->zskbd_rbget;
	while (get != zskbd->zskbd_rbput) {
		ring_data = zskbd->zskbd_rbuf[get];
		get = (get + 1) & ZSKBD_RX_RING_MASK;

		/* low byte of ring_data is rr1 */
		c = (ring_data >> 8) & 0xff;

		if (ring_data & ZSRR1_DO)
			intr_flags |= INTR_RX_OVERRUN;
		if (ring_data & (ZSRR1_FE | ZSRR1_PE)) {
#if 0 /* XXX */
			log(LOG_ERR, "%s: input error (0x%x)\n",
				zskbd->zskbd_dev.dv_xname, ring_data);
			c = -1;	/* signal input error */
#endif
		}

		/* Pass this up to the "middle" layer. */
		zskbd_input(zskbd, c);
	}
	if (intr_flags & INTR_RX_OVERRUN) {
#if 0 /* XXX */
		log(LOG_ERR, "%s: input overrun\n",
		    zskbd->zskbd_dev.dv_xname);
#endif
	}
	zskbd->zskbd_rbget = get;

	if (intr_flags & INTR_TX_EMPTY) {
		/*
		 * Transmit done.  (Not expected.)
		 */
#if 0
		log(LOG_ERR, "%s: transmit interrupt?\n",
		    zskbd->zskbd_dev.dv_xname);
#endif
	}

	if (intr_flags & INTR_ST_CHECK) {
		/*
		 * Status line change.  (Not expected.)
		 */
		log(LOG_ERR, "%s: status interrupt?\n",
		    zskbd->zskbd_dev.dv_xname);
		cs->cs_rr0_delta = 0;
	}

	splx(s);
}

struct zsops zsops_zskbd = {
	zskbd_rxint,	/* receive char available */
	zskbd_stint,	/* external/status */
	zskbd_txint,	/* xmit buffer empty */
	zskbd_softint,	/* process software interrupt */
};
