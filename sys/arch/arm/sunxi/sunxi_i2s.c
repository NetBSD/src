/* $NetBSD: sunxi_i2s.c,v 1.2.2.2 2018/05/21 04:35:59 pgoyette Exp $ */

/*-
 * Copyright (c) 2018 Jared McNeill <jmcneill@invisible.ca>
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sunxi_i2s.c,v 1.2.2.2 2018/05/21 04:35:59 pgoyette Exp $");

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

#define	SUNXI_I2S_CLK_RATE	24576000

#define	DA_CTL		0x00
#define	 DA_CTL_SDO_EN	__BIT(8)
#define	 DA_CTL_MS	__BIT(5)
#define	 DA_CTL_PCM	__BIT(4)
#define	 DA_CTL_TXEN	__BIT(2)
#define	 DA_CTL_RXEN	__BIT(1)
#define	 DA_CTL_GEN	__BIT(0)
#define	DA_FAT0		0x04
#define	 DA_FAT0_LRCP	__BIT(7)
#define	  DA_LRCP_NORMAL	0
#define	  DA_LRCP_INVERTED	1
#define	 DA_FAT0_BCP	__BIT(6)
#define	  DA_BCP_NORMAL		0
#define	  DA_BCP_INVERTED	1
#define	 DA_FAT0_SR	__BITS(5,4)
#define	 DA_FAT0_WSS	__BITS(3,2)
#define	 DA_FAT0_FMT	__BITS(1,0)
#define	  DA_FMT_I2S	0
#define	  DA_FMT_LJ	1
#define	  DA_FMT_RJ	2
#define	DA_FAT1		0x08
#define	DA_ISTA		0x0c
#define	DA_RXFIFO	0x10
#define	DA_FCTL		0x14
#define	 DA_FCTL_HUB_EN	__BIT(31)
#define	 DA_FCTL_FTX	__BIT(25)
#define	 DA_FCTL_FRX	__BIT(24)
#define	DA_FSTA		0x18
#define	DA_INT		0x1c
#define	 DA_INT_TX_DRQ	__BIT(7)
#define	 DA_INT_RX_DRQ	__BIT(3)
#define	DA_TXFIFO	0x20
#define	DA_CLKD		0x24
#define	 DA_CLKD_MCLKO_EN __BIT(7)
#define	 DA_CLKD_BCLKDIV __BITS(6,4)
#define	  DA_CLKD_BCLKDIV_16	5
#define	 DA_CLKD_MCLKDIV __BITS(3,0)
#define	  DA_CLKD_MCLKDIV_1	0
#define	DA_TXCNT	0x28
#define	DA_RXCNT	0x2c

#define	DA_CHSEL_EN	__BITS(11,4)
#define	DA_CHSEL_SEL	__BITS(2,0)

struct sunxi_i2s_config {
	const char	*name;
	bus_size_t	txchsel;
	bus_size_t	txchmap;
	bus_size_t	rxchsel;
	bus_size_t	rxchmap;
};

static const struct sunxi_i2s_config sun50i_a64_codec_config = {
	.name = "Audio Codec (digital part)",
	.txchsel = 0x30,
	.txchmap = 0x34,
	.rxchsel = 0x38,
	.rxchmap = 0x3c,
};

static const struct of_compat_data compat_data[] = {
	{ "allwinner,sun50i-a64-acodec-i2s",
	  (uintptr_t)&sun50i_a64_codec_config },

	{ NULL }
};

struct sunxi_i2s_softc;

struct sunxi_i2s_chan {
	struct sunxi_i2s_softc	*ch_sc;
	u_int			ch_mode;

	struct fdtbus_dma	*ch_dma;
	struct fdtbus_dma_req	ch_req;

	audio_params_t		ch_params;

	bus_addr_t		ch_start_phys;
	bus_addr_t		ch_end_phys;
	bus_addr_t		ch_cur_phys;
	int			ch_blksize;

	void			(*ch_intr)(void *);
	void			*ch_intrarg;
};

struct sunxi_i2s_dma {
	LIST_ENTRY(sunxi_i2s_dma) dma_list;
	bus_dmamap_t		dma_map;
	void			*dma_addr;
	size_t			dma_size;
	bus_dma_segment_t	dma_segs[1];
	int			dma_nsegs;
};

struct sunxi_i2s_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	bus_dma_tag_t		sc_dmat;
	int			sc_phandle;
	bus_addr_t		sc_baseaddr;

	struct sunxi_i2s_config	*sc_cfg;

	LIST_HEAD(, sunxi_i2s_dma) sc_dmalist;

	kmutex_t		sc_lock;
	kmutex_t		sc_intr_lock;

	struct audio_format	sc_format;
	struct audio_encoding_set *sc_encodings;

	struct sunxi_i2s_chan	sc_pchan;
	struct sunxi_i2s_chan	sc_rchan;

	struct audio_dai_device	sc_dai;
};

#define	I2S_READ(sc, reg)			\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	I2S_WRITE(sc, reg, val)		\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static int
sunxi_i2s_allocdma(struct sunxi_i2s_softc *sc, size_t size,
    size_t align, struct sunxi_i2s_dma *dma)
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
sunxi_i2s_freedma(struct sunxi_i2s_softc *sc, struct sunxi_i2s_dma *dma)
{
	bus_dmamap_unload(sc->sc_dmat, dma->dma_map);
	bus_dmamap_destroy(sc->sc_dmat, dma->dma_map);
	bus_dmamem_unmap(sc->sc_dmat, dma->dma_addr, dma->dma_size);
	bus_dmamem_free(sc->sc_dmat, dma->dma_segs, dma->dma_nsegs);
}

static int
sunxi_i2s_transfer(struct sunxi_i2s_chan *ch)
{
	bus_dma_segment_t seg;

	seg.ds_addr = ch->ch_cur_phys;
	seg.ds_len = ch->ch_blksize;
	ch->ch_req.dreq_segs = &seg;
	ch->ch_req.dreq_nsegs = 1;

	return fdtbus_dma_transfer(ch->ch_dma, &ch->ch_req);
}

static int
sunxi_i2s_open(void *priv, int flags)
{
	return 0;
}

static void
sunxi_i2s_close(void *priv)
{
}

static int
sunxi_i2s_query_encoding(void *priv, struct audio_encoding *ae)
{
	struct sunxi_i2s_softc * const sc = priv;

	return auconv_query_encoding(sc->sc_encodings, ae);
}

static int
sunxi_i2s_set_params(void *priv, int setmode, int usemode,
    audio_params_t *play, audio_params_t *rec,
    stream_filter_list_t *pfil, stream_filter_list_t *rfil)
{
	struct sunxi_i2s_softc * const sc = priv;
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

static void *
sunxi_i2s_allocm(void *priv, int dir, size_t size)
{
	struct sunxi_i2s_softc * const sc = priv;
	struct sunxi_i2s_dma *dma;
	int error;

	dma = kmem_alloc(sizeof(*dma), KM_SLEEP);

	error = sunxi_i2s_allocdma(sc, size, 16, dma);
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
sunxi_i2s_freem(void *priv, void *addr, size_t size)
{
	struct sunxi_i2s_softc * const sc = priv;
	struct sunxi_i2s_dma *dma;

	LIST_FOREACH(dma, &sc->sc_dmalist, dma_list)
		if (dma->dma_addr == addr) {
			sunxi_i2s_freedma(sc, dma);
			LIST_REMOVE(dma, dma_list);
			kmem_free(dma, sizeof(*dma));
			break;
		}
}

static paddr_t
sunxi_i2s_mappage(void *priv, void *addr, off_t off, int prot)
{
	struct sunxi_i2s_softc * const sc = priv;
	struct sunxi_i2s_dma *dma;

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
sunxi_i2s_get_props(void *priv)
{
	return AUDIO_PROP_PLAYBACK|AUDIO_PROP_CAPTURE|
	    AUDIO_PROP_MMAP|AUDIO_PROP_FULLDUPLEX|AUDIO_PROP_INDEPENDENT;
}

static int
sunxi_i2s_round_blocksize(void *priv, int bs, int mode,
    const audio_params_t *params)
{
	bs &= ~3;
	if (bs == 0)
		bs = 4;
	return bs;
}

static size_t
sunxi_i2s_round_buffersize(void *priv, int dir, size_t bufsize)
{
	return bufsize;
}

static int
sunxi_i2s_trigger_output(void *priv, void *start, void *end, int blksize,
    void (*intr)(void *), void *intrarg, const audio_params_t *params)
{
	struct sunxi_i2s_softc * const sc = priv;
	struct sunxi_i2s_chan *ch = &sc->sc_pchan;
	struct sunxi_i2s_dma *dma;
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

	/* Flush FIFO */
	val = I2S_READ(sc, DA_FCTL);
	I2S_WRITE(sc, DA_FCTL, val | DA_FCTL_FTX);
	I2S_WRITE(sc, DA_FCTL, val & ~DA_FCTL_FTX);

	/* Reset TX sample counter */
	I2S_WRITE(sc, DA_TXCNT, 0);

	/* Enable transmitter block */
	val = I2S_READ(sc, DA_CTL);
	I2S_WRITE(sc, DA_CTL, val | DA_CTL_TXEN);

	/* Enable TX DRQ */
	val = I2S_READ(sc, DA_INT);
	I2S_WRITE(sc, DA_INT, val | DA_INT_TX_DRQ);

	/* Start DMA transfer */
	error = sunxi_i2s_transfer(ch);
	if (error != 0) {
		aprint_error_dev(sc->sc_dev,
		    "failed to start DMA transfer: %d\n", error);
		return error;
	}

	return 0;
}

static int
sunxi_i2s_trigger_input(void *priv, void *start, void *end, int blksize,
    void (*intr)(void *), void *intrarg, const audio_params_t *params)
{
	struct sunxi_i2s_softc * const sc = priv;
	struct sunxi_i2s_chan *ch = &sc->sc_rchan;
	struct sunxi_i2s_dma *dma;
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

	/* Flush FIFO */
	val = I2S_READ(sc, DA_FCTL);
	I2S_WRITE(sc, DA_FCTL, val | DA_FCTL_FRX);
	I2S_WRITE(sc, DA_FCTL, val & ~DA_FCTL_FRX);

	/* Reset RX sample counter */
	I2S_WRITE(sc, DA_RXCNT, 0);

	/* Enable receiver block */
	val = I2S_READ(sc, DA_CTL);
	I2S_WRITE(sc, DA_CTL, val | DA_CTL_RXEN);

	/* Enable RX DRQ */
	val = I2S_READ(sc, DA_INT);
	I2S_WRITE(sc, DA_INT, val | DA_INT_RX_DRQ);

	/* Start DMA transfer */
	error = sunxi_i2s_transfer(ch);
	if (error != 0) {
		aprint_error_dev(sc->sc_dev,
		    "failed to start DMA transfer: %d\n", error);
		return error;
	}

	return 0;
}

static int
sunxi_i2s_halt_output(void *priv)
{
	struct sunxi_i2s_softc * const sc = priv;
	struct sunxi_i2s_chan *ch = &sc->sc_pchan;
	uint32_t val;

	/* Disable DMA channel */
	fdtbus_dma_halt(ch->ch_dma);

	/* Disable transmitter block */
	val = I2S_READ(sc, DA_CTL);
	I2S_WRITE(sc, DA_CTL, val & ~DA_CTL_TXEN);

	/* Disable TX DRQ */
	val = I2S_READ(sc, DA_INT);
	I2S_WRITE(sc, DA_INT, val & ~DA_INT_TX_DRQ);

	ch->ch_intr = NULL;
	ch->ch_intrarg = NULL;

	return 0;
}

static int
sunxi_i2s_halt_input(void *priv)
{
	struct sunxi_i2s_softc * const sc = priv;
	struct sunxi_i2s_chan *ch = &sc->sc_rchan;
	uint32_t val;

	/* Disable DMA channel */
	fdtbus_dma_halt(ch->ch_dma);

	/* Disable receiver block */
	val = I2S_READ(sc, DA_CTL);
	I2S_WRITE(sc, DA_CTL, val & ~DA_CTL_RXEN);

	/* Disable RX DRQ */
	val = I2S_READ(sc, DA_INT);
	I2S_WRITE(sc, DA_INT, val & ~DA_INT_RX_DRQ);

	return 0;
}

static void
sunxi_i2s_get_locks(void *priv, kmutex_t **intr, kmutex_t **thread)
{
	struct sunxi_i2s_softc * const sc = priv;

	*intr = &sc->sc_intr_lock;
	*thread = &sc->sc_lock;
}

static const struct audio_hw_if sunxi_i2s_hw_if = {
	.open = sunxi_i2s_open,
	.close = sunxi_i2s_close,
	.drain = NULL,
	.query_encoding = sunxi_i2s_query_encoding,
	.set_params = sunxi_i2s_set_params,
	.allocm = sunxi_i2s_allocm,
	.freem = sunxi_i2s_freem,
	.mappage = sunxi_i2s_mappage,
	.get_props = sunxi_i2s_get_props,
	.round_blocksize = sunxi_i2s_round_blocksize,
	.round_buffersize = sunxi_i2s_round_buffersize,
	.trigger_output = sunxi_i2s_trigger_output,
	.trigger_input = sunxi_i2s_trigger_input,
	.halt_output = sunxi_i2s_halt_output,
	.halt_input = sunxi_i2s_halt_input,
	.get_locks = sunxi_i2s_get_locks,
};

static void
sunxi_i2s_dmaintr(void *priv)
{
	struct sunxi_i2s_chan * const ch = priv;
	struct sunxi_i2s_softc * const sc = ch->ch_sc;

	mutex_enter(&sc->sc_intr_lock);
	ch->ch_cur_phys += ch->ch_blksize;
	if (ch->ch_cur_phys >= ch->ch_end_phys)
		ch->ch_cur_phys = ch->ch_start_phys;

	if (ch->ch_intr) {
		ch->ch_intr(ch->ch_intrarg);
		sunxi_i2s_transfer(ch);
	}
	mutex_exit(&sc->sc_intr_lock);
}

static int
sunxi_i2s_chan_init(struct sunxi_i2s_softc *sc,
    struct sunxi_i2s_chan *ch, u_int mode, const char *dmaname)
{
	ch->ch_sc = sc;
	ch->ch_mode = mode;
	ch->ch_dma = fdtbus_dma_get(sc->sc_phandle, dmaname, sunxi_i2s_dmaintr, ch);
	if (ch->ch_dma == NULL) {
		aprint_error(": couldn't get dma channel \"%s\"\n", dmaname);
		return ENXIO;
	}

	if (mode == AUMODE_PLAY) {
		ch->ch_req.dreq_dir = FDT_DMA_WRITE;
		ch->ch_req.dreq_dev_phys =
		    sc->sc_baseaddr + DA_TXFIFO;
	} else {
		ch->ch_req.dreq_dir = FDT_DMA_READ;
		ch->ch_req.dreq_dev_phys =
		    sc->sc_baseaddr + DA_RXFIFO;
	}
	ch->ch_req.dreq_mem_opt.opt_bus_width = 32;
	ch->ch_req.dreq_mem_opt.opt_burst_len = 8;
	ch->ch_req.dreq_dev_opt.opt_bus_width = 32;
	ch->ch_req.dreq_dev_opt.opt_burst_len = 8;

	return 0;
}

static int
sunxi_i2s_dai_set_sysclk(audio_dai_tag_t dai, u_int rate, int dir)
{
	struct sunxi_i2s_softc * const sc = audio_dai_private(dai);
	uint32_t val;

	/* XXX */

	val = DA_CLKD_MCLKO_EN;
	val |= __SHIFTIN(DA_CLKD_BCLKDIV_16, DA_CLKD_BCLKDIV);
	val |= __SHIFTIN(DA_CLKD_MCLKDIV_1, DA_CLKD_MCLKDIV);

	I2S_WRITE(sc, DA_CLKD, val);

	return 0;
}

static int
sunxi_i2s_dai_set_format(audio_dai_tag_t dai, u_int format)
{
	struct sunxi_i2s_softc * const sc = audio_dai_private(dai);
	uint32_t ctl, fat0;

	const u_int fmt = __SHIFTOUT(format, AUDIO_DAI_FORMAT_MASK);
	const u_int pol = __SHIFTOUT(format, AUDIO_DAI_POLARITY_MASK);
	const u_int clk = __SHIFTOUT(format, AUDIO_DAI_CLOCK_MASK);

	ctl = I2S_READ(sc, DA_CTL);
	fat0 = I2S_READ(sc, DA_FAT0);

	fat0 &= ~DA_FAT0_FMT;
	switch (fmt) {
	case AUDIO_DAI_FORMAT_I2S:
		fat0 |= __SHIFTIN(DA_FMT_I2S, DA_FAT0_FMT);
		break;
	case AUDIO_DAI_FORMAT_RJ:
		fat0 |= __SHIFTIN(DA_FMT_RJ, DA_FAT0_FMT);
		break;
	case AUDIO_DAI_FORMAT_LJ:
		fat0 |= __SHIFTIN(DA_FMT_LJ, DA_FAT0_FMT);
		break;
	default:
		return EINVAL;
	}

	fat0 &= ~(DA_FAT0_LRCP|DA_FAT0_BCP);
	if (AUDIO_DAI_POLARITY_B(pol))
		fat0 |= __SHIFTIN(DA_BCP_INVERTED, DA_FAT0_BCP);
	if (AUDIO_DAI_POLARITY_F(pol))
		fat0 |= __SHIFTIN(DA_LRCP_INVERTED, DA_FAT0_LRCP);

	switch (clk) {
	case AUDIO_DAI_CLOCK_CBM_CFM:
		ctl |= DA_CTL_MS;	/* codec is master */
		break;
	case AUDIO_DAI_CLOCK_CBS_CFS:
		ctl &= ~DA_CTL_MS;	/* codec is slave */
		break;
	default:
		return EINVAL;
	}

	ctl &= ~DA_CTL_PCM;

	I2S_WRITE(sc, DA_CTL, ctl);
	I2S_WRITE(sc, DA_FAT0, fat0);

	return 0;
}

static audio_dai_tag_t
sunxi_i2s_dai_get_tag(device_t dev, const void *data, size_t len)
{
	struct sunxi_i2s_softc * const sc = device_private(dev);

	if (len != 4)
		return NULL;

	return &sc->sc_dai;
}

static struct fdtbus_dai_controller_func sunxi_i2s_dai_funcs = {
	.get_tag = sunxi_i2s_dai_get_tag
};

static int
sunxi_i2s_clock_init(int phandle)
{
	struct fdtbus_reset *rst;
	struct clk *clk;
	int error;

	/* Set module clock to 24.576MHz, suitable for 48 kHz sampling rates */
	clk = fdtbus_clock_get(phandle, "mod");
	if (clk == NULL) {
		aprint_error(": couldn't find mod clock\n");
		return ENXIO;
	}
	error = clk_set_rate(clk, SUNXI_I2S_CLK_RATE);
	if (error != 0) {
		aprint_error(": couldn't set mod clock rate: %d\n", error);
		return error;
	}
	error = clk_enable(clk);
	if (error != 0) {
		aprint_error(": couldn't enable mod clock: %d\n", error);
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
	rst = fdtbus_reset_get(phandle, "rst");
	if (rst == NULL) {
		aprint_error(": couldn't find reset\n");
		return ENXIO;
	}
	error = fdtbus_reset_deassert(rst);
	if (error != 0) {
		aprint_error(": couldn't de-assert reset: %d\n", error);
		return error;
	}

	return 0;
}

static int
sunxi_i2s_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compat_data(faa->faa_phandle, compat_data);
}

static void
sunxi_i2s_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_i2s_softc * const sc = device_private(self);
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

	if (sunxi_i2s_clock_init(phandle) != 0)
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

	if (sunxi_i2s_chan_init(sc, &sc->sc_pchan, AUMODE_PLAY, "tx") != 0 ||
	    sunxi_i2s_chan_init(sc, &sc->sc_rchan, AUMODE_RECORD, "rx") != 0) {
		aprint_error(": couldn't setup channels\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": %s\n", sc->sc_cfg->name);

	/* Reset */
	val = I2S_READ(sc, DA_CTL);
	val &= ~(DA_CTL_TXEN|DA_CTL_RXEN|DA_CTL_GEN);
	I2S_WRITE(sc, DA_CTL, val);

	val = I2S_READ(sc, DA_FCTL);
	val &= ~(DA_FCTL_FTX|DA_FCTL_FRX);
	I2S_WRITE(sc, DA_FCTL, val);

	I2S_WRITE(sc, DA_TXCNT, 0);
	I2S_WRITE(sc, DA_RXCNT, 0);

	/* Enable */
	I2S_WRITE(sc, DA_CTL, DA_CTL_GEN | DA_CTL_SDO_EN);

	/* Setup channels */
	I2S_WRITE(sc, sc->sc_cfg->txchmap, 0x76543210);
	I2S_WRITE(sc, sc->sc_cfg->txchsel, __SHIFTIN(1, DA_CHSEL_SEL) |
					   __SHIFTIN(3, DA_CHSEL_EN));
	I2S_WRITE(sc, sc->sc_cfg->rxchmap, 0x76543210);
	I2S_WRITE(sc, sc->sc_cfg->rxchsel, __SHIFTIN(1, DA_CHSEL_SEL) |
					   __SHIFTIN(3, DA_CHSEL_EN));

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

	sc->sc_dai.dai_set_sysclk = sunxi_i2s_dai_set_sysclk;
	sc->sc_dai.dai_set_format = sunxi_i2s_dai_set_format;
	sc->sc_dai.dai_hw_if = &sunxi_i2s_hw_if;
	sc->sc_dai.dai_dev = self;
	sc->sc_dai.dai_priv = sc;
	fdtbus_register_dai_controller(self, phandle, &sunxi_i2s_dai_funcs);
}

CFATTACH_DECL_NEW(sunxi_i2s, sizeof(struct sunxi_i2s_softc),
    sunxi_i2s_match, sunxi_i2s_attach, NULL, NULL);
