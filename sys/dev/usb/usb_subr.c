/*	$NetBSD: usb_subr.c,v 1.271 2022/03/13 11:28:42 riastradh Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: usb_subr.c,v 1.271 2022/03/13 11:28:42 riastradh Exp $");

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

Static void usbd_devinfo(struct usbd_device *, int, char *, size_t);
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
	char *b;
	usbd_status err;

	b = kmem_alloc(USB_MAX_ENCODED_STRING_LEN, KM_SLEEP);
	err = usbd_get_string0(ud, index, b, true);
	if (err != USBD_NORMAL_COMPLETION) {
		kmem_free(b, USB_MAX_ENCODED_STRING_LEN);
		b = NULL;
	} else {
		usbd_trim_spaces(b);
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


void
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
	product = &vendor[USB_MAX_ENCODED_STRING_LEN];

	ep = cp + l;

	usbd_devinfo_vp(dev, vendor, USB_MAX_ENCODED_STRING_LEN,
	    product, USB_MAX_ENCODED_STRING_LEN, 0, 1);
	cp += snprintf(cp, ep - cp, "%s (0x%04x) %s (0x%04x)", vendor,
	    UGETW(udd->idVendor), product, UGETW(udd->idProduct));
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
	USBHIST_FUNC(); USBHIST_CALLARGS(usbdebug, "port %jd", port, 0, 0, 0);
	usb_device_request_t req;
	usbd_status err;
	int n;

	req.bmRequestType = UT_WRITE_CLASS_OTHER;
	req.bRequest = UR_SET_FEATURE;
	USETW(req.wValue, UHF_PORT_RESET);
	USETW(req.wIndex, port);
	USETW(req.wLength, 0);
	err = usbd_do_request(dev, &req, 0);
	DPRINTFN(1, "port %jd reset done, error=%jd", port, err, 0, 0);
	if (err)
		return err;
	n = 10;
	do {
		/* Wait for device to recover from reset. */
		usbd_delay_ms(dev, USB_PORT_RESET_DELAY);
		err = usbd_get_port_status(dev, port, ps);
		if (err) {
			DPRINTF("get status failed %jd", err, 0, 0, 0);
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
		DPRINTF("clear port feature failed %jd", err, 0, 0, 0);
#endif

	/* Wait for the device to recover from reset. */
	usbd_delay_ms(dev, USB_PORT_RESET_RECOVERY);
	return err;
}

usb_interface_descriptor_t *
usbd_find_idesc(usb_config_descriptor_t *cd, int ifaceidx, int altidx)
{
	USBHIST_FUNC();
	USBHIST_CALLARGS(usbdebug, "iface/alt idx %jd/%jd",
	    ifaceidx, altidx, 0, 0);
	char *p = (char *)cd;
	char *end = p + UGETW(cd->wTotalLength);
	usb_descriptor_t *desc;
	usb_interface_descriptor_t *idesc;
	int curidx, lastidx, curaidx = 0;

	for (curidx = lastidx = -1; p < end; ) {
		desc = (usb_descriptor_t *)p;

		DPRINTFN(4, "idx=%jd(%jd) altidx=%jd(%jd)", ifaceidx, curidx,
		    altidx, curaidx);
		DPRINTFN(4, "len=%jd type=%jd", desc->bLength,
		    desc->bDescriptorType, 0, 0);

		if (desc->bLength < USB_DESCRIPTOR_SIZE)
			break;
		p += desc->bLength;
		if (p > end)
			break;

		if (desc->bDescriptorType != UDESC_INTERFACE)
			continue;
		idesc = (usb_interface_descriptor_t *)desc;
		if (idesc->bLength < USB_INTERFACE_DESCRIPTOR_SIZE)
			break;

		if (idesc->bInterfaceNumber != lastidx) {
			lastidx = idesc->bInterfaceNumber;
			curidx++;
			curaidx = 0;
		} else {
			curaidx++;
		}
		if (ifaceidx == curidx && altidx == curaidx)
			return idesc;
	}

	return NULL;
}

usb_endpoint_descriptor_t *
usbd_find_edesc(usb_config_descriptor_t *cd, int ifaceidx, int altidx,
    int endptidx)
{
	char *p = (char *)cd;
	char *end = p + UGETW(cd->wTotalLength);
	usb_interface_descriptor_t *idesc;
	usb_endpoint_descriptor_t *edesc;
	usb_descriptor_t *desc;
	int curidx;

	idesc = usbd_find_idesc(cd, ifaceidx, altidx);
	if (idesc == NULL)
		return NULL;
	if (endptidx >= idesc->bNumEndpoints) /* quick exit */
		return NULL;

	curidx = -1;
	for (p = (char *)idesc + idesc->bLength; p < end; ) {
		desc = (usb_descriptor_t *)p;

		if (desc->bLength < USB_DESCRIPTOR_SIZE)
			break;
		p += desc->bLength;
		if (p > end)
			break;

		if (desc->bDescriptorType == UDESC_INTERFACE)
			break;
		if (desc->bDescriptorType != UDESC_ENDPOINT)
			continue;

		edesc = (usb_endpoint_descriptor_t *)desc;
		if (edesc->bLength < USB_ENDPOINT_DESCRIPTOR_SIZE)
			break;

		curidx++;
		if (curidx == endptidx)
			return edesc;
	}
	return NULL;
}

static void
usbd_iface_init(struct usbd_device *dev, int ifaceidx)
{
	struct usbd_interface *ifc = &dev->ud_ifaces[ifaceidx];

	memset(ifc, 0, sizeof(*ifc));

	ifc->ui_dev = dev;
	ifc->ui_idesc = NULL;
	ifc->ui_index = 0;
	ifc->ui_altindex = 0;
	ifc->ui_endpoints = NULL;
	ifc->ui_busy = 0;
}

static void
usbd_iface_fini(struct usbd_device *dev, int ifaceidx)
{
	struct usbd_interface *ifc __diagused = &dev->ud_ifaces[ifaceidx];

	KASSERT(ifc->ui_dev == dev);
	KASSERT(ifc->ui_idesc == NULL);
	KASSERT(ifc->ui_index == 0);
	KASSERT(ifc->ui_altindex == 0);
	KASSERT(ifc->ui_endpoints == NULL);
	KASSERTMSG(ifc->ui_busy == 0, "%"PRId64, ifc->ui_busy);
}

/*
 * usbd_iface_lock/locked/unlock, usbd_iface_piperef/pipeunref
 *
 *	We lock the interface while we are setting it, and we acquire a
 *	reference to the interface for each pipe opened on it.
 *
 *	Setting the interface while pipes are open is forbidden, and
 *	opening pipes while the interface is being set is forbidden.
 */

bool
usbd_iface_locked(struct usbd_interface *iface)
{
	bool locked;

	mutex_enter(iface->ui_dev->ud_bus->ub_lock);
	locked = (iface->ui_busy == -1);
	mutex_exit(iface->ui_dev->ud_bus->ub_lock);

	return locked;
}

static void
usbd_iface_exlock(struct usbd_interface *iface)
{

	mutex_enter(iface->ui_dev->ud_bus->ub_lock);
	KASSERTMSG(iface->ui_busy == 0, "interface is not idle,"
	    " busy=%"PRId64, iface->ui_busy);
	iface->ui_busy = -1;
	mutex_exit(iface->ui_dev->ud_bus->ub_lock);
}

usbd_status
usbd_iface_lock(struct usbd_interface *iface)
{
	usbd_status err;

	mutex_enter(iface->ui_dev->ud_bus->ub_lock);
	KASSERTMSG(iface->ui_busy != -1, "interface is locked");
	KASSERTMSG(iface->ui_busy >= 0, "%"PRId64, iface->ui_busy);
	if (iface->ui_busy) {
		err = USBD_IN_USE;
	} else {
		iface->ui_busy = -1;
		err = 0;
	}
	mutex_exit(iface->ui_dev->ud_bus->ub_lock);

	return err;
}

void
usbd_iface_unlock(struct usbd_interface *iface)
{

	mutex_enter(iface->ui_dev->ud_bus->ub_lock);
	KASSERTMSG(iface->ui_busy == -1, "interface is not locked,"
	    " busy=%"PRId64, iface->ui_busy);
	iface->ui_busy = 0;
	mutex_exit(iface->ui_dev->ud_bus->ub_lock);
}

usbd_status
usbd_iface_piperef(struct usbd_interface *iface)
{
	usbd_status err;

	mutex_enter(iface->ui_dev->ud_bus->ub_lock);
	KASSERTMSG(iface->ui_busy >= -1, "%"PRId64, iface->ui_busy);
	if (iface->ui_busy == -1) {
		err = USBD_IN_USE;
	} else {
		iface->ui_busy++;
		err = 0;
	}
	mutex_exit(iface->ui_dev->ud_bus->ub_lock);

	return err;
}

void
usbd_iface_pipeunref(struct usbd_interface *iface)
{

	mutex_enter(iface->ui_dev->ud_bus->ub_lock);
	KASSERTMSG(iface->ui_busy != -1, "interface is locked");
	KASSERTMSG(iface->ui_busy != 0, "interface not in use");
	KASSERTMSG(iface->ui_busy >= 1, "%"PRId64, iface->ui_busy);
	iface->ui_busy--;
	mutex_exit(iface->ui_dev->ud_bus->ub_lock);
}

usbd_status
usbd_fill_iface_data(struct usbd_device *dev, int ifaceidx, int altidx)
{
	USBHIST_FUNC();
	USBHIST_CALLARGS(usbdebug, "ifaceidx=%jd altidx=%jd",
	    ifaceidx, altidx, 0, 0);
	struct usbd_interface *ifc = &dev->ud_ifaces[ifaceidx];
	usb_interface_descriptor_t *idesc;
	struct usbd_endpoint *endpoints;
	char *p, *end;
	int endpt, nendpt;

	KASSERT(ifc->ui_dev == dev);
	KASSERT(usbd_iface_locked(ifc));

	idesc = usbd_find_idesc(dev->ud_cdesc, ifaceidx, altidx);
	if (idesc == NULL)
		return USBD_INVAL;

	nendpt = idesc->bNumEndpoints;
	DPRINTFN(4, "found idesc nendpt=%jd", nendpt, 0, 0, 0);
	if (nendpt != 0) {
		endpoints = kmem_alloc(nendpt * sizeof(struct usbd_endpoint),
		    KM_SLEEP);
	} else
		endpoints = NULL;

	p = (char *)idesc + idesc->bLength;
	end = (char *)dev->ud_cdesc + UGETW(dev->ud_cdesc->wTotalLength);
#define ed ((usb_endpoint_descriptor_t *)p)
	for (endpt = 0; endpt < nendpt; endpt++) {
		DPRINTFN(10, "endpt=%jd", endpt, 0, 0, 0);
		for (; p < end; p += ed->bLength) {
			DPRINTFN(10, "p=%#jx end=%#jx len=%jd type=%jd",
			    (uintptr_t)p, (uintptr_t)end, ed->bLength,
			    ed->bDescriptorType);
			if (p + ed->bLength <= end &&
			    ed->bLength >= USB_ENDPOINT_DESCRIPTOR_SIZE &&
			    ed->bDescriptorType == UDESC_ENDPOINT)
				goto found;
			if (ed->bLength == 0 ||
			    ed->bDescriptorType == UDESC_INTERFACE)
				break;
		}
		/* passed end, or bad desc */
		if (p < end) {
			if (ed->bLength == 0) {
				printf("%s: bad descriptor: 0 length\n",
				    __func__);
			} else {
				printf("%s: bad descriptor: iface desc\n",
				    __func__);
			}
		} else {
			printf("%s: no desc found\n", __func__);
		}
		goto bad;
	found:
		endpoints[endpt].ue_edesc = ed;
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
		endpoints[endpt].ue_refcnt = 0;
		endpoints[endpt].ue_toggle = 0;
		p += ed->bLength;
	}
#undef ed

	/* Success!  Free the old endpoints and commit the changes.  */
	if (ifc->ui_endpoints) {
		kmem_free(ifc->ui_endpoints, (sizeof(ifc->ui_endpoints[0]) *
			ifc->ui_idesc->bNumEndpoints));
	}

	ifc->ui_idesc = idesc;
	ifc->ui_index = ifaceidx;
	ifc->ui_altindex = altidx;
	ifc->ui_endpoints = endpoints;

	return USBD_NORMAL_COMPLETION;

 bad:
	if (endpoints)
		kmem_free(endpoints, nendpt * sizeof(struct usbd_endpoint));
	return USBD_INVAL;
}

Static void
usbd_free_iface_data(struct usbd_device *dev, int ifcno)
{
	struct usbd_interface *ifc = &dev->ud_ifaces[ifcno];

	KASSERT(ifc->ui_dev == dev);
	KASSERT(ifc->ui_idesc != NULL);
	KASSERT(usbd_iface_locked(ifc));

	if (ifc->ui_endpoints) {
		int nendpt = ifc->ui_idesc->bNumEndpoints;
		size_t sz = nendpt * sizeof(struct usbd_endpoint);
		kmem_free(ifc->ui_endpoints, sz);
		ifc->ui_endpoints = NULL;
	}

	ifc->ui_altindex = 0;
	ifc->ui_index = 0;
	ifc->ui_idesc = NULL;
}

usbd_status
usbd_set_config_no(struct usbd_device *dev, int no, int msg)
{
	USBHIST_FUNC(); USBHIST_CALLARGS(usbdebug, "%jd", no, 0, 0, 0);
	usb_config_descriptor_t cd;
	usbd_status err;
	int index;

	if (no == USB_UNCONFIG_NO)
		return usbd_set_config_index(dev, USB_UNCONFIG_INDEX, msg);

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
	USBHIST_FUNC();
	USBHIST_CALLARGS(usbdebug, "dev=%#jx index=%jd",
	    (uintptr_t)dev, index, 0, 0);
	usb_config_descriptor_t cd, *cdp;
	usb_bos_descriptor_t *bdp = NULL;
	usbd_status err;
	int i, ifcidx, nifc, len, selfpowered, power;


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
		for (ifcidx = 0; ifcidx < nifc; ifcidx++) {
			usbd_iface_exlock(&dev->ud_ifaces[ifcidx]);
			usbd_free_iface_data(dev, ifcidx);
			usbd_iface_unlock(&dev->ud_ifaces[ifcidx]);
			usbd_iface_fini(dev, ifcidx);
		}
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
			DPRINTF("setting config=0 failed, err = %jd", err,
			    0, 0, 0);
		}
		return err;
	}

	/* Get the short descriptor. */
	err = usbd_get_config_desc(dev, index, &cd);
	if (err) {
		DPRINTF("get_config_desc=%jd", err, 0, 0, 0);
		return err;
	}
	len = UGETW(cd.wTotalLength);
	if (len < USB_CONFIG_DESCRIPTOR_SIZE) {
		DPRINTF("empty short descriptor", 0, 0, 0, 0);
		return USBD_INVAL;
	}
	cdp = kmem_alloc(len, KM_SLEEP);

	/* Get the full descriptor.  Try a few times for slow devices. */
	for (i = 0; i < 3; i++) {
		err = usbd_get_desc(dev, UDESC_CONFIG, index, len, cdp);
		if (!err)
			break;
		usbd_delay_ms(dev, 200);
	}
	if (err) {
		DPRINTF("get_desc=%jd", err, 0, 0, 0);
		goto bad;
	}
	if (cdp->bDescriptorType != UDESC_CONFIG) {
		DPRINTF("bad desc %jd", cdp->bDescriptorType, 0, 0, 0);
		err = USBD_INVAL;
		goto bad;
	}
	if (UGETW(cdp->wTotalLength) != UGETW(cd.wTotalLength)) {
		DPRINTF("bad len %jd", UGETW(cdp->wTotalLength), 0, 0, 0);
		err = USBD_INVAL;
		goto bad;
	}

	if (USB_IS_SS(dev->ud_speed)) {
		usb_bos_descriptor_t bd;

		/* get short bos desc */
		err = usbd_get_bos_desc(dev, index, &bd);
		if (!err) {
			int blen = UGETW(bd.wTotalLength);
			if (blen < USB_BOS_DESCRIPTOR_SIZE) {
				DPRINTF("empty bos descriptor", 0, 0, 0, 0);
				err = USBD_INVAL;
				goto bad;
			}
			bdp = kmem_alloc(blen, KM_SLEEP);

			/* Get the full desc */
			for (i = 0; i < 3; i++) {
				err = usbd_get_desc(dev, UDESC_BOS, index, blen,
				    bdp);
				if (!err)
					break;
				usbd_delay_ms(dev, 200);
			}
			if (err || bdp->bDescriptorType != UDESC_BOS ||
			    UGETW(bdp->wTotalLength) != UGETW(bd.wTotalLength)) {
				DPRINTF("error %jd or bad desc %jd", err,
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

	DPRINTF("addr %jd cno=%jd attr=0x%02jx, selfpowered=%jd",
	    dev->ud_addr, cdp->bConfigurationValue, cdp->bmAttributes,
	    selfpowered);
	DPRINTF("max power=%jd", cdp->bMaxPower * 2, 0, 0, 0);

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
		DPRINTF("power exceeded %jd %jd", power,
		    dev->ud_powersrc->up_power, 0, 0);
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
	DPRINTF("set config %jd", cdp->bConfigurationValue, 0, 0, 0);
	err = usbd_set_config(dev, cdp->bConfigurationValue);
	if (err) {
		DPRINTF("setting config=%jd failed, error=%jd",
		    cdp->bConfigurationValue, err, 0, 0);
		goto bad;
	}

	/* Allocate and fill interface data. */
	nifc = cdp->bNumInterface;
	if (nifc == 0) {
		DPRINTF("no interfaces", 0, 0, 0, 0);
		err = USBD_INVAL;
		goto bad;
	}
	dev->ud_ifaces = kmem_alloc(nifc * sizeof(struct usbd_interface),
	    KM_SLEEP);
	DPRINTFN(5, "dev=%#jx cdesc=%#jx", (uintptr_t)dev, (uintptr_t)cdp,
	    0, 0);
	dev->ud_cdesc = cdp;
	dev->ud_config = cdp->bConfigurationValue;
	for (ifcidx = 0; ifcidx < nifc; ifcidx++) {
		usbd_iface_init(dev, ifcidx);
		usbd_iface_exlock(&dev->ud_ifaces[ifcidx]);
		err = usbd_fill_iface_data(dev, ifcidx, 0);
		usbd_iface_unlock(&dev->ud_ifaces[ifcidx]);
		if (err) {
			while (--ifcidx >= 0) {
				usbd_iface_exlock(&dev->ud_ifaces[ifcidx]);
				usbd_free_iface_data(dev, ifcidx);
				usbd_iface_unlock(&dev->ud_ifaces[ifcidx]);
				usbd_iface_fini(dev, ifcidx);
			}
			kmem_free(dev->ud_ifaces,
			    nifc * sizeof(struct usbd_interface));
			dev->ud_ifaces = NULL;
			goto bad;
		}
	}

	return USBD_NORMAL_COMPLETION;

bad:
	/* XXX Use usbd_set_config() to reset the config? */
	/* XXX Should we forbid USB_UNCONFIG_NO from bConfigurationValue? */
	dev->ud_config = USB_UNCONFIG_NO;
	kmem_free(cdp, len);
	dev->ud_cdesc = NULL;
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
	USBHIST_FUNC();
	USBHIST_CALLARGS(usbdebug, "dev=%#jx addr=%jd iface=%#jx ep=%#jx",
	    (uintptr_t)dev, dev->ud_addr, (uintptr_t)iface, (uintptr_t)ep);
	struct usbd_pipe *p = NULL;
	bool ep_acquired = false;
	usbd_status err;

	/* Block exclusive use of the endpoint by later pipes.  */
	err = usbd_endpoint_acquire(dev, ep, flags & USBD_EXCLUSIVE_USE);
	if (err)
		goto out;
	ep_acquired = true;

	p = kmem_alloc(dev->ud_bus->ub_pipesize, KM_SLEEP);
	DPRINTFN(1, "pipe=%#jx", (uintptr_t)p, 0, 0, 0);
	p->up_dev = dev;
	p->up_iface = iface;
	p->up_endpoint = ep;
	p->up_intrxfer = NULL;
	p->up_running = 0;
	p->up_aborting = 0;
	p->up_serialise = true;
	p->up_repeat = 0;
	p->up_interval = ival;
	p->up_flags = flags;
	SIMPLEQ_INIT(&p->up_queue);
	p->up_callingxfer = NULL;
	cv_init(&p->up_callingcv, "usbpipecb");
	p->up_abortlwp = NULL;

	err = dev->ud_bus->ub_methods->ubm_open(p);
	if (err) {
		DPRINTF("endpoint=%#jx failed, error=%jd",
		    (uintptr_t)ep->ue_edesc->bEndpointAddress, err, 0, 0);
		goto out;
	}

	KASSERT(p->up_methods->upm_start || p->up_serialise == false);

	usb_init_task(&p->up_async_task, usbd_clear_endpoint_stall_task, p,
	    USB_TASKQ_MPSAFE);
	DPRINTFN(1, "pipe=%#jx", (uintptr_t)p, 0, 0, 0);
	*pipe = p;
	p = NULL;		/* handed off to caller */
	ep_acquired = false;	/* handed off to pipe */
	err = USBD_NORMAL_COMPLETION;

out:	if (p) {
		KASSERT(p->up_abortlwp == NULL);
		KASSERT(p->up_callingxfer == NULL);
		cv_destroy(&p->up_callingcv);
		kmem_free(p, dev->ud_bus->ub_pipesize);
	}
	if (ep_acquired)
		usbd_endpoint_release(dev, ep);
	return err;
}

usbd_status
usbd_endpoint_acquire(struct usbd_device *dev, struct usbd_endpoint *ep,
    int flags)
{
	usbd_status err;

	mutex_enter(dev->ud_bus->ub_lock);
	if (ep->ue_refcnt == INT_MAX) {
		err = USBD_IN_USE; /* XXX rule out or switch to 64-bit */
	} else if ((flags & USBD_EXCLUSIVE_USE) && ep->ue_refcnt) {
		err = USBD_IN_USE;
	} else {
		ep->ue_refcnt++;
		err = 0;
	}
	mutex_exit(dev->ud_bus->ub_lock);

	return err;
}

void
usbd_endpoint_release(struct usbd_device *dev, struct usbd_endpoint *ep)
{

	mutex_enter(dev->ud_bus->ub_lock);
	KASSERT(ep->ue_refcnt);
	ep->ue_refcnt--;
	mutex_exit(dev->ud_bus->ub_lock);
}

/* Abort and close the device control pipe. */
void
usbd_kill_pipe(struct usbd_pipe *pipe)
{

	usbd_abort_pipe(pipe);
	usbd_close_pipe(pipe);
}

int
usbd_getnewaddr(struct usbd_bus *bus)
{
	int addr;

	for (addr = 1; addr < USB_MAX_DEVICES; addr++) {
		size_t dindex = usb_addr2dindex(addr);
		if (bus->ub_devices[dindex] == NULL)
			return addr;
	}
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

	KERNEL_LOCK(1, curlwp);
	dv = config_found(parent, &uaa, NULL,
	    CFARGS(.iattr = "usbroothubif"));
	KERNEL_UNLOCK_ONE(curlwp);
	if (dv) {
		dev->ud_subdevs = kmem_alloc(sizeof(dv), KM_SLEEP);
		dev->ud_subdevs[0] = dv;
		dev->ud_subdevlen = 1;
	}
	return USBD_NORMAL_COMPLETION;
}

static void
usbd_properties(device_t dv, struct usbd_device *dev)
{
	usb_device_descriptor_t *dd = &dev->ud_ddesc;
	prop_dictionary_t dict = device_properties(dv);
	int class, subclass, release, proto, vendor, product;

	class = dd->bDeviceClass;
	subclass = dd->bDeviceSubClass;
	release = UGETW(dd->bcdDevice);
	proto = dd->bDeviceProtocol;
	vendor = UGETW(dd->idVendor);
	product = UGETW(dd->idProduct);

	prop_dictionary_set_uint8(dict, "class", class);
	prop_dictionary_set_uint8(dict, "subclass", subclass);
	prop_dictionary_set_uint16(dict, "release", release);
	prop_dictionary_set_uint8(dict, "proto", proto);
	prop_dictionary_set_uint16(dict, "vendor-id", vendor);
	prop_dictionary_set_uint16(dict, "product-id", product);

	if (dev->ud_vendor) {
		prop_dictionary_set_string(dict,
		    "vendor-string", dev->ud_vendor);
	}
	if (dev->ud_product) {
		prop_dictionary_set_string(dict,
		    "product-string", dev->ud_product);
	}
	if (dev->ud_serial) {
		prop_dictionary_set_string(dict,
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

	KASSERT(usb_in_event_thread(parent));

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

	config_pending_incr(parent);

	KERNEL_LOCK(1, curlwp);
	dv = config_found(parent, &uaa, usbd_print,
			  CFARGS(.submatch = config_stdsubmatch,
				 .iattr = "usbdevif",
				 .locators = dlocs));
	KERNEL_UNLOCK_ONE(curlwp);
	if (dv) {
		dev->ud_subdevs = kmem_alloc(sizeof(dv), KM_SLEEP);
		dev->ud_subdevs[0] = dv;
		dev->ud_subdevlen = 1;
		dev->ud_nifaces_claimed = 1; /* XXX */
		usbd_properties(dv, dev);
	}
	config_pending_decr(parent);
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

	KASSERT(usb_in_event_thread(parent));

	nifaces = dev->ud_cdesc->bNumInterface;
	ifaces = kmem_zalloc(nifaces * sizeof(*ifaces), KM_SLEEP);
	for (i = 0; i < nifaces; i++) {
		if (!dev->ud_subdevs[i]) {
			ifaces[i] = &dev->ud_ifaces[i];
		}
		DPRINTF("interface %jd %#jx", i, (uintptr_t)ifaces[i], 0, 0);
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
			DPRINTF("interface %jd claimed", i, 0, 0, 0);
			continue; /* interface already claimed */
		}
		uiaa.uiaa_iface = ifaces[i];
		uiaa.uiaa_class = ifaces[i]->ui_idesc->bInterfaceClass;
		uiaa.uiaa_subclass = ifaces[i]->ui_idesc->bInterfaceSubClass;
		uiaa.uiaa_proto = ifaces[i]->ui_idesc->bInterfaceProtocol;
		uiaa.uiaa_ifaceno = ifaces[i]->ui_idesc->bInterfaceNumber;

		DPRINTF("searching for interface %jd...", i, 0, 0, 0);
		DPRINTF("class %jx subclass %jx proto %jx ifaceno %jd",
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
		KERNEL_LOCK(1, curlwp);
		dv = config_found(parent, &uiaa, usbd_ifprint,
				  CFARGS(.submatch = config_stdsubmatch,
					 .iattr = "usbifif",
					 .locators = ilocs));
		KERNEL_UNLOCK_ONE(curlwp);
		if (!dv)
			continue;

		usbd_properties(dv, dev);

		/* claim */
		ifaces[i] = NULL;
		/* account for ifaces claimed by the driver behind our back */
		for (j = 0; j < nifaces; j++) {

			if (!ifaces[j] && !dev->ud_subdevs[j]) {
				DPRINTF("interface %jd claimed behind our back",
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
	USBHIST_FUNC();
	USBHIST_CALLARGS(usbdebug, "trying device specific drivers", 0, 0, 0, 0);
	usb_device_descriptor_t *dd = &dev->ud_ddesc;
	int confi, nifaces;
	usbd_status err;

	KASSERT(usb_in_event_thread(parent));

	/* First try with device specific drivers. */
	err = usbd_attachwholedevice(parent, dev, port, 0);
	if (dev->ud_nifaces_claimed || err)
		return err;
	DPRINTF("no device specific driver found", 0, 0, 0, 0);

	DPRINTF("looping over %jd configurations", dd->bNumConfigurations,
	    0, 0, 0);
	for (confi = 0; confi < dd->bNumConfigurations; confi++) {
		DPRINTFN(1, "trying config idx=%jd", confi, 0, 0, 0);
		err = usbd_set_config_index(dev, confi, 1);
		if (err) {
			DPRINTF("port %jd, set config at addr %jd failed, "
			    "error=%jd", port, addr, err, 0);
			printf("%s: port %d, set config at addr %d failed\n",
			    device_xname(parent), port, addr);
			return err;
		}
		nifaces = dev->ud_cdesc->bNumInterface;
		dev->ud_subdevs = kmem_zalloc(nifaces * sizeof(device_t),
		    KM_SLEEP);
		dev->ud_subdevlen = nifaces;

		err = usbd_attachinterfaces(parent, dev, port, NULL);

		if (dev->ud_subdevs && dev->ud_nifaces_claimed == 0) {
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

	USBHIST_FUNC();
	USBHIST_CALLARGS(usbdebug, "uhub%jd port=%jd",
	    device_unit(parent), port, 0, 0);

	KASSERT(usb_in_event_thread(parent));

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
 * Called when a new device has been put in the powered state,
 * but not yet in the addressed state.
 * Get initial descriptor, set the address, get full descriptor,
 * and attach a driver.
 */
usbd_status
usbd_new_device(device_t parent, struct usbd_bus *bus, int depth, int speed,
    int port, struct usbd_port *up)
{
	USBHIST_FUNC();
	USBHIST_CALLARGS(usbdebug, "bus=%#jx port=%jd depth=%jd speed=%jd",
	    (uintptr_t)bus, port, depth, speed);
	struct usbd_device *dev, *adev;
	struct usbd_device *hub;
	usb_device_descriptor_t *dd;
	usb_port_status_t ps;
	usbd_status err;
	int addr;
	int i;
	int p;

	KASSERT(usb_in_event_thread(parent));

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
		for (p = 1; p <= hub->ud_hub->uh_hubdesc.bNbrPorts; p++) {
			if (hub->ud_hub->uh_ports[p - 1].up_dev == adev) {
				dev->ud_myhsport =
				    &hub->ud_hub->uh_ports[p - 1];
				goto found;
			}
		}
		panic("usbd_new_device: cannot find HS port");
	found:
		DPRINTFN(1, "high speed port %jd", p, 0, 0, 0);
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
		/*
		 * The root hub can never fail to give the initial descriptor,
		 * but assert it just in case.
		 */
		KASSERT(up->up_parent);
		usbd_delay_ms(dev, 200);
		if ((i & 3) == 3)
			usbd_reset_port(up->up_parent, port, &ps);
	}
	if (err) {
		DPRINTF("addr=%jd, getting first desc failed: %jd", addr, err,
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

	DPRINTF("adding unit addr=%jd, rev=%02jx, class=%jd, subclass=%jd",
	    addr, UGETW(dd->bcdUSB), dd->bDeviceClass, dd->bDeviceSubClass);
	DPRINTF("protocol=%jd, maxpacket=%jd, len=%jd, speed=%jd",
	    dd->bDeviceProtocol, dd->bMaxPacketSize, dd->bLength, dev->ud_speed);

	if (dd->bDescriptorType != UDESC_DEVICE) {
		/* Illegal device descriptor */
		DPRINTF("illegal descriptor %jd", dd->bDescriptorType, 0, 0, 0);
		usbd_remove_device(dev, up);
		return USBD_INVAL;
	}

	if (dd->bLength < USB_DEVICE_DESCRIPTOR_SIZE) {
		DPRINTF("bad length %jd", dd->bLength, 0, 0, 0);
		usbd_remove_device(dev, up);
		return USBD_INVAL;
	}

	USETW(dev->ud_ep0desc.wMaxPacketSize, dd->bMaxPacketSize);

	/* Re-establish the default pipe with the new MPS. */
	usbd_kill_pipe(dev->ud_pipe0);
	dev->ud_pipe0 = NULL;
	err = usbd_setup_pipe_flags(dev, 0, &dev->ud_ep0, USBD_DEFAULT_INTERVAL,
	    &dev->ud_pipe0, USBD_MPSAFE);
	if (err) {
		DPRINTF("setup default pipe failed err %jd", err, 0, 0, 0);
		usbd_remove_device(dev, up);
		return err;
	}

	/* Set the address */
	DPRINTFN(5, "setting device address=%jd", addr, 0, 0, 0);
	err = usbd_set_address(dev, addr);
	if (err) {
		DPRINTF("set address %jd failed, err = %jd", addr, err, 0, 0);
		err = USBD_SET_ADDR_FAILED;
		usbd_remove_device(dev, up);
		return err;
	}

	/* Allow device time to set new address */
	usbd_delay_ms(dev, USB_SET_ADDRESS_SETTLE);
	dev->ud_addr = addr;	/* new device address now */
	bus->ub_devices[usb_addr2dindex(addr)] = dev;

	/* Re-establish the default pipe with the new address. */
	usbd_kill_pipe(dev->ud_pipe0);
	dev->ud_pipe0 = NULL;
	err = usbd_setup_pipe_flags(dev, 0, &dev->ud_ep0, USBD_DEFAULT_INTERVAL,
	    &dev->ud_pipe0, USBD_MPSAFE);
	if (err) {
		DPRINTF("setup default pipe failed, err = %jd", err, 0, 0, 0);
		usbd_remove_device(dev, up);
		return err;
	}

	err = usbd_reload_device_desc(dev);
	if (err) {
		DPRINTF("addr=%jd, getting full desc failed, err = %jd", addr,
		    err, 0, 0);
		usbd_remove_device(dev, up);
		return err;
	}

	/* Assume 100mA bus powered for now. Changed when configured. */
	dev->ud_power = USB_MIN_POWER;
	dev->ud_selfpowered = 0;

	DPRINTF("new dev (addr %jd), dev=%#jx, parent=%#jx",
	    addr, (uintptr_t)dev, (uintptr_t)parent, 0);

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
	if (udd->bDescriptorType != UDESC_DEVICE)
		return USBD_INVAL;
	if (udd->bLength < USB_DEVICE_DESCRIPTOR_SIZE)
		return USBD_INVAL;

	DPRINTFN(15, "bLength             %5ju", udd->bLength, 0, 0, 0);
	DPRINTFN(15, "bDescriptorType     %5ju", udd->bDescriptorType, 0, 0, 0);
	DPRINTFN(15, "bcdUSB              %2jx.%02jx", udd->bcdUSB[1],
	    udd->bcdUSB[0], 0, 0);
	DPRINTFN(15, "bDeviceClass        %5ju", udd->bDeviceClass, 0, 0, 0);
	DPRINTFN(15, "bDeviceSubClass     %5ju", udd->bDeviceSubClass, 0, 0, 0);
	DPRINTFN(15, "bDeviceProtocol     %5ju", udd->bDeviceProtocol, 0, 0, 0);
	DPRINTFN(15, "bMaxPacketSize0     %5ju", udd->bMaxPacketSize, 0, 0, 0);
	DPRINTFN(15, "idVendor            0x%02jx 0x%02jx",
						    udd->idVendor[0],
						    udd->idVendor[1], 0, 0);
	DPRINTFN(15, "idProduct           0x%02jx 0x%02jx",
						    udd->idProduct[0],
						    udd->idProduct[1], 0, 0);
	DPRINTFN(15, "bcdDevice           %2jx.%02jx", udd->bcdDevice[1],
	    udd->bcdDevice[0], 0, 0);
	DPRINTFN(15, "iManufacturer       %5ju", udd->iManufacturer, 0, 0, 0);
	DPRINTFN(15, "iProduct            %5ju", udd->iProduct, 0, 0, 0);
	DPRINTFN(15, "iSerial             %5ju", udd->iSerialNumber, 0, 0, 0);
	DPRINTFN(15, "bNumConfigurations  %5ju", udd->bNumConfigurations, 0, 0,
	    0);

	/* Figure out what's wrong with this device. */
	dev->ud_quirks = usbd_find_quirk(udd);

	return USBD_NORMAL_COMPLETION;
}

void
usbd_remove_device(struct usbd_device *dev, struct usbd_port *up)
{

	USBHIST_FUNC();
	USBHIST_CALLARGS(usbdebug, "dev %#jx up %#jx",
	    (uintptr_t)dev, (uintptr_t)up, 0, 0);

	if (dev->ud_pipe0 != NULL)
		usbd_kill_pipe(dev->ud_pipe0);
	up->up_dev = NULL;
	dev->ud_bus->ub_devices[usb_addr2dindex(dev->ud_addr)] = NULL;

	if (dev->ud_vendor != NULL) {
		kmem_free(dev->ud_vendor, USB_MAX_ENCODED_STRING_LEN);
	}
	if (dev->ud_product != NULL) {
		kmem_free(dev->ud_product, USB_MAX_ENCODED_STRING_LEN);
	}
	if (dev->ud_serial != NULL) {
		kmem_free(dev->ud_serial, USB_MAX_ENCODED_STRING_LEN);
	}
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
	for (i = 1; i <= __arraycount(di->udi_ports) && i <= nports; i++) {
		p = &dev->ud_hub->uh_ports[i - 1];
		if (p->up_dev)
			err = p->up_dev->ud_addr;
		else {
			const int s = UGETW(p->up_status.wPortStatus);
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
		di->udi_ports[i - 1] = err;
	}
	di->udi_nports = nports;
}

void
usb_free_device(struct usbd_device *dev)
{
	int ifcidx, nifc;

	if (dev->ud_pipe0 != NULL)
		usbd_kill_pipe(dev->ud_pipe0);
	if (dev->ud_ifaces != NULL) {
		nifc = dev->ud_cdesc->bNumInterface;
		for (ifcidx = 0; ifcidx < nifc; ifcidx++) {
			usbd_iface_exlock(&dev->ud_ifaces[ifcidx]);
			usbd_free_iface_data(dev, ifcidx);
			usbd_iface_unlock(&dev->ud_ifaces[ifcidx]);
			usbd_iface_fini(dev, ifcidx);
		}
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
 * of the reference count) then a wakeup on the softc is performed
 * signaling to the detach waiter that all references are gone.
 */

/*
 * Called from process context when we discover that a port has
 * been disconnected.
 */
int
usb_disconnect_port(struct usbd_port *up, device_t parent, int flags)
{
	struct usbd_device *dev = up->up_dev;
	device_t subdev;
	char subdevname[16];
	const char *hubname = device_xname(parent);
	int i, rc;

	USBHIST_FUNC();
	USBHIST_CALLARGS(usbdebug, "up=%#jx dev=%#jx port=%jd",
	    (uintptr_t)up, (uintptr_t)dev, up->up_portno, 0);

	if (dev == NULL) {
		return 0;
	}

	usbd_suspend_pipe(dev->ud_pipe0);
	if (dev->ud_subdevlen > 0) {
		DPRINTFN(3, "disconnect subdevs", 0, 0, 0, 0);
		for (i = 0; i < dev->ud_subdevlen; i++) {
			if ((subdev = dev->ud_subdevs[i]) == NULL)
				continue;
			strlcpy(subdevname, device_xname(subdev),
			    sizeof(subdevname));
			KERNEL_LOCK(1, curlwp);
			rc = config_detach(subdev, flags);
			KERNEL_UNLOCK_ONE(curlwp);
			if (rc != 0)
				return rc;
			printf("%s: at %s", subdevname, hubname);
			if (up->up_portno != 0)
				printf(" port %d", up->up_portno);
			printf(" (addr %d) disconnected\n", dev->ud_addr);
		}
		KASSERT(!dev->ud_nifaces_claimed);
	}

	mutex_enter(dev->ud_bus->ub_lock);
	dev->ud_bus->ub_devices[usb_addr2dindex(dev->ud_addr)] = NULL;
	up->up_dev = NULL;
	mutex_exit(dev->ud_bus->ub_lock);

	usbd_add_dev_event(USB_EVENT_DEVICE_DETACH, dev);

	usb_free_device(dev);

	return 0;
}
