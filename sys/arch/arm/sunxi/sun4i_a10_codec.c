/* $NetBSD: sun4i_a10_codec.c,v 1.2.2.2 2017/12/03 11:35:56 jdolecek Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sun4i_a10_codec.c,v 1.2.2.2 2017/12/03 11:35:56 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/bitops.h>

#include <sys/audioio.h>
#include <dev/audio_if.h>

#include <arm/sunxi/sunxi_codec.h>

#define	A10_DEFAULT_PAVOL	0x20

#define	A10_DAC_ACTRL		0x10
#define	 A10_DACAREN		__BIT(31)
#define	 A10_DACALEN		__BIT(30)
#define	 A10_MIXEN		__BIT(29)
#define	 A10_LNG		__BIT(26)
#define	 A10_FMG		__BITS(25,23)
#define	 A10_MICG		__BITS(22,20)
#define	 A10_LLNS		__BIT(19)
#define	 A10_RLNS		__BIT(18)
#define	 A10_LFMS		__BIT(17)
#define	 A10_RFMS		__BIT(16)
#define	 A10_LDACLMIXS		__BIT(15)
#define	 A10_RDACRMIXS		__BIT(14)
#define	 A10_LDACRMIXS		__BIT(13)
#define	 A10_MICLS		__BIT(12)
#define	 A10_MICRS		__BIT(11)
#define	 A10_DACPAS		__BIT(8)
#define	 A10_MIXPAS		__BIT(7)
#define	 A10_PAMUTE		__BIT(6)
#define	 A10_PAVOL		__BITS(5,0)

#define	A10_ADC_ACTRL		0x28
#define	 A10_ADCREN		__BIT(31)
#define	 A10_ADCLEN		__BIT(30)
#define	 A10_PREG1EN		__BIT(29)
#define	 A10_PREG2EN		__BIT(28)
#define	 A10_VMICEN		__BIT(27)
#define	 A10_PREG1		__BITS(26,25)
#define	 A10_PREG2		__BITS(24,23)
#define	 A10_ADCG		__BITS(22,20)
#define	 A10_ADCIS		__BITS(19,17)
#define	 A10_LNRDF		__BIT(16)
#define	 A10_LNPREG		__BITS(15,13)
#define	 A10_MIC1NEN		__BIT(12)
#define	 A10_DITHER		__BIT(8)
#define	 A10_PA_EN		__BIT(4)
#define	 A10_DDE		__BIT(3)
#define	 A10_COMPTEN		__BIT(2)
#define	 A10_PTDBS		__BITS(1,0)

enum a10_codec_mixer_ctrl {
	A10_CODEC_OUTPUT_CLASS,
	A10_CODEC_INPUT_CLASS,

	A10_CODEC_OUTPUT_MASTER_VOLUME,
	A10_CODEC_INPUT_DAC_VOLUME,

	A10_CODEC_MIXER_CTRL_LAST
};

static const struct a10_codec_mixer {
	const char *			name;
	enum a10_codec_mixer_ctrl	mixer_class;
	u_int				reg;
	u_int				mask;
} a10_codec_mixers[A10_CODEC_MIXER_CTRL_LAST] = {
	[A10_CODEC_OUTPUT_MASTER_VOLUME]	= { AudioNmaster,
	    A10_CODEC_OUTPUT_CLASS, A10_DAC_ACTRL, A10_PAVOL },
	[A10_CODEC_INPUT_DAC_VOLUME]		= { AudioNdac,
	    A10_CODEC_INPUT_CLASS, A10_DAC_ACTRL, A10_PAVOL },
};

#define	RD4(sc, reg)			\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	WR4(sc, reg, val)		\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))
#define	SET4(sc, reg, mask)		\
	WR4((sc), (reg), RD4((sc), (reg)) | (mask))
#define	CLR4(sc, reg, mask)		\
	WR4((sc), (reg), RD4((sc), (reg)) & ~(mask))

static int
a10_codec_init(struct sunxi_codec_softc *sc)
{

	/* Unmute PA */
	SET4(sc, A10_DAC_ACTRL, A10_PAMUTE);

	/* Set initial volume */
	CLR4(sc, A10_DAC_ACTRL, A10_PAVOL);
	SET4(sc, A10_DAC_ACTRL, __SHIFTIN(A10_DEFAULT_PAVOL, A10_PAVOL));

	/* Enable DAC analog l/r channels and output mixer */
	SET4(sc, A10_DAC_ACTRL, A10_DACAREN | A10_DACALEN | A10_DACPAS);

	/* Enable ADC analog l/r channels */
	SET4(sc, A10_ADC_ACTRL, A10_ADCREN | A10_ADCLEN);

	/* Enable PA */
	SET4(sc, A10_ADC_ACTRL, A10_PA_EN);

	return 0;
}

static void
a10_codec_mute(struct sunxi_codec_softc *sc, int mute, u_int mode)
{
	if (mode == AUMODE_PLAY) {
		if (sc->sc_pin_pa != NULL)
			fdtbus_gpio_write(sc->sc_pin_pa, !mute);
	}
}

static int
a10_codec_set_port(struct sunxi_codec_softc *sc, mixer_ctrl_t *mc)
{
	const struct a10_codec_mixer *mix;
	u_int val, shift;
	int nvol;

	switch (mc->dev) {
	case A10_CODEC_OUTPUT_MASTER_VOLUME:
	case A10_CODEC_INPUT_DAC_VOLUME:
		mix = &a10_codec_mixers[mc->dev];
		val = RD4(sc, mix->reg);
		shift = 8 - fls32(__SHIFTOUT_MASK(mix->mask));
		nvol = mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT] >> shift;
		val &= ~mix->mask;
		val |= __SHIFTIN(nvol, mix->mask);
		WR4(sc, mix->reg, val);
		return 0;
	}

	return ENXIO;
}

static int
a10_codec_get_port(struct sunxi_codec_softc *sc, mixer_ctrl_t *mc)
{
	const struct a10_codec_mixer *mix;
	u_int val, shift;
	int nvol;

	switch (mc->dev) {
	case A10_CODEC_OUTPUT_MASTER_VOLUME:
	case A10_CODEC_INPUT_DAC_VOLUME:
		mix = &a10_codec_mixers[mc->dev];
		val = RD4(sc, mix->reg);
		shift = 8 - fls32(__SHIFTOUT_MASK(mix->mask));
		nvol = __SHIFTOUT(val, mix->mask) << shift;
		mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT] = nvol;
		mc->un.value.level[AUDIO_MIXER_LEVEL_RIGHT] = nvol;
		return 0;
	}

	return ENXIO;
}

static int
a10_codec_query_devinfo(struct sunxi_codec_softc *sc, mixer_devinfo_t *di)
{
	const struct a10_codec_mixer *mix;

	switch (di->index) {
	case A10_CODEC_OUTPUT_CLASS:
		di->mixer_class = di->index;
		strcpy(di->label.name, AudioCoutputs);
		di->type = AUDIO_MIXER_CLASS;
		di->next = di->prev = AUDIO_MIXER_LAST;
		return 0;

	case A10_CODEC_INPUT_CLASS:
		di->mixer_class = di->index;
		strcpy(di->label.name, AudioCinputs);
		di->type = AUDIO_MIXER_CLASS;
		di->next = di->prev = AUDIO_MIXER_LAST;
		return 0;

	case A10_CODEC_OUTPUT_MASTER_VOLUME:
	case A10_CODEC_INPUT_DAC_VOLUME:
		mix = &a10_codec_mixers[di->index];
		di->mixer_class = mix->mixer_class;
		strcpy(di->label.name, mix->name);
		di->un.v.delta =
		    256 / (__SHIFTOUT_MASK(mix->mask) + 1);
		di->type = AUDIO_MIXER_VALUE;
		di->next = di->prev = AUDIO_MIXER_LAST;
		di->un.v.num_channels = 2;
		strcpy(di->un.v.units.name, AudioNvolume);
		return 0;
	}

	return ENXIO;
}

const struct sunxi_codec_conf sun4i_a10_codecconf = {
	.name = "A10 Audio Codec",

	.init = a10_codec_init,
	.mute = a10_codec_mute,
	.set_port = a10_codec_set_port,
	.get_port = a10_codec_get_port,
	.query_devinfo = a10_codec_query_devinfo,

	.DPC		= 0x00,
	.DAC_FIFOC	= 0x04,
	.DAC_FIFOS	= 0x08,
	.DAC_TXDATA	= 0x0c,
	.ADC_FIFOC	= 0x1c,
	.ADC_FIFOS	= 0x20,
	.ADC_RXDATA	= 0x24,
	.DAC_CNT	= 0x30,
	.ADC_CNT	= 0x34,
};
