/*	$NetBSD: uep.c,v 1.18.6.2 2017/12/03 11:37:34 jdolecek Exp $	*/

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
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uep.c,v 1.18.6.2 2017/12/03 11:37:34 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
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
/* calibration - integer values, perhaps sysctls?  */
#define X_RATIO   293 
#define X_OFFSET  -28
#define Y_RATIO   -348
#define Y_OFFSET  537
/* an X_RATIO of ``312''  means : reduce by a factor 3.12 x axis amplitude */
/* an Y_RATIO of ``-157'' means : reduce by a factor 1.57 y axis amplitude,
 * and reverse y motion */

struct uep_softc {
	device_t sc_dev;
	struct usbd_device *sc_udev;	/* device */
	struct usbd_interface *sc_iface;	/* interface */
	int sc_iface_number;

	int			sc_intr_number; /* interrupt number */
	struct usbd_pipe *	sc_intr_pipe;	/* interrupt pipe */
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

Static void uep_intr(struct usbd_xfer *, void *, usbd_status);

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

	if ((uaa->uaa_vendor == USB_VENDOR_EGALAX) && (
		(uaa->uaa_product == USB_PRODUCT_EGALAX_TPANEL)
		|| (uaa->uaa_product == USB_PRODUCT_EGALAX_TPANEL2)))
		return UMATCH_VENDOR_PRODUCT;

	if ((uaa->uaa_vendor == USB_VENDOR_EGALAX2)
	&&  (uaa->uaa_product == USB_PRODUCT_EGALAX2_TPANEL))
		return UMATCH_VENDOR_PRODUCT;


	return UMATCH_NONE;
}

void
uep_attach(device_t parent, device_t self, void *aux)
{
	struct uep_softc *sc = device_private(self);
	struct usb_attach_arg *uaa = aux;
	struct usbd_device *dev = uaa->uaa_device;
	usb_config_descriptor_t *cdesc;
	usb_interface_descriptor_t *id;
	usb_endpoint_descriptor_t *ed;
	usb_device_request_t req;
	uByte act;
	struct wsmousedev_attach_args a;
	char *devinfop;
	usbd_status err;
	int i;

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

	/* Newer controllers need an activation command */
	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = 0x0a;
	USETW(req.wValue, 'A');
	USETW(req.wIndex, 0);
	USETW(req.wLength, 1);
	usbd_do_request(dev, &req, &act);

	usbd_add_drv_event(USB_EVENT_DRIVER_ATTACH, sc->sc_udev, sc->sc_dev);

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

	usbd_add_drv_event(USB_EVENT_DRIVER_DETACH, sc->sc_udev, sc->sc_dev);
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

	sc->sc_ibuf = kmem_alloc(sc->sc_isize, KM_SLEEP);
	err = usbd_open_pipe_intr(sc->sc_iface, sc->sc_intr_number,
		USBD_SHORT_XFER_OK, &sc->sc_intr_pipe, sc, sc->sc_ibuf,
		sc->sc_isize, uep_intr, USBD_DEFAULT_INTERVAL);
	if (err) {
		kmem_free(sc->sc_ibuf, sc->sc_isize);
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
		kmem_free(sc->sc_ibuf, sc->sc_isize);
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

static int
uep_adjust(int v, int off, int rat)
{
	int num = 100 * v;
	int quot = num / rat;
	int rem = num % rat;
	if (num >= 0 && rem < 0)
		quot++;
	return quot + off;
}

void
uep_intr(struct usbd_xfer *xfer, void *addr, usbd_status status)
{
	struct uep_softc *sc = addr;
	u_char *p = sc->sc_ibuf;
	u_char msk;
	uint32_t len;
	int x = 0, y = 0, s;

	usbd_get_xfer_status(xfer, NULL, NULL, &len, NULL);

	if (status == USBD_CANCELLED)
		return;

	if (status != USBD_NORMAL_COMPLETION) {
		aprint_error_dev(sc->sc_dev, "status %d\n", status);
		usbd_clear_endpoint_stall_async(sc->sc_intr_pipe);
		return;
	}

	/* First bit is always set to 1 */
	if ((p[0] & 0x80) != 0x80) {
		aprint_error_dev(sc->sc_dev, "bad input packet format\n");
		return;
	}

	if (sc->sc_wsmousedev != NULL) {
		/*
		 * Each report package may contain 5 or 6 bytes as below:
		 *
		 * Byte 0	1ZM00HLT
		 * Byte 1	0AAAAAAA
		 * Byte 2	0AAAAAAA
		 * Byte 3	0BBBBBBB
		 * Byte 4	0BBBBBBB
		 * Byte 5	0PPPPPPP
		 *
		 * Z: 1=byte 5 is pressure information, 0=no pressure
		 * M: 1=byte 5 is play id, 0=no player id
		 * T: 1=touched, 0=not touched
		 * H,L: Resolution
		 *	0,0: 11 bits
		 *	0,1: 12 bits
		 *	1,0: 13 bits
		 *	1,1: 14 bits
		 * A: bits of axis A position, MSB to LSB
		 * B: bits of axis B position, MSB to LSB
		 *
		 * The packet has six bytes only if Z or M is set.
		 * Byte 5, if sent, is ignored.
		 *
		 * For the unit I have, A = Y and B = X.
		 * I don't know if units exist with A=X and B=Y,
		 * if so we'll cross that bridge when we come to it.
		 *
		 * The controller sends a stream of T=1 events while the
		 * panel is touched, followed by a single T=0 event.
		 */
		switch (p[0] & 0x06) {
		case 0x02:
			msk = 0x1f;
			break;
		case 0x04:
			msk = 0x3f;
			break;
		case 0x06:
			msk = 0x7f;
			break;
		default:
			msk = 0x0f;	/* H=0, L=0 */
		}
		x = uep_adjust(((p[3] & msk) << 7) | p[4], X_OFFSET, X_RATIO);
		y = uep_adjust(((p[1] & msk) << 7) | p[2], Y_OFFSET, Y_RATIO);

		tpcalib_trans(&sc->sc_tpcalib, x, y, &x, &y);

		s = spltty();
		wsmouse_input(sc->sc_wsmousedev, p[0] & 0x01, x, y, 0, 0,
			WSMOUSE_INPUT_ABSOLUTE_X | WSMOUSE_INPUT_ABSOLUTE_Y);
		splx(s);
	}
}
