/*	$NetBSD: zz9k_ax.c,v 1.1 2023/05/03 13:49:30 phx Exp $ */

/*
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Alain Runa.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *	notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	notice, this list of conditions and the following disclaimer in the
 *	documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: zz9k_ax.c,v 1.1 2023/05/03 13:49:30 phx Exp $");

/* miscellaneous */
#include <sys/types.h>			/* size_t */
#include <sys/stdint.h>			/* uintXX_t */
#include <sys/stdbool.h>		/* bool */

/* driver(9) */
#include <sys/param.h>			/* NODEV */
#include <sys/device.h>			/* CFATTACH_DECL_NEW(), device_priv() */
#include <sys/errno.h>			/* . */

/* bus_space(9) and zorro bus */
#include <sys/bus.h>			/* bus_space_xxx(), bus_space_xxx_t */
#include <sys/cpu.h>			/* kvtop() */
#include <sys/systm.h>			/* aprint_xxx() */

/* mutex(9) */
#include <sys/mutex.h>

/* Interrupt related */
#include <sys/intr.h>
#include <amiga/amiga/isr.h>		/* isr */

/* audio(9) */
#include <sys/audioio.h>
#include <dev/audio/audio_if.h>

/* zz9k related */
#include <amiga/dev/zz9kvar.h>		/* zz9kbus_attach_args */
#include <amiga/dev/zz9kreg.h>		/* ZZ9000 registers */
#include "zz9k_ax.h"			/* NZZ9K_AX */


/* The allmighty softc structure */
struct zzax_softc {
	device_t sc_dev;
	struct bus_space_tag sc_bst;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_regh;
	bus_space_handle_t sc_txbufh;
	size_t sc_txbufsize;

	struct isr sc_isr;

	kmutex_t  sc_lock;
	kmutex_t  sc_intr_lock;
};

static int zzax_intr(void *arg);
static void zzax_set_param(struct zzax_softc *sc, uint16_t param, uint16_t val);

/* audio_hw_if */
static int zzax_open(void *hdl, int flags);
static void zzax_close(void *hdl);
static int zzax_query_format(void *hdl, audio_format_query_t *afp);
static int zzax_set_format(void *hdl, int setmode,
    const audio_params_t *play, const audio_params_t *rec,
    audio_filter_reg_t *pfil, audio_filter_reg_t *rfil);
static int zzax_round_blocksize(void *hdl, int bs, int mode,
    const audio_params_t *param);
static int zzax_commit_settings(void *hdl);
static int zzax_init_output(void *hdl, void *buffer, int size);
static int zzax_init_input(void *hdl, void *buffer, int size);
static int zzax_start_output(void *hdl, void *block, int blksize,
    void (*intr)(void*), void *intrarg);
static int zzax_start_input(void *hdl, void *block, int blksize,
    void (*intr)(void*), void *intrarg);
static int zzax_halt_output(void *hdl);
static int zzax_halt_input(void *hdl);
static int zzax_speaker_ctl(void *hdl, int on);
static int zzax_getdev(void *hdl, struct audio_device *ret);
static int zzax_set_port(void *hdl, mixer_ctrl_t *mc);
static int zzax_get_port(void *hdl, mixer_ctrl_t *mc);
static int zzax_query_devinfo(void *hdl, mixer_devinfo_t *di);
#if 0
static void *zzax_allocm(void *hdl, int direction, size_t size);
static void zzax_freem(void *hdl, void *addr, size_t size);
#endif
static size_t zzax_round_buffersize(void *hdl, int direction, size_t bufsize);
static int zzax_get_props(void *hdl);
static int zzax_trigger_output(void *hdl, void *start, void *end, int blksize,
    void (*intr)(void*), void *intrarg, const audio_params_t *param);
static int zzax_trigger_input(void *hdl, void *start, void *end, int blksize,
    void (*intr)(void*), void *intrarg, const audio_params_t *param);
static int zzax_dev_ioctl(void *hdl, u_long cmd, void *addr, int flag,
    struct lwp *l);
static void zzax_get_locks(void *hdl, kmutex_t **intr, kmutex_t **thread);

static const struct audio_hw_if zzax_hw_if = {
	.open			= zzax_open,
	.close			= zzax_close,
	.query_format		= zzax_query_format,
	.set_format		= zzax_set_format,
	.round_blocksize	= zzax_round_blocksize,
	.commit_settings	= zzax_commit_settings,
	.init_output		= zzax_init_output,
	.init_input		= zzax_init_input,
	.start_output		= zzax_start_output,
	.start_input		= zzax_start_input,
	.halt_output		= zzax_halt_output,
	.halt_input		= zzax_halt_input,
	.speaker_ctl		= zzax_speaker_ctl,
	.getdev			= zzax_getdev,
	.set_port		= zzax_set_port,
	.get_port		= zzax_get_port,
	.query_devinfo		= zzax_query_devinfo,
#if 0
	.allocm			= zzax_allocm,
	.freem			= zzax_freem,
#endif
	.round_buffersize	= zzax_round_buffersize,
	.get_props		= zzax_get_props,
	.trigger_output		= zzax_trigger_output,
	.trigger_input		= zzax_trigger_input,
	.dev_ioctl		= zzax_dev_ioctl,
	.get_locks		= zzax_get_locks
};

static const struct audio_format zzax_format = {
	.mode		= AUMODE_PLAY,
	.encoding	= AUDIO_ENCODING_SLINEAR_BE,
	.validbits	= 16,
	.precision	= 16,
	.channels	= 2,
	.channel_mask	= AUFMT_STEREO,
	.frequency_type	= 0,
	.frequency	= {8000, 12000, 24000, 32000, 44100, 48000},
	.priority	= 0
};

static const struct audio_device zzax_device = {
	.name		= "ZZ9000AX",
	.version	= "1.13",
	.config		= "zzax"
};

/* mixer sets */
#define ZZAX_CHANNELS 0

/* mixer values */
#define ZZAX_VOLUME 1
#define ZZAX_OUTPUT_CLASS 2


/* driver(9) essentials */
static int zzax_match(device_t parent, cfdata_t match, void *aux);
static void zzax_attach(device_t parent, device_t self, void *aux);
CFATTACH_DECL_NEW(zz9k_ax, sizeof(struct zzax_softc),
    zzax_match, zzax_attach, NULL, NULL);


/* Go ahead, make my day. */

static int
zzax_match(device_t parent, cfdata_t match, void *aux)
{
	struct zz9kbus_attach_args *bap = aux;

	if (strcmp(bap->zzaa_name, "zz9k_ax") != 0)
		return 0;

	return 1;
}

static void
zzax_attach(device_t parent, device_t self, void *aux)
{
	struct zz9kbus_attach_args *bap = aux;
	struct zzax_softc *sc = device_private(self);
	struct zz9k_softc *psc = device_private(parent);

	sc->sc_dev = self;
	sc->sc_bst.base = bap->zzaa_base;
	sc->sc_bst.absm = &amiga_bus_stride_1;
	sc->sc_iot = &sc->sc_bst;
	sc->sc_regh = psc->sc_regh;

	uint16_t config = ZZREG_R(ZZ9K_AUDIO_CONFIG);
	aprint_normal(": ZZ9000AX %sdetected.\n",
	    (config == 0) ? "not " : "");
	aprint_normal_dev(sc->sc_dev,
	    "MNT ZZ9000AX driver is not functional yet.\n");

	aprint_debug_dev(sc->sc_dev, "[DEBUG] registers at %p/%p (pa/va)\n",
	    (void *)kvtop((void *)sc->sc_regh),
	    bus_space_vaddr(sc->sc_iot, sc->sc_regh));

	sc->sc_txbufsize = 0x10000;
	uint32_t tx_buffer = psc->sc_zsize - sc->sc_txbufsize;
	if (bus_space_map(sc->sc_iot, tx_buffer, sc->sc_txbufsize,
	    BUS_SPACE_MAP_LINEAR, &sc->sc_txbufh)) {
		aprint_error(": Failed to map MNT ZZ9000AX audio tx buffer.\n");
		return;
	}

	aprint_normal_dev(sc->sc_dev, "base: %p/%p size: %i\n",
	    (void *)kvtop((void *)sc->sc_txbufh),
	    bus_space_vaddr(sc->sc_iot, sc->sc_txbufh),
	    sc->sc_txbufsize);

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_SCHED);

	sc->sc_isr.isr_intr = zzax_intr;
	sc->sc_isr.isr_arg  = sc;
	sc->sc_isr.isr_ipl  = 6;
	add_isr(&sc->sc_isr);

	audio_attach_mi(&zzax_hw_if, sc, self);

	uint16_t conf = ZZREG_R(ZZ9K_AUDIO_CONFIG); /* enable interrupt */
	ZZREG_W(ZZ9K_AUDIO_CONFIG, conf | ZZ9K_AUDIO_CONFIG_INT_AUDIO);
}

static int
zzax_intr(void *arg)
{
	struct zzax_softc *sc = arg;

	uint16_t conf = ZZREG_R(ZZ9K_CONFIG);
	if ((conf & ZZ9K_CONFIG_INT_AUDIO) == 0)
		return 0;
	
#if 0
	uint16_t conf = ZZREG_R(ZZ9K_AUDIO_CONFIG); /* disable interrupt */
	ZZREG_W(ZZ9K_AUDIO_CONFIG, conf & ~ZZ9K_AUDIO_CONFIG_INT_AUDIO);
#endif
	ZZREG_W(ZZ9K_CONFIG, ZZ9K_CONFIG_INT_ACK | ZZ9K_CONFIG_INT_ACK_AUDIO);

	mutex_spin_enter(&sc->sc_intr_lock);
	/* do stuff */
	mutex_spin_exit(&sc->sc_intr_lock);

#if 0
	uint16_t conf = ZZREG_R(ZZ9K_AUDIO_CONFIG); /* enable interrupt */
	ZZREG_W(ZZ9K_AUDIO_CONFIG, conf | ZZ9K_AUDIO_CONFIG_INT_AUDIO);
#endif

	return 1;
}

static void
zzax_set_param(struct zzax_softc *sc, uint16_t param, uint16_t val)
{
	ZZREG_W(ZZ9K_AUDIO_PARAM, param);
	ZZREG_W(ZZ9K_AUDIO_VAL, val);
	ZZREG_W(ZZ9K_AUDIO_PARAM, 0);
}

int
zzax_open(void *hdl, int flags)
{
	struct zzax_softc *sc = hdl;
	uint32_t buffer = (uint32_t)bus_space_vaddr(sc->sc_iot, sc->sc_txbufh);
	zzax_set_param(sc, ZZ9K_AP_TX_BUF_OFFS_HI, buffer >> 16);
	zzax_set_param(sc, ZZ9K_AP_TX_BUF_OFFS_HI, buffer & 0xFFFF);
	printf("zzax_open: %X\n", buffer);
	return 0;
}

static void
zzax_close(void *hdl)
{
	printf("zzax_close:\n");
}

static int
zzax_query_format(void *hdl, audio_format_query_t *afp)
{
	printf("zzax_query_format:\n");
	return audio_query_format(&zzax_format, 1, afp);
}

static int
zzax_set_format(void *hdl, int setmode,
    const audio_params_t *play, const audio_params_t *rec,
    audio_filter_reg_t *pfil, audio_filter_reg_t *rfil)
{
	struct zzax_softc *sc = hdl;

	printf("zzax_set_format:\n");
	if (setmode & AUMODE_PLAY) {
		zzax_set_param(sc, ZZ9K_AP_DSP_SET_LOWPASS, 23900);
		printf("::play->sample_rate: %i\n", play->sample_rate);
		printf("::play->encoding: %i\n", play->encoding);
		printf("::play->precision: %i\n", play->precision);
		printf("::play->validbits: %i\n", play->validbits);
		printf("::play->channels: %i\n", play->channels);
	}

	if (setmode & AUMODE_RECORD) {
		printf("::rec->sample_rate: %i\n", rec->sample_rate);
		printf("::rec->encoding: %i\n", rec->encoding);
		printf("::rec->precision: %i\n", rec->precision);
		printf("::rec->validbits: %i\n", rec->validbits);
		printf("::rec->channels: %i\n", rec->channels);
	}

	return 0;
}

static int
zzax_round_blocksize(void *hdl, int bs, int mode, const audio_params_t *param)
{
	printf("zzax_round_blocksize:\n");
	printf("::bs: %i\n", bs);
	printf("::mode: %i\n", mode);
	printf("::param->sample_rate: %i\n", param->sample_rate);
	printf("::param->encoding: %i\n", param->encoding);
	printf("::param->precision: %i\n", param->precision);
	printf("::param->validbits: %i\n", param->validbits);
	printf("::param->channels: %i\n", param->channels);

	return bs;
}

static int
zzax_commit_settings(void *hdl)
{
	printf("zzax_commit_settings:\n");
	return 0;
}

static int
zzax_init_output(void *hdl, void *buffer, int size)
{
	printf("zzax_init_output:\n");
	return 0;
}

static int
zzax_init_input(void *hdl, void *buffer, int size)
{
	printf("zzax_init_input:\n");
	return 0;
}

static int
zzax_start_output(void *hdl, void *block, int blksize,
    void (*intr)(void*), void *intrarg)
{
	printf("zzax_start_output:\n");
	return 0;
}

static int
zzax_start_input(void *hdl, void *block, int blksize,
    void (*intr)(void*), void *intrarg)
{
	printf("zzax_start_input:\n");
	return ENXIO;
}

static int
zzax_halt_output(void *hdl)
{
	printf("zzax_halt_output:\n");
	return 0;
}

static int
zzax_halt_input(void *hdl)
{
	printf("zzax_halt_input:\n");
	return ENXIO;
}

static int
zzax_speaker_ctl(void *hdl, int on)
{
	printf("zzax_speaker_ctl:\n");
	return 0;
}

static int
zzax_getdev(void *hdl, struct audio_device *ret)
{
	*ret = zzax_device;
	printf("zzax_getdev: %p\n", ret);
	return 0;
}

static int
zzax_set_port(void *hdl, mixer_ctrl_t *mc)
{
	printf("zzax_set_port:\n");
	return 0;
}

static int
zzax_get_port(void *hdl, mixer_ctrl_t *mc)
{
	printf("zzax_get_port:\n");
	return 0;
}

static int
zzax_query_devinfo(void *hdl, mixer_devinfo_t *di)
{
	switch(di->index) {
	case ZZAX_CHANNELS:
		strcpy(di->label.name, "speaker");
		di->type = AUDIO_MIXER_SET;
		di->mixer_class = ZZAX_OUTPUT_CLASS;
		di->prev = AUDIO_MIXER_LAST;
		di->next = AUDIO_MIXER_LAST;
		di->un.s.num_mem = 1;
		strcpy(di->un.s.member[0].label.name, "channel0");
		di->un.s.member[0].mask = 0;
	case ZZAX_VOLUME:
		strcpy(di->label.name, "master");
		di->type = AUDIO_MIXER_VALUE;
		di->mixer_class = ZZAX_OUTPUT_CLASS;
		di->prev = AUDIO_MIXER_LAST;
		di->next = AUDIO_MIXER_LAST;
		di->un.v.num_channels = 1;
		strcpy(di->un.v.units.name, "volume");
		break;
	case ZZAX_OUTPUT_CLASS:
		strcpy(di->label.name, "outputs");
		di->type = AUDIO_MIXER_CLASS;
		di->mixer_class = ZZAX_OUTPUT_CLASS;
		di->prev = AUDIO_MIXER_LAST;
		di->next = AUDIO_MIXER_LAST;
		break;
	default:
		return ENXIO; 
	}

	printf("zzax_query_devinfo: %s\n", di->label.name);

	return 0;
}

#if 0
static void*
zzax_allocm(void *hdl, int direction, size_t size)
{

}

static void
zzax_freem(void *hdl, void *addr, size_t size)
{

}
#endif

static size_t
zzax_round_buffersize(void *hdl, int direction, size_t bufsize)
{
	printf("zzax_round_buffersize:\n");
	printf("::direction: %i\n", direction);
	printf("::bufsize: %zu\n", bufsize);
	return bufsize;
}

static int
zzax_get_props(void *hdl)
{
	return AUDIO_PROP_PLAYBACK;
}

static int
zzax_trigger_output(void *hdl, void *start, void *end, int blksize,
    void (*intr)(void*), void *intrarg, const audio_params_t *param)
{
	printf("zzax_trigger_output:\n");
	return 0;
}

static int
zzax_trigger_input(void *hdl, void *start, void *end, int blksize,
    void (*intr)(void*), void *intrarg, const audio_params_t *param)
{
	return 0;
}

static int
zzax_dev_ioctl(void *hdl, u_long cmd, void *addr, int flag, struct lwp *l)
{
	printf("zzax_dev_ioctl: %lu\n", cmd);
	return 0;
}

static void
zzax_get_locks(void *hdl, kmutex_t **intr, kmutex_t **thread)
{
	struct zzax_softc *sc = hdl;
	*intr = &sc->sc_intr_lock;
	*thread = &sc->sc_lock;
}
