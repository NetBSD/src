/*	$NetBSD: ym.c,v 1.15 2000/07/04 10:02:45 augustss Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by ITOH Yasufumi.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*
 * Copyright (c) 1998 Constantine Sapuntzakis. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

/*
 *  Original code from OpenBSD.
 */

#include "mpu_ym.h"
#include "opt_ym.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/fcntl.h>
#include <sys/kernel.h>
#include <sys/proc.h>

#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/bus.h>

#include <sys/audioio.h>
#include <dev/audio_if.h>

#include <dev/isa/isavar.h>
#include <dev/isa/isadmavar.h>

#include <dev/ic/ad1848reg.h>
#include <dev/isa/ad1848var.h>
#include <dev/ic/opl3sa3reg.h>
#include <dev/isa/wssreg.h>
#if NMPU_YM > 0
#include <dev/ic/mpuvar.h>
#endif
#include <dev/isa/ymvar.h>
#include <dev/isa/sbreg.h>

#ifndef spllowersoftclock
 #error "We depend on the new semantics of splsoftclock(9)."
#endif

/* Power management mode. */
#ifndef YM_POWER_MODE
#define YM_POWER_MODE		YM_POWER_POWERSAVE
#endif

/* Time in second before power down the chip. */
#ifndef YM_POWER_OFF_SEC
#define YM_POWER_OFF_SEC	5
#endif

/* Default mixer settings. */
#ifndef YM_VOL_MASTER
#define YM_VOL_MASTER		220
#endif

#ifndef YM_VOL_DAC
#define YM_VOL_DAC		224
#endif

#ifndef YM_VOL_OPL3
#define YM_VOL_OPL3		184
#endif

#ifndef YM_VOL_EQUAL
#define YM_VOL_EQUAL		128
#endif

#ifdef __i386__		/* XXX */
# include "joy.h"
#else
# define NJOY	0
#endif

#ifdef AUDIO_DEBUG
#define DPRINTF(x)	if (ymdebug) printf x
int	ymdebug = 0;
#else
#define DPRINTF(x)
#endif
#define DVNAME(softc)	((softc)->sc_ad1848.sc_ad1848.sc_dev.dv_xname)

int	ym_getdev __P((void *, struct audio_device *));
int	ym_mixer_set_port __P((void *, mixer_ctrl_t *));
int	ym_mixer_get_port __P((void *, mixer_ctrl_t *));
int	ym_query_devinfo __P((void *, mixer_devinfo_t *));
int	ym_intr __P((void *));
#ifndef AUDIO_NO_POWER_CTL
static void ym_save_codec_regs __P((struct ym_softc *));
static void ym_restore_codec_regs __P((struct ym_softc *));
void	ym_power_hook __P((int, void *));
int	ym_codec_power_ctl __P((void *, int));
static void ym_chip_powerdown __P((struct ym_softc *));
static void ym_chip_powerup __P((struct ym_softc *, int));
void ym_powerdown_blocks __P((void *));
void ym_power_ctl __P((struct ym_softc *, int, int));
#endif

static void ym_init __P((struct ym_softc *));
static void ym_mute __P((struct ym_softc *, int, int));
static void ym_set_master_gain __P((struct ym_softc *, struct ad1848_volume*));
static void ym_set_mic_gain __P((struct ym_softc *, int));
static void ym_set_3d __P((struct ym_softc *, mixer_ctrl_t *,
	struct ad1848_volume *, int));


struct audio_hw_if ym_hw_if = {
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
	ym_getdev,
	NULL,
	ym_mixer_set_port,
	ym_mixer_get_port,
	ym_query_devinfo,
	ad1848_isa_malloc,
	ad1848_isa_free,
	ad1848_isa_round_buffersize,
	ad1848_isa_mappage,
	ad1848_isa_get_props,
	ad1848_isa_trigger_output,
	ad1848_isa_trigger_input,
};

static __inline int ym_read __P((struct ym_softc *, int));
static __inline void ym_write __P((struct ym_softc *, int, int));

void
ym_attach(sc)
	struct ym_softc *sc;
{
	struct ad1848_softc *ac = &sc->sc_ad1848.sc_ad1848;
	static struct ad1848_volume vol_master = {YM_VOL_MASTER, YM_VOL_MASTER};
	static struct ad1848_volume vol_dac    = {YM_VOL_DAC,    YM_VOL_DAC};
	static struct ad1848_volume vol_opl3   = {YM_VOL_OPL3,   YM_VOL_OPL3};
	mixer_ctrl_t mctl;
	struct audio_attach_args arg;

	callout_init(&sc->sc_powerdown_ch);

	/* Mute the output to reduce noise during initialization. */
	ym_mute(sc, SA3_VOL_L, 1);
	ym_mute(sc, SA3_VOL_R, 1);

	sc->sc_ad1848.sc_ih = isa_intr_establish(sc->sc_ic, sc->ym_irq,
						 IST_EDGE, IPL_AUDIO,
						 ym_intr, sc);

#ifndef AUDIO_NO_POWER_CTL
	sc->sc_ad1848.powerctl = ym_codec_power_ctl;
	sc->sc_ad1848.powerarg = sc;
#endif
	ad1848_isa_attach(&sc->sc_ad1848);
	printf("\n");
	ac->parent = sc;

	/* Establish chip in well known mode */
	ym_set_master_gain(sc, &vol_master);
	ym_set_mic_gain(sc, 0);
	sc->master_mute = 0;

	sc->mic_mute = 1;
	ym_mute(sc, SA3_MIC_VOL, sc->mic_mute);

	/* Override ad1848 settings. */
	ad1848_set_channel_gain(ac, AD1848_DAC_CHANNEL, &vol_dac);
	ad1848_set_channel_gain(ac, AD1848_AUX2_CHANNEL, &vol_opl3);

	/* Set tone control to middle position. */
	mctl.un.value.num_channels = 1;
	mctl.un.value.level[AUDIO_MIXER_LEVEL_MONO] = YM_VOL_EQUAL;
	mctl.dev = YM_MASTER_BASS;
	ym_mixer_set_port(sc, &mctl);
	mctl.dev = YM_MASTER_TREBLE;
	ym_mixer_set_port(sc, &mctl);

	/*
	 * Mute all external sources.  If you change this, you must
	 * also change the initial value of sc->sc_external_sources
	 * (currently 0 --- no external source is active).
	 */
	ad1848_mute_channel(ac, AD1848_AUX1_CHANNEL, MUTE_ALL);	/* CD */
	ad1848_mute_channel(ac, AD1848_LINE_CHANNEL, MUTE_ALL);	/* line */
	ac->mute[AD1848_AUX1_CHANNEL] = MUTE_ALL;
	ac->mute[AD1848_LINE_CHANNEL] = MUTE_ALL;
	/* speaker is muted by default */

	sc->sc_version = ym_read(sc, SA3_MISC) & SA3_MISC_VER;

	/* We use only one IRQ (IRQ-A). */
	ym_write(sc, SA3_IRQ_CONF, SA3_IRQ_CONF_MPU_A | SA3_IRQ_CONF_WSS_A);
	ym_write(sc, SA3_HVOL_INTR_CNF, SA3_HVOL_INTR_CNF_A);

	/* audio at ym attachment */
	sc->sc_audiodev = audio_attach_mi(&ym_hw_if, ac, &ac->sc_dev);

	/* opl at ym attachment */
	if (sc->sc_opl_ioh) {
		arg.type = AUDIODEV_TYPE_OPL;
		arg.hwif = 0;
		arg.hdl = 0;
		(void)config_found(&ac->sc_dev, &arg, audioprint);
	}

#if NMPU_YM > 0
	/* mpu at ym attachment */
	if (sc->sc_mpu_ioh) {
		arg.type = AUDIODEV_TYPE_MPU;
		arg.hwif = 0;
		arg.hdl = 0;
		sc->sc_mpudev = config_found(&ac->sc_dev, &arg, audioprint);
	}
#endif

	/* This must be AFTER the attachment of sub-devices. */
	ym_init(sc);

#ifndef AUDIO_NO_POWER_CTL
	/*
	 * Initialize power control.
	 */
	sc->sc_pow_mode = YM_POWER_MODE;
	sc->sc_pow_timeout = YM_POWER_OFF_SEC;

	sc->sc_on_blocks = sc->sc_turning_off =
		YM_POWER_CODEC_P | YM_POWER_CODEC_R |
		YM_POWER_OPL3 | YM_POWER_MPU401 | YM_POWER_3D |
		YM_POWER_CODEC_DA | YM_POWER_CODEC_AD | YM_POWER_OPL3_DA;
#if NJOY > 0
	sc->sc_on_blocks |= YM_POWER_JOYSTICK;	/* prevents chip powerdown */
#endif
	ym_powerdown_blocks(sc);

	powerhook_establish(ym_power_hook, sc);

	if (sc->sc_on_blocks /* & YM_POWER_ACTIVE */)
#endif
	{
		/* Unmute the output now if the chip is on. */
		ym_mute(sc, SA3_VOL_L, sc->master_mute);
		ym_mute(sc, SA3_VOL_R, sc->master_mute);
	}
}

static __inline int
ym_read(sc, reg)
	struct ym_softc *sc;
	int reg;
{
	bus_space_write_1(sc->sc_iot, sc->sc_controlioh,
				SA3_CTL_INDEX, (reg & 0xff));
	return (bus_space_read_1(sc->sc_iot, sc->sc_controlioh, SA3_CTL_DATA));
}

static __inline void
ym_write(sc, reg, data)
	struct ym_softc *sc;
	int reg;
	int data;
{
	bus_space_write_1(sc->sc_iot, sc->sc_controlioh,
				SA3_CTL_INDEX, (reg & 0xff));
	bus_space_write_1(sc->sc_iot, sc->sc_controlioh,
				SA3_CTL_DATA, (data & 0xff));
}

static void
ym_init(sc)
	struct ym_softc *sc;
{
	u_int8_t dpd, apd;

	/* Mute SoundBlaster output if possible. */
	if (sc->sc_sb_ioh) {
		bus_space_write_1(sc->sc_iot, sc->sc_sb_ioh, SBP_MIXER_ADDR,
				  SBP_MASTER_VOL);
		bus_space_write_1(sc->sc_iot, sc->sc_sb_ioh, SBP_MIXER_DATA,
				  0x00);
	}

	/* Figure out which part can be power down. */
	dpd = SA3_DPWRDWN_SB		/* we never use SB */
#if NMPU_YM > 0
		| (sc->sc_mpu_ioh ? 0 : SA3_DPWRDWN_MPU)
#else
		| SA3_DPWRDWN_MPU
#endif
#if NJOY == 0
		| SA3_DPWRDWN_JOY
#endif
		| SA3_DPWRDWN_PNP	/* ISA Plug and Play is done */
		/*
		 * The master clock is for external wavetable synthesizer
		 * OPL4-ML (YMF704) or OPL4-ML2 (YMF721),
		 * and is currently unused.
		 */
		| SA3_DPWRDWN_MCLKO;

	apd = SA3_APWRDWN_SBDAC;	/* we never use SB */

	/* Power down OPL3 if not attached. */
	if (sc->sc_opl_ioh == 0) {
		dpd |= SA3_DPWRDWN_FM;
		apd |= SA3_APWRDWN_FMDAC;
	}
	/* CODEC is always attached. */

	/* Power down unused digital parts. */
	ym_write(sc, SA3_DPWRDWN, dpd);

	/* Power down unused analog parts. */
	ym_write(sc, SA3_APWRDWN, apd);
}


int
ym_getdev(addr, retp)
	void *addr;
	struct audio_device *retp;
{
	struct ym_softc *sc = addr;

	strcpy(retp->name, "OPL3-SA3");
	sprintf(retp->version, "%d", sc->sc_version);
	strcpy(retp->config, "ym");

	return 0;
}


static ad1848_devmap_t mappings[] = {
	{ YM_DAC_LVL, AD1848_KIND_LVL, AD1848_DAC_CHANNEL },
	{ YM_MIDI_LVL, AD1848_KIND_LVL, AD1848_AUX2_CHANNEL },
	{ YM_CD_LVL, AD1848_KIND_LVL, AD1848_AUX1_CHANNEL },
	{ YM_LINE_LVL, AD1848_KIND_LVL, AD1848_LINE_CHANNEL },
	{ YM_SPEAKER_LVL, AD1848_KIND_LVL, AD1848_MONO_CHANNEL },
	{ YM_MONITOR_LVL, AD1848_KIND_LVL, AD1848_MONITOR_CHANNEL },
	{ YM_DAC_MUTE, AD1848_KIND_MUTE, AD1848_DAC_CHANNEL },
	{ YM_MIDI_MUTE, AD1848_KIND_MUTE, AD1848_AUX2_CHANNEL },
	{ YM_CD_MUTE, AD1848_KIND_MUTE, AD1848_AUX1_CHANNEL },
	{ YM_LINE_MUTE, AD1848_KIND_MUTE, AD1848_LINE_CHANNEL },
	{ YM_SPEAKER_MUTE, AD1848_KIND_MUTE, AD1848_MONO_CHANNEL },
	{ YM_MONITOR_MUTE, AD1848_KIND_MUTE, AD1848_MONITOR_CHANNEL },
	{ YM_REC_LVL, AD1848_KIND_RECORDGAIN, -1 },
	{ YM_RECORD_SOURCE, AD1848_KIND_RECORDSOURCE, -1}
};

#define NUMMAP	(sizeof(mappings) / sizeof(mappings[0]))


static void
ym_mute(sc, left_reg, mute)
	struct ym_softc *sc;
	int left_reg;
	int mute;

{
	u_int8_t reg;

	reg = ym_read(sc, left_reg);
	if (mute)
		ym_write(sc, left_reg, reg | 0x80);
	else
		ym_write(sc, left_reg, reg & ~0x80);
}


static void
ym_set_master_gain(sc, vol)
	struct ym_softc *sc;
	struct ad1848_volume *vol;
{
	u_int  atten;

	sc->master_gain = *vol;

	atten = ((AUDIO_MAX_GAIN - vol->left) * (SA3_VOL_MV + 1)) /
		(AUDIO_MAX_GAIN + 1);

	ym_write(sc, SA3_VOL_L, (ym_read(sc, SA3_VOL_L) & ~SA3_VOL_MV) | atten);

	atten = ((AUDIO_MAX_GAIN - vol->right) * (SA3_VOL_MV + 1)) /
		(AUDIO_MAX_GAIN + 1);

	ym_write(sc, SA3_VOL_R, (ym_read(sc, SA3_VOL_R) & ~SA3_VOL_MV) | atten);
}

static void
ym_set_mic_gain(sc, vol)
	struct ym_softc *sc;
	int vol;
{
	u_int atten;

	sc->mic_gain = vol;

	atten = ((AUDIO_MAX_GAIN - vol) * (SA3_MIC_MCV + 1)) /
		(AUDIO_MAX_GAIN + 1);

	ym_write(sc, SA3_MIC_VOL,
		 (ym_read(sc, SA3_MIC_VOL) & ~SA3_MIC_MCV) | atten);
}

static void
ym_set_3d(sc, cp, val, reg)
	struct ym_softc *sc;
	mixer_ctrl_t *cp;
	struct ad1848_volume *val;
	int reg;
{
	u_int8_t e;

	ad1848_to_vol(cp, val);

	e = (val->left * (SA3_3D_BITS + 1) + (SA3_3D_BITS + 1) / 2) /
		(AUDIO_MAX_GAIN + 1) << SA3_3D_LSHIFT |
	    (val->right * (SA3_3D_BITS + 1) + (SA3_3D_BITS + 1) / 2) /
		(AUDIO_MAX_GAIN + 1) << SA3_3D_RSHIFT;

#ifndef AUDIO_NO_POWER_CTL
	/* turn wide stereo on if necessary */
	if (e)
		ym_power_ctl(sc, YM_POWER_3D, 1);
#endif

	ym_write(sc, reg, e);

#ifndef AUDIO_NO_POWER_CTL
	/* turn wide stereo off if necessary */
	if (YM_EQ_OFF(&sc->sc_treble) && YM_EQ_OFF(&sc->sc_bass) &&
	    YM_EQ_OFF(&sc->sc_wide))
		ym_power_ctl(sc, YM_POWER_3D, 0);
#endif
}

int
ym_mixer_set_port(addr, cp)
	void *addr;
	mixer_ctrl_t *cp;
{
	struct ad1848_softc *ac = addr;
	struct ym_softc *sc = ac->parent;
	struct ad1848_volume vol;
	int error = 0;
	u_int8_t extsources;

	DPRINTF(("%s: ym_mixer_set_port: dev 0x%x, type 0x%x, 0x%x (%d; %d, %d)\n",
		DVNAME(sc), cp->dev, cp->type, cp->un.ord,
		cp->un.value.num_channels, cp->un.value.level[0],
		cp->un.value.level[1]));

#ifndef AUDIO_NO_POWER_CTL
	/* Power-up chip */
	ym_power_ctl(sc, YM_POWER_CODEC_CTL, 1);
#endif

	switch (cp->dev) {
	case YM_OUTPUT_LVL:
		ad1848_to_vol(cp, &vol);
		ym_set_master_gain(sc, &vol);
		goto out;

	case YM_OUTPUT_MUTE:
		sc->master_mute = (cp->un.ord != 0);
		ym_mute(sc, SA3_VOL_L, sc->master_mute);
		ym_mute(sc, SA3_VOL_R, sc->master_mute);
		goto out;

	case YM_MIC_LVL:
		if (cp->un.value.num_channels != 1)
			error = EINVAL;
		else
			ym_set_mic_gain(sc,
				cp->un.value.level[AUDIO_MIXER_LEVEL_MONO]);
		goto out;

	case YM_MASTER_EQMODE:
		sc->sc_eqmode = cp->un.ord & SA3_SYS_CTL_YMODE;
		ym_write(sc, SA3_SYS_CTL, (ym_read(sc, SA3_SYS_CTL) &
					   ~SA3_SYS_CTL_YMODE) | sc->sc_eqmode);
		goto out;

	case YM_MASTER_TREBLE:
		ym_set_3d(sc, cp, &sc->sc_treble, SA3_3D_TREBLE);
		goto out;

	case YM_MASTER_BASS:
		ym_set_3d(sc, cp, &sc->sc_bass, SA3_3D_BASS);
		goto out;

	case YM_MASTER_WIDE:
		ym_set_3d(sc, cp, &sc->sc_wide, SA3_3D_WIDE);
		goto out;

#ifndef AUDIO_NO_POWER_CTL
	case YM_PWR_MODE:
		if ((unsigned) cp->un.ord > YM_POWER_NOSAVE)
			error = EINVAL;
		else
			sc->sc_pow_mode = cp->un.ord;
		goto out;

	case YM_PWR_TIMEOUT:
		if (cp->un.value.num_channels != 1)
			error = EINVAL;
		else
			sc->sc_pow_timeout =
				cp->un.value.level[AUDIO_MIXER_LEVEL_MONO];
		goto out;

	/*
	 * Needs power-up to hear external sources.
	 */
	case YM_CD_MUTE:
	case YM_LINE_MUTE:
	case YM_SPEAKER_MUTE:
		extsources = YM_MIXER_TO_XS(cp->dev);
		if (cp->un.ord) {
			if ((sc->sc_external_sources &= ~extsources) == 0) {
				/*
				 * All the external sources are muted
				 *  --- no need to keep the chip on.
				 */
				ym_power_ctl(sc, YM_POWER_EXT_SRC, 0);
				DPRINTF(("%s: ym_mixer_set_port: off for ext\n",
					DVNAME(sc)));
			}
		} else {
			/* mute off - power-up the chip */
			sc->sc_external_sources |= extsources;
			ym_power_ctl(sc, YM_POWER_EXT_SRC, 1);
			DPRINTF(("%s: ym_mixer_set_port: on for ext\n",
				DVNAME(sc)));
		}
		break;	/* fall to ad1848_mixer_set_port() */

	/*
	 * Power on/off the playback part for monitoring.
	 */
	case YM_MONITOR_MUTE:
		if ((ac->open_mode & (FREAD | FWRITE)) == FREAD)
			ym_power_ctl(sc, YM_POWER_CODEC_P | YM_POWER_CODEC_DA,
					cp->un.ord == 0);
		break;	/* fall to ad1848_mixer_set_port() */
#endif
	}

	error = ad1848_mixer_set_port(ac, mappings, NUMMAP, cp);

	if (error != ENXIO)
		goto out;

	error = 0;

	switch (cp->dev) {
	case YM_MIC_MUTE:
		sc->mic_mute = (cp->un.ord != 0);
		ym_mute(sc, SA3_MIC_VOL, sc->mic_mute);
		break;

	default:
		error = ENXIO;
		break;
	}

out:
#ifndef AUDIO_NO_POWER_CTL
	/* Power-down chip */
	ym_power_ctl(sc, YM_POWER_CODEC_CTL, 0);
#endif

	return (error);
}

int
ym_mixer_get_port(addr, cp)
	void *addr;
	mixer_ctrl_t *cp;
{
	struct ad1848_softc *ac = addr;
	struct ym_softc *sc = ac->parent;
	int error;

	switch (cp->dev) {
	case YM_OUTPUT_LVL:
		ad1848_from_vol(cp, &sc->master_gain);
		return 0;

	case YM_OUTPUT_MUTE:
		cp->un.ord = sc->master_mute;
		return 0;

	case YM_MIC_LVL:
		if (cp->un.value.num_channels != 1)
			return EINVAL;
		cp->un.value.level[AUDIO_MIXER_LEVEL_MONO] = sc->mic_gain;
		return 0;

	case YM_MASTER_EQMODE:
		cp->un.ord = sc->sc_eqmode;
		return 0;

	case YM_MASTER_TREBLE:
		ad1848_from_vol(cp, &sc->sc_treble);
		return 0;

	case YM_MASTER_BASS:
		ad1848_from_vol(cp, &sc->sc_bass);
		return 0;

	case YM_MASTER_WIDE:
		ad1848_from_vol(cp, &sc->sc_wide);
		return 0;

#ifndef AUDIO_NO_POWER_CTL
	case YM_PWR_MODE:
		cp->un.ord = sc->sc_pow_mode;
		return 0;

	case YM_PWR_TIMEOUT:
		if (cp->un.value.num_channels != 1)
			return EINVAL;
		cp->un.value.level[AUDIO_MIXER_LEVEL_MONO] = sc->sc_pow_timeout;
		return 0;
#endif
	}

	error = ad1848_mixer_get_port(ac, mappings, NUMMAP, cp);

	if (error != ENXIO)
		return (error);

	error = 0;

	switch (cp->dev) {
	case YM_MIC_MUTE:
		cp->un.ord = sc->mic_mute;
		break;

	default:
		error = ENXIO;
		break;
	}

	return(error);
}

static char *mixer_classes[] = {
	AudioCinputs, AudioCrecord, AudioCoutputs, AudioCmonitor,
	AudioCequalization
#ifndef AUDIO_NO_POWER_CTL
	, AudioCpower
#endif
};

int
ym_query_devinfo(addr, dip)
	void *addr;
	mixer_devinfo_t *dip;
{
	static char *mixer_port_names[] = {
		AudioNdac, AudioNmidi, AudioNcd, AudioNline, AudioNspeaker,
		AudioNmicrophone, AudioNmonitor
	};

	dip->next = dip->prev = AUDIO_MIXER_LAST;

	switch(dip->index) {
	case YM_INPUT_CLASS:			/* input class descriptor */
	case YM_OUTPUT_CLASS:
	case YM_MONITOR_CLASS:
	case YM_RECORD_CLASS:
	case YM_EQ_CLASS:
#ifndef AUDIO_NO_POWER_CTL
	case YM_PWR_CLASS:
#endif
		dip->type = AUDIO_MIXER_CLASS;
		dip->mixer_class = dip->index;
		strcpy(dip->label.name,
		       mixer_classes[dip->index - YM_INPUT_CLASS]);
		break;

	case YM_DAC_LVL:
	case YM_MIDI_LVL:
	case YM_CD_LVL:
	case YM_LINE_LVL:
	case YM_SPEAKER_LVL:
	case YM_MIC_LVL:
	case YM_MONITOR_LVL:
		dip->type = AUDIO_MIXER_VALUE;
		if (dip->index == YM_MONITOR_LVL)
			dip->mixer_class = YM_MONITOR_CLASS;
		else
			dip->mixer_class = YM_INPUT_CLASS;

		dip->next = dip->index + 7;

		strcpy(dip->label.name,
		       mixer_port_names[dip->index - YM_DAC_LVL]);

		if (dip->index == YM_SPEAKER_LVL ||
		    dip->index == YM_MIC_LVL)
			dip->un.v.num_channels = 1;
		else
			dip->un.v.num_channels = 2;

		strcpy(dip->un.v.units.name, AudioNvolume);
		break;

	case YM_DAC_MUTE:
	case YM_MIDI_MUTE:
	case YM_CD_MUTE:
	case YM_LINE_MUTE:
	case YM_SPEAKER_MUTE:
	case YM_MIC_MUTE:
	case YM_MONITOR_MUTE:
		if (dip->index == YM_MONITOR_MUTE)
			dip->mixer_class = YM_MONITOR_CLASS;
		else
			dip->mixer_class = YM_INPUT_CLASS;
		dip->type = AUDIO_MIXER_ENUM;
		dip->prev = dip->index - 7;
	mute:
		strcpy(dip->label.name, AudioNmute);
		dip->un.e.num_mem = 2;
		strcpy(dip->un.e.member[0].label.name, AudioNoff);
		dip->un.e.member[0].ord = 0;
		strcpy(dip->un.e.member[1].label.name, AudioNon);
		dip->un.e.member[1].ord = 1;
		break;


	case YM_OUTPUT_LVL:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = YM_OUTPUT_CLASS;
		dip->next = YM_OUTPUT_MUTE;
		strcpy(dip->label.name, AudioNmaster);
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		break;

	case YM_OUTPUT_MUTE:
		dip->mixer_class = YM_OUTPUT_CLASS;
		dip->type = AUDIO_MIXER_ENUM;
		dip->prev = YM_OUTPUT_LVL;
		goto mute;


	case YM_REC_LVL:	/* record level */
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = YM_RECORD_CLASS;
		dip->next = YM_RECORD_SOURCE;
		strcpy(dip->label.name, AudioNrecord);
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		break;

	case YM_RECORD_SOURCE:
		dip->mixer_class = YM_RECORD_CLASS;
		dip->type = AUDIO_MIXER_ENUM;
		dip->prev = YM_REC_LVL;
		strcpy(dip->label.name, AudioNsource);
		dip->un.e.num_mem = 4;
		strcpy(dip->un.e.member[0].label.name, AudioNmicrophone);
		dip->un.e.member[0].ord = MIC_IN_PORT;
		strcpy(dip->un.e.member[1].label.name, AudioNline);
		dip->un.e.member[1].ord = LINE_IN_PORT;
		strcpy(dip->un.e.member[2].label.name, AudioNdac);
		dip->un.e.member[2].ord = DAC_IN_PORT;
		strcpy(dip->un.e.member[3].label.name, AudioNcd);
		dip->un.e.member[3].ord = AUX1_IN_PORT;
		break;


	case YM_MASTER_EQMODE:
		dip->type = AUDIO_MIXER_ENUM;
		dip->mixer_class = YM_EQ_CLASS;
		strcpy(dip->label.name, AudioNmode);
		strcpy(dip->un.v.units.name, AudioNmode);
		dip->un.e.num_mem = 4;
		strcpy(dip->un.e.member[0].label.name, AudioNdesktop);
		dip->un.e.member[0].ord = SA3_SYS_CTL_YMODE0;
		strcpy(dip->un.e.member[1].label.name, AudioNlaptop);
		dip->un.e.member[1].ord = SA3_SYS_CTL_YMODE1;
		strcpy(dip->un.e.member[2].label.name, AudioNsubnote);
		dip->un.e.member[2].ord = SA3_SYS_CTL_YMODE2;
		strcpy(dip->un.e.member[3].label.name, AudioNhifi);
		dip->un.e.member[3].ord = SA3_SYS_CTL_YMODE3;
		break;

	case YM_MASTER_TREBLE:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = YM_EQ_CLASS;
		strcpy(dip->label.name, AudioNtreble);
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNtreble);
		break;

	case YM_MASTER_BASS:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = YM_EQ_CLASS;
		strcpy(dip->label.name, AudioNbass);
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNbass);
		break;

	case YM_MASTER_WIDE:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = YM_EQ_CLASS;
		strcpy(dip->label.name, AudioNsurround);
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNsurround);
		break;


#ifndef AUDIO_NO_POWER_CTL
	case YM_PWR_MODE:
		dip->type = AUDIO_MIXER_ENUM;
		dip->mixer_class = YM_PWR_CLASS;
		dip->next = YM_PWR_TIMEOUT;
		strcpy(dip->label.name, AudioNsave);
		dip->un.e.num_mem = 3;
		strcpy(dip->un.e.member[0].label.name, AudioNpowerdown);
		dip->un.e.member[0].ord = YM_POWER_POWERDOWN;
		strcpy(dip->un.e.member[1].label.name, AudioNpowersave);
		dip->un.e.member[1].ord = YM_POWER_POWERSAVE;
		strcpy(dip->un.e.member[2].label.name, AudioNnosave);
		dip->un.e.member[2].ord = YM_POWER_NOSAVE;
		break;

	case YM_PWR_TIMEOUT:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = YM_PWR_CLASS;
		dip->prev = YM_PWR_MODE;
		strcpy(dip->label.name, AudioNtimeout);
		dip->un.v.num_channels = 1;
		strcpy(dip->un.v.units.name, AudioNtimeout);
		break;
#endif /* not AUDIO_NO_POWER_CTL */

	default:
		return ENXIO;
		/*NOTREACHED*/
	}

	return 0;
}

int
ym_intr(arg)
	void *arg;
{
	struct ym_softc *sc = arg;
	u_int8_t ist;
	int processed;

	/* OPL3 timer is currently unused. */
	if (((ist = ym_read(sc, SA3_IRQA_STAT)) &
	     ~(SA3_IRQ_STAT_SB|SA3_IRQ_STAT_OPL3)) == 0) {
		DPRINTF(("%s: ym_intr: spurious interrupt\n", DVNAME(sc)));
		return 0;
	}

	/* Process pending interrupts. */
	do {
		processed = 0;
		/*
		 * CODEC interrupts.
		 */
		if (ist & (SA3_IRQ_STAT_TI|SA3_IRQ_STAT_CI|SA3_IRQ_STAT_PI)) {
			ad1848_isa_intr(&sc->sc_ad1848);
			processed = 1;
		}
#if NMPU_YM > 0
		/*
		 * MPU401 interrupt.
		 */
		if (ist & SA3_IRQ_STAT_MPU) {
			mpu_intr(sc->sc_mpudev);
			processed = 1;
		}
#endif
		/*
		 * Hardware volume interrupt.
		 * Recalculate master volume from the hardware setting.
		 */
		if (ist & SA3_IRQ_STAT_MV) {
			sc->master_gain.left =
				(SA3_VOL_MV & ~ym_read(sc, SA3_VOL_L)) *
					(SA3_VOL_MV + 1) + (SA3_VOL_MV + 1) / 2;
			sc->master_gain.right =
				(SA3_VOL_MV & ~ym_read(sc, SA3_VOL_R)) *
					(SA3_VOL_MV + 1) + (SA3_VOL_MV + 1) / 2;

#if 0	/* XXX NOT YET */
			/* Notify the change to async processes. */
			if (sc->sc_audiodev)
				mixer_signal(sc->sc_audiodev);
#endif
			processed = 1;
		}
	} while (processed && (ist = ym_read(sc, SA3_IRQA_STAT)));

	return 1;
}


#ifndef AUDIO_NO_POWER_CTL
static void
ym_save_codec_regs(sc)
	struct ym_softc *sc;
{
	struct ad1848_softc *ac = &sc->sc_ad1848.sc_ad1848;
	int i;

	DPRINTF(("%s: ym_save_codec_regs\n", DVNAME(sc)));

	for (i = 0; i <= 0x1f; i++)
		sc->sc_codec_scan[i] = ad_read(ac, i);
}

static void
ym_restore_codec_regs(sc)
	struct ym_softc *sc;
{
	struct ad1848_softc *ac = &sc->sc_ad1848.sc_ad1848;
	int i, t;

	DPRINTF(("%s: ym_restore_codec_regs\n", DVNAME(sc)));

	for (i = 0; i <= 0x1f; i++) {
		/*
		 * Wait til the chip becomes ready.
		 * This is required after suspend/resume.
		 */
		for (t = 0;
		    t < 100000 && ADREAD(ac, AD1848_IADDR) & SP_IN_INIT; t++)
			;
#ifdef AUDIO_DEBUG
		if (t)
			DPRINTF(("%s: ym_restore_codec_regs: reg %d, t %d\n",
				 DVNAME(sc), i, t));
#endif
		ad_write(ac, i, sc->sc_codec_scan[i]);
	}
}

/*
 * Save and restore the state on suspending / resumning.
 *
 * XXX This is not complete.
 * Currently only the parameters, such as output gain, are restored.
 * DMA state should also be restored.  FIXME.
 */
void
ym_power_hook(why, v)
	int why;
	void *v;
{
	struct ym_softc *sc = v;
	int i;
	int s;

	DPRINTF(("%s: ym_power_hook: why = %d\n", DVNAME(sc), why));

	s = splaudio();

	if (why != PWR_RESUME) {
		/*
		 * suspending...
		 */
		callout_stop(&sc->sc_powerdown_ch);
		if (sc->sc_turning_off)
			ym_powerdown_blocks(sc);

		/*
		 * Save CODEC registers.
		 * Note that the registers read incorrect
		 * if the CODEC part is in power-down mode.
		 */
		if (sc->sc_on_blocks & YM_POWER_CODEC_DIGITAL)
			ym_save_codec_regs(sc);

		/*
		 * Save OPL3-SA3 control registers and power-down the chip.
		 * Note that the registers read incorrect
		 * if the chip is in global power-down mode.
		 */
		sc->sc_sa3_scan[SA3_PWR_MNG] = ym_read(sc, SA3_PWR_MNG);
		if (sc->sc_on_blocks)
			ym_chip_powerdown(sc);
	} else {
		/*
		 * resuming...
		 */
		ym_chip_powerup(sc, 1);
		ym_init(sc);		/* power-on CODEC */

		/* Restore control registers. */
		for (i = SA3_PWR_MNG + 1; i <= YM_SAVE_REG_MAX; i++) {
			if (i == SA3_SB_SCAN || i == SA3_SB_SCAN_DATA ||
			    i == SA3_DPWRDWN)
				continue;
			ym_write(sc, i, sc->sc_sa3_scan[i]);
		}

		/* Restore CODEC registers (including mixer). */
		ym_restore_codec_regs(sc);

		/* Restore global/digital power-down state. */
		ym_write(sc, SA3_PWR_MNG, sc->sc_sa3_scan[SA3_PWR_MNG]);
		ym_write(sc, SA3_DPWRDWN, sc->sc_sa3_scan[SA3_DPWRDWN]);
	}
	splx(s);
}

int
ym_codec_power_ctl(arg, flags)
	void *arg;
	int flags;
{
	struct ym_softc *sc = arg;
	struct ad1848_softc *ac = &sc->sc_ad1848.sc_ad1848;
	int parts;

	DPRINTF(("%s: ym_codec_power_ctl: flags = 0x%x\n", DVNAME(sc), flags));

	if (flags != 0) {
		parts = 0;
		if (flags & FREAD) {
			parts |= YM_POWER_CODEC_R | YM_POWER_CODEC_AD;
			if (ac->mute[AD1848_MONITOR_CHANNEL] == 0)
				parts |= YM_POWER_CODEC_P | YM_POWER_CODEC_DA;
		}
		if (flags & FWRITE)
			parts |= YM_POWER_CODEC_P | YM_POWER_CODEC_DA;
	} else
		parts = YM_POWER_CODEC_P | YM_POWER_CODEC_R |
			YM_POWER_CODEC_DA | YM_POWER_CODEC_AD;

	ym_power_ctl(sc, parts, flags);

	return 0;
}

/*
 * Enter Power Save mode or Global Power Down mode.
 * Total dissipation becomes 5mA and 10uA (typ.) respective.
 *
 * This must be called at splaudio().
 */
static void
ym_chip_powerdown(sc)
	struct ym_softc *sc;
{
	int i;

	DPRINTF(("%s: ym_chip_powerdown\n", DVNAME(sc)));

	/* Save control registers. */
	for (i = SA3_PWR_MNG + 1; i <= YM_SAVE_REG_MAX; i++) {
		if (i == SA3_SB_SCAN || i == SA3_SB_SCAN_DATA)
			continue;
		sc->sc_sa3_scan[i] = ym_read(sc, i);
	}
	ym_write(sc, SA3_PWR_MNG,
		 (sc->sc_pow_mode == YM_POWER_POWERDOWN ?
			SA3_PWR_MNG_PDN : SA3_PWR_MNG_PSV) | SA3_PWR_MNG_PDX);
}

/*
 * Power up from Power Save / Global Power Down Mode.
 *
 * We assume no ym interrupt shall occur, since the chip is
 * in power-down mode (or should be blocked by splaudio()).
 */
static void
ym_chip_powerup(sc, nosleep)
	struct ym_softc *sc;
	int nosleep;
{
	int wchan;
	u_int8_t pw;

	DPRINTF(("%s: ym_chip_powerup\n", DVNAME(sc)));

	pw = ym_read(sc, SA3_PWR_MNG);

	if ((pw & (SA3_PWR_MNG_PSV | SA3_PWR_MNG_PDN | SA3_PWR_MNG_PDX)) == 0)
		return;		/* already on */

	pw &= ~SA3_PWR_MNG_PDX;
	ym_write(sc, SA3_PWR_MNG, pw);

	/* wait 100 ms */
	if (nosleep)
		delay(100000);
	else
		tsleep(&wchan, PWAIT, "ym_pu1", hz / 10);

	pw &= ~(SA3_PWR_MNG_PSV | SA3_PWR_MNG_PDN);
	ym_write(sc, SA3_PWR_MNG, pw);

	/* wait 70 ms */
	if (nosleep)
		delay(70000);
	else
		tsleep(&wchan, PWAIT, "ym_pu2", hz / 14);

	/* The chip is muted automatically --- unmute it now. */
	ym_mute(sc, SA3_VOL_L, sc->master_mute);
	ym_mute(sc, SA3_VOL_R, sc->master_mute);
}

/* callout handler for power-down */
void
ym_powerdown_blocks(arg)
	void *arg;
{
	struct ym_softc *sc = arg;
	u_int16_t parts;
	u_int16_t on_blocks = sc->sc_on_blocks;
	u_int8_t sv;
	int s;

	DPRINTF(("%s: ym_powerdown_blocks: turning_off 0x%x\n",
		DVNAME(sc), sc->sc_turning_off));

	s = splaudio();

	on_blocks = sc->sc_on_blocks;

	/* Be sure not to change the state of the chip.  Save it first. */
	sv =  bus_space_read_1(sc->sc_iot, sc->sc_controlioh, SA3_CTL_INDEX);

	parts = sc->sc_turning_off;

	if (on_blocks & ~parts & YM_POWER_CODEC_CTL)
		parts &= ~(YM_POWER_CODEC_P | YM_POWER_CODEC_R);
	if (parts & YM_POWER_CODEC_CTL) {
		if ((on_blocks & YM_POWER_CODEC_P) == 0)
			parts |= YM_POWER_CODEC_P;
		if ((on_blocks & YM_POWER_CODEC_R) == 0)
			parts |= YM_POWER_CODEC_R;
	}
	parts &= ~YM_POWER_CODEC_PSEUDO;

	/* If CODEC is being off, save the state. */
	if ((sc->sc_on_blocks & YM_POWER_CODEC_DIGITAL) &&
	    (sc->sc_on_blocks & ~sc->sc_turning_off &
				YM_POWER_CODEC_DIGITAL) == 0)
		ym_save_codec_regs(sc);

	ym_write(sc, SA3_DPWRDWN, ym_read(sc, SA3_DPWRDWN) | (u_int8_t) parts);
	ym_write(sc, SA3_APWRDWN, ym_read(sc, SA3_APWRDWN) | (parts >> 8));

	if (((sc->sc_on_blocks &= ~sc->sc_turning_off) & YM_POWER_ACTIVE) == 0)
		ym_chip_powerdown(sc);

	sc->sc_turning_off = 0;

	/* Restore the state of the chip. */
	bus_space_write_1(sc->sc_iot, sc->sc_controlioh, SA3_CTL_INDEX, sv);

	splx(s);
}

/*
 * Power control entry point.
 */
void
ym_power_ctl(sc, parts, onoff)
	struct ym_softc *sc;
	int parts, onoff;
{
	int s;
	int need_restore_codec;

	DPRINTF(("%s: ym_power_ctl: parts = 0x%x, %s\n",
		DVNAME(sc), parts, onoff ? "on" : "off"));

#ifdef DIAGNOSTIC
	if (curproc == NULL)
		panic("ym_power_ctl: no curproc");
#endif
	/* This function may sleep --- needs locking. */
	while (sc->sc_in_power_ctl & YM_POWER_CTL_INUSE) {
		sc->sc_in_power_ctl |= YM_POWER_CTL_WANTED;
		DPRINTF(("%s: ym_power_ctl: sleeping\n", DVNAME(sc)));
		tsleep(&sc->sc_in_power_ctl, PWAIT, "ym_pc", 0);
		DPRINTF(("%s: ym_power_ctl: awaken\n", DVNAME(sc)));
	}
	sc->sc_in_power_ctl |= YM_POWER_CTL_INUSE;

	/* Defeat softclock interrupts. */
	s = splsoftclock();

	/* If ON requested to parts which are scheduled to OFF, cancel it. */
	if (onoff && sc->sc_turning_off && (sc->sc_turning_off &= ~parts) == 0)
		callout_stop(&sc->sc_powerdown_ch);

	if (!onoff && sc->sc_turning_off)
		parts &= ~sc->sc_turning_off;

	/* Discard bits which are currently {on,off}. */
	parts &= onoff ? ~sc->sc_on_blocks : sc->sc_on_blocks;

	/* Cancel previous timeout if needed. */
	if (parts != 0 && sc->sc_turning_off)
		callout_stop(&sc->sc_powerdown_ch);

	(void) splx(s);

	if (parts == 0)
		goto unlock;		/* no work to do */

	if (onoff) {
		/* Turning on is done immediately. */

		/* If the chip is off, turn it on. */
		if ((sc->sc_on_blocks & YM_POWER_ACTIVE) == 0)
			ym_chip_powerup(sc, 0);

		need_restore_codec = (parts & YM_POWER_CODEC_DIGITAL) &&
		    (sc->sc_on_blocks & YM_POWER_CODEC_DIGITAL) == 0;

		sc->sc_on_blocks |= parts;
		if (parts & YM_POWER_CODEC_CTL)
			parts |= YM_POWER_CODEC_P | YM_POWER_CODEC_R;

		s = splaudio();

		ym_write(sc, SA3_DPWRDWN,
			 ym_read(sc, SA3_DPWRDWN) & (u_int8_t)~parts);
		ym_write(sc, SA3_APWRDWN,
			 ym_read(sc, SA3_APWRDWN) & ~(parts >> 8));
		if (need_restore_codec)
			ym_restore_codec_regs(sc);

		(void) splx(s);
	} else {
		/* Turning off is delayed. */
		sc->sc_turning_off |= parts;
	}

	/* Schedule turning off. */
	if (sc->sc_pow_mode != YM_POWER_NOSAVE && sc->sc_turning_off)
		callout_reset(&sc->sc_powerdown_ch, hz * sc->sc_pow_timeout,
		    ym_powerdown_blocks, sc);

unlock:
	if (sc->sc_in_power_ctl & YM_POWER_CTL_WANTED)
		wakeup(&sc->sc_in_power_ctl);
	sc->sc_in_power_ctl = 0;
}
#endif /* not AUDIO_NO_POWER_CTL */
