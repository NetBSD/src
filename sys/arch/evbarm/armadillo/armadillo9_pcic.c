/*	$NetBSD: armadillo9_pcic.c,v 1.2 2012/10/27 17:17:46 chs Exp $	*/

/*
 * Copyright (c) 2005 HAMAJIMA Katsuomi. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: armadillo9_pcic.c,v 1.2 2012/10/27 17:17:46 chs Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <arm/ep93xx/epsocvar.h> 
#include <arm/ep93xx/epgpiovar.h> 
#include <arm/ep93xx/eppcicvar.h> 

#ifdef A9PCIC_DEBUG
int armadillo9pcic_debug = A9PCIC_DEBUG;
#define DPRINTFN(n,x)	if (armadillo9pcic_debug>(n)) printf x;
#else
#define DPRINTFN(n,x)   
#endif  

static int armadillo9pcic_match(device_t, cfdata_t, void *);
static void armadillo9pcic_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(armadillo9pcic, 0,
    armadillo9pcic_match, armadillo9pcic_attach, NULL, NULL);

static int armadillo9pcic_card_mode(void *, int);
static int armadillo9pcic_power_capability(void *, int);
static int armadillo9pcic_power_ctl(void *, int, int);

struct eppcic_chipset_tag armadillo9pcic_tag = {
	armadillo9pcic_card_mode,
	armadillo9pcic_power_capability,
	armadillo9pcic_power_ctl
};

static int
armadillo9pcic_match(device_t parent, cfdata_t match, void *aux)
{
	return 1;
}

static void
armadillo9pcic_attach(device_t parent, device_t self, void *aux)
{
	struct epsoc_attach_args *sa = aux;

	epgpio_out(sa->sa_gpio, PORT_A, 3);
	eppcic_attach_common(parent, self, aux, &armadillo9pcic_tag);
}

static int
armadillo9pcic_card_mode(void *self, int socket)
{
	if (socket == 0)
		return CF_MODE;
	else
		return SLOT_DISABLE;
}

static int
armadillo9pcic_power_capability(void *self, int socket)
{
	int vcc = 0;

	if (socket == 0)
		vcc |= VCC_3V;
	return vcc;
}

static int
armadillo9pcic_power_ctl(void *self, int socket, int onoff)
{
	struct eppcic_softc *sc = device_private(self);

	DPRINTFN(1, ("armadillo9pcic_power_ctl: %s\n",onoff?"on":"off"));

	if (onoff)
		epgpio_clear(sc->sc_gpio, PORT_A, 3);
	else
		epgpio_set(sc->sc_gpio, PORT_A, 3);
	return (300 * 1000);
}
