/*	$NetBSD: ubtbcmfw.c,v 1.11.8.1 2006/05/24 10:58:24 yamt Exp $	*/

/*
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by  Lennart Augustsson <lennart@augustsson.net>.
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
__KERNEL_RCSID(0, "$NetBSD: ubtbcmfw.c,v 1.11.8.1 2006/05/24 10:58:24 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/proc.h>
#include <sys/fcntl.h>
#include <sys/sysctl.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbdevs.h>

/*
 * Download firmware to BCM2033.
 */

#define CONFIG_NO	1
#define IFACE_IDX	0	/* Control interface */
/* Fixed endpoints */
#define INTR_IN_EP	0x81
#define BULK_OUT_EP	0x02

Static char ubtbcmfw_fwpath[128] = "/usr/libdata/firmware/bcm2033";

#define MINI_DRIVER "BCM2033-MD.hex"
#define FIRMWARE    "BCM2033-FW.bin"

struct ubtbcmfw_softc {
	USBBASEDEVICE		sc_dev;		/* base device */
};

Static int
ubtbcmfw_load_file(usbd_device_handle dev, usbd_pipe_handle out,
		   const char *filename);
Static usbd_status
ubtbcmfw_write(usbd_device_handle dev, usbd_pipe_handle out,
	       char *buf, uint count);
Static usbd_status
ubtbcmfw_read(usbd_device_handle dev, usbd_pipe_handle in,
	      char *buf, uint *count);

USB_DECLARE_DRIVER(ubtbcmfw);

USB_MATCH(ubtbcmfw)
{
	USB_MATCH_START(ubtbcmfw, uaa);

	if (uaa->iface != NULL)
		return (UMATCH_NONE);

	/* Match the boot device. */
	if (uaa->vendor == USB_VENDOR_BROADCOM &&
	    uaa->product == USB_PRODUCT_BROADCOM_BCM2033NF)
		return (UMATCH_VENDOR_PRODUCT);

	return (UMATCH_NONE);
}

USB_ATTACH(ubtbcmfw)
{
	USB_ATTACH_START(ubtbcmfw, sc, uaa);
	usbd_device_handle dev = uaa->device;
	usbd_interface_handle iface;
	usbd_status err;
	char *devinfop;
	char name[256];
	char buf[16];
	usbd_pipe_handle intr_in_pipe;
	usbd_pipe_handle bulk_out_pipe;
	uint n;

	devinfop = usbd_devinfo_alloc(dev, 0);
	USB_ATTACH_SETUP;
	printf("%s: %s\n", USBDEVNAME(sc->sc_dev), devinfop);
	usbd_devinfo_free(devinfop);

	err = usbd_set_config_no(dev, CONFIG_NO, 1);
	if (err) {
		printf("%s: setting config no failed\n",
		    USBDEVNAME(sc->sc_dev));
		USB_ATTACH_ERROR_RETURN;
	}
	err = usbd_device2interface_handle(dev, IFACE_IDX, &iface);
	if (err) {
		printf("%s: getting interface handle failed\n",
		    USBDEVNAME(sc->sc_dev));
		USB_ATTACH_ERROR_RETURN;
	}

	/* Will be used as a bulk pipe. */
	err = usbd_open_pipe(iface, INTR_IN_EP, 0, &intr_in_pipe);
	if (err) {
		printf("%s: open bulk in failed\n", USBDEVNAME(sc->sc_dev));
		USB_ATTACH_ERROR_RETURN;
	}
	err = usbd_open_pipe(iface, BULK_OUT_EP, 0, &bulk_out_pipe);
	if (err) {
		printf("%s: open bulk in failed\n", USBDEVNAME(sc->sc_dev));
		USB_ATTACH_ERROR_RETURN;
	}

	printf("%s: downloading firmware\n", USBDEVNAME(sc->sc_dev));
	snprintf(name, sizeof name, "%s/%s", ubtbcmfw_fwpath, MINI_DRIVER);
	err = ubtbcmfw_load_file(dev, bulk_out_pipe, name);
	if (err) {
		printf("%s: loading mini-driver failed\n",
		       USBDEVNAME(sc->sc_dev));
		USB_ATTACH_ERROR_RETURN;
	}
	usbd_delay_ms(dev, 1);
	err = ubtbcmfw_write(dev, bulk_out_pipe, "#", 1);
	if (err) {
		printf("%s: write # failed\n", USBDEVNAME(sc->sc_dev));
		USB_ATTACH_ERROR_RETURN;
	}
	buf[0] = 0;
	n = 10;
	err = ubtbcmfw_read(dev, intr_in_pipe, buf, &n);
	if (err) {
		printf("%s: read # failed\n", USBDEVNAME(sc->sc_dev));
		USB_ATTACH_ERROR_RETURN;
	}
	if (buf[0] != '#') {
		printf("%s: memory select failed\n", USBDEVNAME(sc->sc_dev));
		USB_ATTACH_ERROR_RETURN;
	}
	snprintf(name, sizeof name, "%s/%s", ubtbcmfw_fwpath, FIRMWARE);
	err = ubtbcmfw_load_file(dev, bulk_out_pipe, name);
	if (err) {
		printf("%s: loading firmware failed\n",
		       USBDEVNAME(sc->sc_dev));
		USB_ATTACH_ERROR_RETURN;
	}
	n = 10;
	err = ubtbcmfw_read(dev, intr_in_pipe, buf, &n);
	if (err) {
		printf("%s: read . failed\n", USBDEVNAME(sc->sc_dev));
		USB_ATTACH_ERROR_RETURN;
	}
	if (buf[0] != '.') {
		printf("%s: firmware load failed\n", USBDEVNAME(sc->sc_dev));
		USB_ATTACH_ERROR_RETURN;
	}

	usbd_close_pipe(intr_in_pipe);
	usbd_close_pipe(bulk_out_pipe);

	printf("%s: firmware download complete\n",
	       USBDEVNAME(sc->sc_dev));
	usbd_delay_ms(dev, 500);

	USB_ATTACH_SUCCESS_RETURN;
}

USB_DETACH(ubtbcmfw)
{
	/*USB_DETACH_START(ubtbcmfw, sc);*/

	return (0);
}

int
ubtbcmfw_activate(device_ptr_t self, enum devact act)
{
	return 0;
}

Static int
ubtbcmfw_load_file(usbd_device_handle dev, usbd_pipe_handle out,
		   const char *filename)
{
	struct proc *p = curproc;
	struct nameidata nd;
	struct vnode *vp;
	size_t resid, offs, size;
	int error;
	char buf[1024];
	struct timeval delta;

	NDINIT(&nd, LOOKUP, FOLLOW, UIO_SYSSPACE, filename, curlwp);
	/* Loop until we are well passed boot */
	for (;;) {
		error = vn_open(&nd, FREAD, 0);
		if (!error)
			break;
		timersub(&boottime, &time, &delta);
		if (delta.tv_sec > 60)
			break;
		printf("ubtbcmfw_load_file: waiting for firmware file\n");
		if (tsleep(buf, PZERO, "ubtbcmfw", hz * 5) != EWOULDBLOCK)
			break;
	}
	if (error)
		return (error);
	vp = nd.ni_vp;
	VOP_UNLOCK(vp, 0);
	if (nd.ni_vp->v_type != VREG) {
		error = EACCES;
		goto out;
	}

	for (offs = 0; ; offs += size) {
		size = sizeof buf;
		error = vn_rdwr(UIO_READ, vp, buf, size, offs, UIO_SYSSPACE,
		    IO_NODELOCKED | IO_SYNC, p->p_cred, &resid, NULL);
		size -= resid;
		if (error || size == 0)
			break;
		if (ubtbcmfw_write(dev, out, buf, size)) {
			error = EIO;
			break;
		}
	}

out:
	vn_close(vp, FREAD, p->p_cred, p);
	return error;
}

Static usbd_status
ubtbcmfw_write(usbd_device_handle dev, usbd_pipe_handle out,
	       char *buf, uint count)
{
	usbd_xfer_handle xfer;
	usbd_status err;
	uint n;

	xfer = usbd_alloc_xfer(dev);
	if (xfer == NULL)
		return (USBD_NOMEM);
	n = count;
	err = usbd_bulk_transfer(xfer, out, 0, USBD_DEFAULT_TIMEOUT,
				 buf, &n, "ubtfwr");
	usbd_free_xfer(xfer);
	return (err);
}

Static usbd_status
ubtbcmfw_read(usbd_device_handle dev, usbd_pipe_handle in,
	      char *buf, uint *count)
{
	usbd_xfer_handle xfer;
	usbd_status err;

	xfer = usbd_alloc_xfer(dev);
	if (xfer == NULL)
		return (USBD_NOMEM);
	err = usbd_bulk_transfer(xfer, in, USBD_SHORT_XFER_OK,
				 USBD_DEFAULT_TIMEOUT, buf, count, "ubtfrd");
	usbd_free_xfer(xfer);
	return (err);
}
