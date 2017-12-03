/* $NetBSD: awin_cnt.c,v 1.3.16.2 2017/12/03 11:35:50 jdolecek Exp $ */

/*-
 * Copyright (c) 2014 Jared D. McNeill <jmcneill@invisible.ca>
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

#include "opt_multiprocessor.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: awin_cnt.c,v 1.3.16.2 2017/12/03 11:35:50 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/timetc.h>

#include <arm/allwinner/awin_reg.h>
#include <arm/allwinner/awin_var.h>

struct awin_cnt_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	struct timecounter sc_tc;
	kmutex_t sc_lock;
};

#define CNT_READ(sc, reg) \
    bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define CNT_WRITE(sc, reg, val) \
    bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static int	awin_cnt_match(device_t, cfdata_t, void *);
static void	awin_cnt_attach(device_t, device_t, void *);

static u_int	awin_cnt_get_timecount(struct timecounter *);

CFATTACH_DECL_NEW(awin_cnt, sizeof(struct awin_cnt_softc),
	awin_cnt_match, awin_cnt_attach, NULL, NULL);

static int
awin_cnt_match(device_t parent, cfdata_t cf, void *aux)
{
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;

	if (strcmp(cf->cf_name, loc->loc_name))
		return 0;

	return 1;
}

static void
awin_cnt_attach(device_t parent, device_t self, void *aux)
{
	struct awin_cnt_softc *sc = device_private(self);
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;

	sc->sc_dev = self;
	sc->sc_bst = aio->aio_core_bst;
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_HIGH);
	bus_space_subregion(sc->sc_bst, aio->aio_core_bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc_bsh);

	aprint_naive("\n");
	aprint_normal("\n");

	/* Set OSC24M clock source */
	uint32_t ctrl = CNT_READ(sc, AWIN_CPUCFG_CNT64_CTRL_REG);
	ctrl &= ~AWIN_CPUCFG_CNT64_CLK_SRC_SEL;
	CNT_WRITE(sc, AWIN_CPUCFG_CNT64_CTRL_REG, ctrl);

	sc->sc_tc.tc_get_timecount = awin_cnt_get_timecount;
	sc->sc_tc.tc_poll_pps = NULL;
	sc->sc_tc.tc_counter_mask = ~0;
	sc->sc_tc.tc_frequency = AWIN_REF_FREQ;
	sc->sc_tc.tc_name = "CNT64";
	sc->sc_tc.tc_priv = sc;
	sc->sc_tc.tc_quality = 200;

	tc_init(&sc->sc_tc);
}

static u_int
awin_cnt_get_timecount(struct timecounter *tc)
{
	struct awin_cnt_softc *sc = tc->tc_priv;
	uint32_t ctrl;
	u_int timecount;

	mutex_enter(&sc->sc_lock);

	/* Enable read latch, wait for it to clear */
	ctrl = CNT_READ(sc, AWIN_CPUCFG_CNT64_CTRL_REG);
	ctrl |= AWIN_CPUCFG_CNT64_RL_EN;
	CNT_WRITE(sc, AWIN_CPUCFG_CNT64_CTRL_REG, ctrl);
	do {
		ctrl = CNT_READ(sc, AWIN_CPUCFG_CNT64_CTRL_REG);
	} while (ctrl & AWIN_CPUCFG_CNT64_RL_EN);

	timecount = CNT_READ(sc, AWIN_CPUCFG_CNT64_LOW_REG);

	mutex_exit(&sc->sc_lock);

	return timecount;
}
