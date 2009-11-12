/* $NetBSD: xboxcontroller.c,v 1.12 2009/11/12 20:01:15 dyoung Exp $ */

/*-
 * Copyright (c) 2007 Jared D. McNeill <jmcneill@invisible.ca>
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: xboxcontroller.c,v 1.12 2009/11/12 20:01:15 dyoung Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <sys/bus.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsmousevar.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbdevs.h>

#define XBOX_CONTROLLER_BUFSZ	32

struct xboxcontroller_softc {
	USBBASEDEVICE		sc_dev;

	usbd_device_handle	sc_udev;
	usbd_interface_handle	sc_iface;
	int			sc_ed;
	usbd_pipe_handle	sc_ep;
	unsigned char		*sc_buf;

	int			sc_drvmode;
#define	XBOX_CONTROLLER_MODE_MOUSE	1
#define	XBOX_CONTROLLER_MODE_JOYSTICK	2

	device_t		sc_wsmousedev;
	char			sc_enabled;
	char			sc_dying;
};

static void xboxcontroller_intr(usbd_xfer_handle, usbd_private_handle,
				usbd_status);

static int xboxcontroller_wsmouse_enable(void *);
static int xboxcontroller_wsmouse_ioctl(void *, u_long, void *, int,
					struct lwp *);
static void xboxcontroller_wsmouse_disable(void *);

static const struct wsmouse_accessops xboxcontroller_accessops = {
	xboxcontroller_wsmouse_enable,
	xboxcontroller_wsmouse_ioctl,
	xboxcontroller_wsmouse_disable
};

int xboxcontroller_match(device_t, cfdata_t, void *);
void xboxcontroller_attach(device_t, device_t, void *);
void xboxcontroller_childdet(device_t, device_t);
int xboxcontroller_detach(device_t, int);
int xboxcontroller_activate(device_t, enum devact);
extern struct cfdriver xboxcontroller_cd;
CFATTACH_DECL2_NEW(xboxcontroller, sizeof(struct xboxcontroller_softc),
    xboxcontroller_match, xboxcontroller_attach, xboxcontroller_detach,
    xboxcontroller_activate, NULL, xboxcontroller_childdet);

USB_MATCH(xboxcontroller)
{
	USB_MATCH_START(xboxcontroller, uaa);

	if (uaa->vendor == USB_VENDOR_MICROSOFT) {
		switch (uaa->product) {
		case USB_PRODUCT_MICROSOFT_XBOX_CONTROLLER_S10:
		case USB_PRODUCT_MICROSOFT_XBOX_CONTROLLER_S12:
			return UMATCH_VENDOR_PRODUCT;
		}
	}

	return UMATCH_NONE;
}

USB_ATTACH(xboxcontroller)
{
	USB_ATTACH_START(xboxcontroller, sc, uaa);
	usbd_device_handle dev = uaa->device;
	usbd_status err;
	struct wsmousedev_attach_args waa;
	usb_endpoint_descriptor_t *ed;
	char *devinfo;

	sc->sc_dev = self;

	aprint_naive("\n");
	aprint_normal("\n");

	devinfo = usbd_devinfo_alloc(dev, 0);
	aprint_normal_dev(self, "%s\n", devinfo);
	usbd_devinfo_free(devinfo);

	sc->sc_drvmode = XBOX_CONTROLLER_MODE_MOUSE;

	sc->sc_enabled = sc->sc_dying = 0;
	sc->sc_ep = NULL;
	sc->sc_udev = dev;
	err = usbd_set_config_no(dev, 1, 1);
	if (err) {
		aprint_error_dev(self, "setting config no failed: %s\n",
		    usbd_errstr(err));
		sc->sc_dying = 1;
		USB_ATTACH_ERROR_RETURN;
	}
	err = usbd_device2interface_handle(dev, 0, &sc->sc_iface);
	if (err) {
		aprint_error_dev(self, "failed to get interface: %s\n",
		    usbd_errstr(err));
		sc->sc_dying = 1;
		USB_ATTACH_ERROR_RETURN;
	}

	ed = usbd_interface2endpoint_descriptor(sc->sc_iface, 0);
	if (ed == NULL) {
		aprint_error_dev(sc->sc_dev, "couldn't get ep 0\n");
		sc->sc_dying = 1;
		USB_ATTACH_ERROR_RETURN;
	}
	sc->sc_ed = ed->bEndpointAddress;

	usbd_add_drv_event(USB_EVENT_DRIVER_ATTACH, sc->sc_udev,
	    USBDEV(sc->sc_dev));

	waa.accessops = &xboxcontroller_accessops;
	waa.accesscookie = sc;

	sc->sc_wsmousedev = config_found_ia(self, "wsmousedev", &waa,
	    wsmousedevprint);

	USB_ATTACH_SUCCESS_RETURN;
}

void
xboxcontroller_childdet(device_t self, device_t child)
{
	struct xboxcontroller_softc *sc = device_private(self);

	KASSERT(sc->sc_wsmousedev == child);
	sc->sc_wsmousedev = NULL;
}

USB_DETACH(xboxcontroller)
{
	USB_DETACH_START(xboxcontroller, sc);
	int rv;

	rv = 0;

	if (sc->sc_ep != NULL) {
		usbd_abort_pipe(sc->sc_ep);
		usbd_close_pipe(sc->sc_ep);
		sc->sc_ep = NULL;
	}

	sc->sc_dying = 1;

	if (sc->sc_wsmousedev != NULL)
		rv = config_detach(sc->sc_wsmousedev, flags);

	return rv;
}

int
xboxcontroller_activate(device_ptr_t self, enum devact act)
{
	struct xboxcontroller_softc *sc = device_private(self);

	switch (act) {
	case DVACT_DEACTIVATE:
		sc->sc_dying = 1;
		return 0;
	default:
		return EOPNOTSUPP;
	}
}

static void
xboxcontroller_intr(usbd_xfer_handle xfer, usbd_private_handle priv,
		    usbd_status status)
{
	struct xboxcontroller_softc *sc;
	unsigned char *data;
	int16_t x, y;
	char btnmask;
	uint32_t len;
	int s;

	sc = (struct xboxcontroller_softc *)priv;
	data = sc->sc_buf;

	usbd_get_xfer_status(xfer, NULL, NULL, &len, NULL);
	if (status == USBD_CANCELLED)
		return;

	x = (int16_t)(((int16_t)data[13] << 8) | data[12]);
	y = (int16_t)(((int16_t)data[15] << 8) | data[14]);
	/* z = (int16_t)(((int16_t)data[17] << 8) | data[16]); */
	/* w = (int16_t)(((int16_t)data[19] << 8) | data[18]); */

	/* de-jitter */
	if (x < 8192 && x > -8192)
		x = 0;
	if (y < 8192 && y > -8192)
		y = 0;

	switch (sc->sc_drvmode) {
	case XBOX_CONTROLLER_MODE_MOUSE:
		if (sc->sc_wsmousedev == NULL)
			goto done;
		btnmask = 0;
		if (data[2] & 0x40) btnmask |= 0x01;	/* thumb press left */
		if (data[2] & 0x80) btnmask |= 0x04;	/* thumb press right */
		if (data[2] & 0x10) btnmask |= 0x02;	/* start button */

		s = spltty();
		wsmouse_input(sc->sc_wsmousedev, btnmask,
			      x / 4096, y / 4096,
			      0, 0, /* z / 4096, w / 4096, */
			      WSMOUSE_INPUT_DELTA);
		splx(s);
		break;
	case XBOX_CONTROLLER_MODE_JOYSTICK:
		/* XXX not implemented */
		break;
	}

done:

	return;
}

static int
xboxcontroller_wsmouse_enable(void *opaque)
{
	struct xboxcontroller_softc *sc;
	usbd_status err;

	sc = (struct xboxcontroller_softc *)opaque;

	if (sc->sc_dying)
		return EIO;
	if (sc->sc_enabled)
		return EBUSY;

	sc->sc_buf = malloc(XBOX_CONTROLLER_BUFSZ, M_USBDEV, M_WAITOK);
	err = usbd_open_pipe_intr(sc->sc_iface, sc->sc_ed,
	    USBD_SHORT_XFER_OK, &sc->sc_ep, sc, sc->sc_buf,
	    XBOX_CONTROLLER_BUFSZ, xboxcontroller_intr,
	    USBD_DEFAULT_INTERVAL);
	if (err) {
		aprint_error_dev(sc->sc_dev, "open pipe failed: %s\n",
		    usbd_errstr(err));
		free(sc->sc_buf, M_USBDEV);
		sc->sc_buf = NULL;
		sc->sc_ep = NULL;
		return EIO;
	}

	sc->sc_enabled = 1;

	return 0;
}

static void
xboxcontroller_wsmouse_disable(void *opaque)
{
	struct xboxcontroller_softc *sc;

	sc = (struct xboxcontroller_softc *)opaque;

	if (!sc->sc_enabled) {
		aprint_error_dev(sc->sc_dev, "already disabled!\n");
		return;
	}

	if (sc->sc_ep != NULL) {
		usbd_abort_pipe(sc->sc_ep);
		usbd_close_pipe(sc->sc_ep);
		sc->sc_ep = NULL;
	}

	if (sc->sc_buf != NULL) {
		free(sc->sc_buf, M_USBDEV);
		sc->sc_buf = NULL;
	}

	sc->sc_enabled = 0;

	return;
}

static int
xboxcontroller_wsmouse_ioctl(void *opaque, u_long cmd, void *data, int flag,
			     struct lwp *l)
{

	switch (cmd) {
	case WSMOUSEIO_GTYPE:
		*(u_int *)data = WSMOUSE_TYPE_USB; /* XXX not really... */
		return 0;
	}

	return EPASSTHROUGH;
}
