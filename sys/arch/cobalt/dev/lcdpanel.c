/* $NetBSD: lcdpanel.c,v 1.1 2018/04/09 20:16:16 christos Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: lcdpanel.c,v 1.1 2018/04/09 20:16:16 christos Exp $");

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
#include <sys/reboot.h>

#include <sys/bus.h>
#include <machine/autoconf.h>

#include <dev/ic/hd44780reg.h>
#include <dev/ic/hd44780var.h>
#include <dev/ic/lcdkp_subr.h>

#include "ioconf.h"

#define LCDPANEL_POLLRATE	(hz / 10)
#define LCDPANEL_REGION	0x20
#define DATA_OFFSET	0x10
#define LCDPANEL_COLS	16
#define LCDPANEL_VCOLS	40
#define LCDPANEL_ROWS	2

struct lcdpanel_softc {
	device_t sc_dev;

	struct hd44780_chip sc_lcd;
	struct lcdkp_chip sc_kp;

	struct selinfo sc_selq;
	struct callout sc_callout;
};

struct lcd_message {
	const char firstcol[LCDPANEL_VCOLS];
	const char secondcol[LCDPANEL_VCOLS];
};
static const struct lcd_message startup_message = {
	"NetBSD/cobalt   ",
	"Starting up...  "
};
static const struct lcd_message halt_message = {
	"NetBSD/cobalt   ",
	"Halting...      "
};
static const struct lcd_message reboot_message = {
	"NetBSD/cobalt   ",
	"Rebooting...    "
};

static int	lcdpanel_match(device_t, cfdata_t, void *);
static void	lcdpanel_attach(device_t, device_t, void *);
static bool	lcdpanel_shutdown(device_t, int);

static void	lcdpanel_soft(void *);

static uint8_t	lcdpanel_cbt_kprread(bus_space_tag_t, bus_space_handle_t);
static uint8_t	lcdpanel_cbt_hdreadreg(struct hd44780_chip *, uint32_t, uint32_t);
static void	lcdpanel_cbt_hdwritereg(struct hd44780_chip *, uint32_t, uint32_t,
    uint8_t);

dev_type_open(lcdpanelopen);
dev_type_close(lcdpanelclose);
dev_type_read(lcdpanelread);
dev_type_write(lcdpanelwrite);
dev_type_ioctl(lcdpanelioctl);
dev_type_poll(lcdpanelpoll);

const struct cdevsw lcdpanel_cdevsw = {
	.d_open = lcdpanelopen,
	.d_close = lcdpanelclose,
	.d_read = lcdpanelread,
	.d_write = lcdpanelwrite,
	.d_ioctl = lcdpanelioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = lcdpanelpoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = 0
};

CFATTACH_DECL_NEW(lcdpanel, sizeof(struct lcdpanel_softc),
    lcdpanel_match, lcdpanel_attach, NULL, NULL);

static int
lcdpanel_match(device_t parent, cfdata_t cf, void *aux)
{

	return 1;
}

static void
lcdpanel_attach(device_t parent, device_t self, void *aux)
{
	struct lcdpanel_softc *sc = device_private(self);
	struct mainbus_attach_args *maa = aux;
	struct hd44780_io io;
	static struct lcdkp_xlate keys[] = {
		{ 0xfa, 'h' },
		{ 0xf6, 'k' },
		{ 0xde, 'l' },
		{ 0xee, 'j' },
		{ 0x7e, 's' },
		{ 0xbe, 'e' }
	};

	sc->sc_lcd.sc_dev = self;
	sc->sc_lcd.sc_iot = maa->ma_iot;
	if (bus_space_map(sc->sc_lcd.sc_iot, maa->ma_addr, LCDPANEL_REGION,
	    0, &sc->sc_lcd.sc_ioir)) {
		aprint_error(": unable to map registers\n");
		return;
	}
	bus_space_subregion(sc->sc_lcd.sc_iot, sc->sc_lcd.sc_ioir, DATA_OFFSET,
	    1, &sc->sc_lcd.sc_iodr);

	printf("\n");

	sc->sc_lcd.sc_dev_ok = 1;
	sc->sc_lcd.sc_cols = LCDPANEL_COLS;
	sc->sc_lcd.sc_vcols = LCDPANEL_VCOLS;
	sc->sc_lcd.sc_flags = HD_8BIT | HD_MULTILINE | HD_KEYPAD;

	sc->sc_lcd.sc_writereg = lcdpanel_cbt_hdwritereg;
	sc->sc_lcd.sc_readreg = lcdpanel_cbt_hdreadreg;

	hd44780_attach_subr(&sc->sc_lcd);

	/* Hello World */
	io.dat = 0;
	io.len = LCDPANEL_VCOLS * LCDPANEL_ROWS;
	memcpy(io.buf, &startup_message, io.len);
	hd44780_ddram_io(&sc->sc_lcd, sc->sc_lcd.sc_curchip, &io,
	    HD_DDRAM_WRITE);

	pmf_device_register1(self, NULL, NULL, lcdpanel_shutdown);

	sc->sc_kp.sc_iot = maa->ma_iot;
	sc->sc_kp.sc_ioh = MIPS_PHYS_TO_KSEG1(LCDPANEL_BASE); /* XXX */

	sc->sc_kp.sc_knum = sizeof(keys) / sizeof(struct lcdkp_xlate);
	sc->sc_kp.sc_kpad = keys;
	sc->sc_kp.sc_rread = lcdpanel_cbt_kprread;

	lcdkp_attach_subr(&sc->sc_kp);

	callout_init(&sc->sc_callout, 0);
	selinit(&sc->sc_selq);
}

static bool
lcdpanel_shutdown(device_t self, int howto)
{
	struct lcdpanel_softc *sc = device_private(self);
	struct hd44780_io io;

	/* Goodbye World */
	io.dat = 0;
	io.len = LCDPANEL_VCOLS * LCDPANEL_ROWS;
	if (howto & RB_HALT)
		memcpy(io.buf, &halt_message, io.len);
	else
		memcpy(io.buf, &reboot_message, io.len);
	hd44780_ddram_io(&sc->sc_lcd, sc->sc_lcd.sc_curchip, &io,
	    HD_DDRAM_WRITE);

	return true;
}

static uint8_t
lcdpanel_cbt_kprread(bus_space_tag_t iot, bus_space_handle_t ioh)
{

	delay(HD_TIMEOUT_NORMAL);
	return (bus_space_read_4(iot, ioh, 0x00) >> 24) & 0xff;
}


static void
lcdpanel_cbt_hdwritereg(struct hd44780_chip *hd, uint32_t en, uint32_t rs,
    uint8_t dat)
{

	if (rs)
		bus_space_write_4(hd->sc_iot, hd->sc_iodr, 0x00, dat << 24);
	else
		bus_space_write_4(hd->sc_iot, hd->sc_ioir, 0x00, dat << 24);
	delay(HD_TIMEOUT_NORMAL);
}

static uint8_t
lcdpanel_cbt_hdreadreg(struct hd44780_chip *hd, uint32_t en, uint32_t rs)
{

	delay(HD_TIMEOUT_NORMAL);
	if (rs)
		return (bus_space_read_4(hd->sc_iot, hd->sc_iodr, 0x00) >> 24)
		    & 0xff;
	else
		return (bus_space_read_4(hd->sc_iot, hd->sc_ioir, 0x00) >> 24)
		    & 0xff;
}

int
lcdpanelopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct lcdpanel_softc *sc = device_lookup_private(&lcdpanel_cd, minor(dev));

	return (sc->sc_lcd.sc_dev_ok == 0) ? ENXIO : 0;
}

int
lcdpanelclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct lcdpanel_softc *sc = device_lookup_private(&lcdpanel_cd, minor(dev));

	selnotify(&sc->sc_selq, 0, 0);
	return 0;
}

int
lcdpanelread(dev_t dev, struct uio *uio, int flag)
{
	int error;
	uint8_t b;
	struct lcdpanel_softc *sc = device_lookup_private(&lcdpanel_cd, minor(dev));

	if (uio->uio_resid < sizeof(b))
		return EIO;

	if ((error = lcdkp_readkey(&sc->sc_kp, &b)) != 0)
		return error;

	return uiomove((void*)&b, sizeof(b), uio);
}

int
lcdpanelwrite(dev_t dev, struct uio *uio, int flag)
{
	int error;
	struct hd44780_io io;
	struct lcdpanel_softc *sc = device_lookup_private(&lcdpanel_cd, minor(dev));

	io.dat = 0;
	io.len = uio->uio_resid;
	if (io.len > HD_MAX_CHARS)
		io.len = HD_MAX_CHARS;

	if ((error = uiomove((void*)io.buf, io.len, uio)) != 0)
		return error;

	hd44780_ddram_redraw(&sc->sc_lcd, 0, &io);
	return 0;
}

int
lcdpanelioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct lcdpanel_softc *sc = device_lookup_private(&lcdpanel_cd, minor(dev));

	return hd44780_ioctl_subr(&sc->sc_lcd, cmd, data);
}

int
lcdpanelpoll(dev_t dev, int events, struct lwp *l)
{
	int revents = 0;

	if ((events & (POLLIN | POLLRDNORM)) != 0) {
		struct lcdpanel_softc *sc;

		sc = device_lookup_private(&lcdpanel_cd, minor(dev));
		if (lcdkp_scankey(&sc->sc_kp) != 0) {
			revents = events & (POLLIN | POLLRDNORM);
		} else {
			selrecord(l, &sc->sc_selq);
			callout_reset(&sc->sc_callout, LCDPANEL_POLLRATE,
					lcdpanel_soft, sc);
		}
	}

	return revents;
}

static void
lcdpanel_soft(void *arg)
{
	struct lcdpanel_softc *sc = arg;

	if (lcdkp_scankey(&sc->sc_kp) != 0)
		selnotify(&sc->sc_selq, 0, 0);
	else
		callout_reset(&sc->sc_callout, LCDPANEL_POLLRATE, lcdpanel_soft, sc);
}
