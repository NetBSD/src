/* $NetBSD: spc.c,v 1.9.12.1 2013/02/25 00:28:47 tls Exp $ */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tohru Nishimura.
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: spc.c,v 1.9.12.1 2013/02/25 00:28:47 tls Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/autoconf.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsi_message.h>
#include <dev/scsipi/scsiconf.h>

#include <dev/ic/mb89352reg.h>
#include <dev/ic/mb89352var.h>

#include <luna68k/luna68k/isr.h>

#include "ioconf.h"

static int  spc_mainbus_match(device_t, cfdata_t, void *);
static void spc_mainbus_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(spc, sizeof(struct spc_softc),
    spc_mainbus_match, spc_mainbus_attach, NULL, NULL);

static int
spc_mainbus_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	if (strcmp(ma->ma_name, spc_cd.cd_name))
		return 0;
#if 0
	if (badaddr((void *)ma->ma_addr, 4))
		return 0;
	/* Experiments proved 2nd SPC address does NOT make a buserror. */
#endif
	return 1;
}

static void
spc_mainbus_attach(device_t parent, device_t self, void *aux)
{
	struct spc_softc *sc = device_private(self);
	struct mainbus_attach_args *ma = aux;

	sc->sc_dev = self;
	aprint_normal("\n");

	sc->sc_iot = /* XXX */ 0;
	sc->sc_ioh = ma->ma_addr;
	sc->sc_initiator = 7;

	isrlink_autovec(spc_intr, (void *)sc, ma->ma_ilvl, ISRPRI_BIO);

	spc_attach(sc);
}
