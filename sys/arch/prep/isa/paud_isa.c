/*	$NetBSD: paud_isa.c,v 1.14.2.1 2012/04/17 00:06:49 yamt Exp $	*/

/*-
 * Copyright (c) 1998, 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: paud_isa.c,v 1.14.2.1 2012/04/17 00:06:49 yamt Exp $");

#include "audio.h"
#if NAUDIO > 0

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/kmem.h>

#include <sys/bus.h>
#include <machine/intr.h>

#include <sys/audioio.h>
#include <dev/audio_if.h>

#include <dev/isa/isavar.h>
#include <dev/isa/isadmavar.h>

#include <dev/ic/ad1848reg.h>
#include <dev/ic/cs4231reg.h>

#include <dev/isa/ad1848var.h>
#include <dev/isa/cs4231var.h>

#define	PAUD_MIC_IN_LVL		0
#define	PAUD_LINE_IN_LVL	1
#define	PAUD_DAC_LVL		2
#define	PAUD_REC_LVL		3
#define	PAUD_MONITOR_LVL	4
#define	PAUD_MIC_IN_MUTE	5
#define	PAUD_LINE_IN_MUTE	6
#define	PAUD_DAC_MUTE		7
#define	PAUD_MONITOR_MUTE	8
#define	PAUD_RECORD_SOURCE	9
#define	PAUD_INPUT_CLASS	10
#define	PAUD_RECORD_CLASS	11
#define	PAUD_MONITOR_CLASS	12


/* autoconfiguration driver */
static void paud_attach_isa(device_t, device_t, void *);
static int  paud_match_isa(device_t, cfdata_t, void *);

CFATTACH_DECL_NEW(paud_isa, sizeof(struct ad1848_isa_softc),
    paud_match_isa, paud_attach_isa, NULL, NULL);

/*
 * Define our interface to the higher level audio driver.
 */
static struct audio_device paud_device = {
	"paud,cs4232",
	"",
	""
};

static int paud_intr(void *);
static int paud_getdev(void *, struct audio_device *);
static int paud_mixer_set_port(void *, mixer_ctrl_t *);
static int paud_mixer_get_port(void *, mixer_ctrl_t *);
static int paud_query_devinfo(void *, mixer_devinfo_t *);

static const struct audio_hw_if paud_hw_if = {
	ad1848_isa_open,
	ad1848_isa_close,
	NULL,
	ad1848_query_encoding,
	ad1848_set_params,
	ad1848_round_blocksize,
	ad1848_commit_settings,
	NULL,
	NULL,
	NULL,
	NULL,
	ad1848_isa_halt_output,
	ad1848_isa_halt_input,
	NULL,
	paud_getdev,
	NULL,
	paud_mixer_set_port,
	paud_mixer_get_port,
	paud_query_devinfo,
	ad1848_isa_malloc,
	ad1848_isa_free,
	ad1848_isa_round_buffersize,
	ad1848_isa_mappage,
	ad1848_isa_get_props,
	ad1848_isa_trigger_output,
	ad1848_isa_trigger_input,
	NULL,
	ad1848_get_locks,
};

/* autoconfig routines */

static int
paud_match_isa(device_t parent, cfdata_t cf, void *aux)
{
	struct ad1848_isa_softc probesc, *sc = &probesc;
	struct isa_attach_args *ia = aux;

	if (ia->ia_io[0].ir_addr != 0x830)
		return 0;

	sc->sc_ad1848.sc_iot = ia->ia_iot;
	sc->sc_ic = ia->ia_ic;
	if (ad1848_isa_mapprobe(sc, ia->ia_io[0].ir_addr)) {
		ia->ia_io[0].ir_size = AD1848_NPORT;
		ad1848_isa_unmap(sc);
		return 1;
	}
	return 0;
}

/*
 * Audio chip found.
 */
static void
paud_attach_isa(device_t parent, device_t self, void *aux)
{
	struct ad1848_isa_softc *sc;
	struct isa_attach_args *ia;

	sc = device_private(self);
	sc->sc_ad1848.sc_dev = self;
	ia = aux;
	sc->sc_ad1848.sc_iot = ia->ia_iot;
	sc->sc_ic = ia->ia_ic;

	mutex_init(&sc->sc_ad1848.sc_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->sc_ad1848.sc_intr_lock, MUTEX_DEFAULT, IPL_AUDIO);

	if (ad1848_isa_mapprobe(sc, ia->ia_io[0].ir_addr) == 0) {
		aprint_error(": attach failed\n");
		return;
	}
	sc->sc_playdrq = ia->ia_drq[0].ir_drq;
	sc->sc_recdrq = ia->ia_drq[1].ir_drq;
	sc->sc_ih = isa_intr_establish(ia->ia_ic, ia->ia_irq[0].ir_irq,
	    IST_EDGE, IPL_AUDIO, paud_intr, sc);
	ad1848_isa_attach(sc);
	aprint_normal("\n");
	audio_attach_mi(&paud_hw_if, &sc->sc_ad1848, self);

}

static int
paud_intr(void *addr)
{
	struct ad1848_isa_softc *sc = addr;
	int ret;

	mutex_spin_enter(&sc->sc_ad1848.sc_intr_lock);
	ret = ad1848_isa_intr(sc);
	mutex_spin_exit(&sc->sc_ad1848.sc_intr_lock);

	return ret;
}

static int
paud_getdev(void *addr, struct audio_device *retp)
{

	*retp = paud_device;
	return 0;
}

static ad1848_devmap_t mappings[] = {
	{ PAUD_MIC_IN_LVL, AD1848_KIND_LVL, AD1848_AUX2_CHANNEL },
	{ PAUD_LINE_IN_LVL, AD1848_KIND_LVL, AD1848_AUX1_CHANNEL },
	{ PAUD_DAC_LVL, AD1848_KIND_LVL, AD1848_DAC_CHANNEL },
	{ PAUD_MONITOR_LVL, AD1848_KIND_LVL, AD1848_MONO_CHANNEL },
	{ PAUD_MIC_IN_MUTE, AD1848_KIND_MUTE, AD1848_AUX2_CHANNEL },
	{ PAUD_LINE_IN_MUTE, AD1848_KIND_MUTE, AD1848_AUX1_CHANNEL },
	{ PAUD_DAC_MUTE, AD1848_KIND_MUTE, AD1848_DAC_CHANNEL },
	{ PAUD_MONITOR_MUTE, AD1848_KIND_MUTE, AD1848_MONO_CHANNEL },
	{ PAUD_REC_LVL, AD1848_KIND_RECORDGAIN, -1 },
	{ PAUD_RECORD_SOURCE, AD1848_KIND_RECORDSOURCE, -1}
};

static const int nummap = sizeof(mappings) / sizeof(mappings[0]);

static int
paud_mixer_set_port(void *addr, mixer_ctrl_t *cp)
{
	struct ad1848_softc *ac;

	ac = addr;
	return ad1848_mixer_set_port(ac, mappings, nummap, cp);
}

static int
paud_mixer_get_port(void *addr, mixer_ctrl_t *cp)
{
	struct ad1848_softc *ac;

	ac = addr;
	return ad1848_mixer_get_port(ac, mappings, nummap, cp);
}

static int
paud_query_devinfo(void *addr, mixer_devinfo_t *dip)
{

	switch(dip->index) {
	case PAUD_MIC_IN_LVL:	/* Microphone */
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = PAUD_INPUT_CLASS;
		dip->prev = AUDIO_MIXER_LAST;
		dip->next = PAUD_MIC_IN_MUTE;
		strcpy(dip->label.name, AudioNmicrophone);
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		break;

	case PAUD_LINE_IN_LVL:	/* line/CD */
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = PAUD_INPUT_CLASS;
		dip->prev = AUDIO_MIXER_LAST;
		dip->next = PAUD_LINE_IN_MUTE;
		strcpy(dip->label.name, AudioNcd);
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		break;

	case PAUD_DAC_LVL:		/*  dacout */
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = PAUD_INPUT_CLASS;
		dip->prev = AUDIO_MIXER_LAST;
		dip->next = PAUD_DAC_MUTE;
		strcpy(dip->label.name, AudioNdac);
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		break;

	case PAUD_REC_LVL:	/* record level */
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = PAUD_RECORD_CLASS;
		dip->prev = AUDIO_MIXER_LAST;
		dip->next = PAUD_RECORD_SOURCE;
		strcpy(dip->label.name, AudioNrecord);
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		break;

	case PAUD_MONITOR_LVL:	/* monitor level */
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = PAUD_MONITOR_CLASS;
		dip->prev = AUDIO_MIXER_LAST;
		dip->next = PAUD_MONITOR_MUTE;
		strcpy(dip->label.name, AudioNmonitor);
		dip->un.v.num_channels = 1;
		strcpy(dip->un.v.units.name, AudioNvolume);
		break;

	case PAUD_INPUT_CLASS:			/* input class descriptor */
		dip->type = AUDIO_MIXER_CLASS;
		dip->mixer_class = PAUD_INPUT_CLASS;
		dip->next = dip->prev = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioCinputs);
		break;

	case PAUD_MONITOR_CLASS:			/* monitor class descriptor */
		dip->type = AUDIO_MIXER_CLASS;
		dip->mixer_class = PAUD_MONITOR_CLASS;
		dip->next = dip->prev = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioCmonitor);
		break;

	case PAUD_RECORD_CLASS:			/* record source class */
		dip->type = AUDIO_MIXER_CLASS;
		dip->mixer_class = PAUD_RECORD_CLASS;
		dip->next = dip->prev = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioCrecord);
		break;

	case PAUD_MIC_IN_MUTE:
		dip->mixer_class = PAUD_INPUT_CLASS;
		dip->type = AUDIO_MIXER_ENUM;
		dip->prev = PAUD_MIC_IN_LVL;
		dip->next = AUDIO_MIXER_LAST;
		goto mute;

	case PAUD_LINE_IN_MUTE:
		dip->mixer_class = PAUD_INPUT_CLASS;
		dip->type = AUDIO_MIXER_ENUM;
		dip->prev = PAUD_LINE_IN_LVL;
		dip->next = AUDIO_MIXER_LAST;
		goto mute;

	case PAUD_DAC_MUTE:
		dip->mixer_class = PAUD_INPUT_CLASS;
		dip->type = AUDIO_MIXER_ENUM;
		dip->prev = PAUD_DAC_LVL;
		dip->next = AUDIO_MIXER_LAST;
		goto mute;

	case PAUD_MONITOR_MUTE:
		dip->mixer_class = PAUD_MONITOR_CLASS;
		dip->type = AUDIO_MIXER_ENUM;
		dip->prev = PAUD_MONITOR_LVL;
		dip->next = AUDIO_MIXER_LAST;
	mute:
		strcpy(dip->label.name, AudioNmute);
		dip->un.e.num_mem = 2;
		strcpy(dip->un.e.member[0].label.name, AudioNoff);
		dip->un.e.member[0].ord = 0;
		strcpy(dip->un.e.member[1].label.name, AudioNon);
		dip->un.e.member[1].ord = 1;
		break;

	case PAUD_RECORD_SOURCE:
		dip->mixer_class = PAUD_RECORD_CLASS;
		dip->type = AUDIO_MIXER_ENUM;
		dip->prev = PAUD_REC_LVL;
		dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioNsource);
		dip->un.e.num_mem = 3;
		strcpy(dip->un.e.member[0].label.name, AudioNmicrophone);
		dip->un.e.member[0].ord = PAUD_MIC_IN_LVL;
		strcpy(dip->un.e.member[1].label.name, AudioNcd);
		dip->un.e.member[1].ord = PAUD_LINE_IN_LVL;
		strcpy(dip->un.e.member[2].label.name, AudioNdac);
		dip->un.e.member[2].ord = PAUD_DAC_LVL;
		break;

	default:
		return ENXIO;
		/*NOTREACHED*/
	}

	return 0;
}

#endif
