/* $NetBSD: am335x_trng.c,v 1.1.4.2 2015/09/22 12:05:38 skrll Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: am335x_trng.c,v 1.1.4.2 2015/09/22 12:05:38 skrll Exp $");

#include "opt_omap.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/intr.h>
#include <sys/mutex.h>
#include <sys/bus.h>
#include <sys/rndpool.h>
#include <sys/rndsource.h>

#include <arm/omap/am335x_prcm.h>
#include <arm/omap/omap2_prcm.h>
#include <arm/omap/sitara_cm.h>
#include <arm/omap/sitara_cmreg.h>

#include <arm/omap/omap2_reg.h>
#include <arm/omap/omap2_obiovar.h>

#include <arm/omap/am335x_trngreg.h>

static const struct omap_module rng_module =
        { AM335X_PRCM_CM_PER, CM_PER_RNG_CLKCTRL };

struct trng_softc {
	device_t sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;

	kmutex_t sc_intr_lock;
	kmutex_t sc_rnd_lock;
	u_int sc_bytes_wanted;
	void *sc_sih;
	krndsource_t sc_rndsource;
};

static int	trng_match(device_t, cfdata_t, void *);
static void	trng_attach(device_t, device_t, void *);
static void	trng_softintr(void *);
static void	trng_callback(size_t, void *);

CFATTACH_DECL_NEW(trng, sizeof(struct trng_softc),
    trng_match, trng_attach, NULL, NULL);

#define TRNG_READ(sc, reg) \
	bus_space_read_4((sc)->sc_iot, (sc)->sc_ioh, (reg))
#define TRNG_WRITE(sc, reg, val) \
	bus_space_write_4((sc)->sc_iot, (sc)->sc_ioh, (reg), (val))

static int
trng_match(device_t parent, cfdata_t match, void *aux)
{
	struct obio_attach_args *obio = aux;

	if (obio->obio_addr == AM335X_TRNG_BASE &&
	    obio->obio_size == AM335X_TRNG_SIZE &&
	    obio->obio_intr == AM335X_INT_TRNG)
		return 1;

	return 0;
}

static void
trng_attach(device_t parent, device_t self, void *aux)
{
	struct trng_softc *sc = device_private(self);
	struct obio_attach_args *obio = aux;

	sc->sc_dev = self;
	sc->sc_iot = obio->obio_iot;
	if (bus_space_map(obio->obio_iot, obio->obio_addr, obio->obio_size,
	    0, &sc->sc_ioh) != 0) {
		aprint_error(": couldn't map address spcae\n");
		return;
	}
	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_SERIAL);
	mutex_init(&sc->sc_rnd_lock, MUTEX_DEFAULT, IPL_SERIAL);
	sc->sc_bytes_wanted = 0;
	sc->sc_sih = softint_establish(SOFTINT_SERIAL|SOFTINT_MPSAFE,
	    trng_softintr, sc);
	if (sc->sc_sih == NULL) {
		aprint_error(": couldn't establish softint\n");
		return;
	}

	prcm_module_enable(&rng_module);

	if ((TRNG_READ(sc, TRNG_CONTROL_REG) & TRNG_CONTROL_ENABLE) == 0) {
		TRNG_WRITE(sc, TRNG_CONFIG_REG,
		    __SHIFTIN(0x21, TRNG_CONFIG_MIN_REFILL) |
		    __SHIFTIN(0x22, TRNG_CONFIG_MAX_REFILL));
		TRNG_WRITE(sc, TRNG_CONTROL_REG,
		    __SHIFTIN(0x21, TRNG_CONTROL_STARTUP_CYCLES) |
		    TRNG_CONTROL_ENABLE);
	}

	rndsource_setcb(&sc->sc_rndsource, trng_callback, sc);
	rnd_attach_source(&sc->sc_rndsource, device_xname(self), RND_TYPE_RNG,
	    RND_FLAG_COLLECT_VALUE|RND_FLAG_HASCB);

	aprint_naive("\n");
	aprint_normal("\n");

	trng_callback(RND_POOLBITS / NBBY, sc);
}

static void
trng_softintr(void *priv)
{
	struct trng_softc * const sc = priv;
	uint32_t buf[2];
	u_int retry;

	mutex_enter(&sc->sc_intr_lock);
	while (sc->sc_bytes_wanted) {
		for (retry = 10; retry > 0; retry--) {
			if (TRNG_READ(sc, TRNG_STATUS_REG) & TRNG_STATUS_READY)
				break;
			delay(10);
		}
		if (retry == 0)
			break;
		buf[0] = TRNG_READ(sc, TRNG_OUTPUT_L_REG);
		buf[1] = TRNG_READ(sc, TRNG_OUTPUT_H_REG);
		TRNG_WRITE(sc, TRNG_INTACK_REG, TRNG_INTACK_READY);
		mutex_exit(&sc->sc_intr_lock);
		mutex_enter(&sc->sc_rnd_lock);
		rnd_add_data(&sc->sc_rndsource, buf, sizeof(buf),
		    sizeof(buf) * NBBY);
		mutex_exit(&sc->sc_rnd_lock);
		mutex_enter(&sc->sc_intr_lock);
		sc->sc_bytes_wanted -= MIN(sc->sc_bytes_wanted, sizeof(buf));
	}
	explicit_memset(buf, 0, sizeof(buf));
	mutex_exit(&sc->sc_intr_lock);
}

static void
trng_callback(size_t bytes_wanted, void *priv)
{
	struct trng_softc * const sc = priv;

	mutex_enter(&sc->sc_intr_lock);
	if (sc->sc_bytes_wanted == 0) {
		softint_schedule(sc->sc_sih);
	}
	if (bytes_wanted > (UINT_MAX - sc->sc_bytes_wanted)) {
		sc->sc_bytes_wanted = UINT_MAX;
	} else {
		sc->sc_bytes_wanted += bytes_wanted;
	}
	mutex_exit(&sc->sc_intr_lock);
}
