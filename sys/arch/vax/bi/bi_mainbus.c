/*	$NetBSD: bi_mainbus.c,v 1.11.18.1 2017/12/03 11:36:47 jdolecek Exp $	   */
/*
 * Copyright (c) 1999 Ludd, University of Lule}, Sweden.
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
__KERNEL_RCSID(0, "$NetBSD: bi_mainbus.c,v 1.11.18.1 2017/12/03 11:36:47 jdolecek Exp $");

#define _VAX_BUS_DMA_PRIVATE

#include <sys/param.h>
#include <sys/device.h>
#include <sys/bus.h>
#include <sys/cpu.h>

#include <machine/nexus.h>
#include <machine/sid.h>
#include <machine/scb.h>
#include <machine/mainbus.h>

#include <dev/bi/bivar.h>
#include <dev/bi/bireg.h>

#include "ioconf.h"

static	int bi_mainbus_match(device_t, cfdata_t, void *);
static	void bi_mainbus_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(bi_mainbus, sizeof(struct bi_softc),
    bi_mainbus_match, bi_mainbus_attach, NULL, NULL);

static int
bi_mainbus_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args * const ma = aux;

	return !strcmp(bi_cd.cd_name, ma->ma_type);
}

static void
bi_mainbus_attach(device_t parent, device_t self, void *aux)
{
	struct bi_softc * const sc = device_private(self);
	struct mainbus_attach_args * const ma = aux;

	sc->sc_dev = self;
	/*
	 * Fill in bus specific data.
	 */
	sc->sc_addr = (bus_addr_t)BI_BASE(0, 0);
	sc->sc_iot = ma->ma_iot;	/* No special I/O handling */
	sc->sc_dmat = ma->ma_dmat;	/* No special DMA handling either */
	sc->sc_intcpu = 1 << mfpr(PR_BINID);
	sc->sc_lastiv = 256; /* Lowest available vector address */

	bi_attach(sc);
}
