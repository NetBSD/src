/*	$NetBSD: vs.c,v 1.56 2022/05/26 14:33:29 tsutsui Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: vs.c,v 1.56 2022/05/26 14:33:29 tsutsui Exp $");

#include "audio.h"
#include "vs.h"
#if NAUDIO > 0 && NVS > 0

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kmem.h>

#include <sys/audioio.h>
#include <dev/audio/audio_if.h>

#include <machine/bus.h>
#include <machine/cpu.h>

#include <dev/ic/msm6258var.h>

#include <arch/x68k/dev/dmacvar.h>
#include <arch/x68k/dev/intiovar.h>
#include <arch/x68k/dev/opmvar.h>

#include <arch/x68k/dev/vsvar.h>

#include "ioconf.h"

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
static int  vs_query_format(void *, audio_format_query_t *);
static int  vs_set_format(void *, int,
	const audio_params_t *, const audio_params_t *,
	audio_filter_reg_t *, audio_filter_reg_t *);
static int  vs_commit_settings(void *);
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
static inline void vs_set_panout(struct vs_softc *, u_long);

CFATTACH_DECL_NEW(vs, sizeof(struct vs_softc),
    vs_match, vs_attach, NULL, NULL);

static int vs_attached;

static const struct audio_hw_if vs_hw_if = {
	.query_format		= vs_query_format,
	.set_format		= vs_set_format,
	.commit_settings	= vs_commit_settings,
	.start_output		= vs_start_output,
	.start_input		= vs_start_input,
	.halt_output		= vs_halt_output,
	.halt_input		= vs_halt_input,
	.getdev			= vs_getdev,
	.set_port		= vs_set_port,
	.get_port		= vs_get_port,
	.query_devinfo		= vs_query_devinfo,
	.allocm			= vs_allocm,
	.freem			= vs_freem,
	.round_buffersize	= vs_round_buffersize,
	.get_props		= vs_get_props,
	.get_locks		= vs_get_locks,
};

static struct audio_device vs_device = {
	"OKI MSM6258",
	"",
	"vs"
};

static const struct audio_format vs_formats = {
	.mode		= AUMODE_PLAY | AUMODE_RECORD,
	.encoding	= AUDIO_ENCODING_ADPCM,
	.validbits	= 4,
	.precision	= 4,
	.channels	= 1,
	.channel_mask	= AUFMT_MONAURAL,
	.frequency_type	= 5,
	.frequency	= { VS_RATE_3K, VS_RATE_5K, VS_RATE_7K,
	                    VS_RATE_10K, VS_RATE_15K },
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
	sc->sc_addr = (void *) ia->ia_addr;
	sc->sc_dmas = NULL;
	sc->sc_prev_vd = NULL;
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_VM);

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
vs_query_format(void *hdl, audio_format_query_t *afp)
{

	return audio_query_format(&vs_formats, 1, afp);
}

static int
vs_round_sr(u_long rate)
{
	int i;

	for (i = 0; i < NUM_RATE; i++) {
		if (rate == vs_l2r[i].rate)
			return i;
	}
	return -1;
}

static int
vs_set_format(void *hdl, int setmode,
	const audio_params_t *play, const audio_params_t *rec,
	audio_filter_reg_t *pfil, audio_filter_reg_t *rfil)
{
	struct vs_softc *sc;
	int rate;

	sc = hdl;

	DPRINTF(1, ("%s: mode=%d %s/%dbit/%dch/%dHz: ", __func__,
	    setmode, audio_encoding_name(play->encoding),
	    play->precision, play->channels, play->sample_rate));

	/* *play and *rec are identical because !AUDIO_PROP_INDEPENDENT */

	rate = vs_round_sr(play->sample_rate);
	KASSERT(rate >= 0);
	sc->sc_current.rate = rate;

	if ((setmode & AUMODE_PLAY) != 0) {
		pfil->codec = msm6258_internal_to_adpcm;
		pfil->context = &sc->sc_codecvar;
	}
	if ((setmode & AUMODE_RECORD) != 0) {
		rfil->codec = msm6258_adpcm_to_internal;
		rfil->context = &sc->sc_codecvar;
	}

	DPRINTF(1, ("accepted\n"));
	return 0;
}

static int
vs_commit_settings(void *hdl)
{
	struct vs_softc *sc;
	int rate;

	sc = hdl;
	rate = sc->sc_current.rate;

	DPRINTF(1, ("commit_settings: sample rate to %d, %d\n",
		 rate, (int)vs_l2r[rate].rate));
	bus_space_write_1(sc->sc_iot, sc->sc_ppi, PPI_PORTC,
			  (bus_space_read_1 (sc->sc_iot, sc->sc_ppi,
					     PPI_PORTC) & 0xf0)
			  | vs_l2r[rate].den);
	adpcm_chgclk(vs_l2r[rate].clk);

	return 0;
}

static inline void
vs_set_panout(struct vs_softc *sc, u_long po)
{

	bus_space_write_1(sc->sc_iot, sc->sc_ppi, PPI_PORTC,
			  (bus_space_read_1(sc->sc_iot, sc->sc_ppi, PPI_PORTC)
			   & 0xfc) | po);
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

	if (sc->sc_pintr == NULL) {
		sc->sc_pintr = intr;
		sc->sc_parg  = arg;

		vs_set_panout(sc, VS_PANOUT_LR);
		bus_space_write_1(sc->sc_iot, sc->sc_ioh,
			MSM6258_CMD, MSM6258_CMD_PLAY_START);
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

	if (sc->sc_rintr == NULL) {
		sc->sc_rintr = intr;
		sc->sc_rarg  = arg;

		bus_space_write_1(sc->sc_iot, sc->sc_ioh,
			MSM6258_CMD, MSM6258_CMD_REC_START);
	}

	return 0;
}

static int
vs_halt_output(void *hdl)
{
	struct vs_softc *sc;

	DPRINTF(1, ("vs_halt_output\n"));
	sc = hdl;

	/* stop ADPCM play */
	dmac_abort_xfer(sc->sc_dma_ch->ch_softc, sc->sc_current.xfer);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh,
	    MSM6258_CMD, MSM6258_CMD_STOP);
	sc->sc_pintr = NULL;

	return 0;
}

static int
vs_halt_input(void *hdl)
{
	struct vs_softc *sc;

	DPRINTF(1, ("vs_halt_input\n"));
	sc = hdl;

	/* stop ADPCM recording */
	dmac_abort_xfer(sc->sc_dma_ch->ch_softc, sc->sc_current.xfer);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh,
	    MSM6258_CMD, MSM6258_CMD_STOP);
	sc->sc_rintr = NULL;

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

static int
vs_get_props(void *hdl)
{

	DPRINTF(1, ("vs_get_props\n"));
	return AUDIO_PROP_PLAYBACK | AUDIO_PROP_CAPTURE;
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
