/* $NetBSD: opms_isa.c,v 1.5 2003/07/15 00:04:47 lukem Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: opms_isa.c,v 1.5 2003/07/15 00:04:47 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/tty.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#include <arc/dev/pcconsvar.h>
#include <arc/dev/opmsvar.h>

int	opms_isa_match __P((struct device *, struct cfdata *, void *));
void	opms_isa_attach __P((struct device *, struct device *, void *));

CFATTACH_DECL(opms_isa, sizeof(struct opms_softc),
    opms_isa_match, opms_isa_attach, NULL, NULL);

struct pccons_config *pccons_isa_conf;	/* share stroage with pccons_isa.c */

int
opms_isa_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct isa_attach_args *ia = aux;
	bus_addr_t iobase = IO_KBD;
	bus_size_t iosize = IO_KBDSIZE;
	int irq = 12;

	if (ia->ia_nio < 1)
		return (0);
	if (ia->ia_io[0].ir_addr != ISACF_PORT_DEFAULT)
		iobase = ia->ia_io[0].ir_addr;
#if 0	/* XXX isa.c */
	if (ia->ia_iosize != 0)
		iosize = ia->ia_iosize;
#endif
	if (ia->ia_irq[0].ir_irq != ISACF_IRQ_DEFAULT)
		irq = ia->ia_irq[0].ir_irq;

#if 0
	/* If values are hardwired to something that they can't be, punt. */
	if (iobase != IO_KBD || iosize != IO_KBDSIZE ||
	    ia->ia_maddr != MADDRUNK || ia->ia_msize != 0 ||
	    ia->ia_irq != 1 || ia->ia_drq != DRQUNK)
		return (0);
#endif

	if (pccons_isa_conf == NULL)
		return (0);

	if (!opms_common_match(ia->ia_iot, pccons_isa_conf))
		return (0);

	ia->ia_nio = 1;
	ia->ia_io[0].ir_addr = iobase;
	ia->ia_io[0].ir_size = iosize;

	ia->ia_nirq = 1;
	ia->ia_irq[0].ir_irq = irq;

	ia->ia_niomem = 0;
	ia->ia_ndrq = 0;

	return (1);
}

void
opms_isa_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct opms_softc *sc = (struct opms_softc *)self;
	struct isa_attach_args *ia = aux;

	printf("\n");

	isa_intr_establish(ia->ia_ic, ia->ia_irq[0].ir_irq, IST_EDGE, IPL_TTY,
	    pcintr, self);
	opms_common_attach(sc, ia->ia_iot, pccons_isa_conf);
}
