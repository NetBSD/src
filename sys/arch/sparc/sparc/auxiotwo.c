/*	$NetBSD: auxiotwo.c,v 1.2 2000/03/14 21:18:27 jdc Exp $	*/

/*
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julian Coleman.
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

/*
 * Based on software developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>

#include <machine/autoconf.h>

#include <sparc/include/tctrl.h>
#include <sparc/sparc/auxiotwo.h>
#include <sparc/sparc/vaddrs.h>

static int serial_refcount;
static int serial_power;

static int auxiotwomatch __P((struct device *, struct cfdata *, void *));

static void auxiotwoattach
		__P((struct device *, struct device *, void *));

struct cfattach auxiotwo_obio_ca = {
	sizeof(struct device), auxiotwomatch, auxiotwoattach
};

/*
 * The OPENPROM calls this "auxio2".
 */
static int
auxiotwomatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	union obio_attach_args *uoba = aux;

	if (uoba->uoba_isobio4 != 0)
		return (0);

	return (strcmp("auxio2", uoba->uoba_sbus.sa_name) == 0);
}

static void
auxiotwoattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	union obio_attach_args *uoba = aux;
	struct sbus_attach_args *sa = &uoba->uoba_sbus;
	bus_space_handle_t bh;

	if (sbus_bus_map(sa->sa_bustag,
			 sa->sa_slot, sa->sa_offset,
			 sizeof(long),
			 BUS_SPACE_MAP_LINEAR,
			 0, &bh) != 0) {
		printf("auxiotwoattach: can't map register\n");
		return;
	}

	auxiotwo_reg = (volatile u_char *)bh;
	auxiotwo_regval = *auxiotwo_reg;
	serial_refcount = 0;
	serial_power = PORT_PWR_STANDBY;
	printf("\n");
}

unsigned int
auxiotwobisc(bis, bic)
	int bis, bic;
{
	register int s;

	if (auxiotwo_reg == 0)
		/*
		 * Most machines do not have an `aux2' register; devices that
		 * depend on it should not get configured if it's absent.
		 */
		panic("no aux2 register");

	s = splhigh();
	auxiotwo_regval = (auxiotwo_regval | bis) & ~bic;
	*auxiotwo_reg = auxiotwo_regval;
	splx(s);
	return (auxiotwo_regval);
}

/*
 * Serial port state - called from zs_enable()/zs_disable()
 */
void
auxiotwoserialendis (state)
	int state;
{
	switch (state) {

	case ZS_ENABLE:
		/* Power on the serial ports? */
		serial_refcount++;
		if (serial_refcount == 1 && serial_power == PORT_PWR_STANDBY)
			auxiotwobisc(AUXIOTWO_SON, 0);
		break;
	case ZS_DISABLE:
		/* Power off the serial ports? */
		serial_refcount--;
		if (!serial_refcount && serial_power == PORT_PWR_STANDBY)
			auxiotwobisc(AUXIOTWO_SOF, 1);
		break;
	}
}

/*
 * Set power management - called by tctrl
 */
void
auxiotwoserialsetapm (state)
	int state;
{
	switch (state) {

	case PORT_PWR_ON:
		/* Change to: power always on */
		if (serial_power == PORT_PWR_OFF ||
		    (serial_power == PORT_PWR_STANDBY && !serial_refcount))
			auxiotwobisc(AUXIOTWO_SON, 0);
		serial_power = PORT_PWR_ON;
		break;

	case PORT_PWR_STANDBY:
		/* Change to: power on if open */
		if (serial_power == PORT_PWR_ON && !serial_refcount)
			auxiotwobisc(AUXIOTWO_SOF, 1);
		if (serial_power == PORT_PWR_OFF && serial_refcount)
			auxiotwobisc(AUXIOTWO_SON, 0);
		serial_power = PORT_PWR_STANDBY;
		break;

	case PORT_PWR_OFF:
		/* Change to: power always off */
		if (serial_power == PORT_PWR_ON ||
		    (serial_power == PORT_PWR_STANDBY && serial_refcount))
			auxiotwobisc(AUXIOTWO_SOF, 1);
		serial_power = PORT_PWR_OFF;
		break;
	}
}

/*
 * Get power management - called by tctrl
 */
int
auxiotwoserialgetapm ()
{
	return (serial_power);
}
