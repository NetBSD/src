/*	$NetBSD: auxio_ebus.c,v 1.6 2015/10/06 16:40:36 martin Exp $	*/

/*
 * Copyright (c) 2000, 2001, 2015 Matthew R. Green
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

/*
 * AUXIO registers support on the sbus & ebus2, used for the floppy driver
 * and to control the system LED, for the BLINK option.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: auxio_ebus.c,v 1.6 2015/10/06 16:40:36 martin Exp $");

#include "opt_auxio.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/callout.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>

#include <dev/ebus/ebusreg.h>
#include <dev/ebus/ebusvar.h>
#include <dev/sbus/sbusvar.h>
#include <sparc64/dev/auxioreg.h>
#include <sparc64/dev/auxiovar.h>

static int	auxio_ebus_match(device_t, cfdata_t, void *);
static void	auxio_ebus_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(auxio_ebus, sizeof(struct auxio_softc),
    auxio_ebus_match, auxio_ebus_attach, NULL, NULL);

static int
auxio_ebus_match(device_t parent, cfdata_t cf, void *aux)
{
	struct ebus_attach_args *ea = aux;

	return (strcmp(AUXIO_ROM_NAME, ea->ea_name) == 0);
}

static void
auxio_ebus_attach(device_t parent, device_t self, void *aux)
{
	struct auxio_softc *sc = device_private(self);
	struct ebus_attach_args *ea = aux;

	sc->sc_dev = self;
	sc->sc_tag = ea->ea_bustag;

	if (ea->ea_nreg < 1) {
		printf(": no registers??\n");
		return;
	}

	sc->sc_flags = AUXIO_EBUS;
	if (ea->ea_nreg != 5) {
		printf(": not 5 (%d) registers, only setting led",
		    ea->ea_nreg);
		sc->sc_flags |= AUXIO_LEDONLY;
	} else if (ea->ea_nvaddr == 5) {
		sparc_promaddr_to_handle(sc->sc_tag, 
			ea->ea_vaddr[1], &sc->sc_pci);
		sparc_promaddr_to_handle(sc->sc_tag, 
			ea->ea_vaddr[2], &sc->sc_freq);
		sparc_promaddr_to_handle(sc->sc_tag, 
			ea->ea_vaddr[3], &sc->sc_scsi);
		sparc_promaddr_to_handle(sc->sc_tag, 
			ea->ea_vaddr[4], &sc->sc_temp);
	} else {
		bus_space_map(sc->sc_tag, EBUS_ADDR_FROM_REG(&ea->ea_reg[1]),
			ea->ea_reg[1].size, 0, &sc->sc_pci);
		bus_space_map(sc->sc_tag, EBUS_ADDR_FROM_REG(&ea->ea_reg[2]),
			ea->ea_reg[2].size, 0, &sc->sc_freq);
		bus_space_map(sc->sc_tag, EBUS_ADDR_FROM_REG(&ea->ea_reg[3]),
			ea->ea_reg[3].size, 0, &sc->sc_scsi);
		bus_space_map(sc->sc_tag, EBUS_ADDR_FROM_REG(&ea->ea_reg[4]),
			ea->ea_reg[4].size, 0, &sc->sc_temp);
	}

	if (ea->ea_nvaddr > 0) {
		sparc_promaddr_to_handle(sc->sc_tag, 
			ea->ea_vaddr[0], &sc->sc_led);
	} else {
		bus_space_map(sc->sc_tag, EBUS_ADDR_FROM_REG(&ea->ea_reg[0]),
			ea->ea_reg[0].size, 0, &sc->sc_led);
	}
	
	auxio_attach_common(sc);
}
