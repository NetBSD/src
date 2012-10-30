/*	$NetBSD: ixdp425_led.c,v 1.5.2.1 2012/10/30 17:19:24 yamt Exp $ */
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
__KERNEL_RCSID(0, "$NetBSD: ixdp425_led.c,v 1.5.2.1 2012/10/30 17:19:24 yamt Exp $");

/*
 * LED support for IXDP425
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/callout.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <sys/bus.h>

#include <arm/xscale/ixp425_sipvar.h>
#include <arm/xscale/ixp425var.h>

static int	ixdpled_match(device_t, cfdata_t, void *);
static void	ixdpled_attach(device_t, device_t, void *);
static void	ixdpled_callout(void *);

extern void	ixp425_expbus_init(void);

struct ixdpled_softc {
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_addr_t		sc_baseaddr;
	bus_space_handle_t	sc_pos;
	struct callout		sc_co;
};

CFATTACH_DECL_NEW(ixdpled, sizeof(struct ixdpled_softc),
    ixdpled_match, ixdpled_attach, NULL, NULL);

static int
ixdpled_match(device_t parent, cfdata_t match, void *aux)
{
	return (1);
}

static void
ixdpled_attach(device_t parent, device_t self, void *aux)
{
	struct ixdpled_softc*		sc = device_private(self);
	struct ixpsip_attach_args*	sa = aux;

	printf("\n");

  	sc->sc_iot = sa->sa_iot;
  	sc->sc_baseaddr = sa->sa_addr;

	if(bus_space_map(sc->sc_iot, sa->sa_addr, sa->sa_size, 0,
			 &sc->sc_ioh)) {
		printf("%s: unable to map registers\n", device_xname(self));
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

