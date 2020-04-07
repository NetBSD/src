/*	$NetBSD: usbdi_util.c,v 1.81 2020/02/16 09:53:54 maxv Exp $	*/

/*
 * Copyright (c) 1998, 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at
 * Carlstedt Research & Technology and Matthew R. Green (mrg@eterna.com.au).
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: usbdi_util.c,v 1.81 2020/02/16 09:53:54 maxv Exp $");

#ifdef _KERNEL_OPT
#include "opt_usb.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/proc.h>
#include <sys/device.h>
#include <sys/bus.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbhid.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usb_quirks.h>
#include <dev/usb/usbhist.h>

#define	DPRINTF(FMT,A,B,C,D)	USBHIST_LOGN(usbdebug,1,FMT,A,B,C,D)
#define	DPRINTFN(N,FMT,A,B,C,D)	USBHIST_LOGN(usbdebug,N,FMT,A,B,C,D)

usbd_status
usbd_get_desc(struct usbd_device *dev, int type, int index, int len, void *desc)
{
	usb_device_request_t req;
	usbd_status err;

	USBHIST_FUNC();
	USBHIST_CALLARGS(usbdebug, "type=%jd, index=%jd, len=%jd",
	    type, index, len, 0);

	/*
	 * Provide hard-coded configuration descriptors
	 * for devices that may corrupt it. This cannot
	 * be done for device descriptors which are used
	 * to identify the device.
	 */
	if (type != UDESC_DEVICE &&
	    dev->ud_quirks->uq_flags & UQ_DESC_CORRUPT) {
		err = usbd_get_desc_fake(dev, type, index, len, desc);
		goto out;
	}

	req.bmRequestType = UT_READ_DEVICE;
	req.bRequest = UR_GET_DESCRIPTOR;
	USETW2(req.wValue, type, index);
	USETW(req.wIndex, 0);
	USETW(req.wLength, len);
	err = usbd_do_request(dev, &req, desc);

out:
	return err;
}

usbd_status
usbd_get_config_desc(struct usbd_device *dev, int confidx,
		     usb_config_descriptor_t *d)
{
	USBHIST_FUNC();
	USBHIST_CALLARGS(usbdebug, "confidx=%jd", confidx, 0, 0, 0);
	usbd_status err;

	err = usbd_get_desc(dev, UDESC_CONFIG, confidx,
			    USB_CONFIG_DESCRIPTOR_SIZE, d);
	if (err)
		return err;
	if (d->bDescriptorType != UDESC_CONFIG) {
		DPRINTFN(1, "confidx=%jd, bad desc len=%d type=%d",
		    confidx, d->bLength, d->bDescriptorType, 0);
		return USBD_INVAL;
	}
	return USBD_NORMAL_COMPLETION;
}

usbd_status
usbd_get_config_desc_full(struct usbd_device *dev, int conf, void *d, int size)
{
	USBHIST_FUNC(); USBHIST_CALLARGS(usbdebug, "conf=%jd", conf, 0, 0, 0);

	return usbd_get_desc(dev, UDESC_CONFIG, conf, size, d);
}

usbd_status
usbd_get_bos_desc(struct usbd_device *dev, int confidx,
		     usb_bos_descriptor_t *d)
{
	USBHIST_FUNC();
	USBHIST_CALLARGS(usbdebug, "confidx=%jd", confidx, 0, 0, 0);
	usbd_status err;

	err = usbd_get_desc(dev, UDESC_BOS, confidx,
			    USB_BOS_DESCRIPTOR_SIZE, d);
	if (err)
		return err;
	if (d->bDescriptorType != UDESC_BOS) {
		DPRINTFN(1, "confidx=%jd, bad desc len=%d type=%d",
		    confidx, d->bLength, d->bDescriptorType, 0);
		return USBD_INVAL;
	}
	return USBD_NORMAL_COMPLETION;
}

usbd_status
usbd_get_bos_desc_full(struct usbd_device *dev, int conf, void *d, int size)
{
	USBHIST_FUNC(); USBHIST_CALLARGS(usbdebug, "conf=%jd", conf, 0, 0, 0);

	return usbd_get_desc(dev, UDESC_BOS, conf, size, d);
}

usbd_status
usbd_get_device_desc(struct usbd_device *dev, usb_device_descriptor_t *d)
{
	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);

	return usbd_get_desc(dev, UDESC_DEVICE,
			     0, USB_DEVICE_DESCRIPTOR_SIZE, d);
}

/*
 * Get the first 8 bytes of the device descriptor.
 * Do as Windows does: try to read 64 bytes -- there are devices which
 * recognize the initial descriptor fetch (before the control endpoint's
 * MaxPacketSize is known by the host) by exactly this length.
 */
usbd_status
usbd_get_initial_ddesc(struct usbd_device *dev, usb_device_descriptor_t *desc)
{
	USBHIST_FUNC();
	USBHIST_CALLARGS(usbdebug, "dev %#jx", (uintptr_t)dev, 0, 0, 0);
	usb_device_request_t req;
	char buf[64];
	int res, actlen;

	req.bmRequestType = UT_READ_DEVICE;
	req.bRequest = UR_GET_DESCRIPTOR;
	USETW2(req.wValue, UDESC_DEVICE, 0);
	USETW(req.wIndex, 0);
	USETW(req.wLength, 8);
	res = usbd_do_request_flags(dev, &req, buf, USBD_SHORT_XFER_OK,
		&actlen, USBD_DEFAULT_TIMEOUT);
	if (res)
		return res;
	if (actlen < 8)
		return USBD_SHORT_XFER;
	memcpy(desc, buf, 8);
	return USBD_NORMAL_COMPLETION;
}

usbd_status
usbd_get_string_desc(struct usbd_device *dev, int sindex, int langid,
    usb_string_descriptor_t *sdesc, int *sizep)
{
	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);
	usb_device_request_t req;
	usbd_status err;
	int actlen;

	/*
	 * Pass a full-sized buffer to usbd_do_request_len().  At least
	 * one device has been seen returning additional data beyond the
	 * provided buffers (2-bytes written shortly after the request
	 * claims to have completed and returned the 2 byte header,
	 * corrupting other memory.)
	 */
	req.bmRequestType = UT_READ_DEVICE;
	req.bRequest = UR_GET_DESCRIPTOR;
	USETW2(req.wValue, UDESC_STRING, sindex);
	USETW(req.wIndex, langid);
	USETW(req.wLength, 2);	/* only size byte first */
	err = usbd_do_request_len(dev, &req, sizeof(*sdesc), sdesc,
	    USBD_SHORT_XFER_OK, &actlen, USBD_DEFAULT_TIMEOUT);
	if (err)
		return err;

	if (actlen < 2)
		return USBD_SHORT_XFER;

	if (sdesc->bLength > sizeof(*sdesc))
		return USBD_INVAL;
	USETW(req.wLength, sdesc->bLength);	/* the whole string */
	err = usbd_do_request_len(dev, &req, sizeof(*sdesc), sdesc,
	    USBD_SHORT_XFER_OK, &actlen, USBD_DEFAULT_TIMEOUT);
	if (err)
		return err;

	if (actlen != sdesc->bLength) {
		DPRINTF("expected %jd, got %jd", sdesc->bLength, actlen, 0, 0);
	}

	*sizep = actlen;
	return USBD_NORMAL_COMPLETION;
}

/* -------------------------------------------------------------------------- */

usbd_status
usbd_get_device_status(struct usbd_device *dev, usb_status_t *st)
{
	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);
	usb_device_request_t req;

	req.bmRequestType = UT_READ_DEVICE;
	req.bRequest = UR_GET_STATUS;
	USETW(req.wValue, 0);
	USETW(req.wIndex, 0);
	USETW(req.wLength, sizeof(usb_status_t));
	return usbd_do_request(dev, &req, st);
}

usbd_status
usbd_get_hub_status(struct usbd_device *dev, usb_hub_status_t *st)
{
	USBHIST_FUNC();
	USBHIST_CALLARGS(usbdebug, "dev %#jx", (uintptr_t)dev, 0, 0, 0);
	usb_device_request_t req;

	req.bmRequestType = UT_READ_CLASS_DEVICE;
	req.bRequest = UR_GET_STATUS;
	USETW(req.wValue, 0);
	USETW(req.wIndex, 0);
	USETW(req.wLength, sizeof(usb_hub_status_t));
	return usbd_do_request(dev, &req, st);
}

usbd_status
usbd_get_port_status(struct usbd_device *dev, int port, usb_port_status_t *ps)
{
	USBHIST_FUNC();
	USBHIST_CALLARGS(usbdebug, "dev %#jx port %jd",
	    (uintptr_t)dev, port, 0, 0);
	usb_device_request_t req;

	req.bmRequestType = UT_READ_CLASS_OTHER;
	req.bRequest = UR_GET_STATUS;
	USETW(req.wValue, 0);
	USETW(req.wIndex, port);
	USETW(req.wLength, sizeof(*ps));
	return usbd_do_request(dev, &req, ps);
}

/* USB 3.1 10.16.2.6, 10.16.2.6.3 */
usbd_status
usbd_get_port_status_ext(struct usbd_device *dev, int port,
    usb_port_status_ext_t *pse)
{
	USBHIST_FUNC();
	USBHIST_CALLARGS(usbdebug, "dev %#jx port %jd",
	    (uintptr_t)dev, port, 0, 0);
	usb_device_request_t req;

	req.bmRequestType = UT_READ_CLASS_OTHER;
	req.bRequest = UR_GET_STATUS;
	USETW2(req.wValue, 0, UR_PST_EXT_PORT_STATUS);
	USETW(req.wIndex, port);
	USETW(req.wLength, sizeof(*pse));
	return usbd_do_request(dev, &req, pse);
}

/* -------------------------------------------------------------------------- */

usbd_status
usbd_clear_hub_feature(struct usbd_device *dev, int sel)
{
	USBHIST_FUNC();
	USBHIST_CALLARGS(usbdebug, "dev %#jx sel %jd",
	    (uintptr_t)dev, sel, 0, 0);
	usb_device_request_t req;

	req.bmRequestType = UT_WRITE_CLASS_DEVICE;
	req.bRequest = UR_CLEAR_FEATURE;
	USETW(req.wValue, sel);
	USETW(req.wIndex, 0);
	USETW(req.wLength, 0);
	return usbd_do_request(dev, &req, 0);
}

usbd_status
usbd_set_hub_feature(struct usbd_device *dev, int sel)
{
	USBHIST_FUNC();
	USBHIST_CALLARGS(usbdebug,
	    "dev %#jx sel %jd", (uintptr_t)dev, sel, 0, 0);
	usb_device_request_t req;

	req.bmRequestType = UT_WRITE_CLASS_DEVICE;
	req.bRequest = UR_SET_FEATURE;
	USETW(req.wValue, sel);
	USETW(req.wIndex, 0);
	USETW(req.wLength, 0);
	return usbd_do_request(dev, &req, 0);
}

usbd_status
usbd_clear_port_feature(struct usbd_device *dev, int port, int sel)
{
	USBHIST_FUNC();
	USBHIST_CALLARGS(usbdebug, "dev %#jx port %jd sel %jd",
	    (uintptr_t)dev, port, sel, 0);
	usb_device_request_t req;

	req.bmRequestType = UT_WRITE_CLASS_OTHER;
	req.bRequest = UR_CLEAR_FEATURE;
	USETW(req.wValue, sel);
	USETW(req.wIndex, port);
	USETW(req.wLength, 0);
	return usbd_do_request(dev, &req, 0);
}

usbd_status
usbd_set_port_feature(struct usbd_device *dev, int port, int sel)
{
	USBHIST_FUNC();
	USBHIST_CALLARGS(usbdebug, "dev %#jx port %jd sel %.d",
	    (uintptr_t)dev, sel, 0, 0);
	usb_device_request_t req;

	req.bmRequestType = UT_WRITE_CLASS_OTHER;
	req.bRequest = UR_SET_FEATURE;
	USETW(req.wValue, sel);
	USETW(req.wIndex, port);
	USETW(req.wLength, 0);
	return usbd_do_request(dev, &req, 0);
}

usbd_status
usbd_set_port_u1_timeout(struct usbd_device *dev, int port, int timeout)
{
	USBHIST_FUNC();
	USBHIST_CALLARGS(usbdebug, "dev %#jx port %jd timeout %.d",
	    (uintptr_t)dev, port, timeout, 0);
	usb_device_request_t req;

	req.bmRequestType = UT_WRITE_CLASS_OTHER;
	req.bRequest = UR_SET_FEATURE;
	USETW(req.wValue, UHF_PORT_U1_TIMEOUT);
	USETW2(req.wIndex, timeout, port);
	USETW(req.wLength, 0);
	return usbd_do_request(dev, &req, 0);
}

usbd_status
usbd_set_port_u2_timeout(struct usbd_device *dev, int port, int timeout)
{
	USBHIST_FUNC();
	USBHIST_CALLARGS(usbdebug, "dev %#jx port %jd timeout %jd",
	    (uintptr_t)dev, port, timeout, 0);
	usb_device_request_t req;

	req.bmRequestType = UT_WRITE_CLASS_OTHER;
	req.bRequest = UR_SET_FEATURE;
	USETW(req.wValue, UHF_PORT_U2_TIMEOUT);
	USETW2(req.wIndex, timeout, port);
	USETW(req.wLength, 0);
	return usbd_do_request(dev, &req, 0);
}

usbd_status
usbd_clear_endpoint_feature(struct usbd_device *dev, int epaddr, int sel)
{
	USBHIST_FUNC();
	USBHIST_CALLARGS(usbdebug, "dev %#jx epaddr %jd sel %jd",
	    (uintptr_t)dev, epaddr, sel, 0);
	usb_device_request_t req;

	req.bmRequestType = UT_WRITE_ENDPOINT;
	req.bRequest = UR_CLEAR_FEATURE;
	USETW(req.wValue, sel);
	USETW(req.wIndex, epaddr);
	USETW(req.wLength, 0);
	return usbd_do_request(dev, &req, 0);
}

/* -------------------------------------------------------------------------- */

usbd_status
usbd_get_config(struct usbd_device *dev, uint8_t *conf)
{
	USBHIST_FUNC();
	USBHIST_CALLARGS(usbdebug, "dev %#jx", (uintptr_t)dev, 0, 0, 0);
	usb_device_request_t req;

	req.bmRequestType = UT_READ_DEVICE;
	req.bRequest = UR_GET_CONFIG;
	USETW(req.wValue, 0);
	USETW(req.wIndex, 0);
	USETW(req.wLength, 1);
	return usbd_do_request(dev, &req, conf);
}

usbd_status
usbd_set_config(struct usbd_device *dev, int conf)
{
	USBHIST_FUNC();
	USBHIST_CALLARGS(usbdebug, "dev %#jx conf %jd",
	    (uintptr_t)dev, conf, 0, 0);
	usb_device_request_t req;

	req.bmRequestType = UT_WRITE_DEVICE;
	req.bRequest = UR_SET_CONFIG;
	USETW(req.wValue, conf);
	USETW(req.wIndex, 0);
	USETW(req.wLength, 0);
	return usbd_do_request(dev, &req, 0);
}

usbd_status
usbd_set_address(struct usbd_device *dev, int addr)
{
	USBHIST_FUNC();
	USBHIST_CALLARGS(usbdebug, "dev %#jx addr %jd",
	    (uintptr_t)dev, addr, 0, 0);
	usb_device_request_t req;

	req.bmRequestType = UT_WRITE_DEVICE;
	req.bRequest = UR_SET_ADDRESS;
	USETW(req.wValue, addr);
	USETW(req.wIndex, 0);
	USETW(req.wLength, 0);
	return usbd_do_request(dev, &req, 0);
}

usbd_status
usbd_set_idle(struct usbd_interface *iface, int duration, int id)
{
	usb_interface_descriptor_t *ifd = usbd_get_interface_descriptor(iface);
	struct usbd_device *dev;
	usb_device_request_t req;

	USBHIST_FUNC();
	USBHIST_CALLARGS(usbdebug, "duration %jd id %jd", duration, id, 0, 0);

	if (ifd == NULL)
		return USBD_IOERROR;
	usbd_interface2device_handle(iface, &dev);
	req.bmRequestType = UT_WRITE_CLASS_INTERFACE;
	req.bRequest = UR_SET_IDLE;
	USETW2(req.wValue, duration, id);
	USETW(req.wIndex, ifd->bInterfaceNumber);
	USETW(req.wLength, 0);
	return usbd_do_request(dev, &req, 0);
}

/* -------------------------------------------------------------------------- */

usbd_status
usbd_get_protocol(struct usbd_interface *iface, uint8_t *report)
{
	usb_interface_descriptor_t *id = usbd_get_interface_descriptor(iface);
	struct usbd_device *dev;
	usb_device_request_t req;

	USBHIST_FUNC();
	USBHIST_CALLARGS(usbdebug, "iface=%#jx, endpt=%jd",
	    (uintptr_t)iface, id->bInterfaceNumber, 0, 0);

	if (id == NULL)
		return USBD_IOERROR;

	usbd_interface2device_handle(iface, &dev);
	req.bmRequestType = UT_READ_CLASS_INTERFACE;
	req.bRequest = UR_GET_PROTOCOL;
	USETW(req.wValue, 0);
	USETW(req.wIndex, id->bInterfaceNumber);
	USETW(req.wLength, 1);
	return usbd_do_request(dev, &req, report);
}

usbd_status
usbd_set_protocol(struct usbd_interface *iface, int report)
{
	usb_interface_descriptor_t *id = usbd_get_interface_descriptor(iface);
	struct usbd_device *dev;
	usb_device_request_t req;

	USBHIST_FUNC();
	USBHIST_CALLARGS(usbdebug, "iface=%#jx, report=%jd, endpt=%jd",
	    (uintptr_t)iface, report, id->bInterfaceNumber, 0);

	if (id == NULL)
		return USBD_IOERROR;

	usbd_interface2device_handle(iface, &dev);
	req.bmRequestType = UT_WRITE_CLASS_INTERFACE;
	req.bRequest = UR_SET_PROTOCOL;
	USETW(req.wValue, report);
	USETW(req.wIndex, id->bInterfaceNumber);
	USETW(req.wLength, 0);
	return usbd_do_request(dev, &req, 0);
}

/* -------------------------------------------------------------------------- */

usbd_status
usbd_set_report(struct usbd_interface *iface, int type, int id, void *data,
		int len)
{
	usb_interface_descriptor_t *ifd = usbd_get_interface_descriptor(iface);
	struct usbd_device *dev;
	usb_device_request_t req;

	USBHIST_FUNC();
	USBHIST_CALLARGS(usbdebug, "len=%jd", len, 0, 0, 0);

	if (ifd == NULL)
		return USBD_IOERROR;
	usbd_interface2device_handle(iface, &dev);
	req.bmRequestType = UT_WRITE_CLASS_INTERFACE;
	req.bRequest = UR_SET_REPORT;
	USETW2(req.wValue, type, id);
	USETW(req.wIndex, ifd->bInterfaceNumber);
	USETW(req.wLength, len);
	return usbd_do_request(dev, &req, data);
}

usbd_status
usbd_get_report(struct usbd_interface *iface, int type, int id, void *data,
		int len)
{
	usb_interface_descriptor_t *ifd = usbd_get_interface_descriptor(iface);
	struct usbd_device *dev;
	usb_device_request_t req;

	USBHIST_FUNC(); USBHIST_CALLARGS(usbdebug, "len=%jd", len, 0, 0, 0);

	if (ifd == NULL)
		return USBD_IOERROR;
	usbd_interface2device_handle(iface, &dev);
	req.bmRequestType = UT_READ_CLASS_INTERFACE;
	req.bRequest = UR_GET_REPORT;
	USETW2(req.wValue, type, id);
	USETW(req.wIndex, ifd->bInterfaceNumber);
	USETW(req.wLength, len);
	return usbd_do_request(dev, &req, data);
}

usbd_status
usbd_get_report_descriptor(struct usbd_device *dev, int ifcno,
			   int size, void *d)
{
	USBHIST_FUNC();
	USBHIST_CALLARGS(usbdebug, "dev %#jx ifcno %jd size %jd",
	    (uintptr_t)dev, ifcno, size, 0);
	usb_device_request_t req;

	req.bmRequestType = UT_READ_INTERFACE;
	req.bRequest = UR_GET_DESCRIPTOR;
	USETW2(req.wValue, UDESC_REPORT, 0); /* report id should be 0 */
	USETW(req.wIndex, ifcno);
	USETW(req.wLength, size);
	return usbd_do_request(dev, &req, d);
}

/* -------------------------------------------------------------------------- */

usb_hid_descriptor_t *
usbd_get_hid_descriptor(struct usbd_interface *ifc)
{
	usb_interface_descriptor_t *idesc = usbd_get_interface_descriptor(ifc);
	struct usbd_device *dev;
	usb_config_descriptor_t *cdesc;
	usb_hid_descriptor_t *hd;
	char *p, *end;

	if (idesc == NULL)
		return NULL;
	usbd_interface2device_handle(ifc, &dev);
	cdesc = usbd_get_config_descriptor(dev);

	p = (char *)idesc + idesc->bLength;
	end = (char *)cdesc + UGETW(cdesc->wTotalLength);

	for (; p < end; p += hd->bLength) {
		hd = (usb_hid_descriptor_t *)p;
		if (p + hd->bLength <= end &&
		    hd->bLength >= USB_HID_DESCRIPTOR_SIZE(0) &&
		    hd->bDescriptorType == UDESC_HID)
			return hd;
		if (hd->bDescriptorType == UDESC_INTERFACE)
			break;
	}
	return NULL;
}

usbd_status
usbd_read_report_desc(struct usbd_interface *ifc, void **descp, int *sizep)
{
	usb_interface_descriptor_t *id;
	usb_hid_descriptor_t *hid;
	struct usbd_device *dev;
	usbd_status err;

	usbd_interface2device_handle(ifc, &dev);
	id = usbd_get_interface_descriptor(ifc);
	if (id == NULL)
		return USBD_INVAL;
	hid = usbd_get_hid_descriptor(ifc);
	if (hid == NULL)
		return USBD_IOERROR;
	*sizep = UGETW(hid->descrs[0].wDescriptorLength);
	if (*sizep == 0)
		return USBD_INVAL;
	*descp = kmem_alloc(*sizep, KM_SLEEP);
	err = usbd_get_report_descriptor(dev, id->bInterfaceNumber,
					 *sizep, *descp);
	if (err) {
		kmem_free(*descp, *sizep);
		*descp = NULL;
		return err;
	}
	return USBD_NORMAL_COMPLETION;
}

usbd_status
usbd_bulk_transfer(struct usbd_xfer *xfer, struct usbd_pipe *pipe,
    uint16_t flags, uint32_t timeout, void *buf, uint32_t *size)
{
	usbd_status err;

	USBHIST_FUNC();
	USBHIST_CALLARGS(usbdebug, "start transfer %jd bytes", *size, 0, 0, 0);

	usbd_setup_xfer(xfer, 0, buf, *size, flags, timeout, NULL);
	err = usbd_sync_transfer_sig(xfer);

	usbd_get_xfer_status(xfer, NULL, NULL, size, NULL);
	DPRINTFN(1, "transferred %jd", *size, 0, 0, 0);
	if (err) {
		usbd_clear_endpoint_stall(pipe);
	}
	USBHIST_LOG(usbdebug, "<- done xfer %#jx err %d", (uintptr_t)xfer,
	    err, 0, 0);

	return err;
}

usbd_status
usbd_intr_transfer(struct usbd_xfer *xfer, struct usbd_pipe *pipe,
    uint16_t flags, uint32_t timeout, void *buf, uint32_t *size)
{
	usbd_status err;

	USBHIST_FUNC();
	USBHIST_CALLARGS(usbdebug, "start transfer %jd bytes", *size, 0, 0, 0);

	usbd_setup_xfer(xfer, 0, buf, *size, flags, timeout, NULL);

	err = usbd_sync_transfer_sig(xfer);

	usbd_get_xfer_status(xfer, NULL, NULL, size, NULL);

	DPRINTFN(1, "transferred %jd", *size, 0, 0, 0);
	if (err) {
		usbd_clear_endpoint_stall(pipe);
	}
	USBHIST_LOG(usbdebug, "<- done xfer %#jx err %jd", (uintptr_t)xfer,
	    err, 0, 0);

	return err;
}

void
usb_detach_waitold(device_t dv)
{
	USBHIST_FUNC();
	USBHIST_CALLARGS(usbdebug, "waiting for dv %#jx",
	    (uintptr_t)dv, 0, 0, 0);

	if (tsleep(dv, PZERO, "usbdet", hz * 60)) /* XXXSMP ok */
		aprint_error_dev(dv, "usb_detach_waitold: didn't detach\n");
	DPRINTFN(1, "done", 0, 0, 0, 0);
}

void
usb_detach_wakeupold(device_t dv)
{
	USBHIST_FUNC();
	USBHIST_CALLARGS(usbdebug, "for dv %#jx", (uintptr_t)dv, 0, 0, 0);

	wakeup(dv); /* XXXSMP ok */
}

/* -------------------------------------------------------------------------- */

void
usb_desc_iter_init(struct usbd_device *dev, usbd_desc_iter_t *iter)
{
	const usb_config_descriptor_t *cd = usbd_get_config_descriptor(dev);

	iter->cur = (const uByte *)cd;
	iter->end = (const uByte *)cd + UGETW(cd->wTotalLength);
}

const usb_descriptor_t *
usb_desc_iter_peek(usbd_desc_iter_t *iter)
{
	const usb_descriptor_t *desc;

	if (iter->cur + sizeof(usb_descriptor_t) >= iter->end) {
		if (iter->cur != iter->end)
			printf("%s: bad descriptor\n", __func__);
		return NULL;
	}
	desc = (const usb_descriptor_t *)iter->cur;
	if (desc->bLength < USB_DESCRIPTOR_SIZE) {
		printf("%s: descriptor length too small\n", __func__);
		return NULL;
	}
	if (iter->cur + desc->bLength > iter->end) {
		printf("%s: descriptor length too large\n", __func__);
		return NULL;
	}
	return desc;
}

const usb_descriptor_t *
usb_desc_iter_next(usbd_desc_iter_t *iter)
{
	const usb_descriptor_t *desc = usb_desc_iter_peek(iter);
	if (desc == NULL)
		return NULL;
	iter->cur += desc->bLength;
	return desc;
}

/* Return the next interface descriptor, skipping over any other
 * descriptors.  Returns NULL at the end or on error. */
const usb_interface_descriptor_t *
usb_desc_iter_next_interface(usbd_desc_iter_t *iter)
{
	const usb_descriptor_t *desc;

	while ((desc = usb_desc_iter_peek(iter)) != NULL &&
	       desc->bDescriptorType != UDESC_INTERFACE)
	{
		usb_desc_iter_next(iter);
	}

	return (const usb_interface_descriptor_t *)usb_desc_iter_next(iter);
}

/* Returns the next non-interface descriptor, returning NULL when the
 * next descriptor would be an interface descriptor. */
const usb_descriptor_t *
usb_desc_iter_next_non_interface(usbd_desc_iter_t *iter)
{
	const usb_descriptor_t *desc;

	if ((desc = usb_desc_iter_peek(iter)) != NULL &&
	    desc->bDescriptorType != UDESC_INTERFACE)
	{
		return usb_desc_iter_next(iter);
	} else {
		return NULL;
	}
}

const usb_cdc_descriptor_t *
usb_find_desc(struct usbd_device *dev, int type, int subtype)
{
	usbd_desc_iter_t iter;
	const usb_cdc_descriptor_t *desc;

	usb_desc_iter_init(dev, &iter);
	for (;;) {
		desc = (const usb_cdc_descriptor_t *)usb_desc_iter_next(&iter);
		if (!desc || (desc->bDescriptorType == type &&
			      (subtype == USBD_CDCSUBTYPE_ANY ||
			       subtype == desc->bDescriptorSubtype)))
			break;
	}
	return desc;
}

/* same as usb_find_desc(), but searches only in the specified interface. */
const usb_cdc_descriptor_t *
usb_find_desc_if(struct usbd_device *dev, int type, int subtype,
		 usb_interface_descriptor_t *id)
{
	usbd_desc_iter_t iter;
	const usb_cdc_descriptor_t *desc;

	if (id == NULL)
		return usb_find_desc(dev, type, subtype);

	usb_desc_iter_init(dev, &iter);

	iter.cur = (void *)id;		/* start from the interface desc */
	usb_desc_iter_next(&iter);	/* and skip it */

	while ((desc = (const usb_cdc_descriptor_t *)usb_desc_iter_next(&iter))
	       != NULL) {
		if (desc->bDescriptorType == UDESC_INTERFACE) {
			/* we ran into the next interface --- not found */
			return NULL;
		}
		if (desc->bDescriptorType == type &&
		    (subtype == USBD_CDCSUBTYPE_ANY ||
		     subtype == desc->bDescriptorSubtype))
			break;
	}
	return desc;
}
