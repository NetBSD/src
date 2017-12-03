/* $NetBSD: sun6i_a31_codec.c,v 1.1.2.2 2017/12/03 11:35:56 jdolecek Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: sun6i_a31_codec.c,v 1.1.2.2 2017/12/03 11:35:56 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/bitops.h>

#include <sys/audioio.h>
#include <dev/audio_if.h>

#include <arm/sunxi/sunxi_codec.h>

#define	A31_DEFAULT_HPVOL	0x20

#define	OMIXER_DACA_CTRL	0x20
#define	 DACAREN		__BIT(31)
#define	 DACALEN		__BIT(30)
#define	 RMIXEN			__BIT(29)
#define	 LMIXEN			__BIT(28)
#define	 RMIXMUTE		__BITS(23,17)
#define	 RMIXMUTE_MIC1		__BIT(23)
#define	 RMIXMUTE_MIC2		__BIT(22)
#define	 RMIXMUTE_PHONEP_PHONEN	__BIT(21)
#define	 RMIXMUTE_PHONEP	__BIT(20)
#define	 RMIXMUTE_LINEINR	__BIT(19)
#define	 RMIXMUTE_DACR		__BIT(18)
#define	 RMIXMUTE_DACL		__BIT(17)
#define	 LMIXMUTE		__BITS(16,10)
#define	 LMIXMUTE_MIC1		__BIT(16)
#define	 LMIXMUTE_MIC2		__BIT(15)
#define	 LMIXMUTE_PHONEP_PHONEN	__BIT(14)
#define	 LMIXMUTE_PHONEN	__BIT(13)
#define	 LMIXMUTE_LINEINL	__BIT(12)
#define	 LMIXMUTE_DACL		__BIT(11)
#define	 LMIXMUTE_DACR		__BIT(10)
#define	 RHPIS			__BIT(9)
#define	 LHPIS			__BIT(8)
#define	 RHPPAMUTE		__BIT(7)
#define	 LHPPAMUTE		__BIT(6)
#define	 HPVOL			__BITS(5,0)

#define	OMIXER_PA_CTRL		0x24
#define	 HPPAEN			__BIT(31)
#define	 HPCOM_CTL		__BITS(30,29)
#define	 COMPTEN		__BIT(28)
#define	 PA_ANTI_POP_CTRL	__BITS(27,26)
#define	 MIC1G			__BITS(17,15)
#define	 MIC2G			__BITS(14,12)
#define	 LINEING		__BITS(11,9)
#define	 PHONEG			__BITS(8,6)
#define	 PHONEPG		__BITS(5,3)
#define	 PHONENG		__BITS(2,0)

#define	AC_MIC_CTRL		0x28
#define	 HBIASEN		__BIT(31)
#define	 MBIASEN		__BIT(30)
#define	 HBIASADCEN		__BIT(29)
#define	 MIC1AMPEN		__BIT(28)
#define	 MIC1BOOST		__BITS(27,25)
#define	 MIC2AMPEN		__BIT(24)
#define	 MIC2BOOST		__BITS(23,21)
#define	 MIC2SLT		__BIT(20)
#define	 LINEOUTLEN		__BIT(19)
#define	 LINEOUTREN		__BIT(18)
#define	 LINEOUTLSRC		__BIT(17)
#define	 LINEOUTRSRC		__BIT(16)
#define	 LINEOUTVC		__BITS(15,11)
#define	 PHONEPREG		__BITS(10,8)
#define	 PHONEOUTG		__BITS(7,5)
#define	 PHONEOUTEN		__BIT(4)
#define	 PHONEOUTS0		__BIT(3)
#define	 PHONEOUTS1		__BIT(2)
#define	 PHONEOUTS2		__BIT(1)
#define	 PHONEOUTS3		__BIT(0)

#define	AC_ADCA_CTRL		0x2c
#define	 ADCREN			__BIT(31)
#define	 ADCLEN			__BIT(30)
#define	 ADCRG			__BITS(29,27)
#define	 ADCLG			__BITS(26,24)
#define	 RADCMIXMUTE		__BITS(13,7)
#define	 RADCMIXMUTE_MIC1	__BIT(13)
#define	 RADCMIXMUTE_MIC2	__BIT(12)
#define	 RADCMIXMUTE_PHONEP_PHONEN __BIT(11)
#define	 RADCMIXMUTE_PHONEP	__BIT(10)
#define	 RADCMIXMUTE_LINEINR	__BIT(9)
#define	 RADCMIXMUTE_ROM	__BIT(8)
#define	 RADCMIXMUTE_LOM	__BIT(7)
#define	 LADCMIXMUTE		__BITS(6,0)
#define	 LADCMIXMUTE_MIC1	__BIT(6)
#define	 LADCMIXMUTE_MIC2	__BIT(5)
#define	 LADCMIXMUTE_PHONEP_PHONEN __BIT(4)
#define	 LADCMIXMUTE_PHONEN	__BIT(3)
#define	 LADCMIXMUTE_LINEINL	__BIT(2)
#define	 LADCMIXMUTE_LOM	__BIT(1)
#define	 LADCMIXMUTE_ROM	__BIT(0)

enum a31_codec_mixer_ctrl {
	A31_CODEC_OUTPUT_CLASS,
	A31_CODEC_INPUT_CLASS,

	A31_CODEC_OUTPUT_MASTER_VOLUME,
	A31_CODEC_INPUT_DAC_VOLUME,

	A31_CODEC_MIXER_CTRL_LAST
};

static const struct a31_codec_mixer {
	const char *			name;
	enum a31_codec_mixer_ctrl	mixer_class;
	u_int				reg;
	u_int				mask;
} a31_codec_mixers[A31_CODEC_MIXER_CTRL_LAST] = {
	[A31_CODEC_OUTPUT_MASTER_VOLUME]	= { AudioNmaster,
	    A31_CODEC_OUTPUT_CLASS, OMIXER_DACA_CTRL, HPVOL },
	[A31_CODEC_INPUT_DAC_VOLUME]		= { AudioNdac,
	    A31_CODEC_INPUT_CLASS, OMIXER_DACA_CTRL, HPVOL },
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
a31_codec_init(struct sunxi_codec_softc *sc)
{

	/* Disable HPCOM and HPCOM output protection */
	CLR4(sc, OMIXER_PA_CTRL, HPCOM_CTL | COMPTEN);
	/* Enable headphone power amp */
	SET4(sc, OMIXER_PA_CTRL, HPPAEN);

	/* Set headphone PA input to DAC */
	CLR4(sc, OMIXER_DACA_CTRL, RHPIS | LHPIS);
	/* Mute inputs to headphone PA */
	CLR4(sc, OMIXER_DACA_CTRL, RHPPAMUTE | LHPPAMUTE);
	/* Set initial volume */
	CLR4(sc, OMIXER_DACA_CTRL, HPVOL);
	SET4(sc, OMIXER_DACA_CTRL, __SHIFTIN(A31_DEFAULT_HPVOL, HPVOL));

	/* Disable lineout */
	CLR4(sc, AC_MIC_CTRL, LINEOUTLEN | LINEOUTREN | LINEOUTVC);
	/* Enable headset microphone bias, current sensor, and ADC */
	SET4(sc, AC_MIC_CTRL, HBIASEN | HBIASADCEN);

	return 0;
}

static void
a31_codec_mute(struct sunxi_codec_softc *sc, int mute, u_int mode)
{
	if (mode == AUMODE_PLAY) {
		if (sc->sc_pin_pa != NULL)
			fdtbus_gpio_write(sc->sc_pin_pa, !mute);

		if (mute) {
			CLR4(sc, OMIXER_DACA_CTRL, DACAREN | DACALEN);
		} else {
			CLR4(sc, OMIXER_DACA_CTRL, RMIXMUTE | LMIXMUTE);
			SET4(sc, OMIXER_DACA_CTRL,
			    LHPIS | RHPIS | LHPPAMUTE | RHPPAMUTE |
			    DACAREN | DACALEN | RMIXEN | LMIXEN |
			    RMIXMUTE_DACR | LMIXMUTE_DACL);
		}
	} else {
		if (mute) {
			CLR4(sc, AC_ADCA_CTRL, ADCREN | ADCLEN);
		} else {
			SET4(sc, AC_ADCA_CTRL, ADCREN | ADCLEN);
		}
	}
}

static int
a31_codec_set_port(struct sunxi_codec_softc *sc, mixer_ctrl_t *mc)
{
	const struct a31_codec_mixer *mix;
	u_int val, shift;
	int nvol;

	switch (mc->dev) {
	case A31_CODEC_OUTPUT_MASTER_VOLUME:
	case A31_CODEC_INPUT_DAC_VOLUME:
		mix = &a31_codec_mixers[mc->dev];
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
a31_codec_get_port(struct sunxi_codec_softc *sc, mixer_ctrl_t *mc)
{
	const struct a31_codec_mixer *mix;
	u_int val, shift;
	int nvol;

	switch (mc->dev) {
	case A31_CODEC_OUTPUT_MASTER_VOLUME:
	case A31_CODEC_INPUT_DAC_VOLUME:
		mix = &a31_codec_mixers[mc->dev];
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
a31_codec_query_devinfo(struct sunxi_codec_softc *sc, mixer_devinfo_t *di)
{
	const struct a31_codec_mixer *mix;

	switch (di->index) {
	case A31_CODEC_OUTPUT_CLASS:
		di->mixer_class = di->index;
		strcpy(di->label.name, AudioCoutputs);
		di->type = AUDIO_MIXER_CLASS;
		di->next = di->prev = AUDIO_MIXER_LAST;
		return 0;

	case A31_CODEC_INPUT_CLASS:
		di->mixer_class = di->index;
		strcpy(di->label.name, AudioCinputs);
		di->type = AUDIO_MIXER_CLASS;
		di->next = di->prev = AUDIO_MIXER_LAST;
		return 0;

	case A31_CODEC_OUTPUT_MASTER_VOLUME:
	case A31_CODEC_INPUT_DAC_VOLUME:
		mix = &a31_codec_mixers[di->index];
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

const struct sunxi_codec_conf sun6i_a31_codecconf = {
	.name = "A31 Audio Codec",

	.init = a31_codec_init,
	.mute = a31_codec_mute,
	.set_port = a31_codec_set_port,
	.get_port = a31_codec_get_port,
	.query_devinfo = a31_codec_query_devinfo,

	.DPC		= 0x00,
	.DAC_FIFOC	= 0x04,
	.DAC_FIFOS	= 0x08,
	.DAC_TXDATA	= 0x0c,
	.ADC_FIFOC	= 0x10,
	.ADC_FIFOS	= 0x14,
	.ADC_RXDATA	= 0x18,
	.DAC_CNT	= 0x40,
	.ADC_CNT	= 0x44,
};
