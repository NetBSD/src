/*	$NetBSD: uplcom.c,v 1.11 2001/01/30 13:17:43 ichiro Exp $	*/
/*
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by FUKUHARA ichiro (ichiro@ichiro.org).
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

/*
 * Simple datasheet
 * http://www.prolific.com.tw/download/DataSheet/pl2303_ds11.PDF
 * http://www.nisseisg.co.jp/jyouhou/_cp/@gif/2303.pdf
 * 	(english)
 *
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/ioctl.h>
#include <sys/conf.h>
#include <sys/tty.h>
#include <sys/file.h>
#include <sys/select.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/device.h>
#include <sys/poll.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbcdc.h>

#include <dev/usb/usbdi.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbdevs.h>
#include <dev/usb/usb_quirks.h>

#include <dev/usb/usbdevs.h>
#include <dev/usb/ucomvar.h>

#ifdef UPLCOM_DEBUG
#define DPRINTFN(n, x)  if (uplcomdebug > (n)) logprintf x
int	uplcomdebug = 0xff;
#else
#define DPRINTFN(n, x)
#endif
#define DPRINTF(x) DPRINTFN(0, x)

#define	UPLCOM_CONFIG_INDEX	0
#define	UPLCOM_IFACE_INDEX	0
#define	UPLCOM_SECOND_IFACE_INDEX	1
#define	UPLCOM_RESET		0

struct	uplcom_softc {
	USBBASEDEVICE		sc_dev;		/* base device */
	usbd_device_handle	sc_udev;	/* USB device */
	usbd_interface_handle	sc_iface;	/* first interface */
	usbd_interface_handle	sc_sec_iface;	/* second interface */
	int			sc_iface_number;	/* interface number */

	usb_cdc_line_state_t	sc_line_state;	/* current line state */
	u_char			sc_dtr;		/* current DTR state */
	u_char			sc_rts;		/* current RTS state */

	device_ptr_t		sc_subdev;

	u_char			sc_dying;
};

/*
 * These are the maximum number of bytes transferred per frame.
 * The output buffer size cannot be increased due to the size encoding.
 */
#define UPLCOMIBUFSIZE 256
#define UPLCOMOBUFSIZE 256

Static	usbd_status uplcom_reset(struct uplcom_softc *);
Static	usbd_status uplcom_set_line_coding(struct uplcom_softc *sc,
					   usb_cdc_line_state_t *state);

Static	void	uplcom_set(void *, int, int, int);
Static	void	uplcom_dtr(struct uplcom_softc *, int);
Static	void	uplcom_rts(struct uplcom_softc *, int);
Static	void	uplcom_break(struct uplcom_softc *, int);
Static	void	uplcom_set_line_state(struct uplcom_softc *);
Static	int	uplcom_param(void *, int, struct termios *);
Static	int	uplcom_open(void *, int);
Static	void	uplcom_close(void *, int);

struct	ucom_methods uplcom_methods = {
	NULL,
	uplcom_set,
	uplcom_param,
	NULL,
	uplcom_open,
	uplcom_close,
	NULL,
	NULL,
};

static const struct uplcom_product {
	uint16_t	vendor;
	uint16_t	product;
} uplcom_products [] = {
	/* I/O DATA USB-RSAQ2 */
	{ USB_VENDOR_PROLIFIC, USB_PRODUCT_PROLIFIC_PL2303 },
	{ USB_VENDOR_IODATA, USB_PRODUCT_IODATA_USBRSAQ },

	{ 0, 0 }
};

USB_DECLARE_DRIVER(uplcom);

USB_MATCH(uplcom)
{
	USB_MATCH_START(uplcom, uaa);
	int i;

	if (uaa->iface != NULL)
		return (UMATCH_NONE);

	for (i = 0; uplcom_products[i].vendor != 0; i++) {
		if (uplcom_products[i].vendor == uaa->vendor &&
 		    uplcom_products[i].product == uaa->product) {
			return (UMATCH_VENDOR_PRODUCT);
		}
	}
	return (UMATCH_NONE);
}

USB_ATTACH(uplcom)
{
	USB_ATTACH_START(uplcom, sc, uaa);
	usbd_device_handle dev = uaa->device;
	usbd_interface_handle iface;
	usb_config_descriptor_t *cdesc;
	usb_interface_descriptor_t *id;
	usb_endpoint_descriptor_t *ed;
	
	char devinfo[1024];
	char *devname = USBDEVNAME(sc->sc_dev);
	usbd_status err;
	int i;
	struct ucom_attach_args uca;

        usbd_devinfo(dev, 0, devinfo);
        USB_ATTACH_SETUP;
        printf("%s: %s\n", devname, devinfo);

        sc->sc_udev = dev;

	DPRINTF(("\n\nuplcom attach: sc=%p\n", sc));

	/* Move the device into the configured state. */
	err = usbd_set_config_index(dev, UPLCOM_CONFIG_INDEX, 1);
	if (err) {
		printf("\n%s: failed to set configuration, err=%s\n",
			devname, usbd_errstr(err));
		USB_ATTACH_ERROR_RETURN;
	}

	/* get the config descriptor */
	cdesc = usbd_get_config_descriptor(sc->sc_udev);

	if (cdesc == NULL) {
		printf("%s: failed to get configuration descriptor\n",
			USBDEVNAME(sc->sc_dev));
		USB_ATTACH_ERROR_RETURN;
	}

	/* get the (first) interface */
	err = usbd_device2interface_handle(dev, UPLCOM_IFACE_INDEX, 
							&sc->sc_iface);
	if (err) {
		printf("\n%s: failed to get interface, err=%s\n",
			devname, usbd_errstr(err));
		USB_ATTACH_ERROR_RETURN;
	}
	/*
	 * check second of interface
	 * USB-RSAQ has two interface
	 */
	if (cdesc->bNumInterface == 2) {
		err = usbd_device2interface_handle(dev, 
				UPLCOM_SECOND_IFACE_INDEX, &sc->sc_sec_iface);
		if (err) {
			printf("\n%s: failed to get second interface, err=%s\n",
							devname, usbd_errstr(err));
			USB_ATTACH_ERROR_RETURN;
		}
		iface = sc->sc_sec_iface;
	} else 
		iface = sc->sc_iface;

	/* Find the bulk endpoints */

	id = usbd_get_interface_descriptor(iface);
	sc->sc_iface_number = id->bInterfaceNumber;

	uca.bulkin = uca.bulkout = -1;
	for (i = 0; i < id->bNumEndpoints; i++) {
		ed = usbd_interface2endpoint_descriptor(iface, i);
		if (ed == NULL) {
			printf("%s: no endpoint descriptor for %d\n",
				USBDEVNAME(sc->sc_dev), i);
			USB_ATTACH_ERROR_RETURN;
		}

		if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_IN &&
		    UE_GET_XFERTYPE(ed->bmAttributes) == UE_BULK) {
			uca.bulkin = ed->bEndpointAddress;
		} else if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_IN &&
		    UE_GET_XFERTYPE(ed->bmAttributes) == UE_INTERRUPT) {
			DPRINTF(("interrupt endpoint addr = 0x%x\n",
				 ed->bEndpointAddress));
		} else if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_OUT &&
		    UE_GET_XFERTYPE(ed->bmAttributes) == UE_BULK) {
			uca.bulkout = ed->bEndpointAddress;
		}
	}

	if (uca.bulkin == -1) {
		printf("%s: Could not find data bulk in\n",
			USBDEVNAME(sc->sc_dev));
		USB_ATTACH_ERROR_RETURN;
	}

	if (uca.bulkout == -1) {
		printf("%s: Could not find data bulk out\n",
			USBDEVNAME(sc->sc_dev));
		USB_ATTACH_ERROR_RETURN;
	}

	sc->sc_dtr = -1;
	uca.portno = UCOM_UNK_PORTNO;
	/* bulkin, bulkout set above */
	uca.ibufsize = UPLCOMIBUFSIZE;
	uca.obufsize = UPLCOMOBUFSIZE;
	uca.ibufsizepad = UPLCOMIBUFSIZE;
	uca.opkthdrlen = 0;
	uca.device = dev;
	uca.iface = iface;
	uca.methods = &uplcom_methods;
	uca.arg = sc;
	uca.info = NULL;

	err = uplcom_reset(sc);

	if (err) {
		printf("%s: reset failed, %s\n", USBDEVNAME(sc->sc_dev),
			usbd_errstr(err));
		USB_ATTACH_ERROR_RETURN;
	}

	usbd_add_drv_event(USB_EVENT_DRIVER_ATTACH, sc->sc_udev,
			   USBDEV(sc->sc_dev));

	DPRINTF(("uplcom: in=0x%x out=0x%x\n", uca.bulkin, uca.bulkout));
	sc->sc_subdev = config_found_sm(self, &uca, ucomprint, ucomsubmatch);

	USB_ATTACH_SUCCESS_RETURN;
}

USB_DETACH(uplcom)
{
	USB_DETACH_START(uplcom, sc);
	int rv = 0;

	DPRINTF(("uplcom_detach: sc=%p flags=%d\n", sc, flags));
	sc->sc_dying = 1;
	if (sc->sc_subdev != NULL) {
		rv = config_detach(sc->sc_subdev, flags);
		sc->sc_subdev = NULL;
	}

	usbd_add_drv_event(USB_EVENT_DRIVER_DETACH, sc->sc_udev,
			   USBDEV(sc->sc_dev));

	return (rv);
}

int
uplcom_activate(device_ptr_t self, enum devact act)
{
	struct uplcom_softc *sc = (struct uplcom_softc *)self;
	int rv = 0;

	switch (act) {
	case DVACT_ACTIVATE:
		return (EOPNOTSUPP);
		break;

	case DVACT_DEACTIVATE:
		if (sc->sc_subdev != NULL)
			rv = config_deactivate(sc->sc_subdev);
		sc->sc_dying = 1;
		break;
	}
	return (rv);
}


usbd_status
uplcom_reset(struct uplcom_softc *sc)
{
        usb_device_request_t req;
	usbd_status err;

        req.bmRequestType = UT_WRITE_CLASS_INTERFACE;
        req.bRequest = UPLCOM_RESET;
        USETW(req.wValue, UPLCOM_RESET);
        USETW(req.wIndex, sc->sc_iface_number);
        USETW(req.wLength, 0); 
 
        err = usbd_do_request(sc->sc_udev, &req, 0); 
	if (err)
		return (EIO);

	return (0);
}

void
uplcom_set_line_state(struct uplcom_softc *sc)
{
	usb_device_request_t req;
	int ls;

	ls = (sc->sc_dtr ? UCDC_LINE_DTR : 0) |
		(sc->sc_rts ? UCDC_LINE_RTS : 0);
	req.bmRequestType = UT_WRITE_CLASS_INTERFACE;
	req.bRequest = UCDC_SET_CONTROL_LINE_STATE;
	USETW(req.wValue, ls);
	USETW(req.wIndex, sc->sc_iface_number);
	USETW(req.wLength, 0);

	(void)usbd_do_request(sc->sc_udev, &req, 0);

}

void
uplcom_set(void *addr, int portno, int reg, int onoff)
{
	struct uplcom_softc *sc = addr;

	switch (reg) {
	case UCOM_SET_DTR:
		uplcom_dtr(sc, onoff);
		break;
	case UCOM_SET_RTS:
		uplcom_rts(sc, onoff);
		break;
	case UCOM_SET_BREAK:
		uplcom_break(sc, onoff);
		break;
	default:
		break;
	}
}

void
uplcom_dtr(struct uplcom_softc *sc, int onoff)
{

	DPRINTF(("uplcom_dtr: onoff=%d\n", onoff));

	if (sc->sc_dtr == onoff)
		return;
	sc->sc_dtr = onoff;

	uplcom_set_line_state(sc);
}

void
uplcom_rts(struct uplcom_softc *sc, int onoff)
{
	DPRINTF(("uplcom_rts: onoff=%d\n", onoff));

	if (sc->sc_rts == onoff)
		return;
	sc->sc_rts = onoff;

	uplcom_set_line_state(sc);
}

void
uplcom_break(struct uplcom_softc *sc, int onoff)
{
	usb_device_request_t req;

	DPRINTF(("uplcom_break: onoff=%d\n", onoff));

	req.bmRequestType = UT_WRITE_CLASS_INTERFACE;
	req.bRequest = UCDC_SEND_BREAK;
	USETW(req.wValue, onoff ? UCDC_BREAK_ON : UCDC_BREAK_OFF);
	USETW(req.wIndex, sc->sc_iface_number);
	USETW(req.wLength, 0);

	(void)usbd_do_request(sc->sc_udev, &req, 0);
}

usbd_status
uplcom_set_line_coding(struct uplcom_softc *sc, usb_cdc_line_state_t *state)
{
	usb_device_request_t req;
	usbd_status err;

	DPRINTF(("uplcom_set_line_coding: rate=%d fmt=%d parity=%d bits=%d\n",
		UGETDW(state->dwDTERate), state->bCharFormat,
		state->bParityType, state->bDataBits));

	if (memcmp(state, &sc->sc_line_state, UCDC_LINE_STATE_LENGTH) == 0) {
		DPRINTF(("uplcom_set_line_coding: already set\n"));
		return (USBD_NORMAL_COMPLETION);
	}

	req.bmRequestType = UT_WRITE_CLASS_INTERFACE;
	req.bRequest = UCDC_SET_LINE_CODING;
	USETW(req.wValue, 0);
	USETW(req.wIndex, sc->sc_iface_number);
	USETW(req.wLength, UCDC_LINE_STATE_LENGTH);

	err = usbd_do_request(sc->sc_udev, &req, state);
	if (err) {
		DPRINTF(("uplcom_set_line_coding: failed, err=%s\n",
			usbd_errstr(err)));
		return (err);
	}

	sc->sc_line_state = *state;

	return (USBD_NORMAL_COMPLETION);
}

int
uplcom_param(void *addr, int portno, struct termios *t)
{
	struct uplcom_softc *sc = addr;
	usbd_status err;
	usb_cdc_line_state_t ls;

	DPRINTF(("uplcom_param: sc=%p\n", sc));

	USETDW(ls.dwDTERate, t->c_ospeed);
	if (ISSET(t->c_cflag, CSTOPB))
		ls.bCharFormat = UCDC_STOP_BIT_2;
	else
		ls.bCharFormat = UCDC_STOP_BIT_1;
	if (ISSET(t->c_cflag, PARENB)) {
		if (ISSET(t->c_cflag, PARODD))
			ls.bParityType = UCDC_PARITY_ODD;
		else
			ls.bParityType = UCDC_PARITY_EVEN;
	} else
		ls.bParityType = UCDC_PARITY_NONE;
	switch (ISSET(t->c_cflag, CSIZE)) {
	case CS5:
		ls.bDataBits = 5;
		break;
	case CS6:
		ls.bDataBits = 6;
		break;
	case CS7:
		ls.bDataBits = 7;
		break;
	case CS8:
		ls.bDataBits = 8;
		break;
	}

	err = uplcom_set_line_coding(sc, &ls);
	if (err) {
		DPRINTF(("uplcom_param: err=%s\n", usbd_errstr(err)));
		return (EIO);
	}
	return (0);
}

int
uplcom_open(void *addr, int portno)
{
	struct uplcom_softc *sc = addr;
	usbd_status err;
	
	if (sc->sc_dying)
		return (EIO);

	err = uplcom_reset(sc);
        if (err) {
                printf("%s: reset failed, %s\n", USBDEVNAME(sc->sc_dev),
                        usbd_errstr(err));
        }

	DPRINTF(("uplcom_open: open\n"));

	return (0);
}

void
uplcom_close(void *addr, int portno) 
{
	struct uplcom_softc *sc = addr;
	usbd_status err;

	if (sc->sc_dying)
		return;

	err = uplcom_reset(sc);
	if (err) {
		printf("%s: reset failed, %s\n", USBDEVNAME(sc->sc_dev),
			usbd_errstr(err));
	}

	DPRINTF(("uplcom_close: close\n"));
}
