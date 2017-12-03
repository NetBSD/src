/*	$NetBSD: fwhrng.c,v 1.5.6.2 2017/12/03 11:36:50 jdolecek Exp $	*/

/*
 * Copyright (c) 2000 Michael Shalayeff
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
 * IN NO EVENT SHALL THE AUTHOR OR HIS RELATIVES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF MIND, USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 *	from OpenBSD: pchb.c,v 1.23 2000/10/23 20:07:30 deraadt Exp
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: fwhrng.c,v 1.5.6.2 2017/12/03 11:36:50 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/time.h>
#include <sys/rndsource.h>

#include <sys/bus.h>

#include <arch/x86/pci/i82802reg.h>

struct fwhrng_softc {
	device_t sc_dev;

	bus_space_tag_t sc_st;
	bus_space_handle_t sc_sh;

	struct callout sc_rnd_ch;
	krndsource_t sc_rnd_source;

	int sc_rnd_i;
	uint32_t sc_rnd_ax;
};

static int fwhrng_match(device_t, cfdata_t, void *);
static void fwhrng_attach(device_t, device_t, void *);
static int fwhrng_detach(device_t, int);

static void fwhrng_callout(void *v);

#define	FWHRNG_RETRIES		1000
#define	FWHRNG_MIN_SAMPLES	10

CFATTACH_DECL_NEW(fwhrng, sizeof(struct fwhrng_softc),
    fwhrng_match, fwhrng_attach, fwhrng_detach, NULL);

static int
fwhrng_match(device_t parent, cfdata_t match, void *aux)
{
	bus_space_tag_t bst;
	bus_space_handle_t bsh;
	uint8_t id0, id1, data0, data1;

	bst = x86_bus_space_mem;

	/* read chip ID */
	if (bus_space_map(bst, I82802AB_MEMBASE, I82802AB_WINSIZE, 0, &bsh))
		return 0;

	bus_space_write_1(bst, bsh, 0, 0xff); /* reset */
	data0 = bus_space_read_1(bst, bsh, 0);
	data1 = bus_space_read_1(bst, bsh, 1);
	bus_space_write_1(bst, bsh, 0, 0x90); /* enter read id */
	id0 = bus_space_read_1(bst, bsh, 0);
	id1 = bus_space_read_1(bst, bsh, 1);
	bus_space_write_1(bst, bsh, 0, 0xff); /* reset */

	bus_space_unmap(bst, bsh, I82802AB_WINSIZE);

	aprint_debug_dev(parent, "fwh: data %02x,%02x, id %02x,%02x\n",
	    data0, data1, id0, id1);

	/* unlikely to have these match if we actually read the ID */
	if ((id0 == data0) && (id1 == data1))
		return 0;

	/* check for chips with RNG */
	if (!(id0 == I82802_MFG))
		return 0;
	if (!((id1 == I82802AB_ID) || (id1 == I82802AC_ID)))
		return 0;

	/* check for RNG presence */
	if (bus_space_map(bst, I82802AC_REGBASE, I82802AC_WINSIZE, 0, &bsh))
		return 0;
	data0 = bus_space_read_1(bst, bsh, I82802_RNG_HSR);
	bus_space_unmap(bst, bsh, I82802AC_WINSIZE);
	if ((data0 & I82802_RNG_HSR_PRESENT) == I82802_RNG_HSR_PRESENT)
		return 1;

	return 0;
}

static void
fwhrng_attach(device_t parent, device_t self, void *aux)
{
	struct fwhrng_softc *sc;
	int i, j, count_ff;
	uint8_t reg8;

	sc = device_private(self);
	sc->sc_dev = self;

	aprint_naive("\n");
	aprint_normal(": Intel Firmware Hub Random Number Generator\n");

	sc->sc_st = x86_bus_space_mem;

	if (bus_space_map(sc->sc_st, I82802AC_REGBASE, I82802AC_WINSIZE, 0,
	    &sc->sc_sh) != 0) {
		aprint_error_dev(self, "unable to map registers\n");
		return;
	}

	/* Enable the RNG. */
	reg8 = bus_space_read_1(sc->sc_st, sc->sc_sh, I82802_RNG_HSR);
	bus_space_write_1(sc->sc_st, sc->sc_sh, I82802_RNG_HSR,
	    reg8 | I82802_RNG_HSR_ENABLE);
	reg8 = bus_space_read_1(sc->sc_st, sc->sc_sh, I82802_RNG_HSR);
	if ((reg8 & I82802_RNG_HSR_ENABLE) == 0) {
		aprint_error_dev(self, "unable to enable\n");
		bus_space_unmap(sc->sc_st, sc->sc_sh, I82802AC_WINSIZE);
		return;
	}

	/* Check to see if we can read data from the RNG. */
	count_ff = 0;
	for (j = 0; j < FWHRNG_MIN_SAMPLES; ++j) {
		for (i = 0; i < FWHRNG_RETRIES; i++) {
			reg8 = bus_space_read_1(sc->sc_st, sc->sc_sh,
			    I82802_RNG_DSR);
			if (!(reg8 & I82802_RNG_DSR_VALID)) {
				delay(10);
				continue;
			}
			reg8 = bus_space_read_1(sc->sc_st, sc->sc_sh,
			    I82802_RNG_DR);
			break;
		}
		if (i == FWHRNG_RETRIES) {
			bus_space_unmap(sc->sc_st, sc->sc_sh, I82802AC_WINSIZE);
			aprint_verbose_dev(sc->sc_dev,
			    "timeout reading test samples, RNG disabled.\n");
			return;
		}
		if (reg8 == 0xff)
			++count_ff;
	}

	if (count_ff == FWHRNG_MIN_SAMPLES) {
		/* Disable the RNG. */
		reg8 = bus_space_read_1(sc->sc_st, sc->sc_sh, I82802_RNG_HSR);
		bus_space_write_1(sc->sc_st, sc->sc_sh, I82802_RNG_HSR,
		    reg8 & ~I82802_RNG_HSR_ENABLE);
		bus_space_unmap(sc->sc_st, sc->sc_sh, I82802AC_WINSIZE);
		aprint_error_dev(sc->sc_dev,
		    "returns constant 0xff stream, RNG disabled.\n");
		return;
	}

	/*
	 * Should test entropy source to ensure
	 * that it passes the Statistical Random
	 * Number Generator Tests in section 4.11.1,
	 * FIPS PUB 140-1.
	 *
	 *	http://csrc.nist.gov/fips/fips1401.htm
	 */

	aprint_debug_dev(sc->sc_dev, "random number generator enabled\n");

	callout_init(&sc->sc_rnd_ch, 0);
	/* FWH is polled for entropy, so no estimate is available. */
	rnd_attach_source(&sc->sc_rnd_source, device_xname(sc->sc_dev),
	    RND_TYPE_RNG, RND_FLAG_COLLECT_VALUE);
	sc->sc_rnd_i = sizeof(sc->sc_rnd_ax);
	fwhrng_callout(sc);

	return;
}

static int
fwhrng_detach(device_t self, int flags)
{
	struct fwhrng_softc *sc;
	uint8_t reg8;

	sc = device_private(self);

	rnd_detach_source(&sc->sc_rnd_source);

	callout_halt(&sc->sc_rnd_ch, NULL);
	callout_destroy(&sc->sc_rnd_ch);

	/* Disable the RNG. */
	reg8 = bus_space_read_1(sc->sc_st, sc->sc_sh, I82802_RNG_HSR);
	bus_space_write_1(sc->sc_st, sc->sc_sh, I82802_RNG_HSR,
	    reg8 & ~I82802_RNG_HSR_ENABLE);

	bus_space_unmap(sc->sc_st, sc->sc_sh, I82802AC_WINSIZE);

	return 0;
}

static void
fwhrng_callout(void *v)
{
	struct fwhrng_softc *sc = v;

	if ((bus_space_read_1(sc->sc_st, sc->sc_sh, I82802_RNG_DSR) &
	     I82802_RNG_DSR_VALID) != 0) {
		sc->sc_rnd_ax = (sc->sc_rnd_ax << NBBY) |
		    bus_space_read_1(sc->sc_st, sc->sc_sh, I82802_RNG_DR);
		if (--sc->sc_rnd_i == 0) {
			sc->sc_rnd_i = sizeof(sc->sc_rnd_ax);
			rnd_add_data(&sc->sc_rnd_source, &sc->sc_rnd_ax,
			    sizeof(sc->sc_rnd_ax),
			    sizeof(sc->sc_rnd_ax) * NBBY);
		}
	}
	callout_reset(&sc->sc_rnd_ch, 1, fwhrng_callout, sc);
}
