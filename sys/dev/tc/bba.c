/* $NetBSD: bba.c,v 1.45 2020/08/29 03:24:31 isaki Exp $ */

/*
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
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

/* maxine/alpha baseboard audio (bba) */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bba.c,v 1.45 2020/08/29 03:24:31 isaki Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/kmem.h>

#include <sys/bus.h>
#include <machine/autoconf.h>
#include <sys/cpu.h>

#include <sys/audioio.h>
#include <dev/audio/audio_if.h>

#include <dev/ic/am7930reg.h>
#include <dev/ic/am7930var.h>

#include <dev/tc/tcvar.h>
#include <dev/tc/ioasicreg.h>
#include <dev/tc/ioasicvar.h>

/* include mulaw.c (not .h file) here to expand mulaw32 */
void audio_mulaw32_to_internal(audio_filter_arg_t *);
void audio_internal_to_mulaw32(audio_filter_arg_t *);
#define MULAW32
#include <dev/audio/mulaw.c>

#ifdef AUDIO_DEBUG
#define DPRINTF(x)	if (am7930debug) printf x
#else
#define DPRINTF(x)
#endif  /* AUDIO_DEBUG */

#define BBA_MAX_DMA_SEGMENTS	16
#define BBA_DMABUF_SIZE		(BBA_MAX_DMA_SEGMENTS*IOASIC_DMA_BLOCKSIZE)
#define BBA_DMABUF_ALIGN	IOASIC_DMA_BLOCKSIZE
#define BBA_DMABUF_BOUNDARY	0

struct bba_mem {
	struct bba_mem *next;
	bus_addr_t addr;
	bus_size_t size;
	void *kva;
};

struct bba_dma_state {
	bus_dmamap_t dmam;		/* DMA map */
	int active;
	int curseg;			/* current segment in DMA buffer */
	void (*intr)(void *);		/* higher-level audio handler */
	void *intr_arg;
};

struct bba_softc {
	struct am7930_softc sc_am7930;		/* glue to MI code */

	bus_space_tag_t sc_bst;			/* IOASIC bus tag/handle */
	bus_space_handle_t sc_bsh;
	bus_dma_tag_t sc_dmat;
	bus_space_handle_t sc_codec_bsh;	/* codec bus space handle */

	struct bba_mem *sc_mem_head;		/* list of buffers */

	struct bba_dma_state sc_tx_dma_state;
	struct bba_dma_state sc_rx_dma_state;
};

static int	bba_match(device_t, cfdata_t, void *);
static void	bba_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(bba, sizeof(struct bba_softc),
    bba_match, bba_attach, NULL, NULL);

/*
 * Define our interface into the am7930 MI driver.
 */

static uint8_t	bba_codec_iread(struct am7930_softc *, int);
static uint16_t	bba_codec_iread16(struct am7930_softc *, int);
static void	bba_codec_iwrite(struct am7930_softc *, int, uint8_t);
static void	bba_codec_iwrite16(struct am7930_softc *, int, uint16_t);
static void	bba_onopen(struct am7930_softc *);
static void	bba_onclose(struct am7930_softc *);

struct am7930_glue bba_glue = {
	bba_codec_iread,
	bba_codec_iwrite,
	bba_codec_iread16,
	bba_codec_iwrite16,
	bba_onopen,
	bba_onclose,
};

/*
 * Define our interface to the higher level audio driver.
 */

static int	bba_query_format(void *, audio_format_query_t *);
static int	bba_set_format(void *, int,
				const audio_params_t *, const audio_params_t *,
				audio_filter_reg_t *, audio_filter_reg_t *);
static int	bba_round_blocksize(void *, int, int, const audio_params_t *);
static int	bba_halt_output(void *);
static int	bba_halt_input(void *);
static int	bba_getdev(void *, struct audio_device *);
static void	*bba_allocm(void *, int, size_t);
static void	bba_freem(void *, void *, size_t);
static size_t	bba_round_buffersize(void *, int, size_t);
static int	bba_trigger_output(void *, void *, void *, int,
				   void (*)(void *), void *,
				   const audio_params_t *);
static int	bba_trigger_input(void *, void *, void *, int,
				  void (*)(void *), void *,
				  const audio_params_t *);
static void	bba_get_locks(void *opaque, kmutex_t **intr,
			      kmutex_t **thread);

static const struct audio_hw_if sa_hw_if = {
	.open			= am7930_open,
	.close			= am7930_close,
	.query_format		= bba_query_format,
	.set_format		= bba_set_format,
	.round_blocksize	= bba_round_blocksize,	/* md */
	.commit_settings	= am7930_commit_settings,
	.halt_output		= bba_halt_output,	/* md */
	.halt_input		= bba_halt_input,	/* md */
	.getdev			= bba_getdev,
	.set_port		= am7930_set_port,
	.get_port		= am7930_get_port,
	.query_devinfo		= am7930_query_devinfo,
	.allocm			= bba_allocm,		/* md */
	.freem			= bba_freem,		/* md */
	.round_buffersize	= bba_round_buffersize,	/* md */
	.get_props		= am7930_get_props,
	.trigger_output		= bba_trigger_output,	/* md */
	.trigger_input		= bba_trigger_input,	/* md */
	.get_locks		= bba_get_locks,
};

static struct audio_device bba_device = {
	"am7930",
	"x",
	"bba"
};

static const struct audio_format bba_format = {
	.mode		= AUMODE_PLAY | AUMODE_RECORD,
	.encoding	= AUDIO_ENCODING_ULAW, /* XXX */
	.validbits	= 32,
	.precision	= 32,
	.channels	= 1,
	.channel_mask	= AUFMT_MONAURAL,
	.frequency_type	= 1,
	.frequency	= { 8000 },
};

static int	bba_intr(void *);
static void	bba_reset(struct bba_softc *, int);
static void	bba_codec_dwrite(struct am7930_softc *, int, uint8_t);
static uint8_t	bba_codec_dread(struct am7930_softc *, int);

static int
bba_match(device_t parent, cfdata_t cf, void *aux)
{
	struct ioasicdev_attach_args *ia;

	ia = aux;
	if (strcmp(ia->iada_modname, "isdn") != 0 &&
	    strcmp(ia->iada_modname, "AMD79c30") != 0)
		return 0;

	return 1;
}


static void
bba_attach(device_t parent, device_t self, void *aux)
{
	struct ioasicdev_attach_args *ia;
	struct bba_softc *sc;
	struct am7930_softc *asc;
	struct ioasic_softc *iosc = device_private(parent);

	ia = aux;
	sc = device_private(self);
	asc = &sc->sc_am7930;
	asc->sc_dev = self;
	sc->sc_bst = iosc->sc_bst;
	sc->sc_bsh = iosc->sc_bsh;
	sc->sc_dmat = iosc->sc_dmat;

	/* get the bus space handle for codec */
	if (bus_space_subregion(sc->sc_bst, sc->sc_bsh,
	    ia->iada_offset, 0, &sc->sc_codec_bsh)) {
		aprint_error_dev(self, "unable to map device\n");
		return;
	}

	printf("\n");

	bba_reset(sc, 1);

	/*
	 * Set up glue for MI code early; we use some of it here.
	 */
	asc->sc_glue = &bba_glue;

	/*
	 *  MI initialisation.  We will be doing DMA.
	 */
	am7930_init(asc, AUDIOAMD_DMA_MODE);

	ioasic_intr_establish(parent, ia->iada_cookie, TC_IPL_NONE,
	    bba_intr, sc);

	audio_attach_mi(&sa_hw_if, asc, self);
}


static void
bba_onopen(struct am7930_softc *sc)
{
}


static void
bba_onclose(struct am7930_softc *sc)
{
}


static void
bba_reset(struct bba_softc *sc, int reset)
{
	uint32_t ssr;

	/* disable any DMA and reset the codec */
	ssr = bus_space_read_4(sc->sc_bst, sc->sc_bsh, IOASIC_CSR);
	ssr &= ~(IOASIC_CSR_DMAEN_ISDN_T | IOASIC_CSR_DMAEN_ISDN_R);
	if (reset)
		ssr &= ~IOASIC_CSR_ISDN_ENABLE;
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, IOASIC_CSR, ssr);
	DELAY(10);	/* 400ns required for codec to reset */

	/* initialise DMA pointers */
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, IOASIC_ISDN_X_DMAPTR, -1);
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, IOASIC_ISDN_X_NEXTPTR, -1);
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, IOASIC_ISDN_R_DMAPTR, -1);
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, IOASIC_ISDN_R_NEXTPTR, -1);

	/* take out of reset state */
	if (reset) {
		ssr |= IOASIC_CSR_ISDN_ENABLE;
		bus_space_write_4(sc->sc_bst, sc->sc_bsh, IOASIC_CSR, ssr);
	}
}


static void *
bba_allocm(void *addr, int direction, size_t size)
{
	struct am7930_softc *asc;
	struct bba_softc *sc;
	bus_dma_segment_t seg;
	int rseg;
	void *kva;
	struct bba_mem *m;
	int state;

	DPRINTF(("bba_allocm: size = %zu\n", size));
	asc = addr;
	sc = addr;
	state = 0;

	if (bus_dmamem_alloc(sc->sc_dmat, size, BBA_DMABUF_ALIGN,
	    BBA_DMABUF_BOUNDARY, &seg, 1, &rseg, BUS_DMA_WAITOK)) {
		aprint_error_dev(asc->sc_dev, "can't allocate DMA buffer\n");
		goto bad;
	}
	state |= 1;

	if (bus_dmamem_map(sc->sc_dmat, &seg, rseg, size,
	    &kva, BUS_DMA_WAITOK | BUS_DMA_COHERENT)) {
		aprint_error_dev(asc->sc_dev, "can't map DMA buffer\n");
		goto bad;
	}
	state |= 2;

	m = kmem_alloc(sizeof(struct bba_mem), KM_SLEEP);
	m->addr = seg.ds_addr;
	m->size = seg.ds_len;
	m->kva = kva;
	m->next = sc->sc_mem_head;
	sc->sc_mem_head = m;

	return (void *)kva;

bad:
	if (state & 2)
		bus_dmamem_unmap(sc->sc_dmat, kva, size);
	if (state & 1)
		bus_dmamem_free(sc->sc_dmat, &seg, 1);
	return NULL;
}


static void
bba_freem(void *addr, void *ptr, size_t size)
{
	struct bba_softc *sc;
	struct bba_mem **mp, *m;
	bus_dma_segment_t seg;
	void *kva;

	sc = addr;
	kva = (void *)addr;
	for (mp = &sc->sc_mem_head; *mp && (*mp)->kva != kva;
	    mp = &(*mp)->next)
		continue;
	m = *mp;
	if (m == NULL) {
		printf("bba_freem: freeing unallocated memory\n");
		return;
	}
	*mp = m->next;
	bus_dmamem_unmap(sc->sc_dmat, kva, m->size);

	seg.ds_addr = m->addr;
	seg.ds_len = m->size;
	bus_dmamem_free(sc->sc_dmat, &seg, 1);
	kmem_free(m, sizeof(struct bba_mem));
}


static size_t
bba_round_buffersize(void *addr, int direction, size_t size)
{

	DPRINTF(("bba_round_buffersize: size=%zu\n", size));
	return size > BBA_DMABUF_SIZE ? BBA_DMABUF_SIZE :
	    roundup(size, IOASIC_DMA_BLOCKSIZE);
}


static int
bba_halt_output(void *addr)
{
	struct bba_softc *sc;
	struct bba_dma_state *d;
	uint32_t ssr;

	sc = addr;
	d = &sc->sc_tx_dma_state;
	/* disable any DMA */
	ssr = bus_space_read_4(sc->sc_bst, sc->sc_bsh, IOASIC_CSR);
	ssr &= ~IOASIC_CSR_DMAEN_ISDN_T;
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, IOASIC_CSR, ssr);
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, IOASIC_ISDN_X_DMAPTR, -1);
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, IOASIC_ISDN_X_NEXTPTR, -1);

	if (d->active) {
		bus_dmamap_unload(sc->sc_dmat, d->dmam);
		bus_dmamap_destroy(sc->sc_dmat, d->dmam);
		d->active = 0;
	}

	return 0;
}


static int
bba_halt_input(void *addr)
{
	struct bba_softc *sc;
	struct bba_dma_state *d;
	uint32_t ssr;

	sc = addr;
	d = &sc->sc_rx_dma_state;
	/* disable any DMA */
	ssr = bus_space_read_4(sc->sc_bst, sc->sc_bsh, IOASIC_CSR);
	ssr &= ~IOASIC_CSR_DMAEN_ISDN_R;
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, IOASIC_CSR, ssr);
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, IOASIC_ISDN_R_DMAPTR, -1);
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, IOASIC_ISDN_R_NEXTPTR, -1);

	if (d->active) {
		bus_dmamap_unload(sc->sc_dmat, d->dmam);
		bus_dmamap_destroy(sc->sc_dmat, d->dmam);
		d->active = 0;
	}

	return 0;
}


static int
bba_getdev(void *addr, struct audio_device *retp)
{

	*retp = bba_device;
	return 0;
}


static int
bba_trigger_output(void *addr, void *start, void *end, int blksize,
		   void (*intr)(void *), void *arg,
		   const audio_params_t *param)
{
	struct bba_softc *sc;
	struct bba_dma_state *d;
	uint32_t ssr;
	tc_addr_t phys, nphys;
	int state;

	DPRINTF(("bba_trigger_output: sc=%p start=%p end=%p blksize=%d intr=%p(%p)\n",
	    addr, start, end, blksize, intr, arg));
	sc = addr;
	d = &sc->sc_tx_dma_state;
	state = 0;

	/* disable any DMA */
	ssr = bus_space_read_4(sc->sc_bst, sc->sc_bsh, IOASIC_CSR);
	ssr &= ~IOASIC_CSR_DMAEN_ISDN_T;
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, IOASIC_CSR, ssr);

	if (bus_dmamap_create(sc->sc_dmat, (char *)end - (char *)start,
	    BBA_MAX_DMA_SEGMENTS, IOASIC_DMA_BLOCKSIZE,
	    BBA_DMABUF_BOUNDARY, BUS_DMA_NOWAIT, &d->dmam)) {
		printf("bba_trigger_output: can't create DMA map\n");
		goto bad;
	}
	state |= 1;

	if (bus_dmamap_load(sc->sc_dmat, d->dmam, start,
	    (char *)end - (char *)start, NULL, BUS_DMA_WRITE|BUS_DMA_NOWAIT)) {
		printf("bba_trigger_output: can't load DMA map\n");
		goto bad;
	}
	state |= 2;

	d->intr = intr;
	d->intr_arg = arg;
	d->curseg = 1;

	/* get physical address of buffer start */
	phys = (tc_addr_t)d->dmam->dm_segs[0].ds_addr;
	nphys = (tc_addr_t)d->dmam->dm_segs[1].ds_addr;

	/* setup DMA pointer */
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, IOASIC_ISDN_X_DMAPTR,
	    IOASIC_DMA_ADDR(phys));
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, IOASIC_ISDN_X_NEXTPTR,
	    IOASIC_DMA_ADDR(nphys));

	/* kick off DMA */
	ssr |= IOASIC_CSR_DMAEN_ISDN_T;
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, IOASIC_CSR, ssr);

	d->active = 1;

	return 0;

bad:
	if (state & 2)
		bus_dmamap_unload(sc->sc_dmat, d->dmam);
	if (state & 1)
		bus_dmamap_destroy(sc->sc_dmat, d->dmam);
	return 1;
}


static int
bba_trigger_input(void *addr, void *start, void *end, int blksize,
		  void (*intr)(void *), void *arg, const audio_params_t *param)
{
	struct bba_softc *sc;
	struct bba_dma_state *d;
	tc_addr_t phys, nphys;
	uint32_t ssr;
	int state = 0;

	DPRINTF(("bba_trigger_input: sc=%p start=%p end=%p blksize=%d intr=%p(%p)\n",
	    addr, start, end, blksize, intr, arg));
	sc = addr;
	d = &sc->sc_rx_dma_state;
	state = 0;

	/* disable any DMA */
	ssr = bus_space_read_4(sc->sc_bst, sc->sc_bsh, IOASIC_CSR);
	ssr &= ~IOASIC_CSR_DMAEN_ISDN_R;
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, IOASIC_CSR, ssr);

	if (bus_dmamap_create(sc->sc_dmat, (char *)end - (char *)start,
	    BBA_MAX_DMA_SEGMENTS, IOASIC_DMA_BLOCKSIZE,
	    BBA_DMABUF_BOUNDARY, BUS_DMA_NOWAIT, &d->dmam)) {
		printf("bba_trigger_input: can't create DMA map\n");
		goto bad;
	}
	state |= 1;

	if (bus_dmamap_load(sc->sc_dmat, d->dmam, start,
	    (char *)end - (char *)start, NULL, BUS_DMA_READ|BUS_DMA_NOWAIT)) {
		printf("bba_trigger_input: can't load DMA map\n");
		goto bad;
	}
	state |= 2;

	d->intr = intr;
	d->intr_arg = arg;
	d->curseg = 1;

	/* get physical address of buffer start */
	phys = (tc_addr_t)d->dmam->dm_segs[0].ds_addr;
	nphys = (tc_addr_t)d->dmam->dm_segs[1].ds_addr;

	/* setup DMA pointer */
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, IOASIC_ISDN_R_DMAPTR,
	    IOASIC_DMA_ADDR(phys));
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, IOASIC_ISDN_R_NEXTPTR,
	    IOASIC_DMA_ADDR(nphys));

	/* kick off DMA */
	ssr |= IOASIC_CSR_DMAEN_ISDN_R;
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, IOASIC_CSR, ssr);

	d->active = 1;

	return 0;

bad:
	if (state & 2)
		bus_dmamap_unload(sc->sc_dmat, d->dmam);
	if (state & 1)
		bus_dmamap_destroy(sc->sc_dmat, d->dmam);
	return 1;
}

static void
bba_get_locks(void *opaque, kmutex_t **intr, kmutex_t **thread)
{
	struct bba_softc *bsc = opaque;
	struct am7930_softc *sc = &bsc->sc_am7930;
 
	*intr = &sc->sc_intr_lock;
	*thread = &sc->sc_lock;
}

static int
bba_intr(void *addr)
{
	struct bba_softc *sc;
	struct bba_dma_state *d;
	tc_addr_t nphys;
	int mask;

	sc = addr;
	mutex_enter(&sc->sc_am7930.sc_intr_lock);

	mask = bus_space_read_4(sc->sc_bst, sc->sc_bsh, IOASIC_INTR);

	if (mask & IOASIC_INTR_ISDN_TXLOAD) {
		d = &sc->sc_tx_dma_state;
		d->curseg = (d->curseg+1) % d->dmam->dm_nsegs;
		nphys = (tc_addr_t)d->dmam->dm_segs[d->curseg].ds_addr;
		bus_space_write_4(sc->sc_bst, sc->sc_bsh,
		    IOASIC_ISDN_X_NEXTPTR, IOASIC_DMA_ADDR(nphys));
		if (d->intr != NULL)
			(*d->intr)(d->intr_arg);
	}
	if (mask & IOASIC_INTR_ISDN_RXLOAD) {
		d = &sc->sc_rx_dma_state;
		d->curseg = (d->curseg+1) % d->dmam->dm_nsegs;
		nphys = (tc_addr_t)d->dmam->dm_segs[d->curseg].ds_addr;
		bus_space_write_4(sc->sc_bst, sc->sc_bsh,
		    IOASIC_ISDN_R_NEXTPTR, IOASIC_DMA_ADDR(nphys));
		if (d->intr != NULL)
			(*d->intr)(d->intr_arg);
	}

	mutex_exit(&sc->sc_am7930.sc_intr_lock);

	return 0;
}

static int
bba_query_format(void *addr, audio_format_query_t *afp)
{

	return audio_query_format(&bba_format, 1, afp);
}

static int
bba_set_format(void *addr, int setmode,
		const audio_params_t *play, const audio_params_t *rec,
		audio_filter_reg_t *pfil, audio_filter_reg_t *rfil)
{

	if ((setmode & AUMODE_PLAY) != 0) {
		pfil->codec = audio_internal_to_mulaw32;
	}
	if ((setmode & AUMODE_RECORD) != 0) {
		rfil->codec = audio_mulaw32_to_internal;
	}

	return 0;
}

static int
bba_round_blocksize(void *addr, int blk, int mode, const audio_params_t *param)
{

	return IOASIC_DMA_BLOCKSIZE;
}


/* indirect write */
static void
bba_codec_iwrite(struct am7930_softc *sc, int reg, uint8_t val)
{

	DPRINTF(("bba_codec_iwrite(): sc=%p, reg=%d, val=%d\n", sc, reg, val));
	bba_codec_dwrite(sc, AM7930_DREG_CR, reg);
	bba_codec_dwrite(sc, AM7930_DREG_DR, val);
}


static void
bba_codec_iwrite16(struct am7930_softc *sc, int reg, uint16_t val)
{

	DPRINTF(("bba_codec_iwrite16(): sc=%p, reg=%d, val=%d\n", sc, reg, val));
	bba_codec_dwrite(sc, AM7930_DREG_CR, reg);
	bba_codec_dwrite(sc, AM7930_DREG_DR, val);
	bba_codec_dwrite(sc, AM7930_DREG_DR, val>>8);
}


static uint16_t
bba_codec_iread16(struct am7930_softc *sc, int reg)
{
	uint16_t val;

	DPRINTF(("bba_codec_iread16(): sc=%p, reg=%d\n", sc, reg));
	bba_codec_dwrite(sc, AM7930_DREG_CR, reg);
	val = bba_codec_dread(sc, AM7930_DREG_DR) << 8;
	val |= bba_codec_dread(sc, AM7930_DREG_DR);

	return val;
}


/* indirect read */
static uint8_t
bba_codec_iread(struct am7930_softc *sc, int reg)
{
	uint8_t val;

	DPRINTF(("bba_codec_iread(): sc=%p, reg=%d\n", sc, reg));
	bba_codec_dwrite(sc, AM7930_DREG_CR, reg);
	val = bba_codec_dread(sc, AM7930_DREG_DR);

	DPRINTF(("read 0x%x (%d)\n", val, val));

	return val;
}

/* direct write */
static void
bba_codec_dwrite(struct am7930_softc *asc, int reg, uint8_t val)
{
	struct bba_softc *sc;

	sc = (struct bba_softc *)asc;
	DPRINTF(("bba_codec_dwrite(): sc=%p, reg=%d, val=%d\n", sc, reg, val));

#if defined(__alpha__)
	bus_space_write_4(sc->sc_bst, sc->sc_codec_bsh,
	    reg << 2, val << 8);
#else
	bus_space_write_4(sc->sc_bst, sc->sc_codec_bsh,
	    reg << 6, val);
#endif
}

/* direct read */
static uint8_t
bba_codec_dread(struct am7930_softc *asc, int reg)
{
	struct bba_softc *sc;

	sc = (struct bba_softc *)asc;
	DPRINTF(("bba_codec_dread(): sc=%p, reg=%d\n", sc, reg));

#if defined(__alpha__)
	return ((bus_space_read_4(sc->sc_bst, sc->sc_codec_bsh,
		reg << 2) >> 8) & 0xff);
#else
	return (bus_space_read_4(sc->sc_bst, sc->sc_codec_bsh,
		reg << 6) & 0xff);
#endif
}
