/* $NetBSD: pccons_isa.c,v 1.1.10.3 2002/02/28 04:07:15 nathanw Exp $ */
/* NetBSD: vga_isa.c,v 1.4 2000/08/14 20:14:51 thorpej Exp  */

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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <dev/isa/isavar.h>

#include <arc/dev/pcconsvar.h>
#include <arc/isa/pccons_isavar.h>

int	pccons_isa_match __P((struct device *, struct cfdata *, void *));
void	pccons_isa_attach __P((struct device *, struct device *, void *));

struct cfattach pc_isa_ca = {
	sizeof(struct pc_softc), pccons_isa_match, pccons_isa_attach,
};

struct pccons_config *pccons_isa_conf;

int
pccons_isa_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct isa_attach_args *ia = aux;
	bus_addr_t iobase = 0x3b0;	/* XXX mono 0x3b0 color 0x3c0 */
	bus_size_t iosize = 0x30;	/* XXX 0x20 */
	bus_addr_t maddr = 0xa0000;
	bus_size_t msize = 0x20000;
	int irq = 1;

	if (ia->ia_nio < 1)
		return (0);
	if (ia->ia_io[0].ir_addr != ISACF_PORT_DEFAULT)
		iobase = ia->ia_io[0].ir_addr;
#if 0	/* XXX isa.c */
	if (ia->ia_iosize != 0)
		iosize = ia->ia_iosize;
#endif
	if (ia->ia_niomem < 1)
		return (0);
	if (ia->ia_iomem[0].ir_addr != ISACF_IOMEM_DEFAULT)
		maddr = ia->ia_iomem[0].ir_addr;
	if (ia->ia_iomem[0].ir_size != ISACF_IOSIZ_DEFAULT)
		msize = ia->ia_iomem[0].ir_size;

	if (ia->ia_nirq < 1)
		return (0);
	if (ia->ia_irq[0].ir_irq != ISACF_IRQ_DEFAULT)
		irq = ia->ia_irq[0].ir_irq;

#if 0
	/* If values are hardwired to something that they can't be, punt. */
	if (iobase != 0x3b0 || iosize != 0x30 ||
	    maddr != 0xa0000 || msize != 0x20000 ||
	    ia->ia_irq != 1 || ia->ia_drq != DRQUNK)
		return (0);
#endif

	if (pccons_isa_conf == NULL)
		return (0);

	if (!pccons_common_match(ia->ia_iot, ia->ia_memt, ia->ia_iot,
	    pccons_isa_conf))
		return (0);

	ia->ia_nio = 1;
	ia->ia_io[0].ir_addr = iobase;
	ia->ia_io[0].ir_size = iosize;

	ia->ia_niomem = 1;
	ia->ia_iomem[0].ir_addr = maddr;
	ia->ia_iomem[0].ir_size = msize;

	ia->ia_nirq = 1;
	ia->ia_irq[0].ir_irq = irq;

	ia->ia_ndrq = 0;

	return (1);
}

void
pccons_isa_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct pc_softc *sc = (struct pc_softc *)self;
	struct isa_attach_args *ia = aux;

	isa_intr_establish(ia->ia_ic, ia->ia_irq[0].ir_irq, IST_EDGE, IPL_TTY,
	    pcintr, self);
	pccons_common_attach(sc, ia->ia_iot, ia->ia_memt, ia->ia_iot,
	    pccons_isa_conf);
}

int
pccons_isa_cnattach(iot, memt)
	bus_space_tag_t iot, memt;
{
	if (pccons_isa_conf == NULL)
		return (ENXIO);

	pccons_common_cnattach(iot, memt, iot, pccons_isa_conf);
	return (0);
}
