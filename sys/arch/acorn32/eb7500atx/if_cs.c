/*	$NetBSD: if_cs.c,v 1.1 2004/01/03 14:31:28 chris Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_cs.c,v 1.1 2004/01/03 14:31:28 chris Exp $");

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

#include <sys/queue.h>
#include <uvm/uvm.h>
#include <machine/pmap.h>

#include <acorn32/eb7500atx/rsbus.h>

#include <dev/ic/cs89x0reg.h>
#include <dev/ic/cs89x0var.h>

/*
 * the CS network interface is accessed at the following address locations:
 * 030104f1 		CS8920 PNP Low
 * 03010600 03010640	CS8920 Default I/O registers
 * 030114f1 		CS8920 PNP High
 * 03014000 03016000	CS8920 Default Memory
 *
 * IRQ is mapped as:
 * CS8920 IRQ 3 	INT5
 * 
 * It must be configured as the following:
 * The CS8920 PNP address should be configured for ISA base at 0x300
 * to achieve the default register mapping as specified. 
 * Note memory addresses are all have bit 23 tied high in hardware.
 * This only effects the value programmed into the CS8920 memory offset
 * registers. 
 * 
 * Just to add to the fun the I/O registers are layed out as:
 * xxxxR1R0
 * xxxxR3R2
 * xxxxR5R4
 *
 * This makes access to single registers hard (which does happen on a reset,
 * as we've got to toggle the chip into 16bit mode)
 * 
 * Network DRQ is connected to DRQ5 
 */

/*
 * make a private tag so that we can use mainbus's map/unmap
 */
static struct bus_space cs_rsbus_bs_tag;

int	cs_pioc_probe __P((struct device *, struct cfdata *, void *));
void	cs_pioc_attach __P((struct device *, struct device *, void *));

static u_int8_t cs_rbus_read_1(struct cs_softc *, bus_size_t);

CFATTACH_DECL(cs_rsbus, sizeof(struct cs_softc),
	cs_pioc_probe, cs_pioc_attach, NULL, NULL);

/* Available media */
int cs_rbus_media [] = {
	IFM_ETHER|IFM_10_T,
	IFM_ETHER|IFM_10_T|IFM_FDX
};


int 
cs_pioc_probe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	/* for now it'll always attach */
	return 1;
}
#if 0
	struct isa_attach_args *ia = aux;
	bus_space_tag_t iot = ia->ia_iot;
	bus_space_tag_t memt = ia->ia_memt;
	bus_space_handle_t ioh, memh;
	struct cs_softc sc;
	int rv = 0, have_io = 0, have_mem = 0;
	u_int16_t isa_cfg, isa_membase;
	int maddr, irq;

	if (ia->ia_nio < 1)
		return (0);
	if (ia->ia_nirq < 1)
		return (0);

	if (ISA_DIRECT_CONFIG(ia))
		return (0);

	/*
	 * Disallow wildcarded I/O base.
	 */
	if (ia->ia_io[0].ir_addr == ISACF_PORT_DEFAULT)
		return (0);

	if (ia->ia_niomem > 0)
		maddr = ia->ia_iomem[0].ir_addr;
	else
		maddr = ISACF_IOMEM_DEFAULT;

	/*
	 * Map the I/O space.
	 */
	if (bus_space_map(ia->ia_iot, ia->ia_io[0].ir_addr, CS8900_IOSIZE,
	    0, &ioh))
		goto out;
	have_io = 1;

	memset(&sc, 0, sizeof sc);
	sc.sc_iot = iot;
	sc.sc_ioh = ioh;
	/* Verify that it's a Crystal product. */
	if (CS_READ_PACKET_PAGE_IO(&sc, PKTPG_EISA_NUM) !=
	    EISA_NUM_CRYSTAL)
		goto out;

	/*
	 * Verify that it's a supported chip.
	 */
	switch (CS_READ_PACKET_PAGE_IO(&sc, PKTPG_PRODUCT_ID) &
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
	if (maddr == ISACF_IOMEM_DEFAULT ||
	    ia->ia_irq[0].ir_irq == ISACF_IRQ_DEFAULT) {
		if (cs_verify_eeprom(&sc) == CS_ERROR) {
			printf("cs_isa_probe: EEPROM bad or missing\n");
			goto out;
		}
		if (cs_read_eeprom(&sc, EEPROM_ISA_CFG, &isa_cfg)
		    == CS_ERROR) {
			printf("cs_isa_probe: unable to read ISA_CFG\n");
			goto out;
		}
	}

	/*
	 * If the IRQ wasn't specified, get it from the EEPROM.
	 */
	if (ia->ia_irq[0].ir_irq == ISACF_IRQ_DEFAULT) {
		irq = isa_cfg & ISA_CFG_IRQ_MASK;
		if (irq == 3)
			irq = 5;
		else
			irq += 10;
	} else
		irq = ia->ia_irq[0].ir_irq;

	/*
	 * If the memory address wasn't specified, get it from the EEPROM.
	 */
	if (maddr == ISACF_IOMEM_DEFAULT) {
		if ((isa_cfg & ISA_CFG_MEM_MODE) == 0) {
			/* EEPROM says don't use memory mode. */
			goto out;
		}
		if (cs_read_eeprom(&sc, EEPROM_MEM_BASE, &isa_membase)
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
		ia->ia_nio = 1;
		ia->ia_io[0].ir_size = CS8900_IOSIZE;

		if (maddr == ISACF_IOMEM_DEFAULT)
			ia->ia_niomem = 0;
		else {
			ia->ia_niomem = 1;
			ia->ia_iomem[0].ir_addr = maddr;
			ia->ia_iomem[0].ir_size = CS8900_MEMSIZE;
		}

		ia->ia_nirq = 1;
		ia->ia_irq[0].ir_irq = irq;
	}
	return (rv);
}
#endif
void 
cs_pioc_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct cs_softc *sc = (struct cs_softc *)self;
	struct rsbus_attach_args *rs = (void *)aux;
	u_int iobase;

	/* member copy */
	cs_rsbus_bs_tag = *rs->sa_iot;
	
	/* registers are 4 byte aligned */
	cs_rsbus_bs_tag.bs_cookie = (void *) 1;
	
	sc->sc_iot = sc->sc_memt = &cs_rsbus_bs_tag;

	/*
	 * Do DMA later
	if (ia->ia_ndrq > 0)
		isc->sc_drq = ia->ia_drq[0].ir_drq;
	else
		isc->sc_drq = -1;
	*/

	/* device always interrupts on 3 but that routes to IRQ 5 */
	sc->sc_irq = 3;

	printf("\n");

	/*
	 * Map the device.
	 */
	iobase = 0x03010600;
	printf("mapping iobase=0x%08x, for 0x%08x\n", iobase, CS8900_IOSIZE * 4);
	if (bus_space_map(sc->sc_iot, iobase, CS8900_IOSIZE * 4,
	    0, &sc->sc_ioh)) {
		printf("%s: unable to map i/o space\n", sc->sc_dev.dv_xname);
		return;
	}

#if 0
	bus_space_write_2((sc)->sc_iot, (sc)->sc_ioh, PORT_PKTPG_PTR, PKTPG_EISA_NUM);

	productID = bus_space_read_2((sc)->sc_iot, (sc)->sc_ioh, PORT_PKTPG_DATA);

	printf("Result ID = 0x%08x\n", productID);
	printf("cookie = %p\n", sc->sc_iot->bs_cookie);
	{
		volatile uint32_t *ptr =  (void*) ((char *)((sc)->sc_ioh) + (PORT_PKTPG_PTR  << 1));
		volatile uint32_t *data =(void *)((char *)((sc)->sc_ioh) + (PORT_PKTPG_DATA  << 1));
		volatile char *pnplow = (char *)trunc_page((sc)->sc_ioh) + 0x4f1;
		bus_space_handle_t tag2;
		pt_entry_t *pte;
		
		printf("ioh = %p, ptr = %p, data = %p\n", (void*)(sc)->sc_ioh, ptr, data);
		*ptr = PKTPG_EISA_NUM;
		productID = *data;
		printf("Result ID2 = 0x%08x\n", productID);

		pte = vtopte(trunc_page((sc)->sc_ioh));
		printf("pte = %p, *pte =  0x%08x\n", pte, *pte);
		printf("pnplow = %p, *pnplow = 0x%02x\n", pnplow, *pnplow);

		if (bus_space_map(sc->sc_iot, 0x03011000, 0x1000,
					0, &tag2)) {
		printf("%s: unable to map i/o space\n", sc->sc_dev.dv_xname);
		return;
		}
		pnplow = (char *)trunc_page(tag2) + 0x4f1;
		printf("pnplow = %p, *pnplow = 0x%02x\n", pnplow, *pnplow);

		*pnplow = 0x3;

		*ptr = PKTPG_EISA_NUM;
		productID = *data;
		printf("Result ID2 = 0x%08x\n", productID);
	}
#endif

#if 0
	/*
	 * Map the memory space if it was specified.  If we can do this,
	 * we set ourselves up to use memory mode forever.  Otherwise,
	 * we fall back on I/O mode.
	 */
	if (ia->ia_iomem[0].ir_addr != ISACF_IOMEM_DEFAULT &&
	    ia->ia_iomem[0].ir_size == CS8900_MEMSIZE &&
	    CS8900_MEMBASE_ISVALID(ia->ia_iomem[0].ir_addr)) {
#endif	
#if 0
	printf("mapping iobase=0x%08x, for 0x%08x\n", iobase + 0x3A00,
			CS8900_MEMSIZE * 4);
		if (bus_space_map(sc->sc_memt, iobase + 0x3A00,
					CS8900_MEMSIZE * 4, 0, &sc->sc_memh)) {
			printf("%s: unable to map memory space\n",
			    sc->sc_dev.dv_xname);
		} else {
			sc->sc_cfgflags |= CFGFLG_MEM_MODE;
			sc->sc_pktpgaddr = iobase + 0x3A00;
		}
#endif

	printf("Claiming IRQ\n");
		sc->sc_ih = intr_claim(0x0B, IPL_NET, "cs", cs_intr, sc);
	if (sc->sc_ih == NULL) {
		printf("%s: unable to establish interrupt\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	/* DMA is for later */
	sc->sc_dma_chipinit = NULL;
	sc->sc_dma_attach = NULL;
	sc->sc_dma_process_rx = NULL;

	printf("Cs attach addr: 0x%04x\n", CS_READ_PACKET_PAGE(sc, PKTPG_IND_ADDR));

	/* don't talk to the EEPROM, it seems that the cs driver doesn't use a
	 * normal layout */
	sc->sc_cfgflags |= CFGFLG_NOT_EEPROM;
	sc->sc_io_read_1 = cs_rbus_read_1;
	cs_attach(sc, NULL, cs_rbus_media, sizeof(cs_rbus_media) / sizeof(cs_rbus_media[0]),
			IFM_ETHER|IFM_10_T|IFM_FDX);
}

static u_int8_t
cs_rbus_read_1(struct cs_softc *sc, bus_size_t a)
{
	bus_size_t offset;
	/* this is rather warped if it's an even address then just use the
	 * bus_space_read_1
	 */
	if ((a & 1) == 0)
	{
		return bus_space_read_1(sc->sc_iot, sc->sc_ioh, a);
	}
	/* otherwise we've get to work out the aligned address and then add
	 * one */
	/* first work out the offset */
	offset = (a & ~1) << 1;
	/* add the one */
	offset++;

	/* and read it, with no shift */
	return sc->sc_iot->bs_r_1(0, (sc)->sc_ioh, offset);
}
