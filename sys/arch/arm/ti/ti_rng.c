/* $NetBSD: ti_rng.c,v 1.2.2.2 2019/11/27 13:46:44 martin Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: ti_rng.c,v 1.2.2.2 2019/11/27 13:46:44 martin Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/mutex.h>
#include <sys/bus.h>
#include <sys/rndpool.h>
#include <sys/rndsource.h>

#include <dev/fdt/fdtvar.h>

#include <arm/ti/ti_prcm.h>
#include <arm/ti/ti_rngreg.h>

static const char * const compatible[] = {
	"ti,omap4-rng",
	NULL
};

struct ti_rng_softc {
	device_t sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;

	kmutex_t sc_lock;
	krndsource_t sc_rndsource;
};

static int	ti_rng_match(device_t, cfdata_t, void *);
static void	ti_rng_attach(device_t, device_t, void *);
static void	ti_rng_callback(size_t, void *);

CFATTACH_DECL_NEW(ti_rng, sizeof(struct ti_rng_softc),
    ti_rng_match, ti_rng_attach, NULL, NULL);

#define RD4(sc, reg) \
	bus_space_read_4((sc)->sc_iot, (sc)->sc_ioh, (reg))
#define WR4(sc, reg, val) \
	bus_space_write_4((sc)->sc_iot, (sc)->sc_ioh, (reg), (val))

static int
ti_rng_match(device_t parent, cfdata_t match, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
ti_rng_attach(device_t parent, device_t self, void *aux)
{
	struct ti_rng_softc *sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	if (ti_prcm_enable_hwmod(phandle, 0) != 0) {
		aprint_error(": couldn't enable module\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_iot = faa->faa_bst;
	if (bus_space_map(sc->sc_iot, addr, size, 0, &sc->sc_ioh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_VM);

	if ((RD4(sc, TRNG_CONTROL_REG) & TRNG_CONTROL_ENABLE) == 0) {
		WR4(sc, TRNG_CONFIG_REG,
		    __SHIFTIN(0x21, TRNG_CONFIG_MIN_REFILL) |
		    __SHIFTIN(0x22, TRNG_CONFIG_MAX_REFILL));
		WR4(sc, TRNG_CONTROL_REG,
		    __SHIFTIN(0x21, TRNG_CONTROL_STARTUP_CYCLES) |
		    TRNG_CONTROL_ENABLE);
	}

	rndsource_setcb(&sc->sc_rndsource, ti_rng_callback, sc);
	rnd_attach_source(&sc->sc_rndsource, device_xname(self), RND_TYPE_RNG,
	    RND_FLAG_COLLECT_VALUE|RND_FLAG_HASCB);

	aprint_naive("\n");
	aprint_normal(": RNG\n");

	ti_rng_callback(RND_POOLBITS / NBBY, sc);
}

static void
ti_rng_callback(size_t bytes_wanted, void *priv)
{
	struct ti_rng_softc * const sc = priv;
	uint32_t buf[2];
	u_int retry;

	mutex_enter(&sc->sc_lock);
	while (bytes_wanted) {
		for (retry = 10; retry > 0; retry--) {
			if (RD4(sc, TRNG_STATUS_REG) & TRNG_STATUS_READY)
				break;
			delay(10);
		}
		if (retry == 0)
			break;
		buf[0] = RD4(sc, TRNG_OUTPUT_L_REG);
		buf[1] = RD4(sc, TRNG_OUTPUT_H_REG);
		WR4(sc, TRNG_INTACK_REG, TRNG_INTACK_READY);
		rnd_add_data_sync(&sc->sc_rndsource, buf, sizeof(buf),
		    sizeof(buf) * NBBY);
		bytes_wanted -= MIN(bytes_wanted, sizeof(buf));
	}
	explicit_memset(buf, 0, sizeof(buf));
	mutex_exit(&sc->sc_lock);
}
