/* $NetBSD: uhid_si.c,v 1.1.2.2 2025/12/12 18:38:56 martin Exp $ */

/*-
 * Copyright (c) 2025 Jared McNeill <jmcneill@invisible.ca>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uhid_si.c,v 1.1.2.2 2025/12/12 18:38:56 martin Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/tty.h>

#include <dev/usb/usb.h>
#include <dev/hid/uhid.h>

#include "si.h"

#define UHID_SI_VENDOR	0x057e	/* Nintendo */
#define UHID_SI_PRODUCT	0x0337	/* GameCube adapter */

static int	uhid_si_match(device_t, cfdata_t, void *);
static void	uhid_si_attach(device_t, device_t, void *);

static int	uhid_si_ioctl(struct uhid_softc *, u_long, void *, int,
			      struct lwp *);

CFATTACH_DECL_NEW(uhid_si, sizeof(struct uhid_softc),
	uhid_si_match, uhid_si_attach, NULL, NULL);

static int
uhid_si_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

static void
uhid_si_attach(device_t parent, device_t self, void *aux)
{
	struct uhid_softc * const sc = device_private(self);
	struct si_attach_args * const saa = aux;

	sc->sc_dev = self;
	sc->sc_report_id = saa->saa_index + 1;
	sc->sc_ioctl = uhid_si_ioctl;
	sc->sc_hidev = saa->saa_hidev;

	uhid_attach_common(sc);
}

static int
uhid_si_ioctl(struct uhid_softc *sc, u_long cmd, void *addr, int flag,
    struct lwp *l)
{
	struct usb_device_info *udi;

	switch (cmd) {
	case USB_GET_DEVICEINFO:
		udi = addr;
		udi->udi_vendorNo = UHID_SI_VENDOR;
		udi->udi_productNo = UHID_SI_PRODUCT;
		snprintf(udi->udi_vendor, sizeof(udi->udi_vendor), "Nintendo");
		snprintf(udi->udi_product, sizeof(udi->udi_product),
		    "GameCube Controller");
		return 0;
		break;
	}

	return EINVAL;
}
