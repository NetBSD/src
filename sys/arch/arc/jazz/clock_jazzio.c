/*	$NetBSD: clock_jazzio.c,v 1.1.2.2 2001/01/05 17:33:57 bouyer Exp $	*/
/*	$OpenBSD: clock.c,v 1.6 1998/10/15 21:30:15 imp Exp $	*/

/*
 * Copyright (c) 1997 Per Fogelstrom.
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department and Ralph Campbell.
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
 * from: Utah Hdr: clock.c 1.18 91/01/21
 *
 *	from: @(#)clock.c	8.1 (Berkeley) 6/10/93
 */

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <arc/arc/clockvar.h>
#include <arc/arc/arctype.h>
#include <arc/jazz/jazziovar.h>

#include <dev/isa/isavar.h>
#include <machine/isa_machdep.h>

int	clock_found = 0;
extern int	clock_started;

/* Definition of the driver for autoconfig. */
static int	clock_jazzio_match(struct device *, struct cfdata *, void *);
static void	clock_jazzio_attach(struct device *, struct device *, void *);

extern struct cfdriver aclock_cd;

struct cfattach aclock_jazzio_ca = {
	sizeof(struct clock_softc), clock_jazzio_match, clock_jazzio_attach
};

void	mcclock_attach(struct device *, struct device *, void *);
int	clockintr(void *);

static int
clock_jazzio_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct jazzio_attach_args *ja = aux;

	/* See how many clocks this system has */	
	switch (cputype) {
	case ACER_PICA_61:
	case MAGNUM:
	case NEC_R94:
	case NEC_RAx94:
	case NEC_RD94:
	case NEC_R96:
		/* make sure that we're looking for this type of device. */
		if (strcmp(ja->ja_name, "dallas_rtc") != 0)
			return (0);

		break;

	default:
		panic("unknown CPU");
	}

	if (clock_found)
		return (0);

	return (1);
}

static void
clock_jazzio_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct jazzio_attach_args *ja = aux;

	mcclock_attach(parent, self, aux);

	switch (cputype) {

	case ACER_PICA_61:
	case NEC_R94:
	case NEC_RAx94:
	case NEC_RD94:
	case NEC_R96:
		jazzio_intr_establish(ja->ja_intr,
			(intr_handler_t)hardclock, self);
		break;

	case MAGNUM:
		jazzio_intr_establish(ja->ja_intr,
			(intr_handler_t)clockintr, self);
		break;

	default:
		panic("clockattach: it didn't get here.  really.");
	}

	printf("\n");

	clock_found = 1;
}
