/*	$NetBSD: attimer_isa.c,v 1.3 2006/10/12 01:31:16 christos Exp $	*/

/*
 *  Copyright (c) 2005 The NetBSD Foundation.
 *  All rights reserved.
 *
 *  This code is derived from software contributed to the NetBSD Foundation
 *   by Quentin Garnier.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. All advertising materials mentioning features or use of this software
 *     must display the following acknowledgement:
 *         This product includes software developed by the NetBSD
 *         Foundation, Inc. and its contributors.
 *  4. Neither the name of The NetBSD Foundation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 *  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 *  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

/*
 * ISA attachment for attimer(4)
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: attimer_isa.c,v 1.3 2006/10/12 01:31:16 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/device.h>
#include <sys/proc.h>

#include <machine/bus.h>

#include <dev/ic/attimervar.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

static int	attimer_isa_match(struct device *, struct cfdata *, void *);
static void	attimer_isa_attach(struct device *, struct device *, void *);

CFATTACH_DECL(attimer_isa, sizeof(struct attimer_softc),
    attimer_isa_match, attimer_isa_attach, NULL, NULL);

static int
attimer_isa_match(struct device *parent __unused, struct cfdata *match __unused,
    void *aux)
{
	struct isa_attach_args *ia = aux;
	bus_space_handle_t att_ioh;

	if (ISA_DIRECT_CONFIG(ia))
		return (0);

	/* If values are hardwired to something that they can't be, punt. */
	if (ia->ia_nio < 1 ||
	    (ia->ia_io[0].ir_addr != ISA_UNKNOWN_PORT &&
	    ia->ia_io[0].ir_addr != IO_TIMER1))
		return (0);

	if (ia->ia_niomem > 0 &&
	    (ia->ia_iomem[0].ir_addr != ISA_UNKNOWN_IOMEM))
		return (0);

	if (ia->ia_nirq > 0 &&
	    (ia->ia_irq[0].ir_irq != ISA_UNKNOWN_IRQ))
		return (0);

	if (ia->ia_ndrq > 0 &&
	    (ia->ia_drq[0].ir_drq != ISA_UNKNOWN_DRQ))
		return (0);

	if (bus_space_map(ia->ia_iot, IO_TIMER1, 4, 0, &att_ioh))
		return 0;
	bus_space_unmap(ia->ia_iot, att_ioh, 4);

	ia->ia_io[0].ir_addr = IO_TIMER1;
	ia->ia_io[0].ir_size = 4;
	ia->ia_nio = 1;

	ia->ia_niomem = 0;
	ia->ia_nirq = 0;
	ia->ia_ndrq = 0;

	return 1;
}

static void
attimer_isa_attach(struct device *parent __unused, struct device *self,
    void *aux)
{
	struct attimer_softc *sc = (struct attimer_softc *)self;
	struct isa_attach_args *ia = aux;

	sc->sc_iot = ia->ia_iot;

	aprint_normal(": AT Timer\n");

	if (bus_space_map(sc->sc_iot, IO_TIMER1, 4, 0, &sc->sc_ioh) != 0)
		aprint_error("%s: could not map registers\n",
		    sc->sc_dev.dv_xname);
	else
		attimer_attach(sc);
}
