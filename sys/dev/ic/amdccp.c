/* $NetBSD: amdccp.c,v 1.5 2022/03/19 11:55:03 riastradh Exp $ */

/*
 * Copyright (c) 2018 Jonathan A. Kollasch
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: amdccp.c,v 1.5 2022/03/19 11:55:03 riastradh Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/bus.h>
#include <sys/rndsource.h>

#include <dev/ic/amdccpvar.h>

static uint32_t amdccp_rnd_read(struct amdccp_softc *);
static void amdccp_rnd_callback(size_t, void *);

void
amdccp_common_attach(struct amdccp_softc *sc)
{

	rndsource_setcb(&sc->sc_rndsource, amdccp_rnd_callback, sc);
	rnd_attach_source(&sc->sc_rndsource, device_xname(sc->sc_dev),
	    RND_TYPE_RNG, RND_FLAG_COLLECT_VALUE|RND_FLAG_HASCB);
}

static uint32_t
amdccp_rnd_read(struct amdccp_softc *sc)
{
	size_t retries = 5;
	uint32_t bits;

	/* TRNG register reads 0x0 when bits aren't valid */
	while (retries--) {
		bits = bus_space_read_4(sc->sc_bst, sc->sc_bsh, CCP_TRNG_REG);
		if (bits != 0x0)
			break;
	}

	return bits;
}

static void
amdccp_rnd_callback(size_t bytes_wanted, void *priv)
{
	struct amdccp_softc * const sc = priv;
	uint32_t buf[128];
	size_t cnt;

	while (bytes_wanted) {
		const size_t nbytes = MIN(bytes_wanted, sizeof(buf));
		const size_t nwords = howmany(nbytes, sizeof(buf[0]));

		for (cnt = 0; cnt < nwords; cnt++) {
			buf[cnt] = amdccp_rnd_read(sc);
			if (buf[cnt] == 0) {
				break;
			}
		}
		if (cnt == 0) {
			break;
		}

		const size_t cntby = cnt * sizeof(buf[0]);

		rnd_add_data_sync(&sc->sc_rndsource, buf, cntby, cntby * NBBY);
		bytes_wanted -= MIN(bytes_wanted, cntby);
	}
	explicit_memset(buf, 0, sizeof(buf));
}
