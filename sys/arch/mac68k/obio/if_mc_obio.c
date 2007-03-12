/*	$NetBSD: if_mc_obio.c,v 1.16.2.1 2007/03/12 05:49:02 rmind Exp $	*/

/*-
 * Copyright (c) 1997 David Huang <khym@azeotrope.org>
 * All rights reserved.
 *
 * Portions of this code are based on code by Denton Gentry <denny1@home.com>
 * and Yanagisawa Takeshi <yanagisw@aa.ap.titech.ac.jp>.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
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
 *
 */

/*
 * Bus attachment and DMA routines for the mc driver (Centris/Quadra
 * 660av and Quadra 840av onboard ethernet, based on the AMD Am79C940
 * MACE ethernet chip). Also uses the PSC (Peripheral Subsystem
 * Controller) for DMA to and from the MACE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_mc_obio.c,v 1.16.2.1 2007/03/12 05:49:02 rmind Exp $");

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/socket.h>
#include <sys/systm.h>

#include <net/if.h>
#include <net/if_ether.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>
#include <machine/psc.h>

#include <mac68k/obio/obiovar.h>
#include <mac68k/dev/if_mcreg.h>
#include <mac68k/dev/if_mcvar.h>

#define MACE_REG_BASE	0x50F1C000
#define MACE_PROM_BASE	0x50F08000

hide int	mc_obio_match(struct device *, struct cfdata *, void *);
hide void	mc_obio_attach(struct device *, struct device *, void *);
hide void	mc_obio_init(struct mc_softc *);
hide void	mc_obio_put(struct mc_softc *, u_int);
hide int	mc_dmaintr(void *);
hide void	mc_reset_rxdma(struct mc_softc *);
hide void	mc_reset_rxdma_set(struct mc_softc *, int);
hide void	mc_reset_txdma(struct mc_softc *);
hide int	mc_obio_getaddr(struct mc_softc *, u_int8_t *);

CFATTACH_DECL(mc_obio, sizeof(struct mc_softc),
    mc_obio_match, mc_obio_attach, NULL, NULL);

hide int
mc_obio_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct obio_attach_args *oa = aux;
	bus_space_handle_t bsh;
	int found = 0;

        if (current_mac_model->class != MACH_CLASSAV)
		return 0;

	if (bus_space_map(oa->oa_tag, MACE_REG_BASE, MC_REGSIZE, 0, &bsh))
		return 0;

	/*
	 * Make sure the MACE's I/O space is readable, and if it is,
	 * try to read the CHIPID register. A MACE will always have
	 * 0x?940, where the ? depends on the chip version.
	 */
	if (mac68k_bus_space_probe(oa->oa_tag, bsh, 0, 1)) {
		if ((bus_space_read_1(
			oa->oa_tag, bsh, MACE_REG(MACE_CHIPIDL)) == 0x40) &&
		    ((bus_space_read_1(
			oa->oa_tag, bsh, MACE_REG(MACE_CHIPIDH)) & 0xf) == 9))
			found = 1;
	}

	bus_space_unmap(oa->oa_tag, bsh, MC_REGSIZE);

	return found;
}

hide void
mc_obio_attach(struct device *parent, struct device *self, void *aux)
{
	struct obio_attach_args *oa = (struct obio_attach_args *)aux;
	struct mc_softc *sc = (void *)self;
	u_int8_t myaddr[ETHER_ADDR_LEN];
	int rsegs;

	sc->sc_regt = oa->oa_tag;
	sc->sc_biucc = XMTSP_64;
	sc->sc_fifocc = XMTFW_16 | RCVFW_64 | XMTFWU | RCVFWU |
	    XMTBRST | RCVBRST;
	sc->sc_plscc = PORTSEL_AUI;

	if (bus_space_map(sc->sc_regt, MACE_REG_BASE, MC_REGSIZE, 0,
	    &sc->sc_regh)) {
		printf(": failed to map space for MACE regs.\n");
		return;
	}

	if (mc_obio_getaddr(sc, myaddr)) {
		printf(": failed to get MAC address.\n");
		return;
	}

	/* allocate memory for transmit and receive DMA buffers */
	sc->sc_dmat = oa->oa_dmat;
	if (bus_dmamem_alloc(sc->sc_dmat, 2 * 0x800, 0, 0, &sc->sc_dmasegs_tx,
		1, &rsegs, BUS_DMA_NOWAIT) != 0) {
		printf(": failed to allocate TX DMA buffers.\n");
		return;
	}

	if (bus_dmamem_map(sc->sc_dmat, &sc->sc_dmasegs_tx, rsegs, 2 * 0x800,
		(void*)&sc->sc_txbuf, BUS_DMA_NOWAIT | BUS_DMA_COHERENT) != 0) {
		printf(": failed to map TX DMA buffers.\n");
		return;
	}

	if (bus_dmamem_alloc(sc->sc_dmat, MC_RXDMABUFS * 0x800, 0, 0,
		&sc->sc_dmasegs_rx, 1, &rsegs, BUS_DMA_NOWAIT) != 0) {
		printf(": failed to allocate RX DMA buffers.\n");
		return;
	}

	if (bus_dmamem_map(sc->sc_dmat, &sc->sc_dmasegs_rx, rsegs,
		MC_RXDMABUFS * 0x800, (void*)&sc->sc_rxbuf,
		BUS_DMA_NOWAIT | BUS_DMA_COHERENT) != 0) {
		printf(": failed to map RX DMA buffers.\n");
		return;
	}

	if (bus_dmamap_create(sc->sc_dmat, 2 * 0x800, 1, 2 * 0x800, 0,
	    BUS_DMA_NOWAIT, &sc->sc_dmam_tx) != 0) {
		printf(": failed to allocate TX DMA map.\n");
		return;
	}
	
	if (bus_dmamap_load(sc->sc_dmat, sc->sc_dmam_tx, sc->sc_txbuf,
		2 * 0x800, NULL, BUS_DMA_NOWAIT) != 0) {
		printf(": failed to map TX DMA mapping.\n");
		return;
	}

	if (bus_dmamap_create(sc->sc_dmat, MC_RXDMABUFS * 0x800, 1,
		MC_RXDMABUFS * 0x800, 0, BUS_DMA_NOWAIT, &sc->sc_dmam_rx) != 0) {
		printf(": failed to allocate RX DMA map.\n");
		return;
	}
	
	if (bus_dmamap_load(sc->sc_dmat, sc->sc_dmam_rx, sc->sc_rxbuf,
		MC_RXDMABUFS * 0x800, NULL, BUS_DMA_NOWAIT) != 0) {
		printf(": failed to map RX DMA mapping.\n");
		return;
	}

	sc->sc_txbuf_phys = sc->sc_dmasegs_tx.ds_addr;
	sc->sc_rxbuf_phys = sc->sc_dmasegs_rx.ds_addr;

	sc->sc_bus_init = mc_obio_init;
	sc->sc_putpacket = mc_obio_put;

	/* disable receive DMA */
	psc_reg2(PSC_ENETRD_CTL) = 0x8800;
	psc_reg2(PSC_ENETRD_CTL) = 0x1000;
	psc_reg2(PSC_ENETRD_CMD + PSC_SET0) = 0x1100;
	psc_reg2(PSC_ENETRD_CMD + PSC_SET1) = 0x1100;

	/* disable transmit DMA */
	psc_reg2(PSC_ENETWR_CTL) = 0x8800;
	psc_reg2(PSC_ENETWR_CTL) = 0x1000;
	psc_reg2(PSC_ENETWR_CMD + PSC_SET0) = 0x1100;
	psc_reg2(PSC_ENETWR_CMD + PSC_SET1) = 0x1100;

	/* install interrupt handlers */
	add_psc_lev4_intr(PSCINTR_ENET_DMA, mc_dmaintr, sc);
	add_psc_lev3_intr(mcintr, sc);

	/* enable MACE DMA interrupts */
	psc_reg1(PSC_LEV4_IER) = 0x80 | (1 << PSCINTR_ENET_DMA);

	/* don't know what this does */
	psc_reg2(PSC_ENETWR_CTL) = 0x9000;
	psc_reg2(PSC_ENETRD_CTL) = 0x9000;
	psc_reg2(PSC_ENETWR_CTL) = 0x0400;
	psc_reg2(PSC_ENETRD_CTL) = 0x0400;

	/* enable MACE interrupts */
	psc_reg1(PSC_LEV3_IER) = 0x80 | (1 << PSCINTR_ENET);

	/* mcsetup returns 1 if something fails */
	if (mcsetup(sc, myaddr)) {
		/* disable interrupts */
		psc_reg1(PSC_LEV4_IER) = (1 << PSCINTR_ENET_DMA);
		psc_reg1(PSC_LEV3_IER) = (1 << PSCINTR_ENET);
		/* remove interrupt handlers */
		remove_psc_lev4_intr(PSCINTR_ENET_DMA);
		remove_psc_lev3_intr();

		bus_space_unmap(sc->sc_regt, sc->sc_regh, MC_REGSIZE);
		return;
	}
}

/* Bus-specific initialization */
hide void
mc_obio_init(struct mc_softc *sc)
{
	mc_reset_rxdma(sc);
	mc_reset_txdma(sc);
}

hide void
mc_obio_put(struct mc_softc *sc, u_int len)
{
	int offset = sc->sc_txset == 0 ? 0 : 0x800;

	bus_dmamap_sync(sc->sc_dmat, sc->sc_dmam_tx, offset, 0x800,
	    BUS_DMASYNC_PREWRITE);
	psc_reg4(PSC_ENETWR_ADDR + sc->sc_txset) = sc->sc_txbuf_phys + offset;
	psc_reg4(PSC_ENETWR_LEN + sc->sc_txset) = len;
	psc_reg2(PSC_ENETWR_CMD + sc->sc_txset) = 0x9800;
	bus_dmamap_sync(sc->sc_dmat, sc->sc_dmam_tx, offset, 0x800,
	    BUS_DMASYNC_POSTWRITE);

	sc->sc_txset ^= 0x10;
}

/*
 * Interrupt handler for the MACE DMA completion interrupts
 */
int
mc_dmaintr(void *arg)
{
	struct mc_softc *sc = arg;
	u_int16_t status;
	u_int32_t bufsleft, which;
	int head;

	/*
	 * Not sure what this does... figure out if this interrupt is
	 * really ours?
	 */
	while ((which = psc_reg4(0x804)) != psc_reg4(0x804))
		;
	if ((which & 0x60000000) == 0)
		return 0;

	/* Get the read channel status */
	status = psc_reg2(PSC_ENETRD_CTL);
	if (status & 0x2000) {
		/* I think this is an exceptional condition. Reset the DMA */
		mc_reset_rxdma(sc);
#ifdef MCDEBUG
		printf("%s: resetting receive DMA channel (status 0x%04x)\n",
		    sc->sc_dev.dv_xname, status);
#endif
	} else if (status & 0x100) {
		/* We've received some packets from the MACE */
		int offset;

		/* Clear the interrupt */
		psc_reg2(PSC_ENETRD_CMD + sc->sc_rxset) = 0x1100;

		/* See how may receive buffers are left */
		bufsleft = psc_reg4(PSC_ENETRD_LEN + sc->sc_rxset);
		head = MC_RXDMABUFS - bufsleft;

#if 0 /* I don't think this should ever happen */
		if (head == sc->sc_tail) {
#ifdef MCDEBUG
			printf("%s: head == tail: suspending DMA?\n",
			    sc->sc_dev.dv_xname);
#endif
			psc_reg2(PSC_ENETRD_CMD + sc->sc_rxset) = 0x9000;
		}
#endif

		/* Loop through, processing each of the packets */
		for (; sc->sc_tail < head; sc->sc_tail++) {
			offset = sc->sc_tail * 0x800;

			bus_dmamap_sync(sc->sc_dmat, sc->sc_dmam_rx,
					PAGE_SIZE + offset, 0x800,
					BUS_DMASYNC_PREREAD);

			sc->sc_rxframe.rx_rcvcnt = sc->sc_rxbuf[offset];
			sc->sc_rxframe.rx_rcvsts = sc->sc_rxbuf[offset+2];
			sc->sc_rxframe.rx_rntpc = sc->sc_rxbuf[offset+4];
			sc->sc_rxframe.rx_rcvcc = sc->sc_rxbuf[offset+6];
			sc->sc_rxframe.rx_frame = sc->sc_rxbuf + offset + 16;

			mc_rint(sc);

			bus_dmamap_sync(sc->sc_dmat, sc->sc_dmam_rx,
					PAGE_SIZE + offset, 0x800,
					BUS_DMASYNC_POSTREAD);
		}

		/*
		 * If we're out of buffers, reset this register set
		 * and switch to the other one. Otherwise, reactivate
		 * this set.
		 */
		if (bufsleft == 0) {
			mc_reset_rxdma_set(sc, sc->sc_rxset);
			sc->sc_rxset ^= 0x10;
		} else
			psc_reg2(PSC_ENETRD_CMD + sc->sc_rxset) = 0x9800;
	}

	/* Get the write channel status */
	status = psc_reg2(PSC_ENETWR_CTL);
	if (status & 0x2000) {
		/* I think this is an exceptional condition. Reset the DMA */
		mc_reset_txdma(sc);
#ifdef MCDEBUG
		printf("%s: resetting transmit DMA channel (status 0x%04x)\n",
			sc->sc_dev.dv_xname, status);
#endif
	} else if (status & 0x100) {
		/* Clear the interrupt and switch register sets */
		psc_reg2(PSC_ENETWR_CMD + sc->sc_txseti) = 0x100;
		sc->sc_txseti ^= 0x10;
	}

	return 1;
}


hide void
mc_reset_rxdma(struct mc_softc *sc)
{
	u_int8_t maccc;

	/* Disable receiver, reset the DMA channels */
	maccc = NIC_GET(sc, MACE_MACCC);
	NIC_PUT(sc, MACE_MACCC, maccc & ~ENRCV);
	psc_reg2(PSC_ENETRD_CTL) = 0x8800;
	mc_reset_rxdma_set(sc, 0);
	psc_reg2(PSC_ENETRD_CTL) = 0x400;

	psc_reg2(PSC_ENETRD_CTL) = 0x8800;
	mc_reset_rxdma_set(sc, 0x10);
	psc_reg2(PSC_ENETRD_CTL) = 0x400;

	/* Reenable receiver, reenable DMA */
	NIC_PUT(sc, MACE_MACCC, maccc);
	sc->sc_rxset = 0;

	psc_reg2(PSC_ENETRD_CMD + PSC_SET0) = 0x9800;
	psc_reg2(PSC_ENETRD_CMD + PSC_SET1) = 0x9800;
}

hide void
mc_reset_rxdma_set(struct mc_softc *sc, int set)
{
	/* disable DMA while modifying the registers, then reenable DMA */
	psc_reg2(PSC_ENETRD_CMD + set) = 0x0100;
	psc_reg4(PSC_ENETRD_ADDR + set) = sc->sc_rxbuf_phys;
	psc_reg4(PSC_ENETRD_LEN + set) = MC_RXDMABUFS;
	psc_reg2(PSC_ENETRD_CMD + set) = 0x9800;
	sc->sc_tail = 0;
}

hide void
mc_reset_txdma(struct mc_softc *sc)
{
	u_int8_t maccc;

	psc_reg2(PSC_ENETWR_CTL) = 0x8800;
	maccc = NIC_GET(sc, MACE_MACCC);
	NIC_PUT(sc, MACE_MACCC, maccc & ~ENXMT);
	sc->sc_txset = sc->sc_txseti = 0;
	psc_reg2(PSC_ENETWR_CTL) = 0x400;
	NIC_PUT(sc, MACE_MACCC, maccc);
}

hide int
mc_obio_getaddr(struct mc_softc *sc, u_int8_t *lladdr)
{
	bus_space_handle_t bsh;
	u_char csum;

	if (bus_space_map(sc->sc_regt, MACE_PROM_BASE, 8*16, 0, &bsh)) {
		printf(": failed to map space to read MACE address.\n%s",
		    sc->sc_dev.dv_xname);
		return (-1);
	}

	if (!mac68k_bus_space_probe(sc->sc_regt, bsh, 0, 1)) {
		bus_space_unmap(sc->sc_regt, bsh, 8*16);
		return (-1);
	}

	csum = mc_get_enaddr(sc->sc_regt, bsh, 1, lladdr);
	if (csum != 0xff)
		printf(": ethernet PROM checksum failed (0x%x != 0xff)\n%s",
		    (int)csum, sc->sc_dev.dv_xname);

	bus_space_unmap(sc->sc_regt, bsh, 8*16);

	return (csum == 0xff ? 0 : -1);
}
