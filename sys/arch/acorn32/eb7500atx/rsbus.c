/* $NetBSD: rsbus.c,v 1.2 2005/06/30 17:03:52 drochner Exp $ */

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

#include <sys/param.h>

__KERNEL_RCSID(0, "$NetBSD: rsbus.c,v 1.2 2005/06/30 17:03:52 drochner Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <acorn32/eb7500atx/rsbus.h>

#include "locators.h"

extern struct bus_space rsbus_bs_tag;

/* Declare prototypes */

static int	rsbus_match(struct device *, struct cfdata *, void *);
static void	rsbus_attach(struct device *, struct device *, void *);
static int	rsbus_print(void *, const char *);
static int	rsbus_search(struct device *, struct cfdata *,
			     const locdesc_t *, void *);

CFATTACH_DECL(rsbus, sizeof(struct rsbus_softc),
    rsbus_match, rsbus_attach, NULL, NULL);
 
static int
rsbus_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	return(1);
}

static void
rsbus_attach(struct device *parent, struct device *self, void *aux)
{
	struct rsbus_softc *sc = (void *) self;
	sc->sc_iot = &rsbus_bs_tag;

	printf("\n");

	/*
	 *  Attach each devices
	 */
	config_search_ia(rsbus_search, self, "rsbus", NULL);
}

static int
rsbus_search(parent, cf, ldesc, aux)
	struct device *parent;
	struct cfdata *cf;
	const locdesc_t *ldesc;
	void *aux;
{
	struct rsbus_softc *sc = (struct rsbus_softc *)parent;
	struct rsbus_attach_args sa;

	sa.sa_iot = sc->sc_iot;
	sa.sa_addr = cf->cf_loc[RSBUSCF_ADDR];
	sa.sa_size = cf->cf_loc[RSBUSCF_SIZE];
	sa.sa_intr = cf->cf_loc[RSBUSCF_IRQ];

	if (config_match(parent, cf, &sa) > 0)
		config_attach(parent, cf, &sa, rsbus_print);

	return (0);
}

static int
rsbus_print(aux, name)
	void *aux;
	const char *name;
{
        struct rsbus_attach_args *sa = (struct rsbus_attach_args*)aux;

	if (sa->sa_size)
		aprint_normal(" addr 0x%lx", sa->sa_addr);
	if (sa->sa_size > 1)
		aprint_normal("-0x%lx", sa->sa_addr + sa->sa_size - 1);
	if (sa->sa_intr > 1)
		aprint_normal(" irq %d", sa->sa_intr);

	return (UNCONF);
}

