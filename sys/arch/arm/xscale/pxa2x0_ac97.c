/*	$NetBSD: pxa2x0_ac97.c,v 1.3 2005/12/24 20:06:52 perry Exp $	*/

/*
 * Copyright (c) 2003, 2005 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/select.h>
#include <sys/audioio.h>

#include <machine/intr.h>
#include <machine/bus.h>

#include <dev/audio_if.h>
#include <dev/audiovar.h>
#include <dev/mulaw.h>
#include <dev/auconv.h>
#include <dev/ic/ac97reg.h>
#include <dev/ic/ac97var.h>

#include <arm/xscale/pxa2x0reg.h>
#include <arm/xscale/pxa2x0var.h>
#include <arm/xscale/pxa2x0_gpio.h>
#include <arm/xscale/pxa2x0_dmac.h>

#include "locators.h"

struct acu_dma {
	bus_dmamap_t ad_map;
	caddr_t ad_addr;
#define	ACU_N_SEGS	1	/* XXX: We don't support > 1 */
	bus_dma_segment_t ad_segs[ACU_N_SEGS];
	int ad_nsegs;
	size_t ad_size;
	struct dmac_xfer *ad_dx;
	struct acu_dma *ad_next;
};

#define KERNADDR(ad) ((void *)((ad)->ad_addr))

struct acu_softc {
	struct device sc_dev;
	bus_space_tag_t sc_bust;
	bus_dma_tag_t sc_dmat;
	bus_space_handle_t sc_bush;
	void *sc_irqcookie;
	int sc_in_reset;
	u_int sc_dac_rate;
	u_int sc_adc_rate;

	/* List of DMA ring-buffers allocated by acu_malloc() */
	struct acu_dma *sc_dmas;

	/* Dummy DMA segment which points to the AC97 PCM Fifo register */
	bus_dma_segment_t sc_dr;

	/* PCM Output (Tx) state */
	dmac_peripheral_t sc_txp;
	struct acu_dma *sc_txdma;
	void (*sc_txfunc)(void *);
	void *sc_txarg;

	/* PCM Input (Rx) state */
	dmac_peripheral_t sc_rxp;
	struct acu_dma *sc_rxdma;
	void (*sc_rxfunc)(void *);
	void *sc_rxarg;

	/* AC97 Codec State */
	struct ac97_codec_if *sc_codec_if;
	struct ac97_host_if sc_host_if;

	/* Child audio(4) device */
	struct device *sc_audiodev;

	/* auconv encodings */
	struct audio_encoding_set *sc_encodings;
};

static int	pxaacu_match(struct device *, struct cfdata *, void *);
static void	pxaacu_attach(struct device *, struct device *, void *);

CFATTACH_DECL(pxaacu, sizeof(struct acu_softc),
    pxaacu_match, pxaacu_attach, NULL, NULL);

static int acu_codec_attach(void *, struct ac97_codec_if *);
static int acu_codec_read(void *, u_int8_t, u_int16_t *);
static int acu_codec_write(void *, u_int8_t, u_int16_t);
static int acu_codec_reset(void *);
static int acu_intr(void *);

static int acu_open(void *, int);
static void acu_close(void *);
static int acu_query_encoding(void *, struct audio_encoding *);
static int acu_set_params(void *, int, int, audio_params_t *, audio_params_t *,
	    stream_filter_list_t *, stream_filter_list_t *);
static int acu_round_blocksize(void *, int, int, const audio_params_t *);
static int acu_halt_output(void *);
static int acu_halt_input(void *);
static int acu_trigger_output(void *, void *, void *, int, void (*)(void *),
	    void *, const audio_params_t *);
static int acu_trigger_input(void *, void *, void *, int, void (*)(void *),
	    void *, const audio_params_t *);
static void acu_tx_loop_segment(struct dmac_xfer *, int);
static void acu_rx_loop_segment(struct dmac_xfer *, int);
static int acu_getdev(void *, struct audio_device *);
static int acu_mixer_set_port(void *, mixer_ctrl_t *);
static int acu_mixer_get_port(void *, mixer_ctrl_t *);
static int acu_query_devinfo(void *, mixer_devinfo_t *);
static void *acu_malloc(void *, int, size_t, struct malloc_type *, int);
static void acu_free(void *, void *, struct malloc_type *);
static size_t acu_round_buffersize(void *, int, size_t);
static paddr_t acu_mappage(void *, void *, off_t, int);
static int acu_get_props(void *);

struct audio_hw_if acu_hw_if = {
	acu_open,
	acu_close,
	NULL,
	acu_query_encoding,
	acu_set_params,
	acu_round_blocksize,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	acu_halt_output,
	acu_halt_input,
	NULL,
	acu_getdev,
	NULL,
	acu_mixer_set_port,
	acu_mixer_get_port,
	acu_query_devinfo,
	acu_malloc,
	acu_free,
	acu_round_buffersize,
	acu_mappage,
	acu_get_props,
	acu_trigger_output,
	acu_trigger_input,
	NULL,
};

struct audio_device acu_device = {
	"PXA250 AC97",
	"",
	"acu"
};

static const struct audio_format acu_formats[] = {
	{NULL, AUMODE_PLAY | AUMODE_RECORD, AUDIO_ENCODING_SLINEAR_LE, 16, 16,
	 2, AUFMT_STEREO, 0, {4000, 48000}}
};
#define	ACU_NFORMATS	(sizeof(acu_formats) / sizeof(struct audio_format))

static inline u_int32_t
acu_reg_read(struct acu_softc *sc, int reg)
{

	return (bus_space_read_4(sc->sc_bust, sc->sc_bush, reg));
}

static inline void
acu_reg_write(struct acu_softc *sc, int reg, u_int32_t val)
{

	bus_space_write_4(sc->sc_bust, sc->sc_bush, reg, val);
}

static inline int
acu_codec_ready(struct acu_softc *sc)
{

	return (acu_reg_read(sc, AC97_GSR) & GSR_PCR);
}

static inline int
acu_wait_gsr(struct acu_softc *sc, u_int32_t bit)
{
	int timeout;
	u_int32_t rv;

	for (timeout = 5000; timeout; timeout--) {
		if ((rv = acu_reg_read(sc, AC97_GSR)) & bit) {
			acu_reg_write(sc, AC97_GSR, rv | bit);
			return (0);
		}
		delay(1);
	}

	return (1);
}

static int
pxaacu_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct pxaip_attach_args *pxa = aux;

	if (pxa->pxa_addr != PXA2X0_AC97_BASE ||
	    pxa->pxa_intr != PXA2X0_INT_AC97)
		return (0);

	pxa->pxa_size = PXA2X0_AC97_SIZE;

	return (1);
}

static void
pxaacu_attach(struct device *parent, struct device *self, void *aux)
{
	struct acu_softc *sc = (struct acu_softc *)self;
	struct pxaip_attach_args *pxa = aux;

	sc->sc_bust = pxa->pxa_iot;
	sc->sc_dmat = pxa->pxa_dmat;

	aprint_naive("\n");
	aprint_normal(": AC97 Controller\n");

	if (bus_space_map(sc->sc_bust, pxa->pxa_addr, pxa->pxa_size, 0,
	    &sc->sc_bush)) {
		aprint_error("%s: Can't map registers!\n", sc->sc_dev.dv_xname);
		return;
	}

	sc->sc_irqcookie = pxa2x0_intr_establish(pxa->pxa_intr, IPL_AUDIO,
	    acu_intr, sc);
	KASSERT(sc->sc_irqcookie != NULL);

	/* Make sure the AC97 clock is enabled */
	pxa2x0_clkman_config(CKEN_AC97, TRUE);
	delay(100);
	pxa2x0_gpio_set_function(31, GPIO_CLR | GPIO_ALT_FN_2_OUT);
	pxa2x0_gpio_set_function(30, GPIO_CLR | GPIO_ALT_FN_2_OUT);
	pxa2x0_gpio_set_function(28, GPIO_CLR | GPIO_ALT_FN_1_IN);
	pxa2x0_gpio_set_function(29, GPIO_CLR | GPIO_ALT_FN_1_IN);
	delay(100);

	/* Do a cold reset */
	acu_reg_write(sc, AC97_GCR, 0);
	delay(100);
	acu_reg_write(sc, AC97_GCR, GCR_COLD_RST);
	delay(100);
	acu_reg_write(sc, AC97_CAR, 0);

	if (acu_wait_gsr(sc, GSR_PCR)) {
		acu_reg_write(sc, AC97_GCR, 0);
		delay(100);
		pxa2x0_clkman_config(CKEN_AC97, FALSE);
		bus_space_unmap(sc->sc_bust, sc->sc_bush, pxa->pxa_size);
		aprint_error("%s: Primary codec not ready\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	sc->sc_dr.ds_addr = pxa->pxa_addr + AC97_PCDR;
	sc->sc_dr.ds_len = 4;

	sc->sc_codec_if = NULL;
	sc->sc_host_if.arg = sc;
	sc->sc_host_if.attach = acu_codec_attach;
	sc->sc_host_if.read = acu_codec_read;
	sc->sc_host_if.write = acu_codec_write;
	sc->sc_host_if.reset = acu_codec_reset;
	sc->sc_host_if.flags = NULL;
	sc->sc_in_reset = 0;
	sc->sc_dac_rate = sc->sc_adc_rate = 0;

	if (ac97_attach(&sc->sc_host_if, &sc->sc_dev)) {
		aprint_error("%s: Failed to attach primary codec\n",
		    sc->sc_dev.dv_xname);
 fail:
		acu_reg_write(sc, AC97_GCR, 0);
		delay(100);
		pxa2x0_clkman_config(CKEN_AC97, FALSE);
		bus_space_unmap(sc->sc_bust, sc->sc_bush, pxa->pxa_size);
		return;
	}

	if (auconv_create_encodings(acu_formats, ACU_NFORMATS,
	    &sc->sc_encodings)) {
		aprint_error("%s: Failed to create encodings\n",
		    sc->sc_dev.dv_xname);
		if (sc->sc_codec_if != NULL)
			(sc->sc_codec_if->vtbl->detach)(sc->sc_codec_if);
		goto fail;
	}

	sc->sc_audiodev = audio_attach_mi(&acu_hw_if, sc, &sc->sc_dev);

	/*
	 * As a work-around for braindamage in the PXA250's AC97 controller
	 * (see errata #125), we hold the ACUNIT/Codec in Cold Reset until
	 * acu_open() is called. acu_close() also puts the controller into
	 * Cold Reset.
	 *
	 * While this won't necessarily prevent Rx FIFO overruns, it at least
	 * allows the user to recover by closing then re-opening the audio
	 * device.
	 */
	acu_reg_write(sc, AC97_GCR, 0);
	sc->sc_in_reset = 1;
}

static int
acu_codec_attach(void *arg, struct ac97_codec_if *aci)
{
	struct acu_softc *sc = arg;

	sc->sc_codec_if = aci;
	return (0);
}

static int
acu_codec_read(void *arg, u_int8_t codec_reg, u_int16_t *valp)
{
	struct acu_softc *sc = arg;
	u_int32_t val;
	int s, reg, rv = 1;

	/*
	 * If we're currently closed, return non-zero. The ac97 frontend
	 * will use its cached copy of the register instead.
	 */
	if (sc->sc_in_reset)
		return (1);

	reg = AC97_CODEC_BASE(0) + codec_reg * 2;

	s = splaudio();

	if (!acu_codec_ready(sc) || (acu_reg_read(sc, AC97_CAR) & CAR_CAIP))
		goto out_nocar;

	val = acu_reg_read(sc, AC97_GSR);
	val |= GSR_RDCS | GSR_SDONE;
	acu_reg_write(sc, AC97_GSR, val);

	/*
	 * Dummy read to initiate the real read access
	 */
	(void) acu_reg_read(sc, reg);
	if (acu_wait_gsr(sc, GSR_SDONE))
		goto out;

	(void) acu_reg_read(sc, reg);
	if (acu_wait_gsr(sc, GSR_SDONE))
		goto out;

	val = acu_reg_read(sc, AC97_GSR);
	if (val & GSR_RDCS)
		goto out;

	*valp = acu_reg_read(sc, reg);
	if (acu_wait_gsr(sc, GSR_SDONE))
		goto out;

	rv = 0;

out:
	acu_reg_write(sc, AC97_CAR, 0);
out_nocar:
	splx(s);
	delay(10);
	return (rv);
}

static int
acu_codec_write(void *arg, u_int8_t codec_reg, u_int16_t val)
{
	struct acu_softc *sc = arg;
	u_int16_t rv;
	int s;

	/*
	 * If we're currently closed, chances are the user is just
	 * tweaking mixer settings. Pretend the write succeeded.
	 * The ac97 frontend will cache the value anyway, and it'll
	 * be written correctly when the driver is opened.
	 */
	if (sc->sc_in_reset)
		return (0);

	s = splaudio();

	if (!acu_codec_ready(sc) || (acu_reg_read(sc, AC97_CAR) & CAR_CAIP)) {
		splx(s);
		return (1);
	}

	rv = acu_reg_read(sc, AC97_GSR);
	rv |= GSR_RDCS | GSR_CDONE;
	acu_reg_write(sc, AC97_GSR, rv);

	acu_reg_write(sc, AC97_CODEC_BASE(0) + codec_reg * 2, val);

	/*
	 * Wait for the write to complete
	 */
	(void) acu_wait_gsr(sc, GSR_CDONE);
	acu_reg_write(sc, AC97_CAR, 0);

	splx(s);
	delay(10);
	return (0);
}

static int
acu_codec_reset(void *arg)
{
	struct acu_softc *sc = arg;
	u_int32_t rv;

	rv = acu_reg_read(sc, AC97_GCR);
	acu_reg_write(sc, AC97_GCR, rv | GCR_WARM_RST);
	delay(100);
	acu_reg_write(sc, AC97_GCR, rv);
	delay(100);

	if (acu_wait_gsr(sc, GSR_PCR)) {
		printf("%s: acu_codec_reset: failed to ready after reset\n",
		    sc->sc_dev.dv_xname);
		return (ETIMEDOUT);
	}

	return (0);
}

static int
acu_intr(void *arg)
{
	struct acu_softc *sc = arg;
	u_int32_t gsr, reg;

	gsr = acu_reg_read(sc, AC97_GSR);

	/*
	 * Tx FIFO underruns are no big deal. Just log it and ignore and
	 * subsequent underruns until the next time acu_trigger_output()
	 * is called.
	 */
	if ((gsr & GSR_POINT) && (acu_reg_read(sc, AC97_POCR) & AC97_FEFIE)) {
		acu_reg_write(sc, AC97_POCR, 0);
		reg = acu_reg_read(sc, AC97_POSR);
		acu_reg_write(sc, AC97_POSR, reg);
		printf("%s: Tx PCM Fifo underrun\n", sc->sc_dev.dv_xname);
	}

	/*
	 * Rx FIFO overruns are a different story. See PAX250 Errata #125
	 * for the gory details.
	 * I don't see any way to gracefully recover from this problem,
	 * other than a issuing a Cold Reset in acu_close().
	 * The best we can do here is to report the problem on the console.
	 */
	if ((gsr & GSR_PIINT) && (acu_reg_read(sc, AC97_PICR) & AC97_FEFIE)) {
		acu_reg_write(sc, AC97_PICR, 0);
		reg = acu_reg_read(sc, AC97_PISR);
		acu_reg_write(sc, AC97_PISR, reg);
		printf("%s: Rx PCM Fifo overrun\n", sc->sc_dev.dv_xname);
	}

	return (1);
}

static int
acu_open(void *arg, int flags)
{
	struct acu_softc *sc = arg;

	/*
	 * Deassert Cold Reset
	 */
	acu_reg_write(sc, AC97_GCR, GCR_COLD_RST);
	delay(100);
	acu_reg_write(sc, AC97_CAR, 0);

	/*
	 * Wait for the primary codec to become ready
	 */
	if (acu_wait_gsr(sc, GSR_PCR))
		return (EIO);
	sc->sc_in_reset = 0;

	/*
	 * Restore the codec port settings
	 */
	sc->sc_codec_if->vtbl->restore_ports(sc->sc_codec_if);

	/*
	 * Need to reprogram the sample rates, since 'restore_ports'
	 * doesn't do it.
	 *
	 * XXX: These aren't the only two sample rate registers ...
	 */
	if (sc->sc_dac_rate)
		(void) sc->sc_codec_if->vtbl->set_rate(sc->sc_codec_if,
		    AC97_REG_PCM_FRONT_DAC_RATE, &sc->sc_dac_rate);
	if (sc->sc_adc_rate)
		(void) sc->sc_codec_if->vtbl->set_rate(sc->sc_codec_if,
		    AC97_REG_PCM_LR_ADC_RATE, &sc->sc_adc_rate);

	return (0);
}

static void
acu_close(void *arg)
{
	struct acu_softc *sc = arg;
    
	/*
	 * Make sure the hardware is quiescent
	 */
	acu_halt_output(sc);
	acu_halt_input(sc);
	delay(100);

	/* Assert Cold Reset */
	acu_reg_write(sc, AC97_GCR, 0);
	sc->sc_in_reset = 1;
}

static int
acu_query_encoding(void *arg, struct audio_encoding *fp)
{
	struct acu_softc *sc = arg;

	return (auconv_query_encoding(sc->sc_encodings, fp));
}

static int
acu_set_params(void *arg, int setmode, int usemode,
    audio_params_t *play, audio_params_t *rec,
    stream_filter_list_t *pfil, stream_filter_list_t *rfil)
{
	struct acu_softc *sc = arg;
	struct audio_params *p;
	stream_filter_list_t *fil;
	int mode, err;

	for (mode = AUMODE_RECORD; mode != -1; 
	    mode = (mode == AUMODE_RECORD) ? AUMODE_PLAY : -1) {
		if ((setmode & mode) == 0)
			continue;

		p = (mode == AUMODE_PLAY) ? play : rec;

		if (p->sample_rate < 4000 || p->sample_rate > 48000 ||
		    (p->precision != 8 && p->precision != 16) ||
		    (p->channels != 1 && p->channels != 2)) {
			printf("acu_set_params: precision/channels botch\n");
			printf("acu_set_params: rate %d, prec %d, chan %d\n",
			    p->sample_rate, p->precision, p->channels);
			return (EINVAL);
		}

		fil = (mode == AUMODE_PLAY) ? pfil : rfil;
		err = auconv_set_converter(acu_formats, ACU_NFORMATS,
		    mode, p, TRUE, fil);
		if (err < 0)
			return (EINVAL);

		if (mode == AUMODE_PLAY) {
			err = sc->sc_codec_if->vtbl->set_rate(sc->sc_codec_if,
			    AC97_REG_PCM_FRONT_DAC_RATE, &play->sample_rate);
			sc->sc_dac_rate = play->sample_rate;
		} else {
			err = sc->sc_codec_if->vtbl->set_rate(sc->sc_codec_if,
			    AC97_REG_PCM_LR_ADC_RATE, &rec->sample_rate);
			sc->sc_adc_rate = rec->sample_rate;
		}
		if (err)
			return (EINVAL);
	}

	return (0);
}

static int
acu_round_blocksize(void *arg, int blk, int mode, const audio_params_t *param)
{

	return (blk & ~0x1f);
}

static int
acu_getdev(void *addr, struct audio_device *retp)
{

	*retp = acu_device;
	return (0);
}

static int
acu_mixer_set_port(void *arg, mixer_ctrl_t *cp)
{
	struct acu_softc *sc = arg;

	return (sc->sc_codec_if->vtbl->mixer_set_port(sc->sc_codec_if, cp));
}

static int
acu_mixer_get_port(void *arg, mixer_ctrl_t *cp)
{
	struct acu_softc *sc = arg;

	return (sc->sc_codec_if->vtbl->mixer_get_port(sc->sc_codec_if, cp));
}

static int
acu_query_devinfo(void *arg, mixer_devinfo_t *dip)
{
	struct acu_softc *sc = arg;

	return (sc->sc_codec_if->vtbl->query_devinfo(sc->sc_codec_if, dip));
}

static void *
acu_malloc(void *arg, int direction, size_t size,
    struct malloc_type *pool, int flags)
{
	struct acu_softc *sc = arg;
	struct acu_dma *ad;
	int error;

	if ((ad = malloc(sizeof(*ad), pool, flags)) == NULL)
		return (NULL);

	if ((ad->ad_dx = pxa2x0_dmac_allocate_xfer(M_NOWAIT)) == NULL)
		goto error;

	ad->ad_size = size;

	error = bus_dmamem_alloc(sc->sc_dmat, size, 16, 0, ad->ad_segs,
	    ACU_N_SEGS, &ad->ad_nsegs, BUS_DMA_NOWAIT);
	if (error)
		goto free_xfer;

	error = bus_dmamem_map(sc->sc_dmat, ad->ad_segs, ad->ad_nsegs, size,
	    &ad->ad_addr, BUS_DMA_NOWAIT | BUS_DMA_COHERENT | BUS_DMA_NOCACHE);
	if (error)
		goto free_dmamem;

	error = bus_dmamap_create(sc->sc_dmat, size, 1, size, 0,
	    BUS_DMA_NOWAIT | BUS_DMA_ALLOCNOW, &ad->ad_map);
	if (error)
		goto unmap_dmamem;

	error = bus_dmamap_load(sc->sc_dmat, ad->ad_map, ad->ad_addr, size,
	    NULL, BUS_DMA_NOWAIT);
	if (error) {
		bus_dmamap_destroy(sc->sc_dmat, ad->ad_map);
unmap_dmamem:	bus_dmamem_unmap(sc->sc_dmat, ad->ad_addr, size);
free_dmamem:	bus_dmamem_free(sc->sc_dmat, ad->ad_segs, ad->ad_nsegs);
free_xfer:	pxa2x0_dmac_free_xfer(ad->ad_dx);
error:		free(ad, pool);
		return (NULL);
	}

	ad->ad_dx->dx_cookie = sc;
	ad->ad_dx->dx_priority = DMAC_PRIORITY_HIGH;
	ad->ad_dx->dx_dev_width = DMAC_DEV_WIDTH_4;
	ad->ad_dx->dx_burst_size = DMAC_BURST_SIZE_32;

	ad->ad_next = sc->sc_dmas;
	sc->sc_dmas = ad;
	return (KERNADDR(ad));
}

static void
acu_free(void *arg, void *ptr, struct malloc_type *pool)
{
	struct acu_softc *sc = arg;
	struct acu_dma *ad, **adp;

	for (adp = &sc->sc_dmas; (ad = *adp) != NULL; adp = &ad->ad_next) {
		if (KERNADDR(ad) == ptr) {
			pxa2x0_dmac_abort_xfer(ad->ad_dx);
			pxa2x0_dmac_free_xfer(ad->ad_dx);
			ad->ad_segs[0].ds_len = ad->ad_size;	/* XXX */
			bus_dmamap_unload(sc->sc_dmat, ad->ad_map);
			bus_dmamap_destroy(sc->sc_dmat, ad->ad_map);
			bus_dmamem_unmap(sc->sc_dmat, ad->ad_addr, ad->ad_size);
			bus_dmamem_free(sc->sc_dmat, ad->ad_segs, ad->ad_nsegs);
			*adp = ad->ad_next;
			free(ad, pool);
			return;
		}
	}
}

static size_t
acu_round_buffersize(void *arg, int direction, size_t size)
{

	return (size);
}

static paddr_t
acu_mappage(void *arg, void *mem, off_t off, int prot)
{
	struct acu_softc *sc = arg;
	struct acu_dma *ad;

	if (off < 0)
		return (-1);
	for (ad = sc->sc_dmas; ad && KERNADDR(ad) != mem; ad = ad->ad_next)
		;
	if (ad == NULL)
		return (-1);
	return (bus_dmamem_mmap(sc->sc_dmat, ad->ad_segs, ad->ad_nsegs, 
	    off, prot, BUS_DMA_WAITOK));
}

static int
acu_get_props(void *arg)
{

	return (AUDIO_PROP_MMAP|AUDIO_PROP_INDEPENDENT|AUDIO_PROP_FULLDUPLEX);
}

static int
acu_halt_output(void *arg)
{
	struct acu_softc *sc = arg;
	int s;

	s = splaudio();
	if (sc->sc_txdma) {
		acu_reg_write(sc, AC97_POCR, 0);
		acu_reg_write(sc, AC97_POSR, AC97_FIFOE);
		pxa2x0_dmac_abort_xfer(sc->sc_txdma->ad_dx);
		sc->sc_txdma = NULL;
	}
	splx(s);
	return (0);
}

static int
acu_halt_input(void *arg)
{
	struct acu_softc *sc = arg;
	int s;

	s = splaudio();
	if (sc->sc_rxdma) {
		acu_reg_write(sc, AC97_PICR, 0);
		acu_reg_write(sc, AC97_PISR, AC97_FIFOE);
		pxa2x0_dmac_abort_xfer(sc->sc_rxdma->ad_dx);
		sc->sc_rxdma = NULL;
	}
	splx(s);
	return (0);
}

static int
acu_trigger_output(void *arg, void *start, void *end, int blksize,
    void (*tx_func)(void *), void *tx_arg, const audio_params_t *param)
{
	struct acu_softc *sc = arg;
	struct dmac_xfer *dx;
	struct acu_dma *ad;
	int rv;

	if (sc->sc_txdma)
		return (EBUSY);

	sc->sc_txfunc = tx_func;
	sc->sc_txarg = tx_arg;

	for (ad = sc->sc_dmas; ad && KERNADDR(ad) != start; ad = ad->ad_next)
		;
	if (ad == NULL) {
		printf("acu_trigger_output: bad addr %p\n", start);
		return (EINVAL);
	}

	sc->sc_txdma = ad;
	ad->ad_segs[0].ds_addr = ad->ad_map->dm_segs[0].ds_addr;
	ad->ad_segs[0].ds_len = (uintptr_t)end - (uintptr_t)start;

	/*
	 * Fix up a looping DMA request.
	 * The 'done' function will be called for every 'blksize' bytes
	 * transferred by the DMA engine.
	 */
	dx = ad->ad_dx;
	dx->dx_done = acu_tx_loop_segment;
	dx->dx_peripheral = DMAC_PERIPH_AC97AUDIOTX;
	dx->dx_flow = DMAC_FLOW_CTRL_DEST;
	dx->dx_loop_notify = blksize;
	dx->dx_desc[DMAC_DESC_SRC].xd_addr_hold = FALSE;
	dx->dx_desc[DMAC_DESC_SRC].xd_nsegs = ad->ad_nsegs;
	dx->dx_desc[DMAC_DESC_SRC].xd_dma_segs = ad->ad_segs;
	dx->dx_desc[DMAC_DESC_DST].xd_addr_hold = TRUE;
	dx->dx_desc[DMAC_DESC_DST].xd_nsegs = 1;
	dx->dx_desc[DMAC_DESC_DST].xd_dma_segs = &sc->sc_dr;

	rv = pxa2x0_dmac_start_xfer(dx);
	if (rv == 0) {
		/*
		 * XXX: We should only do this once the request has been
		 * loaded into a DMAC channel.
		 */
		acu_reg_write(sc, AC97_POSR, AC97_FIFOE);
		acu_reg_write(sc, AC97_POCR, AC97_FEFIE);
	}

	return (rv);
}

static int
acu_trigger_input(void *arg, void *start, void *end, int blksize,
    void (*rx_func)(void *), void *rx_arg, const audio_params_t *param)
{
	struct acu_softc *sc = arg;
	struct dmac_xfer *dx;
	struct acu_dma *ad;
	int rv;

	if (sc->sc_rxdma)
		return (EBUSY);

	sc->sc_rxfunc = rx_func;
	sc->sc_rxarg = rx_arg;

	for (ad = sc->sc_dmas; ad && KERNADDR(ad) != start; ad = ad->ad_next)
		;
	if (ad == NULL) {
		printf("acu_trigger_input: bad addr %p\n", start);
		return (EINVAL);
	}

	sc->sc_rxdma = ad;
	ad->ad_segs[0].ds_addr = ad->ad_map->dm_segs[0].ds_addr;
	ad->ad_segs[0].ds_len = (uintptr_t)end - (uintptr_t)start;

	/*
	 * Fix up a looping DMA request.
	 * The 'done' function will be called for every 'blksize' bytes
	 * transferred by the DMA engine.
	 */
	dx = ad->ad_dx;
	dx->dx_done = acu_rx_loop_segment;
	dx->dx_peripheral = DMAC_PERIPH_AC97AUDIORX;
	dx->dx_flow = DMAC_FLOW_CTRL_SRC;
	dx->dx_loop_notify = blksize;
	dx->dx_desc[DMAC_DESC_DST].xd_addr_hold = FALSE;
	dx->dx_desc[DMAC_DESC_DST].xd_nsegs = ad->ad_nsegs;
	dx->dx_desc[DMAC_DESC_DST].xd_dma_segs = ad->ad_segs;
	dx->dx_desc[DMAC_DESC_SRC].xd_addr_hold = TRUE;
	dx->dx_desc[DMAC_DESC_SRC].xd_nsegs = 1;
	dx->dx_desc[DMAC_DESC_SRC].xd_dma_segs = &sc->sc_dr;

	rv = pxa2x0_dmac_start_xfer(dx);

	if (rv == 0) {
		/*
		 * XXX: We should only do this once the request has been
		 * loaded into a DMAC channel.
		 */
		acu_reg_write(sc, AC97_PISR, AC97_FIFOE);
		acu_reg_write(sc, AC97_PICR, AC97_FEFIE);
	}

	return (rv);
}

static void
acu_tx_loop_segment(struct dmac_xfer *dx, int status)
{
	struct acu_softc *sc = dx->dx_cookie;
	struct acu_dma *ad;
	int s;

	if ((ad = sc->sc_txdma) == NULL)
		panic("acu_tx_loop_segment: bad TX dma descriptor!");

	if (ad->ad_dx != dx)
		panic("acu_tx_loop_segment: xfer mismatch!");

	if (status) {
		printf(
		    "%s: acu_tx_loop_segment: non-zero completion status %d\n",
		    sc->sc_dev.dv_xname, status);
	}

	s = splaudio();
	(sc->sc_txfunc)(sc->sc_txarg);
	splx(s);
}

static void
acu_rx_loop_segment(struct dmac_xfer *dx, int status)
{
	struct acu_softc *sc = dx->dx_cookie;
	struct acu_dma *ad;
	int s;

	if ((ad = sc->sc_rxdma) == NULL)
		panic("acu_rx_loop_segment: bad RX dma descriptor!");

	if (ad->ad_dx != dx)
		panic("acu_rx_loop_segment: xfer mismatch!");

	if (status) {
		printf(
		    "%s: acu_rx_loop_segment: non-zero completion status %d\n",
		    sc->sc_dev.dv_xname, status);
	}

	s = splaudio();
	(sc->sc_rxfunc)(sc->sc_rxarg);
	splx(s);
}
