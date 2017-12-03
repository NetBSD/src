/* $NetBSD: rockchip_timer.c,v 1.2.18.2 2017/12/03 11:35:55 jdolecek Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: rockchip_timer.c,v 1.2.18.2 2017/12/03 11:35:55 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/timetc.h>

#include <arm/rockchip/rockchip_reg.h>
#include <arm/rockchip/rockchip_var.h>

#include <arm/rockchip/rockchip_timerreg.h>

struct rktimer_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	struct timecounter sc_tc;
};

#define TIMER_READ(sc, reg) \
    bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define TIMER_WRITE(sc, reg, val) \
    bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static int	rktimer_match(device_t, cfdata_t, void *);
static void	rktimer_attach(device_t, device_t, void *);

static u_int	rktimer_get_timecount(struct timecounter *);

CFATTACH_DECL_NEW(rktimer, sizeof(struct rktimer_softc),
	rktimer_match, rktimer_attach, NULL, NULL);

static int
rktimer_match(device_t parent, cfdata_t cf, void *aux)
{

	if (rockchip_chip_id() == ROCKCHIP_CHIP_ID_RK3066)
		return 0;

	return 1;
}

static void
rktimer_attach(device_t parent, device_t self, void *aux)
{
	struct rktimer_softc *sc = device_private(self);
	struct obio_attach_args * const obio = aux;

	sc->sc_dev = self;
	sc->sc_bst = obio->obio_bst;
	bus_space_subregion(obio->obio_bst, obio->obio_bsh, obio->obio_offset,
	    obio->obio_size, &sc->sc_bsh);

	aprint_naive("\n");
	aprint_normal("\n");

	TIMER_WRITE(sc, TIMER0_CONTROL_REG, 0);
	TIMER_WRITE(sc, TIMER0_LOAD_COUNT0_REG, 0xffffffff);
	TIMER_WRITE(sc, TIMER0_LOAD_COUNT1_REG, 0xffffffff);
	TIMER_WRITE(sc, TIMER0_CONTROL_REG, TIMER_CONTROL_ENABLE);

	sc->sc_tc.tc_get_timecount = rktimer_get_timecount;
	sc->sc_tc.tc_poll_pps = NULL;
	sc->sc_tc.tc_counter_mask = ~0;
	sc->sc_tc.tc_frequency = ROCKCHIP_REF_FREQ;
	sc->sc_tc.tc_name = "TIMER0";
	sc->sc_tc.tc_priv = sc;
	sc->sc_tc.tc_quality = 900;

	tc_init(&sc->sc_tc);
}

static u_int
rktimer_get_timecount(struct timecounter *tc)
{
	struct rktimer_softc *sc = tc->tc_priv;

	return ~TIMER_READ(sc, TIMER0_CURRENT_VALUE0_REG);
}
