/*	$NetBSD: vs.c,v 1.15 2002/04/07 14:52:27 isaki Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Tetsuya Isaki.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#include "audio.h"
#include "vs.h"
#if NAUDIO > 0 && NVS > 0

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <sys/audioio.h>
#include <dev/audio_if.h>

#include <machine/bus.h>
#include <machine/cpu.h>

#include <dev/ic/msm6258var.h>

#include <arch/x68k/dev/dmacvar.h>
#include <arch/x68k/dev/intiovar.h>
#include <arch/x68k/dev/opmreg.h>

#include <arch/x68k/dev/vsvar.h>

#ifdef VS_DEBUG
#define DPRINTF(y,x)	if(vs_debug>=(y))printf x
static int vs_debug;
#ifdef AUDIO_DEBUG
extern int audiodebug;
#endif
#else
#define DPRINTF(y,x)
#endif

static int  vs_match __P((struct device *, struct cfdata *, void *));
static void vs_attach __P((struct device *, struct device *, void *));

static int  vs_dmaintr __P((void *));
static int  vs_dmaerrintr __P((void *));

/* MI audio layer interface */
static int  vs_open __P((void *, int));
static void vs_close __P((void *));
static int  vs_query_encoding __P((void *, struct audio_encoding *));
static int  vs_set_params __P((void *, int, int, struct audio_params *,
	struct audio_params *));
static int  vs_trigger_output __P((void *, void *, void *, int,
				   void (*)(void *), void *,
				   struct audio_params *));
static int  vs_trigger_input __P((void *, void *, void *, int,
				   void (*)(void *), void *,
				   struct audio_params *));
static int  vs_halt_output __P((void *));
static int  vs_halt_input __P((void *));
static int  vs_allocmem __P((struct vs_softc *, size_t, size_t, size_t, int,
			    struct vs_dma *));
static void vs_freemem __P((struct vs_dma *));
static int  vs_getdev __P((void *, struct audio_device *));
static int  vs_set_port __P((void *, mixer_ctrl_t *));
static int  vs_get_port __P((void *, mixer_ctrl_t *));
static int  vs_query_devinfo __P((void *, mixer_devinfo_t *));
static void *vs_allocm __P((void *, int, size_t, int, int));
static void vs_freem __P((void *, void *, int));
static size_t vs_round_buffersize __P((void *, int, size_t));
static int  vs_get_props __P((void *));

/* lower functions */
static int vs_round_sr(u_long);
static void vs_set_sr(struct vs_softc *sc, int);
static inline void vs_set_po(struct vs_softc *sc, u_long);

extern struct cfdata vs_cd;

struct cfattach vs_ca = {
	sizeof(struct vs_softc), vs_match, vs_attach
};

static struct audio_hw_if vs_hw_if = {
	vs_open,
	vs_close,
	NULL,			/* drain */
	
	vs_query_encoding,
	vs_set_params,
	NULL,			/* round_blocksize */
	NULL,			/* commit_settings */
	
	NULL, 			/* init_output */
	NULL, 			/* init_input */
	NULL, 			/* start_output */
	NULL,			/* start_input */

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

	vs_trigger_output,
	vs_trigger_input,

	NULL,
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

struct {
	char	*name;
	int	encoding;
	int	precision;
} vs_encodings[] = {
	{AudioEadpcm,      AUDIO_ENCODING_ADPCM,       4},
	{AudioEslinear,    AUDIO_ENCODING_SLINEAR,     8},
	{AudioEulinear,    AUDIO_ENCODING_ULINEAR,     8},
	{AudioEmulaw,      AUDIO_ENCODING_ULAW,        8},
	{AudioEslinear_be, AUDIO_ENCODING_SLINEAR_BE, 16},
	{AudioEslinear_le, AUDIO_ENCODING_SLINEAR_LE, 16},
};

static int
vs_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct intio_attach_args *ia = aux;

	if (strcmp(ia->ia_name, "vs") || cf->cf_unit > 0)
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
vs_attach(struct device *parent, struct device *self, void *aux)
{
	struct vs_softc *sc = (struct vs_softc *)self;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	struct intio_attach_args *ia = aux;

	printf("\n");
	
	/* Re-map the I/O space */
	iot = ia->ia_bst;
	bus_space_map(iot, ia->ia_addr, 0x2000, BUS_SPACE_MAP_SHIFTED, &ioh);

	/* Initialize sc */
	sc->sc_iot = iot;
	sc->sc_ioh = ioh;
	sc->sc_hw_if = &vs_hw_if;
	sc->sc_addr = (caddr_t) ia->ia_addr;
	sc->sc_dmas = NULL;

	/* Initialize codec */
	sc->sc_codec = msm6258_codec_init();
	if (sc->sc_codec == NULL) {
		printf ("Could not init codec\n");
		return;
	}

	/* XXX */
	bus_space_map(iot, PPI_ADDR, PPI_MAPSIZE, BUS_SPACE_MAP_SHIFTED,
		      &sc->sc_ppi);

	/* Initialize DMAC */
	sc->sc_dmat = ia->ia_dmat;
	sc->sc_dma_ch = dmac_alloc_channel(parent, ia->ia_dma, "vs",
		ia->ia_dmaintr,   vs_dmaintr, sc,
		ia->ia_dmaintr+1, vs_dmaerrintr, sc);

	printf("%s: MSM6258V ADPCM voice synthesizer\n", sc->sc_dev.dv_xname);

	audio_attach_mi(&vs_hw_if, sc, &sc->sc_dev);
}

/*
 * vs interrupt handler
 */
static int
vs_dmaintr(void *hdl)
{
	struct vs_softc *sc = hdl;

	DPRINTF(2, ("vs_dmaintr\n"));

	if (sc->sc_pintr) {
		/* start next transfer */
		sc->sc_current.dmap += sc->sc_current.blksize;
		if (sc->sc_current.dmap + sc->sc_current.blksize
			> sc->sc_current.bufsize)
			sc->sc_current.dmap -= sc->sc_current.bufsize;
		dmac_start_xfer_offset (sc->sc_dma_ch->ch_softc,
					sc->sc_current.xfer,
					sc->sc_current.dmap,
					sc->sc_current.blksize);
		sc->sc_pintr(sc->sc_parg);
	} else if (sc->sc_rintr) {
		/* start next transfer */
		sc->sc_current.dmap += sc->sc_current.blksize;
		if (sc->sc_current.dmap + sc->sc_current.blksize
			> sc->sc_current.bufsize)
			sc->sc_current.dmap -= sc->sc_current.bufsize;
		dmac_start_xfer_offset (sc->sc_dma_ch->ch_softc,
					sc->sc_current.xfer,
					sc->sc_current.dmap,
					sc->sc_current.blksize);
		sc->sc_rintr(sc->sc_rarg);
	} else {
		printf ("vs_dmaintr: spurious interrupt\n");
	}

	return 1;
}

static int
vs_dmaerrintr(void *hdl)
{
	struct vs_softc *sc = hdl;

	DPRINTF(1, ("%s: DMA transfer error.\n", sc->sc_dev.dv_xname));
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
	struct vs_softc *sc = hdl;

	DPRINTF(1, ("vs_open: flags=%d\n", flags));

	sc->sc_pintr = NULL;
	sc->sc_rintr = NULL;

	msm6258_codec_open(sc);

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

	if (fp->index >= sizeof(vs_encodings) / sizeof(vs_encodings[0]))
		return EINVAL;

	strcpy(fp->name, vs_encodings[fp->index].name);
	fp->encoding  = vs_encodings[fp->index].encoding;
	fp->precision = vs_encodings[fp->index].precision;
	if (fp->encoding == AUDIO_ENCODING_ADPCM)
		fp->flags = 0;
	else
		fp->flags = AUDIO_ENCODINGFLAG_EMULATED;
	return 0;
}

static int
vs_round_sr(u_long rate)
{
	int i;
	int diff = rate;
	int nearest = 0;

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
	struct audio_params *play, struct audio_params *rec)
{
	struct vs_softc *sc = hdl;
	struct audio_params *p;
	int mode;
	int rate;
	void (*pswcode)(void *, u_char *, int);
	void (*rswcode)(void *, u_char *, int);

	DPRINTF(1, ("vs_set_params: setmode=%d, usemode=%d\n", setmode, usemode));

	/* set first record info, then play info */
	for (mode = AUMODE_RECORD; mode != -1;
	     mode = (mode == AUMODE_RECORD) ? AUMODE_PLAY : -1) {
		if ((setmode & mode) == 0)
			continue;

		p = (mode == AUMODE_PLAY) ? play : rec;

		if (p->channels != 1)
			return (EINVAL);

		rate = p->sample_rate;
		pswcode = NULL;
		rswcode = NULL;
		p->factor = 1;
		p->factor_denom = 0;
		p->hw_precision = 4;
		p->hw_encoding = AUDIO_ENCODING_ADPCM;
		DPRINTF(1, ("vs_set_params: encoding=%d, precision=%d\n",
			p->encoding, p->precision));
		switch (p->precision) {
		case 4:
			if (p->encoding == AUDIO_ENCODING_ADPCM)
				p->factor_denom = 1;
			break;
		case 8:
			switch (p->encoding) {
			case AUDIO_ENCODING_ULAW:
				p->factor_denom = 2;
				pswcode = msm6258_mulaw_to_adpcm;
				rswcode = msm6258_adpcm_to_mulaw;
				break;
			case AUDIO_ENCODING_SLINEAR:
			case AUDIO_ENCODING_SLINEAR_LE:
			case AUDIO_ENCODING_SLINEAR_BE:
				p->factor_denom = 2;
				pswcode = msm6258_slinear8_to_adpcm;
				rswcode = msm6258_adpcm_to_slinear8;
				break;
			case AUDIO_ENCODING_ULINEAR:
			case AUDIO_ENCODING_ULINEAR_LE:
			case AUDIO_ENCODING_ULINEAR_BE:
				p->factor_denom = 2;
				pswcode = msm6258_ulinear8_to_adpcm;
				rswcode = msm6258_adpcm_to_ulinear8;
				break;
			}
			break;
		case 16:
			switch (p->encoding) {
			case AUDIO_ENCODING_SLINEAR_LE:
				p->factor_denom = 4;
				pswcode = msm6258_slinear16_le_to_adpcm;
				rswcode = msm6258_adpcm_to_slinear16_le;
				break;
			case AUDIO_ENCODING_SLINEAR_BE:
				p->factor_denom = 4;
				pswcode = msm6258_slinear16_be_to_adpcm;
				rswcode = msm6258_adpcm_to_slinear16_be;
				break;
			}
			break;
		}
		if (p->factor_denom == 0) {
			DPRINTF(1, ("vs_set_params: mode=%d, encoding=%d\n",
				mode, p->encoding));
			return EINVAL;
		}

		DPRINTF(1, ("vs_set_params: rate=%d -> ", rate));
		rate = vs_round_sr(rate);
		DPRINTF(1, ("%d\n", rate));
		if (rate < 0)
			return (EINVAL);
		if (mode == AUMODE_PLAY) {
			p->sw_code = pswcode;
			sc->sc_current.prate = rate;
		} else {
			p->sw_code = rswcode;
			sc->sc_current.rrate = rate;
		}
	}

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
vs_trigger_output(void *hdl, void *start, void *end, int bsize,
		  void (*intr)(void *), void *arg,
		  struct audio_params *p)
{
	struct vs_softc *sc = hdl;
	struct vs_dma *vd;
	struct dmac_dma_xfer *xf;
	struct dmac_channel_stat *chan = sc->sc_dma_ch;

	DPRINTF(2, ("vs_trigger_output: start=%p, bsize=%d, intr=%p, arg=%p\n",
		 start, bsize, intr, arg));

	sc->sc_pintr = intr;
	sc->sc_parg  = arg;
	sc->sc_current.blksize = bsize;
	sc->sc_current.bufsize = (char*)end - (char*)start;
	sc->sc_current.dmap = 0;

	/* Find DMA buffer. */
	for (vd = sc->sc_dmas; vd != NULL && KVADDR(vd) != start;
	     vd = vd->vd_next)
		;
	if (vd == NULL) {
		printf("%s: trigger_output: bad addr %p\n",
		    sc->sc_dev.dv_xname, start);
		return (EINVAL);
	}

	vs_set_sr(sc, sc->sc_current.prate);
	vs_set_po(sc, VS_PANOUT_LR);

	xf = dmac_alloc_xfer (chan, sc->sc_dmat, vd->vd_map);
	sc->sc_current.xfer = xf;
	chan->ch_dcr = (DMAC_DCR_XRM_CSWOH | DMAC_DCR_OTYP_EASYNC |
			DMAC_DCR_OPS_8BIT);
	chan->ch_ocr = DMAC_OCR_REQG_EXTERNAL;
	xf->dx_ocr = DMAC_OCR_DIR_MTD;
	xf->dx_scr = DMAC_SCR_MAC_COUNT_UP | DMAC_SCR_DAC_NO_COUNT;
	xf->dx_device = sc->sc_addr + MSM6258_DATA*2 + 1;

	dmac_load_xfer (chan->ch_softc, xf);
	dmac_start_xfer_offset (chan->ch_softc, xf, 0, sc->sc_current.blksize);
	bus_space_write_1 (sc->sc_iot, sc->sc_ioh, MSM6258_STAT, 2);

	return 0;
}

static int
vs_trigger_input(void *hdl, void *start, void *end, int bsize,
		 void (*intr)(void *), void *arg,
		 struct audio_params *p)
{
	struct vs_softc *sc = hdl;
	struct vs_dma *vd;
	struct dmac_dma_xfer *xf;
	struct dmac_channel_stat *chan = sc->sc_dma_ch;

	DPRINTF(2, ("vs_trigger_input: start=%p, bsize=%d, intr=%p, arg=%p\n",
		 start, bsize, intr, arg));

	sc->sc_rintr = intr;
	sc->sc_rarg  = arg;
	sc->sc_current.blksize = bsize;
	sc->sc_current.bufsize = (char*)end - (char*)start;
	sc->sc_current.dmap = 0;

	/* Find DMA buffer. */
	for (vd = sc->sc_dmas; vd != NULL && KVADDR(vd) != start;
	     vd = vd->vd_next)
		;
	if (vd == NULL) {
		printf("%s: trigger_output: bad addr %p\n",
		    sc->sc_dev.dv_xname, start);
		return (EINVAL);
	}

	vs_set_sr(sc, sc->sc_current.rrate);
	xf = dmac_alloc_xfer (chan, sc->sc_dmat, vd->vd_map);
	sc->sc_current.xfer = xf;
	chan->ch_dcr = (DMAC_DCR_XRM_CSWOH | DMAC_DCR_OTYP_EASYNC |
			DMAC_DCR_OPS_8BIT);
	chan->ch_ocr = DMAC_OCR_REQG_EXTERNAL;
	xf->dx_ocr = DMAC_OCR_DIR_DTM;
	xf->dx_scr = DMAC_SCR_MAC_COUNT_UP | DMAC_SCR_DAC_NO_COUNT;
	xf->dx_device = sc->sc_addr + MSM6258_DATA*2 + 1;

	dmac_load_xfer (chan->ch_softc, xf);
	dmac_start_xfer_offset (chan->ch_softc, xf, 0, sc->sc_current.blksize);
	bus_space_write_1 (sc->sc_iot, sc->sc_ioh, MSM6258_STAT, 4);

	return 0;
}

static int
vs_halt_output(void *hdl)
{
	struct vs_softc *sc = hdl;

	DPRINTF(1, ("vs_halt_output\n"));

	/* stop ADPCM play */
	dmac_abort_xfer(sc->sc_dma_ch->ch_softc, sc->sc_current.xfer);
	bus_space_write_1 (sc->sc_iot, sc->sc_ioh, MSM6258_STAT, 1);

	return 0;
}

static int
vs_halt_input(void *hdl)
{
	struct vs_softc *sc = hdl;

	DPRINTF(1, ("vs_halt_input\n"));

	/* stop ADPCM recoding */
	dmac_abort_xfer(sc->sc_dma_ch->ch_softc, sc->sc_current.xfer);
	bus_space_write_1 (sc->sc_iot, sc->sc_ioh, MSM6258_STAT, 1);

	return 0;
}

static int
vs_allocmem(sc, size, align, boundary, flags, vd)
	struct vs_softc *sc;
	size_t size;
	size_t align;
	size_t boundary;
	int flags;
	struct vs_dma *vd;
{
	int error, wait;

#ifdef DIAGNOSTIC
	if (size > DMAC_MAXSEGSZ)
		panic ("vs_allocmem: maximum size exceeded, %d", (int) size);
#endif

	wait = (flags & M_NOWAIT) ? BUS_DMA_NOWAIT : BUS_DMA_WAITOK;
	vd->vd_size = size;
	
	error = bus_dmamem_alloc(vd->vd_dmat, vd->vd_size, align, boundary,
				 vd->vd_segs,
				 sizeof (vd->vd_segs) / sizeof (vd->vd_segs[0]),
				 &vd->vd_nsegs, wait);
	if (error)
		goto out;

	error = bus_dmamem_map(vd->vd_dmat, vd->vd_segs, vd->vd_nsegs,
			       vd->vd_size, &vd->vd_addr,
			       wait | BUS_DMA_COHERENT);
	if (error)
		goto free;

	error = bus_dmamap_create(vd->vd_dmat, vd->vd_size, 1, DMAC_MAXSEGSZ,
				  0, wait, &vd->vd_map);
	if (error)
		goto unmap;

	error = bus_dmamap_load(vd->vd_dmat, vd->vd_map, vd->vd_addr,
				vd->vd_size, NULL, wait);
	if (error)
		goto destroy;

	return (0);

 destroy:
	bus_dmamap_destroy(vd->vd_dmat, vd->vd_map);
 unmap:
	bus_dmamem_unmap(vd->vd_dmat, vd->vd_addr, vd->vd_size);
 free:
	bus_dmamem_free(vd->vd_dmat, vd->vd_segs, vd->vd_nsegs);
 out:
	return (error);
}

static void
vs_freemem(vd)
	struct vs_dma *vd;
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
vs_allocm(hdl, direction, size, type, flags)
	void *hdl;
	int direction;
	size_t size;
	int type, flags;
{
	struct vs_softc *sc = hdl;
	struct vs_dma *vd;
	int error;

	if ((vd = malloc(size, type, flags)) == NULL)
		return (NULL);

	vd->vd_dmat = sc->sc_dmat;

	error = vs_allocmem(sc, size, 32, 0, flags, vd);
	if (error) {
		free(vd, type);
		return (NULL);
	}
	vd->vd_next = sc->sc_dmas;
	sc->sc_dmas = vd;

	return (KVADDR(vd));
}

static void
vs_freem(hdl, addr, type)
	void *hdl;
	void *addr;
	int type;
{
	struct vs_softc *sc = hdl;
	struct vs_dma *p, **pp;

	for (pp = &sc->sc_dmas; (p = *pp) != NULL; pp = &p->vd_next) {
		if (KVADDR(p) == addr) {
			vs_freemem(p);
			*pp = p->vd_next;
			free(p, type);
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
vs_mappage(addr, mem, off, prot)
	void *addr;
	void *mem;
	off_t off;
	int prot;
{
	struct vs_softc *sc = addr;
	struct vs_dma *p;

	if (off < 0)
		return (-1);
	for (p = sc->sc_dmas; p != NULL && KVADDR(p) != mem;
	     p = p->vd_next)
		;
	if (p == NULL) {
		printf("%s: mappage: bad addr %p\n",
		    sc->sc_dev.dv_xname, start);
		return (-1);
	}

	return (bus_dmamem_mmap(sc->sc_dmat, p->vd_segs, p->vd_nsegs, 
				off, prot, BUS_DMA_WAITOK));
}
#endif

static int
vs_get_props(void *hdl)
{
	DPRINTF(1, ("vs_get_props\n"));
	return 0 /* | dependent | half duplex | no mmap */;
}
#endif /* NAUDIO > 0 && NVS > 0*/
