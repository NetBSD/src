/*	$NetBSD: uxrcom.c,v 1.2 2020/07/09 13:43:04 simonb Exp $	*/
/*	$OpenBSD: uxrcom.c,v 1.1 2019/03/27 22:08:51 kettenis Exp $	*/

/*
 * Copyright (c) 1998, 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at
 * Carlstedt Research & Technology and Simon Burge.
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
 * Copyright (c) 2006 Jonathan Gray <jsg@openbsd.org>
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
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uxrcom.c,v 1.2 2020/07/09 13:43:04 simonb Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/tty.h>
#include <sys/device.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbcdc.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbdevs.h>
#include <dev/usb/usbhist.h>

#include <dev/usb/usbdevs.h>
#include <dev/usb/ucomvar.h>
#include <dev/usb/umodemvar.h>


#define UXRCOMBUFSZ		64

/* XXX uxrcomreg.h */
#define XR_SET_REG		0
#define XR_GET_REGN		1

#define XR_FLOW_CONTROL		0x000c
#define  XR_FLOW_CONTROL_ON	1
#define  XR_FLOW_CONTROL_OFF	0
#define XR_TX_BREAK		0x0014
#define  XR_TX_BREAK_ON		1
#define  XR_TX_BREAK_OFF	0
#define XR_GPIO_SET		0x001d
#define XR_GPIO_CLEAR		0x001e
#define  XR_GPIO3		(1 << 3)
#define  XR_GPIO5		(1 << 5)

/* for XR_SET_REG/XR_GET_REGN specify which uart block to use */
#define	XR_UART_BLOCK(sc)	(((sc)->sc_ctl_iface_no / 2) << NBBY)

#ifdef UXRCOM_DEBUG
#define	DPRINTF(x)	if (uxrcomdebug) printf x
int uxrcomdebug = 0;
#else
#define	DPRINTF(x)
#endif


static void	uxrcom_set(void *, int, int, int);
static int	uxrcom_param(void *, int, struct termios *);
static void	uxrcom_break(void *, int, int);

static const struct	ucom_methods uxrcom_methods = {
	.ucom_get_status = umodem_get_status,
	.ucom_set = uxrcom_set,
	.ucom_param = uxrcom_param,
	.ucom_ioctl = NULL,	/* TODO */
	.ucom_open = umodem_open,
	.ucom_close = umodem_close,
};

static const struct usb_devno uxrcom_devs[] = {
	{ USB_VENDOR_EXAR,	USB_PRODUCT_EXAR_XR21V1410 },
	{ USB_VENDOR_EXAR,	USB_PRODUCT_EXAR_XR21V1412 },
	{ USB_VENDOR_EXAR,	USB_PRODUCT_EXAR_XR21V1414 },
};
#define uxrcom_lookup(v, p) usb_lookup(uxrcom_devs, v, p)

static int uxrcom_match(device_t, cfdata_t, void *);
static void uxrcom_attach(device_t, device_t, void *);
static int uxrcom_detach(device_t, int);

CFATTACH_DECL_NEW(uxrcom, sizeof(struct umodem_softc), uxrcom_match,
    uxrcom_attach, uxrcom_detach, NULL);

static int
uxrcom_match(device_t parent, cfdata_t match, void *aux)
{
	struct usbif_attach_arg *uiaa = aux;

	if (uiaa->uiaa_class != UICLASS_CDC ||
	    uiaa->uiaa_subclass != UISUBCLASS_ABSTRACT_CONTROL_MODEL ||
	    !(uiaa->uiaa_proto == UIPROTO_CDC_NOCLASS ||
	      uiaa->uiaa_proto == UIPROTO_CDC_AT))
		return UMATCH_NONE;

	return uxrcom_lookup(uiaa->uiaa_vendor, uiaa->uiaa_product) != NULL ?
		UMATCH_VENDOR_PRODUCT : UMATCH_NONE;
}

static void
uxrcom_attach(device_t parent, device_t self, void *aux)
{
	struct umodem_softc *sc = device_private(self);
	struct usbif_attach_arg *uiaa = aux;
	struct ucom_attach_args ucaa;

	memset(&ucaa, 0, sizeof(ucaa));

	ucaa.ucaa_portno = UCOM_UNK_PORTNO;
	ucaa.ucaa_methods = &uxrcom_methods;
	ucaa.ucaa_info = NULL;

	ucaa.ucaa_ibufsize = UXRCOMBUFSZ;
	ucaa.ucaa_obufsize = UXRCOMBUFSZ;
	ucaa.ucaa_ibufsizepad = UXRCOMBUFSZ;

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler");

	umodem_common_attach(self, sc, uiaa, &ucaa);
}

static int
uxrcom_detach(device_t self, int flags)
{
	struct umodem_softc *sc = device_private(self);

	pmf_device_deregister(self);

	return umodem_common_detach(sc, flags);
}

static void
uxrcom_set(void *addr, int portno, int reg, int onoff)
{
	struct umodem_softc *sc = addr;
	usb_device_request_t req;
	uint16_t index;
	uint8_t value;

	if (sc->sc_dying)
		return;

	index = onoff ? XR_GPIO_SET : XR_GPIO_CLEAR;

	switch (reg) {
	case UCOM_SET_DTR:
		value = XR_GPIO3;
		break;
	case UCOM_SET_RTS:
		value = XR_GPIO5;
		break;
	case UCOM_SET_BREAK:
		uxrcom_break(sc, portno, onoff);
		return;
	default:
		return;
	}
	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = XR_SET_REG;
	USETW(req.wValue, value);
	USETW(req.wIndex, index | XR_UART_BLOCK(sc));
	USETW(req.wLength, 0);
	usbd_do_request(sc->sc_udev, &req, NULL);
}

static usbd_status
uxrcom_set_line_coding(struct umodem_softc *sc, usb_cdc_line_state_t *state)
{
	usb_device_request_t req;
	usbd_status err;

	DPRINTF(("%s: rate=%d fmt=%d parity=%d bits=%d\n", __func__,
		UGETDW(state->dwDTERate), state->bCharFormat,
		state->bParityType, state->bDataBits));

	if (memcmp(state, &sc->sc_line_state, UCDC_LINE_STATE_LENGTH) == 0) {
		DPRINTF(("%s: already set\n", __func__));
		return USBD_NORMAL_COMPLETION;
	}

	req.bmRequestType = UT_WRITE_CLASS_INTERFACE;
	req.bRequest = UCDC_SET_LINE_CODING;
	USETW(req.wValue, 0);
	USETW(req.wIndex, sc->sc_ctl_iface_no);
	USETW(req.wLength, UCDC_LINE_STATE_LENGTH);

	err = usbd_do_request(sc->sc_udev, &req, state);
	if (err) {
		DPRINTF(("%s: failed, err=%u\n", __func__, err));
		return err;
	}

	sc->sc_line_state = *state;

	return USBD_NORMAL_COMPLETION;
}

static int
uxrcom_param(void *addr, int portno, struct termios *t)
{
	struct umodem_softc *sc = addr;
	usb_device_request_t req;
	usbd_status err;
	usb_cdc_line_state_t ls;
	uint8_t flowctrl;

	if (sc->sc_dying)
		return EIO;

	/* slowest supported baud rate is 1200 bps, max is 12 Mbps */
	if (t->c_ospeed < 1200 || t->c_ospeed > 12000000)
		return (EINVAL);

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

	err = uxrcom_set_line_coding(sc, &ls);
	if (err) {
		DPRINTF(("%s: err=%u\n", __func__, err));
		return EIO;
	}

	if (ISSET(t->c_cflag, CRTSCTS)) {
		/*  rts/cts flow ctl */
		flowctrl = XR_FLOW_CONTROL_ON;
	} else {
		/* disable flow ctl */
		flowctrl = XR_FLOW_CONTROL_OFF;
	}
	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = XR_SET_REG;
	USETW(req.wValue, flowctrl);
	USETW(req.wIndex, XR_FLOW_CONTROL | XR_UART_BLOCK(sc));
	USETW(req.wLength, 0);
	usbd_do_request(sc->sc_udev, &req, NULL);

	return (0);
}

static void
uxrcom_break(void *addr, int portno, int onoff)
{
	struct umodem_softc *sc = addr;
	usb_device_request_t req;
	uint8_t brk = onoff ? UCDC_BREAK_ON : UCDC_BREAK_OFF;

	DPRINTF(("%s: port=%d onoff=%d\n", __func__, portno, onoff));

	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = XR_SET_REG;
	USETW(req.wValue, brk);
	USETW(req.wIndex, XR_TX_BREAK | XR_UART_BLOCK(sc));
	USETW(req.wLength, 0);

	(void)usbd_do_request(sc->sc_udev, &req, 0);
}
