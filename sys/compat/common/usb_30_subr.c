/*	$NetBSD: usb_30_subr.c,v 1.1.2.1 2018/03/29 11:20:02 pgoyette Exp $	*/
/*	$FreeBSD: src/sys/dev/usb/usb_subr.c,v 1.18 1999/11/17 22:33:47 n_hibma Exp $	*/

/*
 * Copyright (c) 1998, 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at
 * Carlstedt Research & Technology.
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
__KERNEL_RCSID(0, "$NetBSD: usb_30_subr.c,v 1.1.2.1 2018/03/29 11:20:02 pgoyette Exp $");

#ifdef _KERNEL_OPT
#include "opt_compat_netbsd.h"
#include "opt_usb.h"
#include "opt_usbverbose.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/device.h>
#include <sys/select.h>
#include <sys/proc.h>

#include <sys/bus.h>
#include <sys/module.h>
#include <sys/compat_stub.h>

#include <compat/common/compat_mod.h>

#include <dev/usb/usb.h>

#include <dev/usb/usbdi.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usbdevs.h>
#include <dev/usb/usb_quirks.h>
#include <dev/usb/usb_verbose.h>
#include <dev/usb/usbhist.h>

Static void usb_copy_old_devinfo(struct usb_device_info_old *,
    const struct usb_device_info *);

Static void
usb_copy_old_devinfo(struct usb_device_info_old *uo,
		     const struct usb_device_info *ue)
{
	const unsigned char *p;
	unsigned char *q;
	int i, n;

	uo->udi_bus = ue->udi_bus;
	uo->udi_addr = ue->udi_addr;
	uo->udi_cookie = ue->udi_cookie;
	for (i = 0, p = (const unsigned char *)ue->udi_product,
	     q = (unsigned char *)uo->udi_product;
	     *p && i < USB_MAX_STRING_LEN - 1; p++) {
		if (*p < 0x80)
			q[i++] = *p;
		else {
			q[i++] = '?';
			if ((*p & 0xe0) == 0xe0)
				p++;
			p++;
		}
	}
	q[i] = 0;

	for (i = 0, p = ue->udi_vendor, q = uo->udi_vendor;
	     *p && i < USB_MAX_STRING_LEN - 1; p++) {
		if (* p < 0x80)
			q[i++] = *p;
		else {
			q[i++] = '?';
			p++;
			if ((*p & 0xe0) == 0xe0)
				p++;
		}
	}
	q[i] = 0;

	memcpy(uo->udi_release, ue->udi_release, sizeof(uo->udi_release));

	uo->udi_productNo = ue->udi_productNo;
	uo->udi_vendorNo = ue->udi_vendorNo;
	uo->udi_releaseNo = ue->udi_releaseNo;
	uo->udi_class = ue->udi_class;
	uo->udi_subclass = ue->udi_subclass;
	uo->udi_protocol = ue->udi_protocol;
	uo->udi_config = ue->udi_config;
	uo->udi_speed = ue->udi_speed;
	uo->udi_power = ue->udi_power;
	uo->udi_nports = ue->udi_nports;

	for (n=0; n<USB_MAX_DEVNAMES; n++)
		memcpy(uo->udi_devnames[n],
		       ue->udi_devnames[n], USB_MAX_DEVNAMELEN);
	memcpy(uo->udi_ports, ue->udi_ports, sizeof(uo->udi_ports));
}

static int
usbd_fill_deviceinfo_old(struct usbd_device *dev,
    struct usb_device_info_old *di, int usedev)
{
	struct usbd_port *p;
	int i, j, err;

	di->udi_bus = device_unit(dev->ud_bus->ub_usbctl);
	di->udi_addr = dev->ud_addr;
	di->udi_cookie = dev->ud_cookie;
	(*vec_usbd_devinfo_vp)(dev, di->udi_vendor, sizeof(di->udi_vendor),
	    di->udi_product, sizeof(di->udi_product), usedev, 0);
	(*vec_usbd_printBCD)(di->udi_release, sizeof(di->udi_release),
	    UGETW(dev->ud_ddesc.bcdDevice));
	di->udi_vendorNo = UGETW(dev->ud_ddesc.idVendor);
	di->udi_productNo = UGETW(dev->ud_ddesc.idProduct);
	di->udi_releaseNo = UGETW(dev->ud_ddesc.bcdDevice);
	di->udi_class = dev->ud_ddesc.bDeviceClass;
	di->udi_subclass = dev->ud_ddesc.bDeviceSubClass;
	di->udi_protocol = dev->ud_ddesc.bDeviceProtocol;
	di->udi_config = dev->ud_config;
	di->udi_power = dev->ud_selfpowered ? 0 : dev->ud_power;
	di->udi_speed = dev->ud_speed;

	if (dev->ud_subdevlen > 0) {
		for (i = 0, j = 0; i < dev->ud_subdevlen &&
			     j < USB_MAX_DEVNAMES; i++) {
			if (!dev->ud_subdevs[i])
				continue;
			strncpy(di->udi_devnames[j],
			    device_xname(dev->ud_subdevs[i]), USB_MAX_DEVNAMELEN);
			di->udi_devnames[j][USB_MAX_DEVNAMELEN-1] = '\0';
			j++;
		}
	} else {
		j = 0;
	}
	for (/* j is set */; j < USB_MAX_DEVNAMES; j++)
		di->udi_devnames[j][0] = 0;		 /* empty */

	if (!dev->ud_hub) {
		di->udi_nports = 0;
		return 0;
	}

	const int nports = dev->ud_hub->uh_hubdesc.bNbrPorts;
	for (i = 0; i < __arraycount(di->udi_ports) && i < nports;
	     i++) {
		p = &dev->ud_hub->uh_ports[i];
		if (p->up_dev)
			err = p->up_dev->ud_addr;
		else {
			const int s = UGETW(p->up_status.wPortStatus);
			if (s & UPS_PORT_ENABLED)
				err = USB_PORT_ENABLED;
			else if (s & UPS_SUSPEND)
				err = USB_PORT_SUSPENDED;
			else if (s & UPS_PORT_POWER)
				err = USB_PORT_POWERED;
			else
				err = USB_PORT_DISABLED;
		}
		di->udi_ports[i] = err;
	}
	di->udi_nports = nports;

	return 0;
}

static int
usb_copy_to_old30(struct usb_event *ue, struct usb_event_old *ueo,
    struct uio *uio)
{

	ueo->ue_type = ue->ue_type;
	memcpy(&ueo->ue_time, &ue->ue_time, sizeof(struct timespec));
	switch (ue->ue_type) {
		case USB_EVENT_DEVICE_ATTACH:
		case USB_EVENT_DEVICE_DETACH:
			usb_copy_old_devinfo(&ueo->u.ue_device,
			    &ue->u.ue_device);
			break;

		case USB_EVENT_CTRLR_ATTACH:
		case USB_EVENT_CTRLR_DETACH:
			ueo->u.ue_ctrlr.ue_bus=ue->u.ue_ctrlr.ue_bus;
			break;

		case USB_EVENT_DRIVER_ATTACH:
		case USB_EVENT_DRIVER_DETACH:
			ueo->u.ue_driver.ue_cookie=ue->u.ue_driver.ue_cookie;
			memcpy(ueo->u.ue_driver.ue_devname,
			    ue->u.ue_driver.ue_devname,
			    sizeof(ue->u.ue_driver.ue_devname));
			break;
		default:
			;
	}

	return 0;
}

void
usb_30_init(void)
{

	usbd30_fill_deviceinfo_old = usbd_fill_deviceinfo_old;
	usb30_copy_to_old = usb_copy_to_old30;
}

void
usb_30_fini(void)
{

	usbd30_fill_deviceinfo_old = (void *)enosys;
	usb30_copy_to_old = (void *)enosys;
}
