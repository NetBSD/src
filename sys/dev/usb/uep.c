/*	$NetBSD: uep.c,v 1.14.2.1 2010/11/06 08:08:37 uebayasi Exp $	*/

/*
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tyler C. Sarna (tsarna@netbsd.org).
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
 *  eGalax USB touchpanel controller driver.
 *
 *  For Programming Documentation, see:
 *
 *  http://www.egalax.com/SoftwareProgrammingGuide_1.1.pdf
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uep.c,v 1.14.2.1 2010/11/06 08:08:37 uebayasi Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/vnode.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbdevs.h>
#include <dev/usb/usb_quirks.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsmousevar.h>
#include <dev/wscons/tpcalibvar.h>

#define UIDSTR	"eGalax USB SN000000"

struct uep_softc {
	device_t sc_dev;
	usbd_device_handle sc_udev;	/* device */
	usbd_interface_handle sc_iface;	/* interface */
	int sc_iface_number;

	int			sc_intr_number; /* interrupt number */
	usbd_pipe_handle	sc_intr_pipe;	/* interrupt pipe */
	u_char			*sc_ibuf;
	int			sc_isize;

	device_t		sc_wsmousedev;	/* wsmouse device */
	struct tpcalib_softc	sc_tpcalib;	/* calibration */

	u_char sc_enabled;
	u_char sc_dying;
};

static struct wsmouse_calibcoords default_calib = {
	.minx = 0,
	.miny = 0,
	.maxx = 2047,
	.maxy = 2047,
	.samplelen = WSMOUSE_CALIBCOORDS_RESET,
};

Static void uep_intr(usbd_xfer_handle, usbd_private_handle, usbd_status);

Static int	uep_enable(void *);
Static void	uep_disable(void *);
Static int	uep_ioctl(void *, u_long, void *, int, struct lwp *);

const struct wsmouse_accessops uep_accessops = {
	uep_enable,
	uep_ioctl,
	uep_disable,
};

int uep_match(device_t, cfdata_t, void *);
void uep_attach(device_t, device_t, void *);
void uep_childdet(device_t, device_t);
int uep_detach(device_t, int);
int uep_activate(device_t, enum devact);
extern struct cfdriver uep_cd;
CFATTACH_DECL2_NEW(uep, sizeof(struct uep_softc), uep_match, uep_attach,
    uep_detach, uep_activate, NULL, uep_childdet);

int 
uep_match(device_t parent, cfdata_t match, void *aux)
{
	struct usb_attach_arg *uaa = aux;

	if ((uaa->vendor == USB_VENDOR_EGALAX) && (
		(uaa->product == USB_PRODUCT_EGALAX_TPANEL)
		|| (uaa->product == USB_PRODUCT_EGALAX_TPANEL2)
	))
		return UMATCH_VENDOR_PRODUCT;

	if ((uaa->vendor == USB_VENDOR_EGALAX2)
	&&  (uaa->product == USB_PRODUCT_EGALAX2_TPANEL))
		return UMATCH_VENDOR_PRODUCT;


	return UMATCH_NONE;
}

void 
uep_attach(device_t parent, device_t self, void *aux)
{
	struct uep_softc *sc = device_private(self);
	struct usb_attach_arg *uaa = aux;
	usbd_device_handle dev = uaa->device;
	usb_config_descriptor_t *cdesc;
	usb_interface_descriptor_t *id;
	usb_endpoint_descriptor_t *ed;
	struct wsmousedev_attach_args a;
	char *devinfop;
	usbd_status err;
	int i, found;

	sc->sc_dev = self;

	aprint_naive("\n");
	aprint_normal("\n");

	devinfop = usbd_devinfo_alloc(dev, 0);
	aprint_normal_dev(self, "%s\n", devinfop);
	usbd_devinfo_free(devinfop);

	sc->sc_udev = dev;
	sc->sc_intr_number = -1;
	sc->sc_intr_pipe = NULL;
	sc->sc_enabled = sc->sc_isize = 0;

	/* Move the device into the configured state. */
	err = usbd_set_config_index(dev, 0, 1);
	if (err) {
		aprint_error("\n%s: failed to set configuration, err=%s\n",
			device_xname(sc->sc_dev),  usbd_errstr(err));
		sc->sc_dying = 1;
		return;
	}

	/* get the config descriptor */
	cdesc = usbd_get_config_descriptor(sc->sc_udev);
	if (cdesc == NULL) {
		aprint_error_dev(self,
		    "failed to get configuration descriptor\n");
		sc->sc_dying = 1;
		return;
	}

	/* get the interface */
	err = usbd_device2interface_handle(dev, 0, &sc->sc_iface);
	if (err) {
		aprint_error("\n%s: failed to get interface, err=%s\n",
			device_xname(sc->sc_dev), usbd_errstr(err));
		sc->sc_dying = 1;
		return;
	}

	/* Find the interrupt endpoint */
	id = usbd_get_interface_descriptor(sc->sc_iface);
	sc->sc_iface_number = id->bInterfaceNumber;
	found = 0;

	for (i = 0; i < id->bNumEndpoints; i++) {
		ed = usbd_interface2endpoint_descriptor(sc->sc_iface, i);
		if (ed == NULL) {
			aprint_error_dev(self,
			    "no endpoint descriptor for %d\n", i);
			sc->sc_dying = 1;
			return;
		}

		if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_IN &&
		    UE_GET_XFERTYPE(ed->bmAttributes) == UE_INTERRUPT) {
			sc->sc_intr_number = ed->bEndpointAddress;
			sc->sc_isize = UGETW(ed->wMaxPacketSize);
		}
	}

	if (sc->sc_intr_number== -1) {
		aprint_error_dev(self, "Could not find interrupt in\n");
		sc->sc_dying = 1;
		return;
	}

	usbd_add_drv_event(USB_EVENT_DRIVER_ATTACH, sc->sc_udev,
			   sc->sc_dev);

	a.accessops = &uep_accessops;
	a.accesscookie = sc;

	sc->sc_wsmousedev = config_found(self, &a, wsmousedevprint);

	tpcalib_init(&sc->sc_tpcalib);
	tpcalib_ioctl(&sc->sc_tpcalib, WSMOUSEIO_SCALIBCOORDS,
		(void *)&default_calib, 0, 0);

	return;
}

int 
uep_detach(device_t self, int flags)
{
	struct uep_softc *sc = device_private(self);
	int rv = 0;

	if (sc->sc_intr_pipe != NULL) {
		usbd_abort_pipe(sc->sc_intr_pipe);
		usbd_close_pipe(sc->sc_intr_pipe);
		sc->sc_intr_pipe = NULL;
	}

	sc->sc_dying = 1;

	/* save current calib as defaults */
	default_calib = sc->sc_tpcalib.sc_saved;

	if (sc->sc_wsmousedev != NULL)
		rv = config_detach(sc->sc_wsmousedev, flags);

	usbd_add_drv_event(USB_EVENT_DRIVER_DETACH, sc->sc_udev,
			   sc->sc_dev);

	return rv;
}

void
uep_childdet(device_t self, device_t child)
{
	struct uep_softc *sc = device_private(self);

	KASSERT(sc->sc_wsmousedev == child);
	sc->sc_wsmousedev = NULL;
}

int
uep_activate(device_t self, enum devact act)
{
	struct uep_softc *sc = device_private(self);

	switch (act) {
	case DVACT_DEACTIVATE:
		sc->sc_dying = 1;
		return 0;
	default:
		return EOPNOTSUPP;
	}
}

Static int
uep_enable(void *v)
{
	struct uep_softc *sc = v;
	int err;

	if (sc->sc_dying)
		return EIO;

	if (sc->sc_enabled)
		return EBUSY;

	if (sc->sc_isize == 0)
		return 0;
	sc->sc_ibuf = malloc(sc->sc_isize, M_USBDEV, M_WAITOK);
	err = usbd_open_pipe_intr(sc->sc_iface, sc->sc_intr_number,
		USBD_SHORT_XFER_OK, &sc->sc_intr_pipe, sc, sc->sc_ibuf,
		sc->sc_isize, uep_intr, USBD_DEFAULT_INTERVAL);
	if (err) {
		free(sc->sc_ibuf, M_USBDEV);
		sc->sc_intr_pipe = NULL;
		return EIO;
	}

	sc->sc_enabled = 1;

	return 0;
}

Static void
uep_disable(void *v)
{
	struct uep_softc *sc = v;

	if (!sc->sc_enabled) {
		printf("uep_disable: already disabled!\n");
		return;
	}

	/* Disable interrupts. */
	if (sc->sc_intr_pipe != NULL) {
		usbd_abort_pipe(sc->sc_intr_pipe);
		usbd_close_pipe(sc->sc_intr_pipe);
		sc->sc_intr_pipe = NULL;
	}

	if (sc->sc_ibuf != NULL) {
		free(sc->sc_ibuf, M_USBDEV);
		sc->sc_ibuf = NULL;
	}

	sc->sc_enabled = 0;
}

Static int
uep_ioctl(void *v, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct uep_softc *sc = v;
	struct wsmouse_id *id;

	switch (cmd) {
	case WSMOUSEIO_GTYPE:
		*(u_int *)data = WSMOUSE_TYPE_TPANEL;
		return 0;

	case WSMOUSEIO_GETID:
		/*
		 * return unique ID string
		 * "<vendor> <model> <serial number>"
		 * unfortunately we have no serial number...
		 */
		 id = (struct wsmouse_id *)data;
		 if (id->type != WSMOUSE_ID_TYPE_UIDSTR)
		 	return EINVAL;

		strcpy(id->data, UIDSTR);
		id->length = strlen(UIDSTR);
		return 0;

	case WSMOUSEIO_SCALIBCOORDS:
	case WSMOUSEIO_GCALIBCOORDS:
		return tpcalib_ioctl(&sc->sc_tpcalib, cmd, data, flag, l);
	}

	return EPASSTHROUGH;
}

void
uep_intr(usbd_xfer_handle xfer, usbd_private_handle addr, usbd_status status)
{
	struct uep_softc *sc = addr;
	u_char *p = sc->sc_ibuf;
	u_int32_t len;
	int x = 0, y = 0, s;

	usbd_get_xfer_status(xfer, NULL, NULL, &len, NULL);

	if (status == USBD_CANCELLED)
		return;

	if (status != USBD_NORMAL_COMPLETION) {
		aprint_error_dev(sc->sc_dev, "status %d\n", status);
		usbd_clear_endpoint_stall_async(sc->sc_intr_pipe);
		return;
	}

	if (len != 5) {
#if 0
		printf("%s: bad input length %d != %d\n",
			device_xname(sc->sc_dev), len, sc->sc_isize);
#endif
		return;
	}

	if ((p[0] & 0xFE) != 0x80) {
		aprint_error_dev(sc->sc_dev,
		    "bad input packet format\n");
		return;
	}

	if (sc->sc_wsmousedev != NULL) {
                /*
                 * Packet format is 5 bytes:
                 *
                 * 1000000T
                 * 0000AAAA
                 * 0AAAAAAA
                 * 0000BBBB
                 * 0BBBBBBB
                 *
                 * T: 1=touched 0=not touched
                 * A: bits of axis A position, MSB to LSB
                 * B: bits of axis B position, MSB to LSB
                 *
                 * For the unit I have, A = Y and B = X.
                 * I don't know if units exist with A=X and B=Y,
                 * if so we'll cross that bridge when we come to it.
                 *
                 * The controller sends a stream of T=1 events while the
                 * panel is touched, followed by a single T=0 event.
                 *
                 */

		x = (p[3] << 7) | p[4];
		y = (p[1] << 7) | p[2];

		tpcalib_trans(&sc->sc_tpcalib, x, y, &x, &y);

		s = spltty();
		wsmouse_input(sc->sc_wsmousedev, p[0] & 0x01, x, y, 0, 0,
			WSMOUSE_INPUT_ABSOLUTE_X | WSMOUSE_INPUT_ABSOLUTE_Y);
		splx(s);
	}
}
