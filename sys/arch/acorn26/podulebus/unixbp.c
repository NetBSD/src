/* $NetBSD: unixbp.c,v 1.2 2002/03/24 23:37:45 bjh21 Exp $ */

/*-
 * Copyright (c) 2000 Ben Harris
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
 * unixbp.c - driver for the Unix Backplane, found in the R140 etc 
 */

#include <sys/param.h>

__KERNEL_RCSID(0, "$NetBSD: unixbp.c,v 1.2 2002/03/24 23:37:45 bjh21 Exp $");

#include <sys/device.h>
#include <sys/systm.h>
#include <machine/bus.h>
#include <arch/acorn26/iobus/iocvar.h>
#include <arch/acorn26/podulebus/unixbpreg.h>
#include <arch/acorn26/podulebus/unixbpvar.h>

struct unixbp_softc {
	struct device sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
};

static int unixbp_match(struct device *, struct cfdata *, void *);
static void unixbp_attach(struct device *, struct device *, void *);

struct cfattach unixbp_ca = {
	sizeof(struct unixbp_softc), unixbp_match, unixbp_attach
};

struct device *the_unixbp;

static int
unixbp_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct ioc_attach_args *ioc = aux;
	bus_space_tag_t bst = ioc->ioc_fast_t;
	bus_space_handle_t bsh = ioc->ioc_fast_h;

	/* Ask to disable all interrupts and check the request lines go low. */
	bus_space_write_1(bst, bsh, UNIXBP_REG_MASK, 0);
	if ((bus_space_read_1(bst, bsh, UNIXBP_REG_REQUEST) & 0xf) != 0)
		return 0;
	/* Re-enable interrupts. */
	bus_space_write_1(bst, bsh, UNIXBP_REG_MASK, 0xf);
	return 1;
}

static void
unixbp_attach(struct device *parent, struct device *self, void *aux)
{
	struct ioc_attach_args *ioc = aux;
	struct unixbp_softc *sc = (void *)self;

	if (the_unixbp == NULL)
		the_unixbp = self;
	sc->sc_iot = ioc->ioc_fast_t;
	sc->sc_ioh = ioc->ioc_fast_h;

	printf("\n");
}

int
unixbp_irq_status_full()
{
	struct unixbp_softc *sc = (void *)the_unixbp;

	if (sc == NULL)
		return 0;
	return bus_space_read_1(sc->sc_iot, sc->sc_ioh, UNIXBP_REG_REQUEST)
	    & 0xf;
}

void
unixbp_irq_setmask(int mask)
{
	struct unixbp_softc *sc = (void *)the_unixbp;

	if (sc == NULL)
		return;
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, UNIXBP_REG_MASK, mask & 0xf);
}
