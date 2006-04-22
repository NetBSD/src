/*	$NetBSD: armadillo9_com.c,v 1.2.10.2 2006/04/22 11:37:21 simonb Exp $ */
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
__KERNEL_RCSID(0, "$NetBSD: armadillo9_com.c,v 1.2.10.2 2006/04/22 11:37:21 simonb Exp $");

/* Front-end of epcom */

#include <sys/types.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/param.h>
#include <sys/malloc.h>

#include <sys/termios.h>

#include <machine/intr.h>
#include <machine/bus.h>

#include <arm/ep93xx/epcomreg.h>
#include <arm/ep93xx/epcomvar.h>
#include <arm/ep93xx/ep93xxreg.h>
#include <arm/ep93xx/ep93xxvar.h>
#include <arm/ep93xx/epsocvar.h>
#include "epled.h"
#if NEPLED > 0
#include <arm/ep93xx/epledvar.h>
#endif
#include <evbarm/armadillo/armadillo9var.h>

static int armadillo9com_match(struct device *, struct cfdata *, void *);
static void armadillo9com_attach(struct device *, struct device *, void *);
static int armadillo9com_intr(void *);

CFATTACH_DECL(armadillo9com, sizeof(struct epcom_softc),
    armadillo9com_match, armadillo9com_attach, NULL, NULL);

static int
armadillo9com_match(struct device *parent, struct cfdata *match, void *aux)
{
	if (strcmp(match->cf_name, "epcom") == 0)
		return 1;
	return 0;
}

static void
armadillo9com_attach(struct device *parent, struct device *self, void *aux)
{
	struct epcom_softc *sc = (struct epcom_softc *)self;
	struct epsoc_attach_args *sa = aux;
	u_int32_t pwrcnt;
	bus_space_handle_t ioh;

	sc->sc_iot = sa->sa_iot;
	sc->sc_hwbase = sa->sa_addr;

	printf("\n");

	bus_space_map(sa->sa_iot, sa->sa_addr, sa->sa_size, 0, &sc->sc_ioh);

	bus_space_map(sa->sa_iot, EP93XX_APB_HWBASE + EP93XX_APB_SYSCON,
		EP93XX_APB_SYSCON_SIZE, 0, &ioh);
	pwrcnt = bus_space_read_4(sa->sa_iot, ioh, EP93XX_SYSCON_PwrCnt);
	pwrcnt &= ~(PwrCnt_UARTBAUD);
	bus_space_write_4(sa->sa_iot, ioh, EP93XX_SYSCON_PwrCnt, pwrcnt);
	bus_space_unmap(sa->sa_iot, ioh, EP93XX_APB_SYSCON_SIZE);

	epcom_attach_subr(sc);
	ep93xx_intr_establish(sa->sa_intr, IPL_SERIAL, armadillo9com_intr, sc);
#if NEPLED > 0
	if (armadillo_model->devcfg == DEVCFG_ARMADILLO210)
		epled_red_off();
#endif
}

static int
armadillo9com_intr(void *arg)
{
	int n;

#if NEPLED > 0  
	if (armadillo_model->devcfg == DEVCFG_ARMADILLO210)
        	epled_red_on();
#endif
	n = epcomintr(arg);
#if NEPLED > 0
	if (armadillo_model->devcfg == DEVCFG_ARMADILLO210)
        	epled_red_off();  
#endif  
	return n;
}
