/*	$NetBSD: if_cs_isa.c,v 1.3 2000/12/26 09:42:21 mycroft Exp $	*/

/*
 * Copyright 1997
 * Digital Equipment Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and
 * copied only in accordance with the following terms and conditions.
 * Subject to these conditions, you may download, copy, install,
 * use, modify and distribute this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce
 *    and retain this copyright notice and list of conditions as
 *    they appear in the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Digital Equipment Corporation. Neither the "Digital Equipment
 *    Corporation" name nor any trademark or logo of Digital Equipment
 *    Corporation may be used to endorse or promote products derived
 *    from this software without the prior written permission of
 *    Digital Equipment Corporation.
 *
 * 3) This software is provided "AS-IS" and any express or implied
 *    warranties, including but not limited to, any implied warranties
 *    of merchantability, fitness for a particular purpose, or
 *    non-infringement are disclaimed. In no event shall DIGITAL be
 *    liable for any damages whatsoever, and in particular, DIGITAL
 *    shall not be liable for special, indirect, consequential, or
 *    incidental damages or damages for lost profits, loss of
 *    revenue or loss of use, whether such damages arise in contract,
 *    negligence, tort, under statute, in equity, at law or otherwise,
 *    even if advised of the possibility of such damage.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/socket.h>
#include <sys/device.h>

#include "rnd.h"
#if NRND > 0
#include <sys/rnd.h>
#endif

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
#include <dev/isa/isadmavar.h>

#include <dev/isa/cs89x0reg.h>
#include <dev/isa/cs89x0var.h>

int	cs_isa_probe __P((struct device *, struct cfdata *, void *));
void	cs_isa_attach __P((struct device *, struct device *, void *));

struct cfattach cs_isa_ca = {
	sizeof(struct cs_softc), cs_isa_probe, cs_isa_attach
};

int 
cs_isa_probe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct isa_attach_args *ia = aux;
	bus_space_tag_t iot = ia->ia_iot;
	bus_space_tag_t memt = ia->ia_memt;
	bus_space_handle_t ioh, memh;
	int rv = 0, have_io = 0, have_mem = 0;
	u_int16_t isa_cfg, isa_membase;
	bus_addr_t maddr = ia->ia_maddr;
	int irq = ia->ia_irq;

	/*
	 * Disallow wildcarded I/O base.
	 */
	if (ia->ia_iobase == ISACF_PORT_DEFAULT)
		return (0);

	/*
	 * Map the I/O space.
	 */
	if (bus_space_map(ia->ia_iot, ia->ia_iobase, CS8900_IOSIZE, 0, &ioh))
		goto out;
	have_io = 1;

	/* Verify that it's a Crystal product. */
	if (CS_READ_PACKET_PAGE_IO(iot, ioh, PKTPG_EISA_NUM) !=
	    EISA_NUM_CRYSTAL)
		goto out;

	/*
	 * Verify that it's a supported chip.
	 */
	switch (CS_READ_PACKET_PAGE_IO(iot, ioh, PKTPG_PRODUCT_ID) &
	    PROD_ID_MASK) {
	case PROD_ID_CS8900:
#ifdef notyet
	case PROD_ID_CS8920:
	case PROD_ID_CS8920M:
#endif
		rv = 1;
	}

	/*
	 * If the IRQ or memory address were not specified, read the
	 * ISA_CFG EEPROM location.
	 */
	if (maddr == ISACF_IOMEM_DEFAULT || irq == ISACF_IRQ_DEFAULT) {
		if (cs_verify_eeprom(iot, ioh) == CS_ERROR) {
			printf("cs_isa_probe: EEPROM bad or missing\n");
			goto out;
		}
		if (cs_read_eeprom(iot, ioh, EEPROM_ISA_CFG, &isa_cfg)
		    == CS_ERROR) {
			printf("cs_isa_probe: unable to read ISA_CFG\n");
			goto out;
		}
	}

	/*
	 * If the IRQ wasn't specified, get it from the EEPROM.
	 */
	if (irq == ISACF_IRQ_DEFAULT) {
		irq = isa_cfg & ISA_CFG_IRQ_MASK;
		if (irq == 3)
			irq = 5;
		else
			irq += 10;
	}

	/*
	 * If the memory address wasn't specified, get it from the EEPROM.
	 */
	if (maddr == ISACF_IOMEM_DEFAULT) {
		if ((isa_cfg & ISA_CFG_MEM_MODE) == 0) {
			/* EEPROM says don't use memory mode. */
			goto out;
		}
		if (cs_read_eeprom(iot, ioh, EEPROM_MEM_BASE, &isa_membase)
		    == CS_ERROR) {
			printf("cs_isa_probe: unable to read MEM_BASE\n");
			goto out;
		}

		isa_membase &= MEM_BASE_MASK;
		maddr = (int)isa_membase << 8;
	}

	/*
	 * We now have a valid mem address; attempt to map it.
	 */
	if (bus_space_map(ia->ia_memt, maddr, CS8900_MEMSIZE, 0, &memh)) {
		/* Can't map it; fall back on i/o-only mode. */
		printf("cs_isa_probe: unable to map memory space\n");
		maddr = ISACF_IOMEM_DEFAULT;
	} else
		have_mem = 1;

 out:
	if (have_io)
		bus_space_unmap(iot, ioh, CS8900_IOSIZE);
	if (have_mem)
		bus_space_unmap(memt, memh, CS8900_MEMSIZE);

	if (rv) {
		ia->ia_iosize = CS8900_IOSIZE;
		ia->ia_maddr = maddr;
		ia->ia_irq = irq;
		if (ia->ia_maddr != ISACF_IOMEM_DEFAULT)
			ia->ia_msize = CS8900_MEMSIZE;
	}
	return (rv);
}

void 
cs_isa_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct cs_softc *sc = (struct cs_softc *) self;
	struct isa_attach_args *ia = aux;

	sc->sc_ic = ia->ia_ic;
	sc->sc_iot = ia->ia_iot;
	sc->sc_memt = ia->ia_memt;

	sc->sc_drq = ia->ia_drq;
	sc->sc_irq = ia->ia_irq;

	printf("\n");

	/*
	 * Map the device.
	 */
	if (bus_space_map(sc->sc_iot, ia->ia_iobase, ia->ia_iosize,
	    0, &sc->sc_ioh)) {
		printf("%s: unable to map i/o space\n", sc->sc_dev.dv_xname);
		return;
	}

	/*
	 * Validate IRQ.
	 */
	if (CS8900_IRQ_ISVALID(sc->sc_irq) == 0) {
		printf("%s: invalid IRQ %d\n", sc->sc_dev.dv_xname, sc->sc_irq);
		return;
	}

	/*
	 * Map the memory space if it was specified.  If we can do this,
	 * we set ourselves up to use memory mode forever.  Otherwise,
	 * we fall back on I/O mode.
	 */
	if (ia->ia_maddr != ISACF_IOMEM_DEFAULT &&
	    ia->ia_msize == CS8900_MEMSIZE &&
	    CS8900_MEMBASE_ISVALID(ia->ia_maddr)) {
		if (bus_space_map(sc->sc_memt, ia->ia_maddr, ia->ia_msize,
		    0, &sc->sc_memh)) {
			printf("%s: unable to map memory space\n",
			    sc->sc_dev.dv_xname);
		} else {
			sc->sc_cfgflags |= CFGFLG_MEM_MODE;
			sc->sc_pktpgaddr = ia->ia_maddr;
		}
	}

	sc->sc_ih = isa_intr_establish(ia->ia_ic, sc->sc_irq, IST_EDGE,
	    IPL_NET, cs_intr, sc);
	if (sc->sc_ih == NULL) {
		printf("%s: unable to establish interrupt\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	cs_attach(sc, NULL, NULL, 0, 0);
}
