/*	$NetBSD: intio_dmac.c,v 1.11 2001/05/27 02:18:07 minoura Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include "opt_m680x0.h"

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
#define DPRINTF(n,x)	if (dmacdebug>(n)&0x0f) printf x
#define DDUMPREGS(n,x)	if (dmacdebug>(n)&0x0f) {printf x; dmac_dump_regs();}
int dmacdebug = 0;
#else
#define DPRINTF(n,x)
#define DDUMPREGS(n,x)
#endif

static void dmac_init_channels __P((struct dmac_softc*));
#ifdef DMAC_ARRAYCHAIN
static int dmac_program_arraychain __P((struct device*, struct dmac_dma_xfer*,
					u_int, u_int));
#endif
static int dmac_done __P((void*));
static int dmac_error __P((void*));

#ifdef DMAC_DEBUG
static int dmac_dump_regs __P((void));
#endif

/*
 * autoconf stuff
 */
static int dmac_match __P((struct device *, struct cfdata *, void *));
static void dmac_attach __P((struct device *, struct device *, void *));

struct cfattach dmac_ca = {
	sizeof(struct dmac_softc), dmac_match, dmac_attach
};

static int
dmac_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct intio_attach_args *ia = aux;

	if (strcmp (ia->ia_name, "dmac") != 0)
		return (0);
	if (cf->cf_unit != 0)
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
dmac_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct dmac_softc *sc = (struct dmac_softc *)self;
	struct intio_attach_args *ia = aux;
	int r;

	ia->ia_size = DMAC_CHAN_SIZE * DMAC_NCHAN;
	r = intio_map_allocate_region (parent, ia, INTIO_MAP_ALLOCATE);
#ifdef DIAGNOSTIC
	if (r)
		panic ("IO map for DMAC corruption??");
#endif

	((struct intio_softc*) parent)->sc_dmac = self;
	sc->sc_bst = ia->ia_bst;
	bus_space_map (sc->sc_bst, ia->ia_addr, ia->ia_size, 0, &sc->sc_bht);
	dmac_init_channels(sc);

	printf (": HD63450 DMAC\n%s: 4 channels available.\n", self->dv_xname);
}

static void
dmac_init_channels(sc)
	struct dmac_softc *sc;
{
	int i;
	pmap_t pmap = pmap_kernel();

	DPRINTF (3, ("dmac_init_channels\n"));
	for (i=0; i<DMAC_NCHAN; i++) {
		sc->sc_channels[i].ch_channel = i;
		sc->sc_channels[i].ch_name[0] = 0;
		sc->sc_channels[i].ch_softc = &sc->sc_dev;
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
dmac_alloc_channel(self, ch, name,
		   normalv, normal, normalarg,
		   errorv, error, errorarg)
	struct device *self;
	int ch;
	char *name;
	int normalv, errorv;
	dmac_intr_handler_t normal, error;
	void *normalarg, *errorarg;
{
	struct intio_softc *intio = (void*) self;
	struct dmac_softc *sc = (void*) intio->sc_dmac;
	struct dmac_channel_stat *chan = &sc->sc_channels[ch];
	char intrname[16];
	int r, dummy;

	printf ("%s: allocating ch %d for %s.\n",
		sc->sc_dev.dv_xname, ch, name);
	DPRINTF (3, ("dmamap=%p\n", (void*) chan->ch_xfer.dx_dmamap));
#ifdef DIAGNOSTIC
	if (ch < 0 || ch >= DMAC_NCHAN)
		panic ("Invalid DMAC channel.");
	if (chan->ch_name[0])
		panic ("DMAC: channel in use.");
	if (strlen(name) > 8)
	  	panic ("DMAC: wrong user name.");
#endif

#ifdef DMAC_ARRAYCHAIN
	/* allocate the DMAC arraychaining map */
	r = bus_dmamem_alloc(intio->sc_dmat,
			     sizeof(struct dmac_sg_array) * DMAC_MAPSIZE,
			     4, 0, &chan->ch_seg[0], 1, &dummy,
			     BUS_DMA_NOWAIT);
	if (r)
		panic ("DMAC: cannot alloc DMA safe memory");
	r = bus_dmamem_map(intio->sc_dmat,
			   &chan->ch_seg[0], 1,
			   sizeof(struct dmac_sg_array) * DMAC_MAPSIZE,
			   (caddr_t*) &chan->ch_map,
			   BUS_DMA_NOWAIT|BUS_DMA_COHERENT);
	if (r)
		panic ("DMAC: cannot map DMA safe memory");
#endif

	/* fill the channel status structure by the default values. */
	strcpy(chan->ch_name, name);
	chan->ch_dcr = (DMAC_DCR_XRM_CSWH | DMAC_DCR_OTYP_EASYNC |
			DMAC_DCR_OPS_8BIT);
	chan->ch_ocr = (DMAC_OCR_SIZE_BYTE | DMAC_OCR_REQG_EXTERNAL);
	chan->ch_normalv = normalv;
	chan->ch_errorv = errorv;
	chan->ch_normal = normal;
	chan->ch_error = error;
	chan->ch_normalarg = normalarg;
	chan->ch_errorarg = errorarg;
	chan->ch_xfer.dx_dmamap = 0;

	/* setup the device-specific registers */
	bus_space_write_1 (sc->sc_bst, chan->ch_bht, DMAC_REG_CSR, 0xff);
	bus_space_write_1 (sc->sc_bst, chan->ch_bht,
			   DMAC_REG_DCR, chan->ch_dcr);
	bus_space_write_1 (sc->sc_bst, chan->ch_bht, DMAC_REG_CPR, 0);

	/*
	 * X68k physical user space is a subset of the kernel space;
	 * the memory is always included in the physical user space,
	 * while the device is not.
	 */
	bus_space_write_1 (sc->sc_bst, chan->ch_bht,
			   DMAC_REG_BFCR, DMAC_FC_USER_DATA);
	bus_space_write_1 (sc->sc_bst, chan->ch_bht,
			   DMAC_REG_MFCR, DMAC_FC_USER_DATA);
	bus_space_write_1 (sc->sc_bst, chan->ch_bht,
			   DMAC_REG_DFCR, DMAC_FC_KERNEL_DATA);

	/* setup the interrupt handlers */
	bus_space_write_1 (sc->sc_bst, chan->ch_bht, DMAC_REG_NIVR, normalv);
	bus_space_write_1 (sc->sc_bst, chan->ch_bht, DMAC_REG_EIVR, errorv);

	strcpy(intrname, name);
	strcat(intrname, "dma");
	intio_intr_establish (normalv, intrname, dmac_done, chan);

	strcpy(intrname, name);
	strcat(intrname, "dmaerr");
	intio_intr_establish (errorv, intrname, dmac_error, chan);

	return chan;
}

int
dmac_free_channel(self, ch, channel)
	struct device *self;
	int ch;
	void *channel;
{
	struct intio_softc *intio = (void*) self;
	struct dmac_softc *sc = (void*) intio->sc_dmac;
	struct dmac_channel_stat *chan = &sc->sc_channels[ch];

	DPRINTF (3, ("dmac_free_channel, %d\n", ch));
	DPRINTF (3, ("dmamap=%p\n", (void*) chan->ch_xfer.dx_dmamap));
	if (chan != channel)
		return -1;
	if (ch != chan->ch_channel)
		return -1;

#ifdef DMAC_ARRAYCHAIN
	bus_dmamem_unmap(intio->sc_dmat, (caddr_t) chan->ch_map,
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
dmac_alloc_xfer (chan, dmat, dmamap)
	struct dmac_channel_stat *chan;
	bus_dma_tag_t dmat;
	bus_dmamap_t dmamap;
{
	struct dmac_dma_xfer *xf = &chan->ch_xfer;
	struct dmac_softc *sc = (struct dmac_softc*) chan->ch_softc;

	DPRINTF (3, ("dmac_alloc_xfer\n"));
	xf->dx_channel = chan;
	xf->dx_dmamap = dmamap;
	xf->dx_tag = dmat;
#ifdef DMAC_ARRAYCHAIN
	xf->dx_array = chan->ch_map;
	xf->dx_done = 0;
#endif
	xf->dx_nextoff = xf->dx_nextsize = -1;
	return xf;
}

int
dmac_load_xfer (self, xf)
	struct device *self;
	struct dmac_dma_xfer *xf;
{
	struct dmac_softc *sc = (void*) self;
	struct dmac_channel_stat *chan = xf->dx_channel;

	DPRINTF (3, ("dmac_load_xfer\n"));

	xf->dx_ocr &= ~DMAC_OCR_CHAIN_MASK;
	if (xf->dx_dmamap->dm_nsegs == 1)
		xf->dx_ocr |= DMAC_OCR_CHAIN_DISABLED;
	else {
		xf->dx_ocr |= DMAC_OCR_CHAIN_ARRAY;
		xf->dx_nextoff = ~0;
		xf->dx_nextsize = ~0;
	}

	bus_space_write_1(sc->sc_bst, chan->ch_bht, DMAC_REG_CSR, 0xff);
	bus_space_write_1(sc->sc_bst, chan->ch_bht, DMAC_REG_SCR, xf->dx_scr);
	bus_space_write_1(sc->sc_bst, chan->ch_bht,
			  DMAC_REG_OCR, (xf->dx_ocr | chan->ch_ocr));
	bus_space_write_4(sc->sc_bst, chan->ch_bht,
			  DMAC_REG_DAR, (int) xf->dx_device);

	return 0;
}

struct dmac_dma_xfer *
dmac_prepare_xfer (chan, dmat, dmamap, dir, scr, dar)
	struct dmac_channel_stat *chan;
	bus_dma_tag_t dmat;
	bus_dmamap_t dmamap;
	int dir, scr;
	void *dar;
{
	struct dmac_dma_xfer *xf;
	struct dmac_softc *sc = (struct dmac_softc*) chan->ch_softc;

	xf = dmac_alloc_xfer(chan, dmat, dmamap);

	xf->dx_ocr = dir & DMAC_OCR_DIR_MASK;
	xf->dx_scr = scr & (DMAC_SCR_MAC_MASK|DMAC_SCR_DAC_MASK);
	xf->dx_device = dar;

	dmac_load_xfer(&sc->sc_dev, xf);

	return xf;
}

#ifdef DMAC_DEBUG
static struct dmac_channel_stat *debugchan = 0;
#endif

#ifdef DMAC_DEBUG
static u_int8_t dcsr, dcer, ddcr, docr, dscr, dccr, dcpr, dgcr,
  dnivr, deivr, ddfcr, dmfcr, dbfcr;
static u_int16_t dmtcr, dbtcr;
static u_int32_t ddar, dmar, dbar;
#endif
/*
 * Do the actual transfer.
 */
int
dmac_start_xfer(self, xf)
	struct device *self;
	struct dmac_dma_xfer *xf;
{
	return dmac_start_xfer_offset(self, xf, 0, 0);
}

int
dmac_start_xfer_offset(self, xf, offset, size)
	struct device *self;
	struct dmac_dma_xfer *xf;
	u_int offset;
	u_int size;
{
	struct dmac_softc *sc = (void*) self;
	struct dmac_channel_stat *chan = xf->dx_channel;
	struct x68k_bus_dmamap *dmamap = xf->dx_dmamap;
	int c, go = DMAC_CCR_STR|DMAC_CCR_INT;

	DPRINTF (3, ("dmac_start_xfer\n"));
#ifdef DMAC_DEBUG
	debugchan=chan;
#endif

	if (size == 0) {
#ifdef DIAGNOSTIC
		if (offset != 0)
			panic ("dmac_start_xfer_offset: invalid offset %x",
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
	DPRINTF (3, ("First program:\n"));
#ifdef DIAGNOSTIC
	if ((offset >= dmamap->dm_mapsize) ||
	    (offset + size > dmamap->dm_mapsize))
		panic ("dmac_start_xfer_offset: invalid offset: "
			"offset=%d, size=%d, mapsize=%d",
		       offset, size, dmamap->dm_mapsize);
#endif
	/* program DMAC in single block mode or array chainning mode */
	if (dmamap->dm_nsegs == 1) {
		DPRINTF(3, ("single block mode\n"));
#ifdef DIAGNOSTIC
		if (dmamap->dm_mapsize != dmamap->dm_segs[0].ds_len)
			panic ("dmac_start_xfer_offset: dmamap curruption");
#endif
		if (offset == xf->dx_nextoff &&
		    size == xf->dx_nextsize) {
			/* Use continued operation */
			go |=  DMAC_CCR_CNT;
			xf->dx_nextoff += size;
		} else {
			bus_space_write_4(sc->sc_bst, chan->ch_bht,
					  DMAC_REG_MAR,
					  (int) dmamap->dm_segs[0].ds_addr
					  + offset);
			bus_space_write_2(sc->sc_bst, chan->ch_bht,
					  DMAC_REG_MTCR, (int) size);
			xf->dx_nextoff = offset;
			xf->dx_nextsize = size;
		}
#ifdef DMAC_ARRAYCHAIN
		xf->dx_done = 1;
#endif
	} else {
#ifdef DMAC_ARRAYCHAIN
		c = dmac_program_arraychain(self, xf, offset, size);
		bus_space_write_4(sc->sc_bst, chan->ch_bht,
				  DMAC_REG_BAR, (int) chan->ch_seg[0].ds_addr);
		bus_space_write_2(sc->sc_bst, chan->ch_bht,
				  DMAC_REG_BTCR, c);
#else
		panic ("DMAC: unexpected use of arraychaining mode");
#endif
	}

	bus_space_write_1(sc->sc_bst, chan->ch_bht, DMAC_REG_CSR, 0xff);

	/* START!! */
	DDUMPREGS (3, ("first start\n"));
#ifdef DMAC_DEBUG
	dcsr = bus_space_read_1(sc->sc_bst, chan->ch_bht, DMAC_REG_CSR);
	dcer = bus_space_read_1(sc->sc_bst, chan->ch_bht, DMAC_REG_CER);
	ddcr = bus_space_read_1(sc->sc_bst, chan->ch_bht, DMAC_REG_DCR);
	docr = bus_space_read_1(sc->sc_bst, chan->ch_bht, DMAC_REG_OCR);
	dscr = bus_space_read_1(sc->sc_bst, chan->ch_bht, DMAC_REG_SCR);
	dccr = bus_space_read_1(sc->sc_bst, chan->ch_bht, DMAC_REG_CCR);
	dcpr = bus_space_read_1(sc->sc_bst, chan->ch_bht, DMAC_REG_CPR);
	dgcr = bus_space_read_1(sc->sc_bst, chan->ch_bht, DMAC_REG_GCR);
	dnivr = bus_space_read_1(sc->sc_bst, chan->ch_bht, DMAC_REG_NIVR);
	deivr = bus_space_read_1(sc->sc_bst, chan->ch_bht, DMAC_REG_EIVR);
	ddfcr = bus_space_read_1(sc->sc_bst, chan->ch_bht, DMAC_REG_DFCR);
	dmfcr = bus_space_read_1(sc->sc_bst, chan->ch_bht, DMAC_REG_MFCR);
	dbfcr = bus_space_read_1(sc->sc_bst, chan->ch_bht, DMAC_REG_BFCR);
	dmtcr = bus_space_read_2(sc->sc_bst, chan->ch_bht, DMAC_REG_MTCR);
	dbtcr = bus_space_read_2(sc->sc_bst, chan->ch_bht, DMAC_REG_BTCR);
	ddar = bus_space_read_4(sc->sc_bst, chan->ch_bht, DMAC_REG_DAR);
	dmar = bus_space_read_4(sc->sc_bst, chan->ch_bht, DMAC_REG_MAR);
	dbar = bus_space_read_4(sc->sc_bst, chan->ch_bht, DMAC_REG_BAR);
#endif
#ifdef DMAC_ARRAYCHAIN
#if defined(M68040) || defined(M68060)
	/* flush data cache for the map */
	if (dmamap->dm_nsegs != 1 && mmutype == MMU_68040)
		dma_cachectl((caddr_t) xf->dx_array,
			     sizeof(struct dmac_sg_array) * c);
#endif
#endif
	bus_space_write_1(sc->sc_bst, chan->ch_bht, DMAC_REG_CCR, go);

	if (xf->dx_nextoff != ~0) {
		bus_space_write_4(sc->sc_bst, chan->ch_bht,
				  DMAC_REG_BAR, xf->dx_nextoff);
		bus_space_write_2(sc->sc_bst, chan->ch_bht,
				  DMAC_REG_BTCR, xf->dx_nextsize);
	}

	return 0;
}

#ifdef DMAC_ARRAYCHAIN
static int
dmac_program_arraychain(self, xf, offset, size)
	struct device *self;
	struct dmac_dma_xfer *xf;
	u_int offset;
	u_int size;
{
	struct dmac_channel_stat *chan = xf->dx_channel;
	int ch = chan->ch_channel;
	struct x68k_bus_dmamap *map = xf->dx_dmamap;
	int i, j;

	/* XXX not yet!! */
	if (offset != 0 || size != map->dm_mapsize)
		panic ("dmac_program_arraychain: unsupported offset/size");

	DPRINTF (3, ("dmac_program_arraychain\n"));
	for (i=0, j=xf->dx_done; i<DMAC_MAPSIZE && j<map->dm_nsegs;
	     i++, j++) {
		xf->dx_array[i].da_addr = map->dm_segs[j].ds_addr;
#ifdef DIAGNOSTIC
		if (map->dm_segs[j].ds_len > DMAC_MAXSEGSZ)
			panic ("dmac_program_arraychain: wrong map: %ld",
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
dmac_done(arg)
	void *arg;
{
	struct dmac_channel_stat *chan = arg;
	struct dmac_softc *sc = (void*) chan->ch_softc;
	struct dmac_dma_xfer *xf = &chan->ch_xfer;
	struct x68k_bus_dmamap *map = xf->dx_dmamap;
	int c;

	DPRINTF (3, ("dmac_done\n"));

	bus_space_write_1(sc->sc_bst, chan->ch_bht, DMAC_REG_CSR, 0xff);

#ifdef DMAC_ARRAYCHAIN
	if (xf->dx_done == map->dm_nsegs) {
		xf->dx_done = 0;
#endif
		/* Done */
		return (*chan->ch_normal) (chan->ch_normalarg);
#ifdef DMAC_ARRAYCHAIN
	}
#endif

#ifdef DMAC_ARRAYCHAIN
	/* Continue transfer */
	DPRINTF (3, ("reprograming\n"));
	c = dmac_program_arraychain (&sc->sc_dev, xf, 0, map->dm_mapsize);

	bus_space_write_1(sc->sc_bst, chan->ch_bht, DMAC_REG_CSR, 0xff);
	bus_space_write_4(sc->sc_bst, chan->ch_bht,
			  DMAC_REG_BAR, (int) chan->ch_map);
	bus_space_write_4(sc->sc_bst, chan->ch_bht,
			  DMAC_REG_DAR, (int) xf->dx_device);
	bus_space_write_2(sc->sc_bst, chan->ch_bht, DMAC_REG_BTCR, c);

	/* START!! */
	DDUMPREGS (3, ("restart\n"));
	bus_space_write_1(sc->sc_bst, chan->ch_bht,
			  DMAC_REG_CCR, DMAC_CCR_STR|DMAC_CCR_INT);

	return 1;
#endif
}

static int
dmac_error(arg)
	void *arg;
{
	struct dmac_channel_stat *chan = arg;
	struct dmac_softc *sc = (void*) chan->ch_softc;

	printf ("DMAC transfer error CSR=%02x, CER=%02x\n",
		bus_space_read_1(sc->sc_bst, chan->ch_bht, DMAC_REG_CSR),
		bus_space_read_1(sc->sc_bst, chan->ch_bht, DMAC_REG_CER));
	DPRINTF(5, ("registers were:\n"));
#ifdef DMAC_DEBUG
	if ((dmacdebug & 0x0f) > 5) {
		printf ("CSR=%02x, CER=%02x, DCR=%02x, OCR=%02x, SCR=%02x, "
			"CCR=%02x, CPR=%02x, GCR=%02x\n",
			dcsr, dcer, ddcr, docr, dscr, dccr, dcpr, dgcr);
		printf ("NIVR=%02x, EIVR=%02x, MTCR=%04x, BTCR=%04x, "
			"DFCR=%02x, MFCR=%02x, BFCR=%02x\n",
			dnivr, deivr, dmtcr, dbtcr, ddfcr, dmfcr, dbfcr);
		printf ("DAR=%08x, MAR=%08x, BAR=%08x\n",
			ddar, dmar, dbar);
	}
#endif

	/* Clear the status bits */
	bus_space_write_1(sc->sc_bst, chan->ch_bht, DMAC_REG_CSR, 0xff);
	DDUMPREGS(3, ("dmac_error\n"));

#ifdef DMAC_ARRAYCHAIN
	chan->ch_xfer.dx_done = 0;
#endif

	return (*chan->ch_error) (chan->ch_errorarg);
}

int
dmac_abort_xfer(self, xf)
	struct device *self;
	struct dmac_dma_xfer *xf;
{
	struct dmac_softc *sc = (void*) self;
	struct dmac_channel_stat *chan = xf->dx_channel;
	struct x68k_bus_dmamap *dmamap = xf->dx_dmamap;

	bus_space_write_1(sc->sc_bst, chan->ch_bht, DMAC_REG_CCR,
			  DMAC_CCR_INT | DMAC_CCR_HLT);
	bus_space_write_1(sc->sc_bst, chan->ch_bht, DMAC_REG_CSR, 0xff);
	xf->dx_nextoff = xf->dx_nextsize = -1;

	return 0;
}

#ifdef DMAC_DEBUG
static int
dmac_dump_regs(void)
{
	struct dmac_channel_stat *chan = debugchan;
	struct dmac_softc *sc;

	if ((chan == 0) || (dmacdebug & 0xf0)) return;
	sc = (void*) chan->ch_softc;

	printf ("DMAC channel %d registers\n", chan->ch_channel);
	printf ("CSR=%02x, CER=%02x, DCR=%02x, OCR=%02x, SCR=%02x,"
		"CCR=%02x, CPR=%02x, GCR=%02x\n",
		bus_space_read_1(sc->sc_bst, chan->ch_bht, DMAC_REG_CSR),
		bus_space_read_1(sc->sc_bst, chan->ch_bht, DMAC_REG_CER),
		bus_space_read_1(sc->sc_bst, chan->ch_bht, DMAC_REG_DCR),
		bus_space_read_1(sc->sc_bst, chan->ch_bht, DMAC_REG_OCR),
		bus_space_read_1(sc->sc_bst, chan->ch_bht, DMAC_REG_SCR),
		bus_space_read_1(sc->sc_bst, chan->ch_bht, DMAC_REG_CCR),
		bus_space_read_1(sc->sc_bst, chan->ch_bht, DMAC_REG_CPR),
		bus_space_read_1(sc->sc_bst, chan->ch_bht, DMAC_REG_GCR));
	printf ("NIVR=%02x, EIVR=%02x, MTCR=%04x, BTCR=%04x, DFCR=%02x,"
		"MFCR=%02x, BFCR=%02x\n",
		bus_space_read_1(sc->sc_bst, chan->ch_bht, DMAC_REG_NIVR),
		bus_space_read_1(sc->sc_bst, chan->ch_bht, DMAC_REG_EIVR),
		bus_space_read_2(sc->sc_bst, chan->ch_bht, DMAC_REG_MTCR),
		bus_space_read_2(sc->sc_bst, chan->ch_bht, DMAC_REG_BTCR),
		bus_space_read_1(sc->sc_bst, chan->ch_bht, DMAC_REG_DFCR),
		bus_space_read_1(sc->sc_bst, chan->ch_bht, DMAC_REG_MFCR),
		bus_space_read_1(sc->sc_bst, chan->ch_bht, DMAC_REG_BFCR));
	printf ("DAR=%08x, MAR=%08x, BAR=%08x\n",
		bus_space_read_4(sc->sc_bst, chan->ch_bht, DMAC_REG_DAR),
		bus_space_read_4(sc->sc_bst, chan->ch_bht, DMAC_REG_MAR),
		bus_space_read_4(sc->sc_bst, chan->ch_bht, DMAC_REG_BAR));

	return 0;
}
#endif
