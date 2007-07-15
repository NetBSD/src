/*	$NetBSD: ixdp425_led.c,v 1.1.60.1 2007/07/15 13:15:49 ad Exp $ */
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
__KERNEL_RCSID(0, "$NetBSD: ixdp425_led.c,v 1.1.60.1 2007/07/15 13:15:49 ad Exp $");

/*
 * LED support for IXDP425
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/callout.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <machine/bus.h>

#include <arm/xscale/ixp425_sipvar.h>
#include <arm/xscale/ixp425var.h>

static int	ixdpled_match(struct device *, struct cfdata *, void *);
static void	ixdpled_attach(struct device *, struct device *, void *);
static void	ixdpled_callout(void *);

extern void	ixp425_expbus_init(void);

struct ixdpled_softc {
	struct device		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_addr_t		sc_baseaddr;
	bus_space_handle_t	sc_pos;
	struct callout		sc_co;
};

CFATTACH_DECL(ixdpled, sizeof(struct ixdpled_softc),
    ixdpled_match, ixdpled_attach, NULL, NULL);

static int
ixdpled_match(struct device *parent, struct cfdata *match, void *aux)
{
	return (1);
}

static void
ixdpled_attach(struct device *parent, struct device *self, void *aux)
{
	struct ixdpled_softc*		sc = (struct ixdpled_softc*) self;
	struct ixpsip_attach_args*	sa = aux;

	printf("\n");

  	sc->sc_iot = sa->sa_iot;
  	sc->sc_baseaddr = sa->sa_addr;

	if(bus_space_map(sc->sc_iot, sa->sa_addr, sa->sa_size, 0,
			 &sc->sc_ioh)) {
		printf("%s: unable to map registers\n", self->dv_xname);
		return;
	}

	IXPREG(IXP425_EXP_VBASE + EXP_TIMING_CS2_OFFSET) =
		IXP425_EXP_ADDR_T(3) | IXP425_EXP_SETUP_T(3) |
		IXP425_EXP_STROBE_T(15) | IXP425_EXP_HOLD_T(3) |
		IXP425_EXP_RECOVERY_T(15) | EXP_SZ_512 | EXP_WR_EN |
		IXP425_EXP_CS_EN;

	callout_init(&sc->sc_co, 0);
        callout_reset(&sc->sc_co, hz / 5, ixdpled_callout, sc);

}

static void
ixdpled_callout(void *arg)
{
	struct ixdpled_softc*	sc = arg;

	bus_space_write_2(sc->sc_iot, sc->sc_ioh, 0,
			  0x1000 + (sc->sc_pos++ % 16));

        callout_reset(&sc->sc_co, hz / 5, ixdpled_callout, sc);
}

