/*	$NetBSD: zz9k.c,v 1.1 2023/05/03 13:49:30 phx Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: zz9k.c,v 1.1 2023/05/03 13:49:30 phx Exp $");

/* miscellaneous */
#include <sys/types.h>			/* size_t */
#include <sys/stdint.h>			/* uintXX_t */
#include <sys/stdbool.h>		/* bool */

/* driver(9) includes */
#include <sys/param.h>			/* NODEV */
#include <sys/device.h>			/* CFATTACH_DECL_NEW(), device_priv() */
#include <sys/errno.h>			/* . */

/* bus_space(9) and zorro bus includes */
#include <sys/bus.h>			/* bus_space_xxx(), bus_space_xxx */
#include <sys/cpu.h>			/* kvtop() */
#include <sys/systm.h>			/* aprint_xxx() */
#include <amiga/dev/zbusvar.h>		/* zbus_args */

/* zz9k and amiga related */
#include <amiga/amiga/device.h>		/* amiga_realconfig */
#include <amiga/dev/zz9kvar.h>		/* zz9k_softc */
#include <amiga/dev/zz9kreg.h>		/* ZZ9000 registers */
#include "zz9k.h"			/* NZZ9K */


/* helper functions */
static int zz9k_print(void *aux, const char *pnp);

/* driver(9) essentials */
static int zz9k_match(device_t parent, cfdata_t match, void *aux);
static void zz9k_attach(device_t parent, device_t self, void *aux);
CFATTACH_DECL_NEW(
    zz9k, sizeof(struct zz9k_softc), zz9k_match, zz9k_attach, NULL, NULL);

bool zz9k_exists = false;		/* required by zz9k_fb ZZFB_CONSOLE */


/* It's the year 2020. And so it begins... */

static int
zz9k_match(device_t parent, cfdata_t match, void *aux)
{
	struct zbus_args *zap = aux;
	bool found = false;

	if (zap->manid == ZZ9K_MANID) {
		switch (zap->prodid) {
		case ZZ9K_PRODID_Z2:
		case ZZ9K_PRODID_Z3:
			found = true;
			break;
		}
	}

	if (amiga_realconfig == 0) { /* pre-config, just flag if zz9k exists. */
		zz9k_exists = found;
		return 0;    /* 0, as we don't need zz9k_attach() this round. */
	}

	return found;
}

static void
zz9k_attach(device_t parent, device_t self, void *aux)
{
	struct zbus_args *zap = aux;
	struct zz9k_softc *sc = device_private(self);
	struct zz9kbus_attach_args zz9k_fb;	/* Graphics */
	struct zz9kbus_attach_args zz9k_if;	/* Ethernet */
	struct zz9kbus_attach_args zz9k_ax;	/* Audio */
	struct zz9kbus_attach_args zz9k_usb;	/* USB */

	const bus_addr_t regBase = ZZ9K_REG_BASE;
	const bus_size_t regSize = ZZ9K_REG_SIZE;

	sc->sc_dev = self;
	sc->sc_bst.base = (bus_addr_t)zap->va;
	sc->sc_bst.absm = &amiga_bus_stride_1;
	sc->sc_iot = &sc->sc_bst;
	sc->sc_zsize = zap->size;

	if (bus_space_map(sc->sc_iot, regBase, regSize, 0, &sc->sc_regh)) {
		aprint_error("Failed to map MNT ZZ9000 registers.\n");
		return;
	}

	uint16_t hwVer = ZZREG_R(ZZ9K_HW_VERSION);
	uint16_t fwVer = ZZREG_R(ZZ9K_FW_VERSION);

	aprint_normal(": MNT ZZ9000 Zorro %s (HW: %i.%i, FW: %i.%i)\n",
	    (zap->prodid == ZZ9K_PRODID_Z3) ? "III" : "II",
	    hwVer >> 8, hwVer & 0x00ff, fwVer >> 8, fwVer & 0x00ff);

	if (fwVer < ZZ9K_FW_VER_MIN) {
		aprint_error("Firmware version is not supported. "
		    "Please update to newer firmware from:\n");
		aprint_error(
		    "https://source.mnt.re/amiga/zz9000-firmware/-/releases\n");
		return;
	}

	uint16_t tcore = ZZREG_R(ZZ9K_TEMPERATURE);
	uint16_t vcore = ZZREG_R(ZZ9K_VOLTAGE_CORE);
	uint16_t vaux  = ZZREG_R(ZZ9K_VOLTAGE_AUX);
	
	aprint_normal_dev(sc->sc_dev, "Hardware status "
	    "<Tcore: %i.%i C, Vcore: %i.%i V, Vaux: %i.%i V>\n",
	    tcore/10, tcore%10, vcore/100, vcore%100, vaux/100, vaux%100);

	aprint_debug_dev(sc->sc_dev, "[DEBUG] registers at %p/%p (pa/va), "
	    "MNT ZZ9000 is mapped with %i MB in Zorro %s space.\n",
	    (void *)kvtop((void *)sc->sc_regh),
	    bus_space_vaddr(sc->sc_iot, sc->sc_regh),
	    zap->size / (1024 * 1024),
	    (zap->prodid == ZZ9K_PRODID_Z3) ? "III" : "II");
	
	if (fwVer > ZZ9K_FW_VER) {
		aprint_normal_dev(sc->sc_dev, "The firmware is newer than "
		    "%i.%i and may not be supported yet.\n",
		    ZZ9K_FW_VER >> 8, ZZ9K_FW_VER & 0x00ff);
	}

	/* Add framebuffer */
	zz9k_fb.zzaa_base = (bus_addr_t)zap->va;
	strcpy(zz9k_fb.zzaa_name, "zz9k_fb");
	config_found(sc->sc_dev, &zz9k_fb, zz9k_print, CFARGS_NONE);

	/* Add zz* ethernet interface */
	zz9k_if.zzaa_base = (bus_addr_t)zap->va;
	strcpy(zz9k_if.zzaa_name, "zz9k_if");
	config_found(sc->sc_dev, &zz9k_if, zz9k_print, CFARGS_NONE);

	/* Add AX audio interface */
	zz9k_ax.zzaa_base = (bus_addr_t)zap->va;
	strcpy(zz9k_ax.zzaa_name, "zz9k_ax");
	config_found(sc->sc_dev, &zz9k_ax, zz9k_print, CFARGS_NONE);

	/* Add USB interface */
	zz9k_usb.zzaa_base = (bus_addr_t)zap->va;
	strcpy(zz9k_usb.zzaa_name, "zz9k_usb");
	config_found(sc->sc_dev, &zz9k_usb, zz9k_print, CFARGS_NONE);
}

static int
zz9k_print(void *aux, const char *pnp)
{
	return 0;
}
