/*	$NetBSD: tod.c,v 1.14.40.1 2017/12/03 11:36:45 jdolecek Exp $	*/

/*
 * Copyright (c) 1982, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from: Utah Hdr: clock.c 1.18 91/01/21$
 *	from: @(#)clock.c	8.2 (Berkeley) 1/12/94
 */

/*
 * Copyright (c) 2001 Matthew Fredette
 * Copyright (c) 1994 Gordon W. Ross
 * Copyright (c) 1993 Adam Glass
 * Copyright (c) 1988 University of Utah.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from: Utah Hdr: clock.c 1.18 91/01/21$
 *	from: @(#)clock.c	8.2 (Berkeley) 1/12/94
 */

/*
 * Machine-dependent clock routines for the NS mm58167 time-of-day chip.
 * Written by Matthew Fredette, based on the sun3 clock.c by 
 * Adam Glass and Gordon Ross.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tod.c,v 1.14.40.1 2017/12/03 11:36:45 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <m68k/asm_single.h>

#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/cpu.h>

#include <sun2/sun2/machdep.h>
#include <sun2/sun2/tod.h>

#include <dev/vme/vmereg.h>
#include <dev/vme/vmevar.h>

#include <dev/clock_subr.h>
#include <dev/ic/mm58167var.h>

static int  tod_obio_match(device_t, cfdata_t, void *args);
static void tod_obio_attach(device_t, device_t, void *);
static int  tod_vme_match(device_t, cfdata_t, void *args);
static void tod_vme_attach(device_t, device_t, void *);
static void tod_attach(struct mm58167_softc *);

CFATTACH_DECL_NEW(tod_obio, sizeof(struct mm58167_softc),
    tod_obio_match, tod_obio_attach, NULL, NULL);

CFATTACH_DECL_NEW(tod_vme, sizeof(struct mm58167_softc),
    tod_vme_match, tod_vme_attach, NULL, NULL);

static int tod_attached;

static int 
tod_obio_match(device_t parent, cfdata_t cf, void *args)
{
	struct obio_attach_args *oba = args;
	bus_space_handle_t bh;
	int matched;

	/* This driver only supports one unit. */
	if (tod_attached)
		return 0;

	/* Make sure there is something there... */
	if (bus_space_map(oba->oba_bustag, oba->oba_paddr, MM58167REG_BANK_SZ, 
	    0, &bh))
		return 0;
	matched = (bus_space_peek_1(oba->oba_bustag, bh, 0, NULL) == 0);
	bus_space_unmap(oba->oba_bustag, bh, MM58167REG_BANK_SZ);
	return matched;
}

static void 
tod_obio_attach(device_t parent, device_t self, void *args)
{
	struct obio_attach_args *oba = args;
	struct mm58167_softc *sc;

	tod_attached = 1;

	sc = device_private(self);
	sc->mm58167_dev = self;

	/* Map the device. */
	sc->mm58167_regt = oba->oba_bustag;
	if (bus_space_map(oba->oba_bustag, oba->oba_paddr, MM58167REG_BANK_SZ,
	    0, &sc->mm58167_regh))
		panic("%s: can't map", __func__);

	tod_attach(sc);
}

static int 
tod_vme_match(device_t parent, cfdata_t cf, void *aux)
{
	struct vme_attach_args	*va = aux;
	vme_chipset_tag_t	ct = va->va_vct;
	vme_am_t		mod;
	vme_addr_t		vme_addr;

	/* Make sure there is something there... */
	mod = VME_AM_A24 | VME_AM_MBO | VME_AM_SUPER | VME_AM_DATA;
	vme_addr = va->r[0].offset;

	if (vme_probe(ct, vme_addr, 1, mod, VME_D8, NULL, 0) != 0)
		return 0;

	return 1;
}

static void 
tod_vme_attach(device_t parent, device_t self, void *aux)
{
	struct mm58167_softc *sc;
	struct vme_attach_args	*va = aux;
	vme_chipset_tag_t	ct = va->va_vct;
	bus_space_tag_t		bt;
	bus_space_handle_t	bh;
	vme_am_t		mod;
	vme_mapresc_t resc;

	sc = device_private(self);
	sc->mm58167_dev = self;

	mod = VME_AM_A24 | VME_AM_MBO | VME_AM_SUPER | VME_AM_DATA;

	if (vme_space_map(ct, va->r[0].offset, MM58167REG_BANK_SZ,
	    mod, VME_D8, 0, &bt, &bh, &resc) != 0)
		panic("%s: can't map", __func__);

	sc->mm58167_regt = bt;
	sc->mm58167_regh = bh;

	tod_attach(sc);
}

static void 
tod_attach(struct mm58167_softc *sc)
{
	todr_chip_handle_t	tch;

	/* Call the IC attach code. */
	sc->mm58167_msec_xxx = MM58167REG_MSEC_XXX;
	sc->mm58167_csec = MM58167REG_CSEC;
	sc->mm58167_sec = MM58167REG_SEC;
	sc->mm58167_min = MM58167REG_MIN;
	sc->mm58167_hour = MM58167REG_HOUR;
	sc->mm58167_wday = MM58167REG_WDAY;
	sc->mm58167_day = MM58167REG_DAY;
	sc->mm58167_mon = MM58167REG_MON;
	sc->mm58167_status = MM58167REG_STATUS;
	sc->mm58167_go = MM58167REG_GO;
	if ((tch = mm58167_attach(sc)) == NULL)
		panic("%s: can't attach ic", __func__);

	todr_attach(tch);
	aprint_normal("\n");
}
