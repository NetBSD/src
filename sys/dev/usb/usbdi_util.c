/*	$NetBSD: usbdi_util.c,v 1.66.2.2 2018/11/12 16:01:35 martin Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: usbdi_util.c,v 1.66.2.2 2018/11/12 16:01:35 martin Exp $");

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

	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);

	DPRINTFN(3,"type=%jd, index=%jd, len=%jd", type, index, len, 0);

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
	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);
	usbd_status err;

	DPRINTFN(3, "confidx=%jd", confidx, 0, 0, 0);
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
	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);

	DPRINTFN(3, "conf=%jd", conf, 0, 0, 0);
	return usbd_get_desc(dev, UDESC_CONFIG, conf, size, d);
}

usbd_status
usbd_get_bos_desc(struct usbd_device *dev, int confidx,
		     usb_bos_descriptor_t *d)
{
	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);
	usbd_status err;

	DPRINTFN(3, "confidx=%jd", confidx, 0, 0, 0);
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
	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);

	DPRINTFN(3, "conf=%jd", conf, 0, 0, 0);
	return usbd_get_desc(dev, UDESC_BOS, conf, size, d);
}

usbd_status
usbd_get_device_desc(struct usbd_device *dev, usb_device_descriptor_t *d)
{
	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);

	return usbd_get_desc(dev, UDESC_DEVICE,
			     0, USB_DEVICE_DESCRIPTOR_SIZE, d);
}

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
	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);
	usb_device_request_t req;

	DPRINTF("dev %#jx", (uintptr_t)dev, 0, 0, 0);
	req.bmRequestType = UT_READ_CLASS_DEVICE;
	req.bRequest = UR_GET_STATUS;
	USETW(req.wValue, 0);
	USETW(req.wIndex, 0);
	USETW(req.wLength, sizeof(usb_hub_status_t));
	return usbd_do_request(dev, &req, st);
}

usbd_status
usbd_set_address(struct usbd_device *dev, int addr)
{
	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);
	usb_device_request_t req;

	DPRINTF("dev %#jx addr %jd", (uintptr_t)dev, addr, 0, 0);
	req.bmRequestType = UT_WRITE_DEVICE;
	req.bRequest = UR_SET_ADDRESS;
	USETW(req.wValue, addr);
	USETW(req.wIndex, 0);
	USETW(req.wLength, 0);
	return usbd_do_request(dev, &req, 0);
}

usbd_status
usbd_get_port_status(struct usbd_device *dev, int port, usb_port_status_t *ps)
{
	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);
	usb_device_request_t req;

	DPRINTF("dev %#jx port %jd", (uintptr_t)dev, port, 0, 0);
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
	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);
	usb_device_request_t req;

	DPRINTF("dev %#jx port %jd", (uintptr_t)dev, port, 0, 0);
	req.bmRequestType = UT_READ_CLASS_OTHER;
	req.bRequest = UR_GET_STATUS;
	USETW2(req.wValue, 0, UR_PST_EXT_PORT_STATUS);
	USETW(req.wIndex, port);
	USETW(req.wLength, sizeof(*pse));
	return usbd_do_request(dev, &req, pse);
}

usbd_status
usbd_clear_hub_feature(struct usbd_device *dev, int sel)
{
	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);
	usb_device_request_t req;

	DPRINTF("dev %#jx sel %jd", (uintptr_t)dev, sel, 0, 0);
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
	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);
	usb_device_request_t req;

	DPRINTF("dev %#jx sel %jd", (uintptr_t)dev, sel, 0, 0);
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
	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);
	usb_device_request_t req;

	DPRINTF("dev %#jx port %jd sel %jd", (uintptr_t)dev, port, sel, 0);
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
	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);
	usb_device_request_t req;

	DPRINTF("dev %#jx port %jd sel %.d", (uintptr_t)dev, sel, 0, 0);
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
	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);
	usb_device_request_t req;

	DPRINTF("dev %#jx port %jd timeout %.d", (uintptr_t)dev, port,
	    timeout, 0);
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
	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);
	usb_device_request_t req;

	DPRINTF("dev %#jx port %jd timeout %jd", (uintptr_t)dev, port,
	    timeout, 0);
	req.bmRequestType = UT_WRITE_CLASS_OTHER;
	req.bRequest = UR_SET_FEATURE;
	USETW(req.wValue, UHF_PORT_U2_TIMEOUT);
	USETW2(req.wIndex, timeout, port);
	USETW(req.wLength, 0);
	return usbd_do_request(dev, &req, 0);
}

usbd_status
usbd_get_protocol(struct usbd_interface *iface, uint8_t *report)
{
	usb_interface_descriptor_t *id = usbd_get_interface_descriptor(iface);
	struct usbd_device *dev;
	usb_device_request_t req;

	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);

	if (id == NULL)
		return USBD_IOERROR;
	DPRINTFN(4, "iface=%#jx, endpt=%jd", (uintptr_t)iface,
	    id->bInterfaceNumber, 0, 0);

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

	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);

	if (id == NULL)
		return USBD_IOERROR;
	DPRINTFN(4, "iface=%#jx, report=%jd, endpt=%jd", (uintptr_t)iface,
	    report, id->bInterfaceNumber, 0);

	usbd_interface2device_handle(iface, &dev);
	req.bmRequestType = UT_WRITE_CLASS_INTERFACE;
	req.bRequest = UR_SET_PROTOCOL;
	USETW(req.wValue, report);
	USETW(req.wIndex, id->bInterfaceNumber);
	USETW(req.wLength, 0);
	return usbd_do_request(dev, &req, 0);
}

usbd_status
usbd_set_report(struct usbd_interface *iface, int type, int id, void *data,
		int len)
{
	usb_interface_descriptor_t *ifd = usbd_get_interface_descriptor(iface);
	struct usbd_device *dev;
	usb_device_request_t req;

	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);

	DPRINTFN(4, "len=%jd", len, 0, 0, 0);
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

	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);

	DPRINTFN(4, "len=%jd", len, 0, 0, 0);
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
usbd_set_idle(struct usbd_interface *iface, int duration, int id)
{
	usb_interface_descriptor_t *ifd = usbd_get_interface_descriptor(iface);
	struct usbd_device *dev;
	usb_device_request_t req;

	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);

	DPRINTFN(4, "duration %jd id %jd", duration, id, 0, 0);
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

usbd_status
usbd_get_report_descriptor(struct usbd_device *dev, int ifcno,
			   int size, void *d)
{
	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);
	usb_device_request_t req;

	DPRINTF("dev %#jx ifcno %jd size %jd", (uintptr_t)dev, ifcno, size, 0);
	req.bmRequestType = UT_READ_INTERFACE;
	req.bRequest = UR_GET_DESCRIPTOR;
	USETW2(req.wValue, UDESC_REPORT, 0); /* report id should be 0 */
	USETW(req.wIndex, ifcno);
	USETW(req.wLength, size);
	return usbd_do_request(dev, &req, d);
}

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
		if (p + hd->bLength <= end && hd->bDescriptorType == UDESC_HID)
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
usbd_get_config(struct usbd_device *dev, uint8_t *conf)
{
	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);
	usb_device_request_t req;

	DPRINTF("dev %#jx", (uintptr_t)dev, 0, 0, 0);
	req.bmRequestType = UT_READ_DEVICE;
	req.bRequest = UR_GET_CONFIG;
	USETW(req.wValue, 0);
	USETW(req.wIndex, 0);
	USETW(req.wLength, 1);
	return usbd_do_request(dev, &req, conf);
}

usbd_status
usbd_bulk_transfer(struct usbd_xfer *xfer, struct usbd_pipe *pipe,
    uint16_t flags, uint32_t timeout, void *buf, uint32_t *size)
{
	usbd_status err;

	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);

	usbd_setup_xfer(xfer, 0, buf, *size, flags, timeout, NULL);
	DPRINTFN(1, "start transfer %jd bytes", *size, 0, 0, 0);
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

	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);

	usbd_setup_xfer(xfer, 0, buf, *size, flags, timeout, NULL);

	DPRINTFN(1, "start transfer %jd bytes", *size, 0, 0, 0);
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
usb_detach_wait(device_t dv, kcondvar_t *cv, kmutex_t *lock)
{
	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);

	DPRINTFN(1, "waiting for dv %#jx", (uintptr_t)dv, 0, 0, 0);
	if (cv_timedwait(cv, lock, hz * 60))	// dv, PZERO, "usbdet", hz * 60
		printf("usb_detach_wait: %s didn't detach\n",
			device_xname(dv));
	DPRINTFN(1, "done", 0, 0, 0, 0);
}

void
usb_detach_broadcast(device_t dv, kcondvar_t *cv)
{
	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);

	DPRINTFN(1, "for dv %#jx", (uintptr_t)dv, 0, 0, 0);
	cv_broadcast(cv);
}

void
usb_detach_waitold(device_t dv)
{
	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);

	DPRINTFN(1, "waiting for dv %#jx", (uintptr_t)dv, 0, 0, 0);
	if (tsleep(dv, PZERO, "usbdet", hz * 60)) /* XXXSMP ok */
		printf("usb_detach_waitold: %s didn't detach\n",
			device_xname(dv));
	DPRINTFN(1, "done", 0, 0, 0, 0);
}

void
usb_detach_wakeupold(device_t dv)
{
	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);

	DPRINTFN(1, "for dv %#jx", (uintptr_t)dv, 0, 0, 0);
	wakeup(dv); /* XXXSMP ok */
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
