/*	$NetBSD: zs_ms.c,v 1.5.4.1 2007/03/12 05:50:10 rmind Exp $	*/

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
 * IP12/IP20 serial mouse driver attached to zs channel 1 at 4800bps.
 * This layer feeds wsmouse.
 *
 * 5 byte packets: sync, x1, y1, x2, y2
 * sync format: binary 10000LMR (left, middle, right) 0 is down
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: zs_ms.c,v 1.5.4.1 2007/03/12 05:50:10 rmind Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsmousevar.h>

#include <dev/ic/z8530reg.h>
#include <machine/machtype.h>
#include <machine/z8530var.h>

#define ZSMS_BAUD	4800
#define ZSMS_RXQ_LEN	64	/* power of 2 */

/* protocol */
#define ZSMS_SYNC		0x80
#define ZSMS_SYNC_MASK		0xf8
#define ZSMS_SYNC_BTN_R		0x01
#define ZSMS_SYNC_BTN_M		0x02
#define ZSMS_SYNC_BTN_L		0x04
#define ZSMS_SYNC_BTN_MASK	0x07

struct zsms_softc {
	struct device	sc_dev;

	/* tail-chasing fifo */
	u_char		rxq[ZSMS_RXQ_LEN];
	u_char		rxq_head;
	u_char		rxq_tail;

	/* 5-byte packet as described above */
#define	ZSMS_PACKET_SYNC	0
#define	ZSMS_PACKET_X1		1
#define	ZSMS_PACKET_Y1		2
#define	ZSMS_PACKET_X2		3
#define	ZSMS_PACKET_Y2		4
	int8_t		packet[5];

#define ZSMS_STATE_SYNC	0x01
#define ZSMS_STATE_X1	0x02
#define ZSMS_STATE_Y1	0x04
#define ZSMS_STATE_X2	0x08
#define ZSMS_STATE_Y2	0x10
	u_char		state;

	/* wsmouse bits */
	int		enabled;
	struct device  *wsmousedev;
};

static int	zsms_match(struct device *, struct cfdata *, void *);
static void	zsms_attach(struct device *, struct device *, void *);
static void	zsms_rxint(struct zs_chanstate *);
static void	zsms_txint(struct zs_chanstate *);
static void	zsms_stint(struct zs_chanstate *, int);
static void	zsms_softint(struct zs_chanstate *);

static void	zsms_wsmouse_input(struct zsms_softc *);
static int	zsms_wsmouse_enable(void *);
static void	zsms_wsmouse_disable(void *);
static int	zsms_wsmouse_ioctl(void *, u_long, void *, int,
						   struct lwp *);

CFATTACH_DECL(zsms, sizeof(struct zsms_softc),
	      zsms_match, zsms_attach, NULL, NULL);

static struct zsops zsms_zsops = {
	zsms_rxint,
	zsms_stint,
	zsms_txint,
	zsms_softint
};

static const struct wsmouse_accessops zsms_wsmouse_accessops = {
	zsms_wsmouse_enable,
	zsms_wsmouse_ioctl,
	zsms_wsmouse_disable
};

int
zsms_match(struct device *parent, struct cfdata *cf, void *aux)
{
	if (mach_type == MACH_SGI_IP12 || mach_type == MACH_SGI_IP20) {
		struct zsc_attach_args *args = aux;

		if (args->channel == 1)
			return (1);
	}

	return (0);
}

void
zsms_attach(struct device *parent, struct device *self, void *aux)
{
	int				s, channel;
	struct zsc_softc	       *zsc = (struct zsc_softc *)parent;
	struct zsms_softc	       *sc = (struct zsms_softc *)self;
	struct zsc_attach_args	       *args = aux;
	struct zs_chanstate	       *cs;
	struct wsmousedev_attach_args	wsmaa;

	/* Establish ourself with the MD z8530 driver */
	channel = args->channel;
	cs = zsc->zsc_cs[channel];
	cs->cs_ops = &zsms_zsops;
	cs->cs_private = sc;

	sc->enabled = 0;
	sc->rxq_head = 0;
	sc->rxq_tail = 0;
	sc->state = ZSMS_STATE_SYNC;

	printf(": baud rate %d\n", ZSMS_BAUD);

	s = splzs();
	zs_write_reg(cs, 9, (channel == 0) ? ZSWR9_A_RESET : ZSWR9_B_RESET);
	cs->cs_preg[1] = ZSWR1_RIE;
	zs_set_speed(cs, ZSMS_BAUD);
	zs_loadchannelregs(cs);
	splx(s);

	/* attach wsmouse */
	wsmaa.accessops =	&zsms_wsmouse_accessops;
	wsmaa.accesscookie =	sc;
	sc->wsmousedev =	config_found(self, &wsmaa, wsmousedevprint);
}

void
zsms_rxint(struct zs_chanstate *cs)
{
	u_char c, r;
	struct zsms_softc *sc = (struct zsms_softc *)cs->cs_private;

	/* clear errors */
	r = zs_read_reg(cs, 1);
	if (r & (ZSRR1_FE | ZSRR1_DO | ZSRR1_PE))
		zs_write_csr(cs, ZSWR0_RESET_ERRORS);

	/* read byte and append to our queue */
	c = zs_read_data(cs);

	sc->rxq[sc->rxq_tail] = c;
	sc->rxq_tail = (sc->rxq_tail + 1) & ~ZSMS_RXQ_LEN;

	cs->cs_softreq = 1;
}

/* We should never get here. */
void
zsms_txint(struct zs_chanstate *cs)
{
	zs_write_reg(cs, 0, ZSWR0_RESET_TXINT);

	/* seems like the in thing to do */
	cs->cs_softreq = 1;
}

void
zsms_stint(struct zs_chanstate *cs, int force)
{
	zs_write_csr(cs, ZSWR0_RESET_STATUS);
	cs->cs_softreq = 1;
}

void
zsms_softint(struct zs_chanstate *cs)
{
	struct zsms_softc *sc = (struct zsms_softc *)cs->cs_private;

	/* No need to keep score if nobody is listening */
	if (!sc->enabled) {
		sc->rxq_head = sc->rxq_tail;
		return;
	}

	/*
	 * Here's the real action. Read a full packet and
	 * then let wsmouse know what has happened.
	 */
	while (sc->rxq_head != sc->rxq_tail) {
		int8_t c = sc->rxq[sc->rxq_head];

		switch (sc->state) {
		case ZSMS_STATE_SYNC:
			if ((c & ZSMS_SYNC_MASK) == ZSMS_SYNC) {
				sc->packet[ZSMS_PACKET_SYNC] = c;
				sc->state = ZSMS_STATE_X1;
			}
			break;

		case ZSMS_STATE_X1:
			sc->packet[ZSMS_PACKET_X1] = c;
			sc->state = ZSMS_STATE_Y1;
			break;

		case ZSMS_STATE_Y1:
			sc->packet[ZSMS_PACKET_Y1] = c;
			sc->state = ZSMS_STATE_X2;
			break;

		case ZSMS_STATE_X2:
			sc->packet[ZSMS_PACKET_X2] = c;
			sc->state = ZSMS_STATE_Y2;
			break;

		case ZSMS_STATE_Y2:
			sc->packet[ZSMS_PACKET_Y2] = c;

			/* tweak wsmouse */
			zsms_wsmouse_input(sc);

			sc->state = ZSMS_STATE_SYNC;
		}

		sc->rxq_head = (sc->rxq_head + 1) & ~ZSMS_RXQ_LEN;
	}
}

/******************************************************************************
 * wsmouse glue
 ******************************************************************************/

static void
zsms_wsmouse_input(struct zsms_softc *sc)
{
	u_int	btns;
	int bl, bm, br;
	int	x, y;

	btns = (uint8_t)sc->packet[ZSMS_PACKET_SYNC] & ZSMS_SYNC_BTN_MASK;

	bl = (btns & ZSMS_SYNC_BTN_L) == 0;
	bm = (btns & ZSMS_SYNC_BTN_M) == 0;
	br = (btns & ZSMS_SYNC_BTN_R) == 0;

	/* for wsmouse(4), 1 is down, 0 is up, the most left button is LSB */
	btns = (bl ? (1 << 0) : 0) | (bm ? (1 << 1) : 0) | (br ? (1 << 2) : 0);

	x = (int)sc->packet[ZSMS_PACKET_X1] + (int)sc->packet[ZSMS_PACKET_X2];
	y = (int)sc->packet[ZSMS_PACKET_Y1] + (int)sc->packet[ZSMS_PACKET_Y2];

	wsmouse_input(sc->wsmousedev, btns, x, y, 0, 0, WSMOUSE_INPUT_DELTA);
}

static int
zsms_wsmouse_enable(void *cookie)
{
	struct zsms_softc *sc = (struct zsms_softc *)cookie;

	if (sc->enabled)
		return (EBUSY);

	sc->state = ZSMS_STATE_SYNC;
	sc->enabled = 1;

	return (0);
}

void
zsms_wsmouse_disable(void *cookie)
{
	struct zsms_softc *sc = (struct zsms_softc *)cookie;

	sc->enabled = 0;
}

static int
zsms_wsmouse_ioctl(void *cookie, u_long cmd,
		   void *data, int flag, struct lwp *l)
{
	switch (cmd) {
	case WSMOUSEIO_GTYPE:
		*(u_int *)data = WSMOUSE_TYPE_SGI;
		break;

#ifdef notyet
	case WSMOUSEIO_SRES:
	case WSMOUSEIO_SSCALE:
	case WSMOUSEIO_SRATE:
	case WSMOUSEIO_SCALIBCOORDS:
	case WSMOUSEIO_GCALIBCOORDS:
	case WSMOUSEIO_GETID:
#endif

	default:
		return (EPASSTHROUGH);
	}

	return (0);
}
