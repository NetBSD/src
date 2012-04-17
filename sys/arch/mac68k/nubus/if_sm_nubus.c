/*	$NetBSD: if_sm_nubus.c,v 1.8.112.1 2012/04/17 00:06:37 yamt Exp $	*/

/*
 * Copyright (c) 2000 Allen Briggs.
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
 * 3. The name of the author may not be used to endorse or promote products
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_sm_nubus.c,v 1.8.112.1 2012/04/17 00:06:37 yamt Exp $");

#include "opt_inet.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <sys/systm.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <machine/bus.h>
#include <machine/viareg.h>

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>

#include <dev/ic/smc91cxxreg.h>
#include <dev/ic/smc91cxxvar.h>

#include <mac68k/nubus/nubus.h>

static int	sm_nubus_match(device_t, cfdata_t, void *);
static void	sm_nubus_attach(device_t, device_t, void *);

CFATTACH_DECL(sm_nubus, sizeof(struct smc91cxx_softc),
    sm_nubus_match, sm_nubus_attach, NULL, NULL);

static int
sm_nubus_match(device_t parent, cfdata_t cf, void *aux)
{
	struct nubus_attach_args *na = (struct nubus_attach_args *) aux;
	bus_space_handle_t bsh;
	int rv;

	if (bus_space_map(na->na_tag,
	    NUBUS_SLOT2PA(na->slot), NBMEMSIZE, 0, &bsh))
		return (0);

	rv = 0;

	if (na->category == NUBUS_CATEGORY_NETWORK &&
	    na->type == NUBUS_TYPE_ETHERNET) {
		switch (na->drsw) {
	    	case NUBUS_DRSW_FOCUS:
	    	case NUBUS_DRSW_ASANTEF:
			rv = 1;
			break;
		default:
			rv = 0;
			break;
		}
	}

	bus_space_unmap(na->na_tag, bsh, NBMEMSIZE);

	return rv;
}

/*
 * Install interface into kernel networking data structures
 */
static void
sm_nubus_attach(device_t parent, device_t self, void *aux)
{
	struct smc91cxx_softc *smc = device_private(self);
	struct nubus_attach_args *na = (struct nubus_attach_args *)aux;
	bus_space_tag_t	bst = na->na_tag;
	bus_space_handle_t bsh, prom_bsh;
	u_int8_t myaddr[ETHER_ADDR_LEN];
	int i, success;
	const char *cardtype;

	bst = na->na_tag;
	if (bus_space_map(bst, NUBUS_SLOT2PA(na->slot), NBMEMSIZE, 0, &bsh)) {
		printf(": failed to map memory space.\n");
		return;
	}

	mac68k_bus_space_handle_swapped(bst, &bsh);

	smc->sc_bst = bst;
	smc->sc_bsh = bsh;

	cardtype = nubus_get_card_name(bst, bsh, na->fmt);

	success = 0;

	switch (na->drsw) {
	case NUBUS_DRSW_FOCUS:
		if (bus_space_subregion(bst, bsh, 0xFF8000, 0x20, &prom_bsh)) {
			printf(": failed to map EEPROM space.\n");
			break;
		}

		success = 1;
		break;
	case NUBUS_DRSW_ASANTEF:
		if (bus_space_subregion(bst, bsh, 0xFE0000, 0x20, &prom_bsh)) {
			printf(": failed to map EEPROM space.\n");
			break;
		}

		success = 1;
		break;
	}

	if (!success) {
		bus_space_unmap(bst, bsh, NBMEMSIZE);
		return;
	}
	for (i=0 ; i<6 ; i++) {
		myaddr[i] = bus_space_read_1(bst, prom_bsh, i*4);
	}

	smc->sc_flags |= SMC_FLAGS_ENABLED;

	printf(": %s\n", cardtype);

	smc91cxx_attach(smc, myaddr);

	add_nubus_intr(na->slot, (void (*)(void *))smc91cxx_intr, smc);
}
