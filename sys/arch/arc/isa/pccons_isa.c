/* $NetBSD: pccons_isa.c,v 1.9 2008/07/05 08:46:25 tsutsui Exp $ */
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pccons_isa.c,v 1.9 2008/07/05 08:46:25 tsutsui Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <dev/isa/isavar.h>

#include <arc/dev/pcconsvar.h>
#include <arc/isa/pccons_isavar.h>

static int	pccons_isa_match(device_t, cfdata_t, void *);
static void	pccons_isa_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(pc_isa, sizeof(struct pc_softc),
    pccons_isa_match, pccons_isa_attach, NULL, NULL);

struct pccons_config *pccons_isa_conf;

static int
pccons_isa_match(device_t parent, cfdata_t cf, void *aux)
{
	struct isa_attach_args *ia = aux;
	bus_addr_t iobase = 0x3b0;	/* XXX mono 0x3b0 color 0x3c0 */
	bus_size_t iosize = 0x30;	/* XXX 0x20 */
	bus_addr_t maddr = 0xa0000;
	bus_size_t msize = 0x20000;
	int irq = 1;

	if (ia->ia_nio < 1)
		return 0;
	if (ia->ia_io[0].ir_addr != ISA_UNKNOWN_PORT)
		iobase = ia->ia_io[0].ir_addr;
#if 0	/* XXX isa.c */
	if (ia->ia_iosize != 0)
		iosize = ia->ia_iosize;
#endif
	if (ia->ia_niomem < 1)
		return 0;
	if (ia->ia_iomem[0].ir_addr != ISA_UNKNOWN_IOMEM)
		maddr = ia->ia_iomem[0].ir_addr;
	if (ia->ia_iomem[0].ir_size != ISA_UNKNOWN_IOSIZ)
		msize = ia->ia_iomem[0].ir_size;

	if (ia->ia_nirq < 1)
		return 0;
	if (ia->ia_irq[0].ir_irq != ISA_UNKNOWN_IRQ)
		irq = ia->ia_irq[0].ir_irq;

#if 0
	/* If values are hardwired to something that they can't be, punt. */
	if (iobase != 0x3b0 || iosize != 0x30 ||
	    maddr != 0xa0000 || msize != 0x20000 ||
	    ia->ia_irq != 1 || ia->ia_drq != DRQUNK)
		return 0;
#endif

	if (pccons_isa_conf == NULL)
		return 0;

	if (!pccons_common_match(ia->ia_iot, ia->ia_memt, ia->ia_iot,
	    pccons_isa_conf))
		return 0;

	ia->ia_nio = 1;
	ia->ia_io[0].ir_addr = iobase;
	ia->ia_io[0].ir_size = iosize;

	ia->ia_niomem = 1;
	ia->ia_iomem[0].ir_addr = maddr;
	ia->ia_iomem[0].ir_size = msize;

	ia->ia_nirq = 1;
	ia->ia_irq[0].ir_irq = irq;

	ia->ia_ndrq = 0;

	return 1;
}

static void
pccons_isa_attach(device_t parent, device_t self, void *aux)
{
	struct pc_softc *sc = device_private(self);
	struct isa_attach_args *ia = aux;

	sc->sc_dev = self;

	isa_intr_establish(ia->ia_ic, ia->ia_irq[0].ir_irq, IST_EDGE, IPL_TTY,
	    pcintr, self);
	pccons_common_attach(sc, ia->ia_iot, ia->ia_memt, ia->ia_iot,
	    pccons_isa_conf);
}

int
pccons_isa_cnattach(bus_space_tag_t iot, bus_space_tag_t memt)
{

	if (pccons_isa_conf == NULL)
		return ENXIO;

	pccons_common_cnattach(iot, memt, iot, pccons_isa_conf);
	return 0;
}
