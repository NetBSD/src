/* $NetBSD: bwai.c,v 1.3.2.2 2024/02/03 11:47:05 martin Exp $ */

/*-
 * Copyright (c) 2024 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: bwai.c,v 1.3.2.2 2024/02/03 11:47:05 martin Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/mutex.h>
#include <sys/bitops.h>

#include <dev/audio/audio_dai.h>

#include <machine/wii.h>

#include "mainbus.h"
#include "avenc.h"
#include "bwai.h"

#define	AI_CONTROL		0x00
#define	 AI_CONTROL_RATE	__BIT(6)
#define	 AI_CONTROL_SCRESET	__BIT(5)
#define	 AI_CONTROL_AIINTVLD	__BIT(4)
#define	 AI_CONTROL_AIINT	__BIT(3)
#define	 AI_CONTROL_AIINTMSK	__BIT(2)
#define	 AI_CONTROL_AFR		__BIT(1)
#define	 AI_CONTROL_PSTAT	__BIT(0)
#define AI_AIIT			0x0c

struct bwai_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	int			sc_irq;

	struct audio_dai_device	sc_dai;

	void			(*sc_intr)(void *);
	void			*sc_intrarg;

	kmutex_t		*sc_intr_lock;
};

enum bwai_mixer_ctrl {
	BWAI_OUTPUT_CLASS,
	BWAI_INPUT_CLASS,

	BWAI_OUTPUT_MASTER_VOLUME,
	BWAI_INPUT_DAC_VOLUME,

	BWAI_MIXER_CTRL_LAST
};

#define	RD4(sc, reg)			\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	WR4(sc, reg, val)		\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static int
bwai_intr(void *priv)
{
	struct bwai_softc * const sc = priv;
	uint32_t val;

	val = RD4(sc, AI_CONTROL);
	if ((val & AI_CONTROL_AIINT) != 0) {
		WR4(sc, AI_CONTROL, val | AI_CONTROL_SCRESET);

		mutex_enter(sc->sc_intr_lock);
		if (sc->sc_intr) {
			sc->sc_intr(sc->sc_intrarg);
		}
		mutex_exit(sc->sc_intr_lock);
	}

	return 1;
}

audio_dai_tag_t
bwai_dsp_init(kmutex_t *intr_lock)
{
	struct bwai_softc *sc;
	device_t dev;

	dev = device_find_by_driver_unit("bwai", 0);
	if (dev == NULL) {
		return NULL;
	}
	sc = device_private(dev);

	sc->sc_intr_lock = intr_lock;

	intr_establish_xname(sc->sc_irq, IST_LEVEL, IPL_AUDIO, bwai_intr, sc,
	    device_xname(dev));

	return &sc->sc_dai;
}

static int
bwai_set_port(void *priv, mixer_ctrl_t *mc)
{
	if (mc->dev != BWAI_OUTPUT_MASTER_VOLUME &&
	    mc->dev != BWAI_INPUT_DAC_VOLUME) {
		return ENXIO;
	}

	avenc_set_volume(mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT],
			 mc->un.value.level[AUDIO_MIXER_LEVEL_RIGHT]);

	return 0;
}

static int
bwai_get_port(void *priv, mixer_ctrl_t *mc)
{
	if (mc->dev != BWAI_OUTPUT_MASTER_VOLUME &&
	    mc->dev != BWAI_INPUT_DAC_VOLUME) {
		return ENXIO;
	}

	avenc_get_volume(&mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT],
			 &mc->un.value.level[AUDIO_MIXER_LEVEL_RIGHT]);

	return 0;
}

static int
bwai_query_devinfo(void *priv, mixer_devinfo_t *di)
{
	switch (di->index) {
	case BWAI_OUTPUT_CLASS:
		di->mixer_class = di->index;
		strcpy(di->label.name, AudioCoutputs);
		di->type = AUDIO_MIXER_CLASS;
		di->next = di->prev = AUDIO_MIXER_LAST;
		return 0;

	case BWAI_INPUT_CLASS:
		di->mixer_class = di->index;
		strcpy(di->label.name, AudioCinputs);
		di->type = AUDIO_MIXER_CLASS;
		di->next = di->prev = AUDIO_MIXER_LAST;
		return 0;

	case BWAI_OUTPUT_MASTER_VOLUME:
		di->mixer_class = BWAI_OUTPUT_CLASS;
		strcpy(di->label.name, AudioNmaster);
		di->un.v.delta = 1;
		di->type = AUDIO_MIXER_VALUE;
		di->next = di->prev = AUDIO_MIXER_LAST;
		di->un.v.num_channels = 2;
		strcpy(di->un.v.units.name, AudioNvolume);
		return 0;

	case BWAI_INPUT_DAC_VOLUME:
		di->mixer_class = BWAI_INPUT_CLASS;
		strcpy(di->label.name, AudioNdac);
		di->un.v.delta = 1;
		di->type = AUDIO_MIXER_VALUE;
		di->next = di->prev = AUDIO_MIXER_LAST;
		di->un.v.num_channels = 2;
		strcpy(di->un.v.units.name, AudioNvolume);
		return 0;
	}

	return ENXIO;
}

static int
bwai_set_format(void *priv, int setmode,
    const audio_params_t *play, const audio_params_t *rec,
    audio_filter_reg_t *pfil, audio_filter_reg_t *rfil)
{
	return 0;
}

static int
bwai_trigger_output(void *priv, void *start, void *end, int blksize,
    void (*intr)(void *), void *intrarg, const audio_params_t *params)
{
	struct bwai_softc * const sc = priv;
	uint32_t val;

	sc->sc_intr = intr;
	sc->sc_intrarg = intrarg;

	val = RD4(sc, AI_CONTROL);
	if ((val & AI_CONTROL_PSTAT) != 0) {
		WR4(sc, AI_CONTROL, 0);
	}

	WR4(sc, AI_AIIT, blksize / 4);

	val = AI_CONTROL_SCRESET |
	      AI_CONTROL_AIINT |
	      AI_CONTROL_AIINTMSK |
	      AI_CONTROL_AFR;
	WR4(sc, AI_CONTROL, val);

	val |= AI_CONTROL_PSTAT;
	WR4(sc, AI_CONTROL, val);

	return 0;
}

static int
bwai_halt_output(void *priv)
{
	struct bwai_softc * const sc = priv;

	WR4(sc, AI_CONTROL, 0);

	sc->sc_intr = NULL;
	sc->sc_intrarg = NULL;

	return 0;
}

static const struct audio_hw_if bwai_hw_if = {
	.set_port = bwai_set_port,
	.get_port = bwai_get_port,
	.query_devinfo = bwai_query_devinfo,
	.set_format = bwai_set_format,
	.trigger_output = bwai_trigger_output,
	.halt_output = bwai_halt_output,
};

static int
bwai_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args * const maa = aux;

	return strcmp(maa->maa_name, "bwai") == 0;
}

static void
bwai_attach(device_t parent, device_t self, void *aux)
{
	struct bwai_softc * const sc = device_private(self);
	struct mainbus_attach_args * const maa = aux;

	sc->sc_dev = self;
	sc->sc_bst = maa->maa_bst;
	if (bus_space_map(sc->sc_bst, maa->maa_addr, AI_SIZE, 0,
	    &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}
	sc->sc_irq = maa->maa_irq;

	aprint_naive("\n");
	aprint_normal(": Audio Interface\n");

	sc->sc_dai.dai_hw_if = &bwai_hw_if;
	sc->sc_dai.dai_dev = self;
	sc->sc_dai.dai_priv = sc;
}

CFATTACH_DECL_NEW(bwai, sizeof(struct bwai_softc),
    bwai_match, bwai_attach, NULL, NULL);
