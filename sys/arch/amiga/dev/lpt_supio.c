/*	$NetBSD: lpt_supio.c,v 1.6.20.1 2002/02/28 04:06:52 nathanw Exp $ */

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ignatios Souvatzis.
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
__KERNEL_RCSID(0, "$NetBSD: lpt_supio.c,v 1.6.20.1 2002/02/28 04:06:52 nathanw Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/tty.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include <sys/device.h>

#include <machine/intr.h>
#include <machine/bus.h>

#include <dev/ic/lptreg.h>
#include <dev/ic/lptvar.h>

#include <amiga/dev/supio.h>

struct lptsupio_softc {
	struct lpt_softc sc_lpt;
	struct isr sc_isr;
	void (*sc_intack)(void *);
};

int lpt_supio_match(struct device *, struct cfdata *, void *);
void lpt_supio_attach(struct device *, struct device *, void *);
int lpt_supio_intr(void *p);

struct cfattach lpt_supio_ca = {
	sizeof(struct lptsupio_softc), lpt_supio_match, lpt_supio_attach
};

int
lpt_supio_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct supio_attach_args *supa = aux;

	if (strcmp(supa->supio_name,"lpt"))
		return 0;

	return (1);
}

int
lpt_supio_intr(void *p)
{
	struct lptsupio_softc *sc = (void *)p;
	int rc;

	rc = lptintr(&sc->sc_lpt);
	if (sc->sc_intack)
		(*sc->sc_intack)(p);
	return (rc);
}

void
lpt_supio_attach(struct device *parent, struct device *self, void *aux)
{
	struct lptsupio_softc *sc = (void *)self;
	struct lpt_softc *lsc = &sc->sc_lpt;
	int iobase;
	bus_space_tag_t iot;
	struct supio_attach_args *supa = aux;

	/*
	 * We're living on a superio chip.
	 */
	iobase = supa->supio_iobase;
	iot = lsc->sc_iot = supa->supio_iot;
	sc->sc_intack = (void *)supa->supio_arg;

        if (bus_space_map(iot, iobase, LPT_NPORTS, 0, &lsc->sc_ioh))
		panic("lpt_supio_attach: io mapping failed");

	printf(" port 0x%04x ipl %d\n", iobase, supa->supio_ipl);
	lpt_attach_subr(lsc);

	sc->sc_isr.isr_intr = lpt_supio_intr;
	sc->sc_isr.isr_arg = sc;
	sc->sc_isr.isr_ipl = supa->supio_ipl;
	add_isr(&sc->sc_isr);
}
