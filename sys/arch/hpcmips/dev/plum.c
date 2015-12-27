/*	$NetBSD: plum.c,v 1.14.14.1 2015/12/27 12:09:36 skrll Exp $ */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: plum.c,v 1.14.14.1 2015/12/27 12:09:36 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <hpcmips/tx/tx39var.h>
#include <hpcmips/tx/txcsbusvar.h>
#include <hpcmips/dev/plumvar.h>
#include <hpcmips/dev/plumreg.h>

int plum_match(device_t, cfdata_t, void *);
void plum_attach(device_t, device_t, void *);
int plum_print(void *, const char *);
int plum_search(device_t, cfdata_t, const int *, void *);

struct plum_softc {
	plum_chipset_tag_t	sc_pc;
	bus_space_tag_t		sc_csregt;
	bus_space_tag_t		sc_csiot;
	bus_space_tag_t		sc_csmemt;
	int			sc_irq;
	int			sc_pri;
};

CFATTACH_DECL_NEW(plum, sizeof(struct plum_softc),
    plum_match, plum_attach, NULL, NULL);

plumreg_t plum_idcheck(bus_space_tag_t);

int
plum_match(device_t parent, cfdata_t cf, void *aux)
{
	struct cs_attach_args *ca = aux;

	switch (plum_idcheck(ca->ca_csreg.cstag)) {
	default:
		return (0);
	case PLUM2_1:
	case PLUM2_2:
		/* nothing */;
	}

	return (1);
}

void
plum_attach(device_t parent, device_t self, void *aux)
{
	struct cs_attach_args *ca = aux;
	struct plum_softc *sc = device_private(self);
	plumreg_t reg;

	sc->sc_csregt	= ca->ca_csreg.cstag;
	sc->sc_csiot	= ca->ca_csio.cstag;
	sc->sc_csmemt	= ca->ca_csmem.cstag;
	sc->sc_irq	= ca->ca_irq1;

	switch (reg = plum_idcheck(sc->sc_csregt)) {
	default:
		printf(": unknown revision %#x\n", reg);
		return;
	case PLUM2_1:
		printf(": Plum2 #1\n");
		break;
	case PLUM2_2:
		printf(": Plum2 #2\n");
		break;
	}
	if (!(sc->sc_pc = malloc(sizeof(struct plum_chipset_tag),
	    M_DEVBUF, M_NOWAIT))) {
		panic("no memory");
	}
	memset(sc->sc_pc, 0, sizeof(struct plum_chipset_tag));
	sc->sc_pc->pc_tc = ca->ca_tc;
	
	/* Attach Plum devices */
	/* 
	 * interrupt, power/clock module is used by other plum module.
	 * attach first.
	 */
	sc->sc_pri = 2;
	config_search_ia(plum_search, self, "plumif", plum_print);
	/* 
	 * Other plum module.
	 */
	sc->sc_pri = 1;
	config_search_ia(plum_search, self, "plumif", plum_print);
}

plumreg_t
plum_idcheck(bus_space_tag_t regt)
{
	bus_space_handle_t regh;
	plumreg_t reg;

	if (bus_space_map(regt, PLUM_ID_REGBASE, 
	    PLUM_ID_REGSIZE, 0, &regh)) {
		printf("ID register map failed\n");
		return (0);
	}
	reg = plum_conf_read(regt, regh, PLUM_ID_REG);
	bus_space_unmap(regt, regh, PLUM_ID_REGSIZE);
	
	return (reg);
}

int
plum_print(void *aux, const char *pnp)
{

	return (pnp ? QUIET : UNCONF);
}

int
plum_search(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct plum_softc *sc = device_private(parent);
	struct plum_attach_args pa;
	
	pa.pa_pc	= sc->sc_pc;
	pa.pa_regt	= sc->sc_csregt;
	pa.pa_iot	= sc->sc_csiot;
	pa.pa_memt	= sc->sc_csmemt;
	pa.pa_irq	= sc->sc_irq;

	if (config_match(parent, cf, &pa) == sc->sc_pri) {
		config_attach(parent, cf, &pa, plum_print);
	}

	return 0;
}

plumreg_t
plum_conf_read(bus_space_tag_t t, bus_space_handle_t h, int o)
{
	plumreg_t reg;

	reg = bus_space_read_4(t, h, o);

	return (reg);
}

void
plum_conf_write(bus_space_tag_t t, bus_space_handle_t h, int o, plumreg_t v)
{

	bus_space_write_4(t, h, o, v);
}

void
plum_conf_register_intr(plum_chipset_tag_t t, void *intrt)
{

	if (t->pc_intrt) {
		panic("duplicate intrt");
	}

	t->pc_intrt = intrt;
}

void
plum_conf_register_power(plum_chipset_tag_t t, void *powert)
{

	if (t->pc_powert) {
		panic("duplicate powert");
	}

	t->pc_powert = powert;
}
