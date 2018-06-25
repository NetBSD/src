/*	$NetBSD: videopll.c,v 1.2.2.1 2018/06/25 07:25:43 pgoyette Exp $	*/

/*
 * Copyright (c) 2012 Michael Lorenz
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * A driver for the iic-controlled PLLs used in early Apple onboard video 
 * hardware. For now we support /valkyrie only but others use very similar
 * schemes to program their pixel clock so adding support for those should
 * be simple enough.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: videopll.c,v 1.2.2.1 2018/06/25 07:25:43 pgoyette Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <dev/ofw/openfirm.h>
#include <dev/i2c/i2cvar.h>
#include <arch/macppc/dev/videopllvar.h>

#include "opt_videopll.h"
#ifdef VIDEOPLL_DEBUG
#define DPRINTF printf
#else
#define DPRINTF while (0) printf
#endif

struct videopll_softc {
	device_t sc_dev;
	i2c_tag_t sc_tag;
	int sc_addr;
};

static int  videopll_match(device_t, cfdata_t, void *);
static void videopll_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(videopll, sizeof(struct videopll_softc),
	videopll_match, videopll_attach, NULL, NULL);

static void *glob = NULL;

static int
videopll_match(device_t parent, cfdata_t cfdata, void *aux)
{
	struct i2c_attach_args *ia = aux;
	int match_result;

	if (iic_use_direct_match(ia, cfdata, NULL, &match_result))
		return match_result;
	
	/* This driver is direct-config only. */

	return 0;
}

static void
videopll_attach(device_t parent, device_t self, void *aux)
{
	struct i2c_attach_args *ia = aux;
	struct videopll_softc *sc = device_private(self);

	sc->sc_dev = self;
	sc->sc_tag = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;
	aprint_normal(": Apple onboard video PLL\n");
	glob = sc;
}

/*
 * pixel clock:
 * 3.9064MHz * 2^p2 * p1 / p0
 */
int
videopll_set_freq(int freq)
{
	struct videopll_softc *sc = glob;
	int p0, p1, p2;
	int freq_out, diff, diff_b = 100000000;
	int b0 = 0, b1 = 0, b2 = 0, freq_b = 0;
	uint8_t cmdbuf[4];

	if (glob == NULL)
		return EIO;
	/*
	 * XXX
	 * The parameter ranges were taken from Linux' valkyriefb.c mode list.
	 * We don't really know what exact parameters the PLL supports but
	 * this should be enough for the modes we can actually support
	 */
	for (p2 = 2; p2 < 4; p2++) {
		for (p1 = 27; p1 < 43; p1++) {
			for (p0 = 11; p0 < 26; p0++) {
				freq_out = (3906400 * (1 << p2) * p1) / (p0 * 1000);
				diff = abs(freq - freq_out);
				if (diff < diff_b) {
					diff_b = diff;
					b0 = p0;
					b1 = p1;
					b2 = p2;
					freq_b = freq_out;
				}
			}
		}
	}
	if (freq_b == 0)
		return EINVAL;
	DPRINTF("param: %d %d %d -> %d\n", b0, b1, b2, freq_b);
	iic_acquire_bus(sc->sc_tag, 0);
	cmdbuf[0] = 1;
	cmdbuf[1] = b0;
	iic_exec(sc->sc_tag, I2C_OP_WRITE,
	    sc->sc_addr, cmdbuf, 2, NULL, 0, 0);
	cmdbuf[0] = 2;
	cmdbuf[1] = b1;
	iic_exec(sc->sc_tag, I2C_OP_WRITE,
	    sc->sc_addr, cmdbuf, 2, NULL, 0, 0);
	cmdbuf[0] = 3;
	cmdbuf[1] = b2;
	iic_exec(sc->sc_tag, I2C_OP_WRITE,
	    sc->sc_addr, cmdbuf, 2, NULL, 0, 0);
	iic_release_bus(sc->sc_tag, 0);
	return 0;
}
	


