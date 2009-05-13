/*	$NetBSD: ixpcom_ixm.c,v 1.6.122.1 2009/05/13 17:16:39 jym Exp $ */
/*
 * Copyright (c) 2002
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
__KERNEL_RCSID(0, "$NetBSD: ixpcom_ixm.c,v 1.6.122.1 2009/05/13 17:16:39 jym Exp $");

/* Front-end of ixpcom */

#include <sys/types.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/param.h>
#include <sys/malloc.h>

#include <sys/termios.h>

#include <machine/intr.h>
#include <machine/bus.h>

#include <arm/ixp12x0/ixp12x0_comreg.h>
#include <arm/ixp12x0/ixp12x0_comvar.h>
#include <arm/ixp12x0/ixp12x0reg.h>
#include <arm/ixp12x0/ixp12x0var.h>
#include <arm/ixp12x0/ixpsipvar.h>

#include <evbarm/ixm1200/ixpcom_ixmvar.h>

static int	ixpcom_ixm_match(struct device *, struct cfdata *, void *);
static void	ixpcom_ixm_attach(struct device *, struct device *, void *);

CFATTACH_DECL(ixpcom_ixm, sizeof(struct ixpcom_softc),
    ixpcom_ixm_match, ixpcom_ixm_attach, NULL, NULL);

static int
ixpcom_ixm_match(struct device *parent, struct cfdata *match, void *aux)
{
	if (strcmp(match->cf_name, "ixpcom") == 0)
		return 1;
	return 0;
}

static void
ixpcom_ixm_attach(parent, self, aux)
	struct device *parent;
	struct device *self;  
	void *aux;
{
	struct ixpcom_ixm_softc *isc = (struct ixpcom_ixm_softc *)self;
	struct ixpcom_softc *sc = &isc->sc_ixpcom;
	struct ixpsip_attach_args *sa = aux;

	isc->sc_iot = sa->sa_iot;
	sc->sc_iot = sa->sa_iot;
	sc->sc_baseaddr = sa->sa_addr;

	printf("\n");

	if (bus_space_map(sa->sa_iot, sa->sa_addr, sa->sa_size, 0,
			 &sc->sc_ioh)) {
		printf("%s: unable to map device\n", sc->sc_dev.dv_xname);
		return;
	}

	printf("%s: IXP12x0 UART\n", sc->sc_dev.dv_xname);

	ixpcom_attach_subr(sc);

#ifdef POLLING_COM
	{ void* d; d = d = ixpcomintr; }
#else
	ixp12x0_intr_establish(IXP12X0_INTR_UART, IPL_SERIAL, ixpcomintr, sc);
#endif
}
