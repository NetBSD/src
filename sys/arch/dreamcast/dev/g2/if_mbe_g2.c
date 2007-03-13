/*	$NetBSD: if_mbe_g2.c,v 1.5.30.1 2007/03/13 16:49:57 ad Exp $	*/

/*
 * Copyright (c) 2002 Christian Groessler
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * All Rights Reserved, Copyright (C) Fujitsu Limited 1995
 *
 * This software may be used, modified, copied, distributed, and sold, in
 * both source and binary form provided that the above copyright, these
 * terms and the following disclaimer are retained.  The name of the author
 * and/or the contributor may not be used to endorse or promote products
 * derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND THE CONTRIBUTOR ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR THE CONTRIBUTOR BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION.
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Portions copyright (C) 1993, David Greenman.	 This software may be used,
 * modified, copied, distributed, and sold, in both source and binary form
 * provided that the above copyright and these terms are retained.  Under no
 * circumstances is the author responsible for the proper functioning of this
 * software, nor does the author assume any responsibility for damages
 * incurred with its use.
 */

/*
 * Driver for Sega LAN Adapter (HIT-0300)
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_mbe_g2.c,v 1.5.30.1 2007/03/13 16:49:57 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/sysasicvar.h>
#include <machine/cpu.h>

#include <dev/ic/mb86960reg.h>
#include <dev/ic/mb86960var.h>

#include <dreamcast/dev/g2/g2busvar.h>


int	mbe_g2_match(struct device *, struct cfdata *, void *);
void	mbe_g2_attach(struct device *, struct device *, void *);
static int mbe_g2_detect(bus_space_tag_t, bus_space_handle_t, uint8_t *);

struct mbe_g2_softc {
	struct	mb86960_softc sc_mb86960;	/* real "mb86960" softc */
};

CFATTACH_DECL(mbe_g2bus, sizeof(struct mbe_g2_softc),
    mbe_g2_match, mbe_g2_attach, NULL, NULL);

#define LANA_NPORTS (0x20 * 4)

static struct dreamcast_bus_space mbe_g2_dbs;

/*
 * Determine if the device is present.
 */
int
mbe_g2_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct g2bus_attach_args *ga = aux;
	bus_space_handle_t memh;
	struct dreamcast_bus_space dbs;
	bus_space_tag_t memt = &dbs;
	static int lanafound;
	int rv;
	uint8_t myea[ETHER_ADDR_LEN];

	if (lanafound)
		return 0;

	if (strcmp("mbe", cf->cf_name))
		return 0;

	memcpy(memt, ga->ga_memt, sizeof(struct dreamcast_bus_space));
	g2bus_set_bus_mem_sparse(memt);

	/* Map i/o ports. */
	if (bus_space_map(memt, 0x00600400, LANA_NPORTS, 0, &memh)) {
#ifdef LANA_DEBUG
		printf("mbe_g2_match: couldn't map iospace 0x%x\n",
		    0x00600400);
#endif
		return 0;
	}

	rv = 0;
	if (mbe_g2_detect(memt, memh, myea) == 0) {
#ifdef LANA_DEBUG
		printf("mbe_g2_match: LAN Adapter detection failed\n");
#endif
		goto out;
	}

	rv = 1;
	lanafound = 1;
 out:
	bus_space_unmap(memt, memh, LANA_NPORTS);
	return rv;
}


/*
 * Determine type and ethernet address.
 */
static int
mbe_g2_detect(bus_space_tag_t iot, bus_space_handle_t ioh, uint8_t *enaddr)
{
	uint8_t eeprom[FE_EEPROM_SIZE];

	/* Read the chip type */
	if ((bus_space_read_1(iot, ioh, FE_DLCR7) & FE_D7_IDENT) !=
	    FE_D7_IDENT_86967) {
#ifdef LANA_DEBUG
		printf("mbe_g2_detect: unknown chip type\n");
#endif
		return 0;
	}

	memset(eeprom, 0, FE_EEPROM_SIZE);

	/* Get our station address from EEPROM. */
	mb86965_read_eeprom(iot, ioh, eeprom);
	memcpy(enaddr, eeprom, ETHER_ADDR_LEN);

#if LANA_DEBUG > 1
	printf("Ethernet address: %s\n", ether_sprintf(enaddr));
#endif

	/* Make sure we got a valid station address. */
	if ((enaddr[0] & 0x03) != 0x00 ||
	    (enaddr[0] == 0x00 && enaddr[1] == 0x00 && enaddr[2] == 0x00)) {
#ifdef LANA_DEBUG
		printf("mbe_g2_detect: invalid ethernet address\n");
#endif
		return 0;
	}

	return 1;
}

void
mbe_g2_attach(struct device *parent, struct device *self, void *aux)
{
	struct g2bus_attach_args *ga = aux;
	struct mbe_g2_softc *isc = (struct mbe_g2_softc *)self;
	struct mb86960_softc *sc = &isc->sc_mb86960;
	bus_space_tag_t memt = &mbe_g2_dbs;
	bus_space_handle_t memh;
	uint8_t myea[ETHER_ADDR_LEN];

	memcpy(memt, ga->ga_memt, sizeof(struct dreamcast_bus_space));
	g2bus_set_bus_mem_sparse(memt);

	/* Map i/o ports. */
	if (bus_space_map(memt, 0x00600400, LANA_NPORTS, 0, &memh)) {
		printf(": can't map i/o space\n");
		return;
	}

	sc->sc_bst = memt;
	sc->sc_bsh = memh;

	/* Determine the card type. */
	if (mbe_g2_detect(memt, memh, myea) == 0) {
		printf(": where did the card go?!\n");
		panic("unknown card");
	}

	printf(": Sega LAN-Adapter Ethernet\n");

	/* This interface is always enabled. */
	sc->sc_stat |= FE_STAT_ENABLED;

	/* The LAN-Adapter uses 8 bit bus mode and slow SRAM. */
	sc->sc_flags |= FE_FLAGS_SBW_BYTE | FE_FLAGS_SRAM_150ns;

	/*
	 * Do generic MB86960 attach.
	 */
	mb86960_attach(sc, myea);

	mb86960_config(sc, NULL, 0, 0);

	sysasic_intr_establish(SYSASIC_EVENT_8BIT, IPL_NET, SYSASIC_IRL11,
	    mb86960_intr, sc);
}
