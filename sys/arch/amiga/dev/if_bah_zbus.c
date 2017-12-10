/*	$NetBSD: if_bah_zbus.c,v 1.16.10.1 2017/12/10 10:10:23 snj Exp $ */

/*-
 * Copyright (c) 1994, 1995, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ignatios Souvatzis.
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

#ifdef __m68k__
#include "opt_m68k_arch.h"
#endif

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_bah_zbus.c,v 1.16.10.1 2017/12/10 10:10:23 snj Exp $");

/*
 * Driver frontend for the Commodore Busines Machines and the
 * Ameristar ARCnet card.
 */

#include <sys/types.h>
#include <sys/param.h>

#include <sys/conf.h>
#include <sys/device.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/bus.h>

#include <machine/cpu.h>
#include <machine/intr.h>

#include <net/if.h>
#include <net/route.h>

#include <net/if_arc.h>

#include <amiga/amiga/device.h>
#include <amiga/dev/zbusvar.h>

#include <dev/ic/smc90cx6var.h>

/*
 * A2060 software status per interface
 */
struct bah_zbus_softc {
	struct	bah_softc	sc_bah;
	struct	bus_space_tag	sc_bst;
	struct	isr		sc_isr;
};

int	bah_zbus_match(device_t, cfdata_t, void *);
void	bah_zbus_attach(device_t, device_t, void *);
void	bah_zbus_reset(struct bah_softc *, int);

CFATTACH_DECL_NEW(bah_zbus, sizeof(struct bah_zbus_softc),
    bah_zbus_match, bah_zbus_attach, NULL, NULL);

int
bah_zbus_match(device_t parent, cfdata_t cf, void *aux)
{
	struct zbus_args *zap = aux;

	if ((zap->manid == 514 || zap->manid == 1053) && zap->prodid == 9)
		return (1);

	return (0);
}

void
bah_zbus_attach(device_t parent, device_t self, void *aux)
{
	struct bah_zbus_softc *bsc = device_private(self);
	struct bah_softc *sc = &bsc->sc_bah;
	struct zbus_args *zap = aux;
	int rv;

	sc->sc_dev = self;
#if (defined(BAH_DEBUG) && (BAH_DEBUG > 2))
	printf("\n%s: attach(0x%p, 0x%p, 0x%p)\n",
	    device_xname(self), parent, self, aux);
#endif
	bsc->sc_bst.base = (bus_addr_t)zap->va;
	bsc->sc_bst.absm = &amiga_bus_stride_2;

	sc->sc_bst_r = &bsc->sc_bst;
	sc->sc_regs = bsc->sc_bst.base + 0x4000;

	sc->sc_bst_m = &bsc->sc_bst;
	sc->sc_mem = bsc->sc_bst.base + 0x8000;

	sc->sc_reset = bah_zbus_reset;

	rv = bah_attach_subr(sc);
	if (rv != 0) {
		aprint_error_dev(self, "bah_attach_subr failed(%d)\n", rv);
		return;
	}

	bsc->sc_isr.isr_intr = bahintr;
	bsc->sc_isr.isr_arg = sc;
	bsc->sc_isr.isr_ipl = 2;
	add_isr(&bsc->sc_isr);
}

void
bah_zbus_reset(struct bah_softc *sc, int onoff)
{
	struct bah_zbus_softc *bsc;
	volatile u_int8_t *p;

	bsc = (struct bah_zbus_softc *)sc;

	p = (volatile u_int8_t *)bsc->sc_bst.base;

	p[0x0000] = 0;	/* A2060 reset flipflop */
	p[0xc000] = 0;	/* A560 reset flipflop */

	if (!onoff)
		return;

#ifdef M68060
	/* make sure we flush the store buffer before the delay */
	(void)p[0x8000];
#endif
	DELAY(200);

	p[0x0000] = 0xff;
	p[0xc000] = 0xff;

	return;
}
