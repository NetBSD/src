/*	$NetBSD: zs_kbd.c,v 1.6.26.1 2007/03/12 05:50:10 rmind Exp $	*/

/*
 * Copyright (c) 2004 Steve Rumble
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

/*
 * IP12/IP20 serial keyboard driver attached to zs channel 0 at 600bps.
 * This layer is the parent of wskbd.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: zs_kbd.c,v 1.6.26.1 2007/03/12 05:50:10 rmind Exp $");

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wskbdvar.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsksymvar.h>

#include <dev/ic/z8530reg.h>
#include <machine/machtype.h>
#include <machine/z8530var.h>

#define ZSKBD_BAUD	600
#define ZSKBD_TXQ_LEN	16		/* power of 2 */
#define ZSKBD_RXQ_LEN	64		/* power of 2 */

#define ZSKBD_DIP_SYNC	0x6E
#define ZSKBD_KEY_UP	0x80

#ifdef ZSKBD_DEBUG
int zskbd_debug = 0;

#define DPRINTF(_x) if (zskbd_debug) printf _x
#else
#define DPRINTF(_x)
#endif

struct zskbd_softc {
	struct device   	sc_dev;

	struct zskbd_devconfig *sc_dc;
};

struct zskbd_devconfig {
	/* transmit tail-chasing fifo */
	u_char		txq[ZSKBD_TXQ_LEN];
	u_int		txq_head;
	u_int		txq_tail;

	/* receive tail-chasing fifo */
	u_char		rxq[ZSKBD_RXQ_LEN];
	u_int		rxq_head;
	u_int		rxq_tail;

	/* state */
#define TX_READY	0x1
#define RX_DIP		0x2
	u_int		state;

	/* keyboard configuration */
#define ZSKBD_CTRL_A		0x0
#define ZSKBD_CTRL_A_SBEEP	0x2	/* 200 ms */
#define ZSKBD_CTRL_A_LBEEP	0x4	/* 1000 ms */
#define ZSKBD_CTRL_A_NOCLICK	0x8	/* turn off keyboard click */
#define ZSKBD_CTRL_A_RCB	0x10	/* request config byte */
#define ZSKBD_CTRL_A_NUMLK	0x20	/* num lock led */
#define ZSKBD_CTRL_A_CAPSLK	0x40	/* caps lock led */
#define ZSKBD_CTRL_A_AUTOREP	0x80	/* auto-repeat after 650 ms, 28x/sec */

#define ZSKBD_CTRL_B		0x1
#define ZSKBD_CTRL_B_CMPL_DS1_2	0x2	/* complement of ds1+ds2 (num+capslk) */
#define ZSKBD_CTRL_B_SCRLK	0x4	/* scroll lock light */
#define ZSKBD_CTRL_B_L1		0x8	/* user-configurable lights */
#define ZSKBD_CTRL_B_L2		0x10
#define ZSKBD_CTRL_B_L3		0x20
#define ZSKBD_CTRL_B_L4		0x40
	u_char		kbd_conf[2];

	/* dip switch settings */
	u_char		dip;

	/* wscons glue */
	struct device  *wskbddev;
	int		enabled;
};

static int	zskbd_match(struct device *, struct cfdata *, void *);
static void	zskbd_attach(struct device *, struct device *, void *);
static void	zskbd_rxint(struct zs_chanstate *);
static void	zskbd_stint(struct zs_chanstate *, int);
static void	zskbd_txint(struct zs_chanstate *);
static void	zskbd_softint(struct zs_chanstate *);
static void	zskbd_send(struct zs_chanstate *, u_char *, u_int);
static void	zskbd_ctrl(struct zs_chanstate *, u_char, u_char,
						  u_char, u_char);

static void	zskbd_wskbd_input(struct zs_chanstate *, u_char);
static int	zskbd_wskbd_enable(void *, int);
static void	zskbd_wskbd_set_leds(void *, int);
static int	zskbd_wskbd_get_leds(void *);
static void	zskbd_wskbd_set_keyclick(void *, int);
static int	zskbd_wskbd_get_keyclick(void *);
static int	zskbd_wskbd_ioctl(void *, u_long, void *, int, struct lwp *);

void		zskbd_cnattach(int, int);
static void	zskbd_wskbd_getc(void *, u_int *, int *);
static void	zskbd_wskbd_pollc(void *, int);
static void	zskbd_wskbd_bell(void *, u_int, u_int, u_int);

extern struct zschan   *zs_get_chan_addr(int, int);
extern int		zs_getc(void *);
extern void		zs_putc(void *, int);

CFATTACH_DECL(zskbd, sizeof(struct zskbd_softc),
	      zskbd_match, zskbd_attach, NULL, NULL);

static struct zsops zskbd_zsops = {
	zskbd_rxint,
	zskbd_stint,
	zskbd_txint,
	zskbd_softint
};

extern const struct wscons_keydesc wssgi_keydesctab[];
const struct wskbd_mapdata sgikbd_wskbd_keymapdata = {
	wssgi_keydesctab, KB_US
};

const struct wskbd_accessops zskbd_wskbd_accessops = {
	zskbd_wskbd_enable,
	zskbd_wskbd_set_leds,
	zskbd_wskbd_ioctl
};

const struct wskbd_consops zskbd_wskbd_consops = {
	zskbd_wskbd_getc,
	zskbd_wskbd_pollc,
	zskbd_wskbd_bell
};

static struct zskbd_devconfig	zskbd_console_dc;
static int			zskbd_is_console = 0;

static int
zskbd_match(struct device *parent, struct cfdata *cf, void *aux)
{

	if (mach_type == MACH_SGI_IP12 || mach_type == MACH_SGI_IP20) {
		struct zsc_attach_args *args = aux;

		if (args->channel == 0)
			return (1);
	}

	return (0);
}

static void
zskbd_attach(struct device *parent, struct device *self, void *aux)
{
	struct zskbd_softc	       *sc;
	struct zs_chanstate	       *cs;
	struct zsc_softc      	       *zsc;
	struct zsc_attach_args	       *args;
	struct wskbddev_attach_args	wskaa;
	int				s, channel;

	zsc = (struct zsc_softc *)parent;
	sc = (struct zskbd_softc *)self;
	args = (struct zsc_attach_args *)aux;

	/* Establish ourself with the MD z8530 driver */
	channel = args->channel;
	cs = zsc->zsc_cs[channel];
	cs->cs_ops = &zskbd_zsops;
	cs->cs_private = sc;

	if (zskbd_is_console) {
		sc->sc_dc = &zskbd_console_dc;
		wskaa.console = 1;
		sc->sc_dc->enabled = 1;
	} else {
		wskaa.console = 0;

		sc->sc_dc = malloc(sizeof(struct zskbd_devconfig), M_DEVBUF,
		    M_WAITOK);
		if (sc->sc_dc == NULL)
			panic("zskbd out of memory");

		sc->sc_dc->enabled = 0;
	}

	sc->sc_dc->txq_head = 0;
	sc->sc_dc->txq_tail = 0;
	sc->sc_dc->rxq_head = 0;
	sc->sc_dc->rxq_tail = 0;
	sc->sc_dc->state = TX_READY;
	sc->sc_dc->dip = 0;
	sc->sc_dc->kbd_conf[ZSKBD_CTRL_A] = 0;
	sc->sc_dc->kbd_conf[ZSKBD_CTRL_B] = 0;

	printf(": baud rate %d\n", ZSKBD_BAUD);

	s = splzs();
	zs_write_reg(cs, 9, (channel == 0) ? ZSWR9_A_RESET : ZSWR9_B_RESET);
	cs->cs_preg[1] = ZSWR1_RIE | ZSWR1_TIE;
	cs->cs_preg[4] = (cs->cs_preg[4] & ZSWR4_CLK_MASK) |
			 (ZSWR4_ONESB | ZSWR4_PARENB);	/* 1 stop, odd parity */
	zs_set_speed(cs, ZSKBD_BAUD);
	zs_loadchannelregs(cs);

	/* request DIP switch settings just in case */
	zskbd_ctrl(cs, ZSKBD_CTRL_A_RCB, 0, 0, 0);

	splx(s);

	/* attach wskbd */
	wskaa.keymap =		&sgikbd_wskbd_keymapdata;
	wskaa.accessops =	&zskbd_wskbd_accessops;
	wskaa.accesscookie =	cs;
	sc->sc_dc->wskbddev =	config_found(self, &wskaa, wskbddevprint);
}

static void
zskbd_rxint(struct zs_chanstate *cs)
{
	struct zskbd_softc     *sc;
	struct zskbd_devconfig *dc;
	u_char			c, r;

	sc = (struct zskbd_softc *)cs->cs_private;
	dc = sc->sc_dc;

	/* clear errors */
	r = zs_read_reg(cs, 1);
	if (r & (ZSRR1_FE | ZSRR1_DO | ZSRR1_PE))
		zs_write_csr(cs, ZSWR0_RESET_ERRORS);

	/* read byte and append to our queue */
	c = zs_read_data(cs);

	dc->rxq[dc->rxq_tail] = c;
	dc->rxq_tail = (dc->rxq_tail + 1) & ~ZSKBD_RXQ_LEN;

	cs->cs_softreq = 1;
}

static void
zskbd_stint(struct zs_chanstate *cs, int force)
{

	zs_write_csr(cs, ZSWR0_RESET_STATUS);
	cs->cs_softreq = 1;
}

static void
zskbd_txint(struct zs_chanstate *cs)
{
	struct zskbd_softc *sc;

	sc = (struct zskbd_softc *)cs->cs_private;
	zs_write_reg(cs, 0, ZSWR0_RESET_TXINT);
	sc->sc_dc->state |= TX_READY;
	cs->cs_softreq = 1;
}

static void
zskbd_softint(struct zs_chanstate *cs)
{
	struct zskbd_softc	*sc;
	struct zskbd_devconfig	*dc;

	sc = (struct zskbd_softc *)cs->cs_private;
	dc = sc->sc_dc;

	/* handle pending transmissions */
	if (dc->txq_head != dc->txq_tail && (dc->state & TX_READY)) {
		int s;

		dc->state &= ~TX_READY;

		s = splzs();
		zs_write_data(cs, dc->txq[dc->txq_head]);
		splx(s);

		dc->txq_head = (dc->txq_head + 1) & ~ZSKBD_TXQ_LEN;
	}

	/* don't bother if nobody is listening */
	if (!dc->enabled) {
		dc->rxq_head = dc->rxq_tail;
		return;
	}

	/* handle incoming keystrokes/config */
	while (dc->rxq_head != dc->rxq_tail) {
		u_char key = dc->rxq[dc->rxq_head];

		if (dc->state & RX_DIP) {
			dc->dip = key;
			dc->state &= ~RX_DIP;
		} else if (key == ZSKBD_DIP_SYNC) {
			dc->state |= RX_DIP;
		} else {
			/* toss wskbd a bone */
			zskbd_wskbd_input(cs, key);
		}

		dc->rxq_head = (dc->rxq_head + 1) & ~ZSKBD_RXQ_LEN;
	}
}

/* expects to be in splzs() */
static void
zskbd_send(struct zs_chanstate *cs, u_char *c, u_int len)
{
	u_int			i;
	struct zskbd_softc     *sc;
	struct zskbd_devconfig *dc;

	sc = (struct zskbd_softc *)cs->cs_private;
	dc = sc->sc_dc;

	for (i = 0; i < len; i++) {
		if (dc->state & TX_READY) {
			zs_write_data(cs, c[i]);
			dc->state &= ~TX_READY;
		} else {
			dc->txq[dc->txq_tail] = c[i];
			dc->txq_tail = (dc->txq_tail + 1) & ~ZSKBD_TXQ_LEN;
			cs->cs_softreq = 1;
		}
	}
}

/* expects to be in splzs() */
static void
zskbd_ctrl(struct zs_chanstate *cs, u_char a_on, u_char a_off,
	   u_char b_on, u_char b_off)
{
	struct zskbd_softc	*sc;
	struct zskbd_devconfig	*dc;

	sc = (struct zskbd_softc *)cs->cs_private;
	dc = sc->sc_dc;

	dc->kbd_conf[ZSKBD_CTRL_A] |=   a_on;
	dc->kbd_conf[ZSKBD_CTRL_A] &= ~(a_off | ZSKBD_CTRL_B);
	dc->kbd_conf[ZSKBD_CTRL_B] &=  ~b_off;
	dc->kbd_conf[ZSKBD_CTRL_B] |=  (b_on | ZSKBD_CTRL_B);

	zskbd_send(cs, dc->kbd_conf, 2);

	/* make sure we don't resend these each time */
	dc->kbd_conf[ZSKBD_CTRL_A] &= ~(ZSKBD_CTRL_A_RCB | ZSKBD_CTRL_A_SBEEP |
	    ZSKBD_CTRL_A_LBEEP);
}

/******************************************************************************
 * wskbd glue
 ******************************************************************************/

static void
zskbd_wskbd_input(struct zs_chanstate *cs, u_char key)
{
	struct zskbd_softc     *sc;
	u_int			type;

	sc = (struct zskbd_softc *)cs->cs_private;

	if (key & ZSKBD_KEY_UP)
		type = WSCONS_EVENT_KEY_UP;
	else
		type = WSCONS_EVENT_KEY_DOWN;

	wskbd_input(sc->sc_dc->wskbddev, type, (key & ~ZSKBD_KEY_UP));

	DPRINTF(("zskbd_wskbd_input: inputted key 0x%x\n", key));

#ifdef WSDISPLAY_COMPAT_RAWKBD
	wskbd_rawinput(sc->sc_dc->wskbddev, &key, 1);
#endif
}

static int
zskbd_wskbd_enable(void *cookie, int on)
{
	struct zskbd_softc *sc;

	sc = (struct zskbd_softc *)((struct zs_chanstate *)cookie)->cs_private;

	if (on) {
		if (sc->sc_dc->enabled)
			return (EBUSY);
		else
			sc->sc_dc->enabled = 1;
	} else
		sc->sc_dc->enabled = 0;

	DPRINTF(("zskbd_wskbd_enable: %s\n", on ? "enabled" : "disabled"));

	return (0);
}

static void
zskbd_wskbd_set_leds(void *cookie, int leds)
{
	int 	s;
	u_char	a_on, a_off, b_on, b_off;

	a_on = a_off = b_on = b_off = 0;

	if (leds & WSKBD_LED_CAPS)
		a_on |=  ZSKBD_CTRL_A_CAPSLK;
	else
		a_off |= ZSKBD_CTRL_A_CAPSLK;

	if (leds & WSKBD_LED_NUM)
		a_on |=  ZSKBD_CTRL_A_NUMLK;
	else
		a_off |= ZSKBD_CTRL_A_NUMLK;

	if (leds & WSKBD_LED_SCROLL)
		b_on |=  ZSKBD_CTRL_B_SCRLK;
	else
		b_off |= ZSKBD_CTRL_B_SCRLK;

	s = splzs();
	zskbd_ctrl((struct zs_chanstate *)cookie, a_on, a_off, b_on, b_off);
	splx(s);
}

static int
zskbd_wskbd_get_leds(void *cookie)
{
	struct zskbd_softc     *sc;
	int		 	leds;

	sc = (struct zskbd_softc *)((struct zs_chanstate *)cookie)->cs_private;
	leds = 0;

	if (sc->sc_dc->kbd_conf[ZSKBD_CTRL_A] & ZSKBD_CTRL_A_NUMLK)
		leds |= WSKBD_LED_NUM;

	if (sc->sc_dc->kbd_conf[ZSKBD_CTRL_A] & ZSKBD_CTRL_A_CAPSLK)
		leds |= WSKBD_LED_CAPS;

	if (sc->sc_dc->kbd_conf[ZSKBD_CTRL_B] & ZSKBD_CTRL_B_SCRLK)
		leds |= WSKBD_LED_SCROLL;

	return (leds);
}

static void
zskbd_wskbd_set_keyclick(void *cookie, int on)
{
	int			s;
	struct zs_chanstate    *cs;

	cs = (struct zs_chanstate *)cookie;

	if (on) {
		if (!zskbd_wskbd_get_keyclick(cookie)) {
			s = splzs();
			zskbd_ctrl(cs, 0, ZSKBD_CTRL_A_NOCLICK, 0, 0);
			splx(s);
		}
	} else {
		if (zskbd_wskbd_get_keyclick(cookie)) {
			s = splzs();
			zskbd_ctrl(cs, ZSKBD_CTRL_A_NOCLICK, 0, 0, 0);
			splx(s);
		}
	}
}

static int
zskbd_wskbd_get_keyclick(void *cookie)
{
	struct zskbd_softc *sc;

	sc = (struct zskbd_softc *)((struct zs_chanstate *)cookie)->cs_private;

	if (sc->sc_dc->kbd_conf[ZSKBD_CTRL_A] & ZSKBD_CTRL_A_NOCLICK)
		return (0);
	else
		return (1);
}

static int
zskbd_wskbd_ioctl(void *cookie, u_long cmd,
		  void *data, int flag, struct lwp *l)
{

	switch (cmd) {
	case WSKBDIO_GTYPE:
		*(int *)data = WSKBD_TYPE_SGI;
		break;

#ifdef notyet
	case WSKBDIO_BELL:
	case WSKBDIO_COMPLEXBELL:
	case WSKBDIO_SETBELL:
	case WSKBDIO_GETBELL:
	case WSKBDIO_SETDEFAULTBELL:
	case WSKBDIO_GETDEFAULTBELL:
	case WSKBDIO_SETKEYREPEAT:
	case WSKBDIO_GETKEYREPEAT:
	case WSKBDIO_SETDEFAULTKEYREPEAT:
	case WSKBDIO_GETDEFAULTKEYREPEAT:
#endif

	case WSKBDIO_SETLEDS:
		zskbd_wskbd_set_leds(cookie, *(int *)data);
		break;

	case WSKBDIO_GETLEDS:
		*(int *)data = zskbd_wskbd_get_leds(cookie);
		break;

#ifdef notyet
	case WSKBDIO_GETMAP:
	case WSKBDIO_SETMAP:
	case WSKBDIO_GETENCODING:
	case WSKBDIO_SETENCODING:
	case WSKBDIO_SETMODE:
	case WSKBDIO_GETMODE:
#endif

	case WSKBDIO_SETKEYCLICK:
		zskbd_wskbd_set_keyclick(cookie, *(int *)data);
		break;

	case WSKBDIO_GETKEYCLICK:
		*(int *)data = zskbd_wskbd_get_keyclick(cookie);
		break;

	default:
		return (EPASSTHROUGH);
	}

	return (0);
}

/*
 * console routines
 */
void
zskbd_cnattach(int zsunit, int zschan)
{

	wskbd_cnattach(&zskbd_wskbd_consops, zs_get_chan_addr(zsunit, zschan),
	    &sgikbd_wskbd_keymapdata);
	zskbd_is_console = 1;
}

static void
zskbd_wskbd_getc(void *cookie, u_int *type, int *data)
{
	int key;

	key = zs_getc(cookie);

	if (key & ZSKBD_KEY_UP)
		*type = WSCONS_EVENT_KEY_UP;
	else
		*type = WSCONS_EVENT_KEY_DOWN;

	*data = key & ~ZSKBD_KEY_UP;
}

static void
zskbd_wskbd_pollc(void *cookie, int on)
{
}

static void
zskbd_wskbd_bell(void *cookie, u_int pitch, u_int period, u_int volume)
{

	/*
	 * Since we don't have any state, this'll nuke our lights,
	 * key click, and other bits in ZSKBD_CTRL_A.
	 */
	if (period >= 1000)
		zs_putc(cookie, ZSKBD_CTRL_A_LBEEP);
	else
		zs_putc(cookie, ZSKBD_CTRL_A_SBEEP);
}
