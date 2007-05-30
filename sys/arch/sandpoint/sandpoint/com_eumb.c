/* $NetBSD: com_eumb.c,v 1.1.2.1 2007/05/30 17:02:51 nisimura Exp $ */

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tohru Nishimura.
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
__KERNEL_RCSID(0, "$NetBSD: com_eumb.c,v 1.1.2.1 2007/05/30 17:02:51 nisimura Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/tty.h>
#include <sys/systm.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <sandpoint/sandpoint/eumbvar.h>
#include "locators.h"

static int  com_eumb_match(struct device *, struct cfdata *, void *);
static void com_eumb_attach(struct device *, struct device *, void *);

CFATTACH_DECL(com_eumb, sizeof(struct com_softc),
    com_eumb_match, com_eumb_attach, NULL, NULL);

static int found;
static struct com_regs cnregs;

/*
 * There are two different UART configurations, single 4-wire UART
 * and dual 2-wire.  DCR register selects one of the two operating
 * mode.  A certain group of NAS boxes uses the 2nd UART as system
 * console while the 1st to communicate power management satellite
 * processor. "unit" locator helps to reverse the two.  Default is a
 * single 4-wire UART as console.
 */
int
com_eumb_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct eumb_attach_args *eaa = aux;
	int unit = eaa->eumb_unit;

	if (unit == EUMBCF_UNIT_DEFAULT && found == 0)
		return (1);
	if (unit == 0 || unit == 1)
		return (1);
	return (0);
}

void
com_eumb_attach(struct device *parent, struct device *self, void *aux)
{
	struct com_softc *sc = (struct com_softc *)self;
	struct eumb_attach_args *eaa = aux;
	int comaddr, epicirq;
	bus_space_handle_t ioh;
	extern u_long ticks_per_sec;

	found = 1;

	comaddr = (eaa->eumb_unit == 1) ? 0x4600 : 0x4500;
	if (comaddr == cnregs.cr_iobase)
		sc->sc_regs = cnregs;
	else {
		ioh = comaddr;
		bus_space_map(eaa->eumb_bt, comaddr, COM_NPORTS, 0, &ioh);
		COM_INIT_REGS(sc->sc_regs, eaa->eumb_bt, ioh, comaddr);
	}
	sc->sc_frequency = 4 * ticks_per_sec;
	epicirq = (eaa->eumb_unit == 1) ? 25 : 24;

	com_attach_subr(sc);
	intr_establish(epicirq + 16, IST_LEVEL, IPL_SERIAL, comintr, sc);
}

int
eumbcnattach(bus_space_tag_t tag,
    int conaddr, int conspeed, int confreq, int contype, int conmode)
{
	static int attached = 0;

	if (attached)
		return 0;
	attached = 1;

	cnregs.cr_iot = tag;
	cnregs.cr_iobase = conaddr;
	cnregs.cr_nports = COM_NPORTS;
	/* cnregs.ioh is initialized by comcnattach */
	return comcnattach1(&cnregs, conspeed, confreq, contype, conmode);
}
