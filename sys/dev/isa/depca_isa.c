/*	$NetBSD: depca_isa.c,v 1.1 2000/08/11 02:27:10 thorpej Exp $	*/

/*-
 * Copyright (c) 1997, 1998, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum and by Jason R. Thorpe of the Numerical Aerospace
 * Simulation Facility, NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell and Rick Macklem.
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
 *	@(#)if_le.c	8.2 (Berkeley) 11/16/93
 */

#include "opt_inet.h"
#include "bpfilter.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/syslog.h>
#include <sys/socket.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/if_inarp.h>
#endif

#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/bus.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#include <dev/ic/lancereg.h>
#include <dev/ic/lancevar.h>
#include <dev/ic/am7990reg.h>
#include <dev/ic/am7990var.h>
#include <dev/ic/depcareg.h>
#include <dev/ic/depcavar.h>

int	depca_isa_probe(struct device *, struct cfdata *, void *);
void	depca_isa_attach(struct device *, struct device *, void *);

struct depca_isa_softc {
	struct depca_softc sc_depca;
	isa_chipset_tag_t sc_ic;
	int sc_irq;
};

struct cfattach depca_isa_ca = {
	sizeof(struct depca_isa_softc), depca_isa_probe, depca_isa_attach
};

void	*depca_isa_intr_establish(struct depca_softc *, struct lance_softc *);

int
depca_isa_probe(struct device *parent, struct cfdata *match, void *aux)
{
	struct isa_attach_args *ia = aux;
	bus_space_tag_t iot = ia->ia_iot;
	bus_space_handle_t ioh;
	bus_space_handle_t memh;
	int rv = 0;

	/* Disallow impossible i/o address. */
	if (ia->ia_iobase != 0x200 && ia->ia_iobase != 0x300)
		return (0);

	/* Map i/o space. */
	if (bus_space_map(iot, ia->ia_iobase, 16, 0, &ioh))
		return 0;

	if (ia->ia_maddr == MADDRUNK ||
	    (ia->ia_msize != 32*1024 && ia->ia_msize != 64*1024))
		goto bad;

	/* Map card RAM. */
	if (bus_space_map(ia->ia_memt, ia->ia_maddr, ia->ia_msize, 0, &memh))
		goto bad;

	/* Just needed to check mapability; don't need it anymore. */
	bus_space_unmap(ia->ia_memt, memh, ia->ia_msize);

	/* Stop the LANCE chip and put it in a known state. */
	bus_space_write_2(iot, ioh, DEPCA_RAP, LE_CSR0);
	bus_space_write_2(iot, ioh, DEPCA_RDP, LE_C0_STOP);
	delay(100);

	bus_space_write_2(iot, ioh, DEPCA_RAP, LE_CSR0);
	if (bus_space_read_2(iot, ioh, DEPCA_RDP) != LE_C0_STOP)
		goto bad;

	bus_space_write_2(iot, ioh, DEPCA_RAP, LE_CSR3);
	bus_space_write_2(iot, ioh, DEPCA_RDP, LE_C3_ACON);

	bus_space_write_1(iot, ioh, DEPCA_CSR, DEPCA_CSR_DUM);

	if (depca_readprom(iot, ioh, NULL))
		goto bad;

	ia->ia_iosize = 16;
	rv = 1;

 bad:
	bus_space_unmap(iot, ioh, 16);
	return (rv);
}

void
depca_isa_attach(struct device *parent, struct device *self, void *aux)
{
	struct depca_softc *sc = (void *) self;
	struct depca_isa_softc *isc = (void *) self;
	struct isa_attach_args *ia = aux;

	printf("\n");

	sc->sc_iot = ia->ia_iot;
	sc->sc_memt = ia->ia_memt;

	sc->sc_memsize = ia->ia_msize;

	if (bus_space_map(sc->sc_iot, ia->ia_iobase, ia->ia_iosize,
	    0, &sc->sc_ioh) != 0) {
		printf("%s: unable to map i/o space\n", sc->sc_dev.dv_xname);
		return;
	}
	if (bus_space_map(sc->sc_memt, ia->ia_maddr, ia->ia_msize,
	    0, &sc->sc_memh) != 0) {
		printf("%s: unable to map memory space\n", sc->sc_dev.dv_xname);
		return;
	}

	isc->sc_ic = ia->ia_ic;
	isc->sc_irq = ia->ia_irq;
	sc->sc_intr_establish = depca_isa_intr_establish;

	depca_attach(sc);
}

void *
depca_isa_intr_establish(struct depca_softc *parent, struct lance_softc *child)
{
	struct depca_isa_softc *isc = (void *) parent;
	void *rv;

	rv = isa_intr_establish(isc->sc_ic, isc->sc_irq, IST_EDGE,
	    IPL_NET, depca_intredge, child);
	if (rv == NULL)
		printf("%s: unable to establish interrupt\n",
		    parent->sc_dev.dv_xname);

	return (rv);
}
