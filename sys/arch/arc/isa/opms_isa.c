/* $NetBSD: opms_isa.c,v 1.10.16.1 2011/04/21 01:40:50 rmind Exp $ */

/*
 * Copyright (c) 1995, 1996 Carnegie-Mellon University.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: opms_isa.c,v 1.10.16.1 2011/04/21 01:40:50 rmind Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/tty.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#include <arc/dev/pcconsvar.h>
#include <arc/dev/opmsvar.h>

static int	opms_isa_match(device_t, cfdata_t, void *);
static void	opms_isa_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(opms_isa, sizeof(struct opms_softc),
    opms_isa_match, opms_isa_attach, NULL, NULL);

struct pccons_config *pccons_isa_conf;	/* share stroage with pccons_isa.c */

static int
opms_isa_match(device_t parent, cfdata_t cf, void *aux)
{
	struct isa_attach_args *ia = aux;
	bus_addr_t iobase = IO_KBD;
	bus_size_t iosize = IO_KBDSIZE;
	int irq = 12;

	if (ia->ia_nio < 1)
		return 0;
	if (ia->ia_io[0].ir_addr != ISA_UNKNOWN_PORT)
		iobase = ia->ia_io[0].ir_addr;
#if 0	/* XXX isa.c */
	if (ia->ia_iosize != 0)
		iosize = ia->ia_iosize;
#endif
	if (ia->ia_irq[0].ir_irq != ISA_UNKNOWN_IRQ)
		irq = ia->ia_irq[0].ir_irq;

#if 0
	/* If values are hardwired to something that they can't be, punt. */
	if (iobase != IO_KBD || iosize != IO_KBDSIZE ||
	    ia->ia_maddr != MADDRUNK || ia->ia_msize != 0 ||
	    ia->ia_irq != 1 || ia->ia_drq != DRQUNK)
		return 0;
#endif

	if (pccons_isa_conf == NULL)
		return 0;

	if (!opms_common_match(ia->ia_iot, pccons_isa_conf))
		return 0;

	ia->ia_nio = 1;
	ia->ia_io[0].ir_addr = iobase;
	ia->ia_io[0].ir_size = iosize;

	ia->ia_nirq = 1;
	ia->ia_irq[0].ir_irq = irq;

	ia->ia_niomem = 0;
	ia->ia_ndrq = 0;

	return 1;
}

static void
opms_isa_attach(device_t parent, device_t self, void *aux)
{
	struct opms_softc *sc = device_private(self);
	struct isa_attach_args *ia = aux;

	sc->sc_dev = self;

	aprint_normal("\n");

	isa_intr_establish(ia->ia_ic, ia->ia_irq[0].ir_irq, IST_EDGE, IPL_TTY,
	    pcintr, sc);
	opms_common_attach(sc, ia->ia_iot, pccons_isa_conf);
}
