/* $NetBSD: sunxi_codec.c,v 1.4 2018/04/19 18:19:17 bouyer Exp $ */

/*-
 * Copyright (c) 2014-2017 Jared McNeill <jmcneill@invisible.ca>
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

#include "opt_ddb.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sunxi_codec.c,v 1.4 2018/04/19 18:19:17 bouyer Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/gpio.h>

#include <sys/audioio.h>
#include <dev/audio_if.h>
#include <dev/auconv.h>

#include <dev/fdt/fdtvar.h>

#include <arm/sunxi/sunxi_codec.h>

#define	TX_TRIG_LEVEL	0xf
#define	RX_TRIG_LEVEL	0x7
#define	DRQ_CLR_CNT	0x3

#define	AC_DAC_DPC(_sc)		((_sc)->sc_cfg->DPC)	
#define	 DAC_DPC_EN_DA			0x80000000
#define	AC_DAC_FIFOC(_sc)	((_sc)->sc_cfg->DAC_FIFOC)
#define	 DAC_FIFOC_FS			__BITS(31,29)
#define	  DAC_FS_48KHZ			0
#define	  DAC_FS_32KHZ			1
#define	  DAC_FS_24KHZ			2
#define	  DAC_FS_16KHZ			3
#define	  DAC_FS_12KHZ			4
#define	  DAC_FS_8KHZ			5
#define	  DAC_FS_192KHZ			6
#define	  DAC_FS_96KHZ			7
#define	 DAC_FIFOC_FIFO_MODE		__BITS(25,24)
#define	  FIFO_MODE_24_31_8		0
#define	  FIFO_MODE_16_31_16		0
#define	  FIFO_MODE_16_15_0		1
#define	 DAC_FIFOC_DRQ_CLR_CNT		__BITS(22,21)
#define	 DAC_FIFOC_TX_TRIG_LEVEL	__BITS(14,8)
#define	 DAC_FIFOC_MONO_EN		__BIT(6)
#define	 DAC_FIFOC_TX_BITS		__BIT(5)
#define	 DAC_FIFOC_DRQ_EN		__BIT(4)
#define	 DAC_FIFOC_FIFO_FLUSH		__BIT(0)
#define	AC_DAC_FIFOS(_sc)	((_sc)->sc_cfg->DAC_FIFOS)
#define	AC_DAC_TXDATA(_sc)	((_sc)->sc_cfg->DAC_TXDATA)
#define	AC_ADC_FIFOC(_sc)	((_sc)->sc_cfg->ADC_FIFOC)
#define	 ADC_FIFOC_FS			__BITS(31,29)
#define	  ADC_FS_48KHZ			0
#define	 ADC_FIFOC_EN_AD		__BIT(28)
#define	 ADC_FIFOC_RX_FIFO_MODE		__BIT(24)
#define	 ADC_FIFOC_RX_TRIG_LEVEL	__BITS(12,8)
#define	 ADC_FIFOC_MONO_EN		__BIT(7)
#define	 ADC_FIFOC_RX_BITS		__BIT(6)
#define	 ADC_FIFOC_DRQ_EN		__BIT(4)
#define	 ADC_FIFOC_FIFO_FLUSH		__BIT(0)
#define	AC_ADC_FIFOS(_sc)	((_sc)->sc_cfg->ADC_FIFOS)
#define	AC_ADC_RXDATA(_sc)	((_sc)->sc_cfg->ADC_RXDATA)
#define	AC_DAC_CNT(_sc)		((_sc)->sc_cfg->DAC_CNT)
#define	AC_ADC_CNT(_sc)		((_sc)->sc_cfg->ADC_CNT)

static const struct of_compat_data compat_data[] = {
	A10_CODEC_COMPATDATA,
	A31_CODEC_COMPATDATA,
	H3_CODEC_COMPATDATA,
	{ NULL }
};

#define	CODEC_READ(sc, reg)			\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	CODEC_WRITE(sc, reg, val)		\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static int
sunxi_codec_allocdma(struct sunxi_codec_softc *sc, size_t size,
    size_t align, struct sunxi_codec_dma *dma)
{
	int error;

	dma->dma_size = size;
	error = bus_dmamem_alloc(sc->sc_dmat, dma->dma_size, align, 0,
	    dma->dma_segs, 1, &dma->dma_nsegs, BUS_DMA_WAITOK);
	if (error)
		return error;

	error = bus_dmamem_map(sc->sc_dmat, dma->dma_segs, dma->dma_nsegs,
	    dma->dma_size, &dma->dma_addr, BUS_DMA_WAITOK | BUS_DMA_COHERENT);
	if (error)
		goto free;

	error = bus_dmamap_create(sc->sc_dmat, dma->dma_size, dma->dma_nsegs,
	    dma->dma_size, 0, BUS_DMA_WAITOK, &dma->dma_map);
	if (error)
		goto unmap;

	error = bus_dmamap_load(sc->sc_dmat, dma->dma_map, dma->dma_addr,
	    dma->dma_size, NULL, BUS_DMA_WAITOK);
	if (error)
		goto destroy;

	return 0;

destroy:
	bus_dmamap_destroy(sc->sc_dmat, dma->dma_map);
unmap:
	bus_dmamem_unmap(sc->sc_dmat, dma->dma_addr, dma->dma_size);
free:
	bus_dmamem_free(sc->sc_dmat, dma->dma_segs, dma->dma_nsegs);

	return error;
}

static void
sunxi_codec_freedma(struct sunxi_codec_softc *sc, struct sunxi_codec_dma *dma)
{
	bus_dmamap_unload(sc->sc_dmat, dma->dma_map);
	bus_dmamap_destroy(sc->sc_dmat, dma->dma_map);
	bus_dmamem_unmap(sc->sc_dmat, dma->dma_addr, dma->dma_size);
	bus_dmamem_free(sc->sc_dmat, dma->dma_segs, dma->dma_nsegs);
}

static int
sunxi_codec_transfer(struct sunxi_codec_chan *ch)
{
	bus_dma_segment_t seg;

	seg.ds_addr = ch->ch_cur_phys;
	seg.ds_len = ch->ch_blksize;
	ch->ch_req.dreq_segs = &seg;
	ch->ch_req.dreq_nsegs = 1;

	return fdtbus_dma_transfer(ch->ch_dma, &ch->ch_req);
}

static int
sunxi_codec_open(void *priv, int flags)
{
	return 0;
}

static void
sunxi_codec_close(void *priv)
{
}

static int
sunxi_codec_drain(void *priv)
{
	struct sunxi_codec_softc * const sc = priv;
	uint32_t val;

	val = CODEC_READ(sc, AC_DAC_FIFOC(sc));
	CODEC_WRITE(sc, AC_DAC_FIFOC(sc), val | DAC_FIFOC_FIFO_FLUSH);

	val = CODEC_READ(sc, AC_ADC_FIFOC(sc));
	CODEC_WRITE(sc, AC_ADC_FIFOC(sc), val | ADC_FIFOC_FIFO_FLUSH);

	return 0;
}

static int
sunxi_codec_query_encoding(void *priv, struct audio_encoding *ae)
{
	struct sunxi_codec_softc * const sc = priv;

	return auconv_query_encoding(sc->sc_encodings, ae);
}

static int
sunxi_codec_set_params(void *priv, int setmode, int usemode,
    audio_params_t *play, audio_params_t *rec,
    stream_filter_list_t *pfil, stream_filter_list_t *rfil)
{
	struct sunxi_codec_softc * const sc = priv;
	int index;

	if (play && (setmode & AUMODE_PLAY)) {
		index = auconv_set_converter(&sc->sc_format, 1,
		    AUMODE_PLAY, play, true, pfil);
		if (index < 0)
			return EINVAL;
		sc->sc_pchan.ch_params = pfil->req_size > 0 ?
		    pfil->filters[0].param : *play;
	}
	if (rec && (setmode & AUMODE_RECORD)) {
		index = auconv_set_converter(&sc->sc_format, 1,
		    AUMODE_RECORD, rec, true, rfil);
		if (index < 0)
			return EINVAL;
		sc->sc_rchan.ch_params = rfil->req_size > 0 ?
		    rfil->filters[0].param : *rec;
	}

	return 0;
}

static int
sunxi_codec_set_port(void *priv, mixer_ctrl_t *mc)
{
	struct sunxi_codec_softc * const sc = priv;

	return sc->sc_cfg->set_port(sc, mc);
}

static int
sunxi_codec_get_port(void *priv, mixer_ctrl_t *mc)
{
	struct sunxi_codec_softc * const sc = priv;

	return sc->sc_cfg->get_port(sc, mc);
}

static int
sunxi_codec_query_devinfo(void *priv, mixer_devinfo_t *di)
{
	struct sunxi_codec_softc * const sc = priv;

	return sc->sc_cfg->query_devinfo(sc, di);
}

static void *
sunxi_codec_allocm(void *priv, int dir, size_t size)
{
	struct sunxi_codec_softc * const sc = priv;
	struct sunxi_codec_dma *dma;
	int error;

	dma = kmem_alloc(sizeof(*dma), KM_SLEEP);

	error = sunxi_codec_allocdma(sc, size, 16, dma);
	if (error) {
		kmem_free(dma, sizeof(*dma));
		device_printf(sc->sc_dev, "couldn't allocate DMA memory (%d)\n",
		    error);
		return NULL;
	}

	LIST_INSERT_HEAD(&sc->sc_dmalist, dma, dma_list);

	return dma->dma_addr;
}

static void
sunxi_codec_freem(void *priv, void *addr, size_t size)
{
	struct sunxi_codec_softc * const sc = priv;
	struct sunxi_codec_dma *dma;

	LIST_FOREACH(dma, &sc->sc_dmalist, dma_list)
		if (dma->dma_addr == addr) {
			sunxi_codec_freedma(sc, dma);
			LIST_REMOVE(dma, dma_list);
			kmem_free(dma, sizeof(*dma));
			break;
		}
}

static paddr_t
sunxi_codec_mappage(void *priv, void *addr, off_t off, int prot)
{
	struct sunxi_codec_softc * const sc = priv;
	struct sunxi_codec_dma *dma;

	if (off < 0)
		return -1;

	LIST_FOREACH(dma, &sc->sc_dmalist, dma_list)
		if (dma->dma_addr == addr) {
			return bus_dmamem_mmap(sc->sc_dmat, dma->dma_segs,
			    dma->dma_nsegs, off, prot, BUS_DMA_WAITOK);
		}

	return -1;
}

static int
sunxi_codec_getdev(void *priv, struct audio_device *adev)
{
	struct sunxi_codec_softc * const sc = priv;

	snprintf(adev->name, sizeof(adev->name), "Allwinner");
	snprintf(adev->version, sizeof(adev->version), "%s",
	    sc->sc_cfg->name);
	snprintf(adev->config, sizeof(adev->config), "sunxicodec");

	return 0;
}

static int
sunxi_codec_get_props(void *priv)
{
	return AUDIO_PROP_PLAYBACK|AUDIO_PROP_CAPTURE|
	    AUDIO_PROP_INDEPENDENT|AUDIO_PROP_MMAP|
	    AUDIO_PROP_FULLDUPLEX;
}

static int
sunxi_codec_round_blocksize(void *priv, int bs, int mode,
    const audio_params_t *params)
{
	bs &= ~3;
	if (bs == 0)
		bs = 4;
	return bs;
}

static size_t
sunxi_codec_round_buffersize(void *priv, int dir, size_t bufsize)
{
	return bufsize;
}

static int
sunxi_codec_trigger_output(void *priv, void *start, void *end, int blksize,
    void (*intr)(void *), void *intrarg, const audio_params_t *params)
{
	struct sunxi_codec_softc * const sc = priv;
	struct sunxi_codec_chan *ch = &sc->sc_pchan;
	struct sunxi_codec_dma *dma;
	bus_addr_t pstart;
	bus_size_t psize;
	uint32_t val;
	int error;

	pstart = 0;
	psize = (uintptr_t)end - (uintptr_t)start;

	LIST_FOREACH(dma, &sc->sc_dmalist, dma_list)
		if (dma->dma_addr == start) {
			pstart = dma->dma_map->dm_segs[0].ds_addr;
			break;
		}
	if (pstart == 0) {
		device_printf(sc->sc_dev, "bad addr %p\n", start);
		return EINVAL;
	}

	ch->ch_intr = intr;
	ch->ch_intrarg = intrarg;
	ch->ch_start_phys = ch->ch_cur_phys = pstart;
	ch->ch_end_phys = pstart + psize;
	ch->ch_blksize = blksize;

	/* Flush DAC FIFO */
	val = CODEC_READ(sc, AC_DAC_FIFOC(sc));
	CODEC_WRITE(sc, AC_DAC_FIFOC(sc), val | DAC_FIFOC_FIFO_FLUSH);

	/* Clear DAC FIFO status */
	val = CODEC_READ(sc, AC_DAC_FIFOS(sc));
	CODEC_WRITE(sc, AC_DAC_FIFOS(sc), val);

	/* Unmute output */
	if (sc->sc_cfg->mute)
		sc->sc_cfg->mute(sc, 0, ch->ch_mode);

	/* Configure DAC FIFO */
	CODEC_WRITE(sc, AC_DAC_FIFOC(sc),
	    __SHIFTIN(DAC_FS_48KHZ, DAC_FIFOC_FS) |
	    __SHIFTIN(FIFO_MODE_16_15_0, DAC_FIFOC_FIFO_MODE) |
	    __SHIFTIN(DRQ_CLR_CNT, DAC_FIFOC_DRQ_CLR_CNT) |
	    __SHIFTIN(TX_TRIG_LEVEL, DAC_FIFOC_TX_TRIG_LEVEL));

	/* Enable DAC DRQ */
	val = CODEC_READ(sc, AC_DAC_FIFOC(sc));
	CODEC_WRITE(sc, AC_DAC_FIFOC(sc), val | DAC_FIFOC_DRQ_EN);

	/* Start DMA transfer */
	error = sunxi_codec_transfer(ch);
	if (error != 0) {
		aprint_error_dev(sc->sc_dev,
		    "failed to start DMA transfer: %d\n", error);
		return error;
	}

	return 0;
}

static int
sunxi_codec_trigger_input(void *priv, void *start, void *end, int blksize,
    void (*intr)(void *), void *intrarg, const audio_params_t *params)
{
	struct sunxi_codec_softc * const sc = priv;
	struct sunxi_codec_chan *ch = &sc->sc_rchan;
	struct sunxi_codec_dma *dma;
	bus_addr_t pstart;
	bus_size_t psize;
	uint32_t val;
	int error;

	pstart = 0;
	psize = (uintptr_t)end - (uintptr_t)start;

	LIST_FOREACH(dma, &sc->sc_dmalist, dma_list)
		if (dma->dma_addr == start) {
			pstart = dma->dma_map->dm_segs[0].ds_addr;
			break;
		}
	if (pstart == 0) {
		device_printf(sc->sc_dev, "bad addr %p\n", start);
		return EINVAL;
	}

	ch->ch_intr = intr;
	ch->ch_intrarg = intrarg;
	ch->ch_start_phys = ch->ch_cur_phys = pstart;
	ch->ch_end_phys = pstart + psize;
	ch->ch_blksize = blksize;

	/* Flush ADC FIFO */
	val = CODEC_READ(sc, AC_ADC_FIFOC(sc));
	CODEC_WRITE(sc, AC_ADC_FIFOC(sc), val | ADC_FIFOC_FIFO_FLUSH);

	/* Clear ADC FIFO status */
	val = CODEC_READ(sc, AC_ADC_FIFOS(sc));
	CODEC_WRITE(sc, AC_ADC_FIFOS(sc), val);

	/* Unmute input */
	if (sc->sc_cfg->mute)
		sc->sc_cfg->mute(sc, 0, ch->ch_mode);

	/* Configure ADC FIFO */
	CODEC_WRITE(sc, AC_ADC_FIFOC(sc),
	    __SHIFTIN(ADC_FS_48KHZ, ADC_FIFOC_FS) |
	    __SHIFTIN(RX_TRIG_LEVEL, ADC_FIFOC_RX_TRIG_LEVEL) |
	    ADC_FIFOC_EN_AD | ADC_FIFOC_RX_FIFO_MODE);

	/* Enable ADC DRQ */
	val = CODEC_READ(sc, AC_ADC_FIFOC(sc));
	CODEC_WRITE(sc, AC_ADC_FIFOC(sc), val | ADC_FIFOC_DRQ_EN);

	/* Start DMA transfer */
	error = sunxi_codec_transfer(ch);
	if (error != 0) {
		aprint_error_dev(sc->sc_dev,
		    "failed to start DMA transfer: %d\n", error);
		return error;
	}

	return 0;
}

static int
sunxi_codec_halt_output(void *priv)
{
	struct sunxi_codec_softc * const sc = priv;
	struct sunxi_codec_chan *ch = &sc->sc_pchan;
	uint32_t val;

	/* Disable DMA channel */
	fdtbus_dma_halt(ch->ch_dma);

	/* Mute output */
	if (sc->sc_cfg->mute)
		sc->sc_cfg->mute(sc, 1, ch->ch_mode);

	/* Disable DAC DRQ */
	val = CODEC_READ(sc, AC_DAC_FIFOC(sc));
	CODEC_WRITE(sc, AC_DAC_FIFOC(sc), val & ~DAC_FIFOC_DRQ_EN);

	ch->ch_intr = NULL;
	ch->ch_intrarg = NULL;

	return 0;
}

static int
sunxi_codec_halt_input(void *priv)
{
	struct sunxi_codec_softc * const sc = priv;
	struct sunxi_codec_chan *ch = &sc->sc_rchan;
	uint32_t val;

	/* Disable DMA channel */
	fdtbus_dma_halt(ch->ch_dma);

	/* Mute output */
	if (sc->sc_cfg->mute)
		sc->sc_cfg->mute(sc, 1, ch->ch_mode);

	/* Disable ADC DRQ */
	val = CODEC_READ(sc, AC_ADC_FIFOC(sc));
	CODEC_WRITE(sc, AC_ADC_FIFOC(sc), val & ~ADC_FIFOC_DRQ_EN);

	return 0;
}

static void
sunxi_codec_get_locks(void *priv, kmutex_t **intr, kmutex_t **thread)
{
	struct sunxi_codec_softc * const sc = priv;

	*intr = &sc->sc_intr_lock;
	*thread = &sc->sc_lock;
}

static const struct audio_hw_if sunxi_codec_hw_if = {
	.open = sunxi_codec_open,
	.close = sunxi_codec_close,
	.drain = sunxi_codec_drain,
	.query_encoding = sunxi_codec_query_encoding,
	.set_params = sunxi_codec_set_params,
	.allocm = sunxi_codec_allocm,
	.freem = sunxi_codec_freem,
	.mappage = sunxi_codec_mappage,
	.getdev = sunxi_codec_getdev,
	.set_port = sunxi_codec_set_port,
	.get_port = sunxi_codec_get_port,
	.query_devinfo = sunxi_codec_query_devinfo,
	.get_props = sunxi_codec_get_props,
	.round_blocksize = sunxi_codec_round_blocksize,
	.round_buffersize = sunxi_codec_round_buffersize,
	.trigger_output = sunxi_codec_trigger_output,
	.trigger_input = sunxi_codec_trigger_input,
	.halt_output = sunxi_codec_halt_output,
	.halt_input = sunxi_codec_halt_input,
	.get_locks = sunxi_codec_get_locks,
};

static void
sunxi_codec_dmaintr(void *priv)
{
	struct sunxi_codec_chan * const ch = priv;
	struct sunxi_codec_softc * const sc = ch->ch_sc;

	mutex_enter(&sc->sc_intr_lock);
	ch->ch_cur_phys += ch->ch_blksize;
	if (ch->ch_cur_phys >= ch->ch_end_phys)
		ch->ch_cur_phys = ch->ch_start_phys;

	if (ch->ch_intr) {
		ch->ch_intr(ch->ch_intrarg);
		sunxi_codec_transfer(ch);
	}
	mutex_exit(&sc->sc_intr_lock);
}

static int
sunxi_codec_chan_init(struct sunxi_codec_softc *sc,
    struct sunxi_codec_chan *ch, u_int mode, const char *dmaname)
{
	ch->ch_sc = sc;
	ch->ch_mode = mode;
	ch->ch_dma = fdtbus_dma_get(sc->sc_phandle, dmaname, sunxi_codec_dmaintr, ch);
	if (ch->ch_dma == NULL) {
		aprint_error(": couldn't get dma channel \"%s\"\n", dmaname);
		return ENXIO;
	}

	if (mode == AUMODE_PLAY) {
		ch->ch_req.dreq_dir = FDT_DMA_WRITE;
		ch->ch_req.dreq_dev_phys =
		    sc->sc_baseaddr + AC_DAC_TXDATA(sc);
	} else {
		ch->ch_req.dreq_dir = FDT_DMA_READ;
		ch->ch_req.dreq_dev_phys =
		    sc->sc_baseaddr + AC_ADC_RXDATA(sc);
	}
	ch->ch_req.dreq_mem_opt.opt_bus_width = 16;
	ch->ch_req.dreq_mem_opt.opt_burst_len = 4;
	ch->ch_req.dreq_dev_opt.opt_bus_width = 16;
	ch->ch_req.dreq_dev_opt.opt_burst_len = 4;

	return 0;
}

static int
sunxi_codec_clock_init(int phandle)
{
	struct fdtbus_reset *rst;
	struct clk *clk;
	int error;

	/* Set codec clock to 24.576MHz, suitable for 48 kHz sampling rates */
	clk = fdtbus_clock_get(phandle, "codec");
	if (clk == NULL) {
		aprint_error(": couldn't find codec clock\n");
		return ENXIO;
	}
	error = clk_set_rate(clk, 24576000);
	if (error != 0) {
		aprint_error(": couldn't set codec clock rate: %d\n", error);
		return error;
	}
	error = clk_enable(clk);
	if (error != 0) {
		aprint_error(": couldn't enable codec clock: %d\n", error);
		return error;
	}

	/* Enable APB clock */
	clk = fdtbus_clock_get(phandle, "apb");
	if (clk == NULL) {
		aprint_error(": couldn't find apb clock\n");
		return ENXIO;
	}
	error = clk_enable(clk);
	if (error != 0) {
		aprint_error(": couldn't enable apb clock: %d\n", error);
		return error;
	}

	/* De-assert reset */
	rst = fdtbus_reset_get_index(phandle, 0);
	if (rst != NULL) {
		error = fdtbus_reset_deassert(rst);
		if (error != 0) {
			aprint_error(": couldn't de-assert reset: %d\n", error);
			return error;
		}
	}

	return 0;
}

static int
sunxi_codec_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compat_data(faa->faa_phandle, compat_data);
}

static void
sunxi_codec_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_codec_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;
	uint32_t val;
	int error;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	if (sunxi_codec_clock_init(phandle) != 0)
		return;

	sc->sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_baseaddr = addr;
	sc->sc_bst = faa->faa_bst;
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}
	sc->sc_dmat = faa->faa_dmat;
	LIST_INIT(&sc->sc_dmalist);
	sc->sc_cfg = (void *)of_search_compatible(phandle, compat_data)->data;
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_SCHED);

	if (sunxi_codec_chan_init(sc, &sc->sc_pchan, AUMODE_PLAY, "tx") != 0 ||
	    sunxi_codec_chan_init(sc, &sc->sc_rchan, AUMODE_RECORD, "rx") != 0) {
		aprint_error(": couldn't setup channels\n");
		return;
	}

	/* Optional PA mute GPIO */
	sc->sc_pin_pa = fdtbus_gpio_acquire(phandle, "allwinner,pa-gpios", GPIO_PIN_OUTPUT);

	aprint_naive("\n");
	aprint_normal(": %s\n", sc->sc_cfg->name);

	/* Enable DAC */
	val = CODEC_READ(sc, AC_DAC_DPC(sc));
	val |= DAC_DPC_EN_DA;
	CODEC_WRITE(sc, AC_DAC_DPC(sc), val);

	/* Initialize codec */
	if (sc->sc_cfg->init(sc) != 0) {
		aprint_error_dev(self, "couldn't initialize codec\n");
		return;
	}

	sc->sc_format.mode = AUMODE_PLAY|AUMODE_RECORD;
	sc->sc_format.encoding = AUDIO_ENCODING_SLINEAR_LE;
	sc->sc_format.validbits = 16;
	sc->sc_format.precision = 16;
	sc->sc_format.channels = 2;
	sc->sc_format.channel_mask = AUFMT_STEREO;
	sc->sc_format.frequency_type = 0;
	sc->sc_format.frequency[0] = sc->sc_format.frequency[1] = 48000;

	error = auconv_create_encodings(&sc->sc_format, 1, &sc->sc_encodings);
	if (error) {
		aprint_error_dev(self, "couldn't create encodings\n");
		return;
	}

	audio_attach_mi(&sunxi_codec_hw_if, sc, self);
}

CFATTACH_DECL_NEW(sunxi_codec, sizeof(struct sunxi_codec_softc),
    sunxi_codec_match, sunxi_codec_attach, NULL, NULL);

#ifdef DDB
void sunxicodec_dump(void);

void
sunxicodec_dump(void)
{
	struct sunxi_codec_softc *sc;
	device_t dev;

	dev = device_find_by_driver_unit("sunxicodec", 0);
	if (dev == NULL)
		return;
	sc = device_private(dev);

	device_printf(dev, "AC_DAC_DPC:   %08x\n", CODEC_READ(sc, AC_DAC_DPC(sc)));
	device_printf(dev, "AC_DAC_FIFOC: %08x\n", CODEC_READ(sc, AC_DAC_FIFOC(sc)));
	device_printf(dev, "AC_DAC_FIFOS: %08x\n", CODEC_READ(sc, AC_DAC_FIFOS(sc)));
	device_printf(dev, "AC_ADC_FIFOC: %08x\n", CODEC_READ(sc, AC_ADC_FIFOC(sc)));
	device_printf(dev, "AC_ADC_FIFOS: %08x\n", CODEC_READ(sc, AC_ADC_FIFOS(sc)));
	device_printf(dev, "AC_DAC_CNT:   %08x\n", CODEC_READ(sc, AC_DAC_CNT(sc)));
	device_printf(dev, "AC_ADC_CNT:   %08x\n", CODEC_READ(sc, AC_ADC_CNT(sc)));
}
#endif
