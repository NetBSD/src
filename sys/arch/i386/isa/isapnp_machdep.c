/*	$NetBSD: isapnp_machdep.c,v 1.1.6.1 1997/03/12 14:34:51 is Exp $	*/

/*
 * Copyright (c) 1996 Jason R. Thorpe.  All rights reserved.
 * Copyright (c) 1996 Christos Zoulas.  All rights reserved.
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
 *	This product includes software developed by Christos Zoulas
 *	and Jason R. Thorpe.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Machine-dependent portions of ISA PnP bus autoconfiguration.
 *
 * N.B. This file exists mostly to get around some lameness surrounding
 * the PnP spec.  ISA PnP registers live where some `normal' ISA
 * devices do, but are e.g. write-only registers where the normal
 * device has a read-only register.  This breaks in the presence of
 * i/o port accounting.  This file takes care of mapping ISA PnP
 * registers without actually allocating them in extent maps.
 *
 * Since this is a machine-dependent file, we make all sorts of
 * assumptions about bus.h's guts.  Beware!
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/bus.h>

#include <dev/isa/isavar.h>

#include <dev/isapnp/isapnpreg.h>
#include <dev/isapnp/isapnpvar.h>

/* isapnp_map():
 *	Map I/O regions used by PnP
 */
int
isapnp_map(sc)
	struct isapnp_softc *sc;
{

	if (sc->sc_iot != I386_BUS_SPACE_IO)
		panic("isapnp_map: bogus bus space tag");

	sc->sc_addr_ioh = ISAPNP_ADDR;
	sc->sc_wrdata_ioh = ISAPNP_WRDATA;
	return (0);
}

/* isapnp_unmap():
 *	Unmap I/O regions used by PnP
 */
void
isapnp_unmap(sc)
	struct isapnp_softc *sc;
{

	/* Do nothing. */
}

/* isapnp_map_readport():
 *	Called to map the PnP `read port', which is mapped independently
 *	of the `write' and `addr' ports.
 *
 *	NOTE: assumes the caller has filled in sc->sc_read_port!
 */
int
isapnp_map_readport(sc)
	struct isapnp_softc *sc;
{
	if (sc->sc_iot != I386_BUS_SPACE_IO)
		panic("isapnp_map_readport: bogus bus space tag");

	/* Check if some other device has already claimed this port. */
	return bus_space_map(sc->sc_iot, sc->sc_read_port, 1, 0,
	    &sc->sc_read_ioh);
}

/* isapnp_unmap_readport():
 *	Unmap a previously mapped `read port'.
 */
void
isapnp_unmap_readport(sc)
	struct isapnp_softc *sc;
{
	bus_space_unmap(sc->sc_iot, sc->sc_read_ioh, 1);
}
