/*	$NetBSD: lm_isa.c,v 1.14.2.1 2007/04/20 21:09:02 bouyer Exp $ */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Bill Squier.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: lm_isa.c,v 1.14.2.1 2007/04/20 21:09:02 bouyer Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/conf.h>

#include <machine/bus.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#include <machine/bus.h>

#include <dev/sysmon/sysmonvar.h>

#include <dev/ic/nslm7xvar.h>

#if defined(LMDEBUG)
#define DPRINTF(x)		do { printf x; } while (0)
#else
#define DPRINTF(x)
#endif


int lm_isa_match(struct device *, struct cfdata *, void *);
void lm_isa_attach(struct device *, struct device *, void *);
uint8_t lm_isa_readreg(struct lm_softc *, int);
void lm_isa_writereg(struct lm_softc *, int, int);

CFATTACH_DECL(lm_isa, sizeof(struct lm_softc),
    lm_isa_match, lm_isa_attach, NULL, NULL);


int
lm_isa_match(struct device *parent, struct cfdata *match, void *aux)
{
	bus_space_handle_t ioh;
	struct isa_attach_args *ia = aux;
	int rv;

	/* Must supply an address */
	if (ia->ia_nio < 1)
		return 0;

	if (ISA_DIRECT_CONFIG(ia))
		return 0;

	if (ia->ia_io[0].ir_addr == ISA_UNKNOWN_PORT)
		return 0;

	if (bus_space_map(ia->ia_iot, ia->ia_io[0].ir_addr, 8, 0, &ioh))
		return 0;


	/* Bus independent probe */
	rv = lm_probe(ia->ia_iot, ioh);

	bus_space_unmap(ia->ia_iot, ioh, 8);

	if (rv) {
		ia->ia_nio = 1;
		ia->ia_io[0].ir_size = 8;

		ia->ia_niomem = 0;
		ia->ia_nirq = 0;
		ia->ia_ndrq = 0;
	}

	return rv;
}


void
lm_isa_attach(struct device *parent, struct device *self, void *aux)
{
	struct lm_softc *lmsc = (void *)self;
	struct isa_attach_args *ia = aux;

	lmsc->lm_iot = ia->ia_iot;

	if (bus_space_map(ia->ia_iot, ia->ia_io[0].ir_addr, 8, 0,
	    &lmsc->lm_ioh)) {
		aprint_error(": can't map i/o space\n");
		return;
	}

	/* Bus-independent attachment */
	lmsc->lm_writereg = lm_isa_writereg;
	lmsc->lm_readreg = lm_isa_readreg;

	lm_attach(lmsc);
}


uint8_t
lm_isa_readreg(struct lm_softc *sc, int reg)
{
	bus_space_write_1(sc->lm_iot, sc->lm_ioh, LMC_ADDR, reg);
	return bus_space_read_1(sc->lm_iot, sc->lm_ioh, LMC_DATA);
}


void
lm_isa_writereg(struct lm_softc *sc, int reg, int val)
{
	bus_space_write_1(sc->lm_iot, sc->lm_ioh, LMC_ADDR, reg);
	bus_space_write_1(sc->lm_iot, sc->lm_ioh, LMC_DATA, val);
}
