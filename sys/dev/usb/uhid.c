/*	$NetBSD: uhid.c,v 1.130 2025/12/07 10:05:10 jmcneill Exp $	*/

/*
 * Copyright (c) 1998, 2004, 2008, 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at
 * Carlstedt Research & Technology and Matthew R. Green (mrg@eterna23.net).
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
__KERNEL_RCSID(0, "$NetBSD: uhid.c,v 1.130 2025/12/07 10:05:10 jmcneill Exp $");

#ifdef _KERNEL_OPT
#include "opt_compat_netbsd.h"
#include "opt_hid.h"
#endif

#include <sys/param.h>
#include <sys/types.h>

#include <sys/compat_stub.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/tty.h>
#include <sys/container_of.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbhid.h>

#include <dev/usb/usbdevs.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usb_quirks.h>
#include <dev/hid/hid.h>
#include <dev/hid/uhid.h>

#include <dev/usb/uhidev.h>

#include "ioconf.h"

#ifdef UHID_DEBUG
#define DPRINTF(x)	if (uhiddebug) printf x
#define DPRINTFN(n,x)	if (uhiddebug>(n)) printf x
int	uhiddebug = 0;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif

struct uhid_usb_softc {
	struct uhid_softc sc_base;
	struct uhidev *sc_hdev;
	struct usbd_device *sc_udev;
};

static int	uhid_match(device_t, cfdata_t, void *);
static void	uhid_attach(device_t, device_t, void *);
static int	uhid_detach(device_t, int);

static int	uhid_usb_ioctl(struct uhid_softc *, u_long, void *, int,
			       struct lwp *);

CFATTACH_DECL_NEW(uhid_usb, sizeof(struct uhid_usb_softc),
    uhid_match, uhid_attach, uhid_detach, NULL);

static int
uhid_match(device_t parent, cfdata_t match, void *aux)
{
#ifdef UHID_DEBUG
	struct uhidev_attach_arg *uha = aux;
#endif

	DPRINTF(("uhid_match: report=%d\n", uha->reportid));

	if (match->cf_flags & 1)
		return UMATCH_HIGHEST;
	else
		return UMATCH_IFACECLASS_GENERIC;
}

static void
uhid_attach(device_t parent, device_t self, void *aux)
{
	struct uhid_usb_softc *usc = device_private(self);
	struct uhid_softc *sc = &usc->sc_base;
	struct uhidev_attach_arg *uha = aux;

	sc->sc_dev = self;
	sc->sc_report_id = uha->reportid;
	sc->sc_ioctl = uhid_usb_ioctl;
	sc->sc_hidev = uha->hidev;
	usc->sc_hdev = uha->parent;
	usc->sc_udev = uha->uiaa->uiaa_device;

	uhid_attach_common(sc);
}

static int
uhid_detach(device_t self, int flags)
{
	struct uhid_usb_softc *usc = device_private(self);
	struct uhid_softc *sc = &usc->sc_base;

	return uhid_detach_common(sc);
}

static int
uhid_usb_ioctl(struct uhid_softc *sc, u_long cmd, void *addr, int flag,
    struct lwp *l)
{
	struct uhid_usb_softc *usc =
	    container_of(sc, struct uhid_usb_softc, sc_base);
	int size;
	usbd_status err;

	DPRINTFN(2, ("uhidioctl: cmd=%lx\n", cmd));

	switch (cmd) {
	case USB_GET_DEVICE_DESC:
		*(usb_device_descriptor_t *)addr =
			*usbd_get_device_descriptor(usc->sc_udev);
		break;

	case USB_GET_DEVICEINFO:
		usbd_fill_deviceinfo(usc->sc_udev,
				     (struct usb_device_info *)addr, 0);
		break;
	case USB_GET_DEVICEINFO_30:
		MODULE_HOOK_CALL(usb_subr_fill_30_hook,
                    (usc->sc_udev,
		      (struct usb_device_info30 *)addr, 0,
                      usbd_devinfo_vp, usbd_printBCD),
                    enosys(), err);
		if (err == 0)
			return 0;
		break;
	case USB_GET_STRING_DESC:
	    {
		struct usb_string_desc *si = (struct usb_string_desc *)addr;
		err = usbd_get_string_desc(usc->sc_udev,
			si->usd_string_index,
			si->usd_language_id, &si->usd_desc, &size);
		if (err)
			return EINVAL;
		break;
	    }
	}

	return EINVAL;
}
