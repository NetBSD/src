/*	$NetBSD: intio_dmac.c,v 1.1.2.3 1999/02/10 16:02:26 minoura Exp $	*/

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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/extent.h>
#include <vm/vm.h>

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
static int dmac_program_arraychain __P((struct device*, struct dmac_dma_xfer*));
static int dmac_done __P((void*));
static int dmac_error __P((void*));

static int dmac_dump_regs __P((void));

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

	/* fixed address */
	if (ia->ia_addr != DMAC_ADDR)
		return (0);
	if (ia->ia_intr != -1)
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

#define DMAC_MAPSIZE 64
/* Allocate statically in order to make sure the DMAC can reach the maps. */
static struct dmac_sg_array dmac_map[DMAC_NCHAN][DMAC_MAPSIZE];

static void
dmac_init_channels(sc)
	struct dmac_softc *sc;
{
	int i;
	pmap_t pmap = pmap_kernel();

	for (i=0; i<DMAC_NCHAN; i++) {
		sc->sc_channels[i].ch_channel = i;
		sc->sc_channels[i].ch_name[0] = 0;
		sc->sc_channels[i].ch_softc = &sc->sc_dev;
		sc->sc_channels[i].ch_map =
		  (void*) pmap_extract (pmap, (vaddr_t) &dmac_map[i]);
		bus_space_subregion(sc->sc_bst, sc->sc_bht,
				    DMAC_CHAN_SIZE*i, DMAC_CHAN_SIZE,
				    &sc->sc_channels[i].ch_bht);
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

#ifdef DIAGNOSTIC
	if (ch < 0 || ch >= DMAC_NCHAN)
		panic ("Invalid DMAC channel.");
	if (chan->ch_name[0])
		panic ("DMAC: channel in use.");
	if (strlen(name) > 8)
	  	panic ("DMAC: wrong user name.");
#endif

	/* fill the channel status structure. */
	strcpy(chan->ch_name, name);
	chan->ch_dcr = (DMAC_DCR_XRM_CSWOH | DMAC_DCR_OTYP_EASYNC |
			DMAC_DCR_OPS_8BIT);
	chan->ch_ocr = (DMAC_OCR_SIZE_BYTE_NOPACK | DMAC_OCR_CHAIN_ARRAY |
			DMAC_OCR_REQG_EXTERNAL);
	chan->ch_normalv = normalv;
	chan->ch_errorv = errorv;
	chan->ch_normal = normal;
	chan->ch_error = error;
	chan->ch_normalarg = normalarg;
	chan->ch_errorarg = errorarg;
	chan->ch_xfer_in_progress = 0;

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
	struct dmac_softc *sc = (void*) self;
	struct dmac_channel_stat *chan = &sc->sc_channels[ch];

	if (chan != channel)
		return -1;
	if (ch != chan->ch_channel)
		return -1;
#if DIAGNOSTIC
	if (chan->ch_xfer_in_progress)
		panic ("dmac_free_channel: DMA transfer in progress");
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
dmac_prepare_xfer (chan, dmat, dmamap, dir, scr, dar)
	struct dmac_channel_stat *chan;
	bus_dma_tag_t dmat;
	bus_dmamap_t dmamap;
	int dir, scr;
	void *dar;
{
	struct dmac_dma_xfer *r = malloc (sizeof (struct dmac_dma_xfer),
					  M_DEVBUF, M_WAITOK);

	r->dx_channel = chan;
	r->dx_dmamap = dmamap;
	r->dx_tag = dmat;
	r->dx_ocr = dir & DMAC_OCR_DIR_MASK;
	r->dx_scr = scr & (DMAC_SCR_MAC_MASK|DMAC_SCR_DAC_MASK);
	r->dx_device = dar;
	r->dx_done = 0;

	return r;
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
	struct dmac_softc *sc = (void*) self;
	struct dmac_channel_stat *chan = xf->dx_channel;
	int c;


#ifdef DMAC_DEBUG
	debugchan=chan;
#endif
	bus_space_write_1(sc->sc_bst, chan->ch_bht,
			  DMAC_REG_OCR, (xf->dx_ocr | chan->ch_ocr));
	bus_space_write_1(sc->sc_bst, chan->ch_bht,
			  DMAC_REG_SCR, xf->dx_scr);

	/* program DMAC in array chainning mode */
	xf->dx_done = 0;
	DPRINTF (3, ("First program:\n"));
	c = dmac_program_arraychain(self, xf);

	/* setup the address/count registers */
	bus_space_write_4(sc->sc_bst, chan->ch_bht,
			  DMAC_REG_BAR, (int) chan->ch_map);
	bus_space_write_4(sc->sc_bst, chan->ch_bht,
			  DMAC_REG_DAR, (int) xf->dx_device);
	bus_space_write_1(sc->sc_bst, chan->ch_bht,
			  DMAC_REG_CSR, 0xff);
	bus_space_write_2(sc->sc_bst, chan->ch_bht,
			  DMAC_REG_BTCR, c);

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
#if defined(M68040) || defined(M68060)
	if (mmutype == MMU_68040)
		dma_cachectl((caddr_t) &dmac_map[chan->ch_channel],
			     sizeof(struct dmac_sg_array)*DMAC_MAPSIZE);
#endif
	bus_space_write_1(sc->sc_bst, chan->ch_bht,
			  DMAC_REG_CCR, DMAC_CCR_STR|DMAC_CCR_INT);
	chan->ch_xfer_in_progress = xf;

	return 0;
}

static int
dmac_program_arraychain(self, xf)
	struct device *self;
	struct dmac_dma_xfer *xf;
{
	struct dmac_softc *sc = (void*) self;
	struct dmac_channel_stat *chan = xf->dx_channel;
	int ch = chan->ch_channel;
	struct x68k_bus_dmamap *map = xf->dx_dmamap;
	int i, j;

	for (i=0, j=xf->dx_done; i<DMAC_MAPSIZE && j<map->dm_nsegs;
	     i++, j++) {
		dmac_map[ch][i].da_addr = map->dm_segs[j].ds_addr;
#ifdef DIAGNOSTIC
		if (map->dm_segs[j].ds_len > 0xff00)
			panic ("dmac_program_arraychain: wrong map: %d", map->dm_segs[j].ds_len);
#endif
		dmac_map[ch][i].da_count = map->dm_segs[j].ds_len;
	}
	xf->dx_done = j;

	return i;
}

/*
 * interrupt handlers.
 */
static int
dmac_done(arg)
	void *arg;
{
	struct dmac_channel_stat *chan = arg;
	struct dmac_softc *sc = (void*) chan->ch_softc;
	struct dmac_dma_xfer *xf = chan->ch_xfer_in_progress;
	struct x68k_bus_dmamap *map = xf->dx_dmamap;
	int c;

	DPRINTF (3, ("dmac_done\n"));
	bus_space_write_1(sc->sc_bst, chan->ch_bht, DMAC_REG_CSR, 0xff);

	if (xf->dx_done == map->dm_nsegs) {
		/* Done */
		chan->ch_xfer_in_progress = 0;
		return (*chan->ch_normal) (chan->ch_normalarg);
	}

	/* Continue transfer */
	DPRINTF (3, ("reprograming\n"));
	c = dmac_program_arraychain (&sc->sc_dev, xf);

	bus_space_write_4(sc->sc_bst, chan->ch_bht,
			  DMAC_REG_BAR, (int) chan->ch_map);
	bus_space_write_4(sc->sc_bst, chan->ch_bht,
			  DMAC_REG_DAR, (int) xf->dx_device);
	bus_space_write_1(sc->sc_bst, chan->ch_bht,
			  DMAC_REG_CSR, 0xff);
	bus_space_write_2(sc->sc_bst, chan->ch_bht,
			  DMAC_REG_BTCR, c);

	/* START!! */
	DDUMPREGS (3, ("restart\n"));
	bus_space_write_1(sc->sc_bst, chan->ch_bht,
			  DMAC_REG_CCR, DMAC_CCR_STR|DMAC_CCR_INT);

	return 1;
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
		printf ("CSR=%02x, CER=%02x, DCR=%02x, OCR=%02x, SCR=%02x,"
			"CCR=%02x, CPR=%02x, GCR=%02x\n",
			dcsr, dcer, ddcr, docr, dscr, dccr, dcpr, dgcr);
		printf ("NIVR=%02x, EIVR=%02x, MTCR=%04x, BTCR=%04x, "
			"DFCR=%02x, MFCR=%02x, BFCR=%02x\n",
			dnivr, deivr, dmtcr, dbtcr, ddfcr, dmfcr, dbfcr);
		printf ("DAR=%08x, MAR=%08x, BAR=%08x\n",
			ddar, dmar, dbar);
	}
#endif

	bus_space_write_1(sc->sc_bst, chan->ch_bht, DMAC_REG_CSR, 0xff);
	DDUMPREGS(3, ("dmac_error\n"));

	return (*chan->ch_error) (chan->ch_errorarg);
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
