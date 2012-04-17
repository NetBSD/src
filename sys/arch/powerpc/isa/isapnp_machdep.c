/*	$NetBSD: isapnp_machdep.c,v 1.5.2.1 2012/04/17 00:06:47 yamt Exp $	*/

/*-
 * Copyright (c) 1996, 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center and by Christos Zoulas.
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
 * Copyright (c) 1997 Thomas Gerner.  All rights reserved.
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
 *
 * Note: We just use bus_space{,un}map() here.  This works because we
 * don't do space accounting (we'd need a back-door to bypass it
 * if we did!).
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: isapnp_machdep.c,v 1.5.2.1 2012/04/17 00:06:47 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/bus.h>

#include <dev/isa/isavar.h>

#include <dev/isapnp/isapnpreg.h>
#include <dev/isapnp/isapnpvar.h>

/* isapnp_map():
 *	Map I/O regions used by PnP
 */
int
isapnp_map(struct isapnp_softc *sc)
{
#ifdef _KERNEL
	int error;
#endif

#ifdef _KERNEL
	if ((error = bus_space_map(sc->sc_iot, ISAPNP_ADDR, 1, 0,
	     &sc->sc_addr_ioh)) != 0)
		return error;
	if ((error = bus_space_map(sc->sc_iot, ISAPNP_WRDATA, 1, 0,
	     &sc->sc_wrdata_ioh)) != 0)
		return error;
#endif
	return (0);
}

/* isapnp_unmap():
 *	Unmap I/O regions used by PnP
 */
void
isapnp_unmap(struct isapnp_softc *sc)
{

	bus_space_unmap(sc->sc_iot, sc->sc_addr_ioh, 1);
	bus_space_unmap(sc->sc_iot, sc->sc_wrdata_ioh, 1);
}

/* isapnp_map_readport():
 *	Called to map the PnP `read port', which is mapped independently
 *	of the `write' and `addr' ports.
 *
 *	NOTE: assumes the caller has filled in sc->sc_read_port!
 */
int
isapnp_map_readport(struct isapnp_softc *sc)
{
#ifdef _KERNEL
	int error;
#endif

#ifdef _KERNEL
	/* Check if some other device has already claimed this port. */
	if ((error = bus_space_map(sc->sc_iot, sc->sc_read_port, 1, 0,
	    &sc->sc_read_ioh)) != 0)
		return error;
#endif
	return 0;
}

/* isapnp_unmap_readport():
 *	Pretend to unmap a previously mapped `read port'.
 */
void
isapnp_unmap_readport(struct isapnp_softc *sc)
{

	bus_space_unmap(sc->sc_iot, sc->sc_read_ioh, 1);
}
