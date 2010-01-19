/*      $NetBSD: if_ze.c,v 1.16 2010/01/19 22:06:23 pooka Exp $ */
/*
 * Copyright (c) 1999 Ludd, University of Lule}, Sweden. All rights reserved.
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
 *      This product includes software developed at Ludd, University of 
 *      Lule}, Sweden and its contributors.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
__KERNEL_RCSID(0, "$NetBSD: if_ze.c,v 1.16 2010/01/19 22:06:23 pooka Exp $");

#include "opt_cputype.h"

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/sockio.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_dl.h>

#include <netinet/in.h>
#include <netinet/if_inarp.h>

#include <machine/bus.h>
#include <machine/nexus.h>
#include <machine/cpu.h>
#include <machine/scb.h>
#include <machine/sid.h>
#include <machine/mainbus.h>

#include <dev/ic/sgecreg.h>
#include <dev/ic/sgecvar.h>

#include "ioconf.h"

/*
 * Addresses.
 */
#define SGECADDR        0x20008000
#define NISA_ROM        0x20084000
#define NISA_ROM_VXT    0x200c4000
#define	NISA_ROM_VSBUS	0x27800000
#define	SGECVEC		0x108

static int	ze_mainbus_match(device_t, cfdata_t, void *);
static void	ze_mainbus_attach(device_t, device_t, void *);

static const struct sgec_data {
	uint32_t sd_boardtype;
	bus_addr_t sd_addr;
	bus_addr_t sd_rom;
	uint16_t sd_intvec;
	uint8_t sd_romshift;
} sgec_data[] = {
	{ VAX_BTYP_660, SGECADDR, NISA_ROM, SGECVEC, 24, },
#if VXT2000 || VAXANY
	{ VAX_BTYP_VXT, SGECADDR, NISA_ROM_VXT, 0x200, 0, },
#endif
	{ VAX_BTYP_670, SGECADDR, NISA_ROM, SGECVEC, 8, },
	{ VAX_BTYP_680, SGECADDR, NISA_ROM, SGECVEC, 8, },
	{ VAX_BTYP_681, SGECADDR, NISA_ROM, SGECVEC, 8, },
	{ VAX_BTYP_48, SGECADDR, NISA_ROM_VSBUS, SGECVEC, 0, },
	{ VAX_BTYP_49, SGECADDR, NISA_ROM_VSBUS, SGECVEC, 0, },
	{ VAX_BTYP_53, SGECADDR, NISA_ROM, SGECVEC, 8, },
};

static const struct sgec_data *
ze_find(void)
{
	size_t i;
	const struct sgec_data *sd;
	for (i = 0, sd = sgec_data; i < __arraycount(sgec_data); i++, sd++) {
		if (vax_boardtype == sd->sd_boardtype)
			return sd;
	}

	return NULL;
}

CFATTACH_DECL_NEW(ze_mainbus, sizeof(struct ze_softc),
    ze_mainbus_match, ze_mainbus_attach, NULL, NULL);

/*
 * Check for present SGEC.
 */
int
ze_mainbus_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args * const ma = aux;

	/*
	 * Should some more intelligent checking be done???
	 */
	if (strcmp(ma->ma_type, "sgec"))
		return 0;

	return ze_find() != NULL;
}

/*
 * Interface exists: make available by filling in network interface
 * record.  System will initialize the interface when it is ready
 * to accept packets.
 */
void
ze_mainbus_attach(device_t parent, device_t self, void *aux)
{
	struct mainbus_attach_args * const ma = aux;
	struct ze_softc * const sc = device_private(self);
	const struct sgec_data * const sd = ze_find();
	const uint32_t *ea;
	size_t i;
	int error;

	sc->sc_dev = self;

	/*
	 * Map in SGEC registers.
	 */
	sc->sc_dmat = ma->ma_dmat;
	sc->sc_iot = ma->ma_iot;
	sc->sc_intvec = sd->sd_intvec;
	error = bus_space_map(sc->sc_iot, sd->sd_addr, PAGE_SIZE, 0,
	    &sc->sc_ioh);
	if (error) {
		aprint_error(": failed to map %#lx: %d\n", sd->sd_addr, error);
		return;
	}

	/*
	 * Map in, read and release ethernet rom address.
	 */
	ea = (uint32_t *)vax_map_physmem(sd->sd_rom, 1);
	for (i = 0; i < ETHER_ADDR_LEN; i++)
		sc->sc_enaddr[i] = (ea[i] >> sd->sd_romshift) & 0377;
	vax_unmap_physmem((vaddr_t)ea, 1);

	scb_vecalloc(sc->sc_intvec, (void (*)(void *)) sgec_intr, sc,
	    SCB_ISTACK, &sc->sc_intrcnt);

	sgec_attach(sc);
}
