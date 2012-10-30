/*	$NetBSD: auxreg.c,v 1.39.2.1 2012/10/30 17:20:22 yamt Exp $ */

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
 * 3. Neither the name of the University nor the names of its contributors
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: auxreg.c,v 1.39.2.1 2012/10/30 17:20:22 yamt Exp $");

#include "opt_blink.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/device.h>
#include <sys/kernel.h>

#include <machine/autoconf.h>

#include <sparc/sparc/vaddrs.h>
#include <sparc/sparc/auxreg.h>

volatile u_char *auxio_reg;
u_char auxio_regval;

static int auxregmatch_mainbus(device_t, cfdata_t, void *);
static int auxregmatch_obio(device_t, cfdata_t, void *);
static void auxregattach_mainbus(device_t, device_t, void *);
static void auxregattach_obio(device_t, device_t, void *);

static void auxregattach(void);

CFATTACH_DECL_NEW(auxreg_mainbus, 0,
    auxregmatch_mainbus, auxregattach_mainbus, NULL, NULL);

CFATTACH_DECL_NEW(auxreg_obio, 0,
    auxregmatch_obio, auxregattach_obio, NULL, NULL);

#ifdef BLINK
static callout_t blink_ch;

static void blink(void *);

static void
blink(void *zero)
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
auxregmatch_mainbus(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	return (strcmp("auxiliary-io", ma->ma_name) == 0);
}

static int
auxregmatch_obio(device_t parent, cfdata_t cf, void *aux)
{
	union obio_attach_args *uoba = aux;

	if (uoba->uoba_isobio4 != 0)
		return (0);

	return (strcmp("auxio", uoba->uoba_sbus.sa_name) == 0);
}

/* ARGSUSED */
static void
auxregattach_mainbus(device_t parent, device_t self, void *aux)
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
	auxregattach();
}

static void
auxregattach_obio(device_t parent, device_t self, void *aux)
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
	auxregattach();
}

static void
auxregattach(void)
{

	printf("\n");
#ifdef BLINK
	callout_init(&blink_ch, 0);
	blink((void *)0);
#else
	LED_ON;
#endif
}

unsigned int
auxregbisc(int bis, int bic)
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
