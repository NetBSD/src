/*	$NetBSD: com_elb.c,v 1.5.14.3 2008/09/28 10:39:55 mjf Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Juergen Hannken-Illjes.
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
__KERNEL_RCSID(0, "$NetBSD: com_elb.c,v 1.5.14.3 2008/09/28 10:39:55 mjf Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/tty.h>

#include <machine/bus.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <evbppc/explora/dev/elbvar.h>

struct com_elb_softc {
	struct com_softc sc_com;
	void *sc_ih;
};

static int	com_elb_probe(device_t, cfdata_t , void *);
static void	com_elb_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(com_elb, sizeof(struct com_elb_softc),
    com_elb_probe, com_elb_attach, NULL, NULL);

int
com_elb_probe(device_t parent, cfdata_t cf, void *aux)
{
	struct elb_attach_args *oaa = aux;

	if (strcmp(oaa->elb_name, cf->cf_name) != 0)
		return 0;

	return (1);
}

void
com_elb_attach(device_t parent, device_t self, void *aux)
{
	struct com_elb_softc *msc = device_private(self);
	struct com_softc *sc = &msc->sc_com;
	struct elb_attach_args *eaa = aux;
	bus_space_handle_t ioh;

	sc->sc_dev = self;

	bus_space_map(eaa->elb_bt,
	    _BUS_SPACE_UNSTRIDE(eaa->elb_bt, eaa->elb_base),
	    COM_NPORTS, 0, &ioh);
	COM_INIT_REGS(sc->sc_regs, eaa->elb_bt, ioh,
	    _BUS_SPACE_UNSTRIDE(eaa->elb_bt, eaa->elb_base));

	sc->sc_frequency = COM_FREQ;

	com_attach_subr(sc);

	intr_establish(eaa->elb_irq, IST_LEVEL, IPL_SERIAL, comintr, sc);
}
