/*	$NetBSD: rng200.c,v 1.4 2022/03/19 11:55:03 riastradh Exp $	*/

/*
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Michael van Elst
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
 * Driver for the Broadcom iProc RNG200
 */

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/mutex.h>
#include <sys/rndsource.h>

#include <dev/ic/rng200var.h>
#include <dev/ic/rng200reg.h>

#define READ4(sc, r) \
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (r))

#define WRITE4(sc, r, v) \
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (r), (v))

static void
rng200_reset(struct rng200_softc *sc)
{
	uint32_t ctl, rng, rbg;

	/* Disable RBG */
	ctl = READ4(sc, RNG200_CONTROL);
	ctl &= ~RNG200_RBG_MASK;
	WRITE4(sc, RNG200_CONTROL, ctl);

	/* Clear interrupts */
	WRITE4(sc, RNG200_STATUS, 0xffffffff);

	/* Reset RNG and RBG */
	rbg = READ4(sc, RNG200_RBG_RESET);
	rng = READ4(sc, RNG200_RNG_RESET);
	WRITE4(sc, RNG200_RBG_RESET, rbg | RBG_RESET);
	WRITE4(sc, RNG200_RNG_RESET, rng | RNG_RESET);
	WRITE4(sc, RNG200_RNG_RESET, rng);
	WRITE4(sc, RNG200_RBG_RESET, rbg);

	/* Enable RBG */
	WRITE4(sc, RNG200_CONTROL, ctl | RNG200_RBG_ENABLE);
}

static void
rng200_get(size_t bytes_wanted, void *priv)
{
	struct rng200_softc * const sc = priv;
	uint32_t w, data;
	unsigned count;

	while (bytes_wanted) {

		w = READ4(sc, RNG200_STATUS);
		if ((w & (RNG200_MASTER_FAIL | RNG200_NIST_FAIL)) != 0)
			rng200_reset(sc);

		w = READ4(sc, RNG200_COUNT);
		count = __SHIFTOUT(w, RNG200_COUNT_MASK);

		if (count == 0)
			break;

		data = READ4(sc, RNG200_DATA);
		rnd_add_data_sync(&sc->sc_rndsource, &data,
		    sizeof(data), sizeof(data) * NBBY);
		bytes_wanted -= MIN(bytes_wanted, sizeof(data));
	}
	explicit_memset(&data, 0, sizeof(data));
}

void
rng200_attach(struct rng200_softc *sc)
{

	rndsource_setcb(&sc->sc_rndsource, rng200_get, sc);
	rnd_attach_source(&sc->sc_rndsource, sc->sc_name,
		RND_TYPE_RNG, RND_FLAG_COLLECT_VALUE|RND_FLAG_HASCB);
}

void
rng200_detach(struct rng200_softc *sc)
{

	rnd_detach_source(&sc->sc_rndsource);
}

