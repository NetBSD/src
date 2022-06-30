/*	$NetBSD: uintuos.c,v 1.1 2022/06/30 06:30:44 macallan Exp $	*/

/*
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Yorick Hardy.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  Wacom Intuos Pen driver.
 *  (partially based on uep.c and ums.c)
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uintuos.c,v 1.1 2022/06/30 06:30:44 macallan Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/vnode.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbhid.h>

#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbdevs.h>
#include <dev/usb/uhidev.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsmousevar.h>
#include <dev/wscons/tpcalibvar.h>

struct uintuos_softc {
	struct uhidev sc_hdev;

	device_t		sc_wsmousedev;	/* wsmouse device */
	struct tpcalib_softc	sc_tpcalib;	/* calibration */

	u_char sc_enabled;
	u_char sc_dying;
};

Static void uintuos_cth490_intr(struct uhidev *, void *, u_int);
Static void uintuos_ctl6100_intr(struct uhidev *, void *, u_int);

Static int	uintuos_enable(void *);
Static void	uintuos_disable(void *);
Static int	uintuos_ioctl(void *, u_long, void *, int, struct lwp *);

const struct wsmouse_accessops uintuos_accessops = {
	uintuos_enable,
	uintuos_ioctl,
	uintuos_disable,
};

int uintuos_match(device_t, cfdata_t, void *);
void uintuos_attach(device_t, device_t, void *);
void uintuos_childdet(device_t, device_t);
int uintuos_detach(device_t, int);
int uintuos_activate(device_t, enum devact);

CFATTACH_DECL2_NEW(uintuos, sizeof(struct uintuos_softc), uintuos_match, uintuos_attach,
    uintuos_detach, uintuos_activate, NULL, uintuos_childdet);

int
uintuos_match(device_t parent, cfdata_t match, void *aux)
{
	struct uhidev_attach_arg *uha = aux;

	if ((uha->uiaa->uiaa_vendor == USB_VENDOR_WACOM) &&
		(uha->uiaa->uiaa_product == USB_PRODUCT_WACOM_CTH490K0) &&
		(uha->reportid == 16))
		return UMATCH_VENDOR_PRODUCT;

	if ((uha->uiaa->uiaa_vendor == USB_VENDOR_WACOM) &&
		(uha->uiaa->uiaa_product == USB_PRODUCT_WACOM_CTL6100WL) &&
		(uha->reportid == 16))
		return UMATCH_VENDOR_PRODUCT;

	return UMATCH_NONE;
}

void
uintuos_attach(device_t parent, device_t self, void *aux)
{
	struct uintuos_softc *sc = device_private(self);
	struct uhidev_attach_arg *uha = aux;
	struct wsmousedev_attach_args a;
	struct wsmouse_calibcoords default_calib;

	aprint_normal("\n");
	aprint_naive("\n");

	sc->sc_hdev.sc_dev = self;
	sc->sc_hdev.sc_parent = uha->parent;
	sc->sc_hdev.sc_report_id = uha->reportid;

	switch (uha->uiaa->uiaa_product) {
	case USB_PRODUCT_WACOM_CTH490K0:
		default_calib.minx = 0,
		default_calib.miny = 0,
		default_calib.maxx = 7600,
		default_calib.maxy = 4750,
		sc->sc_hdev.sc_intr = uintuos_cth490_intr;
		break;
	case USB_PRODUCT_WACOM_CTL6100WL:
		default_calib.minx = 0,
		default_calib.miny = 0,
		default_calib.maxx = 21600,
		default_calib.maxy = 13471,
		sc->sc_hdev.sc_intr = uintuos_ctl6100_intr;
		break;
	default:
		sc->sc_hdev.sc_intr = uintuos_cth490_intr;
		aprint_error_dev(self, "unsupported product\n");
		break;
	}


	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");

	a.accessops = &uintuos_accessops;
	a.accesscookie = sc;

	sc->sc_wsmousedev = config_found(self, &a, wsmousedevprint);

	default_calib.samplelen = WSMOUSE_CALIBCOORDS_RESET,
	tpcalib_init(&sc->sc_tpcalib);
	tpcalib_ioctl(&sc->sc_tpcalib, WSMOUSEIO_SCALIBCOORDS,
		(void *)&default_calib, 0, 0);

	return;
}

int
uintuos_detach(device_t self, int flags)
{
	struct uintuos_softc *sc = device_private(self);
	int rv = 0;

	sc->sc_dying = 1;

	if (sc->sc_wsmousedev != NULL)
		rv = config_detach(sc->sc_wsmousedev, flags);

	pmf_device_deregister(self);

	return rv;
}

void
uintuos_childdet(device_t self, device_t child)
{
	struct uintuos_softc *sc = device_private(self);

	KASSERT(sc->sc_wsmousedev == child);
	sc->sc_wsmousedev = NULL;
}

int
uintuos_activate(device_t self, enum devact act)
{
	struct uintuos_softc *sc = device_private(self);

	switch (act) {
	case DVACT_DEACTIVATE:
		sc->sc_dying = 1;
		return 0;
	default:
		return EOPNOTSUPP;
	}
}

Static int
uintuos_enable(void *v)
{
	struct uintuos_softc *sc = v;
	int error;

	if (sc->sc_dying)
		return EIO;

	if (sc->sc_enabled)
		return EBUSY;

	sc->sc_enabled = 1;

	error = uhidev_open(&sc->sc_hdev);
	if (error)
		sc->sc_enabled = 0;

	return error;
}

Static void
uintuos_disable(void *v)
{
	struct uintuos_softc *sc = v;

	if (!sc->sc_enabled) {
		printf("uintuos_disable: not enabled\n");
		return;
	}

	sc->sc_enabled = 0;
	uhidev_close(&sc->sc_hdev);
}

Static int
uintuos_ioctl(void *v, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct uintuos_softc *sc = v;
	struct wsmouse_id *id;

	switch (cmd) {
	case WSMOUSEIO_GTYPE:
		*(u_int *)data = WSMOUSE_TYPE_TPANEL;
		return 0;

	case WSMOUSEIO_GETID:
		id = (struct wsmouse_id *)data;
		if (id->type != WSMOUSE_ID_TYPE_UIDSTR)
		 	return EINVAL;

		snprintf(id->data, WSMOUSE_ID_MAXLEN, "%s %s %s",
			sc->sc_hdev.sc_parent->sc_udev->ud_vendor,
			sc->sc_hdev.sc_parent->sc_udev->ud_product,
			sc->sc_hdev.sc_parent->sc_udev->ud_serial);
		id->length = strlen(id->data);
		return 0;

	case WSMOUSEIO_SCALIBCOORDS:
	case WSMOUSEIO_GCALIBCOORDS:
		return tpcalib_ioctl(&sc->sc_tpcalib, cmd, data, flag, l);
	}

	return EPASSTHROUGH;
}

void
uintuos_cth490_intr(struct uhidev *addr, void *ibuf, u_int len)
{
	struct uintuos_softc *sc = (struct uintuos_softc *)addr;
	u_char *p = ibuf;
	u_int btns = 0;
	int x = 0, y = 0, z = 0, s;

	if (len != 9) {
		aprint_error_dev(sc->sc_hdev.sc_dev, "wrong report size - ignoring\n");
		return;
	}

	/*
	 * Each report package contains 9 bytes as below (guessed by inspection):
	 *
	 * Byte 0	?VR? ?21T
	 * Byte 1	X coordinate (high byte)
	 * Byte 2	X coordinate (low byte)
	 * Byte 3	Y coordinate (high byte)
	 * Byte 4	Y coordinate (low byte) 
	 * Byte 5	Pressure (high byte)
	 * Byte 6	Pressure (low byte)
	 * Byte 7	zero
	 * Byte 8	quality
	 *
	 * V: 1=valid data, 0=don't use
	 * R: 1=in range, 2=cannot sense
	 * 1: barrel button 1, 1=pressed, 0=not pressed
	 * 2: barrel button 2, 1=pressed, 0=not pressed
	 * T: 1=touched, 0=not touched (unreliable?)
	 * quality: 0 - 255, 255 = most reliable?
	 *
	 */

	/* no valid data or not in range */
	if ((p[0] & 0x40) == 0 || (p[0] & 0x20) == 0)
		return;

	if (sc->sc_wsmousedev != NULL) {
		x = (p[1] << 8) | p[2];
		y = (p[3] << 8) | p[4];
		z = (p[5] << 8) | p[6];

		/*
		 * The "T" bit seems to require a *lot* of pressure to remain "1",
		 * use the pressure value instead (> 255) for button 1
		 *
		 */
		if (p[5] != 0)
			btns |= 1;

		/* barrel button 1 => button 2 */
		if (p[0] & 0x02)
			btns |= 2;

		/* barrel button 2 => button 3 */
		if (p[0] & 0x04)
			btns |= 4;

		tpcalib_trans(&sc->sc_tpcalib, x, y, &x, &y);

		s = spltty();
		wsmouse_input(sc->sc_wsmousedev, btns, x, y, z, 0,
			WSMOUSE_INPUT_ABSOLUTE_X |
			WSMOUSE_INPUT_ABSOLUTE_Y |
			WSMOUSE_INPUT_ABSOLUTE_Z);
		splx(s);
	}
}

void
uintuos_ctl6100_intr(struct uhidev *addr, void *ibuf, u_int len)
{
	struct uintuos_softc *sc = (struct uintuos_softc *)addr;
	u_char *p = ibuf;
	u_int btns = 0;
	int x = 0, y = 0, z = 0, s;

	if (len != 26) {
		aprint_error_dev(sc->sc_hdev.sc_dev, "wrong report size - ignoring\n");
		return;
	}

	/*
	 * Each report package contains 26 bytes as below (guessed by inspection):
	 *
	 * Byte 0	?VR? ?21T
	 * Byte 1	X coordinate (low byte)
	 * Byte 2	X coordinate (high byte)
	 * Byte 3	zero
	 * Byte 4	Y coordinate (low byte)
	 * Byte 5	Y coordinate (high byte) 
	 * Byte 6	zero
	 * Byte 7	Pressure (low byte)
	 * Byte 8	Pressure (high byte)
	 * Byte 9	zero
	 * Byte 10..14	zero
	 * Byte 15	quality?
	 * Byte 16..25	unknown
	 *
	 * V: 1=valid data, 0=don't use
	 * R: 1=in range, 0=cannot sense
	 * 1: barrel button 1, 1=pressed, 0=not pressed
	 * 2: barrel button 2, 1=pressed, 0=not pressed
	 * T: 1=touched, 0=not touched
	 * quality: 0 - 63, 0 = most reliable?
	 *
	 */

	/* no valid data or not in range */
	if ((p[0] & 0x40) == 0 || (p[0] & 0x20) == 0)
		return;

	if (sc->sc_wsmousedev != NULL) {
		x = (p[2] << 8) | p[1];
		y = (p[5] << 8) | p[4];
		z = (p[8] << 8) | p[7];

		btns = p[0] & 0x7;

		tpcalib_trans(&sc->sc_tpcalib, x, y, &x, &y);

		s = spltty();
		wsmouse_input(sc->sc_wsmousedev, btns, x, y, z, 0,
			WSMOUSE_INPUT_ABSOLUTE_X |
			WSMOUSE_INPUT_ABSOLUTE_Y |
			WSMOUSE_INPUT_ABSOLUTE_Z);
		splx(s);
	}
}
