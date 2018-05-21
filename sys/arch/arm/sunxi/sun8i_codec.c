/* $NetBSD: sun8i_codec.c,v 1.5.2.2 2018/05/21 04:35:59 pgoyette Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: sun8i_codec.c,v 1.5.2.2 2018/05/21 04:35:59 pgoyette Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/bitops.h>
#include <sys/gpio.h>
#include <sys/workqueue.h>

#include <dev/audio_dai.h>

#include <dev/fdt/fdtvar.h>

#define	SYSCLK_CTL		0x00c
#define	 AIF1CLK_ENA		__BIT(11)
#define	 AIF1CLK_SRC		__BITS(9,8)
#define	  AIF1CLK_SRC_PLL	2
#define	 SYSCLK_ENA		__BIT(3)
#define	 SYSCLK_SRC		__BIT(0)

#define	MOD_CLK_ENA		0x010
#define	MOD_RST_CTL		0x014
#define	 MOD_AIF1		__BIT(15)
#define	 MOD_ADC		__BIT(3)
#define	 MOD_DAC		__BIT(2)

#define	SYS_SR_CTRL		0x018
#define	 AIF1_FS		__BITS(15,12)
#define	  AIF_FS_48KHZ		8

#define	AIF1CLK_CTRL		0x040
#define	 AIF1_MSTR_MOD		__BIT(15)
#define	 AIF1_BCLK_INV		__BIT(14)
#define	 AIF1_LRCK_INV		__BIT(13)
#define	 AIF1_BCLK_DIV		__BITS(12,9)
#define	  AIF1_BCLK_DIV_16	6
#define	 AIF1_LRCK_DIV		__BITS(8,6)
#define	  AIF1_LRCK_DIV_16	0
#define	  AIF1_LRCK_DIV_64	2
#define	 AIF1_WORD_SIZ		__BITS(5,4)
#define	  AIF1_WORD_SIZ_16	1
#define	 AIF1_DATA_FMT		__BITS(3,2)
#define	  AIF1_DATA_FMT_I2S	0
#define	  AIF1_DATA_FMT_LJ	1
#define	  AIF1_DATA_FMT_RJ	2
#define	  AIF1_DATA_FMT_DSP	3

#define	AIF1_DACDAT_CTRL	0x048
#define	 AIF1_DAC0L_ENA		__BIT(15)
#define	 AIF1_DAC0R_ENA		__BIT(14)

#define	ADC_DIG_CTRL		0x100
#define	 ADC_DIG_CTRL_ENAD	__BIT(15)

#define	HMIC_CTRL1		0x110
#define	 HMIC_CTRL1_N		__BITS(11,8)
#define	 HMIC_CTRL1_JACK_IN_IRQ_EN __BIT(4)
#define	 HMIC_CTRL1_JACK_OUT_IRQ_EN __BIT(3)
#define	 HMIC_CTRL1_MIC_DET_IRQ_EN __BIT(0)

#define	HMIC_CTRL2		0x114
#define	 HMIC_CTRL2_MDATA_THRES	__BITS(12,8)

#define	HMIC_STS		0x118
#define	 HMIC_STS_MIC_PRESENT	__BIT(6)
#define	 HMIC_STS_JACK_DET_OIRQ	__BIT(4)
#define	 HMIC_STS_JACK_DET_IIRQ	__BIT(3)
#define	 HMIC_STS_MIC_DET_ST	__BIT(0)

#define	DAC_DIG_CTRL		0x120
#define	 DAC_DIG_CTRL_ENDA	__BIT(15)

#define	DAC_MXR_SRC		0x130
#define	 DACL_MXR_SRC		__BITS(15,12)
#define	  DACL_MXR_SRC_AIF1_DAC0L 0x8
#define	 DACR_MXR_SRC		__BITS(11,8)
#define	  DACR_MXR_SRC_AIF1_DAC0R 0x8

struct sun8i_codec_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	int			sc_phandle;

	struct workqueue	*sc_workq;
	struct work		sc_work;

	struct audio_dai_device	sc_dai;
	audio_dai_tag_t		sc_codec_analog;
	int			sc_jackdet_pol;

	struct fdtbus_gpio_pin	*sc_pin_pa;

	struct clk		*sc_clk_gate;
	struct clk		*sc_clk_mod;
};

#define	RD4(sc, reg)			\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	WR4(sc, reg, val)		\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static int
sun8i_codec_set_params(void *priv, int setmode, int usemode,
    audio_params_t *play, audio_params_t *rec,
    stream_filter_list_t *pfil, stream_filter_list_t *rfil)
{
	if (play && (setmode & AUMODE_PLAY))
		if (play->sample_rate != 48000)
			return EINVAL;

	if (rec && (setmode & AUMODE_RECORD))
		if (rec->sample_rate != 48000)
			return EINVAL;

	return 0;
}

static int
sun8i_codec_set_port(void *priv, mixer_ctrl_t *mc)
{
	struct sun8i_codec_softc * const sc = priv;

	if (!sc->sc_codec_analog)
		return ENXIO;

	return audio_dai_set_port(sc->sc_codec_analog, mc);
}

static int
sun8i_codec_get_port(void *priv, mixer_ctrl_t *mc)
{
	struct sun8i_codec_softc * const sc = priv;

	if (!sc->sc_codec_analog)
		return ENXIO;

	return audio_dai_get_port(sc->sc_codec_analog, mc);
}

static int
sun8i_codec_query_devinfo(void *priv, mixer_devinfo_t *di)
{
	struct sun8i_codec_softc * const sc = priv;

	if (!sc->sc_codec_analog)
		return ENXIO;

	return audio_dai_query_devinfo(sc->sc_codec_analog, di);
}

static const struct audio_hw_if sun8i_codec_hw_if = {
	.set_params = sun8i_codec_set_params,
	.set_port = sun8i_codec_set_port,
	.get_port = sun8i_codec_get_port,
	.query_devinfo = sun8i_codec_query_devinfo,
};

static audio_dai_tag_t
sun8i_codec_dai_get_tag(device_t dev, const void *data, size_t len)
{
	struct sun8i_codec_softc * const sc = device_private(dev);

	if (len != 4)
		return NULL;

	return &sc->sc_dai;
}

static struct fdtbus_dai_controller_func sun8i_codec_dai_funcs = {
	.get_tag = sun8i_codec_dai_get_tag
};

static int
sun8i_codec_dai_set_format(audio_dai_tag_t dai, u_int format)
{
	struct sun8i_codec_softc * const sc = audio_dai_private(dai);
	uint32_t val;

        const u_int fmt = __SHIFTOUT(format, AUDIO_DAI_FORMAT_MASK);
        const u_int pol = __SHIFTOUT(format, AUDIO_DAI_POLARITY_MASK);
        const u_int clk = __SHIFTOUT(format, AUDIO_DAI_CLOCK_MASK);

	val = RD4(sc, AIF1CLK_CTRL);

	val &= ~AIF1_DATA_FMT;
	switch (fmt) {
	case AUDIO_DAI_FORMAT_I2S:
		val |= __SHIFTIN(AIF1_DATA_FMT_I2S, AIF1_DATA_FMT);
		break;
	case AUDIO_DAI_FORMAT_RJ:
		val |= __SHIFTIN(AIF1_DATA_FMT_RJ, AIF1_DATA_FMT);
		break;
	case AUDIO_DAI_FORMAT_LJ:
		val |= __SHIFTIN(AIF1_DATA_FMT_LJ, AIF1_DATA_FMT);
		break;
	case AUDIO_DAI_FORMAT_DSPA:
	case AUDIO_DAI_FORMAT_DSPB:
		val |= __SHIFTIN(AIF1_DATA_FMT_DSP, AIF1_DATA_FMT);
		break;
	default:
		return EINVAL;
	}

	val &= ~(AIF1_BCLK_INV|AIF1_LRCK_INV);
	/* Codec LRCK polarity is inverted (datasheet is wrong) */
	if (!AUDIO_DAI_POLARITY_F(pol))
		val |= AIF1_LRCK_INV;
	if (AUDIO_DAI_POLARITY_B(pol))
		val |= AIF1_BCLK_INV;

	switch (clk) {
	case AUDIO_DAI_CLOCK_CBM_CFM:
		val &= ~AIF1_MSTR_MOD;	/* codec is master */
		break;
	case AUDIO_DAI_CLOCK_CBS_CFS:
		val |= AIF1_MSTR_MOD;	/* codec is slave */
		break;
	default:
		return EINVAL;
	}

	val &= ~AIF1_LRCK_DIV;
	val |= __SHIFTIN(AIF1_LRCK_DIV_64, AIF1_LRCK_DIV);

	val &= ~AIF1_BCLK_DIV;
	val |= __SHIFTIN(AIF1_BCLK_DIV_16, AIF1_BCLK_DIV);

	WR4(sc, AIF1CLK_CTRL, val);

	return 0;
}

static int
sun8i_codec_dai_add_device(audio_dai_tag_t dai, audio_dai_tag_t aux)
{
	struct sun8i_codec_softc * const sc = audio_dai_private(dai);

	if (sc->sc_codec_analog != NULL)
		return 0;

	sc->sc_codec_analog = aux;

	return 0;
}

static void
sun8i_codec_set_jackdet(struct sun8i_codec_softc *sc, bool enable)
{
	const uint32_t mask =
	    HMIC_CTRL1_JACK_IN_IRQ_EN |
	    HMIC_CTRL1_JACK_OUT_IRQ_EN |
	    HMIC_CTRL1_MIC_DET_IRQ_EN;
	uint32_t val;

	val = RD4(sc, HMIC_CTRL1);
	if (enable)
		val |= mask;
	else
		val &= ~mask;
	WR4(sc, HMIC_CTRL1, val);
}

static int
sun8i_codec_intr(void *priv)
{
	const uint32_t mask =
	    HMIC_STS_JACK_DET_OIRQ |
	    HMIC_STS_JACK_DET_IIRQ |
	    HMIC_STS_MIC_DET_ST;
	struct sun8i_codec_softc * const sc = priv;
	uint32_t val;

	val = RD4(sc, HMIC_STS);
	if (val & mask) {
		/* Disable jack detect IRQ until work is complete */
		sun8i_codec_set_jackdet(sc, false);

		/* Schedule pending jack detect task */
		workqueue_enqueue(sc->sc_workq, &sc->sc_work, NULL);
	}

	return 1;
}


static void
sun8i_codec_thread(struct work *wk, void *priv)
{
	struct sun8i_codec_softc * const sc = priv;
	int hpdet = -1, micdet = -1;
	uint32_t val;

	val = RD4(sc, HMIC_STS);

	if (sc->sc_codec_analog) {
		if (val & HMIC_STS_JACK_DET_OIRQ)
			hpdet = 0 ^ sc->sc_jackdet_pol;
		else if (val & HMIC_STS_JACK_DET_IIRQ)
			hpdet = 1 ^ sc->sc_jackdet_pol;

		if (val & HMIC_STS_MIC_DET_ST)
			micdet = !!(val & HMIC_STS_MIC_PRESENT);

		if (hpdet != -1) {
			audio_dai_jack_detect(sc->sc_codec_analog,
			    AUDIO_DAI_JACK_HP, hpdet);
		}
		if (micdet != -1) {
			audio_dai_jack_detect(sc->sc_codec_analog,
			    AUDIO_DAI_JACK_MIC, micdet);
		}
	}

	WR4(sc, HMIC_STS, val);

	/* Re-enable jack detect IRQ */
	sun8i_codec_set_jackdet(sc, true);
}

static const char * compatible[] = {
	"allwinner,sun50i-a64-codec",
	NULL
};

static int
sun8i_codec_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
sun8i_codec_attach(device_t parent, device_t self, void *aux)
{
	struct sun8i_codec_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	char intrstr[128];
	bus_addr_t addr;
	bus_size_t size;
	uint32_t val;
	void *ih;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": couldn't decode interrupt\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}
	sc->sc_jackdet_pol = 1;

	sc->sc_clk_gate = fdtbus_clock_get(phandle, "bus");
	sc->sc_clk_mod = fdtbus_clock_get(phandle, "mod");
	if (!sc->sc_clk_gate || !sc->sc_clk_mod) {
		aprint_error(": couldn't get clocks\n");
		return;
	}
	if (clk_enable(sc->sc_clk_gate) != 0) {
		aprint_error(": couldn't enable bus clock\n");
		return;
	}

	sc->sc_phandle = phandle;

	aprint_naive("\n");
	aprint_normal(": Audio Codec\n");

	if (workqueue_create(&sc->sc_workq, "jackdet", sun8i_codec_thread,
	    sc, PRI_NONE, IPL_VM, 0) != 0) {
		aprint_error_dev(self, "couldn't create jackdet workqueue\n");
		return;
	}

	/* Enable clocks */
	val = RD4(sc, SYSCLK_CTL);
	val |= AIF1CLK_ENA;
	val &= ~AIF1CLK_SRC;
	val |= __SHIFTIN(AIF1CLK_SRC_PLL, AIF1CLK_SRC);
	val |= SYSCLK_ENA;
	val &= ~SYSCLK_SRC;
	WR4(sc, SYSCLK_CTL, val);
	WR4(sc, MOD_CLK_ENA, MOD_AIF1 | MOD_ADC | MOD_DAC);
	WR4(sc, MOD_RST_CTL, MOD_AIF1 | MOD_ADC | MOD_DAC);

	/* Enable digital parts */
	WR4(sc, DAC_DIG_CTRL, DAC_DIG_CTRL_ENDA);
	WR4(sc, ADC_DIG_CTRL, ADC_DIG_CTRL_ENAD);

	/* Set AIF1 to 48 kHz */
	val = RD4(sc, SYS_SR_CTRL);
	val &= ~AIF1_FS;
	val |= __SHIFTIN(AIF_FS_48KHZ, AIF1_FS);
	WR4(sc, SYS_SR_CTRL, val);

	/* Set AIF1 to 16-bit */
	val = RD4(sc, AIF1CLK_CTRL);
	val &= ~AIF1_WORD_SIZ;
	val |= __SHIFTIN(AIF1_WORD_SIZ_16, AIF1_WORD_SIZ);
	WR4(sc, AIF1CLK_CTRL, val);

	/* Enable AIF1 DAC timelot 0 */
	val = RD4(sc, AIF1_DACDAT_CTRL);
	val |= AIF1_DAC0L_ENA;
	val |= AIF1_DAC0R_ENA;
	WR4(sc, AIF1_DACDAT_CTRL, val);

	/* DAC mixer source select */
	val = RD4(sc, DAC_MXR_SRC);
	val &= ~DACL_MXR_SRC;
	val |= __SHIFTIN(DACL_MXR_SRC_AIF1_DAC0L, DACL_MXR_SRC);
	val &= ~DACR_MXR_SRC;
	val |= __SHIFTIN(DACR_MXR_SRC_AIF1_DAC0R, DACR_MXR_SRC);
	WR4(sc, DAC_MXR_SRC, val);

	/* Enable PA power */
	sc->sc_pin_pa = fdtbus_gpio_acquire(phandle, "allwinner,pa-gpios", GPIO_PIN_OUTPUT);
	if (sc->sc_pin_pa)
		fdtbus_gpio_write(sc->sc_pin_pa, 1);

	/* Enable jack detect */
	val = RD4(sc, HMIC_CTRL1);
	val |= __SHIFTIN(0xff, HMIC_CTRL1_N);
	WR4(sc, HMIC_CTRL1, val);

	val = RD4(sc, HMIC_CTRL2);
	val &= ~HMIC_CTRL2_MDATA_THRES;
	val |= __SHIFTIN(0x17, HMIC_CTRL2_MDATA_THRES);
	WR4(sc, HMIC_CTRL2, val);

	/* Schedule initial jack detect task */
	workqueue_enqueue(sc->sc_workq, &sc->sc_work, NULL);

	ih = fdtbus_intr_establish(phandle, 0, IPL_VM, FDT_INTR_MPSAFE,
	    sun8i_codec_intr, sc);
	if (ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt on %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	sc->sc_dai.dai_set_format = sun8i_codec_dai_set_format;
	sc->sc_dai.dai_add_device = sun8i_codec_dai_add_device;
	sc->sc_dai.dai_hw_if = &sun8i_codec_hw_if;
	sc->sc_dai.dai_dev = self;
	sc->sc_dai.dai_priv = sc;
	fdtbus_register_dai_controller(self, phandle, &sun8i_codec_dai_funcs);
}

CFATTACH_DECL_NEW(sun8i_codec, sizeof(struct sun8i_codec_softc),
    sun8i_codec_match, sun8i_codec_attach, NULL, NULL);
