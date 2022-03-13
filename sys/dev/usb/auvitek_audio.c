/* $NetBSD: auvitek_audio.c,v 1.6 2022/03/13 12:49:36 riastradh Exp $ */

/*-
 * Copyright (c) 2010 Jared D. McNeill <jmcneill@invisible.ca>
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
 * Auvitek AU0828 USB controller - audio support
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: auvitek_audio.c,v 1.6 2022/03/13 12:49:36 riastradh Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/bus.h>
#include <sys/kmem.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbdevs.h>

#include <dev/usb/auvitekreg.h>
#include <dev/usb/auvitekvar.h>

#include "locators.h"

static int	auvitek_ifprint(void *, const char *);

int
auvitek_audio_attach(struct auvitek_softc *sc)
{
	struct usbif_attach_arg uiaa;
	struct usbd_device *udev = sc->sc_udev;
	usb_device_descriptor_t *dd = &udev->ud_ddesc;
	struct usbd_interface **ifaces;
	int ilocs[USBIFIFCF_NLOCS], nifaces, i;

	nifaces = udev->ud_cdesc->bNumInterface;
	ifaces = kmem_zalloc(nifaces * sizeof(*ifaces), KM_SLEEP);
	for (i = 0; i < nifaces; i++) {
		ifaces[i] = &udev->ud_ifaces[i];
	}

	uiaa.uiaa_device = udev;
	uiaa.uiaa_port = sc->sc_uport;
	uiaa.uiaa_vendor = UGETW(dd->idVendor);
	uiaa.uiaa_product = UGETW(dd->idProduct);
	uiaa.uiaa_release = UGETW(dd->bcdDevice);
	uiaa.uiaa_configno = udev->ud_cdesc->bConfigurationValue;
	uiaa.uiaa_ifaces = ifaces;
	uiaa.uiaa_nifaces = nifaces;
	ilocs[USBIFIFCF_PORT] = uiaa.uiaa_port;
	ilocs[USBIFIFCF_VENDOR] = uiaa.uiaa_vendor;
	ilocs[USBIFIFCF_PRODUCT] = uiaa.uiaa_product;
	ilocs[USBIFIFCF_RELEASE] = uiaa.uiaa_release;
	ilocs[USBIFIFCF_CONFIGURATION] = uiaa.uiaa_configno;

	for (i = 0; i < nifaces; i++) {
		if (!ifaces[i])
			continue;
		uiaa.uiaa_class = ifaces[i]->ui_idesc->bInterfaceClass;
		uiaa.uiaa_subclass = ifaces[i]->ui_idesc->bInterfaceSubClass;
		if (uiaa.uiaa_class != UICLASS_AUDIO)
			continue;
		if (uiaa.uiaa_subclass != UISUBCLASS_AUDIOCONTROL)
			continue;
		uiaa.uiaa_iface = ifaces[i];
		uiaa.uiaa_proto = ifaces[i]->ui_idesc->bInterfaceProtocol;
		uiaa.uiaa_ifaceno = ifaces[i]->ui_idesc->bInterfaceNumber;
		ilocs[USBIFIFCF_INTERFACE] = uiaa.uiaa_ifaceno;
		sc->sc_audiodev =
		    config_found(sc->sc_dev, &uiaa, auvitek_ifprint,
				 CFARGS(.submatch = config_stdsubmatch,
					.iattr = "usbifif",
					.locators = ilocs));
		if (sc->sc_audiodev)
			break;
	}

	kmem_free(ifaces, nifaces * sizeof(*ifaces));

	return 0;
}

int
auvitek_audio_detach(struct auvitek_softc *sc, int flags)
{

	return 0;
}

void
auvitek_audio_childdet(struct auvitek_softc *sc, device_t child)
{
	if (sc->sc_audiodev == child)
		sc->sc_audiodev = NULL;
}

static int
auvitek_ifprint(void *opaque, const char *pnp)
{
	struct usbif_attach_arg *uiaa = opaque;

	if (pnp)
		return QUIET;

	aprint_normal(" port %d", uiaa->uiaa_port);
	aprint_normal(" configuration %d", uiaa->uiaa_configno);
	aprint_normal(" interface %d", uiaa->uiaa_ifaceno);

	return UNCONF;
}
