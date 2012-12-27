/* $NetBSD: aubtfwl.c,v 1.4 2012/12/27 16:42:32 skrll Exp $ */

/*
 * Copyright (c) 2011 Jonathan A. Kollasch
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: aubtfwl.c,v 1.4 2012/12/27 16:42:32 skrll Exp $");

#include <sys/param.h>
#include <dev/usb/usb.h>
#include <dev/usb/usbdevs.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdi_util.h>
#include <dev/firmload.h>

#define AR3K_FIRMWARE_HEADER_SIZE 20
#define AR3K_FIRMWARE_CHUNK_SIZE 4096

static int aubtfwl_match(device_t, cfdata_t, void *);
static void aubtfwl_attach(device_t, device_t, void *);
static int aubtfwl_detach(device_t, int);
static void aubtfwl_attach_hook(device_t);

struct aubtfwl_softc {
	usbd_device_handle sc_udev;
};

CFATTACH_DECL_NEW(aubtfwl, sizeof(struct aubtfwl_softc), aubtfwl_match, aubtfwl_attach, aubtfwl_detach, NULL);

static int
aubtfwl_match(device_t parent, cfdata_t match, void *aux)
{
	const struct usb_attach_arg * const uaa = aux;

	if (uaa->vendor == USB_VENDOR_ATHEROS2 &&
	    uaa->product == USB_PRODUCT_ATHEROS2_AR3011)
		return UMATCH_VENDOR_PRODUCT;

	return UMATCH_NONE;
}

static void
aubtfwl_attach(device_t parent, device_t self, void *aux)
{
	const struct usb_attach_arg * const uaa = aux;
	struct aubtfwl_softc * const sc = device_private(self);
	aprint_naive("\n");
	aprint_normal("\n");
	sc->sc_udev = uaa->device;

	config_mountroot(self, aubtfwl_attach_hook);
}

static int
aubtfwl_detach(device_t self, int flags)
{
	struct aubtfwl_softc * const sc = device_private(self);

	usbd_add_drv_event(USB_EVENT_DRIVER_DETACH, sc->sc_udev, self);

	return 0;
}

static void
aubtfwl_attach_hook(device_t self)
{
	struct aubtfwl_softc * const sc = device_private(self);
	usbd_interface_handle iface;
	usbd_pipe_handle pipe;
	usbd_xfer_handle xfer;
	void *buf;
	usb_device_request_t req;
	int error;
	firmware_handle_t fwh;
	size_t fws;
	size_t fwo = 0;
	uint32_t n;
	
	memset(&req, 0, sizeof(req));

	error = firmware_open("ubt", "ath3k-1.fw", &fwh); /* XXX revisit name */
	if (error != 0) {
		aprint_error_dev(self, "ath3k-1.fw open fail %d\n", error);
		return;
	}
	fws = firmware_get_size(fwh);

	error = usbd_set_config_no(sc->sc_udev, 1, 0);
	if (error != 0) {
		aprint_error_dev(self, "failed to set configuration"
		    ", err=%s\n", usbd_errstr(error));
		goto out_firmware;
	}

	error = usbd_device2interface_handle(sc->sc_udev, 0, &iface);
	if (error) {
		aprint_error_dev(self, "failed to get interface, %s\n",
		   usbd_errstr(error));
		goto out_firmware;
	}

	error = usbd_open_pipe(iface, UE_DIR_OUT|2, USBD_EXCLUSIVE_USE, &pipe);
	if (error) {
		aprint_error_dev(self, "failed to open pipe, %s\n",
		   usbd_errstr(error));
		goto out_firmware;
	}

	xfer = usbd_alloc_xfer(sc->sc_udev);
	if (xfer == NULL) {
		aprint_error_dev(self, "failed to alloc xfer\n");
		goto out_pipe;
	}

	buf = usbd_alloc_buffer(xfer, 4096);
	if (buf == NULL) {
		aprint_error_dev(self, "failed to alloc buffer\n");
		goto out_xfer;
	}

	error = firmware_read(fwh, fwo, buf, AR3K_FIRMWARE_HEADER_SIZE);
	if (error != 0) {
		aprint_error_dev(self, "firmware_read failed %d\n", error);
		goto out_xfer;
	}

	req.bRequest = 1;
	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	USETW(req.wValue, 0);
	USETW(req.wIndex, 0);
	USETW(req.wLength, AR3K_FIRMWARE_HEADER_SIZE);

	aprint_verbose_dev(self, "beginning firmware load\n");

	error = usbd_do_request(sc->sc_udev, &req, buf);
	if (error != 0) {
		aprint_error_dev(self, "%s\n", usbd_errstr(error));
		return;
	}
	fwo = AR3K_FIRMWARE_HEADER_SIZE;

	while (fwo < fws) {
		n = min(AR3K_FIRMWARE_CHUNK_SIZE, fws - fwo);
		error = firmware_read(fwh, fwo, buf, n);
		if (error != 0) {
			break;;
		}
		error = usbd_bulk_transfer(xfer, pipe,
		    USBD_NO_COPY, USBD_DEFAULT_TIMEOUT,
		    buf, &n, device_xname(self));
		if (error != USBD_NORMAL_COMPLETION) {
			aprint_error_dev(self, "xfer failed, %s\n",
			   usbd_errstr(error));
			break;;
		}
		fwo += n;
	}
	aprint_verbose_dev(self, "firmware load complete\n");

out_xfer:
	usbd_free_xfer(xfer);
out_pipe:
	usbd_close_pipe(pipe);
out_firmware:
	firmware_close(fwh);

	return;
}
