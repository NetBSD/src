/*	$NetBSD: iic_iomd.c,v 1.6 2003/07/15 00:24:44 lukem Exp $	*/

/*
 * Copyright (c) 1994-1996 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
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
 *	This product includes software developed by Mark Brinicombe.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * iic.c
 *
 * Routines to communicate with IIC devices
 *
 * Created      : 13/10/94
 *
 * Based of kate/display/iiccontrol.c
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: iic_iomd.c,v 1.6 2003/07/15 00:24:44 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/intr.h>
#include <arm/iomd/iicvar.h>
#include <arm/iomd/iomdvar.h>

static int  iic_iomd_probe  __P((struct device *, struct cfdata *, void *));
static void iic_iomd_attach __P((struct device *, struct device *, void *));

CFATTACH_DECL(iic_iomd, sizeof(struct iic_softc),
    iic_iomd_probe, iic_iomd_attach, NULL, NULL);

/*
 * iic device probe function
 *
 * just validate the attach args
 */

static int
iic_iomd_probe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct iic_attach_args *ia = aux;

	if (strcmp(ia->ia_name, "iic") == 0)
		return(1);

	return(0);
}

/*
 * iic device attach function
 *
 * Initialise the softc structure and do a search for children
 */

static void
iic_iomd_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct iic_softc *sc = (void *)self;
	struct iic_attach_args *ia = aux;

	sc->sc_iot = ia->ia_iot;
	sc->sc_ioh = ia->ia_ioh;
	
	printf("\n");

	config_search(iicsearch, self, NULL);
}

/* End of iic_iomd.c */
