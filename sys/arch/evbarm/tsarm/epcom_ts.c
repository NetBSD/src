/*	$NetBSD: epcom_ts.c,v 1.2.92.1 2009/05/13 17:16:39 jym Exp $ */
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
__KERNEL_RCSID(0, "$NetBSD: epcom_ts.c,v 1.2.92.1 2009/05/13 17:16:39 jym Exp $");

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

#include <evbarm/tsarm/epcom_tsvar.h>

static int	epcom_ts_match(struct device *, struct cfdata *, void *);
static void	epcom_ts_attach(struct device *, struct device *, void *);

CFATTACH_DECL(epcom_ts, sizeof(struct epcom_ts_softc),
    epcom_ts_match, epcom_ts_attach, NULL, NULL);

static int
epcom_ts_match(struct device *parent, struct cfdata *match, void *aux)
{
	if (strcmp(match->cf_name, "epcom") == 0)
		return 1;
	return 0;
}

static void
epcom_ts_attach(parent, self, aux)
	struct device *parent;
	struct device *self;  
	void *aux;
{
	struct epcom_ts_softc *esc = (struct epcom_ts_softc *)self;
	struct epcom_softc *sc = &esc->sc_epcom;
	struct epsoc_attach_args *sa = aux;
	u_int32_t pwrcnt;
	bus_space_handle_t ioh;

	esc->sc_iot = sa->sa_iot;
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
	ep93xx_intr_establish(sa->sa_intr, IPL_SERIAL, epcomintr, sc);
}
