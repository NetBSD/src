/* $NetBSD: ikbd.c,v 1.1 2024/12/09 22:05:17 jmcneill Exp $ */

/*	$OpenBSD: ikbd.c,v 1.2 2022/09/03 15:48:16 kettenis Exp $	*/
/*
 * HID-over-i2c keyboard driver
 *
 * Copyright (c) 2016 Mark Kettenis <kettenis@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/ioctl.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/ihidev.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wskbdvar.h>
#include <dev/wscons/wsksymdef.h>

#include <dev/hid/hid.h>
#include <dev/hid/hidkbdsc.h>

extern const struct wscons_keydesc hidkbd_keydesctab[];
static struct wskbd_mapdata ikbd_keymapdata = {
	hidkbd_keydesctab,
	KB_US,
};

struct ikbd_softc {
	struct ihidev	sc_hdev;
	struct hidkbd	sc_kbd;
	int		sc_spl;
};

void	ikbd_intr(struct ihidev *addr, void *ibuf, u_int len);

void	ikbd_cngetc(void *, u_int *, int *);
void	ikbd_cnpollc(void *, int);

const struct wskbd_consops ikbd_consops = {
	.getc = ikbd_cngetc,
	.pollc = ikbd_cnpollc,
	.bell = NULL,
};

int	ikbd_enable(void *, int);
void	ikbd_set_leds(void *, int);
int	ikbd_ioctl(void *, u_long, void *, int, lwp_t *);

const struct wskbd_accessops ikbd_accessops = {
	.enable = ikbd_enable,
	.set_leds = ikbd_set_leds,
	.ioctl = ikbd_ioctl,
};

int	ikbd_match(device_t, cfdata_t, void *);
void	ikbd_attach(device_t, device_t, void *);
int	ikbd_detach(device_t, int);

CFATTACH_DECL_NEW(ikbd, sizeof(struct ikbd_softc),
    ikbd_match, ikbd_attach, ikbd_detach, NULL);

int
ikbd_match(device_t parent, cfdata_t match, void *aux)
{
	struct ihidev_attach_arg *iha = aux;
	int size;
	void *desc;

	ihidev_get_report_desc(iha->parent, &desc, &size);
	if (!hid_is_collection(desc, size, iha->reportid,
	    HID_USAGE2(HUP_GENERIC_DESKTOP, HUG_KEYBOARD)))
		return (IMATCH_NONE);

	return (IMATCH_IFACECLASS);
}

void
ikbd_attach(device_t parent, device_t self, void *aux)
{
	struct ikbd_softc *sc = device_private(self);
	struct hidkbd *kbd = &sc->sc_kbd;
	struct ihidev_attach_arg *iha = (struct ihidev_attach_arg *)aux;
	int dlen, repid;
	void *desc;

	sc->sc_hdev.sc_idev = self;
	sc->sc_hdev.sc_intr = ikbd_intr;
	sc->sc_hdev.sc_parent = iha->parent;
	sc->sc_hdev.sc_report_id = iha->reportid;

	ihidev_get_report_desc(iha->parent, &desc, &dlen);
	repid = iha->reportid;
	sc->sc_hdev.sc_isize = hid_report_size(desc, dlen, hid_input, repid);
	sc->sc_hdev.sc_osize = hid_report_size(desc, dlen, hid_output, repid);
	sc->sc_hdev.sc_fsize = hid_report_size(desc, dlen, hid_feature, repid);

	if (hidkbd_attach(self, kbd, 1, 0, repid, desc, dlen) != 0)
		return;

	aprint_naive("\n");
	aprint_normal("\n");

	if (kbd->sc_console_keyboard) {
		wskbd_cnattach(&ikbd_consops, sc, &ikbd_keymapdata);
		ikbd_enable(sc, 1);
	}

	hidkbd_attach_wskbd(kbd, KB_US, &ikbd_accessops);
}

int
ikbd_detach(device_t self, int flags)
{
	struct ikbd_softc *sc = device_private(self);
	struct hidkbd *kbd = &sc->sc_kbd;

	return hidkbd_detach(kbd, flags);
}

void
ikbd_intr(struct ihidev *addr, void *ibuf, u_int len)
{
	struct ikbd_softc *sc = (struct ikbd_softc *)addr;
	struct hidkbd *kbd = &sc->sc_kbd;

	if (kbd->sc_enabled != 0)
		hidkbd_input(kbd, (uint8_t *)ibuf, len);
}

int
ikbd_enable(void *v, int on)
{
	struct ikbd_softc *sc = v;
	struct hidkbd *kbd = &sc->sc_kbd;
	int rv;

	if ((rv = hidkbd_enable(kbd, on)) != 0)
		return rv;

	if (on) {
		return ihidev_open(&sc->sc_hdev);
	} else {
		ihidev_close(&sc->sc_hdev);
		return 0;
	}
}

void
ikbd_set_leds(void *v, int leds)
{
}

int
ikbd_ioctl(void *v, u_long cmd, void *data, int flag, lwp_t *l)
{
	struct ikbd_softc *sc = v;
	struct hidkbd *kbd = &sc->sc_kbd;

	switch (cmd) {
	case WSKBDIO_GTYPE:
		/* XXX: should we set something else? */
		*(u_int *)data = WSKBD_TYPE_USB;
		return 0;
	default:
		return hidkbd_ioctl(kbd, cmd, data, flag, l);
	}
}

/* Console interface. */
void
ikbd_cngetc(void *v, u_int *type, int *data)
{
	struct ikbd_softc *sc = v;
	struct hidkbd *kbd = &sc->sc_kbd;

	kbd->sc_polling = 1;
#if notyet
	while (kbd->sc_npollchar <= 0) {
		ihidev_poll(sc->sc_hdev.sc_parent);
		delay(1000);
	}
#endif
	kbd->sc_polling = 0;
	hidkbd_cngetc(kbd, type, data);
}

void
ikbd_cnpollc(void *v, int on)
{
	struct ikbd_softc *sc = v;

	if (on)
		sc->sc_spl = spltty();
	else
		splx(sc->sc_spl);
}
