/*	$NetBSD: octeon_rnm.c,v 1.1.18.2 2017/12/03 11:36:27 jdolecek Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: octeon_rnm.c,v 1.1.18.2 2017/12/03 11:36:27 jdolecek Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/sysctl.h>
#include <sys/kernel.h>
#include <sys/rndsource.h>

#include <mips/locore.h>
#include <mips/cavium/include/iobusvar.h>
#include <mips/cavium/dev/octeon_rnmreg.h>
#include <mips/cavium/dev/octeon_corereg.h>
#include <mips/cavium/octeonvar.h>

#include <sys/bus.h>
#include <machine/param.h>

#define RNG_DELAY_CLOCK 91
#define RNG_DEF_BURST_COUNT 10

int octeon_rnm_burst_count = RNG_DEF_BURST_COUNT;

struct octeon_rnm_softc {
	device_t sc_dev;

	bus_space_tag_t		sc_bust;
	bus_space_handle_t	sc_regh;

	krndsource_t		sc_rndsrc;	/* /dev/random source */
	struct callout		sc_rngto;	/* rng timeout */
	int			sc_rnghz;	/* rng poll time */
};

static int octeon_rnm_match(device_t, struct cfdata *, void *);
static void octeon_rnm_attach(device_t, device_t, void *);
static void octeon_rnm_rng(void *);
static inline uint64_t octeon_rnm_load(struct octeon_rnm_softc *);
static inline int octeon_rnm_iobdma(struct octeon_rnm_softc *);

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
	int status;

	aprint_normal("\n");

	sc->sc_dev = self;
	sc->sc_bust = aa->aa_bust;
	status = bus_space_map(aa->aa_bust, aa->aa_unit->addr, RNM_SIZE,
	    0, &sc->sc_regh);
	if (status != 0)
		panic(": can't map i/o space");

	bus_space_write_8(sc->sc_bust, sc->sc_regh, RNM_CTL_STATUS_OFFSET,
	    RNM_CTL_STATUS_RNG_EN | RNM_CTL_STATUS_ENT_EN);

	if (hz >= 100)
		sc->sc_rnghz = hz / 100;
	else 
		sc->sc_rnghz = 1;

	rnd_attach_source(&sc->sc_rndsrc, device_xname(sc->sc_dev),
	    RND_TYPE_RNG, RND_FLAG_NO_ESTIMATE);

	callout_init(&sc->sc_rngto, 0);

	octeon_rnm_rng(sc);

	aprint_normal("%s: random number generator enabled: %dhz\n",
	    device_xname(sc->sc_dev), sc->sc_rnghz);
}

static void
octeon_rnm_rng(void *vsc)
{
	struct octeon_rnm_softc *sc = vsc;
	uint64_t rn;
	int i;

	for (i = 0; i < octeon_rnm_burst_count; i++) {
		rn = octeon_rnm_load(sc);
		rnd_add_data(&sc->sc_rndsrc,
				&rn, sizeof(rn), sizeof(rn) * NBBY);
		/*
		 * XXX
		 * delay should be over RNG_DELAY_CLOCK cycles at least,
		 * we need nanodelay() or clkdelay().
		 */
		delay(1);
	}

	callout_reset(&sc->sc_rngto, sc->sc_rnghz, octeon_rnm_rng, sc);
}

static inline uint64_t
octeon_rnm_load(struct octeon_rnm_softc *sc)
{
	uint64_t addr =
	    RNM_OPERATION_BASE_IO_BIT |
	    __BITS64_SET(RNM_OPERATION_BASE_MAJOR_DID, 0x08) |
	    __BITS64_SET(RNM_OPERATION_BASE_SUB_DID, 0x00);

	return octeon_xkphys_read_8(addr);
}

static inline int
octeon_rnm_iobdma(struct octeon_rnm_softc *sc)
{

	/* XXX */
	return 0;
}

SYSCTL_SETUP(octeon_rnm_sysctl, "sysctl octeon_rnm subtree setup")
{
	int rc, root_num;
	const struct sysctlnode *node;

	if ( (rc = sysctl_createv(clog, 0, NULL, NULL,
			CTLFLAG_PERMANENT, CTLTYPE_NODE, "hw", NULL,
			NULL, 0, NULL, 0, CTL_HW, CTL_EOL)) != 0) {
		goto err;
	}

	if ( (rc = sysctl_createv(clog, 0, NULL, &node,
			CTLFLAG_PERMANENT, CTLTYPE_NODE, "octeon_rnm",
			SYSCTL_DESCR("octeon_rnm controls"),
			NULL, 0, NULL, 0, CTL_HW, CTL_CREATE, CTL_EOL)) != 0) {
		goto err;
	}

	root_num = node->sysctl_num;

	if ( (rc = sysctl_createv(clog, 0, NULL, NULL,
			CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
			CTLTYPE_INT, "burst_count",
			SYSCTL_DESCR("Burst read count per callout"),
			NULL, 0, &octeon_rnm_burst_count,
			0, CTL_HW, root_num, CTL_CREATE, CTL_EOL)) != 0) {
		goto err;
	}

	return;
err:
	printf("%s: sysctl_createv failed (rc = %d)\n", __func__, rc);
}
