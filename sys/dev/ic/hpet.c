/* $NetBSD: hpet.c,v 1.9 2010/01/08 20:02:39 dyoung Exp $ */

/*
 * Copyright (c) 2006 Nicolas Joly
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS
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
 * High Precision Event Timer.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: hpet.c,v 1.9 2010/01/08 20:02:39 dyoung Exp $");

#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <sys/time.h>
#include <sys/timetc.h>

#include <sys/bus.h>

#include <dev/ic/hpetreg.h>
#include <dev/ic/hpetvar.h>

static u_int	hpet_get_timecount(struct timecounter *);
static bool	hpet_resume(device_t, pmf_qual_t);

int
hpet_detach(device_t dv, int flags)
{
	struct hpet_softc *sc = device_private(dv);
	int rc;

	if ((rc = tc_detach(&sc->sc_tc)) != 0)
		return rc;

	pmf_device_deregister(dv);

	bus_space_write_4(sc->sc_memt, sc->sc_memh, HPET_CONFIG, sc->sc_config);

	return 0;
}

void
hpet_attach_subr(device_t dv)
{
	struct hpet_softc *sc = device_private(dv);
	struct timecounter *tc;
	uint32_t val;

	tc = &sc->sc_tc;

	tc->tc_name = device_xname(dv);
	tc->tc_get_timecount = hpet_get_timecount;
	tc->tc_quality = 2000;

	tc->tc_counter_mask = 0xffffffff;

	/* Get frequency */
	val = bus_space_read_4(sc->sc_memt, sc->sc_memh, HPET_PERIOD);
	if (val == 0) {
		aprint_error_dev(dv, "invalid timer period\n");
		return;
	}
	tc->tc_frequency = 1000000000000000ULL / val;

	/* Enable timer */
	val = bus_space_read_4(sc->sc_memt, sc->sc_memh, HPET_CONFIG);
	sc->sc_config = val;
	if ((val & HPET_CONFIG_ENABLE) == 0) {
		val |= HPET_CONFIG_ENABLE;
		bus_space_write_4(sc->sc_memt, sc->sc_memh, HPET_CONFIG, val);
	}

	tc->tc_priv = sc;
	tc_init(tc);

	if (!pmf_device_register(dv, NULL, hpet_resume))
		aprint_error_dev(dv, "couldn't establish power handler\n");
}

static u_int
hpet_get_timecount(struct timecounter *tc)
{
	struct hpet_softc *sc = tc->tc_priv;

	return bus_space_read_4(sc->sc_memt, sc->sc_memh, HPET_MCOUNT_LO);
}

static bool
hpet_resume(device_t dv, pmf_qual_t qual)
{
	struct hpet_softc *sc = device_private(dv);
	uint32_t val;

	val = bus_space_read_4(sc->sc_memt, sc->sc_memh, HPET_CONFIG);
	val |= HPET_CONFIG_ENABLE;
	bus_space_write_4(sc->sc_memt, sc->sc_memh, HPET_CONFIG, val);

	return true;
}
