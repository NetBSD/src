/*	$NetBSD: ums.c,v 1.102 2022/03/28 12:43:12 riastradh Exp $	*/

/*
 * Copyright (c) 1998, 2017 The NetBSD Foundation, Inc.
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

/*
 * HID spec: http://www.usb.org/developers/devclass_docs/HID1_11.pdf
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ums.c,v 1.102 2022/03/28 12:43:12 riastradh Exp $");

#ifdef _KERNEL_OPT
#include "opt_usb.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/select.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/poll.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbhid.h>

#include <dev/usb/usbdi.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbdevs.h>
#include <dev/usb/usb_quirks.h>
#include <dev/usb/uhidev.h>
#include <dev/hid/hid.h>
#include <dev/hid/hidms.h>

#ifdef UMS_DEBUG
#define DPRINTF(x)	if (umsdebug) printf x
#define DPRINTFN(n,x)	if (umsdebug>(n)) printf x
int	umsdebug = 0;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif

#define UMSUNIT(s)	(minor(s))

struct ums_softc {
	struct uhidev sc_hdev;
	struct usbd_device *sc_udev;
	struct hidms sc_ms;

	bool	sc_alwayson;

	int	sc_enabled;
	char	sc_dying;
};

Static void ums_intr(struct uhidev *, void *, u_int);

Static int	ums_enable(void *);
Static void	ums_disable(void *);
Static int	ums_ioctl(void *, u_long, void *, int, struct lwp *);

static const struct wsmouse_accessops ums_accessops = {
	ums_enable,
	ums_ioctl,
	ums_disable,
};

static int ums_match(device_t, cfdata_t, void *);
static void ums_attach(device_t, device_t, void *);
static void ums_childdet(device_t, device_t);
static int ums_detach(device_t, int);
static int ums_activate(device_t, enum devact);

CFATTACH_DECL2_NEW(ums, sizeof(struct ums_softc), ums_match, ums_attach,
    ums_detach, ums_activate, NULL, ums_childdet);

static int
ums_match(device_t parent, cfdata_t match, void *aux)
{
	struct uhidev_attach_arg *uha = aux;
	int size;
	void *desc;

	/*
	 * Some (older) Griffin PowerMate knobs may masquerade as a
	 * mouse, avoid treating them as such, they have only one axis.
	 */
	if (uha->uiaa->uiaa_vendor == USB_VENDOR_GRIFFIN &&
	    uha->uiaa->uiaa_product == USB_PRODUCT_GRIFFIN_POWERMATE)
		return UMATCH_NONE;

	uhidev_get_report_desc(uha->parent, &desc, &size);
	if (!hid_is_collection(desc, size, uha->reportid,
			       HID_USAGE2(HUP_GENERIC_DESKTOP, HUG_MOUSE)) &&
	    !hid_is_collection(desc, size, uha->reportid,
			       HID_USAGE2(HUP_GENERIC_DESKTOP, HUG_POINTER)) &&
	    !hid_is_collection(desc, size, uha->reportid,
			       HID_USAGE2(HUP_DIGITIZERS, 0x0002)))
		return UMATCH_NONE;

	return UMATCH_IFACECLASS;
}

static void
ums_attach(device_t parent, device_t self, void *aux)
{
	struct ums_softc *sc = device_private(self);
	struct uhidev_attach_arg *uha = aux;
	struct hid_data *d;
	struct hid_item item;
	int size, error;
	void *desc;
	uint32_t quirks;

	aprint_naive("\n");

	sc->sc_hdev.sc_dev = self;
	sc->sc_hdev.sc_intr = ums_intr;
	sc->sc_hdev.sc_parent = uha->parent;
	sc->sc_hdev.sc_report_id = uha->reportid;
	sc->sc_udev = uha->uiaa->uiaa_device;

	quirks = usbd_get_quirks(sc->sc_udev)->uq_flags;
	if (quirks & UQ_MS_REVZ)
		sc->sc_ms.flags |= HIDMS_REVZ;
	if (quirks & UQ_SPUR_BUT_UP)
		sc->sc_ms.flags |= HIDMS_SPUR_BUT_UP;
	if (quirks & UQ_ALWAYS_ON)
		sc->sc_alwayson = true;

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");

	uhidev_get_report_desc(uha->parent, &desc, &size);

	if (!hidms_setup(self, &sc->sc_ms, uha->reportid, desc, size))
		return;

	if (uha->uiaa->uiaa_vendor == USB_VENDOR_MICROSOFT) {
		int fixpos;
		int woffset = 8;
		/*
		 * The Microsoft Wireless Laser Mouse 6000 v2.0 and the
		 * Microsoft Comfort Mouse 2.0 report a bad position for
		 * the wheel and wheel tilt controls -- should be in bytes
		 * 3 & 4 of the report. Fix this if necessary.
		 */
		switch (uha->uiaa->uiaa_product) {
		case USB_PRODUCT_MICROSOFT_24GHZ_XCVR10:
		case USB_PRODUCT_MICROSOFT_24GHZ_XCVR20:
		case USB_PRODUCT_MICROSOFT_NATURAL_6000:
			fixpos = 24;
			break;
		case USB_PRODUCT_MICROSOFT_24GHZ_XCVR80:
			fixpos = 40;
			woffset = sc->sc_ms.hidms_loc_z.size;
			break;
		case USB_PRODUCT_MICROSOFT_CM6000:
			fixpos = 40;
			break;
		default:
			fixpos = 0;
			break;
		}
		if (fixpos) {
			if ((sc->sc_ms.flags & HIDMS_Z) &&
			    sc->sc_ms.hidms_loc_z.pos == 0)
				sc->sc_ms.hidms_loc_z.pos = fixpos;
			if ((sc->sc_ms.flags & HIDMS_W) &&
			    sc->sc_ms.hidms_loc_w.pos == 0)
				sc->sc_ms.hidms_loc_w.pos =
				    sc->sc_ms.hidms_loc_z.pos + woffset;
		}
	}

	tpcalib_init(&sc->sc_ms.sc_tpcalib);

	/* calibrate the pointer if it reports absolute events */
	if (sc->sc_ms.flags & HIDMS_ABS) {
		memset(&sc->sc_ms.sc_calibcoords, 0, sizeof(sc->sc_ms.sc_calibcoords));
		sc->sc_ms.sc_calibcoords.maxx = 0;
		sc->sc_ms.sc_calibcoords.maxy = 0;
		sc->sc_ms.sc_calibcoords.samplelen = WSMOUSE_CALIBCOORDS_RESET;
		d = hid_start_parse(desc, size, hid_input);
		if (d != NULL) {
			while (hid_get_item(d, &item)) {
				if (item.kind != hid_input
				    || HID_GET_USAGE_PAGE(item.usage) != HUP_GENERIC_DESKTOP
				    || item.report_ID != sc->sc_hdev.sc_report_id)
					continue;
				if (HID_GET_USAGE(item.usage) == HUG_X) {
					sc->sc_ms.sc_calibcoords.minx = item.logical_minimum;
					sc->sc_ms.sc_calibcoords.maxx = item.logical_maximum;
				}
				if (HID_GET_USAGE(item.usage) == HUG_Y) {
					sc->sc_ms.sc_calibcoords.miny = item.logical_minimum;
					sc->sc_ms.sc_calibcoords.maxy = item.logical_maximum;
				}
			}
			hid_end_parse(d);
		}
        	tpcalib_ioctl(&sc->sc_ms.sc_tpcalib, WSMOUSEIO_SCALIBCOORDS,
        	    (void *)&sc->sc_ms.sc_calibcoords, 0, 0);
	}

	hidms_attach(self, &sc->sc_ms, &ums_accessops);

	if (sc->sc_alwayson) {
		error = uhidev_open(&sc->sc_hdev);
		if (error != 0) {
			aprint_error_dev(self,
			    "WARNING: couldn't open always-on device\n");
			sc->sc_alwayson = false;
		}
	}
}

static int
ums_activate(device_t self, enum devact act)
{
	struct ums_softc *sc = device_private(self);

	switch (act) {
	case DVACT_DEACTIVATE:
		sc->sc_dying = 1;
		return 0;
	default:
		return EOPNOTSUPP;
	}
}

static void
ums_childdet(device_t self, device_t child)
{
	struct ums_softc *sc = device_private(self);

	KASSERT(sc->sc_ms.hidms_wsmousedev == child);
	sc->sc_ms.hidms_wsmousedev = NULL;
}

static int
ums_detach(device_t self, int flags)
{
	struct ums_softc *sc = device_private(self);
	int rv = 0;

	DPRINTF(("ums_detach: sc=%p flags=%d\n", sc, flags));

	if (sc->sc_alwayson)
		uhidev_close(&sc->sc_hdev);

	/* No need to do reference counting of ums, wsmouse has all the goo. */
	if (sc->sc_ms.hidms_wsmousedev != NULL)
		rv = config_detach(sc->sc_ms.hidms_wsmousedev, flags);

	pmf_device_deregister(self);

	return rv;
}

Static void
ums_intr(struct uhidev *addr, void *ibuf, u_int len)
{
	struct ums_softc *sc = (struct ums_softc *)addr;

	if (sc->sc_enabled)
		hidms_intr(&sc->sc_ms, ibuf, len);
}

Static int
ums_enable(void *v)
{
	struct ums_softc *sc = v;
	int error = 0;

	DPRINTFN(1,("ums_enable: sc=%p\n", sc));

	if (sc->sc_dying)
		return EIO;

	if (sc->sc_enabled)
		return EBUSY;

	sc->sc_enabled = 1;
	sc->sc_ms.hidms_buttons = 0;

	if (!sc->sc_alwayson) {
		error = uhidev_open(&sc->sc_hdev);
		if (error)
			sc->sc_enabled = 0;
	}

	return error;
}

Static void
ums_disable(void *v)
{
	struct ums_softc *sc = v;

	DPRINTFN(1,("ums_disable: sc=%p\n", sc));
#ifdef DIAGNOSTIC
	if (!sc->sc_enabled) {
		printf("ums_disable: not enabled\n");
		return;
	}
#endif

	if (sc->sc_enabled) {
		sc->sc_enabled = 0;
		if (!sc->sc_alwayson)
			uhidev_close(&sc->sc_hdev);
	}
}

Static int
ums_ioctl(void *v, u_long cmd, void *data, int flag,
    struct lwp *l)

{
	struct ums_softc *sc = v;
	int error;

	if (sc->sc_ms.flags & HIDMS_ABS) {
		error = tpcalib_ioctl(&sc->sc_ms.sc_tpcalib, cmd, data,
		    flag, l);
		if (error != EPASSTHROUGH)
			return error;
	}

	switch (cmd) {
	case WSMOUSEIO_GTYPE:
		if (sc->sc_ms.flags & HIDMS_ABS)
			*(u_int *)data = WSMOUSE_TYPE_TPANEL;
		else
			*(u_int *)data = WSMOUSE_TYPE_USB;
		return 0;
	}

	return EPASSTHROUGH;
}
