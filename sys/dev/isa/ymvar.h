/*	$NetBSD: ymvar.h,v 1.4 1999/10/05 03:46:08 itohy Exp $	*/

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
 * Copyright (c) 1994 John Brezak
 * Copyright (c) 1991-1993 Regents of the University of California.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/*
 *  Original code from OpenBSD.
 */

/*
 * Mixer devices
 */
#define YM_DAC_LVL		0
#define YM_MIDI_LVL		1
#define YM_CD_LVL		2
#define YM_LINE_LVL		3
#define YM_SPEAKER_LVL		4
#define YM_MIC_LVL		5
#define YM_MONITOR_LVL		6
#define YM_DAC_MUTE		7
#define YM_MIDI_MUTE		8
#define YM_CD_MUTE		9
#define YM_LINE_MUTE		10
#define YM_SPEAKER_MUTE		11
#define YM_MIC_MUTE		12
#define YM_MONITOR_MUTE		13

#define YM_REC_LVL		14
#define YM_RECORD_SOURCE	15

#define YM_OUTPUT_LVL		16
#define YM_OUTPUT_MUTE		17

#define YM_MASTER_EQMODE	18
#define YM_MASTER_TREBLE	19
#define YM_MASTER_BASS		20
#define YM_MASTER_WIDE		21

#ifndef AUDIO_NO_POWER_CTL
#define YM_PWR_MODE		22
#define YM_PWR_TIMEOUT		23
#endif

/* Classes - don't change this without looking at mixer_classes array */
#ifdef AUDIO_NO_POWER_CTL
#define YM_CLASS_TOP		22
#else
#define YM_CLASS_TOP		24
#endif
#define YM_INPUT_CLASS		(YM_CLASS_TOP + 0)
#define YM_RECORD_CLASS		(YM_CLASS_TOP + 1)
#define YM_OUTPUT_CLASS		(YM_CLASS_TOP + 2)
#define YM_MONITOR_CLASS	(YM_CLASS_TOP + 3)
#define YM_EQ_CLASS		(YM_CLASS_TOP + 4)
#ifndef AUDIO_NO_POWER_CTL
#define YM_PWR_CLASS		(YM_CLASS_TOP + 5)
#endif

/* XXX should be in <sys/audioio.h> */
#define AudioNmode		"mode"
#define AudioNdesktop		"desktop"
#define AudioNlaptop		"laptop"
#define AudioNsubnote		"subnote"
#define AudioNhifi		"hifi"

#ifndef AUDIO_NO_POWER_CTL
#define AudioCpower		"power"
#define AudioNpower		"power"
#define AudioNtimeout		"timeout"
#define AudioNpowerdown		"powerdown"
#define AudioNpowersave		"powersave"
#define AudioNnosave		"nosave"
#endif


struct ym_softc {
	struct	ad1848_isa_softc sc_ad1848;
#define ym_irq		sc_ad1848.sc_irq
#define ym_playdrq	sc_ad1848.sc_playdrq
#define ym_recdrq	sc_ad1848.sc_recdrq

	bus_space_tag_t sc_iot;		/* tag */
	bus_space_handle_t sc_ioh;	/* handle */
	isa_chipset_tag_t sc_ic;

	bus_space_handle_t sc_controlioh;
	bus_space_handle_t sc_opl_ioh;
	bus_space_handle_t sc_sb_ioh;	/* only used to disable it */

	int  master_mute, mic_mute;
	struct ad1848_volume master_gain;
	u_int8_t mic_gain;

	u_int8_t sc_version;		/* hardware version */

	/* 3D encehamcement */
	u_int8_t sc_eqmode;
	struct ad1848_volume sc_treble, sc_bass, sc_wide;
#define YM_EQ_OFF(v)	\
	((v)->left < (AUDIO_MAX_GAIN + 1) / (SA3_3D_BITS + 1) &&	\
	(v)->right < (AUDIO_MAX_GAIN + 1) / (SA3_3D_BITS + 1))

	struct device *sc_audiodev;

#if NMPU_YM > 0
	bus_space_handle_t sc_mpu_ioh;
	struct device *sc_mpudev;
#endif

#ifndef AUDIO_NO_POWER_CTL
	enum ym_pow_mode {
		YM_POWER_POWERDOWN, YM_POWER_POWERSAVE, YM_POWER_NOSAVE
	} sc_pow_mode;
	int	sc_pow_timeout;

	u_int8_t sc_codec_scan[0x20];
#define YM_SAVE_REG_MAX	SA3_HVOL_INTR_CNF
	u_int8_t sc_sa3_scan[YM_SAVE_REG_MAX + 1];

	u_int16_t sc_on_blocks;
	u_int16_t sc_turning_off;

	int	sc_in_power_ctl;
#define YM_POWER_CTL_INUSE	1
#define YM_POWER_CTL_WANTED	2
#endif /* not AUDIO_NO_POWER_CTL */
};

#ifndef AUDIO_NO_POWER_CTL
/* digital */
#define YM_POWER_CODEC_P	SA3_DPWRDWN_WSS_P
#define YM_POWER_CODEC_R	SA3_DPWRDWN_WSS_R
#define YM_POWER_OPL3		SA3_DPWRDWN_FM
#define YM_POWER_MPU401		SA3_DPWRDWN_MPU
#define YM_POWER_JOYSTICK	SA3_DPWRDWN_JOY
/* analog */
#define YM_POWER_3D		(SA3_APWRDWN_WIDE << 8)
#define YM_POWER_CODEC_DA	(SA3_APWRDWN_DA << 8)
#define YM_POWER_CODEC_AD	(SA3_APWRDWN_AD << 8)
#define YM_POWER_OPL3_DA	(SA3_APWRDWN_FMDAC << 8)
/* pseudo */
#define YM_POWER_CODEC_CTL	0x8000

#define YM_POWER_CODEC_DIGITAL	\
		(YM_POWER_CODEC_P | YM_POWER_CODEC_R | YM_POWER_CODEC_CTL)
/* 3D enhance is passive */
#define YM_POWER_ACTIVE		(0xffff & ~YM_POWER_3D)

#ifdef _KERNEL
void	ym_power_ctl __P((struct ym_softc *, int, int));
#endif
#endif /* not AUDIO_NO_POWER_CTL */

#ifdef _KERNEL
void	ym_attach __P((struct ym_softc *));
#endif
