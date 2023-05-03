/*	$NetBSD: zz9k_usb.c,v 1.1 2023/05/03 13:49:30 phx Exp $ */

/*
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Alain Runa.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *	notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	notice, this list of conditions and the following disclaimer in the
 *	documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: zz9k_usb.c,v 1.1 2023/05/03 13:49:30 phx Exp $");

/* miscellaneous */
#include <sys/types.h>			/* size_t */
#include <sys/stdint.h>			/* uintXX_t */
#include <sys/stdbool.h>		/* bool */

/* driver(9) */
#include <sys/param.h>			/* NODEV */
#include <sys/device.h>			/* CFATTACH_DECL_NEW(), device_priv() */
#include <sys/errno.h>			/* . */

/* bus_space(9) and zorro bus */
#include <sys/bus.h>			/* bus_space_xxx(), bus_space_xxx_t */
#include <sys/cpu.h>			/* kvtop() */
#include <sys/systm.h>			/* aprint_xxx() */

/* zz9k related */
#include <amiga/dev/zz9kvar.h>		/* zz9kbus_attach_args */
#include <amiga/dev/zz9kreg.h>		/* ZZ9000 registers */
#include "zz9k_usb.h"			/* NZZ9K_USB */


/* The allmighty softc structure */
struct zzusb_softc {
	device_t sc_dev;
	struct bus_space_tag sc_bst;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_regh;
};

/* driver(9) essentials */
static int zzusb_match(device_t parent, cfdata_t match, void *aux);
static void zzusb_attach(device_t parent, device_t self, void *aux);
CFATTACH_DECL_NEW(zz9k_usb, sizeof(struct zzusb_softc),
    zzusb_match, zzusb_attach, NULL, NULL);


/* Go ahead, make my day. */

static int
zzusb_match(device_t parent, cfdata_t match, void *aux)
{
	struct zz9kbus_attach_args *bap = aux;

	if (strcmp(bap->zzaa_name, "zz9k_usb") != 0)
		return 0;

	return 1;
}

static void
zzusb_attach(device_t parent, device_t self, void *aux)
{
	struct zz9kbus_attach_args *bap = aux;
	struct zzusb_softc *sc = device_private(self);
	struct zzusb_softc *psc = device_private(parent);

	sc->sc_dev = self;
	sc->sc_bst.base = bap->zzaa_base;
	sc->sc_bst.absm = &amiga_bus_stride_1;
	sc->sc_iot = &sc->sc_bst;
	sc->sc_regh = psc->sc_regh;

	uint16_t capacity  = ZZREG_R(ZZ9K_USB_CAPACITY);
	aprint_normal(": USB device %sdetected.\n",
	    (capacity == 0) ? "not " : "");
	aprint_normal_dev (sc->sc_dev,
	    "MNT ZZ9000 USB driver is not implemented yet.\n");

	aprint_debug_dev(sc->sc_dev, "[DEBUG] registers at %p/%p (pa/va)\n",
	    (void *)kvtop((void *)sc->sc_regh),
	    bus_space_vaddr(sc->sc_iot, sc->sc_regh));

	/* to be implemented */
}
