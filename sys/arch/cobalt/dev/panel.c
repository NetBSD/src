/* $NetBSD: panel.c,v 1.1 2003/01/20 01:26:14 soren Exp $ */

/*
 * Copyright (c) 2002 Dennis I. Chernoivanov
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/poll.h>
#include <sys/conf.h>
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/callout.h>
#include <sys/select.h>
#include <sys/conf.h>

#include <machine/bus.h>
#include <machine/autoconf.h>

#include <dev/ic/hd44780reg.h>
#include <dev/ic/hd44780_subr.h>
#include <dev/ic/lcdkp_subr.h>

#define PANEL_POLLRATE	(hz / 10)
#define DATA_OFFSET	0x10

struct panel_softc {
	struct device sc_dev;

	struct hd44780_chip sc_lcd;
	struct lcdkp_chip sc_kp;

	struct selinfo sc_selq;
	struct callout sc_callout;
};

static int	panel_match(struct device *, struct cfdata *, void *);
static void	panel_attach(struct device *, struct device *, void *);

static void	panel_soft(void *);

static void	panel_cbt_rwrite(bus_space_tag_t, bus_space_handle_t, u_int8_t);
static u_int8_t	panel_cbt_rread(bus_space_tag_t, bus_space_handle_t);

dev_type_open(panelopen);
dev_type_close(panelclose);
dev_type_read(panelread);
dev_type_write(panelwrite);
dev_type_ioctl(panelioctl);
dev_type_poll(panelpoll);

const struct cdevsw panel_cdevsw = {
	panelopen, panelclose, panelread, panelwrite, panelioctl,
	nostop, notty, panelpoll, nommap,
};

extern struct cfdriver panel_cd;

CFATTACH_DECL(panel, sizeof(struct panel_softc),
    panel_match, panel_attach, NULL, NULL);

static int
panel_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	return 1;
}

static void
panel_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct panel_softc *sc = (void *)self;
	struct mainbus_attach_args *maa = aux;

	static struct lcdkp_xlate keys[] = {
		{ 0xfa, 'h' },
		{ 0xf6, 'k' },
		{ 0xde, 'l' },
		{ 0xee, 'j' },
		{ 0x7e, 's' },
		{ 0xbe, 'e' }
	};

	sc->sc_lcd.sc_iot = maa->ma_iot;
	sc->sc_lcd.sc_ioir = maa->ma_ioh;
	bus_space_subregion(sc->sc_lcd.sc_iot, sc->sc_lcd.sc_ioir, DATA_OFFSET,
			    1, &sc->sc_lcd.sc_iodr);

	sc->sc_lcd.sc_dev_ok = 1;
	sc->sc_lcd.sc_rows = 16;
	sc->sc_lcd.sc_vrows = 40;
	sc->sc_lcd.sc_flags = HD_8BIT | HD_MULTILINE | HD_KEYPAD;

	sc->sc_lcd.sc_rwrite = panel_cbt_rwrite;
	sc->sc_lcd.sc_rread = panel_cbt_rread;

	hd44780_attach_subr(&sc->sc_lcd);

	sc->sc_kp.sc_iot = maa->ma_iot;
	sc->sc_kp.sc_ioh = MIPS_PHYS_TO_KSEG1(0x1d000000); /* XXX */

	sc->sc_kp.sc_knum = sizeof(keys) / sizeof(struct lcdkp_xlate);
	sc->sc_kp.sc_kpad = keys;
	sc->sc_kp.sc_rread = panel_cbt_rread;

	lcdkp_attach_subr(&sc->sc_kp);

	callout_init(&sc->sc_callout);

	printf("\n");
}

static void
panel_cbt_rwrite(iot, ioh, cmd)
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	u_int8_t cmd;
{
	bus_space_write_4(iot, ioh, 0x00, cmd << 24);
	delay(TIMEOUT_NORMAL);
}

static u_int8_t
panel_cbt_rread(iot, ioh)
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
{
	delay(TIMEOUT_NORMAL);
	return (bus_space_read_4(iot, ioh, 0x00) >> 24) & 0xff;
}

int
panelopen(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	struct panel_softc *sc = device_lookup(&panel_cd, minor(dev));
	return ((sc->sc_lcd.sc_dev_ok == 0) ? ENXIO : 0);
}

int
panelclose(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	struct panel_softc *sc = device_lookup(&panel_cd, minor(dev));
	selwakeup(&sc->sc_selq);
	return 0;
}

int
panelread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	int error;
	u_int8_t b;
	struct panel_softc *sc = device_lookup(&panel_cd, minor(dev));
 
	if (uio->uio_resid < sizeof(b))
		return EIO;

	if ((error = lcdkp_readkey(&sc->sc_kp, &b)) != 0)
		return error;

	return uiomove((void*)&b, sizeof(b), uio);
}

int
panelwrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	int error;
	struct hd44780_io io;
	struct panel_softc *sc = device_lookup(&panel_cd, minor(dev));

	io.dat = 0;
	io.len = uio->uio_resid;
	if (io.len > HD_MAX_CHARS)
		io.len = HD_MAX_CHARS;

	if ((error = uiomove((void*)io.buf, io.len, uio)) != 0)
		return error;

	hd44780_ddram_redraw(&sc->sc_lcd, &io);
	return 0;
}

int
panelioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	struct panel_softc *sc = device_lookup(&panel_cd, minor(dev));
	return hd44780_ioctl_subr(&sc->sc_lcd, cmd, data);
}

int
panelpoll(dev, events, p)
	dev_t dev;
	int events;
	struct proc *p;
{
	int revents = 0;

	if ((events & (POLLIN | POLLRDNORM)) != 0) {
		struct panel_softc *sc = device_lookup(&panel_cd, minor(dev));

		if (lcdkp_scankey(&sc->sc_kp) != 0) {
			revents = events & (POLLIN | POLLRDNORM);
		} else {
			selrecord(p, &sc->sc_selq);
			callout_reset(&sc->sc_callout, PANEL_POLLRATE,
					panel_soft, sc);
		}
	}

	return revents;
}

static void
panel_soft(arg)
	void *arg;
{
	struct panel_softc *sc = arg;
	if (lcdkp_scankey(&sc->sc_kp) != 0)
		selwakeup(&sc->sc_selq);
	else
		callout_reset(&sc->sc_callout, PANEL_POLLRATE, panel_soft, sc);
}
