/*	$NetBSD: u3g.c,v 1.8 2009/11/12 19:52:14 dyoung Exp $	*/

/*
 * Copyright (c) 2008 AnyWi Technologies
 *   Author: Andrea Guzzo <aguzzo@anywi.com>
 *   * based on uark.c 1.1 2006/08/14 08:30:22 jsg *
 *   * parts from ubsa.c 183348 2008-09-25 12:00:56Z phk *
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
 *
 * $FreeBSD$
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: u3g.c,v 1.8 2009/11/12 19:52:14 dyoung Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/module.h>
#include <sys/bus.h>
#include <sys/ioccom.h>
#include <sys/fcntl.h>
#include <sys/conf.h>
#include <sys/tty.h>
#include <sys/file.h>
#include <sys/selinfo.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usbdi_util.h>

#include <dev/usb/ucomvar.h>

#include "usbdevs.h"

#define U3GBUFSZ        1024
#define U3G_MAXPORTS           4

struct u3g_softc {
	device_t                    sc_ucom[U3G_MAXPORTS];
	device_t                    sc_dev;
	usbd_device_handle          sc_udev;
	u_char                      sc_msr;
	u_char                      sc_lsr;
	u_char                      numports;

	usbd_interface_handle       sc_intr_iface;   /* interrupt interface */
#ifdef U3G_DEBUG
	int                         sc_intr_number;  /* interrupt number */
	usbd_pipe_handle            sc_intr_pipe;    /* interrupt pipe */
	u_char                      *sc_intr_buf;    /* interrupt buffer */
#endif
	int                         sc_isize;
	bool                        sc_pseudodev;
};

struct ucom_methods u3g_methods = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};

static const struct usb_devno u3g_devs[] = {
	/* OEM: Option N.V. */
	{ USB_VENDOR_OPTIONNV, USB_PRODUCT_OPTIONNV_QUADPLUSUMTS },
	{ USB_VENDOR_OPTIONNV, USB_PRODUCT_OPTIONNV_HSDPA },
	{ USB_VENDOR_OPTIONNV, USB_PRODUCT_OPTIONNV_GTMAXHSUPA },
	/* OEM: Qualcomm, Inc. */
	{ USB_VENDOR_QUALCOMMINC, USB_PRODUCT_QUALCOMMINC_CDMA_MSM },
	/* OEM: Huawei */
	{ USB_VENDOR_HUAWEI, USB_PRODUCT_HUAWEI_MOBILE },
	{ USB_VENDOR_HUAWEI, USB_PRODUCT_HUAWEI_E220 },
	/* OEM: Novatel */
	{ USB_VENDOR_NOVATEL2, USB_PRODUCT_NOVATEL2_MERLINV620 },
	{ USB_VENDOR_NOVATEL2, USB_PRODUCT_NOVATEL2_ES620 },
	{ USB_VENDOR_NOVATEL2, USB_PRODUCT_NOVATEL2_MC950D },
	{ USB_VENDOR_NOVATEL2, USB_PRODUCT_NOVATEL2_U720 },
	{ USB_VENDOR_NOVATEL2, USB_PRODUCT_NOVATEL2_U727 },
	{ USB_VENDOR_NOVATEL2, USB_PRODUCT_NOVATEL2_MERLINU740 },
	{ USB_VENDOR_NOVATEL2, USB_PRODUCT_NOVATEL2_U740_2 },
	{ USB_VENDOR_NOVATEL2, USB_PRODUCT_NOVATEL2_U870 },
	{ USB_VENDOR_NOVATEL2, USB_PRODUCT_NOVATEL2_MERLINV620 },
	{ USB_VENDOR_NOVATEL2, USB_PRODUCT_NOVATEL2_S720 },
	{ USB_VENDOR_NOVATEL2, USB_PRODUCT_NOVATEL2_V740 },
	{ USB_VENDOR_NOVATEL2, USB_PRODUCT_NOVATEL2_X950D },
	{ USB_VENDOR_NOVATEL2, USB_PRODUCT_NOVATEL2_XU870 },
        { USB_VENDOR_DELL, USB_PRODUCT_DELL_W5500 },
#if 0
	{ USB_VENDOR_NOVATEL2, USB_PRODUCT_NOVATEL2_MC950D_DRIVER },
#endif
	/* OEM: Merlin */
	{ USB_VENDOR_MERLIN, USB_PRODUCT_MERLIN_V620 },

	/* OEM: Sierra Wireless: */
	{ USB_VENDOR_SIERRA, USB_PRODUCT_SIERRA_AIRCARD580 },
	{ USB_VENDOR_SIERRA, USB_PRODUCT_SIERRA_AIRCARD595 },
	{ USB_VENDOR_SIERRA, USB_PRODUCT_SIERRA_AC595U },
	{ USB_VENDOR_SIERRA, USB_PRODUCT_SIERRA_AC597E },
	{ USB_VENDOR_SIERRA, USB_PRODUCT_SIERRA_C597 },
	{ USB_VENDOR_SIERRA, USB_PRODUCT_SIERRA_AC880 },
	{ USB_VENDOR_SIERRA, USB_PRODUCT_SIERRA_AC880E },
	{ USB_VENDOR_SIERRA, USB_PRODUCT_SIERRA_AC880U },
	{ USB_VENDOR_SIERRA, USB_PRODUCT_SIERRA_AC881 },
	{ USB_VENDOR_SIERRA, USB_PRODUCT_SIERRA_AC881E },
	{ USB_VENDOR_SIERRA, USB_PRODUCT_SIERRA_AC881U },
	{ USB_VENDOR_SIERRA, USB_PRODUCT_SIERRA_EM5625 },
	{ USB_VENDOR_SIERRA, USB_PRODUCT_SIERRA_MC5720 },
	{ USB_VENDOR_SIERRA, USB_PRODUCT_SIERRA_MC5720_2 },
	{ USB_VENDOR_SIERRA, USB_PRODUCT_SIERRA_MC5725 },
	{ USB_VENDOR_SIERRA, USB_PRODUCT_SIERRA_MINI5725 },
	{ USB_VENDOR_SIERRA, USB_PRODUCT_SIERRA_AIRCARD875 },
	{ USB_VENDOR_SIERRA, USB_PRODUCT_SIERRA_MC8755 },
	{ USB_VENDOR_SIERRA, USB_PRODUCT_SIERRA_MC8755_2 },
	{ USB_VENDOR_SIERRA, USB_PRODUCT_SIERRA_MC8755_3 },
	{ USB_VENDOR_SIERRA, USB_PRODUCT_SIERRA_MC8765 },
	{ USB_VENDOR_SIERRA, USB_PRODUCT_SIERRA_AC875U },
	{ USB_VENDOR_SIERRA, USB_PRODUCT_SIERRA_MC8775_2 },
	{ USB_VENDOR_SIERRA, USB_PRODUCT_SIERRA_MC8780 },
	{ USB_VENDOR_SIERRA, USB_PRODUCT_SIERRA_MC8781 },
};

#ifdef U3G_DEBUG
static void
u3g_intr(usbd_xfer_handle xfer, usbd_private_handle priv, usbd_status status)
{
	struct u3g_softc *sc = (struct u3g_softc *)priv;
	aprint_normal_dev(sc->sc_dev, "INTERRUPT CALLBACK\n");
}
#endif

static int
u3g_novatel_reinit(struct usb_attach_arg *uaa)
{
	unsigned char cmd[31];
	usbd_interface_handle iface;
	usb_interface_descriptor_t *id;
	usb_endpoint_descriptor_t *ed;
	usbd_pipe_handle pipe;
	usbd_xfer_handle xfer;
	int err, i;

	memset(cmd, 0, sizeof(cmd));
	/* Byte 0..3: Command Block Wrapper (CBW) signature */
	cmd[0] = 0x55; 
	cmd[1] = 0x53;
	cmd[2] = 0x42;
	cmd[3] = 0x43;
	/* 4..7: CBW Tag, has to unique, but only a single transfer used. */
	cmd[4] = 0x01;
	/* 8..11: CBW Transfer Length, no data here */
	/* 12: CBW Flag: output, so 0 */
	/* 13: CBW Lun: 0 */
	/* 14: CBW Length */
	cmd[14] = 0x06;
	/* Rest is the SCSI payload */
	/* 0: SCSI START/STOP opcode */
	cmd[15] = 0x1b;
	/* 1..3 unused */
	/* 4 Load/Eject command */
	cmd[19] = 0x02;
	/* 5: unused */


	/* Move the device into the configured state. */
	err = usbd_set_config_index(uaa->device, 0, 0);
	if (err) {
		aprint_error("u3g: failed to set configuration index\n");
		return UMATCH_NONE;
	}

	err = usbd_device2interface_handle(uaa->device, 0, &iface);
	if (err != 0) {
		aprint_error("u3g: failed to get interface\n");
		return UMATCH_NONE;
	}

	id = usbd_get_interface_descriptor(iface);
	ed = NULL;
	for (i = 0 ; i < id->bNumEndpoints ; i++) {
		ed = usbd_interface2endpoint_descriptor(iface, i);
		if (ed == NULL)
			continue;
		if (UE_GET_DIR(ed->bEndpointAddress) != UE_DIR_OUT)
			continue;
		if ((ed->bmAttributes & UE_XFERTYPE) == UE_BULK)
			break;
	}

	if (i == id->bNumEndpoints)
		return UMATCH_NONE;

	err = usbd_open_pipe(iface, ed->bEndpointAddress, USBD_EXCLUSIVE_USE,
	    &pipe);
	if (err != 0) {
		aprint_error("u3g: failed to open bulk transfer pipe %d\n",
		    ed->bEndpointAddress);
		return UMATCH_NONE;
	}

	xfer = usbd_alloc_xfer(uaa->device);
	if (xfer != NULL) {
		usbd_setup_xfer(xfer, pipe, NULL, cmd, sizeof(cmd),
		    USBD_SYNCHRONOUS, USBD_DEFAULT_TIMEOUT, NULL);

		err = usbd_transfer(xfer);
		if (err)
			aprint_error("u3g: transfer failed\n");
		usbd_free_xfer(xfer);
	} else {
		aprint_error("u3g: failed to allocate xfer\n");
		err = USBD_NOMEM;
	}

	usbd_abort_pipe(pipe);
	usbd_close_pipe(pipe);

	return (err == USBD_NORMAL_COMPLETION ? UMATCH_HIGHEST : UMATCH_NONE);
}

static int
u3g_huawei_reinit(usbd_device_handle dev)
{
	/* The Huawei device presents itself as a umass device with Windows
	 * drivers on it. After installation of the driver, it reinits into a
	 * 3G serial device.
	 */
	usb_device_request_t req;
	usb_config_descriptor_t *cdesc;

	/* Get the config descriptor */
	cdesc = usbd_get_config_descriptor(dev);
	if (cdesc == NULL)
		return (UMATCH_NONE);

	/* One iface means umass mode, more than 1 (4 usually) means 3G mode */
	if (cdesc->bNumInterface > 1)
		return (UMATCH_VENDOR_PRODUCT);

	req.bmRequestType = UT_WRITE_DEVICE;
	req.bRequest = UR_SET_FEATURE;
	USETW(req.wValue, UF_DEVICE_REMOTE_WAKEUP);
	USETW(req.wIndex, UHF_PORT_SUSPEND);
	USETW(req.wLength, 0);

	(void) usbd_do_request(dev, &req, 0);


	return (UMATCH_HIGHEST); /* Match to prevent umass from attaching */
}

static int
u3g_sierra_reinit(usbd_device_handle dev)
{
	/* Some Sierra devices presents themselves as a umass device with
	 * Windows drivers on it. After installation of the driver, it
	 * reinits into a * 3G serial device.
	 */
	usb_device_request_t req;
	usb_config_descriptor_t *cdesc;

	/* Get the config descriptor */
	cdesc = usbd_get_config_descriptor(dev);
	if (cdesc == NULL)
		return (UMATCH_NONE);

	req.bmRequestType = UT_VENDOR;
	req.bRequest = UR_SET_INTERFACE;
	USETW(req.wValue, UF_DEVICE_REMOTE_WAKEUP);
	USETW(req.wIndex, UHF_PORT_CONNECTION);
	USETW(req.wLength, 0);

	(void) usbd_do_request(dev, &req, 0);

	return (UMATCH_HIGHEST); /* Match to prevent umass from attaching */
}

static int
u3g_match(device_t parent, cfdata_t match, void *aux)
{
	struct usb_attach_arg *uaa = aux;

	if (uaa->vendor == USB_VENDOR_HUAWEI)
		return u3g_huawei_reinit(uaa->device);

	if (uaa->vendor == USB_VENDOR_NOVATEL2 &&
	    uaa->product == USB_PRODUCT_NOVATEL2_MC950D_DRIVER)
		return u3g_novatel_reinit(uaa);

	if (uaa->vendor == USB_VENDOR_SIERRA &&
	    uaa->product == USB_PRODUCT_SIERRA_INSTALLER)
		return u3g_sierra_reinit(uaa->device);

	if (usb_lookup(u3g_devs, uaa->vendor, uaa->product))
		return UMATCH_VENDOR_PRODUCT;

	return UMATCH_NONE;
}

static void
u3g_attach(device_t parent, device_t self, void *aux)
{
	struct u3g_softc *sc = device_private(self);
	struct usb_attach_arg *uaa = aux;
	usbd_device_handle dev = uaa->device;
	usbd_interface_handle iface;
	usb_interface_descriptor_t *id;
	usb_endpoint_descriptor_t *ed;
	usbd_status error;
	int i, n; 
	usb_config_descriptor_t *cdesc;

	aprint_naive("\n");
	aprint_normal("\n");

	if (uaa->vendor == USB_VENDOR_NOVATEL2 &&
	    uaa->product == USB_PRODUCT_NOVATEL2_MC950D_DRIVER) {
		/* About to disappear... */
		sc->sc_pseudodev = true;
		return;
	}

	sc->sc_dev = self;
#ifdef U3G_DEBUG
	sc->sc_intr_number = -1;
	sc->sc_intr_pipe = NULL;
#endif
	/* Move the device into the configured state. */
	error = usbd_set_config_index(dev, 0, 1);
	if (error) {
		aprint_error_dev(self, "failed to set configuration: %s\n",
			      usbd_errstr(error));
		return;
	}

	/* get the config descriptor */
	cdesc = usbd_get_config_descriptor(dev);

	if (cdesc == NULL) {
		aprint_error_dev(self, "failed to get configuration descriptor\n");
		return;
	}

	if (uaa->vendor == USB_VENDOR_HUAWEI && cdesc->bNumInterface > 1) {
		/* About to disappear... */
		sc->sc_pseudodev = true;
		return;
	}

	if (uaa->vendor == USB_VENDOR_SIERRA &&
	    uaa->product == USB_PRODUCT_SIERRA_INSTALLER) {
		/* About to disappear... */
		sc->sc_pseudodev = true;
		return;
	}

	sc->sc_udev = dev;
	sc->numports = (cdesc->bNumInterface <= U3G_MAXPORTS)?cdesc->bNumInterface:U3G_MAXPORTS;
	for ( i = 0; i < sc->numports; i++ ) {
		struct ucom_attach_args uca;

		error = usbd_device2interface_handle(dev, i, &iface);
		if (error) {
			aprint_error_dev(self,
			    "failed to get interface, err=%s\n",
			    usbd_errstr(error));
			return;
		}
		id = usbd_get_interface_descriptor(iface);

		uca.info = "Generic 3G Serial Device";
		uca.ibufsize = U3GBUFSZ;
		uca.obufsize = U3GBUFSZ;
		uca.ibufsizepad = U3GBUFSZ;
		uca.portno = i;
		uca.opkthdrlen = 0;
		uca.device = dev;
		uca.iface = iface;
		uca.methods = &u3g_methods;
		uca.arg = sc;

		uca.bulkin = uca.bulkout = -1;
		for (n = 0; n < id->bNumEndpoints; n++) {
			ed = usbd_interface2endpoint_descriptor(iface, n);
			if (ed == NULL) {
				aprint_error_dev(self,
					"could not read endpoint descriptor\n");
				return;
			}
			if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_IN &&
			    UE_GET_XFERTYPE(ed->bmAttributes) == UE_BULK)
				uca.bulkin = ed->bEndpointAddress;
			else if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_OUT &&
			    UE_GET_XFERTYPE(ed->bmAttributes) == UE_BULK)
				uca.bulkout = ed->bEndpointAddress;
		}
		if (uca.bulkin == -1 || uca.bulkout == -1) {
			aprint_error_dev(self, "missing endpoint\n");
			return;
		}

		sc->sc_ucom[i] = config_found_sm_loc(self, "ucombus", NULL, &uca,
						    ucomprint, ucomsubmatch);
	}

#ifdef U3G_DEBUG
	if (sc->sc_intr_number != -1 && sc->sc_intr_pipe == NULL) {
		sc->sc_intr_buf = malloc(sc->sc_isize, M_USBDEV, M_WAITOK);
		error = usbd_open_pipe_intr(sc->sc_intr_iface,
					    sc->sc_intr_number,
					    USBD_SHORT_XFER_OK,
					    &sc->sc_intr_pipe,
					    sc,
					    sc->sc_intr_buf,
					    sc->sc_isize,
					    u3g_intr,
					    100);
		if (error) {
			aprint_error_dev(self,
			    "cannot open interrupt pipe (addr %d)\n",
			    sc->sc_intr_number);
			return;
		}
	}
#endif


	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");
}

static int
u3g_detach(device_t self, int flags)
{
	struct u3g_softc *sc = device_private(self);
	int rv = 0;
	int i;

	if (sc->sc_pseudodev)
		return 0;

	pmf_device_deregister(self);

	for (i = 0; i < sc->numports; i++) {
		if(sc->sc_ucom[i]) {
			rv = config_detach(sc->sc_ucom[i], flags);
			if(rv != 0) {
				aprint_verbose_dev(self, "Can't deallocat port %d", i);
				return rv;
			}
		}
	}

#ifdef U3G_DEBUG
	if (sc->sc_intr_pipe != NULL) {
		int err = usbd_abort_pipe(sc->sc_intr_pipe);
		if (err)
			aprint_error_dev(self,
				"abort interrupt pipe failed: %s\n",
				usbd_errstr(err));
		err = usbd_close_pipe(sc->sc_intr_pipe);
		if (err)
			aprint_error_dev(self,
			    "close interrupt pipe failed: %s\n",
			    usbd_errstr(err));
		free(sc->sc_intr_buf, M_USBDEV);
		sc->sc_intr_pipe = NULL;
	}
#endif

	return 0;
}

static void
u3g_childdet(device_t self, device_t child)
{
	struct u3g_softc *sc = device_private(self);
	int i;

	for (i = 0; i < sc->numports; i++) {
		if (sc->sc_ucom[i] == child)
			sc->sc_ucom[i] = NULL;
	}
}

CFATTACH_DECL2_NEW(u3g, sizeof(struct u3g_softc), u3g_match,
    u3g_attach, u3g_detach, NULL, NULL, u3g_childdet);
