/*	$NetBSD: g2bus.c,v 1.8 2003/07/15 01:31:38 lukem Exp $	*/

/*-
 * Copyright (c) 2001 Marcus Comstedt
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
 *	This product includes software developed by Marcus Comstedt.
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


#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: g2bus.c,v 1.8 2003/07/15 01:31:38 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/proc.h>

#include <dreamcast/dev/g2/g2busvar.h>

int	g2busmatch(struct device *, struct cfdata *, void *);
void	g2busattach(struct device *, struct device *, void *);
int	g2busprint(void *, const char *);

CFATTACH_DECL(g2bus, sizeof(struct g2bus_softc),
    g2busmatch, g2busattach, NULL, NULL);

int	g2bussearch(struct device *, struct cfdata *, void *);

int
g2busmatch(struct device *parent, struct cfdata *cf, void *aux)
{

        return (1);
}

void
g2busattach(struct device *parent, struct device *self, void *aux)
{
	struct g2bus_softc *sc = (void *) self;
	struct g2bus_attach_args ga;

	printf("\n");

	TAILQ_INIT(&sc->sc_subdevs);

	g2bus_bus_mem_init(sc);

	ga.ga_memt = &sc->sc_memt;

	config_search(g2bussearch, self, &ga);
}

int
g2busprint(void *aux, const char *pnp)
{

	return (UNCONF);
}

int
g2bussearch(struct device *parent, struct cfdata *cf, void *aux)
{

	if (config_match(parent, cf, aux) > 0)
		config_attach(parent, cf, aux, g2busprint);

	return (0);
}
