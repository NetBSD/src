/* $NetBSD: ioeb.c,v 1.2.6.2 2002/06/23 17:33:46 jdolecek Exp $ */

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

#include <sys/param.h>

__KERNEL_RCSID(0, "$NetBSD: ioeb.c,v 1.2.6.2 2002/06/23 17:33:46 jdolecek Exp $");

#include <sys/device.h>
#include <sys/systm.h>

#include <machine/bus.h>

#include <arch/acorn26/iobus/iocvar.h>
#include <arch/acorn26/ioc/ioebreg.h>
#include <arch/acorn26/ioc/ioebvar.h>

struct ioeb_softc {
	struct device sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
};

static int ioeb_match(struct device *, struct cfdata *, void *);
static void ioeb_attach(struct device *, struct device *, void *);

struct cfattach ioeb_ca = {
	sizeof(struct ioeb_softc), ioeb_match, ioeb_attach
};

struct device *the_ioeb;

/* IOEB is only four bits wide */
#define ioeb_read(t, h, o) (bus_space_read_1(t, h, o) & 0xf)

static int
ioeb_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct ioc_attach_args *ioc = aux;
	int id;

	id = ioeb_read(ioc->ioc_fast_t, ioc->ioc_fast_h, IOEB_REG_ID);
	if (id == IOEB_ID_IOEB)
		return 1;
	if (id != 0xf)
		printf("ioeb_match: ID = %x\n", id);
	return 0;
}

static void
ioeb_attach(struct device *parent, struct device *self, void *aux)
{
	struct ioeb_softc *sc = (void *)self;
	struct ioc_attach_args *ioc = aux;

	if (the_ioeb == NULL)
		the_ioeb = self;
	sc->sc_iot = ioc->ioc_fast_t;
	sc->sc_ioh = ioc->ioc_fast_h;
	printf("\n");
}

void
ioeb_irq_clear(int mask)
{
	struct ioeb_softc *sc = (void *)the_ioeb;

	/* The IOEB only controls interrupt 0 */
	if (mask & IOEB_IRQ_CLEARABLE_MASK)
		bus_space_write_1(sc->sc_iot, sc->sc_ioh, IOEB_REG_INTRCLR, 0);
}
