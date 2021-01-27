/* $NetBSD: rk_i2s.c,v 1.10 2021/01/27 03:10:19 thorpej Exp $ */

/*-
 * Copyright (c) 2019 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: rk_i2s.c,v 1.10 2021/01/27 03:10:19 thorpej Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/kmem.h>

#include <sys/audioio.h>
#include <dev/audio/audio_if.h>
#include <dev/audio/linear.h>

#include <dev/fdt/fdtvar.h>
#include <dev/fdt/syscon.h>

#define	RK_I2S_FIFO_DEPTH	32
#define	RK_I2S_SAMPLE_RATE	48000

#define	I2S_TXCR		0x00
#define	 TXCR_RCNT			__BITS(22,17)
#define	 TXCR_TCSR			__BITS(16,15)
#define	 TXCR_HWT			__BIT(14)
#define	 TXCR_SJM			__BIT(12)
#define	 TXCR_FBM			__BIT(11)
#define	 TXCR_IBM			__BITS(10,9)
#define	 TXCR_PBM			__BITS(8,7)
#define	 TXCR_TFS			__BIT(5)
#define	 TXCR_VDW			__BITS(4,0)
#define	I2S_RXCR		0x04
#define	 RXCR_RCSR			__BITS(16,15)
#define	 RXCR_HWT			__BIT(14)
#define	 RXCR_SJM			__BIT(12)
#define	 RXCR_FBM			__BIT(11)
#define	 RXCR_IBM			__BITS(10,9)
#define	 RXCR_PBM			__BITS(8,7)
#define	 RXCR_TFS			__BIT(5)
#define	 RXCR_VDW			__BITS(4,0)
#define	I2S_CKR			0x08
#define	 CKR_TRCM			__BITS(29,28)
#define	 CKR_MSS			__BIT(27)
#define	 CKR_CKP			__BIT(26)
#define	 CKR_RLP			__BIT(25)
#define	 CKR_TLP			__BIT(24)
#define	 CKR_MDIV			__BITS(23,16)
#define	 CKR_RSD			__BITS(15,8)
#define	 CKR_TSD			__BITS(7,0)
#define	I2S_TXFIFOLR		0x0c
#define	 TXFIFOLR_TFL(n)		__BITS((n) * 6 + 5, (n) * 6)
#define	I2S_DMACR		0x10
#define	 DMACR_RDE			__BIT(24)
#define	 DMACR_RDL			__BITS(20,16)
#define	 DMACR_TDE			__BIT(8)
#define	 DMACR_TDL			__BITS(4,0)
#define	I2S_INTCR		0x14
#define	 INTCR_RFT			__BITS(24,20)
#define	 INTCR_RXOIC			__BIT(18)
#define	 INTCR_RXOIE			__BIT(17)
#define	 INTCR_RXFIE			__BIT(16)
#define	 INTCR_TFT			__BITS(8,4)
#define	 INTCR_TXUIC			__BIT(2)
#define	 INTCR_TXUIE			__BIT(1)
#define	 INTCR_TXEIE			__BIT(0)
#define	I2S_INTSR		0x18
#define	 INTSR_RXOI			__BIT(17)
#define	 INTSR_RXFI			__BIT(16)
#define	 INTSR_TXUI			__BIT(1)
#define	 INTSR_TXEI			__BIT(0)
#define	I2S_XFER		0x1c
#define	 XFER_RXS			__BIT(1)
#define	 XFER_TXS			__BIT(0)
#define	I2S_CLR			0x20
#define	 CLR_RXC			__BIT(1)
#define	 CLR_TXC			__BIT(0)
#define	I2S_TXDR		0x24
#define	I2S_RXDR		0x28
#define	I2S_RXFIFOLR		0x2c
#define	 RXFIFOLR_RFL(n)		__BITS((n) * 6 + 5, (n) * 6)

struct rk_i2s_config {
	bus_size_t		oe_reg;
	u_int			oe_mask;
	u_int			oe_val;
};

static const struct rk_i2s_config rk3399_i2s_config = {
	.oe_reg = 0x0e220,
	.oe_mask = __BITS(13,11),
	.oe_val = 0x7,
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "rockchip,rk3066-i2s", },
	{ .compat = "rockchip,rk3188-i2s", },
	{ .compat = "rockchip,rk3288-i2s", },
	{ .compat = "rockchip,rk3399-i2s",	.data = &rk3399_i2s_config },
	DEVICE_COMPAT_EOL
};

struct rk_i2s_softc;

struct rk_i2s_chan {
	uint32_t		*ch_start;
	uint32_t		*ch_end;
	uint32_t		*ch_cur;

	int			ch_blksize;
	int			ch_resid;

	void			(*ch_intr)(void *);
	void			*ch_intrarg;
};

struct rk_i2s_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	int			sc_phandle;
	struct clk		*sc_clk;
	struct syscon		*sc_grf;
	const struct rk_i2s_config *sc_conf;

	kmutex_t		sc_lock;
	kmutex_t		sc_intr_lock;

	struct audio_format	sc_format;

	struct rk_i2s_chan	sc_pchan;
	struct rk_i2s_chan	sc_rchan;

	u_int			sc_active;

	struct audio_dai_device	sc_dai;
};

#define	RD4(sc, reg)			\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	WR4(sc, reg, val)		\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static int
rk_i2s_query_format(void *priv, audio_format_query_t *afp)
{
	struct rk_i2s_softc * const sc = priv;

	return audio_query_format(&sc->sc_format, 1, afp);
}

static int
rk_i2s_set_format(void *priv, int setmode,
    const audio_params_t *play, const audio_params_t *rec,
    audio_filter_reg_t *pfil, audio_filter_reg_t *rfil)
{
	struct rk_i2s_softc * const sc = priv;
	uint32_t ckr, txcr, rxcr;

	ckr = RD4(sc, I2S_CKR);
	if ((ckr & CKR_MSS) == 0) {
		const u_int mclk_rate = clk_get_rate(sc->sc_clk);
		const u_int bclk_rate = 2 * 32 * RK_I2S_SAMPLE_RATE;
		const u_int bclk_div = mclk_rate / bclk_rate;
		const u_int lrck_div = bclk_rate / RK_I2S_SAMPLE_RATE;

		ckr &= ~CKR_MDIV;
		ckr |= __SHIFTIN(bclk_div - 1, CKR_MDIV);
		ckr &= ~CKR_TSD;
		ckr |= __SHIFTIN(lrck_div - 1, CKR_TSD);
		ckr &= ~CKR_RSD;
		ckr |= __SHIFTIN(lrck_div - 1, CKR_RSD);
	}

	ckr &= ~CKR_TRCM;
	ckr |= __SHIFTIN(0, CKR_TRCM);
	WR4(sc, I2S_CKR, ckr);

	if (play && (setmode & AUMODE_PLAY) != 0) {
		if (play->channels & 1)
			return EINVAL;
		txcr = RD4(sc, I2S_TXCR);
		txcr &= ~TXCR_VDW;
		txcr |= __SHIFTIN(play->validbits - 1, TXCR_VDW);
		txcr &= ~TXCR_TCSR;
		txcr |= __SHIFTIN(play->channels / 2 - 1, TXCR_TCSR);
		WR4(sc, I2S_TXCR, txcr);
	}

	if (rec && (setmode & AUMODE_RECORD) != 0) {
		if (rec->channels & 1)
			return EINVAL;
		rxcr = RD4(sc, I2S_RXCR);
		rxcr &= ~RXCR_VDW;
		rxcr |= __SHIFTIN(rec->validbits - 1, RXCR_VDW);
		rxcr &= ~RXCR_RCSR;
		rxcr |= __SHIFTIN(rec->channels / 2 - 1, RXCR_RCSR);
		WR4(sc, I2S_RXCR, rxcr);
	}

	return 0;
}

static int
rk_i2s_get_props(void *priv)
{

	return AUDIO_PROP_PLAYBACK | AUDIO_PROP_CAPTURE |
	    AUDIO_PROP_FULLDUPLEX;
}

static void *
rk_i2s_allocm(void *priv, int dir, size_t size)
{
	return kmem_zalloc(size, KM_SLEEP);
}

static void
rk_i2s_freem(void *priv, void *addr, size_t size)
{
	kmem_free(addr, size);
}

static int
rk_i2s_trigger_output(void *priv, void *start, void *end, int blksize,
    void (*intr)(void *), void *intrarg, const audio_params_t *params)
{
	struct rk_i2s_softc * const sc = priv;
	struct rk_i2s_chan *ch = &sc->sc_pchan;
	uint32_t val;

	if (sc->sc_active == 0) {
		val = RD4(sc, I2S_XFER);
		val |= (XFER_TXS | XFER_RXS);
		WR4(sc, I2S_XFER, val);
	}

	sc->sc_active |= XFER_TXS;

	val = RD4(sc, I2S_INTCR);
	val |= INTCR_TXEIE;
	val &= ~INTCR_TFT;
	val |= __SHIFTIN(RK_I2S_FIFO_DEPTH / 2, INTCR_TFT);
	WR4(sc, I2S_INTCR, val);

	ch->ch_intr = intr;
	ch->ch_intrarg = intrarg;
	ch->ch_start = ch->ch_cur = start;
	ch->ch_end = end;
	ch->ch_blksize = blksize;
	ch->ch_resid = blksize;

	return 0;
}

static int
rk_i2s_trigger_input(void *priv, void *start, void *end, int blksize,
    void (*intr)(void *), void *intrarg, const audio_params_t *params)
{
	return EIO;
}

static int
rk_i2s_halt_output(void *priv)
{
	struct rk_i2s_softc * const sc = priv;
	struct rk_i2s_chan *ch = &sc->sc_pchan;
	uint32_t val;

	sc->sc_active &= ~XFER_TXS;
	if (sc->sc_active == 0) {
		val = RD4(sc, I2S_XFER);
		val &= ~(XFER_TXS|XFER_RXS);
		WR4(sc, I2S_XFER, val);
	}

	val = RD4(sc, I2S_INTCR);
	val &= ~INTCR_TXEIE;
	WR4(sc, I2S_INTCR, val);

	val = RD4(sc, I2S_CLR);
	val |= CLR_TXC;
	WR4(sc, I2S_CLR, val);

	while ((RD4(sc, I2S_CLR) & CLR_TXC) != 0)
		delay(1);

	ch->ch_intr = NULL;
	ch->ch_intrarg = NULL;

	return 0;
}

static int
rk_i2s_halt_input(void *priv)
{
	struct rk_i2s_softc * const sc = priv;
	struct rk_i2s_chan *ch = &sc->sc_rchan;
	uint32_t val;

	sc->sc_active &= ~XFER_RXS;
	if (sc->sc_active == 0) {
		val = RD4(sc, I2S_XFER);
		val &= ~(XFER_TXS|XFER_RXS);
		WR4(sc, I2S_XFER, val);
	}

	val = RD4(sc, I2S_INTCR);
	val &= ~INTCR_RXFIE;
	WR4(sc, I2S_INTCR, val);

	ch->ch_intr = NULL;
	ch->ch_intrarg = NULL;

	return 0;
}

static void
rk_i2s_get_locks(void *priv, kmutex_t **intr, kmutex_t **thread)
{
	struct rk_i2s_softc * const sc = priv;

	*intr = &sc->sc_intr_lock;
	*thread = &sc->sc_lock;
}

static const struct audio_hw_if rk_i2s_hw_if = {
	.query_format = rk_i2s_query_format,
	.set_format = rk_i2s_set_format,
	.get_props = rk_i2s_get_props,
	.allocm = rk_i2s_allocm,
	.freem = rk_i2s_freem,
	.trigger_output = rk_i2s_trigger_output,
	.trigger_input = rk_i2s_trigger_input,
	.halt_output = rk_i2s_halt_output,
	.halt_input = rk_i2s_halt_input,
	.get_locks = rk_i2s_get_locks,
};

static int
rk_i2s_intr(void *priv)
{
	struct rk_i2s_softc * const sc = priv;
	struct rk_i2s_chan * const pch = &sc->sc_pchan;
#if notyet
	struct rk_i2s_chan * const rch = &sc->sc_rchan;
#endif
	uint32_t sr, val;
	int fifolr;

	mutex_enter(&sc->sc_intr_lock);

	sr = RD4(sc, I2S_INTSR);

	if ((sr & INTSR_RXFI) != 0) {
#if notyet
		val = RD4(sc, I2S_RXFIFOLR);
		fifolr = __SHIFTOUT(val, RXFIFOLR_RFL(0));
		while (fifolr > 0) {
			*rch->ch_data = RD4(sc, I2S_RXDR);
			rch->ch_data++;
			rch->ch_resid -= 4;
			if (rch->ch_resid == 0)
				rch->ch_intr(rch->ch_intrarg);
			--fifolr;
		}
#endif
	}

	if ((sr & INTSR_TXEI) != 0) {
		val = RD4(sc, I2S_TXFIFOLR);
		fifolr = __SHIFTOUT(val, TXFIFOLR_TFL(0));
		fifolr = uimin(fifolr, RK_I2S_FIFO_DEPTH);
		while (fifolr < RK_I2S_FIFO_DEPTH - 1) {
			WR4(sc, I2S_TXDR, *pch->ch_cur);
			pch->ch_cur++;
			if (pch->ch_cur == pch->ch_end)
				pch->ch_cur = pch->ch_start;
			pch->ch_resid -= 4;
			if (pch->ch_resid == 0) {
				pch->ch_intr(pch->ch_intrarg);
				pch->ch_resid = pch->ch_blksize;
			}
			++fifolr;
		}
	}

	mutex_exit(&sc->sc_intr_lock);

	return 0;
}

static int
rk_i2s_dai_set_sysclk(audio_dai_tag_t dai, u_int rate, int dir)
{
	struct rk_i2s_softc * const sc = audio_dai_private(dai);
	int error;

	error = clk_set_rate(sc->sc_clk, rate);
	if (error != 0) {
		device_printf(sc->sc_dev, "failed to set sysclk to %u Hz: %d\n",
		    rate, error);
		return error;
	}

	return 0;
}

static int
rk_i2s_dai_set_format(audio_dai_tag_t dai, u_int format)
{
	struct rk_i2s_softc * const sc = audio_dai_private(dai);
	uint32_t txcr, rxcr, ckr;

	const u_int fmt = __SHIFTOUT(format, AUDIO_DAI_FORMAT_MASK);
	const u_int pol = __SHIFTOUT(format, AUDIO_DAI_POLARITY_MASK);
	const u_int clk = __SHIFTOUT(format, AUDIO_DAI_CLOCK_MASK);

	txcr = RD4(sc, I2S_TXCR);
	rxcr = RD4(sc, I2S_RXCR);
	ckr = RD4(sc, I2S_CKR);

	txcr &= ~(TXCR_IBM|TXCR_PBM|TXCR_TFS);
	rxcr &= ~(RXCR_IBM|RXCR_PBM|RXCR_TFS);
	switch (fmt) {
	case AUDIO_DAI_FORMAT_I2S:
		txcr |= __SHIFTIN(0, TXCR_IBM);
		rxcr |= __SHIFTIN(0, RXCR_IBM);
		break;
	case AUDIO_DAI_FORMAT_LJ:
		txcr |= __SHIFTIN(1, TXCR_IBM);
		rxcr |= __SHIFTIN(1, RXCR_IBM);
		break;
	case AUDIO_DAI_FORMAT_RJ:
		txcr |= __SHIFTIN(2, TXCR_IBM);
		rxcr |= __SHIFTIN(2, RXCR_IBM);
		break;
	case AUDIO_DAI_FORMAT_DSPA:
		txcr |= __SHIFTIN(0, TXCR_PBM);
		txcr |= TXCR_TFS;
		rxcr |= __SHIFTIN(0, RXCR_PBM);
		txcr |= RXCR_TFS;
		break;
	case AUDIO_DAI_FORMAT_DSPB:
		txcr |= __SHIFTIN(1, TXCR_PBM);
		txcr |= TXCR_TFS;
		rxcr |= __SHIFTIN(1, RXCR_PBM);
		txcr |= RXCR_TFS;
		break;
	default:
		return EINVAL;
	}

	WR4(sc, I2S_TXCR, txcr);
	WR4(sc, I2S_RXCR, rxcr);

	switch (pol) {
	case AUDIO_DAI_POLARITY_IB_NF:
		ckr |= CKR_CKP;
		break;
	case AUDIO_DAI_POLARITY_NB_NF:
		ckr &= ~CKR_CKP;
		break;
	default:
		return EINVAL;
	}

	switch (clk) {
	case AUDIO_DAI_CLOCK_CBM_CFM:
		ckr |= CKR_MSS;		/* sclk input */
		break;
	case AUDIO_DAI_CLOCK_CBS_CFS:
		ckr &= ~CKR_MSS;	/* sclk output */
		break;
	default:
		return EINVAL;
	}

	WR4(sc, I2S_CKR, ckr);

	return 0;
}

static audio_dai_tag_t
rk_i2s_dai_get_tag(device_t dev, const void *data, size_t len)
{
	struct rk_i2s_softc * const sc = device_private(dev);

	if (len != 4)
		return NULL;

	return &sc->sc_dai;
}

static struct fdtbus_dai_controller_func rk_i2s_dai_funcs = {
	.get_tag = rk_i2s_dai_get_tag
};

static int
rk_i2s_clock_init(struct rk_i2s_softc *sc)
{
	const int phandle = sc->sc_phandle;
	int error;

	sc->sc_clk = fdtbus_clock_get(phandle, "i2s_clk");
	if (sc->sc_clk == NULL) {
		aprint_error(": couldn't find i2s_clk clock\n");
		return ENXIO;
	}
	error = clk_enable(sc->sc_clk);
	if (error != 0) {
		aprint_error(": couldn't enable i2s_clk clock: %d\n", error);
		return error;
	}

	/* Enable bus clock */
	error = fdtbus_clock_enable(phandle, "i2s_hclk", true);
	if (error != 0) {
		aprint_error(": couldn't enable i2s_hclk clock: %d\n", error);
		return error;
	}

	return 0;
}

static int
rk_i2s_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
rk_i2s_attach(device_t parent, device_t self, void *aux)
{
	struct rk_i2s_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	char intrstr[128];
	bus_addr_t addr;
	bus_size_t size;
	uint32_t val;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}
	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": couldn't decode interrupt\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_bst = faa->faa_bst;
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_SCHED);

	sc->sc_conf = of_compatible_lookup(phandle, compat_data)->data;
	if (sc->sc_conf != NULL && sc->sc_conf->oe_mask != 0) {
		sc->sc_grf = fdtbus_syscon_acquire(phandle, "rockchip,grf");
		if (sc->sc_grf == NULL) {
			aprint_error(": couldn't find grf\n");
			return;
		}
		syscon_lock(sc->sc_grf);
		val = __SHIFTIN(sc->sc_conf->oe_val, sc->sc_conf->oe_mask);
		val |= (sc->sc_conf->oe_mask << 16);
		syscon_write_4(sc->sc_grf, sc->sc_conf->oe_reg, val);
		syscon_unlock(sc->sc_grf);
	}

	if (rk_i2s_clock_init(sc) != 0)
		return;

	aprint_naive("\n");
	aprint_normal(": I2S/PCM controller\n");

	if (fdtbus_intr_establish_xname(phandle, 0, IPL_AUDIO, FDT_INTR_MPSAFE,
	    rk_i2s_intr, sc, device_xname(self)) == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt on %s\n", intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	sc->sc_format.mode = AUMODE_PLAY|AUMODE_RECORD;
	sc->sc_format.encoding = AUDIO_ENCODING_SLINEAR_LE;
	sc->sc_format.validbits = 16;
	sc->sc_format.precision = 16;
	sc->sc_format.channels = 2;
	sc->sc_format.channel_mask = AUFMT_STEREO;
	sc->sc_format.frequency_type = 1;
	sc->sc_format.frequency[0] = RK_I2S_SAMPLE_RATE;

	sc->sc_dai.dai_set_sysclk = rk_i2s_dai_set_sysclk;
	sc->sc_dai.dai_set_format = rk_i2s_dai_set_format;
	sc->sc_dai.dai_hw_if = &rk_i2s_hw_if;
	sc->sc_dai.dai_dev = self;
	sc->sc_dai.dai_priv = sc;
	fdtbus_register_dai_controller(self, phandle, &rk_i2s_dai_funcs);
}

CFATTACH_DECL_NEW(rk_i2s, sizeof(struct rk_i2s_softc),
    rk_i2s_match, rk_i2s_attach, NULL, NULL);
