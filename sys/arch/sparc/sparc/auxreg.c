/*	$NetBSD: auxreg.c,v 1.28.4.1 2002/03/16 15:59:50 jdolecek Exp $ */

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)auxreg.c	8.1 (Berkeley) 6/11/93
 */

#include "opt_blink.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/device.h>
#include <sys/kernel.h>

#include <machine/autoconf.h>

#include <sparc/sparc/vaddrs.h>
#include <sparc/sparc/auxreg.h>

static int auxregmatch_mainbus __P((struct device *, struct cfdata *, void *));
static int auxregmatch_obio __P((struct device *, struct cfdata *, void *));
static void auxregattach_mainbus
		__P((struct device *, struct device *, void *));
static void auxregattach_obio
		__P((struct device *, struct device *, void *));

static void auxregattach __P((struct device *));

struct cfattach auxreg_mainbus_ca = {
	sizeof(struct device), auxregmatch_mainbus, auxregattach_mainbus
};
struct cfattach auxreg_obio_ca = {
	sizeof(struct device), auxregmatch_obio, auxregattach_obio
};

#ifdef BLINK
static struct callout blink_ch = CALLOUT_INITIALIZER;

static void blink __P((void *zero));

static void
blink(zero)
	void *zero;
{
	register int s;

	s = splhigh();
	LED_FLIP;
	splx(s);
	/*
	 * Blink rate is:
	 *	full cycle every second if completely idle (loadav = 0)
	 *	full cycle every 2 seconds if loadav = 1
	 *	full cycle every 3 seconds if loadav = 2
	 * etc.
	 */
	s = (((averunnable.ldavg[0] + FSCALE) * hz) >> (FSHIFT + 1));
	callout_reset(&blink_ch, s, blink, NULL);
}
#endif

/*
 * The OPENPROM calls this "auxiliary-io" (sun4c) or "auxio" (sun4m).
 */
static int
auxregmatch_mainbus(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct mainbus_attach_args *ma = aux;

	return (strcmp("auxiliary-io", ma->ma_name) == 0);
}

static int
auxregmatch_obio(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	union obio_attach_args *uoba = aux;

	if (uoba->uoba_isobio4 != 0)
		return (0);

	return (strcmp("auxio", uoba->uoba_sbus.sa_name) == 0);
}

/* ARGSUSED */
static void
auxregattach_mainbus(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct mainbus_attach_args *ma = aux;
	bus_space_handle_t bh;

	if (bus_space_map2(ma->ma_bustag,
			  (bus_addr_t)ma->ma_paddr,
			  sizeof(long),
			  BUS_SPACE_MAP_LINEAR,
			  AUXREG_VA,
			  &bh) != 0) {
		printf("auxregattach_mainbus: can't map register\n");
		return;
	}

	auxio_reg = AUXIO4C_REG;
	auxio_regval = *AUXIO4C_REG | AUXIO4C_FEJ | AUXIO4C_MB1;
	auxregattach(self);
}

static void
auxregattach_obio(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	union obio_attach_args *uoba = aux;
	struct sbus_attach_args *sa = &uoba->uoba_sbus;
	bus_space_handle_t bh;

	if (bus_space_map2(sa->sa_bustag,
			  BUS_ADDR(sa->sa_slot, sa->sa_offset),
			  sizeof(long),
			  BUS_SPACE_MAP_LINEAR,
			  AUXREG_VA, &bh) != 0) {
		printf("auxregattach_obio: can't map register\n");
		return;
	}

	auxio_reg = AUXIO4M_REG;
	auxio_regval = *AUXIO4M_REG | AUXIO4M_MB1;
	auxregattach(self);
}

static void
auxregattach(self)
	struct device *self;
{

	printf("\n");
#ifdef BLINK
	blink((caddr_t)0);
#else
	LED_ON;
#endif
}

unsigned int
auxregbisc(bis, bic)
	int bis, bic;
{
	register int s;

	if (auxio_reg == 0)
		/*
		 * Not all machines have an `aux' register; devices that
		 * depend on it should not get configured if it's absent.
		 */
		panic("no aux register");

	s = splhigh();
	auxio_regval = (auxio_regval | bis) & ~bic;
	*auxio_reg = auxio_regval;
	splx(s);
	return (auxio_regval);
}
