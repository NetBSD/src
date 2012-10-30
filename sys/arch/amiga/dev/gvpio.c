/*	$NetBSD: gvpio.c,v 1.19.2.1 2012/10/30 17:18:48 yamt Exp $ */

/*
 * Copyright (c) 1997 Ignatios Souvatzis
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gvpio.c,v 1.19.2.1 2012/10/30 17:18:48 yamt Exp $");

/*
 * GVP I/O Extender
 */

#include <sys/types.h>

#include <sys/conf.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/param.h>
#include <sys/bus.h>

#include <machine/intr.h>

#include <amiga/include/cpu.h>

#include <amiga/amiga/device.h>
#include <amiga/amiga/drcustom.h>

#include <amiga/dev/supio.h>
#include <amiga/dev/zbusvar.h>
#include <amiga/dev/gvpbusvar.h>

struct gvpio_softc {
	struct bus_space_tag sc_bst;
	void *sc_cntr;
	LIST_HEAD(, gvpcom_int_hdl) sc_comhdls;
	struct isr sc_comisr;
};

int gvpiomatch(device_t, cfdata_t, void *);
void gvpioattach(device_t, device_t, void *);
int gvpioprint(void *, const char *);
int gvp_com_intr(void *);
void gvp_com_intr_establish(device_t, struct gvpcom_int_hdl *);

CFATTACH_DECL_NEW(gvpio, sizeof(struct gvpio_softc),
    gvpiomatch, gvpioattach, NULL, NULL);

int
gvpiomatch(device_t parent, cfdata_t cf, void *aux)
{

	struct gvpbus_args *gap;

	gap = aux;

	if (gap->flags & GVP_IO)
		return (1);

	return (0);
}

struct gvpio_devs {
	char *name;
	int off;
	int arg;
	int ipl;
} gvpiodevs[] = {
	{ "com", 0x0b0, 115200 * 16 * 4, 6 },
	{ "com", 0x130, 115200 * 16 * 4, 6 },
	{ "lpt", 0x1b0, 0, 2 },
	{ 0 }
};

void
gvpioattach(device_t parent, device_t self, void *aux)
{
	struct gvpio_softc *giosc;
	struct gvpio_devs  *giosd;
	struct gvpbus_args *gap;
	struct supio_attach_args supa;
	volatile void *gbase;
#ifdef __m68k__
	u_int16_t needpsl;
#endif

	giosc = device_private(self);
	gap = aux;

	if (parent)
		printf("\n");

	gbase = gap->zargs.va;
	giosc->sc_cntr = &gbase[0x41];
	giosc->sc_bst.base = (u_long)gbase + 1;
	giosc->sc_bst.absm = &amiga_bus_stride_2;
	LIST_INIT(&giosc->sc_comhdls);
	giosd = gvpiodevs;

	supa.supio_iot = &giosc->sc_bst;

	gbase[0x041] = 0;
	gbase[0x161 + 1*2] = 0;
	gbase[0x261 + 1*2] = 0;
	gbase[0x361 + 2*2] = 0;
	gbase[0x461] = 0xd0;

	while (giosd->name) {
		supa.supio_name = giosd->name;
		supa.supio_iobase = giosd->off;
		supa.supio_arg = giosd->arg;
		supa.supio_ipl = giosd->ipl;
		config_found(self, &supa, gvpioprint); /* XXX */
		++giosd;
	}
	if (giosc->sc_comhdls.lh_first) {
#ifdef __m68k__
		/* XXX this should be really in the interrupt stuff */
		needpsl = PSL_S|PSL_IPL6;
		if (ipl2spl_table[IPL_SERIAL] < needpsl) {
			printf("%s: raising ipl2spl_table[IPL_SERIAL] "
			    "from 0x%x to 0x%x\n",
			    device_xname(self), ipl2spl_table[IPL_SERIAL],
			    needpsl);
			ipl2spl_table[IPL_SERIAL] = needpsl;
		}
#endif
		giosc->sc_comisr.isr_intr = gvp_com_intr;
		giosc->sc_comisr.isr_arg = giosc;
		giosc->sc_comisr.isr_ipl = 6;
		add_isr(&giosc->sc_comisr);
	}
	gbase[0x041] = 0x08;
}

int
gvpioprint(void *aux, const char *pnp)
{
	struct supio_attach_args *supa;

	supa = aux;

	if (pnp == NULL)
		return(QUIET);

	aprint_normal("%s at %s port 0x%02x ipl %d",
	    supa->supio_name, pnp, supa->supio_iobase, supa->supio_ipl);

	return(UNCONF);
}

void
gvp_com_intr_establish(device_t self, struct gvpcom_int_hdl *p)
{
	struct gvpio_softc *sc;

	sc = device_private(self);
	LIST_INSERT_HEAD(&sc->sc_comhdls, p, next);

}

int
gvp_com_intr(void *p)
{
	struct gvpio_softc *sc;
	struct gvpcom_int_hdl *np;
	volatile void *cntr;

	sc = (struct gvpio_softc *)p;

	cntr = sc->sc_cntr;

	if (!(*cntr & 0x2))
		return (0);

	*cntr &= ~0xa;

	for (np = sc->sc_comhdls.lh_first; np != NULL;
	    np = np->next.le_next) {

		(*np->f)(np->p);
	}
	*cntr |= 0x8;

	return (1);
}
