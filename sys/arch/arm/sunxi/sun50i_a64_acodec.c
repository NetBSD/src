/* $NetBSD: sun50i_a64_acodec.c,v 1.8.2.2 2018/05/21 04:35:59 pgoyette Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: sun50i_a64_acodec.c,v 1.8.2.2 2018/05/21 04:35:59 pgoyette Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/bitops.h>

#include <dev/audio_dai.h>

#include <dev/fdt/fdtvar.h>

#define	A64_PR_CFG		0x00
#define	 A64_AC_PR_RST		__BIT(28)
#define	 A64_AC_PR_RW		__BIT(24)
#define	 A64_AC_PR_ADDR		__BITS(20,16)
#define	 A64_ACDA_PR_WDAT	__BITS(15,8)
#define	 A64_ACDA_PR_RDAT	__BITS(7,0)

#define	A64_HP_CTRL		0x00
#define	 A64_HPPA_EN		__BIT(6)
#define	 A64_HPVOL		__BITS(5,0)
#define	A64_OL_MIX_CTRL		0x01
#define	 A64_LMIXMUTE_LDAC	__BIT(1)
#define	A64_OR_MIX_CTRL		0x02
#define	 A64_RMIXMUTE_RDAC	__BIT(1)
#define	A64_LINEOUT_CTRL0	0x05
#define	 A64_LINEOUT_LEFT_EN	__BIT(7)
#define	 A64_LINEOUT_RIGHT_EN	__BIT(6)
#define	 A64_LINEOUT_EN		(A64_LINEOUT_LEFT_EN|A64_LINEOUT_RIGHT_EN)
#define	A64_LINEOUT_CTRL1	0x06
#define	 A64_LINEOUT_VOL	__BITS(4,0)
#define	A64_MIC1_CTRL		0x07
#define	 A64_MIC1G		__BITS(6,4)
#define	 A64_MIC1AMPEN		__BIT(3)
#define	 A64_MIC1BOOST		__BITS(2,0)
#define	A64_MIC2_CTRL		0x08
#define	 A64_MIC2_SEL		__BIT(7)
#define	 A64_MIC2G		__BITS(6,4)
#define	 A64_MIC2AMPEN		__BIT(3)
#define	 A64_MIC2BOOST		__BITS(2,0)
#define	A64_LINEIN_CTRL		0x09
#define	 A64_LINEING		__BITS(6,4)
#define	A64_MIX_DAC_CTRL	0x0a
#define	 A64_DACAREN		__BIT(7)
#define	 A64_DACALEN		__BIT(6)
#define	 A64_RMIXEN		__BIT(5)
#define	 A64_LMIXEN		__BIT(4)
#define	 A64_RHPPAMUTE		__BIT(3)
#define	 A64_LHPPAMUTE		__BIT(2)
#define	 A64_RHPIS		__BIT(1)
#define	 A64_LHPIS		__BIT(0)
#define	A64_L_ADCMIX_SRC	0x0b
#define	A64_R_ADCMIX_SRC	0x0c
#define	 A64_ADCMIX_SRC_MIC1	__BIT(6)
#define	 A64_ADCMIX_SRC_MIC2	__BIT(5)
#define	 A64_ADCMIX_SRC_LINEIN	__BIT(2)
#define	 A64_ADCMIX_SRC_OMIXER	__BIT(1)
#define	A64_ADC_CTRL		0x0d
#define	 A64_ADCREN		__BIT(7)
#define	 A64_ADCLEN		__BIT(6)
#define	 A64_ADCG		__BITS(2,0)
#define	A64_JACK_MIC_CTRL	0x1d
#define	 A64_JACKDETEN		__BIT(7)
#define	 A64_INNERRESEN		__BIT(6)
#define	 A64_AUTOPLEN		__BIT(1)

struct a64_acodec_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	int			sc_phandle;

	struct audio_dai_device	sc_dai;
	int			sc_master_dev;
};

enum a64_acodec_mixer_ctrl {
	A64_CODEC_OUTPUT_CLASS,
	A64_CODEC_INPUT_CLASS,
	A64_CODEC_RECORD_CLASS,

	A64_CODEC_OUTPUT_MASTER_VOLUME,
	A64_CODEC_OUTPUT_MUTE,
	A64_CODEC_OUTPUT_SOURCE,
	A64_CODEC_INPUT_LINE_VOLUME,
	A64_CODEC_INPUT_HP_VOLUME,
	A64_CODEC_RECORD_LINE_VOLUME,
	A64_CODEC_RECORD_MIC1_VOLUME,
	A64_CODEC_RECORD_MIC1_PREAMP,
	A64_CODEC_RECORD_MIC2_VOLUME,
	A64_CODEC_RECORD_MIC2_PREAMP,
	A64_CODEC_RECORD_AGC_VOLUME,
	A64_CODEC_RECORD_SOURCE,

	A64_CODEC_MIXER_CTRL_LAST
};

#define	A64_OUTPUT_SOURCE_LINE	__BIT(0)
#define	A64_OUTPUT_SOURCE_HP	__BIT(1)

static const struct a64_acodec_mixer {
	const char *			name;
	enum a64_acodec_mixer_ctrl	mixer_class;
	u_int				reg;
	u_int				mask;
} a64_acodec_mixers[A64_CODEC_MIXER_CTRL_LAST] = {
	[A64_CODEC_INPUT_LINE_VOLUME]	= { AudioNline,
	    A64_CODEC_INPUT_CLASS, A64_LINEOUT_CTRL1, A64_LINEOUT_VOL },
	[A64_CODEC_INPUT_HP_VOLUME]	= { AudioNheadphone,
	    A64_CODEC_INPUT_CLASS, A64_HP_CTRL, A64_HPVOL },

	[A64_CODEC_RECORD_LINE_VOLUME]	= { AudioNline,
	    A64_CODEC_RECORD_CLASS, A64_LINEIN_CTRL, A64_LINEING },
	[A64_CODEC_RECORD_MIC1_VOLUME]	= { AudioNmicrophone,
	    A64_CODEC_RECORD_CLASS, A64_MIC1_CTRL, A64_MIC1G },
	[A64_CODEC_RECORD_MIC2_VOLUME]	= { AudioNmicrophone "2",
	    A64_CODEC_RECORD_CLASS, A64_MIC2_CTRL, A64_MIC2G },
	[A64_CODEC_RECORD_AGC_VOLUME]	= { AudioNagc,
	    A64_CODEC_RECORD_CLASS, A64_ADC_CTRL, A64_ADCG },
};

#define	RD4(sc, reg)			\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	WR4(sc, reg, val)		\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static u_int
a64_acodec_pr_read(struct a64_acodec_softc *sc, u_int addr)
{
	uint32_t val;

	/* Read current value */
	val = RD4(sc, A64_PR_CFG);

	/* De-assert reset */
	val |= A64_AC_PR_RST;
	WR4(sc, A64_PR_CFG, val);

	/* Read mode */
	val &= ~A64_AC_PR_RW;
	WR4(sc, A64_PR_CFG, val);

	/* Set address */
	val &= ~A64_AC_PR_ADDR;
	val |= __SHIFTIN(addr, A64_AC_PR_ADDR);
	WR4(sc, A64_PR_CFG, val);

	/* Read data */
	return __SHIFTOUT(RD4(sc, A64_PR_CFG), A64_ACDA_PR_RDAT);
}

static void
a64_acodec_pr_write(struct a64_acodec_softc *sc, u_int addr, u_int data)
{
	uint32_t val;

	/* Read current value */
	val = RD4(sc, A64_PR_CFG);

	/* De-assert reset */
	val |= A64_AC_PR_RST;
	WR4(sc, A64_PR_CFG, val);

	/* Set address */
	val &= ~A64_AC_PR_ADDR;
	val |= __SHIFTIN(addr, A64_AC_PR_ADDR);
	WR4(sc, A64_PR_CFG, val);

	/* Write data */
	val &= ~A64_ACDA_PR_WDAT;
	val |= __SHIFTIN(data, A64_ACDA_PR_WDAT);
	WR4(sc, A64_PR_CFG, val);

	/* Write mode */
	val |= A64_AC_PR_RW;
	WR4(sc, A64_PR_CFG, val);

	/* Clear write mode */
	val &= ~A64_AC_PR_RW;
	WR4(sc, A64_PR_CFG, val);
}

static void
a64_acodec_pr_set_clear(struct a64_acodec_softc *sc, u_int addr, u_int set, u_int clr)
{
	u_int old, new;

	old = a64_acodec_pr_read(sc, addr);
	new = set | (old & ~clr);
	a64_acodec_pr_write(sc, addr, new);
}

static int
a64_acodec_trigger_output(void *priv, void *start, void *end, int blksize,
    void (*intr)(void *), void *intrarg, const audio_params_t *params)
{
	struct a64_acodec_softc * const sc = priv;

	/* Enable DAC analog l/r channels, HP PA, and output mixer */
	a64_acodec_pr_set_clear(sc, A64_MIX_DAC_CTRL,
	    A64_DACAREN | A64_DACALEN | A64_RMIXEN | A64_LMIXEN |
	    A64_RHPPAMUTE | A64_LHPPAMUTE, 0);

	return 0;
}

static int
a64_acodec_trigger_input(void *priv, void *start, void *end, int blksize,
    void (*intr)(void *), void *intrarg, const audio_params_t *params)
{
	struct a64_acodec_softc * const sc = priv;

	/* Enable ADC analog l/r channels */
	a64_acodec_pr_set_clear(sc, A64_ADC_CTRL,
	    A64_ADCREN | A64_ADCLEN, 0);

	return 0;
}

static int
a64_acodec_halt_output(void *priv)
{
	struct a64_acodec_softc * const sc = priv;

	/* Disable DAC analog l/r channels, HP PA, and output mixer */
	a64_acodec_pr_set_clear(sc, A64_MIX_DAC_CTRL,
	    0, A64_DACAREN | A64_DACALEN | A64_RMIXEN | A64_LMIXEN |
	    A64_RHPPAMUTE | A64_LHPPAMUTE);

	return 0;
}

static int
a64_acodec_halt_input(void *priv)
{
	struct a64_acodec_softc * const sc = priv;

	/* Disable ADC analog l/r channels */
	a64_acodec_pr_set_clear(sc, A64_ADC_CTRL,
	    0, A64_ADCREN | A64_ADCLEN);

	return 0;
}

static int
a64_acodec_set_port(void *priv, mixer_ctrl_t *mc)
{
	struct a64_acodec_softc * const sc = priv;
	const struct a64_acodec_mixer *mix;
	u_int val, shift;
	int nvol, dev;

	dev = mc->dev;
	if (dev == A64_CODEC_OUTPUT_MASTER_VOLUME)
		dev = sc->sc_master_dev;

	switch (dev) {
	case A64_CODEC_INPUT_LINE_VOLUME:
	case A64_CODEC_INPUT_HP_VOLUME:
	case A64_CODEC_RECORD_LINE_VOLUME:
	case A64_CODEC_RECORD_MIC1_VOLUME:
	case A64_CODEC_RECORD_MIC2_VOLUME:
	case A64_CODEC_RECORD_AGC_VOLUME:
		mix = &a64_acodec_mixers[dev];
		val = a64_acodec_pr_read(sc, mix->reg);
		shift = 8 - fls32(__SHIFTOUT_MASK(mix->mask));
		nvol = mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT] >> shift;
		val &= ~mix->mask;
		val |= __SHIFTIN(nvol, mix->mask);
		a64_acodec_pr_write(sc, mix->reg, val);
		return 0;

	case A64_CODEC_RECORD_MIC1_PREAMP:
		if (mc->un.ord < 0 || mc->un.ord > 1)
			return EINVAL;
		if (mc->un.ord) {
			a64_acodec_pr_set_clear(sc, A64_MIC1_CTRL, A64_MIC1AMPEN, 0);
		} else {
			a64_acodec_pr_set_clear(sc, A64_MIC1_CTRL, 0, A64_MIC1AMPEN);
		}
		return 0;
			
	case A64_CODEC_RECORD_MIC2_PREAMP:
		if (mc->un.ord < 0 || mc->un.ord > 1)
			return EINVAL;
		if (mc->un.ord) {
			a64_acodec_pr_set_clear(sc, A64_MIC2_CTRL, A64_MIC2AMPEN, 0);
		} else {
			a64_acodec_pr_set_clear(sc, A64_MIC2_CTRL, 0, A64_MIC2AMPEN);
		}
		return 0;

	case A64_CODEC_OUTPUT_MUTE:
		if (mc->un.ord < 0 || mc->un.ord > 1)
			return EINVAL;
		if (mc->un.ord) {
			a64_acodec_pr_set_clear(sc, A64_OL_MIX_CTRL,
			    0, A64_LMIXMUTE_LDAC);
			a64_acodec_pr_set_clear(sc, A64_OR_MIX_CTRL,
			    0, A64_RMIXMUTE_RDAC);
		} else {
			a64_acodec_pr_set_clear(sc, A64_OL_MIX_CTRL,
			    A64_LMIXMUTE_LDAC, 0);
			a64_acodec_pr_set_clear(sc, A64_OR_MIX_CTRL,
			    A64_RMIXMUTE_RDAC, 0);
		}
		return 0;

	case A64_CODEC_OUTPUT_SOURCE:
		if (mc->un.mask & A64_OUTPUT_SOURCE_LINE)
			a64_acodec_pr_set_clear(sc, A64_LINEOUT_CTRL0,
			    A64_LINEOUT_EN, 0);
		else
			a64_acodec_pr_set_clear(sc, A64_LINEOUT_CTRL0,
			    0, A64_LINEOUT_EN);

		if (mc->un.mask & A64_OUTPUT_SOURCE_HP)
			a64_acodec_pr_set_clear(sc, A64_HP_CTRL,
			    A64_HPPA_EN, 0);
		else
			a64_acodec_pr_set_clear(sc, A64_HP_CTRL,
			    0, A64_HPPA_EN);
		return 0;

	case A64_CODEC_RECORD_SOURCE:
		a64_acodec_pr_write(sc, A64_L_ADCMIX_SRC, mc->un.mask);
		a64_acodec_pr_write(sc, A64_R_ADCMIX_SRC, mc->un.mask);
		return 0;
	}

	return ENXIO;
}

static int
a64_acodec_get_port(void *priv, mixer_ctrl_t *mc)
{
	struct a64_acodec_softc * const sc = priv;
	const struct a64_acodec_mixer *mix;
	u_int val, shift;
	int nvol, dev;

	dev = mc->dev;
	if (dev == A64_CODEC_OUTPUT_MASTER_VOLUME)
		dev = sc->sc_master_dev;

	switch (dev) {
	case A64_CODEC_INPUT_LINE_VOLUME:
	case A64_CODEC_INPUT_HP_VOLUME:
	case A64_CODEC_RECORD_LINE_VOLUME:
	case A64_CODEC_RECORD_MIC1_VOLUME:
	case A64_CODEC_RECORD_MIC2_VOLUME:
	case A64_CODEC_RECORD_AGC_VOLUME:
		mix = &a64_acodec_mixers[dev];
		val = a64_acodec_pr_read(sc, mix->reg);
		shift = 8 - fls32(__SHIFTOUT_MASK(mix->mask));
		nvol = __SHIFTOUT(val, mix->mask) << shift;
		mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT] = nvol;
		mc->un.value.level[AUDIO_MIXER_LEVEL_RIGHT] = nvol;
		return 0;

	case A64_CODEC_RECORD_MIC1_PREAMP:
		mc->un.ord = !!(a64_acodec_pr_read(sc, A64_MIC1_CTRL) & A64_MIC1AMPEN);
		return 0;
			
	case A64_CODEC_RECORD_MIC2_PREAMP:
		mc->un.ord = !!(a64_acodec_pr_read(sc, A64_MIC2_CTRL) & A64_MIC2AMPEN);
		return 0;

	case A64_CODEC_OUTPUT_MUTE:
		mc->un.ord = 1;
		if (a64_acodec_pr_read(sc, A64_OL_MIX_CTRL) & A64_LMIXMUTE_LDAC)
			mc->un.ord = 0;
		if (a64_acodec_pr_read(sc, A64_OR_MIX_CTRL) & A64_RMIXMUTE_RDAC)
			mc->un.ord = 0;
		return 0;

	case A64_CODEC_OUTPUT_SOURCE:
		mc->un.mask = 0;
		if (a64_acodec_pr_read(sc, A64_LINEOUT_CTRL0) & A64_LINEOUT_EN)
			mc->un.mask |= A64_OUTPUT_SOURCE_LINE;
		if (a64_acodec_pr_read(sc, A64_HP_CTRL) & A64_HPPA_EN)
			mc->un.mask |= A64_OUTPUT_SOURCE_HP;
		return 0;

	case A64_CODEC_RECORD_SOURCE:
		mc->un.mask =
		    a64_acodec_pr_read(sc, A64_L_ADCMIX_SRC) |
		    a64_acodec_pr_read(sc, A64_R_ADCMIX_SRC);
		return 0;
	}

	return ENXIO;
}

static int
a64_acodec_query_devinfo(void *priv, mixer_devinfo_t *di)
{
	struct a64_acodec_softc * const sc = priv;
	const struct a64_acodec_mixer *mix;

	switch (di->index) {
	case A64_CODEC_OUTPUT_CLASS:
		di->mixer_class = di->index;
		strcpy(di->label.name, AudioCoutputs);
		di->type = AUDIO_MIXER_CLASS;
		di->next = di->prev = AUDIO_MIXER_LAST;
		return 0;

	case A64_CODEC_INPUT_CLASS:
		di->mixer_class = di->index;
		strcpy(di->label.name, AudioCinputs);
		di->type = AUDIO_MIXER_CLASS;
		di->next = di->prev = AUDIO_MIXER_LAST;
		return 0;

	case A64_CODEC_RECORD_CLASS:
		di->mixer_class = di->index;
		strcpy(di->label.name, AudioCrecord);
		di->type = AUDIO_MIXER_CLASS;
		di->next = di->prev = AUDIO_MIXER_LAST;
		return 0;

	case A64_CODEC_OUTPUT_MASTER_VOLUME:
		mix = &a64_acodec_mixers[sc->sc_master_dev];
		di->mixer_class = A64_CODEC_OUTPUT_CLASS;
		strcpy(di->label.name, AudioNmaster);
		di->un.v.delta =
		    256 / (__SHIFTOUT_MASK(mix->mask) + 1);
		di->type = AUDIO_MIXER_VALUE;
		di->next = di->prev = AUDIO_MIXER_LAST;
		di->un.v.num_channels = 2;
		strcpy(di->un.v.units.name, AudioNvolume);
		return 0;

	case A64_CODEC_INPUT_LINE_VOLUME:
	case A64_CODEC_INPUT_HP_VOLUME:
	case A64_CODEC_RECORD_LINE_VOLUME:
	case A64_CODEC_RECORD_MIC1_VOLUME:
	case A64_CODEC_RECORD_MIC2_VOLUME:
	case A64_CODEC_RECORD_AGC_VOLUME:
		mix = &a64_acodec_mixers[di->index];
		di->mixer_class = mix->mixer_class;
		strcpy(di->label.name, mix->name);
		di->un.v.delta =
		    256 / (__SHIFTOUT_MASK(mix->mask) + 1);
		di->type = AUDIO_MIXER_VALUE;
		di->prev = AUDIO_MIXER_LAST;
		if (di->index == A64_CODEC_RECORD_MIC1_VOLUME)
			di->next = A64_CODEC_RECORD_MIC1_PREAMP;
		else if (di->index == A64_CODEC_RECORD_MIC2_VOLUME)
			di->next = A64_CODEC_RECORD_MIC2_PREAMP;
		else
			di->next = AUDIO_MIXER_LAST;
		di->un.v.num_channels = 2;
		strcpy(di->un.v.units.name, AudioNvolume);
		return 0;

	case A64_CODEC_RECORD_MIC1_PREAMP:
	case A64_CODEC_RECORD_MIC2_PREAMP:
		di->mixer_class = A64_CODEC_RECORD_CLASS;
		strcpy(di->label.name, AudioNpreamp);
		di->type = AUDIO_MIXER_ENUM;
		if (di->index == A64_CODEC_RECORD_MIC1_PREAMP)
			di->prev = A64_CODEC_RECORD_MIC1_VOLUME;
		else
			di->prev = A64_CODEC_RECORD_MIC2_VOLUME;
		di->next = AUDIO_MIXER_LAST;
		di->un.e.num_mem = 2;
		strcpy(di->un.e.member[0].label.name, AudioNoff);
		di->un.e.member[0].ord = 0;
		strcpy(di->un.e.member[1].label.name, AudioNon);
		di->un.e.member[1].ord = 1;
		return 0;

	case A64_CODEC_OUTPUT_MUTE:
		di->mixer_class = A64_CODEC_OUTPUT_CLASS;
		strcpy(di->label.name, AudioNmute);
		di->type = AUDIO_MIXER_ENUM;
		di->next = di->prev = AUDIO_MIXER_LAST;
		di->un.e.num_mem = 2;
		strcpy(di->un.e.member[0].label.name, AudioNoff);
		di->un.e.member[0].ord = 0;
		strcpy(di->un.e.member[1].label.name, AudioNon);
		di->un.e.member[1].ord = 1;
		return 0;

	case A64_CODEC_OUTPUT_SOURCE:
		di->mixer_class = A64_CODEC_OUTPUT_CLASS;
		strcpy(di->label.name, AudioNsource);
		di->type = AUDIO_MIXER_SET;
		di->next = di->prev = AUDIO_MIXER_LAST;
		di->un.s.num_mem = 2;
		strcpy(di->un.s.member[0].label.name, AudioNline);
		di->un.s.member[0].mask = A64_OUTPUT_SOURCE_LINE;
		strcpy(di->un.s.member[1].label.name, AudioNheadphone);
		di->un.s.member[1].mask = A64_OUTPUT_SOURCE_HP;
		return 0;

	case A64_CODEC_RECORD_SOURCE:
		di->mixer_class = A64_CODEC_RECORD_CLASS;
		strcpy(di->label.name, AudioNsource);
		di->type = AUDIO_MIXER_SET;
		di->next = di->prev = AUDIO_MIXER_LAST;
		di->un.s.num_mem = 4;
		strcpy(di->un.s.member[0].label.name, AudioNline);
		di->un.s.member[0].mask = A64_ADCMIX_SRC_LINEIN;
		strcpy(di->un.s.member[1].label.name, AudioNmicrophone);
		di->un.s.member[1].mask = A64_ADCMIX_SRC_MIC1;
		strcpy(di->un.s.member[2].label.name, AudioNmicrophone "2");
		di->un.s.member[2].mask = A64_ADCMIX_SRC_MIC2;
		strcpy(di->un.s.member[3].label.name, AudioNdac);
		di->un.s.member[3].mask = A64_ADCMIX_SRC_OMIXER;
		return 0;

	}

	return ENXIO;
}

static const struct audio_hw_if a64_acodec_hw_if = {
	.trigger_output = a64_acodec_trigger_output,
	.trigger_input = a64_acodec_trigger_input,
	.halt_output = a64_acodec_halt_output,
	.halt_input = a64_acodec_halt_input,
	.set_port = a64_acodec_set_port,
	.get_port = a64_acodec_get_port,
	.query_devinfo = a64_acodec_query_devinfo,
};

static audio_dai_tag_t
a64_acodec_dai_get_tag(device_t dev, const void *data, size_t len)
{
	struct a64_acodec_softc * const sc = device_private(dev);

	if (len != 4)
		return NULL;

	return &sc->sc_dai;
}

static struct fdtbus_dai_controller_func a64_acodec_dai_funcs = {
	.get_tag = a64_acodec_dai_get_tag
};

static int
a64_acodec_dai_jack_detect(audio_dai_tag_t dai, u_int jack, int present)
{
	struct a64_acodec_softc * const sc = audio_dai_private(dai);

	switch (jack) {
	case AUDIO_DAI_JACK_HP:
		if (present) {
			a64_acodec_pr_set_clear(sc, A64_LINEOUT_CTRL0,
			    0, A64_LINEOUT_EN);
			a64_acodec_pr_set_clear(sc, A64_HP_CTRL,
			    A64_HPPA_EN, 0);
		} else {
			a64_acodec_pr_set_clear(sc, A64_LINEOUT_CTRL0,
			    A64_LINEOUT_EN, 0);
			a64_acodec_pr_set_clear(sc, A64_HP_CTRL,
			    0, A64_HPPA_EN);
		}

		/* Master volume controls either HP or line out */
		sc->sc_master_dev = present ?
		    A64_CODEC_INPUT_HP_VOLUME : A64_CODEC_INPUT_LINE_VOLUME;

		break;

	case AUDIO_DAI_JACK_MIC:
		/* XXX TODO */
		break;
	}

	return 0;
}

static const char * compatible[] = {
	"allwinner,sun50i-a64-codec-analog",
	NULL
};

static int
a64_acodec_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
a64_acodec_attach(device_t parent, device_t self, void *aux)
{
	struct a64_acodec_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;

	sc->sc_dev = self;
	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}
	sc->sc_bst = faa->faa_bst;
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	sc->sc_phandle = phandle;

	aprint_naive("\n");
	aprint_normal(": A64 Audio Codec (analog part)\n");

	/* Right & Left Headphone PA enable */
	a64_acodec_pr_set_clear(sc, A64_HP_CTRL,
	    A64_HPPA_EN, 0);

	/* Jack detect enable */
	sc->sc_master_dev = A64_CODEC_INPUT_HP_VOLUME;
	a64_acodec_pr_set_clear(sc, A64_JACK_MIC_CTRL,
	    A64_JACKDETEN | A64_INNERRESEN | A64_AUTOPLEN, 0);

	/* Unmute DAC to output mixer */
	a64_acodec_pr_set_clear(sc, A64_OL_MIX_CTRL,
	    A64_LMIXMUTE_LDAC, 0);
	a64_acodec_pr_set_clear(sc, A64_OR_MIX_CTRL,
	    A64_RMIXMUTE_RDAC, 0);

	sc->sc_dai.dai_jack_detect = a64_acodec_dai_jack_detect;
	sc->sc_dai.dai_hw_if = &a64_acodec_hw_if;
	sc->sc_dai.dai_dev = self;
	sc->sc_dai.dai_priv = sc;
	fdtbus_register_dai_controller(self, phandle, &a64_acodec_dai_funcs);
}

CFATTACH_DECL_NEW(a64_acodec, sizeof(struct a64_acodec_softc),
    a64_acodec_match, a64_acodec_attach, NULL, NULL);
