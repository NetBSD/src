/*	$NetBSD: g1bus.c,v 1.1.18.2 2017/12/03 11:36:00 jdolecek Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: g1bus.c,v 1.1.18.2 2017/12/03 11:36:00 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/proc.h>

#include <dreamcast/dev/g1/g1busvar.h>

int	g1busmatch(device_t, cfdata_t, void *);
void	g1busattach(device_t, device_t, void *);
int	g1busprint(void *, const char *);

CFATTACH_DECL_NEW(g1bus, sizeof(struct g1bus_softc),
    g1busmatch, g1busattach, NULL, NULL);

int	g1bussearch(device_t, cfdata_t, const int *, void *);

int
g1busmatch(device_t parent, cfdata_t cf, void *aux)
{

	return 1;
}

void
g1busattach(device_t parent, device_t self, void *aux)
{
	struct g1bus_softc *sc = device_private(self);
	struct g1bus_attach_args ga;

	sc->sc_dev = self;
	printf("\n");

	g1bus_bus_mem_init(sc);

	ga.ga_memt = &sc->sc_memt;

	config_search_ia(g1bussearch, self, "g1bus", &ga);
}

int
g1busprint(void *aux, const char *pnp)
{

	return UNCONF;
}

int
g1bussearch(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{

	if (config_match(parent, cf, aux) > 0)
		config_attach(parent, cf, aux, g1busprint);

	return 0;
}
