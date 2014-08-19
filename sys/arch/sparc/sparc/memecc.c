/*	$NetBSD: memecc.c,v 1.14.2.2 2014/08/20 00:03:24 tls Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
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

/*
 * ECC memory control.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: memecc.c,v 1.14.2.2 2014/08/20 00:03:24 tls Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <sys/bus.h>
#include <machine/autoconf.h>
#include <sparc/sparc/memeccreg.h>

struct memecc_softc {
	bus_space_tag_t		sc_bt;
	bus_space_handle_t	sc_bh;
};

struct memecc_softc *memecc_sc;

/* autoconfiguration driver */
static void	memecc_attach(device_t, device_t, void *);
static int	memecc_match(device_t, cfdata_t, void *);
static int	memecc_error(void);

extern int (*memerr_handler)(void);

CFATTACH_DECL_NEW(eccmemctl, sizeof(struct memecc_softc),
    memecc_match, memecc_attach, NULL, NULL);

int
memecc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	return (strcmp("eccmemctl", ma->ma_name) == 0);
}

/*
 * Attach the device.
 */
void
memecc_attach(device_t parent, device_t self, void *aux)
{
	struct memecc_softc *sc = device_private(self);
	struct mainbus_attach_args *ma = aux;
	uint32_t reg;

	if (memerr_handler) {
		printf("%s: already attached\n", __func__);
		return;
	}

	sc->sc_bt = ma->ma_bustag;

	/*
	 * Map registers
	 */
	if (bus_space_map(
			ma->ma_bustag,
			ma->ma_paddr,
			ma->ma_size,
			0,
			&sc->sc_bh) != 0) {
		printf("memecc_attach: cannot map registers\n");
		return;
	}

	reg = bus_space_read_4(sc->sc_bt, sc->sc_bh, ECC_EN_REG);

	printf(": version 0x%x/0x%x\n",
		(reg & ECC_EN_VER) >> 24,
		(reg & ECC_EN_IMPL) >> 28);

	/* Enable checking & interrupts */
	reg |= ECC_EN_EE | ECC_EN_EI;
	bus_space_write_4(sc->sc_bt, sc->sc_bh, ECC_EN_REG, reg);
	memecc_sc = sc;

	memerr_handler = memecc_error;
}

/*
 * Called if the MEMORY ERROR bit is set after a level 25 interrupt.
 */
int
memecc_error(void)
{
	bus_space_handle_t bh = memecc_sc->sc_bh;
	uint32_t efsr, efar0, efar1;
	char bits[64];

	efsr = bus_space_read_4(memecc_sc->sc_bt, bh, ECC_FSR_REG);
	efar0 = bus_space_read_4(memecc_sc->sc_bt, bh, ECC_AFR0_REG);
	efar1 = bus_space_read_4(memecc_sc->sc_bt, bh, ECC_AFR1_REG);
	snprintb(bits, sizeof(bits), ECC_FSR_BITS, efsr);
	printf("memory error:\n\tEFSR: %s\n", bits);
	snprintb(bits, sizeof(bits), ECC_AFR_BITS, efar0);
	printf("\tMBus transaction: %s\n", bits);
	printf("\taddress: 0x%x%x\n", efar0 & ECC_AFR_PAH, efar1);
	printf("\tmodule location: %s\n",
		prom_pa_location(efar1, efar0 & ECC_AFR_PAH));

	/* Unlock registers and clear interrupt */
	bus_space_write_4(memecc_sc->sc_bt, bh, ECC_FSR_REG, efsr);

	/* Return 0 if this was a correctable error */
	return ((efsr & ECC_FSR_CE) == 0);
}
