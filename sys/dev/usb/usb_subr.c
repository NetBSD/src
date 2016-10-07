/*	$NetBSD: usb_subr.c,v 1.198.2.33 2016/10/07 10:35:25 skrll Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: usb_subr.c,v 1.198.2.33 2016/10/07 10:35:25 skrll Exp $");

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

#include <dev/usb/usb.h>

#include <dev/usb/usbdi.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usbdevs.h>
#include <dev/usb/usb_quirks.h>
#include <dev/usb/usb_verbose.h>
#include <dev/usb/usbhist.h>

#include "locators.h"

#define	DPRINTF(FMT,A,B,C,D)	USBHIST_LOG(usbdebug,FMT,A,B,C,D)
#define	DPRINTFN(N,FMT,A,B,C,D)	USBHIST_LOGN(usbdebug,N,FMT,A,B,C,D)

Static usbd_status usbd_set_config(struct usbd_device *, int);
Static void usbd_devinfo(struct usbd_device *, int, char *, size_t);
Static void usbd_devinfo_vp(struct usbd_device *, char *, size_t, char *, size_t,
    int, int);
Static int usbd_getnewaddr(struct usbd_bus *);
Static int usbd_print(void *, const char *);
Static int usbd_ifprint(void *, const char *);
Static void usbd_free_iface_data(struct usbd_device *, int);

uint32_t usb_cookie_no = 0;

Static const char * const usbd_error_strs[] = {
	"NORMAL_COMPLETION",
	"IN_PROGRESS",
	"PENDING_REQUESTS",
	"NOT_STARTED",
	"INVAL",
	"NOMEM",
	"CANCELLED",
	"BAD_ADDRESS",
	"IN_USE",
	"NO_ADDR",
	"SET_ADDR_FAILED",
	"NO_POWER",
	"TOO_DEEP",
	"IOERROR",
	"NOT_CONFIGURED",
	"TIMEOUT",
	"SHORT_XFER",
	"STALLED",
	"INTERRUPTED",
	"XXX",
};

DEV_VERBOSE_DEFINE(usb);

const char *
usbd_errstr(usbd_status err)
{
	static char buffer[5];

	if (err < USBD_ERROR_MAX) {
		return usbd_error_strs[err];
	} else {
		snprintf(buffer, sizeof(buffer), "%d", err);
		return buffer;
	}
}

usbd_status
usbd_get_string_desc(struct usbd_device *dev, int sindex, int langid,
		     usb_string_descriptor_t *sdesc, int *sizep)
{
	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);
	usb_device_request_t req;
	usbd_status err;
	int actlen;

	req.bmRequestType = UT_READ_DEVICE;
	req.bRequest = UR_GET_DESCRIPTOR;
	USETW2(req.wValue, UDESC_STRING, sindex);
	USETW(req.wIndex, langid);
	USETW(req.wLength, 2);	/* only size byte first */
	err = usbd_do_request_flags(dev, &req, sdesc, USBD_SHORT_XFER_OK,
		&actlen, USBD_DEFAULT_TIMEOUT);
	if (err)
		return err;

	if (actlen < 2)
		return USBD_SHORT_XFER;

	USETW(req.wLength, sdesc->bLength);	/* the whole string */
 	err = usbd_do_request_flags(dev, &req, sdesc, USBD_SHORT_XFER_OK,
 		&actlen, USBD_DEFAULT_TIMEOUT);
	if (err)
		return err;

	if (actlen != sdesc->bLength) {
		DPRINTF("expected %d, got %d", sdesc->bLength, actlen, 0, 0);
	}

	*sizep = actlen;
	return USBD_NORMAL_COMPLETION;
}

static void
usbd_trim_spaces(char *p)
{
	char *q, *e;

	q = e = p;
	while (*q == ' ')		/* skip leading spaces */
		q++;
	while ((*p = *q++))		/* copy string */
		if (*p++ != ' ')	/* remember last non-space */
			e = p;
	*e = '\0';			/* kill trailing spaces */
}

static void
usbd_get_device_string(struct usbd_device *ud, uByte index, char **buf)
{
	char *b = kmem_alloc(USB_MAX_ENCODED_STRING_LEN, KM_SLEEP);
	if (b) {
		usbd_status err = usbd_get_string0(ud, index, b, true);
		if (err != USBD_NORMAL_COMPLETION) {
			kmem_free(b, USB_MAX_ENCODED_STRING_LEN);
			b = NULL;
		} else {
			usbd_trim_spaces(b);
		}
	}
	*buf = b;
}

void
usbd_get_device_strings(struct usbd_device *ud)
{
	usb_device_descriptor_t *udd = &ud->ud_ddesc;

	usbd_get_device_string(ud, udd->iManufacturer, &ud->ud_vendor);
	usbd_get_device_string(ud, udd->iProduct, &ud->ud_product);
	usbd_get_device_string(ud, udd->iSerialNumber, &ud->ud_serial);
}


Static void
usbd_devinfo_vp(struct usbd_device *dev, char *v, size_t vl, char *p,
    size_t pl, int usedev, int useencoded)
{
	usb_device_descriptor_t *udd = &dev->ud_ddesc;
	if (dev == NULL)
		return;

	v[0] = p[0] = '\0';

	if (usedev) {
		if (usbd_get_string0(dev, udd->iManufacturer, v, useencoded) ==
		    USBD_NORMAL_COMPLETION)
			usbd_trim_spaces(v);
		if (usbd_get_string0(dev, udd->iProduct, p, useencoded) ==
		    USBD_NORMAL_COMPLETION)
			usbd_trim_spaces(p);
	} else {
		if (dev->ud_vendor) {
			strlcpy(v, dev->ud_vendor, vl);
		}
		if (dev->ud_product) {
			strlcpy(p, dev->ud_product, pl);
		}
	}
	if (v[0] == '\0')
		usb_findvendor(v, vl, UGETW(udd->idVendor));
	if (p[0] == '\0')
		usb_findproduct(p, pl, UGETW(udd->idVendor),
		    UGETW(udd->idProduct));
}

int
usbd_printBCD(char *cp, size_t l, int bcd)
{
	return snprintf(cp, l, "%x.%02x", bcd >> 8, bcd & 0xff);
}

Static void
usbd_devinfo(struct usbd_device *dev, int showclass, char *cp, size_t l)
{
	usb_device_descriptor_t *udd = &dev->ud_ddesc;
	char *vendor, *product;
	int bcdDevice, bcdUSB;
	char *ep;

	vendor = kmem_alloc(USB_MAX_ENCODED_STRING_LEN * 2, KM_SLEEP);
	if (vendor == NULL) {
		*cp = '\0';
		return;
	}
	product = &vendor[USB_MAX_ENCODED_STRING_LEN];

	ep = cp + l;

	usbd_devinfo_vp(dev, vendor, USB_MAX_ENCODED_STRING_LEN,
	    product, USB_MAX_ENCODED_STRING_LEN, 0, 1);
	cp += snprintf(cp, ep - cp, "%s %s", vendor, product);
	if (showclass)
		cp += snprintf(cp, ep - cp, ", class %d/%d",
		    udd->bDeviceClass, udd->bDeviceSubClass);
	bcdUSB = UGETW(udd->bcdUSB);
	bcdDevice = UGETW(udd->bcdDevice);
	cp += snprintf(cp, ep - cp, ", rev ");
	cp += usbd_printBCD(cp, ep - cp, bcdUSB);
	*cp++ = '/';
	cp += usbd_printBCD(cp, ep - cp, bcdDevice);
	cp += snprintf(cp, ep - cp, ", addr %d", dev->ud_addr);
	*cp = 0;
	kmem_free(vendor, USB_MAX_ENCODED_STRING_LEN * 2);
}

char *
usbd_devinfo_alloc(struct usbd_device *dev, int showclass)
{
	char *devinfop;

	devinfop = kmem_alloc(DEVINFOSIZE, KM_SLEEP);
	usbd_devinfo(dev, showclass, devinfop, DEVINFOSIZE);
	return devinfop;
}

void
usbd_devinfo_free(char *devinfop)
{
	kmem_free(devinfop, DEVINFOSIZE);
}

/* Delay for a certain number of ms */
void
usb_delay_ms_locked(struct usbd_bus *bus, u_int ms, kmutex_t *lock)
{
	/* Wait at least two clock ticks so we know the time has passed. */
	if (bus->ub_usepolling || cold)
		delay((ms+1) * 1000);
	else
		kpause("usbdly", false, (ms*hz+999)/1000 + 1, lock);
}

void
usb_delay_ms(struct usbd_bus *bus, u_int ms)
{
	usb_delay_ms_locked(bus, ms, NULL);
}

/* Delay given a device handle. */
void
usbd_delay_ms_locked(struct usbd_device *dev, u_int ms, kmutex_t *lock)
{
	usb_delay_ms_locked(dev->ud_bus, ms, lock);
}

/* Delay given a device handle. */
void
usbd_delay_ms(struct usbd_device *dev, u_int ms)
{
	usb_delay_ms_locked(dev->ud_bus, ms, NULL);
}

usbd_status
usbd_reset_port(struct usbd_device *dev, int port, usb_port_status_t *ps)
{
	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);
	usb_device_request_t req;
	usbd_status err;
	int n;

	req.bmRequestType = UT_WRITE_CLASS_OTHER;
	req.bRequest = UR_SET_FEATURE;
	USETW(req.wValue, UHF_PORT_RESET);
	USETW(req.wIndex, port);
	USETW(req.wLength, 0);
	err = usbd_do_request(dev, &req, 0);
	DPRINTFN(1, "port %d reset done, error=%d", port, err, 0, 0);
	if (err)
		return err;
	n = 10;
	do {
		/* Wait for device to recover from reset. */
		usbd_delay_ms(dev, USB_PORT_RESET_DELAY);
		err = usbd_get_port_status(dev, port, ps);
		if (err) {
			DPRINTF("get status failed %d", err, 0, 0, 0);
			return err;
		}
		/* If the device disappeared, just give up. */
		if (!(UGETW(ps->wPortStatus) & UPS_CURRENT_CONNECT_STATUS))
			return USBD_NORMAL_COMPLETION;
	} while ((UGETW(ps->wPortChange) & UPS_C_PORT_RESET) == 0 && --n > 0);
	if (n == 0)
		return USBD_TIMEOUT;
	err = usbd_clear_port_feature(dev, port, UHF_C_PORT_RESET);
#ifdef USB_DEBUG
	if (err)
		DPRINTF("clear port feature failed %d", err, 0, 0, 0);
#endif

	/* Wait for the device to recover from reset. */
	usbd_delay_ms(dev, USB_PORT_RESET_RECOVERY);
	return err;
}

usb_interface_descriptor_t *
usbd_find_idesc(usb_config_descriptor_t *cd, int ifaceidx, int altidx)
{
	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);
	char *p = (char *)cd;
	char *end = p + UGETW(cd->wTotalLength);
	usb_interface_descriptor_t *d;
	int curidx, lastidx, curaidx = 0;

	for (curidx = lastidx = -1; p < end; ) {
		d = (usb_interface_descriptor_t *)p;
		DPRINTFN(4, "idx=%d(%d) altidx=%d(%d)", ifaceidx, curidx,
		    altidx, curaidx);
		DPRINTFN(4, "len=%d type=%d", d->bLength, d->bDescriptorType,
		    0, 0);
		if (d->bLength == 0) /* bad descriptor */
			break;
		p += d->bLength;
		if (p <= end && d->bDescriptorType == UDESC_INTERFACE) {
			if (d->bInterfaceNumber != lastidx) {
				lastidx = d->bInterfaceNumber;
				curidx++;
				curaidx = 0;
			} else
				curaidx++;
			if (ifaceidx == curidx && altidx == curaidx)
				return d;
		}
	}
	return NULL;
}

usb_endpoint_descriptor_t *
usbd_find_edesc(usb_config_descriptor_t *cd, int ifaceidx, int altidx,
		int endptidx)
{
	char *p = (char *)cd;
	char *end = p + UGETW(cd->wTotalLength);
	usb_interface_descriptor_t *d;
	usb_endpoint_descriptor_t *e;
	int curidx;

	d = usbd_find_idesc(cd, ifaceidx, altidx);
	if (d == NULL)
		return NULL;
	if (endptidx >= d->bNumEndpoints) /* quick exit */
		return NULL;

	curidx = -1;
	for (p = (char *)d + d->bLength; p < end; ) {
		e = (usb_endpoint_descriptor_t *)p;
		if (e->bLength == 0) /* bad descriptor */
			break;
		p += e->bLength;
		if (p <= end && e->bDescriptorType == UDESC_INTERFACE)
			return NULL;
		if (p <= end && e->bDescriptorType == UDESC_ENDPOINT) {
			curidx++;
			if (curidx == endptidx)
				return e;
		}
	}
	return NULL;
}

usbd_status
usbd_fill_iface_data(struct usbd_device *dev, int ifaceidx, int altidx)
{
	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);
	struct usbd_interface *ifc = &dev->ud_ifaces[ifaceidx];
	usb_interface_descriptor_t *idesc;
	char *p, *end;
	int endpt, nendpt;

	DPRINTFN(4, "ifaceidx=%d altidx=%d", ifaceidx, altidx, 0, 0);
	idesc = usbd_find_idesc(dev->ud_cdesc, ifaceidx, altidx);
	if (idesc == NULL)
		return USBD_INVAL;
	ifc->ui_dev = dev;
	ifc->ui_idesc = idesc;
	ifc->ui_index = ifaceidx;
	ifc->ui_altindex = altidx;
	nendpt = ifc->ui_idesc->bNumEndpoints;
	DPRINTFN(4, "found idesc nendpt=%d", nendpt, 0, 0, 0);
	if (nendpt != 0) {
		ifc->ui_endpoints = kmem_alloc(nendpt * sizeof(struct usbd_endpoint),
				KM_SLEEP);
		if (ifc->ui_endpoints == NULL)
			return USBD_NOMEM;
	} else
		ifc->ui_endpoints = NULL;
	ifc->ui_priv = NULL;
	p = (char *)ifc->ui_idesc + ifc->ui_idesc->bLength;
	end = (char *)dev->ud_cdesc + UGETW(dev->ud_cdesc->wTotalLength);
#define ed ((usb_endpoint_descriptor_t *)p)
	for (endpt = 0; endpt < nendpt; endpt++) {
		DPRINTFN(10, "endpt=%d", endpt, 0, 0, 0);
		for (; p < end; p += ed->bLength) {
			DPRINTFN(10, "p=%p end=%p len=%d type=%d",
			    p, end, ed->bLength, ed->bDescriptorType);
			if (p + ed->bLength <= end && ed->bLength != 0 &&
			    ed->bDescriptorType == UDESC_ENDPOINT)
				goto found;
			if (ed->bLength == 0 ||
			    ed->bDescriptorType == UDESC_INTERFACE)
				break;
		}
		/* passed end, or bad desc */
		printf("usbd_fill_iface_data: bad descriptor(s): %s\n",
		       ed->bLength == 0 ? "0 length" :
		       ed->bDescriptorType == UDESC_INTERFACE ? "iface desc":
		       "out of data");
		goto bad;
	found:
		ifc->ui_endpoints[endpt].ue_edesc = ed;
		if (dev->ud_speed == USB_SPEED_HIGH) {
			u_int mps;
			/* Control and bulk endpoints have max packet limits. */
			switch (UE_GET_XFERTYPE(ed->bmAttributes)) {
			case UE_CONTROL:
				mps = USB_2_MAX_CTRL_PACKET;
				goto check;
			case UE_BULK:
				mps = USB_2_MAX_BULK_PACKET;
			check:
				if (UGETW(ed->wMaxPacketSize) != mps) {
					USETW(ed->wMaxPacketSize, mps);
#ifdef DIAGNOSTIC
					printf("usbd_fill_iface_data: bad max "
					       "packet size\n");
#endif
				}
				break;
			default:
				break;
			}
		}
		ifc->ui_endpoints[endpt].ue_refcnt = 0;
		ifc->ui_endpoints[endpt].ue_toggle = 0;
		p += ed->bLength;
	}
#undef ed
	LIST_INIT(&ifc->ui_pipes);
	return USBD_NORMAL_COMPLETION;

 bad:
	if (ifc->ui_endpoints != NULL) {
		kmem_free(ifc->ui_endpoints, nendpt * sizeof(struct usbd_endpoint));
		ifc->ui_endpoints = NULL;
	}
	return USBD_INVAL;
}

void
usbd_free_iface_data(struct usbd_device *dev, int ifcno)
{
	struct usbd_interface *ifc = &dev->ud_ifaces[ifcno];
	if (ifc->ui_endpoints) {
		int nendpt = ifc->ui_idesc->bNumEndpoints;
		size_t sz = nendpt * sizeof(struct usbd_endpoint);
		kmem_free(ifc->ui_endpoints, sz);
	}
}

Static usbd_status
usbd_set_config(struct usbd_device *dev, int conf)
{
	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);
	usb_device_request_t req;

	DPRINTFN(5, "dev %p conf %d", dev, conf, 0, 0);

	req.bmRequestType = UT_WRITE_DEVICE;
	req.bRequest = UR_SET_CONFIG;
	USETW(req.wValue, conf);
	USETW(req.wIndex, 0);
	USETW(req.wLength, 0);
	return usbd_do_request(dev, &req, 0);
}

usbd_status
usbd_set_config_no(struct usbd_device *dev, int no, int msg)
{
	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);
	usb_config_descriptor_t cd;
	usbd_status err;
	int index;

	if (no == USB_UNCONFIG_NO)
		return usbd_set_config_index(dev, USB_UNCONFIG_INDEX, msg);

	DPRINTFN(5, "%d", no, 0, 0, 0);
	/* Figure out what config index to use. */
	for (index = 0; index < dev->ud_ddesc.bNumConfigurations; index++) {
		err = usbd_get_config_desc(dev, index, &cd);
		if (err)
			return err;
		if (cd.bConfigurationValue == no)
			return usbd_set_config_index(dev, index, msg);
	}
	return USBD_INVAL;
}

usbd_status
usbd_set_config_index(struct usbd_device *dev, int index, int msg)
{
	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);
	usb_config_descriptor_t cd, *cdp;
	usb_bos_descriptor_t *bdp = NULL;
	usbd_status err;
	int i, ifcidx, nifc, len, selfpowered, power;

	DPRINTFN(5, "dev=%p index=%d", dev, index, 0, 0);

	if (index >= dev->ud_ddesc.bNumConfigurations &&
	    index != USB_UNCONFIG_INDEX) {
		/* panic? */
		printf("usbd_set_config_index: illegal index\n");
		return USBD_INVAL;
	}

	/* XXX check that all interfaces are idle */
	if (dev->ud_config != USB_UNCONFIG_NO) {
		DPRINTF("free old config", 0, 0, 0, 0);
		/* Free all configuration data structures. */
		nifc = dev->ud_cdesc->bNumInterface;
		for (ifcidx = 0; ifcidx < nifc; ifcidx++)
			usbd_free_iface_data(dev, ifcidx);
		kmem_free(dev->ud_ifaces, nifc * sizeof(struct usbd_interface));
		kmem_free(dev->ud_cdesc, UGETW(dev->ud_cdesc->wTotalLength));
		if (dev->ud_bdesc != NULL)
			kmem_free(dev->ud_bdesc,
			    UGETW(dev->ud_bdesc->wTotalLength));
		dev->ud_ifaces = NULL;
		dev->ud_cdesc = NULL;
		dev->ud_bdesc = NULL;
		dev->ud_config = USB_UNCONFIG_NO;
	}

	if (index == USB_UNCONFIG_INDEX) {
		/* We are unconfiguring the device, so leave unallocated. */
		DPRINTF("set config 0", 0, 0, 0, 0);
		err = usbd_set_config(dev, USB_UNCONFIG_NO);
		if (err) {
			DPRINTF("setting config=0 failed, err = %d", err,
			    0, 0, 0);
		}
		return err;
	}

	/* Get the short descriptor. */
	err = usbd_get_config_desc(dev, index, &cd);
	if (err) {
		DPRINTF("get_config_desc=%d", err, 0, 0, 0);
		return err;
	}
	len = UGETW(cd.wTotalLength);
	cdp = kmem_alloc(len, KM_SLEEP);
	if (cdp == NULL)
		return USBD_NOMEM;

	/* Get the full descriptor.  Try a few times for slow devices. */
	for (i = 0; i < 3; i++) {
		err = usbd_get_desc(dev, UDESC_CONFIG, index, len, cdp);
		if (!err)
			break;
		usbd_delay_ms(dev, 200);
	}
	if (err) {
		DPRINTF("get_desc=%d", err, 0, 0, 0);
		goto bad;
	}
	if (cdp->bDescriptorType != UDESC_CONFIG) {
		DPRINTF("bad desc %d", cdp->bDescriptorType, 0, 0, 0);
		err = USBD_INVAL;
		goto bad;
	}

	if (USB_IS_SS(dev->ud_speed)) {
		usb_bos_descriptor_t bd;

		/* get short bos desc */
		err = usbd_get_bos_desc(dev, index, &bd);
		if (!err) {
			int blen = UGETW(bd.wTotalLength);
			bdp = kmem_alloc(blen, KM_SLEEP);
			if (bdp == NULL) {
				err = USBD_NOMEM;
				goto bad;
			}

			/* Get the full desc */
			for (i = 0; i < 3; i++) {
				err = usbd_get_desc(dev, UDESC_BOS, index, blen,
				    bdp);
				if (!err)
					break;
				usbd_delay_ms(dev, 200);
			}
			if (err || bdp->bDescriptorType != UDESC_BOS) {
				DPRINTF("error %d or bad desc %d", err,
				    bdp->bDescriptorType, 0, 0);
				kmem_free(bdp, blen);
				bdp = NULL;
			}
		}
	}
	dev->ud_bdesc = bdp;

	/*
	 * Figure out if the device is self or bus powered.
	 */
#if 0 /* XXX various devices don't report the power state correctly */
	selfpowered = 0;
	err = usbd_get_device_status(dev, &ds);
	if (!err && (UGETW(ds.wStatus) & UDS_SELF_POWERED))
		selfpowered = 1;
#endif
	/*
	 * Use the power state in the configuration we are going
	 * to set. This doesn't necessarily reflect the actual
	 * power state of the device; the driver can control this
	 * by choosing the appropriate configuration.
	 */
	selfpowered = !!(cdp->bmAttributes & UC_SELF_POWERED);

	DPRINTF("addr %d cno=%d attr=0x%02x, selfpowered=%d",
	    dev->ud_addr, cdp->bConfigurationValue, cdp->bmAttributes,
	    selfpowered);
	DPRINTF("max power=%d", cdp->bMaxPower * 2, 0, 0, 0);

	/* Check if we have enough power. */
#if 0 /* this is a no-op, see above */
	if ((cdp->bmAttributes & UC_SELF_POWERED) && !selfpowered) {
		if (msg)
			printf("%s: device addr %d (config %d): "
				 "can't set self powered configuration\n",
			       device_xname(dev->ud_bus->bdev), dev->ud_addr,
			       cdp->bConfigurationValue);
		err = USBD_NO_POWER;
		goto bad;
	}
#endif
#ifdef USB_DEBUG
	if (dev->ud_powersrc == NULL) {
		DPRINTF("No power source?", 0, 0, 0, 0);
		err = USBD_IOERROR;
		goto bad;
	}
#endif
	power = cdp->bMaxPower * 2;
	if (power > dev->ud_powersrc->up_power) {
		DPRINTF("power exceeded %d %d", power, dev->ud_powersrc->up_power,
		    0, 0);
		/* XXX print nicer message. */
		if (msg)
			printf("%s: device addr %d (config %d) exceeds power "
				 "budget, %d mA > %d mA\n",
			       device_xname(dev->ud_bus->ub_usbctl), dev->ud_addr,
			       cdp->bConfigurationValue,
			       power, dev->ud_powersrc->up_power);
		err = USBD_NO_POWER;
		goto bad;
	}
	dev->ud_power = power;
	dev->ud_selfpowered = selfpowered;

	/* Set the actual configuration value. */
	DPRINTF("set config %d", cdp->bConfigurationValue, 0, 0, 0);
	err = usbd_set_config(dev, cdp->bConfigurationValue);
	if (err) {
		DPRINTF("setting config=%d failed, error=%d",
		    cdp->bConfigurationValue, err, 0, 0);
		goto bad;
	}

	/* Allocate and fill interface data. */
	nifc = cdp->bNumInterface;
	dev->ud_ifaces = kmem_alloc(nifc * sizeof(struct usbd_interface),
	    KM_SLEEP);
	if (dev->ud_ifaces == NULL) {
		err = USBD_NOMEM;
		goto bad;
	}
	DPRINTFN(5, "dev=%p cdesc=%p", dev, cdp, 0, 0);
	dev->ud_cdesc = cdp;
	dev->ud_config = cdp->bConfigurationValue;
	for (ifcidx = 0; ifcidx < nifc; ifcidx++) {
		err = usbd_fill_iface_data(dev, ifcidx, 0);
		if (err) {
			while (--ifcidx >= 0)
				usbd_free_iface_data(dev, ifcidx);
			goto bad;
		}
	}

	return USBD_NORMAL_COMPLETION;

 bad:
	kmem_free(cdp, len);
	if (bdp != NULL) {
		kmem_free(bdp, UGETW(bdp->wTotalLength));
		dev->ud_bdesc = NULL;
	}
	return err;
}

/* XXX add function for alternate settings */

usbd_status
usbd_setup_pipe(struct usbd_device *dev, struct usbd_interface *iface,
		struct usbd_endpoint *ep, int ival, struct usbd_pipe **pipe)
{
	return usbd_setup_pipe_flags(dev, iface, ep, ival, pipe, 0);
}

usbd_status
usbd_setup_pipe_flags(struct usbd_device *dev, struct usbd_interface *iface,
    struct usbd_endpoint *ep, int ival, struct usbd_pipe **pipe, uint8_t flags)
{
	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);
	struct usbd_pipe *p;
	usbd_status err;

	p = kmem_alloc(dev->ud_bus->ub_pipesize, KM_SLEEP);
	DPRINTFN(1, "dev=%p addr=%d iface=%p ep=%p pipe=%p", dev, dev->ud_addr, iface, ep);
	if (p == NULL) {
		DPRINTFN(1, "(nomem)", 0, 0, 0, 0);
		return USBD_NOMEM;
	}
	p->up_dev = dev;
	p->up_iface = iface;
	p->up_endpoint = ep;
	ep->ue_refcnt++;
	p->up_intrxfer = NULL;
	p->up_running = 0;
	p->up_aborting = 0;
	p->up_serialise = true;
	p->up_repeat = 0;
	p->up_interval = ival;
	p->up_flags = flags;
	p->up_serialise = true;
	SIMPLEQ_INIT(&p->up_queue);
	err = dev->ud_bus->ub_methods->ubm_open(p);
	if (err) {
		DPRINTF("endpoint=0x%x failed, error=%d",
		    ep->ue_edesc->bEndpointAddress, err, 0, 0);
		kmem_free(p, dev->ud_bus->ub_pipesize);
		return err;
	}

	KASSERT(p->up_methods->upm_start || p->up_serialise == false);

	usb_init_task(&p->up_async_task, usbd_clear_endpoint_stall_task, p,
	    USB_TASKQ_MPSAFE);
	DPRINTFN(1, "pipe=%p", p, 0, 0, 0);
	*pipe = p;
	return USBD_NORMAL_COMPLETION;
}

/* Abort the device control pipe. */
void
usbd_kill_pipe(struct usbd_pipe *pipe)
{
	usbd_abort_pipe(pipe);
	usbd_lock_pipe(pipe);
	pipe->up_methods->upm_close(pipe);
	usbd_unlock_pipe(pipe);
	usb_rem_task(pipe->up_dev, &pipe->up_async_task);
	pipe->up_endpoint->ue_refcnt--;
	kmem_free(pipe, pipe->up_dev->ud_bus->ub_pipesize);
}

int
usbd_getnewaddr(struct usbd_bus *bus)
{
	int addr;

	for (addr = 1; addr < USB_MAX_DEVICES; addr++)
		if (bus->ub_devices[addr] == NULL)
			return addr;
	return -1;
}

usbd_status
usbd_attach_roothub(device_t parent, struct usbd_device *dev)
{
	struct usb_attach_arg uaa;
	usb_device_descriptor_t *dd = &dev->ud_ddesc;
	device_t dv;

	uaa.uaa_device = dev;
	uaa.uaa_usegeneric = 0;
	uaa.uaa_port = 0;
	uaa.uaa_vendor = UGETW(dd->idVendor);
	uaa.uaa_product = UGETW(dd->idProduct);
	uaa.uaa_release = UGETW(dd->bcdDevice);
	uaa.uaa_class = dd->bDeviceClass;
	uaa.uaa_subclass = dd->bDeviceSubClass;
	uaa.uaa_proto = dd->bDeviceProtocol;

	dv = config_found_ia(parent, "usbroothubif", &uaa, 0);
	if (dv) {
		dev->ud_subdevs = kmem_alloc(sizeof(dv), KM_SLEEP);
		if (dev->ud_subdevs == NULL)
			return USBD_NOMEM;
		dev->ud_subdevs[0] = dv;
		dev->ud_subdevlen = 1;
	}
	return USBD_NORMAL_COMPLETION;
}

static void
usbd_serialnumber(device_t dv, struct usbd_device *dev)
{
	if (dev->ud_serial) {
		prop_dictionary_set_cstring(device_properties(dv),
		    "serialnumber", dev->ud_serial);
	}
}

static usbd_status
usbd_attachwholedevice(device_t parent, struct usbd_device *dev, int port,
	int usegeneric)
{
	struct usb_attach_arg uaa;
	usb_device_descriptor_t *dd = &dev->ud_ddesc;
	device_t dv;
	int dlocs[USBDEVIFCF_NLOCS];

	uaa.uaa_device = dev;
	uaa.uaa_usegeneric = usegeneric;
	uaa.uaa_port = port;
	uaa.uaa_vendor = UGETW(dd->idVendor);
	uaa.uaa_product = UGETW(dd->idProduct);
	uaa.uaa_release = UGETW(dd->bcdDevice);
	uaa.uaa_class = dd->bDeviceClass;
	uaa.uaa_subclass = dd->bDeviceSubClass;
	uaa.uaa_proto = dd->bDeviceProtocol;

	dlocs[USBDEVIFCF_PORT] = uaa.uaa_port;
	dlocs[USBDEVIFCF_VENDOR] = uaa.uaa_vendor;
	dlocs[USBDEVIFCF_PRODUCT] = uaa.uaa_product;
	dlocs[USBDEVIFCF_RELEASE] = uaa.uaa_release;
	/* the rest is historical ballast */
	dlocs[USBDEVIFCF_CONFIGURATION] = -1;
	dlocs[USBDEVIFCF_INTERFACE] = -1;

	KERNEL_LOCK(1, NULL);
	dv = config_found_sm_loc(parent, "usbdevif", dlocs, &uaa, usbd_print,
				 config_stdsubmatch);
	KERNEL_UNLOCK_ONE(NULL);
	if (dv) {
		dev->ud_subdevs = kmem_alloc(sizeof(dv), KM_SLEEP);
		if (dev->ud_subdevs == NULL)
			return USBD_NOMEM;
		dev->ud_subdevs[0] = dv;
		dev->ud_subdevlen = 1;
		dev->ud_nifaces_claimed = 1; /* XXX */
		usbd_serialnumber(dv, dev);
	}
	return USBD_NORMAL_COMPLETION;
}

static usbd_status
usbd_attachinterfaces(device_t parent, struct usbd_device *dev,
		      int port, const int *locators)
{
	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);
	struct usbif_attach_arg uiaa;
	int ilocs[USBIFIFCF_NLOCS];
	usb_device_descriptor_t *dd = &dev->ud_ddesc;
	int nifaces;
	struct usbd_interface **ifaces;
	int i, j, loc;
	device_t dv;

	nifaces = dev->ud_cdesc->bNumInterface;
	ifaces = kmem_zalloc(nifaces * sizeof(*ifaces), KM_SLEEP);
	if (!ifaces)
		return USBD_NOMEM;
	for (i = 0; i < nifaces; i++) {
		if (!dev->ud_subdevs[i]) {
			ifaces[i] = &dev->ud_ifaces[i];
		}
		DPRINTF("interface %d %p", i, ifaces[i], 0, 0);
	}


	uiaa.uiaa_device = dev;
	uiaa.uiaa_port = port;
	uiaa.uiaa_vendor = UGETW(dd->idVendor);
	uiaa.uiaa_product = UGETW(dd->idProduct);
	uiaa.uiaa_release = UGETW(dd->bcdDevice);
	uiaa.uiaa_configno = dev->ud_cdesc->bConfigurationValue;
	uiaa.uiaa_ifaces = ifaces;
	uiaa.uiaa_nifaces = nifaces;
	ilocs[USBIFIFCF_PORT] = uiaa.uiaa_port;
	ilocs[USBIFIFCF_VENDOR] = uiaa.uiaa_vendor;
	ilocs[USBIFIFCF_PRODUCT] = uiaa.uiaa_product;
	ilocs[USBIFIFCF_RELEASE] = uiaa.uiaa_release;
	ilocs[USBIFIFCF_CONFIGURATION] = uiaa.uiaa_configno;

	for (i = 0; i < nifaces; i++) {
		if (!ifaces[i]) {
			DPRINTF("interface %d claimed", i, 0, 0, 0);
			continue; /* interface already claimed */
		}
		uiaa.uiaa_iface = ifaces[i];
		uiaa.uiaa_class = ifaces[i]->ui_idesc->bInterfaceClass;
		uiaa.uiaa_subclass = ifaces[i]->ui_idesc->bInterfaceSubClass;
		uiaa.uiaa_proto = ifaces[i]->ui_idesc->bInterfaceProtocol;
		uiaa.uiaa_ifaceno = ifaces[i]->ui_idesc->bInterfaceNumber;

		DPRINTF("searching for interface %d...", i, 0, 0, 0);
		DPRINTF("class %x subclass %x proto %x ifaceno %d",
		    uiaa.uiaa_class, uiaa.uiaa_subclass, uiaa.uiaa_proto,
		    uiaa.uiaa_ifaceno);
		ilocs[USBIFIFCF_INTERFACE] = uiaa.uiaa_ifaceno;
		if (locators != NULL) {
			loc = locators[USBIFIFCF_CONFIGURATION];
			if (loc != USBIFIFCF_CONFIGURATION_DEFAULT &&
			    loc != uiaa.uiaa_configno)
				continue;
			loc = locators[USBIFIFCF_INTERFACE];
			if (loc != USBIFIFCF_INTERFACE_DEFAULT &&
			    loc != uiaa.uiaa_ifaceno)
				continue;
		}
		KERNEL_LOCK(1, NULL);
		dv = config_found_sm_loc(parent, "usbifif", ilocs, &uiaa,
					 usbd_ifprint, config_stdsubmatch);
		KERNEL_UNLOCK_ONE(NULL);
		if (!dv)
			continue;

		usbd_serialnumber(dv, dev);

		/* claim */
		ifaces[i] = NULL;
		/* account for ifaces claimed by the driver behind our back */
		for (j = 0; j < nifaces; j++) {

			if (!ifaces[j] && !dev->ud_subdevs[j]) {
				DPRINTF("interface %d claimed behind our back",
				    j, 0, 0, 0);
				dev->ud_subdevs[j] = dv;
				dev->ud_nifaces_claimed++;
			}
		}
	}

	kmem_free(ifaces, nifaces * sizeof(*ifaces));
	return USBD_NORMAL_COMPLETION;
}

usbd_status
usbd_probe_and_attach(device_t parent, struct usbd_device *dev,
                      int port, int addr)
{
	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);
	usb_device_descriptor_t *dd = &dev->ud_ddesc;
	int confi, nifaces;
	usbd_status err;

	/* First try with device specific drivers. */
	DPRINTF("trying device specific drivers", 0, 0, 0, 0);
	err = usbd_attachwholedevice(parent, dev, port, 0);
	if (dev->ud_nifaces_claimed || err)
		return err;
	DPRINTF("no device specific driver found", 0, 0, 0, 0);

	DPRINTF("looping over %d configurations", dd->bNumConfigurations,
	    0, 0, 0);
	for (confi = 0; confi < dd->bNumConfigurations; confi++) {
		DPRINTFN(1, "trying config idx=%d", confi, 0, 0, 0);
		err = usbd_set_config_index(dev, confi, 1);
		if (err) {
			DPRINTF("port %d, set config at addr %d failed, "
			    "error=%d", port, addr, err, 0);
			printf("%s: port %d, set config at addr %d failed\n",
			    device_xname(parent), port, addr);
			return err;
		}
		nifaces = dev->ud_cdesc->bNumInterface;
		dev->ud_subdevs = kmem_zalloc(nifaces * sizeof(device_t),
		    KM_SLEEP);
		if (dev->ud_subdevs == NULL)
			return USBD_NOMEM;
		dev->ud_subdevlen = nifaces;

		err = usbd_attachinterfaces(parent, dev, port, NULL);

		if (!dev->ud_nifaces_claimed) {
			kmem_free(dev->ud_subdevs,
			    dev->ud_subdevlen * sizeof(device_t));
			dev->ud_subdevs = 0;
			dev->ud_subdevlen = 0;
		}
		if (dev->ud_nifaces_claimed || err)
			return err;
	}
	/* No interfaces were attached in any of the configurations. */

	if (dd->bNumConfigurations > 1) /* don't change if only 1 config */
		usbd_set_config_index(dev, 0, 0);

	DPRINTF("no interface drivers found", 0, 0, 0, 0);

	/* Finally try the generic driver. */
	err = usbd_attachwholedevice(parent, dev, port, 1);

	/*
	 * The generic attach failed, but leave the device as it is.
	 * We just did not find any drivers, that's all.  The device is
	 * fully operational and not harming anyone.
	 */
	DPRINTF("generic attach failed", 0, 0, 0, 0);

	return USBD_NORMAL_COMPLETION;
}

/**
 * Called from uhub_rescan().  usbd_new_device() for the target dev must be
 * called before calling this.
 */
usbd_status
usbd_reattach_device(device_t parent, struct usbd_device *dev,
                     int port, const int *locators)
{
	int i, loc;

	if (locators != NULL) {
		loc = locators[USBIFIFCF_PORT];
		if (loc != USBIFIFCF_PORT_DEFAULT && loc != port)
			return USBD_NORMAL_COMPLETION;
		loc = locators[USBIFIFCF_VENDOR];
		if (loc != USBIFIFCF_VENDOR_DEFAULT &&
		    loc != UGETW(dev->ud_ddesc.idVendor))
			return USBD_NORMAL_COMPLETION;
		loc = locators[USBIFIFCF_PRODUCT];
		if (loc != USBIFIFCF_PRODUCT_DEFAULT &&
		    loc != UGETW(dev->ud_ddesc.idProduct))
			return USBD_NORMAL_COMPLETION;
		loc = locators[USBIFIFCF_RELEASE];
		if (loc != USBIFIFCF_RELEASE_DEFAULT &&
		    loc != UGETW(dev->ud_ddesc.bcdDevice))
			return USBD_NORMAL_COMPLETION;
	}
	if (dev->ud_subdevlen == 0) {
		/* XXX: check USBIFIFCF_CONFIGURATION and
		 * USBIFIFCF_INTERFACE too */
		return usbd_probe_and_attach(parent, dev, port, dev->ud_addr);
	} else if (dev->ud_subdevlen != dev->ud_cdesc->bNumInterface) {
		/* device-specific or generic driver is already attached. */
		return USBD_NORMAL_COMPLETION;
	}
	/* Does the device have unconfigured interfaces? */
	for (i = 0; i < dev->ud_subdevlen; i++) {
		if (dev->ud_subdevs[i] == NULL) {
			break;
		}
	}
	if (i >= dev->ud_subdevlen)
		return USBD_NORMAL_COMPLETION;
	return usbd_attachinterfaces(parent, dev, port, locators);
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
	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);
	usb_device_request_t req;
	char buf[64];
	int res, actlen;

	DPRINTFN(5, "dev %p", dev, 0, 0, 0);

	req.bmRequestType = UT_READ_DEVICE;
	req.bRequest = UR_GET_DESCRIPTOR;
	USETW2(req.wValue, UDESC_DEVICE, 0);
	USETW(req.wIndex, 0);
	USETW(req.wLength, 64);
	res = usbd_do_request_flags(dev, &req, buf, USBD_SHORT_XFER_OK,
		&actlen, USBD_DEFAULT_TIMEOUT);
	if (res)
		return res;
	if (actlen < 8)
		return USBD_SHORT_XFER;
	memcpy(desc, buf, 8);
	return USBD_NORMAL_COMPLETION;
}

/*
 * Called when a new device has been put in the powered state,
 * but not yet in the addressed state.
 * Get initial descriptor, set the address, get full descriptor,
 * and attach a driver.
 */
usbd_status
usbd_new_device(device_t parent, struct usbd_bus* bus, int depth,
                int speed, int port, struct usbd_port *up)
{
	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);
	struct usbd_device *dev, *adev;
	struct usbd_device *hub;
	usb_device_descriptor_t *dd;
	usb_port_status_t ps;
	usbd_status err;
	int addr;
	int i;
	int p;

	DPRINTF("bus=%p port=%d depth=%d speed=%d", bus, port, depth, speed);

	if (bus->ub_methods->ubm_newdev != NULL)
		return (bus->ub_methods->ubm_newdev)(parent, bus, depth, speed,
		    port, up);

	addr = usbd_getnewaddr(bus);
	if (addr < 0) {
		printf("%s: No free USB addresses, new device ignored.\n",
		       device_xname(bus->ub_usbctl));
		return USBD_NO_ADDR;
	}

	dev = kmem_zalloc(sizeof(*dev), KM_SLEEP);
	if (dev == NULL)
		return USBD_NOMEM;

	dev->ud_bus = bus;

	/* Set up default endpoint handle. */
	dev->ud_ep0.ue_edesc = &dev->ud_ep0desc;

	/* Set up default endpoint descriptor. */
	dev->ud_ep0desc.bLength = USB_ENDPOINT_DESCRIPTOR_SIZE;
	dev->ud_ep0desc.bDescriptorType = UDESC_ENDPOINT;
	dev->ud_ep0desc.bEndpointAddress = USB_CONTROL_ENDPOINT;
	dev->ud_ep0desc.bmAttributes = UE_CONTROL;
	/*
	 * temporary, will be fixed after first descriptor fetch
	 * (which uses 64 bytes so it shouldn't be less),
	 * highspeed devices must support 64 byte packets anyway
	 */
	if (speed == USB_SPEED_HIGH || speed == USB_SPEED_FULL)
		USETW(dev->ud_ep0desc.wMaxPacketSize, 64);
	else
		USETW(dev->ud_ep0desc.wMaxPacketSize, USB_MAX_IPACKET);

	dev->ud_ep0desc.bInterval = 0;

	/* doesn't matter, just don't leave it uninitialized */
	dev->ud_ep0.ue_toggle = 0;

	dev->ud_quirks = &usbd_no_quirk;
	dev->ud_addr = USB_START_ADDR;
	dev->ud_ddesc.bMaxPacketSize = 0;
	dev->ud_depth = depth;
	dev->ud_powersrc = up;
	dev->ud_myhub = up->up_parent;

	up->up_dev = dev;

	/* Locate port on upstream high speed hub */
	for (adev = dev, hub = up->up_parent;
	     hub != NULL && hub->ud_speed != USB_SPEED_HIGH;
	     adev = hub, hub = hub->ud_myhub)
		;
	if (hub) {
		for (p = 0; p < hub->ud_hub->uh_hubdesc.bNbrPorts; p++) {
			if (hub->ud_hub->uh_ports[p].up_dev == adev) {
				dev->ud_myhsport = &hub->ud_hub->uh_ports[p];
				goto found;
			}
		}
		panic("usbd_new_device: cannot find HS port");
	found:
		DPRINTFN(1, "high speed port %d", p, 0, 0, 0);
	} else {
		dev->ud_myhsport = NULL;
	}
	dev->ud_speed = speed;
	dev->ud_langid = USBD_NOLANG;
	dev->ud_cookie.cookie = ++usb_cookie_no;

	/* Establish the default pipe. */
	err = usbd_setup_pipe_flags(dev, 0, &dev->ud_ep0, USBD_DEFAULT_INTERVAL,
			      &dev->ud_pipe0, USBD_MPSAFE);
	if (err) {
		usbd_remove_device(dev, up);
		return err;
	}

	dd = &dev->ud_ddesc;
	/* Try a few times in case the device is slow (i.e. outside specs.) */
	for (i = 0; i < 10; i++) {
		/* Get the first 8 bytes of the device descriptor. */
		err = usbd_get_initial_ddesc(dev, dd);
		if (!err)
			break;
		usbd_delay_ms(dev, 200);
		if ((i & 3) == 3)
			usbd_reset_port(up->up_parent, port, &ps);
	}
	if (err) {
		DPRINTF("addr=%d, getting first desc failed: %d", addr, err,
		    0, 0);
		usbd_remove_device(dev, up);
		return err;
	}

	/* Windows resets the port here, do likewise */
	if (up->up_parent)
		usbd_reset_port(up->up_parent, port, &ps);

	if (speed == USB_SPEED_HIGH) {
		/* Max packet size must be 64 (sec 5.5.3). */
		if (dd->bMaxPacketSize != USB_2_MAX_CTRL_PACKET) {
#ifdef DIAGNOSTIC
			printf("usbd_new_device: addr=%d bad max packet "
			    "size=%d. adjusting to %d.\n",
			    addr, dd->bMaxPacketSize, USB_2_MAX_CTRL_PACKET);
#endif
			dd->bMaxPacketSize = USB_2_MAX_CTRL_PACKET;
		}
	}

	DPRINTF("adding unit addr=%d, rev=%02x, class=%d, subclass=%d", addr,
	    UGETW(dd->bcdUSB), dd->bDeviceClass, dd->bDeviceSubClass);
	DPRINTF("protocol=%d, maxpacket=%d, len=%d, speed=%d",
	    dd->bDeviceProtocol, dd->bMaxPacketSize, dd->bLength, dev->ud_speed);

	if (dd->bDescriptorType != UDESC_DEVICE) {
		/* Illegal device descriptor */
		DPRINTF("illegal descriptor %d", dd->bDescriptorType, 0, 0, 0);
		usbd_remove_device(dev, up);
		return USBD_INVAL;
	}

	if (dd->bLength < USB_DEVICE_DESCRIPTOR_SIZE) {
		DPRINTF("bad length %d", dd->bLength, 0, 0, 0);
		usbd_remove_device(dev, up);
		return USBD_INVAL;
	}

	USETW(dev->ud_ep0desc.wMaxPacketSize, dd->bMaxPacketSize);

	/* Re-establish the default pipe with the new MPS. */
	usbd_kill_pipe(dev->ud_pipe0);
	err = usbd_setup_pipe_flags(dev, 0, &dev->ud_ep0, USBD_DEFAULT_INTERVAL,
	    &dev->ud_pipe0, USBD_MPSAFE);
	if (err) {
		DPRINTF("setup default pipe failed err %d", err, 0, 0, 0);
		usbd_remove_device(dev, up);
		return err;
	}

	/* Set the address */
	DPRINTFN(5, "setting device address=%d", addr, 0, 0, 0);
	err = usbd_set_address(dev, addr);
	if (err) {
		DPRINTF("set address %d failed, err = %d", addr, err, 0, 0);
		err = USBD_SET_ADDR_FAILED;
		usbd_remove_device(dev, up);
		return err;
	}

	/* Allow device time to set new address */
	usbd_delay_ms(dev, USB_SET_ADDRESS_SETTLE);
	dev->ud_addr = addr;	/* new device address now */
	bus->ub_devices[addr] = dev;

	/* Re-establish the default pipe with the new address. */
	usbd_kill_pipe(dev->ud_pipe0);
	err = usbd_setup_pipe_flags(dev, 0, &dev->ud_ep0, USBD_DEFAULT_INTERVAL,
	    &dev->ud_pipe0, USBD_MPSAFE);
	if (err) {
		DPRINTF("setup default pipe failed, err = %d", err, 0, 0, 0);
		usbd_remove_device(dev, up);
		return err;
	}

	err = usbd_reload_device_desc(dev);
	if (err) {
		DPRINTF("addr=%d, getting full desc failed, err = %d", addr,
		    err, 0, 0);
		usbd_remove_device(dev, up);
		return err;
	}

	/* Assume 100mA bus powered for now. Changed when configured. */
	dev->ud_power = USB_MIN_POWER;
	dev->ud_selfpowered = 0;

	DPRINTF("new dev (addr %d), dev=%p, parent=%p", addr, dev, parent, 0);

	usbd_get_device_strings(dev);

	usbd_add_dev_event(USB_EVENT_DEVICE_ATTACH, dev);

	if (port == 0) { /* root hub */
		KASSERT(addr == 1);
		usbd_attach_roothub(parent, dev);
		return USBD_NORMAL_COMPLETION;
	}

	err = usbd_probe_and_attach(parent, dev, port, addr);
	if (err) {
		usbd_remove_device(dev, up);
		return err;
	}

	return USBD_NORMAL_COMPLETION;
}

usbd_status
usbd_reload_device_desc(struct usbd_device *dev)
{
	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);
	usb_device_descriptor_t *udd = &dev->ud_ddesc;
	usbd_status err;

	/* Get the full device descriptor. */
	err = usbd_get_device_desc(dev, udd);
	if (err)
		return err;

	DPRINTFN(15, "bLength             %5u", udd->bLength, 0, 0, 0);
	DPRINTFN(15, "bDescriptorType     %5u", udd->bDescriptorType, 0, 0, 0);
	DPRINTFN(15, "bcdUSB              %2x.%02x", udd->bcdUSB[1],
	    udd->bcdUSB[0], 0, 0);
	DPRINTFN(15, "bDeviceClass        %5u", udd->bDeviceClass, 0, 0, 0);
	DPRINTFN(15, "bDeviceSubClass     %5u", udd->bDeviceSubClass, 0, 0, 0);
	DPRINTFN(15, "bDeviceProtocol     %5u", udd->bDeviceProtocol, 0, 0, 0);
	DPRINTFN(15, "bMaxPacketSize0     %5u", udd->bMaxPacketSize, 0, 0, 0);
	DPRINTFN(15, "idVendor           0x%04x", udd->idVendor, 0, 0, 0);
	DPRINTFN(15, "idProduct          0x%04x", udd->idProduct, 0, 0, 0);
	DPRINTFN(15, "bcdDevice           %2x.%02x", udd->bcdDevice[1],
	    udd->bcdDevice[0], 0, 0);
	DPRINTFN(15, "iManufacturer       %5u", udd->iManufacturer, 0, 0, 0);
	DPRINTFN(15, "iProduct            %5u", udd->iProduct, 0, 0, 0);
	DPRINTFN(15, "iSerial             %5u", udd->iSerialNumber, 0, 0, 0);
	DPRINTFN(15, "bNumConfigurations  %5u", udd->bNumConfigurations, 0, 0,
	    0);

	/* Figure out what's wrong with this device. */
	dev->ud_quirks = usbd_find_quirk(udd);

	return USBD_NORMAL_COMPLETION;
}

void
usbd_remove_device(struct usbd_device *dev, struct usbd_port *up)
{

	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);

	DPRINTF("dev %p up %p", dev, up, 0, 0);

	if (dev->ud_pipe0 != NULL)
		usbd_kill_pipe(dev->ud_pipe0);
	up->up_dev = NULL;
	dev->ud_bus->ub_devices[dev->ud_addr] = NULL;

	kmem_free(dev, sizeof(*dev));
}

int
usbd_print(void *aux, const char *pnp)
{
	struct usb_attach_arg *uaa = aux;

	if (pnp) {
#define USB_DEVINFO 1024
		char *devinfo;
		if (!uaa->uaa_usegeneric)
			return QUIET;
		devinfo = kmem_alloc(USB_DEVINFO, KM_SLEEP);
		usbd_devinfo(uaa->uaa_device, 1, devinfo, USB_DEVINFO);
		aprint_normal("%s, %s", devinfo, pnp);
		kmem_free(devinfo, USB_DEVINFO);
	}
	aprint_normal(" port %d", uaa->uaa_port);
#if 0
	/*
	 * It gets very crowded with these locators on the attach line.
	 * They are not really needed since they are printed in the clear
	 * by each driver.
	 */
	if (uaa->uaa_vendor != UHUB_UNK_VENDOR)
		aprint_normal(" vendor 0x%04x", uaa->uaa_vendor);
	if (uaa->uaa_product != UHUB_UNK_PRODUCT)
		aprint_normal(" product 0x%04x", uaa->uaa_product);
	if (uaa->uaa_release != UHUB_UNK_RELEASE)
		aprint_normal(" release 0x%04x", uaa->uaa_release);
#endif
	return UNCONF;
}

int
usbd_ifprint(void *aux, const char *pnp)
{
	struct usbif_attach_arg *uiaa = aux;

	if (pnp)
		return QUIET;
	aprint_normal(" port %d", uiaa->uiaa_port);
	aprint_normal(" configuration %d", uiaa->uiaa_configno);
	aprint_normal(" interface %d", uiaa->uiaa_ifaceno);
#if 0
	/*
	 * It gets very crowded with these locators on the attach line.
	 * They are not really needed since they are printed in the clear
	 * by each driver.
	 */
	if (uaa->uaa_vendor != UHUB_UNK_VENDOR)
		aprint_normal(" vendor 0x%04x", uaa->uaa_vendor);
	if (uaa->uaa_product != UHUB_UNK_PRODUCT)
		aprint_normal(" product 0x%04x", uaa->uaa_product);
	if (uaa->uaa_release != UHUB_UNK_RELEASE)
		aprint_normal(" release 0x%04x", uaa->uaa_release);
#endif
	return UNCONF;
}

void
usbd_fill_deviceinfo(struct usbd_device *dev, struct usb_device_info *di,
		     int usedev)
{
	struct usbd_port *p;
	int i, j, err;

	di->udi_bus = device_unit(dev->ud_bus->ub_usbctl);
	di->udi_addr = dev->ud_addr;
	di->udi_cookie = dev->ud_cookie;
	usbd_devinfo_vp(dev, di->udi_vendor, sizeof(di->udi_vendor),
	    di->udi_product, sizeof(di->udi_product), usedev, 1);
	usbd_printBCD(di->udi_release, sizeof(di->udi_release),
	    UGETW(dev->ud_ddesc.bcdDevice));
	if (usedev) {
		usbd_status uerr = usbd_get_string(dev,
		    dev->ud_ddesc.iSerialNumber, di->udi_serial);
		if (uerr != USBD_NORMAL_COMPLETION) {
			di->udi_serial[0] = '\0';
		} else {
			usbd_trim_spaces(di->udi_serial);
		}
	} else {
		di->udi_serial[0] = '\0';
		if (dev->ud_serial) {
			strlcpy(di->udi_serial, dev->ud_serial,
			    sizeof(di->udi_serial));
		}
	}

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
		di->udi_devnames[j][0] = 0;                 /* empty */

	if (!dev->ud_hub) {
		di->udi_nports = 0;
		return;
	}

	const int nports = dev->ud_hub->uh_hubdesc.bNbrPorts;
	for (i = 0; i < __arraycount(di->udi_ports) && i < nports; i++) {
		p = &dev->ud_hub->uh_ports[i];
		if (p->up_dev)
			err = p->up_dev->ud_addr;
		else {
			int s = UGETW(p->up_status.wPortStatus);
			const bool sshub_p = USB_IS_SS(dev->ud_speed);
			if (s & UPS_PORT_ENABLED)
				err = USB_PORT_ENABLED;
			else if (s & UPS_SUSPEND)
				err = USB_PORT_SUSPENDED;
			/*
			 * Note: UPS_PORT_POWER_SS is available only
			 * on 3.x, and UPS_PORT_POWER is available
			 * only on 2.0 or 1.1.
			 */
			else if (sshub_p && (s & UPS_PORT_POWER_SS))
				err = USB_PORT_POWERED;
			else if (!sshub_p && (s & UPS_PORT_POWER))
				err = USB_PORT_POWERED;
			else
				err = USB_PORT_DISABLED;
		}
		di->udi_ports[i] = err;
	}
	di->udi_nports = nports;
}

#ifdef COMPAT_30
void
usbd_fill_deviceinfo_old(struct usbd_device *dev, struct usb_device_info_old *di,
                         int usedev)
{
	struct usbd_port *p;
	int i, j, err;

	di->udi_bus = device_unit(dev->ud_bus->ub_usbctl);
	di->udi_addr = dev->ud_addr;
	di->udi_cookie = dev->ud_cookie;
	usbd_devinfo_vp(dev, di->udi_vendor, sizeof(di->udi_vendor),
	    di->udi_product, sizeof(di->udi_product), usedev, 0);
	usbd_printBCD(di->udi_release, sizeof(di->udi_release),
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
		return;
	}

	const int nports = dev->ud_hub->uh_hubdesc.bNbrPorts;
	for (i = 0; i < __arraycount(di->udi_ports) && i < nports;
	     i++) {
		p = &dev->ud_hub->uh_ports[i];
		if (p->up_dev)
			err = p->up_dev->ud_addr;
		else {
			int s = UGETW(p->up_status.wPortStatus);
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
}
#endif


void
usb_free_device(struct usbd_device *dev)
{
	int ifcidx, nifc;

	if (dev->ud_pipe0 != NULL)
		usbd_kill_pipe(dev->ud_pipe0);
	if (dev->ud_ifaces != NULL) {
		nifc = dev->ud_cdesc->bNumInterface;
		for (ifcidx = 0; ifcidx < nifc; ifcidx++)
			usbd_free_iface_data(dev, ifcidx);
		kmem_free(dev->ud_ifaces,
		    nifc * sizeof(struct usbd_interface));
	}
	if (dev->ud_cdesc != NULL)
		kmem_free(dev->ud_cdesc, UGETW(dev->ud_cdesc->wTotalLength));
	if (dev->ud_bdesc != NULL)
		kmem_free(dev->ud_bdesc, UGETW(dev->ud_bdesc->wTotalLength));
	if (dev->ud_subdevlen > 0) {
		kmem_free(dev->ud_subdevs,
		    dev->ud_subdevlen * sizeof(device_t));
		dev->ud_subdevlen = 0;
	}
	if (dev->ud_vendor) {
		kmem_free(dev->ud_vendor, USB_MAX_ENCODED_STRING_LEN);
	}
	if (dev->ud_product) {
		kmem_free(dev->ud_product, USB_MAX_ENCODED_STRING_LEN);
	}
	if (dev->ud_serial) {
		kmem_free(dev->ud_serial, USB_MAX_ENCODED_STRING_LEN);
	}
	kmem_free(dev, sizeof(*dev));
}

/*
 * The general mechanism for detaching drivers works as follows: Each
 * driver is responsible for maintaining a reference count on the
 * number of outstanding references to its softc (e.g.  from
 * processing hanging in a read or write).  The detach method of the
 * driver decrements this counter and flags in the softc that the
 * driver is dying and then wakes any sleepers.  It then sleeps on the
 * softc.  Each place that can sleep must maintain the reference
 * count.  When the reference count drops to -1 (0 is the normal value
 * of the reference count) the a wakeup on the softc is performed
 * signaling to the detach waiter that all references are gone.
 */

/*
 * Called from process context when we discover that a port has
 * been disconnected.
 */
int
usb_disconnect_port(struct usbd_port *up, device_t parent, int flags)
{
	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);
	struct usbd_device *dev = up->up_dev;
	device_t subdev;
	char subdevname[16];
	const char *hubname = device_xname(parent);
	int i, rc;

	DPRINTFN(3, "up=%p dev=%p port=%d", up, dev, up->up_portno, 0);

	if (dev == NULL) {
		return 0;
	}

	if (dev->ud_subdevlen > 0) {
		DPRINTFN(3, "disconnect subdevs", 0, 0, 0, 0);
		for (i = 0; i < dev->ud_subdevlen; i++) {
			if ((subdev = dev->ud_subdevs[i]) == NULL)
				continue;
			strlcpy(subdevname, device_xname(subdev),
			    sizeof(subdevname));
			if ((rc = config_detach(subdev, flags)) != 0)
				return rc;
			printf("%s: at %s", subdevname, hubname);
			if (up->up_portno != 0)
				printf(" port %d", up->up_portno);
			printf(" (addr %d) disconnected\n", dev->ud_addr);
		}
		KASSERT(!dev->ud_nifaces_claimed);
	}

	usbd_add_dev_event(USB_EVENT_DEVICE_DETACH, dev);
	dev->ud_bus->ub_devices[dev->ud_addr] = NULL;
	up->up_dev = NULL;
	usb_free_device(dev);
	return 0;
}
