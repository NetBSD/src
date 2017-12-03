/*	$NetBSD: oioc.c,v 1.2.12.2 2017/12/03 11:36:41 jdolecek Exp $	*/

/*
 * Copyright (c) 2009 Stephen M. Rumble
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

/*
 * IOC1/IOC2 chips on IP4 and IP6/IP10 machines. This interfaces the SCSI
 * and Ethernet controllers, performs DMA for the former, and does address
 * space translation for the latter (maps the lance memory space to physical
 * pages).
 *
 * 'I/O Controller' is a sufficiently generic name that SGI created another
 * one for IP24, which basically stuffed a bunch of miscellany on an ASIC.
 * So, we'll call ourselves 'Old IOC' and hope that there wasn't an even older
 * one.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: oioc.c,v 1.2.12.2 2017/12/03 11:36:41 jdolecek Exp $");

#include <sys/param.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <machine/locore.h>
#include <machine/autoconf.h>
#include <sys/bus.h>
#include <machine/machtype.h>
#include <machine/sysconf.h>

#include <sgimips/ioc/oiocreg.h>
#include <sgimips/ioc/oiocvar.h>

#include "locators.h"

struct oioc_softc {
	int			sc_burst_dma;

	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
};

static int      oioc_match(device_t, cfdata_t, void *);
static void     oioc_attach(device_t, device_t, void *);
static int	oioc_print(void *, const char *);

CFATTACH_DECL_NEW(oioc, sizeof(struct oioc_softc),
    oioc_match, oioc_attach, NULL, NULL);

struct oioc_device {
	const char *od_name;
	int         od_irq;
} oioc_devices[] = {
	{ "oiocsc", 4 },
	{ "le",     5 },
	{ NULL,   0 }
};

static int
oioc_match(device_t parent, cfdata_t match, void *aux)
{

	switch(mach_type) {
	case MACH_SGI_IP4:
	case MACH_SGI_IP6 | MACH_SGI_IP10:
		return (1);
	}

	return (0);
}

static void
oioc_attach(device_t parent, device_t self, void *aux)
{
	struct oioc_softc *sc = device_private(self);
	struct mainbus_attach_args *ma = aux;
	uint32_t reg1, reg2;
	int oiocrev, i;

	sc->sc_iot = normal_memt;
	if (bus_space_map(sc->sc_iot, ma->ma_addr, OIOC_SCSI_REGS_SIZE,
	    BUS_SPACE_MAP_LINEAR, &sc->sc_ioh))
		panic("oioc_attach: could not allocate memory\n");

	if (platform.badaddr((void *)MIPS_PHYS_TO_KSEG1(ma->ma_addr +
	    OIOC2_CONFIG), 4))
		oiocrev = 1;
	else
		oiocrev = 2;

	printf("\noioc0: Old SGI IOC%d\n", oiocrev);

	if (oiocrev == 2) {
		char buf[64];

		/* Try to enable burst mode. If we can't, we can't... */
		reg1  = 12 << OIOC2_CONFIG_HIWAT_SHFT;
		reg1 |= OIOC2_CONFIG_BURST_MASK;
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, OIOC2_CONFIG, reg1);
		DELAY(1000);
		reg2 = bus_space_read_4(sc->sc_iot, sc->sc_ioh, OIOC2_CONFIG);
		if ((reg2 & (reg1 | OIOC2_CONFIG_NOSYNC_MASK)) == reg1)
			sc->sc_burst_dma = 1;

		snprintb(buf, sizeof(buf),
		    "\177\020"
		    "f\0\4HIWAT\0"
		    "f\4\2ID\0"
		    "b\6NOSYNC\0"
		    "b\7BURST\0"
		    "f\x8\7COUNT\0"
		    "f\x10\6SCP\0"
		    "f\x1c\4IOP\0\0",
		    (u_quad_t)reg2 & 0xffffffff);
		printf("oioc0: %s\n", buf);
	}

	printf("oioc0: Burst DMA %ssupported\n",
	    (sc->sc_burst_dma) ? "" : "not ");

	for (i = 0; oioc_devices[i].od_name != NULL; i++) {
		struct oioc_attach_args oa;

		oa.oa_name      = oioc_devices[i].od_name;
		oa.oa_irq       = oioc_devices[i].od_irq;
		oa.oa_burst_dma = sc->sc_burst_dma;
		oa.oa_st        = normal_memt;
		oa.oa_sh        = sc->sc_ioh;
		oa.oa_dmat      = &sgimips_default_bus_dma_tag;
		config_found_ia(self, "oioc", &oa, oioc_print);
	}
}

static int
oioc_print(void *aux, const char *pnp)
{
	struct oioc_attach_args *oa = aux;

	if (pnp)
		printf("%s at %s", oa->oa_name, pnp);

	return (UNCONF);
}
