/*	$NetBSD: igsfb_pci.c,v 1.2.2.3 2002/08/01 02:45:21 nathanw Exp $ */

/*
 * Copyright (c) 2002 Valeriy E. Ushakov
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
 * Integraphics Systems IGA 168x and CyberPro series.
 * Only tested on IGA 1682 in Krups JavaStation-NC.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: igsfb_pci.c,v 1.2.2.3 2002/08/01 02:45:21 nathanw Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/buf.h>

#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#include <dev/wscons/wsconsio.h>
#include <dev/ic/igsfbreg.h>
#include <dev/ic/igsfbvar.h>


static int	igsfb_pci_match(struct device *, struct cfdata *, void *);
static void	igsfb_pci_attach(struct device *, struct device *, void *);

const struct cfattach igsfb_pci_ca = {
	sizeof(struct igsfb_softc), igsfb_pci_match, igsfb_pci_attach,
};


static int
igsfb_pci_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pci_attach_args *pa = aux;

	if (PCI_VENDOR(pa->pa_id) != PCI_VENDOR_INTEGRAPHICS)
		return (0);

	/* probably can drive iga1680 and cyberpro cards as well */
	if (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_INTEGRAPHICS_IGA1682)
		return (1);

	return (0);
}


/*
 * Note, that the chip may still be not enabled (e.g. JavaStation PROM
 * doesn't bother to init the chip if PROM output goes to serial).
 */
static void
igsfb_pci_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct igsfb_softc *sc = (struct igsfb_softc *)self;
	struct pci_attach_args *pa = aux;
	bus_addr_t iobase;
	int ioflags;
	int isconsole;
	char devinfo[256];

	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo);
	printf(": %s, revision 0x%02x\n", devinfo, PCI_REVISION(pa->pa_class));

	if (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_INTEGRAPHICS_IGA1682)
		sc->sc_is2k = 0;
	else
		sc->sc_is2k = 1;

	/*
	 * Enable the chip.  This always goes through i/o space
	 * because that's the only way that is guaranteed to enable
	 * completely uninitialized card.
	 */
	if (igsfb_enable(pa->pa_iot) != 0)
		return;

	/*
	 * Configure memory space first since for CyberPro we use
	 * memory-mapped i/o access.  Note that we are NOT mapping any
	 * of it yet.  (XXX: search for memory BAR)
	 */
#define IGS_MEM_MAPREG (PCI_MAPREG_START + 0)

	sc->sc_memt = pa->pa_memt;
	if (pci_mapreg_info(pa->pa_pc, pa->pa_tag,
		IGS_MEM_MAPREG, PCI_MAPREG_TYPE_MEM,
		&sc->sc_memaddr, &sc->sc_memsz, &sc->sc_memflags) != 0)
	{
		printf("unable to configure memory space\n");
		return;
	}

	/*
	 * Configure I/O space.  On CyberPro use MMIO.  IGS 168x
	 * doesn't have a BAR for its i/o, so we have to hardcode it.
	 */
	if (sc->sc_is2k) {
		sc->sc_iot = sc->sc_memt;
		iobase = IGS_MEM_MMIO_SELECT | sc->sc_memaddr;
		ioflags = sc->sc_memflags;
	} else {
		/* feh, 1682 config denies having io space registers */
		sc->sc_iot = pa->pa_iot;
		iobase = 0;
		ioflags = 0;
	}

	/*
	 * Map I/O registers.  This is done in bus glue, not in common
	 * code because on e.g. ISA bus we'd need to access registers
	 * to obtain/program linear memory location.
	 */
	if (bus_space_map(sc->sc_iot,
			  iobase + IGS_REG_BASE, IGS_REG_SIZE, ioflags,
			  &sc->sc_ioh) != 0)
	{
		printf("unable to map I/O registers\n");
		return;
	}

	/* TODO: Map CRTC???  This simple driver doesn't use it so far. */

	/*
	 * TODO: Map graphic coprocessor registers.  not sure if this
	 * needs to be done in bus glue or can be moved to common
	 * attach code.
	 */

	isconsole = 0;
#ifdef __sparc__  /* XXX: this doesn't belong here */
	if (PCITAG_NODE(pa->pa_tag) == prom_instance_to_package(prom_stdout()))
		isconsole = 1;
#endif

	igsfb_common_attach(sc, isconsole);
}
