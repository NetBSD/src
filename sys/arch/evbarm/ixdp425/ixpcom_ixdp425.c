/*	$NetBSD: ixpcom_ixdp425.c,v 1.2 2003/06/01 01:49:57 ichiro Exp $ */
/*
 * Copyright (c) 2003
 *	Ichiro FUKUHARA <ichiro@ichiro.org>.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Ichiro FUKUHARA.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ICHIRO FUKUHARA ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ICHIRO FUKUHARA OR THE VOICES IN HIS HEAD BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ixpcom_ixdp425.c,v 1.2 2003/06/01 01:49:57 ichiro Exp $");

/* Front-end of ixpcom */

#include <sys/types.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/param.h>
#include <sys/malloc.h>

#include <sys/termios.h>

#include <machine/intr.h>
#include <machine/bus.h>

#include <arm/xscale/ixp425reg.h>
#include <arm/xscale/ixp425var.h>
#include <arm/xscale/ixp425_sipvar.h>
#include <arm/xscale/ixp425_comvar.h>

struct ixpcom_ixdp_softc {
	struct ixp4xx_com_softc	sc_ixpcom;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
};

static int	ixpcom_ixdp_match(struct device *, struct cfdata *, void *);
static void	ixpcom_ixdp_attach(struct device *, struct device *, void *);

CFATTACH_DECL(ixpcom_ixdp, sizeof(struct ixp4xx_com_softc),
    ixpcom_ixdp_match, ixpcom_ixdp_attach, NULL, NULL);

const struct uart_info comcn_config[] = {
	{ "HighSpeed Serial (UART0)",
	  IXP425_UART0_HWBASE,
	  IXP425_UART0_VBASE,
	  IXP425_INT_UART0,
	},
	{ "Console (UART1)",
	  IXP425_UART1_HWBASE,
	  IXP425_UART1_VBASE,
	  IXP425_INT_UART1,
	},
};

static int
ixpcom_ixdp_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	if (strcmp(match->cf_name, "ixpcom") == 0)
		return 1;
	return 0;
}

static void
ixpcom_ixdp_attach(parent, self, aux)
	struct device *parent;
	struct device *self;  
	void *aux;
{
	struct ixpcom_ixdp_softc *isc = (struct ixpcom_ixdp_softc *)self;
	struct ixp4xx_com_softc *sc = &isc->sc_ixpcom;
	struct ixpsip_attach_args *sa = aux;
	int unit = sa->sa_index;

	isc->sc_iot = sa->sa_iot;
	sc->sc_iot = sa->sa_iot;
	sc->sc_baseaddr = sa->sa_addr;

	printf("\n");

	if (bus_space_map(sc->sc_iot, sa->sa_addr, sa->sa_size, 0,
			 &sc->sc_ioh)) {
		printf("%s: unable to map device\n", sc->sc_dev.dv_xname);
		return;
	}

	ixp425_intr_establish(comcn_config[unit].intr, IPL_SERIAL, ixp4xxcomintr, sc);

	ixp4xx_com_attach_subr(sc);
}

int
ixdp_ixp4xx_comcnattach(bus_space_tag_t iot, int unit, int rate,
    int frequency, tcflag_t cflag)
{
	return ixp4xx_comcnattach(iot, comcn_config + unit,
		rate, frequency, cflag);
}
