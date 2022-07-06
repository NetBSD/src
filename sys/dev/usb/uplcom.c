/*	$NetBSD: uplcom.c,v 1.92 2022/07/06 06:00:40 nat Exp $	*/

/*
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ichiro FUKUHARA (ichiro@ichiro.org).
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
 * General information: http://www.prolific.com.tw/fr_pl2303.htm
 * http://www.hitachi-hitec.com/jyouhou/prolific/2303.pdf
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uplcom.c,v 1.92 2022/07/06 06:00:40 nat Exp $");

#ifdef _KERNEL_OPT
#include "opt_usb.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/ioctl.h>
#include <sys/conf.h>
#include <sys/tty.h>
#include <sys/file.h>
#include <sys/select.h>
#include <sys/proc.h>
#include <sys/device.h>
#include <sys/poll.h>
#include <sys/sysctl.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbcdc.h>

#include <dev/usb/usbdi.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbdevs.h>
#include <dev/usb/usb_quirks.h>
#include <dev/usb/usbhist.h>

#include <dev/usb/ucomvar.h>

#ifdef USB_DEBUG
#ifndef UPLCOM_DEBUG
#define uplcomdebug 0
#else
int	uplcomdebug = 0;

SYSCTL_SETUP(sysctl_hw_uplcom_setup, "sysctl hw.uplcom setup")
{
	int err;
	const struct sysctlnode *rnode;
	const struct sysctlnode *cnode;

	err = sysctl_createv(clog, 0, NULL, &rnode,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "uplcom",
	    SYSCTL_DESCR("uplcom global controls"),
	    NULL, 0, NULL, 0, CTL_HW, CTL_CREATE, CTL_EOL);

	if (err)
		goto fail;

	/* control debugging printfs */
	err = sysctl_createv(clog, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_INT,
	    "debug", SYSCTL_DESCR("Enable debugging output"),
	    NULL, 0, &uplcomdebug, sizeof(uplcomdebug), CTL_CREATE, CTL_EOL);
	if (err)
		goto fail;

	return;
fail:
	aprint_error("%s: sysctl_createv failed (err = %d)\n", __func__, err);
}

#endif /* UCOM_DEBUG */
#endif /* USB_DEBUG */


#define DPRINTF(FMT,A,B,C,D)    USBHIST_LOGN(uplcomdebug,1,FMT,A,B,C,D)
#define DPRINTFN(N,FMT,A,B,C,D) USBHIST_LOGN(uplcomdebug,N,FMT,A,B,C,D)
#define UPLCOMHIST_FUNC()       USBHIST_FUNC()
#define UPLCOMHIST_CALLED(name) USBHIST_CALLED(uplcomdebug)

#define	UPLCOM_CONFIG_INDEX	0
#define	UPLCOM_IFACE_INDEX	0
#define	UPLCOM_SECOND_IFACE_INDEX	1

#define	UPLCOM_SET_REQUEST		0x01
#define	UPLCOM_SET_CRTSCTS_0		0x41
#define	UPLCOM_SET_CRTSCTS_HX		0x61

#define	UPLCOM_N_SERIAL_CTS		0x80

#define UPLCOM_HXN_SET_REQUEST		0x80
#define UPLCOM_HXN_SET_CRTSCTS_REG	0x0A
#define UPLCOM_HXN_SET_CRTSCTS		0xFA
#define UPLCOM_HX_STATUS_REG		0x8080

enum  pl2303_type {
	UPLCOM_TYPE_0,	/* we use this for all non-HX variants */
	UPLCOM_TYPE_HX,
	UPLCOM_TYPE_HXN,
};

struct	uplcom_softc {
	device_t		sc_dev;		/* base device */
	struct usbd_device *	sc_udev;	/* USB device */
	struct usbd_interface *	sc_iface;	/* interface */
	int			sc_iface_number;	/* interface number */

	struct usbd_interface *	sc_intr_iface;	/* interrupt interface */
	int			sc_intr_number;	/* interrupt number */
	struct usbd_pipe *	sc_intr_pipe;	/* interrupt pipe */
	u_char			*sc_intr_buf;	/* interrupt buffer */
	int			sc_isize;

	usb_cdc_line_state_t	sc_line_state;	/* current line state */
	int			sc_dtr;		/* current DTR state */
	int			sc_rts;		/* current RTS state */

	device_t		sc_subdev;	/* ucom device */

	bool			sc_dying;	/* disconnecting */

	u_char			sc_lsr;		/* Local status register */
	u_char			sc_msr;		/* uplcom status register */

	enum pl2303_type	sc_type;	/* PL2303 chip type */
};

/*
 * These are the maximum number of bytes transferred per frame.
 * The output buffer size cannot be increased due to the size encoding.
 */
#define UPLCOMIBUFSIZE 256
#define UPLCOMOBUFSIZE 256

static	usbd_status uplcom_reset(struct uplcom_softc *);
static	usbd_status uplcom_set_line_coding(struct uplcom_softc *,
					   usb_cdc_line_state_t *);
static	usbd_status uplcom_set_crtscts(struct uplcom_softc *);
static	void uplcom_intr(struct usbd_xfer *, void *, usbd_status);

static	void uplcom_set(void *, int, int, int);
static	void uplcom_dtr(struct uplcom_softc *, int);
static	void uplcom_rts(struct uplcom_softc *, int);
static	void uplcom_break(struct uplcom_softc *, int);
static	void uplcom_set_line_state(struct uplcom_softc *);
static	void uplcom_get_status(void *, int, u_char *, u_char *);
#if TODO
static	int  uplcom_ioctl(void *, int, u_long, void *, int, proc_t *);
#endif
static	int  uplcom_param(void *, int, struct termios *);
static	int  uplcom_open(void *, int);
static	void uplcom_close(void *, int);
static usbd_status uplcom_vendor_control_write(struct usbd_device *, uint16_t, uint16_t);
static void uplcom_close_pipe(struct uplcom_softc *);

static const struct	ucom_methods uplcom_methods = {
	.ucom_get_status = uplcom_get_status,
	.ucom_set = uplcom_set,
	.ucom_param = uplcom_param,
	.ucom_ioctl = NULL,	/* TODO */
	.ucom_open = uplcom_open,
	.ucom_close = uplcom_close,
};

static const struct usb_devno uplcom_devs[] = {
	/* I/O DATA USB-RSAQ2 */
	{ USB_VENDOR_PROLIFIC, USB_PRODUCT_PROLIFIC_RSAQ2 },
	/* I/O DATA USB-RSAQ3 */
	{ USB_VENDOR_PROLIFIC, USB_PRODUCT_PROLIFIC_RSAQ3 },
	/* I/O DATA USB-RSAQ */
	{ USB_VENDOR_IODATA, USB_PRODUCT_IODATA_USBRSAQ },
	/* I/O DATA USB-RSAQ5 */
	{ USB_VENDOR_IODATA, USB_PRODUCT_IODATA_USBRSAQ5 },
	/* PLANEX USB-RS232 URS-03 */
	{ USB_VENDOR_ATEN, USB_PRODUCT_ATEN_UC232A },
	/* various */
	{ USB_VENDOR_PROLIFIC, USB_PRODUCT_PROLIFIC_PL2303 },
	/* SMART Technologies USB to serial */
	{ USB_VENDOR_PROLIFIC2, USB_PRODUCT_PROLIFIC2_PL2303 },
	/* IOGEAR/ATENTRIPPLITE */
	{ USB_VENDOR_TRIPPLITE, USB_PRODUCT_TRIPPLITE_U209 },
	/* ELECOM UC-SGT */
	{ USB_VENDOR_ELECOM, USB_PRODUCT_ELECOM_UCSGT },
	/* ELECOM UC-SGT0 */
	{ USB_VENDOR_ELECOM, USB_PRODUCT_ELECOM_UCSGT0 },
	/* Panasonic 50" Touch Panel */
	{ USB_VENDOR_PANASONIC, USB_PRODUCT_PANASONIC_TYTP50P6S },
	/* RATOC REX-USB60 */
	{ USB_VENDOR_RATOC, USB_PRODUCT_RATOC_REXUSB60 },
	/* TDK USB-PHS Adapter UHA6400 */
	{ USB_VENDOR_TDK, USB_PRODUCT_TDK_UHA6400 },
	/* TDK USB-PDC Adapter UPA9664 */
	{ USB_VENDOR_TDK, USB_PRODUCT_TDK_UPA9664 },
	/* Sony Ericsson USB Cable */
	{ USB_VENDOR_SUSTEEN, USB_PRODUCT_SUSTEEN_DCU10 },
	/* SOURCENEXT KeikaiDenwa 8 */
	{ USB_VENDOR_SOURCENEXT, USB_PRODUCT_SOURCENEXT_KEIKAI8 },
	/* SOURCENEXT KeikaiDenwa 8 with charger */
	{ USB_VENDOR_SOURCENEXT, USB_PRODUCT_SOURCENEXT_KEIKAI8_CHG },
	/* HAL Corporation Crossam2+USB */
	{ USB_VENDOR_HAL, USB_PRODUCT_HAL_IMR001 },
	/* Sitecom USB to serial cable */
	{ USB_VENDOR_SITECOM, USB_PRODUCT_SITECOM_CN104 },
	/* Pharos USB GPS - Microsoft version */
	{ USB_VENDOR_PROLIFIC, USB_PRODUCT_PROLIFIC_PL2303X },
	/* Willcom WS002IN (DD) */
	{ USB_VENDOR_NETINDEX, USB_PRODUCT_NETINDEX_WS002IN },
	/* COREGA CG-USBRS232R */
	{ USB_VENDOR_COREGA, USB_PRODUCT_COREGA_CGUSBRS232R },
	/* Sharp CE-175TU (USB to Zaurus option port 15 adapter) */
	{ USB_VENDOR_SHARP, USB_PRODUCT_SHARP_CE175TU },
	/* Various */
	{ USB_VENDOR_PROLIFIC, USB_PRODUCT_PROLIFIC_PL2303GB },
	{ USB_VENDOR_PROLIFIC, USB_PRODUCT_PROLIFIC_PL2303GC },
	{ USB_VENDOR_PROLIFIC, USB_PRODUCT_PROLIFIC_PL2303GE },
	{ USB_VENDOR_PROLIFIC, USB_PRODUCT_PROLIFIC_PL2303GL },
	{ USB_VENDOR_PROLIFIC, USB_PRODUCT_PROLIFIC_PL2303GS },
	{ USB_VENDOR_PROLIFIC, USB_PRODUCT_PROLIFIC_PL2303GT },
};
#define uplcom_lookup(v, p) usb_lookup(uplcom_devs, v, p)

static int uplcom_match(device_t, cfdata_t, void *);
static void uplcom_attach(device_t, device_t, void *);
static void uplcom_childdet(device_t, device_t);
static int uplcom_detach(device_t, int);

CFATTACH_DECL2_NEW(uplcom, sizeof(struct uplcom_softc), uplcom_match,
    uplcom_attach, uplcom_detach, NULL, NULL, uplcom_childdet);

static int
uplcom_match(device_t parent, cfdata_t match, void *aux)
{
	struct usb_attach_arg *uaa = aux;

	return uplcom_lookup(uaa->uaa_vendor, uaa->uaa_product) != NULL ?
		UMATCH_VENDOR_PRODUCT : UMATCH_NONE;
}

static void
uplcom_attach(device_t parent, device_t self, void *aux)
{
	struct uplcom_softc *sc = device_private(self);
	struct usb_attach_arg *uaa = aux;
	struct usbd_device *dev = uaa->uaa_device;
	usb_device_descriptor_t *ddesc;
	usb_config_descriptor_t *cdesc;
	usb_interface_descriptor_t *id;
	usb_endpoint_descriptor_t *ed;
	usb_device_request_t req;
	char *devinfop;
	const char *devname = device_xname(self);
	usbd_status err;
	uint8_t val;
	int i;
	struct ucom_attach_args ucaa;

	UPLCOMHIST_FUNC(); UPLCOMHIST_CALLED();
	DPRINTF("sc=%#jx", (uintptr_t)sc, 0, 0, 0);

	sc->sc_dev = self;
	sc->sc_dying = false;

	aprint_naive("\n");
	aprint_normal("\n");

	devinfop = usbd_devinfo_alloc(dev, 0);
	aprint_normal_dev(self, "%s\n", devinfop);
	usbd_devinfo_free(devinfop);

	sc->sc_udev = dev;

	/* initialize endpoints */
	ucaa.ucaa_bulkin = ucaa.ucaa_bulkout = -1;
	sc->sc_intr_number = -1;
	sc->sc_intr_pipe = NULL;

	/* Move the device into the configured state. */
	err = usbd_set_config_index(dev, UPLCOM_CONFIG_INDEX, 1);
	if (err) {
		aprint_error("\n%s: failed to set configuration, err=%s\n",
			devname, usbd_errstr(err));
		sc->sc_dying = true;
		return;
	}

	/* determine chip type */
	ddesc = usbd_get_device_descriptor(dev);
	if (ddesc->bDeviceClass != UDCLASS_COMM &&
	    ddesc->bMaxPacketSize == 0x40) {
		sc->sc_type = UPLCOM_TYPE_HX;
	}

	if (sc->sc_type == UPLCOM_TYPE_HX) {
		req.bmRequestType = UT_READ_VENDOR_DEVICE;
		req.bRequest = UPLCOM_SET_REQUEST;
		USETW(req.wValue, UPLCOM_HX_STATUS_REG);
		USETW(req.wIndex, sc->sc_iface_number);
		USETW(req.wLength, 1);

		err = usbd_do_request(sc->sc_udev, &req, &val);
		if (err)
			sc->sc_type = UPLCOM_TYPE_HXN;
	}

#ifdef UPLCOM_DEBUG
	/* print the chip type */
	if (sc->sc_type == UPLCOM_TYPE_HXN) {
		DPRINTF("chiptype HXN", 0, 0, 0, 0);
	else if (sc->sc_type == UPLCOM_TYPE_HX) {
		DPRINTF("chiptype HX", 0, 0, 0, 0);
	} else {
		DPRINTF("chiptype 0", 0, 0, 0, 0);
	}
#endif

	/* Move the device into the configured state. */
	err = usbd_set_config_index(dev, UPLCOM_CONFIG_INDEX, 1);
	if (err) {
		aprint_error_dev(self, "failed to set configuration: %s\n",
		    usbd_errstr(err));
		sc->sc_dying = true;
		return;
	}

	/* get the config descriptor */
	cdesc = usbd_get_config_descriptor(sc->sc_udev);

	if (cdesc == NULL) {
		aprint_error_dev(self,
		    "failed to get configuration descriptor\n");
		sc->sc_dying = true;
		return;
	}

	/* get the (first/common) interface */
	err = usbd_device2interface_handle(dev, UPLCOM_IFACE_INDEX,
							&sc->sc_iface);
	if (err) {
		aprint_error("\n%s: failed to get interface, err=%s\n",
			devname, usbd_errstr(err));
		sc->sc_dying = true;
		return;
	}

	/* Find the interrupt endpoints */

	id = usbd_get_interface_descriptor(sc->sc_iface);
	sc->sc_iface_number = id->bInterfaceNumber;

	for (i = 0; i < id->bNumEndpoints; i++) {
		ed = usbd_interface2endpoint_descriptor(sc->sc_iface, i);
		if (ed == NULL) {
			aprint_error_dev(self,
			    "no endpoint descriptor for %d\n", i);
			sc->sc_dying = true;
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
		sc->sc_dying = true;
		return;
	}

	/* keep interface for interrupt */
	sc->sc_intr_iface = sc->sc_iface;

	/*
	 * USB-RSAQ1 has two interface
	 *
	 *  USB-RSAQ1       | USB-RSAQ2
	 * -----------------+-----------------
	 * Interface 0      |Interface 0
	 *  Interrupt(0x81) | Interrupt(0x81)
	 * -----------------+ BulkIN(0x02)
	 * Interface 1	    | BulkOUT(0x83)
	 *   BulkIN(0x02)   |
	 *   BulkOUT(0x83)  |
	 */
	if (cdesc->bNumInterface == 2) {
		err = usbd_device2interface_handle(dev,
				UPLCOM_SECOND_IFACE_INDEX, &sc->sc_iface);
		if (err) {
			aprint_error("\n%s: failed to get second interface, "
			    "err=%s\n", devname, usbd_errstr(err));
			sc->sc_dying = true;
			return;
		}
	}

	/* Find the bulk{in,out} endpoints */

	id = usbd_get_interface_descriptor(sc->sc_iface);
	sc->sc_iface_number = id->bInterfaceNumber;

	for (i = 0; i < id->bNumEndpoints; i++) {
		ed = usbd_interface2endpoint_descriptor(sc->sc_iface, i);
		if (ed == NULL) {
			aprint_error_dev(self,
			    "no endpoint descriptor for %d\n", i);
			sc->sc_dying = true;
			return;
		}

		if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_IN &&
		    UE_GET_XFERTYPE(ed->bmAttributes) == UE_BULK) {
			ucaa.ucaa_bulkin = ed->bEndpointAddress;
		} else if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_OUT &&
		    UE_GET_XFERTYPE(ed->bmAttributes) == UE_BULK) {
			ucaa.ucaa_bulkout = ed->bEndpointAddress;
		}
	}

	if (ucaa.ucaa_bulkin == -1) {
		aprint_error_dev(self, "Could not find data bulk in\n");
		sc->sc_dying = true;
		return;
	}

	if (ucaa.ucaa_bulkout == -1) {
		aprint_error_dev(self, "Could not find data bulk out\n");
		sc->sc_dying = true;
		return;
	}

	sc->sc_dtr = sc->sc_rts = -1;
	ucaa.ucaa_portno = UCOM_UNK_PORTNO;
	/* ucaa_bulkin, ucaa_bulkout set above */
	ucaa.ucaa_ibufsize = UPLCOMIBUFSIZE;
	ucaa.ucaa_obufsize = UPLCOMOBUFSIZE;
	ucaa.ucaa_ibufsizepad = UPLCOMIBUFSIZE;
	ucaa.ucaa_opkthdrlen = 0;
	ucaa.ucaa_device = dev;
	ucaa.ucaa_iface = sc->sc_iface;
	ucaa.ucaa_methods = &uplcom_methods;
	ucaa.ucaa_arg = sc;
	ucaa.ucaa_info = NULL;

	err = uplcom_reset(sc);

	if (err) {
		aprint_error_dev(self, "reset failed, %s\n", usbd_errstr(err));
		sc->sc_dying = true;
		return;
	}

	usbd_add_drv_event(USB_EVENT_DRIVER_ATTACH, sc->sc_udev, sc->sc_dev);

	DPRINTF("in=%#jx out=%#jx intr=%#jx",
	    ucaa.ucaa_bulkin, ucaa.ucaa_bulkout, sc->sc_intr_number, 0);
	sc->sc_subdev = config_found(self, &ucaa, ucomprint,
	    CFARGS(.submatch = ucomsubmatch));

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");

	return;
}

static void
uplcom_childdet(device_t self, device_t child)
{
	struct uplcom_softc *sc = device_private(self);

	UPLCOMHIST_FUNC(); UPLCOMHIST_CALLED();

	KASSERT(sc->sc_subdev == child);
	sc->sc_subdev = NULL;
}
 
static void
uplcom_close_pipe(struct uplcom_softc *sc)
{

	if (sc->sc_intr_pipe != NULL) {
		usbd_abort_pipe(sc->sc_intr_pipe);
		usbd_close_pipe(sc->sc_intr_pipe);
		sc->sc_intr_pipe = NULL;
	}
	if (sc->sc_intr_buf != NULL) {
		kmem_free(sc->sc_intr_buf, sc->sc_isize);
		sc->sc_intr_buf = NULL;
	}
}

static int
uplcom_detach(device_t self, int flags)
{
	struct uplcom_softc *sc = device_private(self);
	int rv = 0;

	UPLCOMHIST_FUNC(); UPLCOMHIST_CALLED();
	DPRINTF("sc=%#jx flags=%jd", (uintptr_t)sc, flags, 0, 0);

	sc->sc_dying = true;
 
	uplcom_close_pipe(sc);

	if (sc->sc_subdev != NULL) {
		rv = config_detach(sc->sc_subdev, flags);
		sc->sc_subdev = NULL;
	}

	usbd_add_drv_event(USB_EVENT_DRIVER_DETACH, sc->sc_udev, sc->sc_dev);

	pmf_device_deregister(self);

	return rv;
}

usbd_status
uplcom_reset(struct uplcom_softc *sc)
{
	usb_device_request_t req;
	usbd_status err;

	if (sc->sc_type == UPLCOM_TYPE_HXN)
		return 0;

	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = UPLCOM_SET_REQUEST;
	USETW(req.wValue, 0);
	USETW(req.wIndex, sc->sc_iface_number);
	USETW(req.wLength, 0);

	err = usbd_do_request(sc->sc_udev, &req, 0);
	if (err)
		return EIO;

	return 0;
}

struct pl2303x_init {
	uint8_t		req_type;
	uint8_t		request;
	uint16_t	value;
	uint16_t	index;
};

static const struct pl2303x_init pl2303x[] = {
	{ UT_READ_VENDOR_DEVICE,  UPLCOM_SET_REQUEST, 0x8484,    0 },
	{ UT_WRITE_VENDOR_DEVICE, UPLCOM_SET_REQUEST, 0x0404,    0 },
	{ UT_READ_VENDOR_DEVICE,  UPLCOM_SET_REQUEST, 0x8484,    0 },
	{ UT_READ_VENDOR_DEVICE,  UPLCOM_SET_REQUEST, 0x8383,    0 },
	{ UT_READ_VENDOR_DEVICE,  UPLCOM_SET_REQUEST, 0x8484,    0 },
	{ UT_WRITE_VENDOR_DEVICE, UPLCOM_SET_REQUEST, 0x0404,    1 },
	{ UT_READ_VENDOR_DEVICE,  UPLCOM_SET_REQUEST, 0x8484,    0 },
	{ UT_READ_VENDOR_DEVICE,  UPLCOM_SET_REQUEST, 0x8383,    0 },
	{ UT_WRITE_VENDOR_DEVICE, UPLCOM_SET_REQUEST,      0,    1 },
	{ UT_WRITE_VENDOR_DEVICE, UPLCOM_SET_REQUEST,      1,    0 },
	{ UT_WRITE_VENDOR_DEVICE, UPLCOM_SET_REQUEST,      2, 0x44 }
};
#define N_PL2302X_INIT  (sizeof(pl2303x)/sizeof(pl2303x[0]))

static usbd_status
uplcom_pl2303x_init(struct uplcom_softc *sc)
{
	usb_device_request_t req;
	usbd_status err;
	int i;

	for (i = 0; i < N_PL2302X_INIT; i++) {
		char buf[1];
		void *b;

		req.bmRequestType = pl2303x[i].req_type;
		req.bRequest = pl2303x[i].request;
		USETW(req.wValue, pl2303x[i].value);
		USETW(req.wIndex, pl2303x[i].index);
		if (UT_GET_DIR(req.bmRequestType) == UT_READ) {
			b = buf;
			USETW(req.wLength, sizeof(buf));
		} else {
			b = NULL;
			USETW(req.wLength, 0);
		}

		err = usbd_do_request(sc->sc_udev, &req, b);
		if (err) {
			aprint_error_dev(sc->sc_dev,
			    "uplcom_pl2303x_init failed: %s\n",
			    usbd_errstr(err));
			return EIO;
		}
	}

	return 0;
}

static void
uplcom_set_line_state(struct uplcom_softc *sc)
{
	usb_device_request_t req;
	int ls;

	/* make sure we have initialized state for sc_dtr and sc_rts */
	if (sc->sc_dtr == -1)
		sc->sc_dtr = 0;
	if (sc->sc_rts == -1)
		sc->sc_rts = 0;

	ls = (sc->sc_dtr ? UCDC_LINE_DTR : 0) |
		(sc->sc_rts ? UCDC_LINE_RTS : 0);

	req.bmRequestType = UT_WRITE_CLASS_INTERFACE;
	req.bRequest = UCDC_SET_CONTROL_LINE_STATE;
	USETW(req.wValue, ls);
	USETW(req.wIndex, sc->sc_iface_number);
	USETW(req.wLength, 0);

	(void)usbd_do_request(sc->sc_udev, &req, 0);
}

static void
uplcom_set(void *addr, int portno, int reg, int onoff)
{
	struct uplcom_softc *sc = addr;

	if (sc->sc_dying)
		return;

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

static void
uplcom_dtr(struct uplcom_softc *sc, int onoff)
{

	UPLCOMHIST_FUNC(); UPLCOMHIST_CALLED();
	DPRINTF("onoff=%jd", onoff, 0, 0, 0);

	if (sc->sc_dtr != -1 && !sc->sc_dtr == !onoff)
		return;

	sc->sc_dtr = !!onoff;

	uplcom_set_line_state(sc);
}

static void
uplcom_rts(struct uplcom_softc *sc, int onoff)
{
	UPLCOMHIST_FUNC(); UPLCOMHIST_CALLED();
	DPRINTF("onoff=%jd", onoff, 0, 0, 0);

	if (sc->sc_rts != -1 && !sc->sc_rts == !onoff)
		return;

	sc->sc_rts = !!onoff;

	uplcom_set_line_state(sc);
}

static void
uplcom_break(struct uplcom_softc *sc, int onoff)
{
	usb_device_request_t req;

	UPLCOMHIST_FUNC(); UPLCOMHIST_CALLED();
	DPRINTF("onoff=%jd", onoff, 0, 0, 0);

	req.bmRequestType = UT_WRITE_CLASS_INTERFACE;
	req.bRequest = UCDC_SEND_BREAK;
	USETW(req.wValue, onoff ? UCDC_BREAK_ON : UCDC_BREAK_OFF);
	USETW(req.wIndex, sc->sc_iface_number);
	USETW(req.wLength, 0);

	(void)usbd_do_request(sc->sc_udev, &req, 0);
}

static usbd_status
uplcom_set_crtscts(struct uplcom_softc *sc)
{
	usb_device_request_t req;
	usbd_status err;

	UPLCOMHIST_FUNC(); UPLCOMHIST_CALLED();

	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	if (sc->sc_type == UPLCOM_TYPE_HXN) {
		req.bRequest = UPLCOM_HXN_SET_REQUEST;
		USETW(req.wValue, UPLCOM_HXN_SET_CRTSCTS_REG);
	} else {
		req.bRequest = UPLCOM_SET_REQUEST;
		USETW(req.wValue, 0);
	}

	if (sc->sc_type == UPLCOM_TYPE_HXN)
		USETW(req.wIndex, UPLCOM_HXN_SET_CRTSCTS);
	else if (sc->sc_type == UPLCOM_TYPE_HX)
		USETW(req.wIndex, UPLCOM_SET_CRTSCTS_HX);
	else
		USETW(req.wIndex, UPLCOM_SET_CRTSCTS_0);
	USETW(req.wLength, 0);

	err = usbd_do_request(sc->sc_udev, &req, 0);
	if (err) {
		DPRINTF("failed, err=%jd", err, 0, 0, 0);
		return err;
	}

	return USBD_NORMAL_COMPLETION;
}

static usbd_status
uplcom_set_line_coding(struct uplcom_softc *sc, usb_cdc_line_state_t *state)
{
	usb_device_request_t req;
	usbd_status err;

	UPLCOMHIST_FUNC(); UPLCOMHIST_CALLED();

	DPRINTF("rate=%jd fmt=%jd parity=%jd bits=%jd",
		UGETDW(state->dwDTERate), state->bCharFormat,
		state->bParityType, state->bDataBits);

	if (memcmp(state, &sc->sc_line_state, UCDC_LINE_STATE_LENGTH) == 0) {
		DPRINTF("already set", 0, 0, 0, 0);
		return USBD_NORMAL_COMPLETION;
	}

	req.bmRequestType = UT_WRITE_CLASS_INTERFACE;
	req.bRequest = UCDC_SET_LINE_CODING;
	USETW(req.wValue, 0);
	USETW(req.wIndex, sc->sc_iface_number);
	USETW(req.wLength, UCDC_LINE_STATE_LENGTH);

	err = usbd_do_request(sc->sc_udev, &req, state);
	if (err) {
		DPRINTF("failed, err=%ju", err, 0, 0, 0);
		return err;
	}

	sc->sc_line_state = *state;

	return USBD_NORMAL_COMPLETION;
}

static int
uplcom_param(void *addr, int portno, struct termios *t)
{
	struct uplcom_softc *sc = addr;
	usbd_status err;
	usb_cdc_line_state_t ls;

	UPLCOMHIST_FUNC(); UPLCOMHIST_CALLED();
	DPRINTF("sc=%#jx", (uintptr_t)sc, 0, 0, 0);

	if (sc->sc_dying)
		return EIO;

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
		DPRINTF("err=%jd", err, 0, 0, 0);
		return EIO;
	}

	if (ISSET(t->c_cflag, CRTSCTS))
		uplcom_set_crtscts(sc);

	if (sc->sc_rts == -1 || sc->sc_dtr == -1)
		uplcom_set_line_state(sc);

	if (err) {
		DPRINTF("err=%jd", err, 0, 0, 0);
		return EIO;
	}

	return 0;
}

static usbd_status
uplcom_vendor_control_write(struct usbd_device *dev, uint16_t value,
    uint16_t index)
{
	usb_device_request_t req;
	usbd_status err;

	UPLCOMHIST_FUNC(); UPLCOMHIST_CALLED();

	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = UPLCOM_SET_REQUEST;
	USETW(req.wValue, value);
	USETW(req.wIndex, index);
	USETW(req.wLength, 0);

	err = usbd_do_request(dev, &req, NULL);

	if (err) {
		DPRINTF("vendor write failed, err=%jd", err, 0, 0, 0);
	}

	return err;
}

static int
uplcom_open(void *addr, int portno)
{
	struct uplcom_softc *sc = addr;
	usbd_status err = 0;
 
	UPLCOMHIST_FUNC(); UPLCOMHIST_CALLED();
	DPRINTF("sc=%#jx", (uintptr_t)sc, 0, 0, 0);

	if (sc->sc_dying)
		return EIO;

	/* Some unknown device frobbing. */
	if (sc->sc_type == UPLCOM_TYPE_HX)
		uplcom_vendor_control_write(sc->sc_udev, 2, 0x44);
	else
		uplcom_vendor_control_write(sc->sc_udev, 2, 0x24);

	if (sc->sc_intr_number != -1 && sc->sc_intr_pipe == NULL) {
		sc->sc_intr_buf = kmem_alloc(sc->sc_isize, KM_SLEEP);
		err = usbd_open_pipe_intr(sc->sc_intr_iface, sc->sc_intr_number,
			USBD_SHORT_XFER_OK, &sc->sc_intr_pipe, sc,
			sc->sc_intr_buf, sc->sc_isize,
			uplcom_intr, USBD_DEFAULT_INTERVAL);
		if (err) {
			DPRINTF("cannot open interrupt pipe (addr %jd)",
				sc->sc_intr_number, 0, 0, 0);
		}
	}

	if (err == 0 && sc->sc_type == UPLCOM_TYPE_HX)
		err = uplcom_pl2303x_init(sc);

	return err;
}

static void
uplcom_close(void *addr, int portno)
{
	struct uplcom_softc *sc = addr;

	UPLCOMHIST_FUNC(); UPLCOMHIST_CALLED();
	DPRINTF("sc=%#jx", (uintptr_t)sc, 0, 0, 0);

	if (sc->sc_dying)
		return;

	uplcom_close_pipe(sc);
}

static void
uplcom_intr(struct usbd_xfer *xfer, void *priv, usbd_status status)
{
	struct uplcom_softc *sc = priv;
	u_char *buf = sc->sc_intr_buf;
	u_char pstatus;

	UPLCOMHIST_FUNC(); UPLCOMHIST_CALLED();

	if (sc->sc_dying)
		return;

	if (status != USBD_NORMAL_COMPLETION) {
		if (status == USBD_NOT_STARTED || status == USBD_CANCELLED)
			return;

		DPRINTF("abnormal status: %ju", status, 0, 0, 0);
		usbd_clear_endpoint_stall_async(sc->sc_intr_pipe);
		return;
	}

	DPRINTF("uplcom status = %02jx", buf[8], 0, 0, 0);

	sc->sc_lsr = sc->sc_msr = 0;
	pstatus = buf[8];
	if (ISSET(pstatus, UPLCOM_N_SERIAL_CTS))
		sc->sc_msr |= UMSR_CTS;
	if (ISSET(pstatus, UCDC_N_SERIAL_RI))
		sc->sc_msr |= UMSR_RI;
	if (ISSET(pstatus, UCDC_N_SERIAL_DSR))
		sc->sc_msr |= UMSR_DSR;
	if (ISSET(pstatus, UCDC_N_SERIAL_DCD))
		sc->sc_msr |= UMSR_DCD;
	ucom_status_change(device_private(sc->sc_subdev));
}

static void
uplcom_get_status(void *addr, int portno, u_char *lsr, u_char *msr)
{
	struct uplcom_softc *sc = addr;

	UPLCOMHIST_FUNC(); UPLCOMHIST_CALLED();

	if (sc->sc_dying)
		return;

	*lsr = sc->sc_lsr;
	*msr = sc->sc_msr;
}

#if TODO
static int
uplcom_ioctl(void *addr, int portno, u_long cmd, void *data, int flag,
	     proc_t *p)
{
	struct uplcom_softc *sc = addr;
	int error = 0;

	UPLCOMHIST_FUNC(); UPLCOMHIST_CALLED();

	if (sc->sc_dying)
		return EIO;

	DPRINTF("cmd=0x%08lx", cmd, 0, 0, 0);

	switch (cmd) {
	case TIOCNOTTY:
	case TIOCMGET:
	case TIOCMSET:
	case USB_GET_CM_OVER_DATA:
	case USB_SET_CM_OVER_DATA:
		break;

	default:
		DPRINTF("unknown", 0, 0, 0, 0);
		error = ENOTTY;
		break;
	}

	return error;
}
#endif
