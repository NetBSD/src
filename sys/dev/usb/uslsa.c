/* $NetBSD: uslsa.c,v 1.2.2.2 2007/06/09 21:37:26 ad Exp $ */

/*
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jonathan A. Kollasch <jakllsch@kollasch.net>.  Craig Shelley's Linux
 * driver provided invaluable documentation.
 *
 * This code is derived from ugensa.c, software contributed to
 * The NetBSD Foundation by Roland C. Dowdeswell <elric@netbsd.org>.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: uslsa.c,v 1.2.2.2 2007/06/09 21:37:26 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/tty.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbhid.h>

#include <dev/usb/usbdi.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbdevs.h>

#include <dev/usb/ucomvar.h>

#ifdef DEBUG
#define USLSA_DEBUG
#endif

#ifdef USLSA_DEBUG
#define DPRINTF(x)	if (uslsadebug) printf x
#define DPRINTFN(n,x)	if (uslsadebug>(n)) printf x
int uslsadebug = 0;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif

#define USLSA_REQUEST_SET	0x41
#define USLSA_REQUEST_GET	0xc1

#define USLSA_REQ_SET_STATE	0x00

#define USLSA_REQ_SET_BPS	0x01
#define USLSA_REQ_GET_BPS	0x02

#define USLSA_REQ_SET_DPS	0x03
#define USLSA_REQ_GET_DPS	0x04

#define USLSA_REQ_SET_BREAK	0x05
#define USLSA_REQ_GET_BREAK	0x06

#define USLSA_REQ_SET_FLOW	0x07
#define USLSA_REQ_GET_FLOW	0x08

#define USLSA_REQ_SET_MODEM	0x13
#define USLSA_REQ_GET_MODEM	0x14

#define USLSA_REQ_SET_MISC	0x19
#define USLSA_REQ_GET_MISC	0x20

#define USLSA_STATE_DISABLE	0x0000
#define USLSA_STATE_ENABLE	0x0001

#define USLSA_BPS(b)	(3686400/b)

#define USLSA_DPS_DATA_MASK		0x0f00
#define	USLSA_DPS_DATA_FIVE		0x0500
#define	USLSA_DPS_DATA_SIX		0x0600
#define	USLSA_DPS_DATA_SEVEN		0x0700
#define	USLSA_DPS_DATA_EIGHT		0x0800
#define	USLSA_DPS_DATA_NINE		0x0900

#define USLSA_DPS_PARITY_MASK		0x00f0
#define USLSA_DPS_PARITY_SPACE		0x0040
#define USLSA_DPS_PARITY_MARK		0x0030
#define USLSA_DPS_PARITY_EVEN		0x0020
#define USLSA_DPS_PARITY_ODD		0x0010
#define USLSA_DPS_PARITY_NONE		0x0000

#define USLSA_DPS_STOP_MASK		0x000f
#define USLSA_DPS_STOP_TWO		0x0002
#define USLSA_DPS_STOP_ONE_FIVE		0x0001
#define USLSA_DPS_STOP_ONE		0x0000

#define USLSA_BREAK_DISABLE	0x0001
#define USLSA_BREAK_ENABLE	0x0000

#define USLSA_FLOW_SET_RTS	0x0200
#define USLSA_FLOW_SET_DTR	0x0100
#define USLSA_FLOW_MSR_MASK	0x00f0
#define USLSA_FLOW_MSR_DCD	0x0080
#define USLSA_FLOW_MSR_RI	0x0040
#define USLSA_FLOW_MSR_DSR	0x0020
#define USLSA_FLOW_MSR_CTS	0x0010
#define USLSA_FLOW_RTS		0x0002
#define USLSA_FLOW_DTR		0x0001

struct uslsa_softc {
	USBBASEDEVICE		sc_dev;		/* base device */
	usbd_device_handle	sc_udev;	/* device */
	usbd_interface_handle	sc_iface;	/* interface */

	device_ptr_t		sc_subdev;	/* ucom device */

	u_char			sc_dying;	/* disconnecting */

	u_char			sc_lsr;		/* local status register */
	u_char			sc_msr;		/* uslsa status register */
};

static void uslsa_get_status(void *sc, int, u_char *, u_char *);
static void uslsa_set(void *, int, int, int);
static int uslsa_param(void *, int, struct termios *);
static int uslsa_open(void *, int);
static void uslsa_close(void *, int);

static int uslsa_request_set(struct uslsa_softc *, uint8_t, uint16_t);
static void uslsa_set_flow(struct uslsa_softc *, tcflag_t, tcflag_t);

struct ucom_methods uslsa_methods = {
	uslsa_get_status,
	uslsa_set,
	uslsa_param,
	NULL,
	uslsa_open,
	uslsa_close,
	NULL,
	NULL,
};

#define USLSA_CONFIG_INDEX	0
#define USLSA_IFACE_INDEX	0
#define USLSA_BUFSIZE		256

static const struct usb_devno uslsa_devs[] = {
        { USB_VENDOR_BALTECH,           USB_PRODUCT_BALTECH_CARDREADER },
        { USB_VENDOR_DYNASTREAM,        USB_PRODUCT_DYNASTREAM_ANTDEVBOARD },
        { USB_VENDOR_JABLOTRON,         USB_PRODUCT_JABLOTRON_PC60B },
        { USB_VENDOR_SILABS,            USB_PRODUCT_SILABS_ARGUSISP },
        { USB_VENDOR_SILABS,            USB_PRODUCT_SILABS_CRUMB128 },
        { USB_VENDOR_SILABS,            USB_PRODUCT_SILABS_DEGREECONT },
        { USB_VENDOR_SILABS,            USB_PRODUCT_SILABS_DESKTOPMOBILE },
        { USB_VENDOR_SILABS,            USB_PRODUCT_SILABS_IPLINK1220 },
        { USB_VENDOR_SILABS,            USB_PRODUCT_SILABS_LIPOWSKY_HARP },
        { USB_VENDOR_SILABS,            USB_PRODUCT_SILABS_LIPOWSKY_JTAG },
        { USB_VENDOR_SILABS,            USB_PRODUCT_SILABS_LIPOWSKY_LIN },
        { USB_VENDOR_SILABS,            USB_PRODUCT_SILABS_POLOLU },
        { USB_VENDOR_SILABS,            USB_PRODUCT_SILABS_CP210X_1 },
        { USB_VENDOR_SILABS,            USB_PRODUCT_SILABS_CP210X_2 },
        { USB_VENDOR_SILABS,            USB_PRODUCT_SILABS_SUNNTO },
        { USB_VENDOR_SILABS2,           USB_PRODUCT_SILABS2_DCU11CLONE },
        { USB_VENDOR_USI,               USB_PRODUCT_USI_MC60 }
};
#define uslsa_lookup(v, p) usb_lookup(uslsa_devs, v, p)

USB_DECLARE_DRIVER(uslsa);

USB_MATCH(uslsa)
{
	USB_MATCH_START(uslsa, uaa);

	return (uslsa_lookup(uaa->vendor, uaa->product) != NULL ?
	        UMATCH_VENDOR_PRODUCT : UMATCH_NONE);
}

USB_ATTACH(uslsa)
{
	USB_ATTACH_START(uslsa, sc, uaa);
	usbd_device_handle dev = uaa->device;
	usbd_interface_handle iface;
	usb_interface_descriptor_t *id;
	usb_endpoint_descriptor_t *ed;
	char *devinfop;
	char *devname;
	usbd_status err;
	struct ucom_attach_args uca;
	int i;

	devname = USBDEVNAME(sc->sc_dev);

	DPRINTFN(10, ("\nuslsa_attach: sc=%p\n", sc));

	/* Move the device into the configured state. */
	err = usbd_set_config_index(dev, USLSA_CONFIG_INDEX, 1);
	if (err) {
		printf("\n%s: failed to set configuration, err=%s\n",
	 	       devname, usbd_errstr(err));
		goto bad;
	}

	err = usbd_device2interface_handle(dev, USLSA_IFACE_INDEX, &iface);
	if (err) {
		printf("\n%s: failed to get interface, err=%s\n",
		       devname, usbd_errstr(err));
		goto bad;
	}

	devinfop = usbd_devinfo_alloc(dev, 0);
	USB_ATTACH_SETUP;
	printf("%s: %s\n", devname, devinfop);
	usbd_devinfo_free(devinfop);

	id = usbd_get_interface_descriptor(iface);

	sc->sc_udev = dev;
	sc->sc_iface = iface;

	uca.info = "Silicon Labs CP210x";
	uca.ibufsize = USLSA_BUFSIZE;
	uca.obufsize = USLSA_BUFSIZE;
	uca.ibufsizepad = USLSA_BUFSIZE;
	uca.opkthdrlen = 0;
	uca.device = dev;
	uca.iface = iface;
	uca.methods = &uslsa_methods;
	uca.arg = sc;

	usbd_add_drv_event(USB_EVENT_DRIVER_ATTACH, sc->sc_udev,
	                   USBDEV(sc->sc_dev));

	uca.bulkin = uca.bulkout = -1;
	for (i = 0; i < id->bNumEndpoints; i++) {
		int addr, dir, attr;

		ed = usbd_interface2endpoint_descriptor(iface, i);
		if (ed == NULL) {
			printf("%s: could not read endpoint descriptor"
			       ": %s\n", devname, usbd_errstr(err));
			goto bad;
		}
		addr = ed->bEndpointAddress;
		dir = UE_GET_DIR(ed->bEndpointAddress);
		attr = ed->bmAttributes & UE_XFERTYPE;
		if (dir == UE_DIR_IN && attr == UE_BULK)
			uca.bulkin = addr;
		else if (dir == UE_DIR_OUT && attr == UE_BULK)
			uca.bulkout = addr;
		else
			printf("%s: unexpected endpoint\n", devname);
	}
	if (uca.bulkin == -1) {
		printf("%s: Could not find data bulk in\n",
		       USBDEVNAME(sc->sc_dev));
		goto bad;
	}
	if (uca.bulkout == -1) {
		printf("%s: Could not find data bulk out\n",
		       USBDEVNAME(sc->sc_dev));
		goto bad;
	}

	DPRINTF(("uslsa: in=0x%x out=0x%x\n", uca.bulkin, uca.bulkout));
	sc->sc_subdev = config_found_sm_loc(self, "ucombus", NULL, &uca,
	                                    ucomprint, ucomsubmatch);

	USB_ATTACH_SUCCESS_RETURN;

bad:
	DPRINTF(("uslsa_attach: ATTACH ERROR\n"));
	sc->sc_dying = 1;
	USB_ATTACH_ERROR_RETURN;
}

int
uslsa_activate(device_ptr_t self, enum devact act)
{
	struct uslsa_softc *sc = (struct uslsa_softc *) self;
	int rv = 0;

	switch (act) {
	case DVACT_ACTIVATE:
		return (EOPNOTSUPP);
		break;

	case DVACT_DEACTIVATE:
		sc->sc_dying = 1;
		if (sc->sc_subdev)
			rv = config_deactivate(sc->sc_subdev);
		break;
	}
	return (rv);
}

USB_DETACH(uslsa)
{
	USB_DETACH_START(uslsa, sc);
	int rv = 0;

	DPRINTF(("uslsa_detach: sc=%p flags=%d\n", sc, flags));

	sc->sc_dying = 1;

	if (sc->sc_subdev != NULL)
		rv = config_detach(sc->sc_subdev, flags);

	usbd_add_drv_event(USB_EVENT_DRIVER_DETACH, sc->sc_udev,
	                   USBDEV(sc->sc_dev));

	return (rv);
}

static void
uslsa_get_status(void *vsc, int portno, u_char *lsr, u_char *msr)
{
	struct uslsa_softc *sc;
	usb_device_request_t req;
	usbd_status err;
	int actlen;
	uint16_t flowreg;

	sc = vsc;

	DPRINTF(("uslsa_get_status:\n"));

	req.bmRequestType = USLSA_REQUEST_GET;
	req.bRequest = USLSA_REQ_GET_FLOW;
	USETW(req.wValue, 0);
	USETW(req.wIndex, 0);
	USETW(req.wLength, sizeof(flowreg));

	err = usbd_do_request_flags(sc->sc_udev, &req, &flowreg,
	    USBD_SHORT_XFER_OK, &actlen, USBD_DEFAULT_TIMEOUT);
	if (err)
		printf("%s: uslsa_get_status: %s\n",
		    USBDEVNAME(sc->sc_dev), usbd_errstr(err));

	DPRINTF(("uslsa_get_status: flowreg=0x%x\n", flowreg));

	sc->sc_msr = (u_char)(USLSA_FLOW_MSR_MASK & flowreg);

	if (lsr != NULL)
		*lsr = sc->sc_lsr;
	if (msr != NULL)
		*msr = sc->sc_msr;
}

static void
uslsa_set(void *vsc, int portno, int reg, int onoff)
{
	struct uslsa_softc *sc;

	sc = vsc;

	DPRINTF(("uslsa_set: sc=%p, port=%d reg=%d onoff=%d\n", sc, portno,
	         reg, onoff));

	switch (reg) {
	case UCOM_SET_DTR:
		if (uslsa_request_set(sc, USLSA_REQ_SET_FLOW,
			(onoff ? (USLSA_FLOW_DTR | USLSA_FLOW_SET_DTR) :
			    USLSA_FLOW_SET_DTR)))
			printf("%s: uslsa_set_dtr failed\n",
			       USBDEVNAME(sc->sc_dev));
		break;
	case UCOM_SET_RTS:
		if (uslsa_request_set(sc, USLSA_REQ_SET_FLOW,
			(onoff ? (USLSA_FLOW_RTS | USLSA_FLOW_SET_RTS) :
			    USLSA_FLOW_SET_RTS)))
			printf("%s: uslsa_set_rts failed\n",
			       USBDEVNAME(sc->sc_dev));
		break;
	case UCOM_SET_BREAK:
		if (uslsa_request_set(sc, USLSA_REQ_SET_BREAK,
			(onoff ? USLSA_BREAK_ENABLE :
			    USLSA_BREAK_DISABLE)))
			printf("%s: uslsa_set_break failed\n",
			       USBDEVNAME(sc->sc_dev));
		break;
	default:
		break;
	}
}

static int
uslsa_param(void *vsc, int portno, struct termios * t)
{
	struct uslsa_softc *sc;
	uint16_t data;

	sc = vsc;

	DPRINTF(("uslsa_param: sc=%p\n", sc));

	switch (t->c_ospeed) {
	case B600:
	case B1200:
	case B2400:
	case B4800:
	case B9600:
	case B19200:
	case B38400:
	case B57600:
	case B115200:
	case B230400:
	case B460800:
	case B921600:
		data = USLSA_BPS(t->c_ospeed);
		break;
	default:
		printf("%s: uslsa_param: unsupported data rate, "
		       "forcing default of 115200bps\n",
		       USBDEVNAME(sc->sc_dev));
		data = USLSA_BPS(B115200);
	};

	if (uslsa_request_set(sc, USLSA_REQ_SET_BPS, data))
		printf("%s: uslsa_param: setting data rate failed\n",
		       USBDEVNAME(sc->sc_dev));

	data = 0;

	if (ISSET(t->c_cflag, CSTOPB))
		data |= USLSA_DPS_STOP_TWO;
	else
		data |= USLSA_DPS_STOP_ONE;

	if (ISSET(t->c_cflag, PARENB)) {
		if (ISSET(t->c_cflag, PARODD))
			data |= USLSA_DPS_PARITY_ODD;
		else
			data |= USLSA_DPS_PARITY_EVEN;
	} else
		data |= USLSA_DPS_PARITY_NONE;

	switch (ISSET(t->c_cflag, CSIZE)) {
	case CS5:
		data |= USLSA_DPS_DATA_FIVE;
		break;
	case CS6:
		data |= USLSA_DPS_DATA_SIX;
		break;
	case CS7:
		data |= USLSA_DPS_DATA_SEVEN;
		break;
	case CS8:
		data |= USLSA_DPS_DATA_EIGHT;
		break;
	}

	DPRINTF(("uslsa_param: setting DPS register to 0x%x\n", data));
	if (uslsa_request_set(sc, USLSA_REQ_SET_DPS, data))
		printf("%s: setting DPS register failed: invalid argument\n",
		       USBDEVNAME(sc->sc_dev));

	uslsa_set_flow(sc, t->c_cflag, t->c_iflag);

	return 0;
}


static int
uslsa_open(void *vsc, int portno)
{
	struct uslsa_softc *sc;

	sc = vsc;

	DPRINTF(("uslsa_open: sc=%p\n", sc));

	if (sc->sc_dying)
		return (EIO);

	if (uslsa_request_set(sc, USLSA_REQ_SET_STATE, USLSA_STATE_ENABLE))
		return (EIO);

	return 0;
}

void
uslsa_close(void *vsc, int portno)
{
	struct uslsa_softc *sc;

	sc = vsc;

	if (sc->sc_dying)
		return;

	DPRINTF(("uslsa_close: sc=%p\n", sc));

	if (uslsa_request_set(sc, USLSA_REQ_SET_STATE,
	    USLSA_STATE_DISABLE))
		printf("%s: disable-on-close failed\n",
		       USBDEVNAME(sc->sc_dev));
}

/*
 * uslsa_request_set(), wrapper for doing sets with usbd_do_request()
 */
static int
uslsa_request_set(struct uslsa_softc * sc, uint8_t request, uint16_t value)
{
	usb_device_request_t req;
	usbd_status err;

	req.bmRequestType = USLSA_REQUEST_SET;
	req.bRequest = request;
	USETW(req.wValue, value);
	USETW(req.wIndex, 0);
	USETW(req.wLength, 0);

	err = usbd_do_request(sc->sc_udev, &req, 0);
	if (err)
		printf("%s: uslsa_request: %s\n",
		       USBDEVNAME(sc->sc_dev), usbd_errstr(err));
	return err;
}

/*
 * uslsa_set_flow() does some magic to set up hardware flow control
 */
static void
uslsa_set_flow(struct uslsa_softc *sc, tcflag_t cflag, tcflag_t iflag)
{
	uint8_t mysterydata[16];
	usb_device_request_t req;
	usbd_status err;

	DPRINTF(("uslsa_set_flow: cflag = 0x%x, iflag = 0x%x\n",
	         cflag, iflag));

	req.bmRequestType = USLSA_REQUEST_GET;
	req.bRequest = USLSA_REQ_GET_MODEM;
	USETW(req.wValue, 0);
	USETW(req.wIndex, 0);
	USETW(req.wLength, 16);

	err = usbd_do_request(sc->sc_udev, &req, mysterydata);
	if (err)
		printf("%s: uslsa_set_flow: %s\n",
		       USBDEVNAME(sc->sc_dev), usbd_errstr(err));

	if (ISSET(cflag, CRTSCTS)) {
		mysterydata[0] &= ~0x7b;
		mysterydata[0] |= 0x09;
		mysterydata[4] = 0x80;
	} else {
		mysterydata[0] &= ~0x7b;
		mysterydata[0] |= 0x01;
		mysterydata[4] = 0x40;
	}

	req.bmRequestType = USLSA_REQUEST_SET;
	req.bRequest = USLSA_REQ_SET_MODEM;
	USETW(req.wValue, 0);
	USETW(req.wIndex, 0);
	USETW(req.wLength, 16);

	err = usbd_do_request(sc->sc_udev, &req, mysterydata);
	if (err)
		printf("%s: uslsa_set_flow: %s\n",
		       USBDEVNAME(sc->sc_dev), usbd_errstr(err));
}
