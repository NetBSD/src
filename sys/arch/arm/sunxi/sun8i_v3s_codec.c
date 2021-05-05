/* $NetBSD: sun8i_v3s_codec.c,v 1.1 2021/05/05 10:24:04 jmcneill Exp $ */

/*-
 * Copyright (c) 2021 Rui-Xiang Guo
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
__KERNEL_RCSID(0, "$NetBSD: sun8i_v3s_codec.c,v 1.1 2021/05/05 10:24:04 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/bitops.h>

#include <sys/audioio.h>
#include <dev/audio/audio_if.h>

#include <arm/sunxi/sunxi_codec.h>

#define	V3S_PR_CFG		0x00
#define	 V3S_PR_RST		__BIT(28)
#define	 V3S_PR_RW		__BIT(24)
#define	 V3S_PR_ADDR		__BITS(20,16)
#define	 V3S_ADDA_PR_WDAT	__BITS(15,8)
#define	 V3S_ADDA_PR_RDAT	__BITS(7,0)

#define	V3S_PAG_HPV		0x00
#define	 V3S_HPVOL		__BITS(5,0)

#define	V3S_LMIXMUTE		0x01
#define	 V3S_LMIXMUTE_LDAC	__BIT(1)
#define	V3S_RMIXMUTE		0x02
#define	 V3S_RMIXMUTE_RDAC	__BIT(1)
#define	V3S_DAC_PA_SRC		0x03
#define	 V3S_DACAREN		__BIT(7)
#define	 V3S_DACALEN		__BIT(6)
#define	 V3S_RMIXEN		__BIT(5)
#define	 V3S_LMIXEN		__BIT(4)
#define	 V3S_RHPPAMUTE		__BIT(3)
#define	 V3S_LHPPAMUTE		__BIT(2)
#define	V3S_MIC_GCTR		0x06
#define	 V3S_MIC_GAIN		__BITS(6,4)
#define	V3S_HP_CTRL		0x07
#define	 V3S_HPPAEN		__BIT(7)
#define	V3S_LADCMIXMUTE		0x0c
#define	V3S_RADCMIXMUTE		0x0d
#define	 V3S_ADCMIXMUTE_MIC	__BIT(6)
#define	 V3S_ADCMIXMUTE_MIXER	__BITS(1,0)
#define	V3S_ADC_CTRL		0x0f
#define	 V3S_ADCREN		__BIT(7)
#define	 V3S_ADCLEN		__BIT(6)
#define	 V3S_ADCG		__BITS(2,0)

struct v3s_codec_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	int			sc_phandle;
};

enum v3s_codec_mixer_ctrl {
	V3S_CODEC_OUTPUT_CLASS,
	V3S_CODEC_INPUT_CLASS,
	V3S_CODEC_RECORD_CLASS,

	V3S_CODEC_OUTPUT_MASTER_VOLUME,
	V3S_CODEC_INPUT_MIC_VOLUME,
	V3S_CODEC_RECORD_AGC_VOLUME,
	V3S_CODEC_RECORD_SOURCE,

	V3S_CODEC_MIXER_CTRL_LAST
};

static const struct v3s_codec_mixer {
	const char *			name;
	enum v3s_codec_mixer_ctrl	mixer_class;
	u_int				reg;
	u_int				mask;
} v3s_codec_mixers[V3S_CODEC_MIXER_CTRL_LAST] = {
	[V3S_CODEC_OUTPUT_MASTER_VOLUME]	= { AudioNmaster,
	    V3S_CODEC_OUTPUT_CLASS, V3S_PAG_HPV, V3S_HPVOL },
	[V3S_CODEC_INPUT_MIC_VOLUME]	= { "mic",
	    V3S_CODEC_INPUT_CLASS, V3S_MIC_GCTR, V3S_MIC_GAIN },
	[V3S_CODEC_RECORD_AGC_VOLUME]	= { AudioNagc,
	    V3S_CODEC_RECORD_CLASS, V3S_ADC_CTRL, V3S_ADCG },
};

#define	RD4(sc, reg)			\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	WR4(sc, reg, val)		\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static struct v3s_codec_softc *
v3s_codec_find(int phandle)
{
	struct v3s_codec_softc *csc;
	device_t dev;

	dev = device_find_by_driver_unit("v3scodec", 0);
	if (dev == NULL)
		return NULL;
	csc = device_private(dev);
	if (csc->sc_phandle != phandle)
		return NULL;

	return csc;
}

static u_int
v3s_codec_pr_read(struct v3s_codec_softc *csc, u_int addr)
{
	uint32_t val;

	/* Read current value */
	val = RD4(csc, V3S_PR_CFG);

	/* De-assert reset */
	val |= V3S_PR_RST;
	WR4(csc, V3S_PR_CFG, val);

	/* Read mode */
	val &= ~V3S_PR_RW;
	WR4(csc, V3S_PR_CFG, val);

	/* Set address */
	val &= ~V3S_PR_ADDR;
	val |= __SHIFTIN(addr, V3S_PR_ADDR);
	WR4(csc, V3S_PR_CFG, val);

	/* Read data */
	return __SHIFTOUT(RD4(csc, V3S_PR_CFG), V3S_ADDA_PR_RDAT);
}

static void
v3s_codec_pr_write(struct v3s_codec_softc *csc, u_int addr, u_int data)
{
	uint32_t val;

	/* Read current value */
	val = RD4(csc, V3S_PR_CFG);

	/* De-assert reset */
	val |= V3S_PR_RST;
	WR4(csc, V3S_PR_CFG, val);

	/* Set address */
	val &= ~V3S_PR_ADDR;
	val |= __SHIFTIN(addr, V3S_PR_ADDR);
	WR4(csc, V3S_PR_CFG, val);

	/* Write data */
	val &= ~V3S_ADDA_PR_WDAT;
	val |= __SHIFTIN(data, V3S_ADDA_PR_WDAT);
	WR4(csc, V3S_PR_CFG, val);

	/* Write mode */
	val |= V3S_PR_RW;
	WR4(csc, V3S_PR_CFG, val);

	/* Clear write mode */
	val &= ~V3S_PR_RW;
	WR4(csc, V3S_PR_CFG, val);
}

static void
v3s_codec_pr_set_clear(struct v3s_codec_softc *csc, u_int addr, u_int set, u_int clr)
{
	u_int old, new;

	old = v3s_codec_pr_read(csc, addr);
	new = set | (old & ~clr);
	v3s_codec_pr_write(csc, addr, new);
}

static int
v3s_codec_init(struct sunxi_codec_softc *sc)
{
	struct v3s_codec_softc *csc;
	int phandle;

	/* Lookup the codec analog controls phandle */
	phandle = fdtbus_get_phandle(sc->sc_phandle,
	    "allwinner,codec-analog-controls");
	if (phandle < 0) {
		aprint_error_dev(sc->sc_dev,
		    "missing allwinner,codec-analog-controls property\n");
		return ENXIO;
	}

	/* Find a matching v3scodec instance */
	sc->sc_codec_priv = v3s_codec_find(phandle);
	if (sc->sc_codec_priv == NULL) {
		aprint_error_dev(sc->sc_dev, "couldn't find codec analog controls\n");
		return ENOENT;
	}
	csc = sc->sc_codec_priv;

	/* Right & Left Headphone enable */
	v3s_codec_pr_set_clear(csc, V3S_HP_CTRL, V3S_HPPAEN, 0);

	return 0;
}

static void
v3s_codec_mute(struct sunxi_codec_softc *sc, int mute, u_int mode)
{
	struct v3s_codec_softc * const csc = sc->sc_codec_priv;

	if (mode == AUMODE_PLAY) {
		if (mute) {
			/* Mute DAC l/r channels to output mixer */
			v3s_codec_pr_set_clear(csc, V3S_LMIXMUTE,
			    0, V3S_LMIXMUTE_LDAC);
			v3s_codec_pr_set_clear(csc, V3S_RMIXMUTE,
			    0, V3S_RMIXMUTE_RDAC);
			/* Disable DAC analog l/r channels and output mixer */
			v3s_codec_pr_set_clear(csc, V3S_DAC_PA_SRC,
			    0, V3S_DACAREN | V3S_DACALEN | V3S_RMIXEN | V3S_LMIXEN | V3S_RHPPAMUTE | V3S_LHPPAMUTE);
		} else {
			/* Enable DAC analog l/r channels and output mixer */
			v3s_codec_pr_set_clear(csc, V3S_DAC_PA_SRC,
			    V3S_DACAREN | V3S_DACALEN | V3S_RMIXEN | V3S_LMIXEN | V3S_RHPPAMUTE | V3S_LHPPAMUTE, 0);
			/* Unmute DAC l/r channels to output mixer */
			v3s_codec_pr_set_clear(csc, V3S_LMIXMUTE, V3S_LMIXMUTE_LDAC, 0);
			v3s_codec_pr_set_clear(csc, V3S_RMIXMUTE, V3S_RMIXMUTE_RDAC, 0);
		}
	} else {
		if (mute) {
			/* Disable ADC analog l/r channels */
			v3s_codec_pr_set_clear(csc, V3S_ADC_CTRL,
			    0, V3S_ADCREN | V3S_ADCLEN);
		} else {
			/* Enable ADC analog l/r channels */
			v3s_codec_pr_set_clear(csc, V3S_ADC_CTRL,
			    V3S_ADCREN | V3S_ADCLEN, 0);
		}
	}
}

static int
v3s_codec_set_port(struct sunxi_codec_softc *sc, mixer_ctrl_t *mc)
{
	struct v3s_codec_softc * const csc = sc->sc_codec_priv;
	const struct v3s_codec_mixer *mix;
	u_int val, shift;
	int nvol;

	switch (mc->dev) {
	case V3S_CODEC_OUTPUT_MASTER_VOLUME:
	case V3S_CODEC_INPUT_MIC_VOLUME:
	case V3S_CODEC_RECORD_AGC_VOLUME:
		mix = &v3s_codec_mixers[mc->dev];
		val = v3s_codec_pr_read(csc, mix->reg);
		shift = 8 - fls32(__SHIFTOUT_MASK(mix->mask));
		nvol = mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT] >> shift;
		val &= ~mix->mask;
		val |= __SHIFTIN(nvol, mix->mask);
		v3s_codec_pr_write(csc, mix->reg, val);
		return 0;

	case V3S_CODEC_RECORD_SOURCE:
		v3s_codec_pr_write(csc, V3S_LADCMIXMUTE, mc->un.mask);
		v3s_codec_pr_write(csc, V3S_RADCMIXMUTE, mc->un.mask);
		return 0;
	}

	return ENXIO;
}

static int
v3s_codec_get_port(struct sunxi_codec_softc *sc, mixer_ctrl_t *mc)
{
	struct v3s_codec_softc * const csc = sc->sc_codec_priv;
	const struct v3s_codec_mixer *mix;
	u_int val, shift;
	int nvol;

	switch (mc->dev) {
	case V3S_CODEC_OUTPUT_MASTER_VOLUME:
	case V3S_CODEC_INPUT_MIC_VOLUME:
	case V3S_CODEC_RECORD_AGC_VOLUME:
		mix = &v3s_codec_mixers[mc->dev];
		val = v3s_codec_pr_read(csc, mix->reg);
		shift = 8 - fls32(__SHIFTOUT_MASK(mix->mask));
		nvol = __SHIFTOUT(val, mix->mask) << shift;
		mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT] = nvol;
		mc->un.value.level[AUDIO_MIXER_LEVEL_RIGHT] = nvol;
		return 0;

	case V3S_CODEC_RECORD_SOURCE:
		mc->un.mask =
		    v3s_codec_pr_read(csc, V3S_LADCMIXMUTE) |
		    v3s_codec_pr_read(csc, V3S_RADCMIXMUTE);
		return 0;
	}

	return ENXIO;
}

static int
v3s_codec_query_devinfo(struct sunxi_codec_softc *sc, mixer_devinfo_t *di)
{
	const struct v3s_codec_mixer *mix;

	switch (di->index) {
	case V3S_CODEC_OUTPUT_CLASS:
		di->mixer_class = di->index;
		strcpy(di->label.name, AudioCoutputs);
		di->type = AUDIO_MIXER_CLASS;
		di->next = di->prev = AUDIO_MIXER_LAST;
		return 0;

	case V3S_CODEC_INPUT_CLASS:
		di->mixer_class = di->index;
		strcpy(di->label.name, AudioCinputs);
		di->type = AUDIO_MIXER_CLASS;
		di->next = di->prev = AUDIO_MIXER_LAST;
		return 0;

	case V3S_CODEC_RECORD_CLASS:
		di->mixer_class = di->index;
		strcpy(di->label.name, AudioCrecord);
		di->type = AUDIO_MIXER_CLASS;
		di->next = di->prev = AUDIO_MIXER_LAST;
		return 0;

	case V3S_CODEC_OUTPUT_MASTER_VOLUME:
	case V3S_CODEC_INPUT_MIC_VOLUME:
	case V3S_CODEC_RECORD_AGC_VOLUME:
		mix = &v3s_codec_mixers[di->index];
		di->mixer_class = mix->mixer_class;
		strcpy(di->label.name, mix->name);
		di->un.v.delta =
		    256 / (__SHIFTOUT_MASK(mix->mask) + 1);
		di->type = AUDIO_MIXER_VALUE;
		di->next = di->prev = AUDIO_MIXER_LAST;
		di->un.v.num_channels = 2;
		strcpy(di->un.v.units.name, AudioNvolume);
		return 0;

	case V3S_CODEC_RECORD_SOURCE:
		di->mixer_class = V3S_CODEC_RECORD_CLASS;
		strcpy(di->label.name, AudioNsource);
		di->type = AUDIO_MIXER_SET;
		di->next = di->prev = AUDIO_MIXER_LAST;
		di->un.s.num_mem = 2;
		strcpy(di->un.s.member[0].label.name, "mic");
		di->un.s.member[1].mask = V3S_ADCMIXMUTE_MIC;
		strcpy(di->un.s.member[1].label.name, AudioNdac);
		di->un.s.member[3].mask = V3S_ADCMIXMUTE_MIXER;
		return 0;

	}

	return ENXIO;
}

const struct sunxi_codec_conf sun8i_v3s_codecconf = {
	.name = "V3s Audio Codec",

	.init = v3s_codec_init,
	.mute = v3s_codec_mute,
	.set_port = v3s_codec_set_port,
	.get_port = v3s_codec_get_port,
	.query_devinfo = v3s_codec_query_devinfo,

	.DPC		= 0x00,
	.DAC_FIFOC	= 0x04,
	.DAC_FIFOS	= 0x08,
	.DAC_TXDATA	= 0x20,
	.ADC_FIFOC	= 0x10,
	.ADC_FIFOS	= 0x14,
	.ADC_RXDATA	= 0x18,
	.DAC_CNT	= 0x40,
	.ADC_CNT	= 0x44,
};

/*
 * Device glue, only here to claim resources on behalf of the sunxi_codec driver.
 */

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "allwinner,sun8i-v3s-codec-analog" },
	DEVICE_COMPAT_EOL
};

static int
v3s_codec_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
v3s_codec_attach(device_t parent, device_t self, void *aux)
{
	struct v3s_codec_softc * const sc = device_private(self);
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
	aprint_normal(": V3s Audio Codec (analog part)\n");
}

CFATTACH_DECL_NEW(v3s_codec, sizeof(struct v3s_codec_softc),
    v3s_codec_match, v3s_codec_attach, NULL, NULL);
