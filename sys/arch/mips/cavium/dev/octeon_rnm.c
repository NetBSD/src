/*	$NetBSD: octeon_rnm.c,v 1.4 2020/05/12 14:04:50 simonb Exp $	*/

/*
 * Copyright (c) 2007 Internet Initiative Japan, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: octeon_rnm.c,v 1.4 2020/05/12 14:04:50 simonb Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/rndsource.h>
#include <sys/systm.h>

#include <mips/locore.h>
#include <mips/cavium/include/iobusvar.h>
#include <mips/cavium/dev/octeon_rnmreg.h>
#include <mips/cavium/dev/octeon_corereg.h>
#include <mips/cavium/octeonvar.h>

#include <sys/bus.h>

#define RNG_DELAY_CLOCK 91

struct octeon_rnm_softc {
	bus_space_tag_t		sc_bust;
	bus_space_handle_t	sc_regh;
	kmutex_t		sc_lock;
	krndsource_t		sc_rndsrc;	/* /dev/random source */
};

static int octeon_rnm_match(device_t, struct cfdata *, void *);
static void octeon_rnm_attach(device_t, device_t, void *);
static void octeon_rnm_rng(size_t, void *);
static uint64_t octeon_rnm_load(struct octeon_rnm_softc *);

CFATTACH_DECL_NEW(octeon_rnm, sizeof(struct octeon_rnm_softc),
    octeon_rnm_match, octeon_rnm_attach, NULL, NULL);

static int
octeon_rnm_match(device_t parent, struct cfdata *cf, void *aux)
{
	struct iobus_attach_args *aa = aux;
	int result = 0;

	if (strcmp(cf->cf_name, aa->aa_name) != 0)
		goto out;
	if (cf->cf_unit != aa->aa_unitno)
		goto out;
	result = 1;

out:
	return result;
}

static void
octeon_rnm_attach(device_t parent, device_t self, void *aux)
{
	struct octeon_rnm_softc *sc = device_private(self);
	struct iobus_attach_args *aa = aux;
	uint64_t bist_status;

	aprint_normal("\n");

	sc->sc_bust = aa->aa_bust;
	if (bus_space_map(aa->aa_bust, aa->aa_unit->addr, RNM_SIZE,
	    0, &sc->sc_regh) != 0) {
		aprint_error_dev(self, "unable to map device\n");
		return;
	}

	bist_status = bus_space_read_8(sc->sc_bust, sc->sc_regh,
	    RNM_BIST_STATUS_OFFSET);
	if (bist_status) {
		aprint_error_dev(self, "RNG built in self test failed: %#lx\n",
		    bist_status);
		return;
	}

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_VM);

#ifdef notyet
	/*
	 * Enable the internal ring oscillator entropy source (ENT),
	 * but disable the LFSR/SHA-1 engine (RNG) so we get the raw RO
	 * samples.
	 *
	 * XXX simonb
	 * To access the raw entropy, it looks like this needs to be
	 * done through the IOBDMA.  Put this in the "Too Hard For Now"
	 * basket and just use the RNG.
	 */
	bus_space_write_8(sc->sc_bust, sc->sc_regh, RNM_CTL_STATUS_OFFSET,
	    RNM_CTL_STATUS_EXP_ENT | RNM_CTL_STATUS_ENT_EN);

	/*
	 * Once entropy is enabled, 64 bits of raw entropy is available
	 * every 8 clock cycles.  Wait a microsecond now before the
	 * random callback is called to much sure random data is
	 * available.
	 */
	delay(1);
#else
	/* Enable the LFSR/SHA-1 engine (RNG). */
	bus_space_write_8(sc->sc_bust, sc->sc_regh, RNM_CTL_STATUS_OFFSET,
	    RNM_CTL_STATUS_RNG_EN | RNM_CTL_STATUS_ENT_EN);

	/*
	 * Once entropy is enabled, a 64-bit random number is available
	 * every 81 clock cycles.  Wait a microsecond now before the
	 * random callback is called to much sure random data is
	 * available.
	 */
	delay(1);
#endif

	rndsource_setcb(&sc->sc_rndsrc, octeon_rnm_rng, sc);
	rnd_attach_source(&sc->sc_rndsrc, device_xname(self), RND_TYPE_RNG,
	    RND_FLAG_DEFAULT | RND_FLAG_HASCB);
}

static void
octeon_rnm_rng(size_t nbytes, void *vsc)
{
	struct octeon_rnm_softc *sc = vsc;
	uint64_t rn;
	int i;

	/* Prevent concurrent access from emptying the FIFO.  */
	mutex_enter(&sc->sc_lock);
	for (i = 0; i < howmany(nbytes, sizeof(rn)); i++) {
		rn = octeon_rnm_load(sc);
		rnd_add_data(&sc->sc_rndsrc,
				&rn, sizeof(rn), sizeof(rn) * NBBY);
		/*
		 * XXX
		 *
		 * If accessing RNG data, the 512 byte FIFO that gets
		 * 8 bytes of RNG data added every 81 clock cycles.
		 *
		 * If accessing raw oscillator entropy, the 512 byte
		 * FIFO gets 8 bytes of raw entropy added every 8 clock
		 * cycles.
		 *
		 * We should in theory rate limit calls to
		 * octeon_rnm_load() to observe this limit.  In practice
		 * we don't appear to call octeon_rnm_load() anywhere
		 * near that often.
		 */
	}
	mutex_exit(&sc->sc_lock);
}

static uint64_t
octeon_rnm_load(struct octeon_rnm_softc *sc)
{
	uint64_t addr =
	    RNM_OPERATION_BASE_IO_BIT |
	    __BITS64_SET(RNM_OPERATION_BASE_MAJOR_DID, 0x08) |
	    __BITS64_SET(RNM_OPERATION_BASE_SUB_DID, 0x00);

	return octeon_xkphys_read_8(addr);
}
