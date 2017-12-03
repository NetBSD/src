/* $NetBSD: sun8i_h3_codec.c,v 1.2.6.2 2017/12/03 11:35:56 jdolecek Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: sun8i_h3_codec.c,v 1.2.6.2 2017/12/03 11:35:56 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/bitops.h>

#include <sys/audioio.h>
#include <dev/audio_if.h>

#include <arm/sunxi/sunxi_codec.h>

#define	H3_PR_CFG		0x00
#define	 H3_AC_PR_RW		__BIT(24)
#define	 H3_AC_PR_RST		__BIT(18)
#define	 H3_AC_PR_ADDR		__BITS(20,16)
#define	 H3_ACDA_PR_WDAT	__BITS(15,8)
#define	 H3_ACDA_PR_RDAT	__BITS(7,0)

#define	H3_LOMIXSC		0x01
#define	 H3_LOMIXSC_LDAC	__BIT(1)
#define	H3_ROMIXSC		0x02
#define	 H3_ROMIXSC_RDAC	__BIT(1)
#define	H3_DAC_PA_SRC		0x03
#define	 H3_DACAREN		__BIT(7)
#define	 H3_DACALEN		__BIT(6)
#define	 H3_RMIXEN		__BIT(5)
#define	 H3_LMIXEN		__BIT(4)
#define	H3_LINEIN_GCTR		0x05
#define	 H3_LINEING		__BITS(6,4)
#define	H3_MIC_GCTR		0x06
#define	 H3_MIC1_GAIN		__BITS(6,4)
#define	 H3_MIC2_GAIN		__BITS(2,0)
#define	H3_PAEN_CTR		0x07
#define	 H3_LINEOUTEN		__BIT(7)
#define	H3_LINEOUT_VOLC		0x09
#define	 H3_LINEOUTVOL		__BITS(7,3)
#define	H3_MIC2G_LINEOUT_CTR	0x0a
#define	 H3_LINEOUT_LSEL	__BIT(3)
#define	 H3_LINEOUT_RSEL	__BIT(2)
#define	H3_LADCMIXSC		0x0c
#define	H3_RADCMIXSC		0x0d
#define	 H3_ADCMIXSC_MIC1	__BIT(6)
#define	 H3_ADCMIXSC_MIC2	__BIT(5)
#define	 H3_ADCMIXSC_LINEIN	__BIT(2)
#define	 H3_ADCMIXSC_OMIXER	__BITS(1,0)
#define	H3_ADC_AP_EN		0x0f
#define	 H3_ADCREN		__BIT(7)
#define	 H3_ADCLEN		__BIT(6)
#define	 H3_ADCG		__BITS(2,0)

struct h3_codec_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	int			sc_phandle;
};

enum h3_codec_mixer_ctrl {
	H3_CODEC_OUTPUT_CLASS,
	H3_CODEC_INPUT_CLASS,
	H3_CODEC_RECORD_CLASS,

	H3_CODEC_OUTPUT_MASTER_VOLUME,
	H3_CODEC_INPUT_DAC_VOLUME,
	H3_CODEC_INPUT_LINEIN_VOLUME,
	H3_CODEC_INPUT_MIC1_VOLUME,
	H3_CODEC_INPUT_MIC2_VOLUME,
	H3_CODEC_RECORD_AGC_VOLUME,
	H3_CODEC_RECORD_SOURCE,

	H3_CODEC_MIXER_CTRL_LAST
};

static const struct h3_codec_mixer {
	const char *			name;
	enum h3_codec_mixer_ctrl	mixer_class;
	u_int				reg;
	u_int				mask;
} h3_codec_mixers[H3_CODEC_MIXER_CTRL_LAST] = {
	[H3_CODEC_OUTPUT_MASTER_VOLUME]	= { AudioNmaster,
	    H3_CODEC_OUTPUT_CLASS, H3_LINEOUT_VOLC, H3_LINEOUTVOL },
	[H3_CODEC_INPUT_DAC_VOLUME]	= { AudioNdac,
	    H3_CODEC_INPUT_CLASS, H3_LINEOUT_VOLC, H3_LINEOUTVOL },
	[H3_CODEC_INPUT_LINEIN_VOLUME]	= { AudioNline,
	    H3_CODEC_INPUT_CLASS, H3_LINEIN_GCTR, H3_LINEING },
	[H3_CODEC_INPUT_MIC1_VOLUME]	= { "mic1",
	    H3_CODEC_INPUT_CLASS, H3_MIC_GCTR, H3_MIC1_GAIN },
	[H3_CODEC_INPUT_MIC2_VOLUME]	= { "mic2",
	    H3_CODEC_INPUT_CLASS, H3_MIC_GCTR, H3_MIC2_GAIN },
	[H3_CODEC_RECORD_AGC_VOLUME]	= { AudioNagc,
	    H3_CODEC_RECORD_CLASS, H3_ADC_AP_EN, H3_ADCG },
};

#define	RD4(sc, reg)			\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	WR4(sc, reg, val)		\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static struct h3_codec_softc *
h3_codec_find(int phandle)
{
	struct h3_codec_softc *csc;
	device_t dev;

	dev = device_find_by_driver_unit("h3codec", 0);
	if (dev == NULL)
		return NULL;
	csc = device_private(dev);
	if (csc->sc_phandle != phandle)
		return NULL;

	return csc;
}

static u_int
h3_codec_pr_read(struct h3_codec_softc *csc, u_int addr)
{
	uint32_t val;

	/* Read current value */
	val = RD4(csc, H3_PR_CFG);

	/* De-assert reset */
	val |= H3_AC_PR_RST;
	WR4(csc, H3_PR_CFG, val);

	/* Read mode */
	val &= ~H3_AC_PR_RW;
	WR4(csc, H3_PR_CFG, val);

	/* Set address */
	val &= ~H3_AC_PR_ADDR;
	val |= __SHIFTIN(addr, H3_AC_PR_ADDR);
	WR4(csc, H3_PR_CFG, val);

	/* Read data */
	return __SHIFTOUT(RD4(csc, H3_PR_CFG), H3_ACDA_PR_RDAT);
}

static void
h3_codec_pr_write(struct h3_codec_softc *csc, u_int addr, u_int data)
{
	uint32_t val;

	/* Read current value */
	val = RD4(csc, H3_PR_CFG);

	/* De-assert reset */
	val |= H3_AC_PR_RST;
	WR4(csc, H3_PR_CFG, val);

	/* Set address */
	val &= ~H3_AC_PR_ADDR;
	val |= __SHIFTIN(addr, H3_AC_PR_ADDR);
	WR4(csc, H3_PR_CFG, val);

	/* Write data */
	val &= ~H3_ACDA_PR_WDAT;
	val |= __SHIFTIN(data, H3_ACDA_PR_WDAT);
	WR4(csc, H3_PR_CFG, val);

	/* Write mode */
	val |= H3_AC_PR_RW;
	WR4(csc, H3_PR_CFG, val);

	/* Clear write mode */
	val &= ~H3_AC_PR_RW;
	WR4(csc, H3_PR_CFG, val);
}

static void
h3_codec_pr_set_clear(struct h3_codec_softc *csc, u_int addr, u_int set, u_int clr)
{
	u_int old, new;

	old = h3_codec_pr_read(csc, addr);
	new = set | (old & ~clr);
	h3_codec_pr_write(csc, addr, new);
}

static int
h3_codec_init(struct sunxi_codec_softc *sc)
{
	struct h3_codec_softc *csc;
	int phandle;

	/* Lookup the codec analog controls phandle */
	phandle = fdtbus_get_phandle(sc->sc_phandle,
	    "allwinner,codec-analog-controls");
	if (phandle < 0) {
		aprint_error_dev(sc->sc_dev,
		    "missing allwinner,codec-analog-controls property\n");
		return ENXIO;
	}

	/* Find a matching h3codec instance */
	sc->sc_codec_priv = h3_codec_find(phandle);
	if (sc->sc_codec_priv == NULL) {
		aprint_error_dev(sc->sc_dev, "couldn't find codec analog controls\n");
		return ENOENT;
	}
	csc = sc->sc_codec_priv;

	/* Right & Left LINEOUT enable */
	h3_codec_pr_set_clear(csc, H3_PAEN_CTR, H3_LINEOUTEN, 0);
	h3_codec_pr_set_clear(csc, H3_MIC2G_LINEOUT_CTR,
	    H3_LINEOUT_LSEL | H3_LINEOUT_RSEL, 0);

	return 0;
}

static void
h3_codec_mute(struct sunxi_codec_softc *sc, int mute, u_int mode)
{
	struct h3_codec_softc * const csc = sc->sc_codec_priv;

	if (mode == AUMODE_PLAY) {
		if (mute) {
			/* Mute DAC l/r channels to output mixer */
			h3_codec_pr_set_clear(csc, H3_LOMIXSC,
			    0, H3_LOMIXSC_LDAC);
			h3_codec_pr_set_clear(csc, H3_ROMIXSC,
			    0, H3_ROMIXSC_RDAC);
			/* Disable DAC analog l/r channels and output mixer */
			h3_codec_pr_set_clear(csc, H3_DAC_PA_SRC,
			    0, H3_DACAREN | H3_DACALEN | H3_RMIXEN | H3_LMIXEN);
		} else {
			/* Enable DAC analog l/r channels and output mixer */
			h3_codec_pr_set_clear(csc, H3_DAC_PA_SRC,
			    H3_DACAREN | H3_DACALEN | H3_RMIXEN | H3_LMIXEN, 0);
			/* Unmute DAC l/r channels to output mixer */
			h3_codec_pr_set_clear(csc, H3_LOMIXSC, H3_LOMIXSC_LDAC, 0);
			h3_codec_pr_set_clear(csc, H3_ROMIXSC, H3_ROMIXSC_RDAC, 0);
		}
	} else {
		if (mute) {
			/* Disable ADC analog l/r channels */
			h3_codec_pr_set_clear(csc, H3_ADC_AP_EN,
			    0, H3_ADCREN | H3_ADCLEN);
		} else {
			/* Enable ADC analog l/r channels */
			h3_codec_pr_set_clear(csc, H3_ADC_AP_EN,
			    H3_ADCREN | H3_ADCLEN, 0);
		}
	}
}

static int
h3_codec_set_port(struct sunxi_codec_softc *sc, mixer_ctrl_t *mc)
{
	struct h3_codec_softc * const csc = sc->sc_codec_priv;
	const struct h3_codec_mixer *mix;
	u_int val, shift;
	int nvol;

	switch (mc->dev) {
	case H3_CODEC_OUTPUT_MASTER_VOLUME:
	case H3_CODEC_INPUT_DAC_VOLUME:
	case H3_CODEC_INPUT_LINEIN_VOLUME:
	case H3_CODEC_INPUT_MIC1_VOLUME:
	case H3_CODEC_INPUT_MIC2_VOLUME:
	case H3_CODEC_RECORD_AGC_VOLUME:
		mix = &h3_codec_mixers[mc->dev];
		val = h3_codec_pr_read(csc, mix->reg);
		shift = 8 - fls32(__SHIFTOUT_MASK(mix->mask));
		nvol = mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT] >> shift;
		val &= ~mix->mask;
		val |= __SHIFTIN(nvol, mix->mask);
		h3_codec_pr_write(csc, mix->reg, val);
		return 0;

	case H3_CODEC_RECORD_SOURCE:
		h3_codec_pr_write(csc, H3_LADCMIXSC, mc->un.mask);
		h3_codec_pr_write(csc, H3_RADCMIXSC, mc->un.mask);
		return 0;
	}

	return ENXIO;
}

static int
h3_codec_get_port(struct sunxi_codec_softc *sc, mixer_ctrl_t *mc)
{
	struct h3_codec_softc * const csc = sc->sc_codec_priv;
	const struct h3_codec_mixer *mix;
	u_int val, shift;
	int nvol;

	switch (mc->dev) {
	case H3_CODEC_OUTPUT_MASTER_VOLUME:
	case H3_CODEC_INPUT_DAC_VOLUME:
	case H3_CODEC_INPUT_LINEIN_VOLUME:
	case H3_CODEC_INPUT_MIC1_VOLUME:
	case H3_CODEC_INPUT_MIC2_VOLUME:
	case H3_CODEC_RECORD_AGC_VOLUME:
		mix = &h3_codec_mixers[mc->dev];
		val = h3_codec_pr_read(csc, mix->reg);
		shift = 8 - fls32(__SHIFTOUT_MASK(mix->mask));
		nvol = __SHIFTOUT(val, mix->mask) << shift;
		mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT] = nvol;
		mc->un.value.level[AUDIO_MIXER_LEVEL_RIGHT] = nvol;
		return 0;

	case H3_CODEC_RECORD_SOURCE:
		mc->un.mask =
		    h3_codec_pr_read(csc, H3_LADCMIXSC) |
		    h3_codec_pr_read(csc, H3_RADCMIXSC);
		return 0;
	}

	return ENXIO;
}

static int
h3_codec_query_devinfo(struct sunxi_codec_softc *sc, mixer_devinfo_t *di)
{
	const struct h3_codec_mixer *mix;

	switch (di->index) {
	case H3_CODEC_OUTPUT_CLASS:
		di->mixer_class = di->index;
		strcpy(di->label.name, AudioCoutputs);
		di->type = AUDIO_MIXER_CLASS;
		di->next = di->prev = AUDIO_MIXER_LAST;
		return 0;

	case H3_CODEC_INPUT_CLASS:
		di->mixer_class = di->index;
		strcpy(di->label.name, AudioCinputs);
		di->type = AUDIO_MIXER_CLASS;
		di->next = di->prev = AUDIO_MIXER_LAST;
		return 0;

	case H3_CODEC_RECORD_CLASS:
		di->mixer_class = di->index;
		strcpy(di->label.name, AudioCrecord);
		di->type = AUDIO_MIXER_CLASS;
		di->next = di->prev = AUDIO_MIXER_LAST;
		return 0;

	case H3_CODEC_OUTPUT_MASTER_VOLUME:
	case H3_CODEC_INPUT_DAC_VOLUME:
	case H3_CODEC_INPUT_LINEIN_VOLUME:
	case H3_CODEC_INPUT_MIC1_VOLUME:
	case H3_CODEC_INPUT_MIC2_VOLUME:
	case H3_CODEC_RECORD_AGC_VOLUME:
		mix = &h3_codec_mixers[di->index];
		di->mixer_class = mix->mixer_class;
		strcpy(di->label.name, mix->name);
		di->un.v.delta =
		    256 / (__SHIFTOUT_MASK(mix->mask) + 1);
		di->type = AUDIO_MIXER_VALUE;
		di->next = di->prev = AUDIO_MIXER_LAST;
		di->un.v.num_channels = 2;
		strcpy(di->un.v.units.name, AudioNvolume);
		return 0;

	case H3_CODEC_RECORD_SOURCE:
		di->mixer_class = H3_CODEC_RECORD_CLASS;
		strcpy(di->label.name, AudioNsource);
		di->type = AUDIO_MIXER_SET;
		di->next = di->prev = AUDIO_MIXER_LAST;
		di->un.s.num_mem = 4;
		strcpy(di->un.s.member[0].label.name, AudioNline);
		di->un.s.member[0].mask = H3_ADCMIXSC_LINEIN;
		strcpy(di->un.s.member[1].label.name, "mic1");
		di->un.s.member[1].mask = H3_ADCMIXSC_MIC1;
		strcpy(di->un.s.member[2].label.name, "mic2");
		di->un.s.member[2].mask = H3_ADCMIXSC_MIC2;
		strcpy(di->un.s.member[3].label.name, AudioNdac);
		di->un.s.member[3].mask = H3_ADCMIXSC_OMIXER;
		return 0;

	}

	return ENXIO;
}

const struct sunxi_codec_conf sun8i_h3_codecconf = {
	.name = "H3 Audio Codec",

	.init = h3_codec_init,
	.mute = h3_codec_mute,
	.set_port = h3_codec_set_port,
	.get_port = h3_codec_get_port,
	.query_devinfo = h3_codec_query_devinfo,

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

static const char * compatible[] = {
	"allwinner,sun8i-h3-codec-analog",
	NULL
};

static int
h3_codec_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
h3_codec_attach(device_t parent, device_t self, void *aux)
{
	struct h3_codec_softc * const sc = device_private(self);
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
	aprint_normal(": H3 Audio Codec (analog part)\n");
}

CFATTACH_DECL_NEW(h3_codec, sizeof(struct h3_codec_softc),
    h3_codec_match, h3_codec_attach, NULL, NULL);
