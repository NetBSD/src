/*	$NetBSD: vs.c,v 1.35.8.2 2017/12/03 11:36:48 jdolecek Exp $	*/

/*
 * Copyright (c) 2001 Tetsuya Isaki. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * VS - OKI MSM6258 ADPCM voice synthesizer device driver.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vs.c,v 1.35.8.2 2017/12/03 11:36:48 jdolecek Exp $");

#include "audio.h"
#include "vs.h"
#if NAUDIO > 0 && NVS > 0

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kmem.h>

#include <sys/audioio.h>
#include <dev/audio_if.h>
#include <dev/mulaw.h>

#include <machine/bus.h>
#include <machine/cpu.h>

#include <dev/ic/msm6258var.h>

#include <arch/x68k/dev/dmacvar.h>
#include <arch/x68k/dev/intiovar.h>
#include <arch/x68k/dev/opmvar.h>

#include <arch/x68k/dev/vsvar.h>

#ifdef VS_DEBUG
#define DPRINTF(y,x)	if (vs_debug >= (y)) printf x
static int vs_debug;
#ifdef AUDIO_DEBUG
extern int audiodebug;
#endif
#else
#define DPRINTF(y,x)
#endif

static int  vs_match(device_t, cfdata_t, void *);
static void vs_attach(device_t, device_t, void *);

static int  vs_dmaintr(void *);
static int  vs_dmaerrintr(void *);

/* MI audio layer interface */
static int  vs_open(void *, int);
static void vs_close(void *);
static int  vs_query_encoding(void *, struct audio_encoding *);
static int  vs_set_params(void *, int, int, audio_params_t *,
	audio_params_t *, stream_filter_list_t *, stream_filter_list_t *);
static int  vs_init_output(void *, void *, int);
static int  vs_init_input(void *, void *, int);
static int  vs_start_input(void *, void *, int, void (*)(void *), void *);
static int  vs_start_output(void *, void *, int, void (*)(void *), void *);
static int  vs_halt_output(void *);
static int  vs_halt_input(void *);
static int  vs_allocmem(struct vs_softc *, size_t, size_t, size_t,
	struct vs_dma *);
static void vs_freemem(struct vs_dma *);
static int  vs_getdev(void *, struct audio_device *);
static int  vs_set_port(void *, mixer_ctrl_t *);
static int  vs_get_port(void *, mixer_ctrl_t *);
static int  vs_query_devinfo(void *, mixer_devinfo_t *);
static void *vs_allocm(void *, int, size_t);
static void vs_freem(void *, void *, size_t);
static size_t vs_round_buffersize(void *, int, size_t);
static int  vs_get_props(void *);
static void vs_get_locks(void *, kmutex_t **, kmutex_t **);

/* lower functions */
static int vs_round_sr(u_long);
static void vs_set_sr(struct vs_softc *, int);
static inline void vs_set_po(struct vs_softc *, u_long);

extern struct cfdriver vs_cd;

CFATTACH_DECL_NEW(vs, sizeof(struct vs_softc),
    vs_match, vs_attach, NULL, NULL);

static int vs_attached;

static const struct audio_hw_if vs_hw_if = {
	vs_open,
	vs_close,
	NULL,			/* drain */
	vs_query_encoding,
	vs_set_params,
	NULL,			/* round_blocksize */
	NULL,			/* commit_settings */
	vs_init_output,
	vs_init_input,
	vs_start_output,
	vs_start_input,
	vs_halt_output,
	vs_halt_input,
	NULL,			/* speaker_ctl */
	vs_getdev,
	NULL,			/* setfd */
	vs_set_port,
	vs_get_port,
	vs_query_devinfo,
	vs_allocm,
	vs_freem,
	vs_round_buffersize,
	NULL,			/* mappage */
	vs_get_props,
	NULL,			/* trigger_output */
	NULL,			/* trigger_input */
	NULL,
	vs_get_locks,
};

static struct audio_device vs_device = {
	"OKI MSM6258",
	"",
	"vs"
};

struct {
	u_long rate;
	u_char clk;
	u_char den;
} vs_l2r[] = {
	{ VS_RATE_15K, VS_CLK_8MHZ, VS_SRATE_512 },
	{ VS_RATE_10K, VS_CLK_8MHZ, VS_SRATE_768 },
	{ VS_RATE_7K,  VS_CLK_8MHZ, VS_SRATE_1024},
	{ VS_RATE_5K,  VS_CLK_4MHZ, VS_SRATE_768 },
	{ VS_RATE_3K,  VS_CLK_4MHZ, VS_SRATE_1024}
};

#define NUM_RATE	(sizeof(vs_l2r)/sizeof(vs_l2r[0]))

static int
vs_match(device_t parent, cfdata_t cf, void *aux)
{
	struct intio_attach_args *ia;

	ia = aux;
	if (strcmp(ia->ia_name, "vs") || vs_attached)
		return 0;

	if (ia->ia_addr == INTIOCF_ADDR_DEFAULT)
		ia->ia_addr = VS_ADDR;
	if (ia->ia_dma == INTIOCF_DMA_DEFAULT)
		ia->ia_dma = VS_DMA;
	if (ia->ia_dmaintr == INTIOCF_DMAINTR_DEFAULT)
		ia->ia_dmaintr = VS_DMAINTR;

	/* fixed parameters */
	if (ia->ia_addr != VS_ADDR)
		return 0;
	if (ia->ia_dma != VS_DMA)
		return 0;
	if (ia->ia_dmaintr != VS_DMAINTR)
		return 0;

#ifdef VS_DEBUG
	vs_debug = 1;
#ifdef AUDIO_DEBUG
	audiodebug = 2;
#endif
#endif

	return 1;
}

static void
vs_attach(device_t parent, device_t self, void *aux)
{
	struct vs_softc *sc;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	struct intio_attach_args *ia;

	sc = device_private(self);
	sc->sc_dev = self;
	ia = aux;
	vs_attached = 1;

	printf("\n");

	/* Re-map the I/O space */
	iot = ia->ia_bst;
	bus_space_map(iot, ia->ia_addr, 0x2000, BUS_SPACE_MAP_SHIFTED, &ioh);

	/* Initialize sc */
	sc->sc_iot = iot;
	sc->sc_ioh = ioh;
	sc->sc_hw_if = &vs_hw_if;
	sc->sc_addr = (void *) ia->ia_addr;
	sc->sc_dmas = NULL;
	sc->sc_prev_vd = NULL;
	sc->sc_active = 0;
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_SCHED);

	/* XXX */
	bus_space_map(iot, PPI_ADDR, PPI_MAPSIZE, BUS_SPACE_MAP_SHIFTED,
		      &sc->sc_ppi);

	/* Initialize DMAC */
	sc->sc_dmat = ia->ia_dmat;
	sc->sc_dma_ch = dmac_alloc_channel(parent, ia->ia_dma, "vs",
		ia->ia_dmaintr,   vs_dmaintr, sc,
		ia->ia_dmaintr+1, vs_dmaerrintr, sc,
		(DMAC_DCR_XRM_CSWOH | DMAC_DCR_OTYP_EASYNC | DMAC_DCR_OPS_8BIT),
		(DMAC_OCR_SIZE_BYTE | DMAC_OCR_REQG_EXTERNAL));

	aprint_normal_dev(self, "MSM6258V ADPCM voice synthesizer\n");

	audio_attach_mi(&vs_hw_if, sc, sc->sc_dev);
}

/*
 * vs interrupt handler
 */
static int
vs_dmaintr(void *hdl)
{
	struct vs_softc *sc;

	DPRINTF(2, ("vs_dmaintr\n"));
	sc = hdl;

	mutex_spin_enter(&sc->sc_intr_lock);

	if (sc->sc_pintr) {
		sc->sc_pintr(sc->sc_parg);
	} else if (sc->sc_rintr) {
		sc->sc_rintr(sc->sc_rarg);
	} else {
		printf("vs_dmaintr: spurious interrupt\n");
	}

	mutex_spin_exit(&sc->sc_intr_lock);

	return 1;
}

static int
vs_dmaerrintr(void *hdl)
{
	struct vs_softc *sc;

	sc = hdl;
	DPRINTF(1, ("%s: DMA transfer error.\n", device_xname(sc->sc_dev)));
	/* XXX */
	vs_dmaintr(sc);

	return 1;
}


/*
 * audio MD layer interfaces
 */

static int
vs_open(void *hdl, int flags)
{
	struct vs_softc *sc;

	DPRINTF(1, ("vs_open: flags=%d\n", flags));
	sc = hdl;
	sc->sc_pintr = NULL;
	sc->sc_rintr = NULL;
	sc->sc_active = 0;

	return 0;
}

static void
vs_close(void *hdl)
{

	DPRINTF(1, ("vs_close\n"));
}

static int
vs_query_encoding(void *hdl, struct audio_encoding *fp)
{

	DPRINTF(1, ("vs_query_encoding\n"));

	if (fp->index == 0) {
		strcpy(fp->name, AudioEslinear);
		fp->encoding = AUDIO_ENCODING_SLINEAR;
		fp->precision = 8;
		fp->flags = 0;
		return 0;
	}
	if (fp->index == 1) {
		strcpy(fp->name, AudioEslinear_be);
		fp->encoding = AUDIO_ENCODING_SLINEAR_BE;
		fp->precision = 16;
		fp->flags = 0;
		return 0;
	}
	return EINVAL;
}

static int
vs_round_sr(u_long rate)
{
	int i;
	int diff;
	int nearest;

	diff = rate;
	nearest = 0;
	for (i = 0; i < NUM_RATE; i++) {
		if (rate >= vs_l2r[i].rate) {
			if (rate - vs_l2r[i].rate < diff) {
				diff = rate - vs_l2r[i].rate;
				nearest = i;
			}
		} else {
			if (vs_l2r[i].rate - rate < diff) {
				diff = vs_l2r[i].rate - rate;
				nearest = i;
			}
		}
	}
	if (diff * 100 / rate > 15)
		return -1;
	else
		return nearest;
}

static int
vs_set_params(void *hdl, int setmode, int usemode,
	audio_params_t *play, audio_params_t *rec,
	stream_filter_list_t *pfil, stream_filter_list_t *rfil)
{
	struct vs_softc *sc;
	audio_params_t hw;
	stream_filter_factory_t *pconv;
	stream_filter_factory_t *rconv;
	int rate;

	sc = hdl;

	DPRINTF(1, ("vs_set_params: mode=%d enc=%d rate=%d prec=%d ch=%d: ",
		setmode, play->encoding, play->sample_rate,
		play->precision, play->channels));

	/* *play and *rec are identical because !AUDIO_PROP_INDEPENDENT */

	if (play->channels != 1) {
		DPRINTF(1, ("channels not matched\n"));
		return EINVAL;
	}

	rate = vs_round_sr(play->sample_rate);
	if (rate < 0) {
		DPRINTF(1, ("rate not matched\n"));
		return EINVAL;
	}

	if (play->precision == 8 && play->encoding == AUDIO_ENCODING_SLINEAR) {
		pconv = msm6258_linear8_to_adpcm;
		rconv = msm6258_adpcm_to_linear8;
	} else if (play->precision == 16 &&
	           play->encoding == AUDIO_ENCODING_SLINEAR_BE) {
		pconv = msm6258_slinear16_to_adpcm;
		rconv = msm6258_adpcm_to_slinear16;
	} else {
		DPRINTF(1, ("prec/enc not matched\n"));
		return EINVAL;
	}

	sc->sc_current.rate = rate;

	/* pfil and rfil are independent even if !AUDIO_PROP_INDEPENDENT */

	if ((setmode & AUMODE_PLAY) != 0) {
		hw = *play;
		hw.encoding = AUDIO_ENCODING_ADPCM;
		hw.precision = 4;
		hw.validbits = 4;
		pfil->prepend(pfil, pconv, &hw);
	}
	if ((setmode & AUMODE_RECORD) != 0) {
		hw = *rec;
		hw.encoding = AUDIO_ENCODING_ADPCM;
		hw.precision = 4;
		hw.validbits = 4;
		rfil->prepend(rfil, rconv, &hw);
	}

	DPRINTF(1, ("accepted\n"));
	return 0;
}

static void
vs_set_sr(struct vs_softc *sc, int rate)
{

	DPRINTF(1, ("setting sample rate to %d, %d\n",
		 rate, (int)vs_l2r[rate].rate));
	bus_space_write_1(sc->sc_iot, sc->sc_ppi, PPI_PORTC,
			  (bus_space_read_1 (sc->sc_iot, sc->sc_ppi,
					     PPI_PORTC) & 0xf0)
			  | vs_l2r[rate].den);
	adpcm_chgclk(vs_l2r[rate].clk);
}

static inline void
vs_set_po(struct vs_softc *sc, u_long po)
{
	bus_space_write_1(sc->sc_iot, sc->sc_ppi, PPI_PORTC,
			  (bus_space_read_1(sc->sc_iot, sc->sc_ppi, PPI_PORTC)
			   & 0xfc) | po);
}

static int
vs_init_output(void *hdl, void *buffer, int size)
{
	struct vs_softc *sc;

	DPRINTF(1, ("%s\n", __func__));
	sc = hdl;

	/* Set rate and pan */
	vs_set_sr(sc, sc->sc_current.rate);
	vs_set_po(sc, VS_PANOUT_LR);

	return 0;
}

static int
vs_init_input(void *hdl, void *buffer, int size)
{
	struct vs_softc *sc;

	DPRINTF(1, ("%s\n", __func__));
	sc = hdl;

	/* Set rate */
	vs_set_sr(sc, sc->sc_current.rate);

	return 0;
}

static int
vs_start_output(void *hdl, void *block, int blksize, void (*intr)(void *),
	void *arg)
{
	struct vs_softc *sc;
	struct vs_dma *vd;
	struct dmac_channel_stat *chan;

	DPRINTF(2, ("%s: block=%p blksize=%d\n", __func__, block, blksize));
	sc = hdl;

	sc->sc_pintr = intr;
	sc->sc_parg  = arg;

	/* Find DMA buffer. */
	for (vd = sc->sc_dmas; vd != NULL; vd = vd->vd_next) {
		if (KVADDR(vd) <= block && block < KVADDR_END(vd)
			break;
	}
	if (vd == NULL) {
		printf("%s: start_output: bad addr %p\n",
		    device_xname(sc->sc_dev), block);
		return EINVAL;
	}

	chan = sc->sc_dma_ch;

	if (vd != sc->sc_prev_vd) {
		sc->sc_current.xfer = dmac_prepare_xfer(chan, sc->sc_dmat,
		    vd->vd_map, DMAC_OCR_DIR_MTD,
		    (DMAC_SCR_MAC_COUNT_UP | DMAC_SCR_DAC_NO_COUNT),
		    sc->sc_addr + MSM6258_DATA * 2 + 1);
		sc->sc_prev_vd = vd;
	}
	dmac_start_xfer_offset(chan->ch_softc, sc->sc_current.xfer,
	    (int)block - (int)KVADDR(vd), blksize);

	if (sc->sc_active == 0) {
		bus_space_write_1(sc->sc_iot, sc->sc_ioh,
			MSM6258_CMD, MSM6258_CMD_PLAY_START);
		sc->sc_active = 1;
	}

	return 0;
}

static int
vs_start_input(void *hdl, void *block, int blksize, void (*intr)(void *),
	void *arg)
{
	struct vs_softc *sc;
	struct vs_dma *vd;
	struct dmac_channel_stat *chan;

	DPRINTF(2, ("%s: block=%p blksize=%d\n", __func__, block, blksize));
	sc = hdl;

	sc->sc_rintr = intr;
	sc->sc_rarg  = arg;

	/* Find DMA buffer. */
	for (vd = sc->sc_dmas; vd != NULL; vd = vd->vd_next) {
		if (KVADDR(vd) <= block && block < KVADDR_END(vd)
			break;
	}
	if (vd == NULL) {
		printf("%s: start_output: bad addr %p\n",
		    device_xname(sc->sc_dev), block);
		return EINVAL;
	}

	chan = sc->sc_dma_ch;

	if (vd != sc->sc_prev_vd) {
		sc->sc_current.xfer = dmac_prepare_xfer(chan, sc->sc_dmat,
		    vd->vd_map, DMAC_OCR_DIR_DTM,
		    (DMAC_SCR_MAC_COUNT_UP | DMAC_SCR_DAC_NO_COUNT),
		    sc->sc_addr + MSM6258_DATA * 2 + 1);
		sc->sc_prev_vd = vd;
	}
	dmac_start_xfer_offset(chan->ch_softc, sc->sc_current.xfer,
	    (int)block - (int)KVADDR(vd), blksize);

	if (sc->sc_active == 0) {
		bus_space_write_1(sc->sc_iot, sc->sc_ioh,
			MSM6258_CMD, MSM6258_CMD_REC_START);
		sc->sc_active = 1;
	}

	return 0;
}

static int
vs_halt_output(void *hdl)
{
	struct vs_softc *sc;

	DPRINTF(1, ("vs_halt_output\n"));
	sc = hdl;
	if (sc->sc_active) {
		/* stop ADPCM play */
		dmac_abort_xfer(sc->sc_dma_ch->ch_softc, sc->sc_current.xfer);
		bus_space_write_1(sc->sc_iot, sc->sc_ioh,
			MSM6258_CMD, MSM6258_CMD_STOP);
		sc->sc_active = 0;
	}

	return 0;
}

static int
vs_halt_input(void *hdl)
{
	struct vs_softc *sc;

	DPRINTF(1, ("vs_halt_input\n"));
	sc = hdl;
	if (sc->sc_active) {
		/* stop ADPCM recoding */
		dmac_abort_xfer(sc->sc_dma_ch->ch_softc, sc->sc_current.xfer);
		bus_space_write_1(sc->sc_iot, sc->sc_ioh,
			MSM6258_CMD, MSM6258_CMD_STOP);
		sc->sc_active = 0;
	}

	return 0;
}

static int
vs_allocmem(struct vs_softc *sc, size_t size, size_t align, size_t boundary,
	struct vs_dma *vd)
{
	int error;

#ifdef DIAGNOSTIC
	if (size > DMAC_MAXSEGSZ)
		panic ("vs_allocmem: maximum size exceeded, %d", (int) size);
#endif

	vd->vd_size = size;

	error = bus_dmamem_alloc(vd->vd_dmat, vd->vd_size, align, boundary,
				 vd->vd_segs,
				 sizeof (vd->vd_segs) / sizeof (vd->vd_segs[0]),
				 &vd->vd_nsegs, BUS_DMA_WAITOK);
	if (error)
		goto out;

	error = bus_dmamem_map(vd->vd_dmat, vd->vd_segs, vd->vd_nsegs,
			       vd->vd_size, &vd->vd_addr,
			       BUS_DMA_WAITOK | BUS_DMA_COHERENT);
	if (error)
		goto free;

	error = bus_dmamap_create(vd->vd_dmat, vd->vd_size, 1, DMAC_MAXSEGSZ,
				  0, BUS_DMA_WAITOK, &vd->vd_map);
	if (error)
		goto unmap;

	error = bus_dmamap_load(vd->vd_dmat, vd->vd_map, vd->vd_addr,
				vd->vd_size, NULL, BUS_DMA_WAITOK);
	if (error)
		goto destroy;

	return 0;

 destroy:
	bus_dmamap_destroy(vd->vd_dmat, vd->vd_map);
 unmap:
	bus_dmamem_unmap(vd->vd_dmat, vd->vd_addr, vd->vd_size);
 free:
	bus_dmamem_free(vd->vd_dmat, vd->vd_segs, vd->vd_nsegs);
 out:
	return error;
}

static void
vs_freemem(struct vs_dma *vd)
{

	bus_dmamap_unload(vd->vd_dmat, vd->vd_map);
	bus_dmamap_destroy(vd->vd_dmat, vd->vd_map);
	bus_dmamem_unmap(vd->vd_dmat, vd->vd_addr, vd->vd_size);
	bus_dmamem_free(vd->vd_dmat, vd->vd_segs, vd->vd_nsegs);
}

static int
vs_getdev(void *hdl, struct audio_device *retp)
{

	DPRINTF(1, ("vs_getdev\n"));
	*retp = vs_device;
	return 0;
}

static int
vs_set_port(void *hdl, mixer_ctrl_t *cp)
{

	DPRINTF(1, ("vs_set_port\n"));
	return 0;
}

static int
vs_get_port(void *hdl, mixer_ctrl_t *cp)
{

	DPRINTF(1, ("vs_get_port\n"));
	return 0;
}

static int
vs_query_devinfo(void *hdl, mixer_devinfo_t *mi)
{

	DPRINTF(1, ("vs_query_devinfo\n"));
	switch (mi->index) {
	default:
		return EINVAL;
	}
	return 0;
}

static void *
vs_allocm(void *hdl, int direction, size_t size)
{
	struct vs_softc *sc;
	struct vs_dma *vd;
	int error;

	vd = kmem_alloc(sizeof(*vd), KM_SLEEP);
	sc = hdl;
	vd->vd_dmat = sc->sc_dmat;

	error = vs_allocmem(sc, size, 32, 0, vd);
	if (error) {
		kmem_free(vd, sizeof(*vd));
		return NULL;
	}
	vd->vd_next = sc->sc_dmas;
	sc->sc_dmas = vd;

	return KVADDR(vd);
}

static void
vs_freem(void *hdl, void *addr, size_t size)
{
	struct vs_softc *sc;
	struct vs_dma *p, **pp;

	sc = hdl;
	for (pp = &sc->sc_dmas; (p = *pp) != NULL; pp = &p->vd_next) {
		if (KVADDR(p) == addr) {
			vs_freemem(p);
			*pp = p->vd_next;
			kmem_free(p, sizeof(*p));
			return;
		}
	}
}

static size_t
vs_round_buffersize(void *hdl, int direction, size_t bufsize)
{

	if (bufsize > DMAC_MAXSEGSZ)
		bufsize = DMAC_MAXSEGSZ;
	return bufsize;
}

#if 0
paddr_t
vs_mappage(void *addr, void *mem, off_t off, int prot)
{
	struct vs_softc *sc;
	struct vs_dma *p;

	if (off < 0)
		return -1;
	sc = addr;
	for (p = sc->sc_dmas; p != NULL && KVADDR(p) != mem;
	     p = p->vd_next)
		continue;
	if (p == NULL) {
		printf("%s: mappage: bad addr %p\n",
		    device_xname(sc->sc_dev), start);
		return -1;
	}

	return bus_dmamem_mmap(sc->sc_dmat, p->vd_segs, p->vd_nsegs,
			       off, prot, BUS_DMA_WAITOK);
}
#endif

static int
vs_get_props(void *hdl)
{

	DPRINTF(1, ("vs_get_props\n"));
	return 0 /* | dependent | half duplex | no mmap */;
}

static void
vs_get_locks(void *hdl, kmutex_t **intr, kmutex_t **thread)
{
	struct vs_softc *sc;

	DPRINTF(1, ("vs_get_locks\n"));
	sc = hdl;
	*intr = &sc->sc_intr_lock;
	*thread = &sc->sc_lock;
}

#endif /* NAUDIO > 0 && NVS > 0*/
