/*	$NetBSD: lm_isa.c,v 1.1 2000/02/25 02:17:44 groo Exp $ */

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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/errno.h>
#include <sys/conf.h>

#include <sys/envsys.h>

#include <machine/bus.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#include <machine/intr.h>
#include <machine/bus.h>

#include <dev/ic/nslm7xvar.h>

#if defined(LMDEBUG)
#define DPRINTF(x)		do { printf x; } while (0)
#else
#define DPRINTF(x)
#endif


int lm_isa_match __P((struct device *, struct cfdata *, void *));
void lm_isa_attach __P((struct device *, struct device *, void *));

struct cfattach lm_isa_ca = {
	sizeof(struct lm_softc), lm_isa_match, lm_isa_attach
};


int
lm_isa_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	struct isa_attach_args *ia = aux;
	int iobase;
	int rv;
	u_int8_t cr;

	/* Must supply an address */
	if (ia->ia_iobase == ISACF_PORT_DEFAULT)
		return 0;

	iot = ia->ia_iot;
	iobase = ia->ia_iobase;

	if (bus_space_map(iot, iobase, 8, 0, &ioh))
		return (0);

	/* Check for some power-on defaults */
	bus_space_write_1(iot, ioh, LMC_ADDR, LMD_CONFIG);

	/* Perform LM78 reset */
	bus_space_write_1(iot, ioh, LMC_DATA, 0x80);

	/* XXX - Why do I have to reselect the register? */
	bus_space_write_1(iot, ioh, LMC_ADDR, LMD_CONFIG);
	cr = bus_space_read_1(iot, ioh, LMC_DATA);

	/* XXX - spec says *only* 0x08! */
	if ((cr == 0x08) || (cr == 0x01))
		rv = 1;
	else
		rv = 0;

	DPRINTF(("lm: rv = %d, cr = %x\n", rv, cr));

	bus_space_unmap(iot, ioh, 8);

	if (rv) {
		ia->ia_iosize = 8;
		ia->ia_msize = 0;
	}

	return (rv);
}


void
lm_isa_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct lm_softc *lmsc = (void *)self;
	int iobase;
	bus_space_tag_t iot;
	struct isa_attach_args *ia = aux;

        iobase = ia->ia_iobase;
	iot = lmsc->lm_iot = ia->ia_iot;

	if (bus_space_map(iot, iobase, 8, 0, &lmsc->lm_ioh)) {
		printf(": can't map i/o space\n");
		return;
	}

	/* Bus-independant attachment */
	lm_attach(lmsc);
}
