/*	$NetBSD: xmi_mainbus.c,v 1.8.18.1 2017/12/03 11:36:48 jdolecek Exp $	   */
/*
 * Copyright (c) 2000 Ludd, University of Lule}, Sweden.
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: xmi_mainbus.c,v 1.8.18.1 2017/12/03 11:36:48 jdolecek Exp $");

#define _VAX_BUS_DMA_PRIVATE

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <machine/nexus.h>
#include <machine/sid.h>
#include <machine/scb.h>
#include <machine/mainbus.h>

#include <dev/xmi/xmivar.h>

#include "ioconf.h"

static	int xmi_mainbus_match(device_t, cfdata_t, void *);
static	void xmi_mainbus_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(xmi_mainbus, sizeof(struct xmi_softc),
    xmi_mainbus_match, xmi_mainbus_attach, NULL, NULL);

extern	int mastercpu;

static int
xmi_mainbus_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args * const ma = aux;

	return !strcmp(xmi_cd.cd_name, ma->ma_type);
}

static void
xmi_mainbus_attach(device_t parent, device_t self, void *aux)
{
	struct xmi_softc * const sc = device_private(self);
	struct mainbus_attach_args * const ma = aux;

	/*
	 * Fill in bus specific data.
	 */
	sc->sc_dev = self;
	sc->sc_addr = (bus_addr_t)0x21800000; /* XXX */
	sc->sc_iot = ma->ma_iot;	/* No special I/O handling */
	sc->sc_dmat = ma->ma_dmat;	/* No special DMA handling either */
	sc->sc_intcpu = 1 << mastercpu;

	xmi_attach(sc);
}

void
xmi_intr_establish(void *icookie, int vec, void (*func)(void *), void *arg,
	struct evcnt *ev)
{
	scb_vecalloc(vec, func, arg, SCB_ISTACK, ev);
}
