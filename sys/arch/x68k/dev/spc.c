/*	$NetBSD: spc.c,v 1.30.24.1 2007/03/13 16:50:13 ad Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jarle Greipsland, Charles M. Hannum and Masaru Oki.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: spc.c,v 1.30.24.1 2007/03/13 16:50:13 ad Exp $");

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/cpu.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <arch/x68k/dev/intiovar.h>
#include <arch/x68k/dev/scsiromvar.h>
#include <arch/x68k/x68k/iodevice.h>

#include <dev/ic/mb89352var.h>
#include <dev/ic/mb89352reg.h>

static int spc_intio_match(struct device *, struct cfdata *, void *);
static void spc_intio_attach(struct device *, struct device *, void *);

CFATTACH_DECL(spc_intio, sizeof(struct spc_softc),
    spc_intio_match, spc_intio_attach, NULL, NULL);

static int
spc_intio_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct intio_attach_args *ia = aux;
	bus_space_tag_t iot = ia->ia_bst;
	bus_space_handle_t ioh;

	ia->ia_size=0x20;

	if (intio_map_allocate_region(device_parent(parent), ia,
				      INTIO_MAP_TESTONLY) < 0)
		return 0;

	if (bus_space_map(iot, ia->ia_addr, 0x20, BUS_SPACE_MAP_SHIFTED,
			  &ioh) < 0)
		return 0;
	if (badaddr(INTIO_ADDR(ia->ia_addr + BDID)))
		return 0;
	bus_space_unmap(iot, ioh, 0x20);

	return 1;
}

static void
spc_intio_attach(struct device *parent, struct device *self, void *aux)
{
	struct spc_softc *sc = (struct spc_softc *)self;
	struct intio_attach_args *ia = aux;
	bus_space_tag_t iot = ia->ia_bst;
	bus_space_handle_t ioh;

	printf("\n");

	intio_map_allocate_region(device_parent(parent), ia,
				  INTIO_MAP_ALLOCATE);
	if (bus_space_map(iot, ia->ia_addr, 0x20, BUS_SPACE_MAP_SHIFTED,
			  &ioh)) {
		printf("%s: can't map i/o space\n", sc->sc_dev.dv_xname);
		return;
	}

	sc->sc_iot = iot;
	sc->sc_ioh = ioh;
	sc->sc_initiator = IODEVbase->io_sram[0x70] & 0x7; /* XXX */

	if (intio_intr_establish(ia->ia_intr, "spc", spc_intr, sc))
		panic("spcattach: interrupt vector busy");

	spc_attach(sc);
}
