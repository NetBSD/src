/*	$NetBSD: intio_dmac.c,v 1.33.18.3 2017/12/03 11:36:48 jdolecek Exp $	*/

/*-
 * Copyright (c) 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Minoura Makoto.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

/*
 * Hitachi HD63450 (= Motorola MC68450) DMAC driver for x68k.
 */

#include "opt_m68k_arch.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: intio_dmac.c,v 1.33.18.3 2017/12/03 11:36:48 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <uvm/uvm_extern.h>

#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/frame.h>

#include <arch/x68k/dev/intiovar.h>
#include <arch/x68k/dev/dmacvar.h>

#ifdef DMAC_DEBUG
#define DPRINTF(n,x)	if (dmacdebug>((n)&0x0f)) printf x
#define DDUMPREGS(n,x)	if (dmacdebug>((n)&0x0f)) {printf x; dmac_dump_regs();}
int dmacdebug = 0;
#else
#define DPRINTF(n,x)
#define DDUMPREGS(n,x)
#endif

static void dmac_init_channels(struct dmac_softc *);
#ifdef DMAC_ARRAYCHAIN
static int dmac_program_arraychain(device_t, struct dmac_dma_xfer *,
	u_int, u_int);
#endif
static int dmac_done(void *);
static int dmac_error(void *);

#ifdef DMAC_DEBUG
static int dmac_dump_regs(void);
#endif

/*
 * autoconf stuff
 */
static int dmac_match(device_t, cfdata_t, void *);
static void dmac_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(dmac, sizeof(struct dmac_softc),
    dmac_match, dmac_attach, NULL, NULL);

static int dmac_attached;

static int
dmac_match(device_t parent, cfdata_t cf, void *aux)
{
	struct intio_attach_args *ia = aux;

	if (strcmp(ia->ia_name, "dmac") != 0)
		return (0);
	if (dmac_attached)
		return (0);

	if (ia->ia_addr == INTIOCF_ADDR_DEFAULT)
		ia->ia_addr = DMAC_ADDR;

	/* fixed address */
	if (ia->ia_addr != DMAC_ADDR)
		return (0);
	if (ia->ia_intr != INTIOCF_INTR_DEFAULT)
		return (0);

	return 1;
}

static void
dmac_attach(device_t parent, device_t self, void *aux)
{
	struct dmac_softc *sc = device_private(self);
	struct intio_attach_args *ia = aux;
	struct intio_softc *intio;
	int r __diagused;

	sc->sc_dev = self;
	dmac_attached = 1;

	ia->ia_size = DMAC_CHAN_SIZE * DMAC_NCHAN;
	r = intio_map_allocate_region(parent, ia, INTIO_MAP_ALLOCATE);
#ifdef DIAGNOSTIC
	if (r)
		panic("IO map for DMAC corruption??");
#endif

	intio = device_private(parent);
	intio->sc_dmac = self;
	sc->sc_bst = ia->ia_bst;
	bus_space_map(sc->sc_bst, ia->ia_addr, ia->ia_size, 0, &sc->sc_bht);
	dmac_init_channels(sc);

	aprint_normal(": HD63450 DMAC\n");
	aprint_normal_dev(self, "4 channels available.\n");
}

static void
dmac_init_channels(struct dmac_softc *sc)
{
	int i;

	DPRINTF(3, ("dmac_init_channels\n"));
	for (i=0; i<DMAC_NCHAN; i++) {
		sc->sc_channels[i].ch_channel = i;
		sc->sc_channels[i].ch_name[0] = 0;
		sc->sc_channels[i].ch_softc = sc;
		bus_space_subregion(sc->sc_bst, sc->sc_bht,
				    DMAC_CHAN_SIZE*i, DMAC_CHAN_SIZE,
				    &sc->sc_channels[i].ch_bht);
		sc->sc_channels[i].ch_xfer.dx_dmamap = 0;
		/* reset the status register */
		bus_space_write_1(sc->sc_bst, sc->sc_channels[i].ch_bht,
				  DMAC_REG_CSR, 0xff);
	}

	return;
}


/*
 * Channel initialization/deinitialization per user device.
 */
struct dmac_channel_stat *
dmac_alloc_channel(device_t self, int ch, const char *name,
    int normalv, dmac_intr_handler_t normal, void *normalarg,
    int errorv,  dmac_intr_handler_t error,  void *errorarg,
    uint8_t dcr, uint8_t ocr)
{
	struct intio_softc *intio = device_private(self);
	struct dmac_softc *dmac = device_private(intio->sc_dmac);
	struct dmac_channel_stat *chan = &dmac->sc_channels[ch];
#ifdef DMAC_ARRAYCHAIN
	int r, dummy;
#endif

	aprint_normal_dev(dmac->sc_dev, "allocating ch %d for %s.\n",
		ch, name);
	DPRINTF(3, ("dmamap=%p\n", (void *)chan->ch_xfer.dx_dmamap));
#ifdef DIAGNOSTIC
	if (ch < 0 || ch >= DMAC_NCHAN)
		panic("Invalid DMAC channel.");
	if (chan->ch_name[0])
		panic("DMAC: channel in use.");
	if (strlen(name) > 8)
	  	panic("DMAC: wrong user name.");
#endif

#ifdef DMAC_ARRAYCHAIN
	/* allocate the DMAC arraychaining map */
	r = bus_dmamem_alloc(intio->sc_dmat,
			     sizeof(struct dmac_sg_array) * DMAC_MAPSIZE,
			     4, 0, &chan->ch_seg[0], 1, &dummy,
			     BUS_DMA_NOWAIT);
	if (r)
		panic("DMAC: cannot alloc DMA safe memory");
	r = bus_dmamem_map(intio->sc_dmat,
			   &chan->ch_seg[0], 1,
			   sizeof(struct dmac_sg_array) * DMAC_MAPSIZE,
			   (void **) &chan->ch_map,
			   BUS_DMA_NOWAIT|BUS_DMA_COHERENT);
	if (r)
		panic("DMAC: cannot map DMA safe memory");
#endif

	/* fill the channel status structure by the default values. */
	strcpy(chan->ch_name, name);
	chan->ch_dcr = dcr;
	chan->ch_ocr = ocr;
	chan->ch_normalv = normalv;
	chan->ch_errorv = errorv;
	chan->ch_normal = normal;
	chan->ch_error = error;
	chan->ch_normalarg = normalarg;
	chan->ch_errorarg = errorarg;
	chan->ch_xfer.dx_dmamap = 0;

	/* setup the device-specific registers */
	bus_space_write_1(dmac->sc_bst, chan->ch_bht, DMAC_REG_CSR, 0xff);
	bus_space_write_1(dmac->sc_bst, chan->ch_bht,
			   DMAC_REG_DCR, chan->ch_dcr);
	bus_space_write_1(dmac->sc_bst, chan->ch_bht, DMAC_REG_CPR, 0);
	/* OCR will be written at dmac_load_xfer() */

	/*
	 * X68k physical user space is a subset of the kernel space;
	 * the memory is always included in the physical user space,
	 * while the device is not.
	 */
	bus_space_write_1(dmac->sc_bst, chan->ch_bht,
			   DMAC_REG_BFCR, DMAC_FC_USER_DATA);
	bus_space_write_1(dmac->sc_bst, chan->ch_bht,
			   DMAC_REG_MFCR, DMAC_FC_USER_DATA);
	bus_space_write_1(dmac->sc_bst, chan->ch_bht,
			   DMAC_REG_DFCR, DMAC_FC_KERNEL_DATA);

	/* setup the interrupt handlers */
	bus_space_write_1(dmac->sc_bst, chan->ch_bht, DMAC_REG_NIVR, normalv);
	bus_space_write_1(dmac->sc_bst, chan->ch_bht, DMAC_REG_EIVR, errorv);

	intio_intr_establish_ext(normalv, name, "dma", dmac_done, chan);
	intio_intr_establish_ext(errorv, name, "dmaerr", dmac_error, chan);

	return chan;
}

int
dmac_free_channel(device_t self, int ch, void *channel)
{
	struct intio_softc *intio = device_private(self);
	struct dmac_softc *dmac = device_private(intio->sc_dmac);
	struct dmac_channel_stat *chan = &dmac->sc_channels[ch];

	DPRINTF(3, ("dmac_free_channel, %d\n", ch));
	DPRINTF(3, ("dmamap=%p\n", (void *)chan->ch_xfer.dx_dmamap));
	if (chan != channel)
		return -1;
	if (ch != chan->ch_channel)
		return -1;

#ifdef DMAC_ARRAYCHAIN
	bus_dmamem_unmap(intio->sc_dmat, (void *)chan->ch_map,
			 sizeof(struct dmac_sg_array) * DMAC_MAPSIZE);
	bus_dmamem_free(intio->sc_dmat, &chan->ch_seg[0], 1);
#endif
	chan->ch_name[0] = 0;
	intio_intr_disestablish(chan->ch_normalv, channel);
	intio_intr_disestablish(chan->ch_errorv, channel);

	return 0;
}

/*
 * Initialization / deinitialization per transfer.
 */
struct dmac_dma_xfer *
dmac_alloc_xfer(struct dmac_channel_stat *chan, bus_dma_tag_t dmat,
    bus_dmamap_t dmamap)
{
	struct dmac_dma_xfer *xf = &chan->ch_xfer;

	DPRINTF(3, ("dmac_alloc_xfer\n"));
	xf->dx_channel = chan;
	xf->dx_dmamap = dmamap;
	xf->dx_tag = dmat;
#ifdef DMAC_ARRAYCHAIN
	xf->dx_array = chan->ch_map;
	xf->dx_done = 0;
#endif
	return xf;
}

int
dmac_load_xfer(struct dmac_softc *dmac, struct dmac_dma_xfer *xf)
{
	struct dmac_channel_stat *chan = xf->dx_channel;

	DPRINTF(3, ("dmac_load_xfer\n"));

	xf->dx_ocr &= ~DMAC_OCR_CHAIN_MASK;
	if (xf->dx_dmamap->dm_nsegs == 1)
		xf->dx_ocr |= DMAC_OCR_CHAIN_DISABLED;
	else {
		xf->dx_ocr |= DMAC_OCR_CHAIN_ARRAY;
	}

	bus_space_write_1(dmac->sc_bst, chan->ch_bht, DMAC_REG_CSR, 0xff);
	bus_space_write_1(dmac->sc_bst, chan->ch_bht, DMAC_REG_SCR, xf->dx_scr);
	bus_space_write_1(dmac->sc_bst, chan->ch_bht,
			  DMAC_REG_OCR, (xf->dx_ocr | chan->ch_ocr));
	bus_space_write_4(dmac->sc_bst, chan->ch_bht,
			  DMAC_REG_DAR, (int) xf->dx_device);

	return 0;
}

struct dmac_dma_xfer *
dmac_prepare_xfer(struct dmac_channel_stat *chan, bus_dma_tag_t dmat,
    bus_dmamap_t dmamap, int dir, int scr, void *dar)
{
	struct dmac_dma_xfer *xf;
	struct dmac_softc *dmac = chan->ch_softc;

	xf = dmac_alloc_xfer(chan, dmat, dmamap);

	xf->dx_ocr = dir & DMAC_OCR_DIR_MASK;
	xf->dx_scr = scr & (DMAC_SCR_MAC_MASK|DMAC_SCR_DAC_MASK);
	xf->dx_device = dar;

	dmac_load_xfer(dmac, xf);

	return xf;
}

#ifdef DMAC_DEBUG
static struct dmac_channel_stat *debugchan = 0;
#endif

/*
 * Do the actual transfer.
 */
int
dmac_start_xfer(struct dmac_softc *dmac, struct dmac_dma_xfer *xf)
{
	return dmac_start_xfer_offset(dmac, xf, 0, 0);
}

int
dmac_start_xfer_offset(struct dmac_softc *dmac, struct dmac_dma_xfer *xf,
    u_int offset, u_int size)
{
	struct dmac_channel_stat *chan = xf->dx_channel;
	struct x68k_bus_dmamap *dmamap = xf->dx_dmamap;
	int go = DMAC_CCR_STR|DMAC_CCR_INT;
	bus_addr_t paddr;
	uint8_t csr;
#ifdef DMAC_ARRAYCHAIN
	int c;
#endif

	DPRINTF(3, ("dmac_start_xfer\n"));
#ifdef DMAC_DEBUG
	debugchan=chan;
#endif

	if (size == 0) {
#ifdef DIAGNOSTIC
		if (offset != 0)
			panic("dmac_start_xfer_offset: invalid offset %x",
			       offset);
#endif
		size = dmamap->dm_mapsize;
	}

#ifdef DMAC_ARRAYCHAIN
#ifdef DIAGNOSTIC
	if (xf->dx_done)
		panic("dmac_start_xfer: DMA transfer in progress");
#endif
#endif
	DPRINTF(3, ("First program:\n"));
#ifdef DIAGNOSTIC
	if ((offset >= dmamap->dm_mapsize) ||
	    (offset + size > dmamap->dm_mapsize))
		panic("dmac_start_xfer_offset: invalid offset: "
			"offset=%d, size=%d, mapsize=%ld",
		       offset, size, dmamap->dm_mapsize);
#endif
	/* program DMAC in single block mode or array chainning mode */
	if (dmamap->dm_nsegs == 1) {
		DPRINTF(3, ("single block mode\n"));
#ifdef DIAGNOSTIC
		if (dmamap->dm_mapsize != dmamap->dm_segs[0].ds_len)
			panic("dmac_start_xfer_offset: dmamap curruption");
#endif
		paddr = dmamap->dm_segs[0].ds_addr + offset;
		csr = bus_space_read_1(dmac->sc_bst, chan->ch_bht, DMAC_REG_CSR);
		if ((csr & DMAC_CSR_ACT) != 0) {
			/* Use 'Continue Mode' */
			bus_space_write_4(dmac->sc_bst, chan->ch_bht,
			    DMAC_REG_BAR, paddr);
			bus_space_write_2(dmac->sc_bst, chan->ch_bht,
			    DMAC_REG_BTCR, (int) size);
			go |=  DMAC_CCR_CNT;
			go &= ~DMAC_CCR_STR;
		} else {
			bus_space_write_4(dmac->sc_bst, chan->ch_bht,
					  DMAC_REG_MAR, paddr);
			bus_space_write_2(dmac->sc_bst, chan->ch_bht,
					  DMAC_REG_MTCR, (int) size);
		}
#ifdef DMAC_ARRAYCHAIN
		xf->dx_done = 1;
#endif
	} else {
#ifdef DMAC_ARRAYCHAIN
		c = dmac_program_arraychain(self, xf, offset, size);
		bus_space_write_4(dmac->sc_bst, chan->ch_bht,
				  DMAC_REG_BAR, (int) chan->ch_seg[0].ds_addr);
		bus_space_write_2(dmac->sc_bst, chan->ch_bht,
				  DMAC_REG_BTCR, c);
#else
		panic("DMAC: unexpected use of arraychaining mode");
#endif
	}

	bus_space_write_1(dmac->sc_bst, chan->ch_bht, DMAC_REG_CSR, 0xff);

	/* START!! */
	DDUMPREGS(3, ("first start\n"));

#ifdef DMAC_ARRAYCHAIN
#if defined(M68040) || defined(M68060)
	/* flush data cache for the map */
	if (dmamap->dm_nsegs != 1 && mmutype == MMU_68040)
		dma_cachectl((void *) xf->dx_array,
			     sizeof(struct dmac_sg_array) * c);
#endif
#endif
	bus_space_write_1(dmac->sc_bst, chan->ch_bht, DMAC_REG_CCR, go);

	return 0;
}

#ifdef DMAC_ARRAYCHAIN
static int
dmac_program_arraychain(device_t self, struct dmac_dma_xfer *xf,
    u_int offset, u_int size)
{
	struct dmac_channel_stat *chan = xf->dx_channel;
	int ch = chan->ch_channel;
	struct x68k_bus_dmamap *map = xf->dx_dmamap;
	int i, j;

	/* XXX not yet!! */
	if (offset != 0 || size != map->dm_mapsize)
		panic("dmac_program_arraychain: unsupported offset/size");

	DPRINTF(3, ("dmac_program_arraychain\n"));
	for (i=0, j=xf->dx_done; i<DMAC_MAPSIZE && j<map->dm_nsegs;
	     i++, j++) {
		xf->dx_array[i].da_addr = map->dm_segs[j].ds_addr;
#ifdef DIAGNOSTIC
		if (map->dm_segs[j].ds_len > DMAC_MAXSEGSZ)
			panic("dmac_program_arraychain: wrong map: %ld",
			       map->dm_segs[j].ds_len);
#endif
		xf->dx_array[i].da_count = map->dm_segs[j].ds_len;
	}
	xf->dx_done = j;

	return i;
}
#endif

/*
 * interrupt handlers.
 */
static int
dmac_done(void *arg)
{
	struct dmac_channel_stat *chan = arg;
	struct dmac_softc *sc = chan->ch_softc;
#ifdef DMAC_ARRAYCHAIN
	struct dmac_dma_xfer *xf = &chan->ch_xfer;
	struct x68k_bus_dmamap *map = xf->dx_dmamap;
	int c;
#endif

	DPRINTF(3, ("dmac_done\n"));

	bus_space_write_1(sc->sc_bst, chan->ch_bht, DMAC_REG_CSR, 0xff);

#ifdef DMAC_ARRAYCHAIN
	if (xf->dx_done == map->dm_nsegs) {
		xf->dx_done = 0;
#endif
		/* Done */
		return (*chan->ch_normal)(chan->ch_normalarg);
#ifdef DMAC_ARRAYCHAIN
	}
#endif

#ifdef DMAC_ARRAYCHAIN
	/* Continue transfer */
	DPRINTF(3, ("reprograming\n"));
	c = dmac_program_arraychain(sc->sc_dev, xf, 0, map->dm_mapsize);

	bus_space_write_1(sc->sc_bst, chan->ch_bht, DMAC_REG_CSR, 0xff);
	bus_space_write_4(sc->sc_bst, chan->ch_bht,
			  DMAC_REG_BAR, (int) chan->ch_map);
	bus_space_write_4(sc->sc_bst, chan->ch_bht,
			  DMAC_REG_DAR, (int) xf->dx_device);
	bus_space_write_2(sc->sc_bst, chan->ch_bht, DMAC_REG_BTCR, c);

	/* START!! */
	DDUMPREGS(3, ("restart\n"));
	bus_space_write_1(sc->sc_bst, chan->ch_bht,
			  DMAC_REG_CCR, DMAC_CCR_STR|DMAC_CCR_INT);

	return 1;
#endif
}

static int
dmac_error(void *arg)
{
	struct dmac_channel_stat *chan = arg;
	struct dmac_softc *sc = chan->ch_softc;
	uint8_t csr, cer;

	csr = bus_space_read_1(sc->sc_bst, chan->ch_bht, DMAC_REG_CSR);
	cer = bus_space_read_1(sc->sc_bst, chan->ch_bht, DMAC_REG_CER);

#ifndef DMAC_DEBUG
	/* Software abort (CER=0x11) could happen on normal xfer termination */
	if (cer != 0x11)
#endif
	{
		printf("DMAC transfer error CSR=%02x, CER=%02x\n", csr, cer);
	}
	DDUMPREGS(3, ("registers were:\n"));

	/* Clear the status bits */
	bus_space_write_1(sc->sc_bst, chan->ch_bht, DMAC_REG_CSR, 0xff);

#ifdef DMAC_ARRAYCHAIN
	chan->ch_xfer.dx_done = 0;
#endif

	return (*chan->ch_error)(chan->ch_errorarg);
}

int
dmac_abort_xfer(struct dmac_softc *dmac, struct dmac_dma_xfer *xf)
{
	struct dmac_channel_stat *chan = xf->dx_channel;

	bus_space_write_1(dmac->sc_bst, chan->ch_bht, DMAC_REG_CCR,
			  DMAC_CCR_INT | DMAC_CCR_SAB);
	bus_space_write_1(dmac->sc_bst, chan->ch_bht, DMAC_REG_CSR, 0xff);

	return 0;
}

#ifdef DMAC_DEBUG
static int
dmac_dump_regs(void)
{
	struct dmac_channel_stat *chan = debugchan;
	struct dmac_softc *sc;

	if ((chan == 0) || (dmacdebug & 0xf0))
		return 0;
	sc = chan->ch_softc;

	printf("DMAC channel %d registers\n", chan->ch_channel);
	printf("CSR=%02x, CER=%02x, DCR=%02x, OCR=%02x, SCR=%02x, "
		"CCR=%02x, CPR=%02x, GCR=%02x\n",
		bus_space_read_1(sc->sc_bst, chan->ch_bht, DMAC_REG_CSR),
		bus_space_read_1(sc->sc_bst, chan->ch_bht, DMAC_REG_CER),
		bus_space_read_1(sc->sc_bst, chan->ch_bht, DMAC_REG_DCR),
		bus_space_read_1(sc->sc_bst, chan->ch_bht, DMAC_REG_OCR),
		bus_space_read_1(sc->sc_bst, chan->ch_bht, DMAC_REG_SCR),
		bus_space_read_1(sc->sc_bst, chan->ch_bht, DMAC_REG_CCR),
		bus_space_read_1(sc->sc_bst, chan->ch_bht, DMAC_REG_CPR),
		bus_space_read_1(sc->sc_bst, chan->ch_bht, DMAC_REG_GCR));
	printf("NIVR=%02x, EIVR=%02x, MTCR=%04x, BTCR=%04x, DFCR=%02x, "
		"MFCR=%02x, BFCR=%02x\n",
		bus_space_read_1(sc->sc_bst, chan->ch_bht, DMAC_REG_NIVR),
		bus_space_read_1(sc->sc_bst, chan->ch_bht, DMAC_REG_EIVR),
		bus_space_read_2(sc->sc_bst, chan->ch_bht, DMAC_REG_MTCR),
		bus_space_read_2(sc->sc_bst, chan->ch_bht, DMAC_REG_BTCR),
		bus_space_read_1(sc->sc_bst, chan->ch_bht, DMAC_REG_DFCR),
		bus_space_read_1(sc->sc_bst, chan->ch_bht, DMAC_REG_MFCR),
		bus_space_read_1(sc->sc_bst, chan->ch_bht, DMAC_REG_BFCR));
	printf("DAR=%08x, MAR=%08x, BAR=%08x\n",
		bus_space_read_4(sc->sc_bst, chan->ch_bht, DMAC_REG_DAR),
		bus_space_read_4(sc->sc_bst, chan->ch_bht, DMAC_REG_MAR),
		bus_space_read_4(sc->sc_bst, chan->ch_bht, DMAC_REG_BAR));

	return 0;
}
#endif
