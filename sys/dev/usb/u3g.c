/*	$NetBSD: u3g.c,v 1.9 2010/01/07 00:15:20 martin Exp $	*/

/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation.
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
__KERNEL_RCSID(0, "$NetBSD: u3g.c,v 1.9 2010/01/07 00:15:20 martin Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/bus.h>
#include <sys/conf.h>
#include <sys/tty.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usbdi_util.h>

#include <dev/usb/ucomvar.h>

#include "usbdevs.h"

/*
 * We read/write data from/to the device in 4KB chunks to maximise
 * performance.
 */
#define U3G_BUFF_SIZE	4096

/*
 * Some 3G devices (the Huawei E160/E220 springs to mind here) buffer up
 * data internally even when the USB pipes are closed. So on first open,
 * we can receive a large chunk of stale data.
 *
 * This causes a real problem because the default TTYDEF_LFLAG (applied
 * on first open) has the ECHO flag set, resulting in all the stale data
 * being echoed straight back to the device by the tty(4) layer. Some
 * devices (again, the Huawei E160/E220 for example) react to this spew
 * by going catatonic.
 *
 * All this happens before the application gets a chance to disable ECHO.
 *
 * We work around this by ignoring all data received from the device for
 * a period of two seconds, or until the application starts sending data -
 * whichever comes first.
 */
#define	U3G_PURGE_SECS	2

/*
 * Define bits for the virtual modem control pins.
 * The input pin states are reported via the interrupt pipe on some devices.
 */
#define	U3G_OUTPIN_DTR	(1u << 0)
#define	U3G_OUTPIN_RTS	(1u << 1)
#define	U3G_INPIN_DCD	(1u << 0)
#define	U3G_INPIN_DSR	(1u << 1)
#define	U3G_INPIN_RI	(1u << 3)

/*
 * USB request to set the output pin status
 */
#define	U3G_SET_PIN	0x22

struct u3g_softc {
	device_t		sc_dev;
	usbd_device_handle	sc_udev;
	bool			sc_dying;	/* We're going away */

	device_t		sc_ucom;	/* Child ucom(4) handle */
	int			sc_ifaceno;	/* Device interface number */

	bool			sc_open;	/* Device is in use */
	bool			sc_purging;	/* Purging stale data */
	struct timeval		sc_purge_start;	/* Control duration of purge */

	u_char			sc_msr;		/* Emulated 'msr' */
	uint16_t		sc_outpins;	/* Output pin state */

	usbd_pipe_handle	sc_intr_pipe;	/* Interrupt pipe */
	u_char			*sc_intr_buff;	/* Interrupt buffer */
};

/*
 * The device driver has two personalities. The first uses the 'usbdevif'
 * interface attribute so that a match will claim the entire USB device
 * for itself. This is used for when a device needs to be mode-switched
 * and ensures any other interfaces present cannot be claimed by other
 * drivers while the mode-switch is in progress.
 *
 * The second personality uses the 'usbifif' interface attribute so that
 * it can claim the 3G modem interfaces for itself, leaving others (such
 * as the mass storage interfaces on some devices) for other drivers.
 */
static int u3ginit_match(device_t, cfdata_t, void *);
static void u3ginit_attach(device_t, device_t, void *);
static int u3ginit_detach(device_t, int);

CFATTACH_DECL2_NEW(u3ginit, 0, u3ginit_match,
    u3ginit_attach, u3ginit_detach, NULL, NULL, NULL);


static int u3g_match(device_t, cfdata_t, void *);
static void u3g_attach(device_t, device_t, void *);
static int u3g_detach(device_t, int);
static int u3g_activate(device_t, enum devact);
static void u3g_childdet(device_t, device_t);

CFATTACH_DECL2_NEW(u3g, sizeof(struct u3g_softc), u3g_match,
    u3g_attach, u3g_detach, u3g_activate, NULL, u3g_childdet);


static void u3g_intr(usbd_xfer_handle, usbd_private_handle, usbd_status);
static void u3g_get_status(void *, int, u_char *, u_char *);
static void u3g_set(void *, int, int, int);
static int  u3g_open(void *, int);
static void u3g_close(void *, int);
static void u3g_read(void *, int, u_char **, uint32_t *);
static void u3g_write(void *, int, u_char *, u_char *, u_int32_t *);

struct ucom_methods u3g_methods = {
	u3g_get_status,
	u3g_set,
	NULL,
	NULL,
	u3g_open,
	u3g_close,
	u3g_read,
	u3g_write,
};

/*
 * Allegedly supported devices
 */
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

static int
u3g_novatel_reinit(usbd_device_handle dev)
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
	err = usbd_set_config_index(dev, 0, 0);
	if (err) {
		aprint_error("u3g: failed to set configuration index\n");
		return UMATCH_NONE;
	}

	err = usbd_device2interface_handle(dev, 0, &iface);
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

	xfer = usbd_alloc_xfer(dev);
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
	/*
	 * The Huawei device presents itself as a umass device with Windows
	 * drivers on it. After installation of the driver, it reinits into a
	 * 3G serial device.
	 */
	usb_device_request_t req;
	usb_config_descriptor_t *cdesc;

	/* Get the config descriptor */
	cdesc = usbd_get_config_descriptor(dev);
	if (cdesc == NULL) {
		usb_device_descriptor_t dd;

		if (usbd_get_device_desc(dev, &dd) != 0)
			return (UMATCH_NONE);

		if (dd.bNumConfigurations != 1)
			return (UMATCH_NONE);

		if (usbd_set_config_index(dev, 0, 1) != 0)
			return (UMATCH_NONE);

		cdesc = usbd_get_config_descriptor(dev);

		if (cdesc == NULL)
			return (UMATCH_NONE);
	}

	/*
	 * One iface means umass mode, more than 1 (4 usually) means 3G mode.
	 *
	 * XXX: We should check the first interface's device class just to be
	 * sure. If it's a mass storage device, then we can be fairly certain
	 * it needs a mode-switch.
	 */
	if (cdesc->bNumInterface > 1)
		return (UMATCH_NONE);

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

	req.bmRequestType = UT_VENDOR;
	req.bRequest = UR_SET_INTERFACE;
	USETW(req.wValue, UF_DEVICE_REMOTE_WAKEUP);
	USETW(req.wIndex, UHF_PORT_CONNECTION);
	USETW(req.wLength, 0);

	(void) usbd_do_request(dev, &req, 0);

	return (UMATCH_HIGHEST); /* Match to prevent umass from attaching */
}


/*
 * First personality:
 *
 * Claim the entire device if a mode-switch is required.
 */

static int
u3ginit_match(device_t parent, cfdata_t match, void *aux)
{
	struct usb_attach_arg *uaa = aux;

	if (uaa->vendor == USB_VENDOR_HUAWEI)
		return u3g_huawei_reinit(uaa->device);

	if (uaa->vendor == USB_VENDOR_NOVATEL2 &&
	    uaa->product == USB_PRODUCT_NOVATEL2_MC950D_DRIVER)
		return u3g_novatel_reinit(uaa->device);

	if (uaa->vendor == USB_VENDOR_SIERRA &&
	    uaa->product == USB_PRODUCT_SIERRA_INSTALLER)
		return u3g_sierra_reinit(uaa->device);

	return UMATCH_NONE;
}

static void
u3ginit_attach(device_t parent, device_t self, void *aux)
{
	struct usb_attach_arg *uaa = aux;

	aprint_naive("\n");
	aprint_normal(": Switching to 3G mode\n");

	if (uaa->vendor == USB_VENDOR_NOVATEL2 &&
	    uaa->product == USB_PRODUCT_NOVATEL2_MC950D_DRIVER) {
		/* About to disappear... */
		return;
	}

	/* Move the device into the configured state. */
	(void) usbd_set_config_index(uaa->device, 0, 1);
}

static int
u3ginit_detach(device_t self, int flags)
{

	return (0);
}


/*
 * Second personality:
 *
 * Claim only those interfaces required for 3G modem operation.
 */

static int
u3g_match(device_t parent, cfdata_t match, void *aux)
{
	struct usbif_attach_arg *uaa = aux;
	usbd_interface_handle iface;
	usb_interface_descriptor_t *id;
	usbd_status error;

	if (!usb_lookup(u3g_devs, uaa->vendor, uaa->product))
		return (UMATCH_NONE);

	error = usbd_device2interface_handle(uaa->device, uaa->ifaceno, &iface);
	if (error) {
		printf("u3g_match: failed to get interface, err=%s\n",
		    usbd_errstr(error));
		return (UMATCH_NONE);
	}

	id = usbd_get_interface_descriptor(iface);
	if (id == NULL) {
		printf("u3g_match: failed to get interface descriptor\n");
		return (UMATCH_NONE);
	}

	/*
	 * 3G modems generally report vendor-specific class
	 *
	 * XXX: this may be too generalised.
	 */
	return ((id->bInterfaceClass == UICLASS_VENDOR) ?
	    UMATCH_VENDOR_PRODUCT : UMATCH_NONE);
}

static void
u3g_attach(device_t parent, device_t self, void *aux)
{
	struct u3g_softc *sc = device_private(self);
	struct usbif_attach_arg *uaa = aux;
	usbd_device_handle dev = uaa->device;
	usbd_interface_handle iface;
	usb_interface_descriptor_t *id;
	usb_endpoint_descriptor_t *ed;
	struct ucom_attach_args uca;
	usbd_status error;
	int n, intr_address, intr_size; 

	aprint_naive("\n");
	aprint_normal("\n");

	sc->sc_dev = self;
	sc->sc_dying = false;
	sc->sc_udev = dev;

	error = usbd_device2interface_handle(dev, uaa->ifaceno, &iface);
	if (error) {
		aprint_error_dev(self, "failed to get interface, err=%s\n",
		    usbd_errstr(error));
		return;
	}

	id = usbd_get_interface_descriptor(iface);

	uca.info = "3G Modem";
	uca.ibufsize = U3G_BUFF_SIZE;
	uca.obufsize = U3G_BUFF_SIZE;
	uca.ibufsizepad = U3G_BUFF_SIZE;
	uca.portno = uaa->ifaceno;
	uca.opkthdrlen = 0;
	uca.device = dev;
	uca.iface = iface;
	uca.methods = &u3g_methods;
	uca.arg = sc;
	uca.bulkin = uca.bulkout = -1;

	sc->sc_outpins = 0;
	sc->sc_msr = UMSR_DSR | UMSR_CTS | UMSR_DCD;
	sc->sc_ifaceno = uaa->ifaceno;
	sc->sc_open = false;
	sc->sc_purging = false;

	intr_address = -1;
	intr_size = 0;

	for (n = 0; n < id->bNumEndpoints; n++) {
		ed = usbd_interface2endpoint_descriptor(iface, n);
		if (ed == NULL) {
			aprint_error_dev(self, "no endpoint descriptor "
			    "for %d (interface: %d)\n", n, sc->sc_ifaceno);
			sc->sc_dying = true;
			return;
		}

		if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_IN &&
		    UE_GET_XFERTYPE(ed->bmAttributes) == UE_INTERRUPT) {
			intr_address = ed->bEndpointAddress;
			intr_size = UGETW(ed->wMaxPacketSize);
		} else
		if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_IN &&
		    UE_GET_XFERTYPE(ed->bmAttributes) == UE_BULK) {
			uca.bulkin = ed->bEndpointAddress;
		} else
		if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_OUT &&
		    UE_GET_XFERTYPE(ed->bmAttributes) == UE_BULK) {
			uca.bulkout = ed->bEndpointAddress;
		}
	}

	if (uca.bulkin == -1) {
		aprint_error_dev(self, "Missing bulk in for interface %d\n",
		    sc->sc_ifaceno);
		sc->sc_dying = true;
		return;
	}

	if (uca.bulkout == -1) {
		aprint_error_dev(self, "Missing bulk out for interface %d\n",
		    sc->sc_ifaceno);
		sc->sc_dying = true;
		return;
	}

	sc->sc_ucom = config_found_sm_loc(self, "ucombus",
	    NULL, &uca, ucomprint, ucomsubmatch);

	/*
	 * If the interface has an interrupt pipe, open it immediately so
	 * that we can track input pin state changes regardless of whether
	 * the tty(4) device is open or not.
	 */
	if (intr_address != -1) {
		sc->sc_intr_buff = malloc(intr_size, M_USBDEV, M_WAITOK);
		error = usbd_open_pipe_intr(iface, intr_address,
		    USBD_SHORT_XFER_OK, &sc->sc_intr_pipe, sc, sc->sc_intr_buff,
		    intr_size, u3g_intr, 100);
		if (error) {
			aprint_error_dev(self, "cannot open interrupt pipe "
			    "(addr %d)\n", intr_address);
			return;
		}
	} else {
		sc->sc_intr_pipe = NULL;
		sc->sc_intr_buff = NULL;
	}

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");
}

static int
u3g_detach(device_t self, int flags)
{
	struct u3g_softc *sc = device_private(self);
	int rv;

	if (sc->sc_dying)
		return 0;

	pmf_device_deregister(self);

	if (sc->sc_ucom != NULL) {
		rv = config_detach(sc->sc_ucom, flags);
		if (rv != 0) {
			aprint_verbose_dev(self, "Can't deallocate "
			    "port (%d)", rv);
		}
	}

	if (sc->sc_intr_pipe != NULL) {
		(void) usbd_abort_pipe(sc->sc_intr_pipe);
		(void) usbd_close_pipe(sc->sc_intr_pipe);
		sc->sc_intr_pipe = NULL;
	}
	if (sc->sc_intr_buff != NULL) {
		free(sc->sc_intr_buff, M_USBDEV);
		sc->sc_intr_buff = NULL;
	}

	return (0);
}

static void
u3g_childdet(device_t self, device_t child)
{
	struct u3g_softc *sc = device_private(self);

	if (sc->sc_ucom == child)
		sc->sc_ucom = NULL;
}

static int
u3g_activate(device_t self, enum devact act)
{
	struct u3g_softc *sc = device_private(self);
	int rv;

	switch (act) {
	case DVACT_DEACTIVATE:
		if (sc->sc_ucom != NULL && config_deactivate(sc->sc_ucom))
			rv = -1;
		else
			rv = 0;
		break;

	default:
		rv = 0;
		break;
	}

	return (rv);
}

static void
u3g_intr(usbd_xfer_handle xfer, usbd_private_handle priv, usbd_status status)
{
	struct u3g_softc *sc = (struct u3g_softc *)priv;
	u_char *buf;

	if (sc->sc_dying)
		return;

	if (status != USBD_NORMAL_COMPLETION) {
		if (status == USBD_NOT_STARTED || status == USBD_CANCELLED)
			return;
		usbd_clear_endpoint_stall_async(sc->sc_intr_pipe);
		return;
	}

	buf = sc->sc_intr_buff;
	if (buf[0] == 0xa1 && buf[1] == 0x20) {
		u_char msr;

		msr = sc->sc_msr & ~(UMSR_DCD | UMSR_DSR | UMSR_RI);

		if (buf[8] & U3G_INPIN_DCD)
			msr |= UMSR_DCD;

		if (buf[8] & U3G_INPIN_DSR)
			msr |= UMSR_DSR;

		if (buf[8] & U3G_INPIN_RI)
			msr |= UMSR_RI;

		if (msr != sc->sc_msr) {
			sc->sc_msr = msr;
			if (sc->sc_open)
				ucom_status_change(device_private(sc->sc_ucom));
		}
	}
}

/*ARGSUSED*/
static void
u3g_get_status(void *arg, int portno, u_char *lsr, u_char *msr)
{
	struct u3g_softc *sc = arg;

	if (lsr != NULL)
		*lsr = 0;	/* LSR isn't supported */
	if (msr != NULL)
		*msr = sc->sc_msr;
}

/*ARGSUSED*/
static void
u3g_set(void *arg, int portno, int reg, int onoff)
{
	struct u3g_softc *sc = arg;
	usb_device_request_t req;
	uint16_t mask, new_state;
	usbd_status err;

	if (sc->sc_dying)
		return;

	switch (reg) {
	case UCOM_SET_DTR:
		mask = U3G_OUTPIN_DTR;
		break;
	case UCOM_SET_RTS:
		mask = U3G_OUTPIN_RTS;
		break;
	default:
		return;
	}

	new_state = sc->sc_outpins & ~mask;
	if (onoff)
		new_state |= mask;

	if (new_state == sc->sc_outpins)
		return;

	sc->sc_outpins = new_state;

	req.bmRequestType = UT_WRITE_CLASS_INTERFACE;
	req.bRequest = U3G_SET_PIN;
	USETW(req.wValue, new_state);
	USETW(req.wIndex, sc->sc_ifaceno);
	USETW(req.wLength, 0);

	err = usbd_do_request(sc->sc_udev, &req, 0);
	if (err == USBD_STALLED)
		usbd_clear_endpoint_stall(sc->sc_udev->default_pipe);
}

/*ARGSUSED*/
static int 
u3g_open(void *arg, int portno)
{
	struct u3g_softc *sc = arg;
	usb_device_request_t req;
	usb_endpoint_descriptor_t *ed;
	usb_interface_descriptor_t *id;
	usbd_interface_handle ih;
	usbd_status err;
	int i;

	if (sc->sc_dying)
		return (0);

	err = usbd_device2interface_handle(sc->sc_udev, portno, &ih);
	if (err)
		return (EIO);

	id = usbd_get_interface_descriptor(ih);

	for (i = 0; i < id->bNumEndpoints; i++) {
		ed = usbd_interface2endpoint_descriptor(ih, i);
		if (ed == NULL)	
			return (EIO);

		if (UE_GET_XFERTYPE(ed->bmAttributes) == UE_BULK) {
			/* Issue ENDPOINT_HALT request */
			req.bmRequestType = UT_WRITE_ENDPOINT;
			req.bRequest = UR_CLEAR_FEATURE;
			USETW(req.wValue, UF_ENDPOINT_HALT);
			USETW(req.wIndex, ed->bEndpointAddress);
			USETW(req.wLength, 0);
			err = usbd_do_request(sc->sc_udev, &req, 0);
			if (err)
				return (EIO);
		}
	}

	sc->sc_open = true;
	sc->sc_purging = true;
	getmicrotime(&sc->sc_purge_start);

	return (0);
}

/*ARGSUSED*/
static void 
u3g_close(void *arg, int portno)
{
	struct u3g_softc *sc = arg;

	sc->sc_open = false;
}

/*ARGSUSED*/
static void
u3g_read(void *arg, int portno, u_char **cpp, uint32_t *ccp)
{
	struct u3g_softc *sc = arg;
	struct timeval curr_tv, diff_tv;

	/*
	 * If we're not purging input data following first open, do nothing.
	 */
	if (sc->sc_purging == false)
		return;

	/*
	 * Otherwise check if the purge timeout has expired
	 */
	getmicrotime(&curr_tv);
	timersub(&curr_tv, &sc->sc_purge_start, &diff_tv);

	if (diff_tv.tv_sec >= U3G_PURGE_SECS) {
		/* Timeout expired. */
		sc->sc_purging = false;
	} else {
		/* Still purging. Adjust the caller's byte count. */
		*ccp = 0;
	}
}

/*ARGSUSED*/
static void
u3g_write(void *arg, int portno, u_char *to, u_char *from, u_int32_t *count)
{
	struct u3g_softc *sc = arg;

	/*
	 * Stop purging as soon as the first data is written to the device.
	 */
	sc->sc_purging = false;
	memcpy(to, from, *count);
}
