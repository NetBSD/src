/*      $NetBSD: ac97.c,v 1.39 2003/01/20 07:22:15 simonb Exp $ */
/*	$OpenBSD: ac97.c,v 1.8 2000/07/19 09:01:35 csapuntz Exp $	*/

/*
 * Copyright (c) 1999, 2000 Constantine Sapuntzakis
 *
 * Author:        Constantine Sapuntzakis <csapuntz@stanford.edu>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE
 */

/* Partially inspired by FreeBSD's sys/dev/pcm/ac97.c. It came with
   the following copyright */

/*
 * Copyright (c) 1999 Cameron Grant <gandalf@vilnya.demon.co.uk>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ac97.c,v 1.39 2003/01/20 07:22:15 simonb Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/device.h>

#include <sys/audioio.h>
#include <dev/audio_if.h>

#include <dev/ic/ac97reg.h>
#include <dev/ic/ac97var.h>

#define Ac97Ntone	"tone"
#define Ac97Nphone	"phone"

static const struct audio_mixer_enum ac97_on_off = { 2,
					       { { { AudioNoff } , 0 },
						 { { AudioNon }  , 1 } }};

static const struct audio_mixer_enum ac97_mic_select = { 2,
					       { { { AudioNmicrophone "0" },
						   0 },
						 { { AudioNmicrophone "1" },
						   1 } }};

static const struct audio_mixer_enum ac97_mono_select = { 2,
					       { { { AudioNmixerout },
						   0 },
						 { { AudioNmicrophone },
						   1 } }};

static const struct audio_mixer_enum ac97_source = { 8,
					       { { { AudioNmicrophone } , 0 },
						 { { AudioNcd }, 1 },
						 { { AudioNvideo }, 2 },
						 { { AudioNaux }, 3 },
						 { { AudioNline }, 4 },
						 { { AudioNmixerout }, 5 },
						 { { AudioNmixerout AudioNmono }, 6 },
						 { { Ac97Nphone }, 7 }}};

/*
 * Due to different values for each source that uses these structures,
 * the ac97_query_devinfo function sets delta in mixer_devinfo_t using
 * ac97_source_info.bits.
 */
static const struct audio_mixer_value ac97_volume_stereo = { { AudioNvolume },
						       2 };

static const struct audio_mixer_value ac97_volume_mono = { { AudioNvolume },
						     1 };

#define WRAP(a)  &a, sizeof(a)

const struct ac97_source_info {
	const char *class;
	const char *device;
	const char *qualifier;

	int  type;
	const void *info;
	int  info_size;

	u_int8_t  reg;
	u_int16_t default_value;
	u_int8_t  bits:3;
	u_int8_t  ofs:4;
	u_int8_t  mute:1;
	u_int8_t  polarity:1;   /* Does 0 == MAX or MIN */
	enum {
		CHECK_NONE = 0,
		CHECK_SURROUND,
		CHECK_CENTER,
		CHECK_LFE,
		CHECK_HEADPHONES,
		CHECK_TONE,
		CHECK_MIC,
		CHECK_LOUDNESS,
		CHECK_3D
	} req_feature;

	int  prev;
	int  next;
	int  mixer_class;
} source_info[] = {
	{ AudioCinputs,		NULL,		NULL,
	  AUDIO_MIXER_CLASS, },
	{ AudioCoutputs,	NULL,		NULL,
	  AUDIO_MIXER_CLASS, },
	{ AudioCrecord,		NULL,		NULL,
	  AUDIO_MIXER_CLASS, },
	/* Stereo master volume*/
	{ AudioCoutputs,	AudioNmaster,	NULL,
	  AUDIO_MIXER_VALUE, WRAP(ac97_volume_stereo),
	  AC97_REG_MASTER_VOLUME, 0x8000, 5, 0, 1,
	},
	/* Mono volume */
	{ AudioCoutputs,	AudioNmono,	NULL,
	  AUDIO_MIXER_VALUE, WRAP(ac97_volume_mono),
	  AC97_REG_MASTER_VOLUME_MONO, 0x8000, 6, 0, 1,
	},
	{ AudioCoutputs,	AudioNmono,	AudioNsource,
	  AUDIO_MIXER_ENUM, WRAP(ac97_mono_select),
	  AC97_REG_GP, 0x0000, 1, 9, 0,
	},
	/* Headphone volume */
	{ AudioCoutputs,	AudioNheadphone, NULL,
	  AUDIO_MIXER_VALUE, WRAP(ac97_volume_stereo),
	  AC97_REG_HEADPHONE_VOLUME, 0x8000, 6, 0, 1, 0, CHECK_HEADPHONES
	},
	/* Surround volume - logic hard coded for mute */
	{ AudioCoutputs,	AudioNsurround,	NULL,
	  AUDIO_MIXER_VALUE, WRAP(ac97_volume_stereo),
	  AC97_REG_SURR_MASTER, 0x8080, 5, 0, 1, 0, CHECK_SURROUND
	},
	/* Center volume*/
	{ AudioCoutputs,	AudioNcenter,	NULL,
	  AUDIO_MIXER_VALUE, WRAP(ac97_volume_mono),
	  AC97_REG_CENTER_LFE_MASTER, 0x8080, 5, 0, 0, 0, CHECK_CENTER
	},
	{ AudioCoutputs,	AudioNcenter,	AudioNmute,
	  AUDIO_MIXER_ENUM, WRAP(ac97_on_off),
	  AC97_REG_CENTER_LFE_MASTER, 0x8080, 1, 7, 0, 0, CHECK_CENTER
	},
	/* LFE volume*/
	{ AudioCoutputs,	AudioNlfe,	NULL,
	  AUDIO_MIXER_VALUE, WRAP(ac97_volume_mono),
	  AC97_REG_CENTER_LFE_MASTER, 0x8080, 5, 8, 0, 0, CHECK_LFE
	},
	{ AudioCoutputs,	AudioNlfe,	AudioNmute,
	  AUDIO_MIXER_ENUM, WRAP(ac97_on_off),
	  AC97_REG_CENTER_LFE_MASTER, 0x8080, 1, 15, 0, 0, CHECK_LFE
	},
	/* Tone */
	{ AudioCoutputs,	Ac97Ntone,	NULL,
	  AUDIO_MIXER_VALUE, WRAP(ac97_volume_stereo),
	  AC97_REG_MASTER_TONE, 0x0f0f, 4, 0, 0, 0, CHECK_TONE
	},
	/* PC Beep Volume */
	{ AudioCinputs,		AudioNspeaker,	NULL,
	  AUDIO_MIXER_VALUE, WRAP(ac97_volume_mono),
	  AC97_REG_PCBEEP_VOLUME, 0x0000, 4, 1, 1,
	},

	/* Phone */
	{ AudioCinputs,		Ac97Nphone,	NULL,
	  AUDIO_MIXER_VALUE, WRAP(ac97_volume_mono),
	  AC97_REG_PHONE_VOLUME, 0x8008, 5, 0, 1,
	},
	/* Mic Volume */
	{ AudioCinputs,		AudioNmicrophone, NULL,
	  AUDIO_MIXER_VALUE, WRAP(ac97_volume_mono),
	  AC97_REG_MIC_VOLUME, 0x8008, 5, 0, 1,
	},
	{ AudioCinputs,		AudioNmicrophone, AudioNpreamp,
	  AUDIO_MIXER_ENUM, WRAP(ac97_on_off),
	  AC97_REG_MIC_VOLUME, 0x8008, 1, 6, 0,
	},
	{ AudioCinputs,		AudioNmicrophone, AudioNsource,
	  AUDIO_MIXER_ENUM, WRAP(ac97_mic_select),
	  AC97_REG_GP, 0x0000, 1, 8, 0,
	},
	/* Line in Volume */
	{ AudioCinputs,		AudioNline,	NULL,
	  AUDIO_MIXER_VALUE, WRAP(ac97_volume_stereo),
	  AC97_REG_LINEIN_VOLUME, 0x8808, 5, 0, 1,
	},
	/* CD Volume */
	{ AudioCinputs,		AudioNcd,	NULL,
	  AUDIO_MIXER_VALUE, WRAP(ac97_volume_stereo),
	  AC97_REG_CD_VOLUME, 0x8808, 5, 0, 1,
	},
	/* Video Volume */
	{ AudioCinputs,		AudioNvideo,	NULL,
	  AUDIO_MIXER_VALUE, WRAP(ac97_volume_stereo),
	  AC97_REG_VIDEO_VOLUME, 0x8808, 5, 0, 1,
	},
	/* AUX volume */
	{ AudioCinputs,		AudioNaux,	NULL,
	  AUDIO_MIXER_VALUE, WRAP(ac97_volume_stereo),
	  AC97_REG_AUX_VOLUME, 0x8808, 5, 0, 1,
	},
	/* PCM out volume */
	{ AudioCinputs,		AudioNdac,	NULL,
	  AUDIO_MIXER_VALUE, WRAP(ac97_volume_stereo),
	  AC97_REG_PCMOUT_VOLUME, 0x8808, 5, 0, 1,
	},
	/* Record Source - some logic for this is hard coded - see below */
	{ AudioCrecord,		AudioNsource,	NULL,
	  AUDIO_MIXER_ENUM, WRAP(ac97_source),
	  AC97_REG_RECORD_SELECT, 0x0000, 3, 0, 0,
	},
	/* Record Gain */
	{ AudioCrecord,		AudioNvolume,	NULL,
	  AUDIO_MIXER_VALUE, WRAP(ac97_volume_stereo),
	  AC97_REG_RECORD_GAIN, 0x8000, 4, 0, 1, 1,
	},
	/* Record Gain mic */
	{ AudioCrecord,		AudioNmicrophone, NULL,
	  AUDIO_MIXER_VALUE, WRAP(ac97_volume_mono),
	  AC97_REG_RECORD_GAIN_MIC, 0x8000, 4, 0, 1, 1, CHECK_MIC
	},
	/* */
	{ AudioCoutputs,	AudioNloudness,	NULL,
	  AUDIO_MIXER_ENUM, WRAP(ac97_on_off),
	  AC97_REG_GP, 0x0000, 1, 12, 0, 0, CHECK_LOUDNESS
	},
	{ AudioCoutputs,	AudioNspatial,	NULL,
	  AUDIO_MIXER_ENUM, WRAP(ac97_on_off),
	  AC97_REG_GP, 0x0000, 1, 13, 0, 1, CHECK_3D
	},
	{ AudioCoutputs,	AudioNspatial,	"center",
	  AUDIO_MIXER_VALUE, WRAP(ac97_volume_mono),
	  AC97_REG_3D_CONTROL, 0x0000, 4, 8, 0, 1, CHECK_3D
	},
	{ AudioCoutputs,	AudioNspatial,	"depth",
	  AUDIO_MIXER_VALUE, WRAP(ac97_volume_mono),
	  AC97_REG_3D_CONTROL, 0x0000, 4, 0, 0, 1, CHECK_3D
	},

	/* Missing features: Simulated Stereo, POP, Loopback mode */
} ;

#define SOURCE_INFO_SIZE (sizeof(source_info)/sizeof(source_info[0]))

/*
 * Check out http://developer.intel.com/pc-supp/platform/ac97/ for
 * information on AC-97
 */

struct ac97_softc {
	/* ac97_codec_if must be at the first of ac97_softc. */
	struct ac97_codec_if codec_if;

	struct ac97_host_if *host_if;

#define MAX_SOURCES	(2 * SOURCE_INFO_SIZE)
	struct ac97_source_info source_info[MAX_SOURCES];
	int num_source_info;

	enum ac97_host_flags host_flags;
	unsigned int ac97_clock; /* usually 48000 */
#define AC97_STANDARD_CLOCK	48000U
	u_int16_t caps;		/* -> AC97_REG_RESET */
	u_int16_t ext_id;	/* -> AC97_REG_EXT_AUDIO_ID */
	u_int16_t shadow_reg[128];
};

int ac97_mixer_get_port __P((struct ac97_codec_if *self, mixer_ctrl_t *cp));
int ac97_mixer_set_port __P((struct ac97_codec_if *self, mixer_ctrl_t *));
int ac97_query_devinfo __P((struct ac97_codec_if *self, mixer_devinfo_t *));
int ac97_get_portnum_by_name __P((struct ac97_codec_if *, const char *,
				  const char *, const char *));
void ac97_restore_shadow __P((struct ac97_codec_if *self));
int ac97_set_rate(struct ac97_codec_if *codec_if, int target, u_long *rate);
void ac97_set_clock(struct ac97_codec_if *codec_if, unsigned int clock);
u_int16_t ac97_get_extcaps(struct ac97_codec_if *codec_if);
int ac97_add_port(struct ac97_softc *as, const struct ac97_source_info *src);

static void ac97_alc650_init(struct ac97_softc *);
static void ac97_vt1616_init(struct ac97_softc *);

struct ac97_codec_if_vtbl ac97civ = {
	ac97_mixer_get_port,
	ac97_mixer_set_port,
	ac97_query_devinfo,
	ac97_get_portnum_by_name,
	ac97_restore_shadow,
	ac97_get_extcaps,
	ac97_set_rate,
	ac97_set_clock,
};

static const struct ac97_codecid {
	u_int32_t id;
	u_int32_t mask;
	const char *name;
	void (*init)(struct ac97_softc *);
} ac97codecid[] = {
	/*
	 * Analog Devices SoundMAX
	 * http://www.soundmax.com/products/information/codecs.html
	 * http://www.analog.com/productSelection/pdf/AD1881A_0.pdf
	 * http://www.analog.com/productSelection/pdf/AD1885_0.pdf
	 * http://www.analog.com/productSelection/pdf/AD1981A_0.pdf
	 * http://www.analog.com/productSelection/pdf/AD1981B_0.pdf
	 */
	{ AC97_CODEC_ID('A', 'D', 'S', 3),
	  0xffffffff,			"Analog Devices AD1819B" },
	{ AC97_CODEC_ID('A', 'D', 'S', 0x40),
	  0xffffffff,			"Analog Devices AD1881" },
	{ AC97_CODEC_ID('A', 'D', 'S', 0x48),
	  0xffffffff,			"Analog Devices AD1881A" },
	{ AC97_CODEC_ID('A', 'D', 'S', 0x60),
	  0xffffffff,			"Analog Devices AD1885" },
	{ AC97_CODEC_ID('A', 'D', 'S', 0x61),
	  0xffffffff,			"Analog Devices AD1886" },
	{ AC97_CODEC_ID('A', 'D', 'S', 0x63),
	  0xffffffff,			"Analog Devices AD1886A" },
	{ AC97_CODEC_ID('A', 'D', 'S', 0x70),
	  0xffffffff,			"Analog Devices AD1981" },
	{ AC97_CODEC_ID('A', 'D', 'S', 0x72),
	  0xffffffff,			"Analog Devices AD1981A" },
	{ AC97_CODEC_ID('A', 'D', 'S', 0x74),
	  0xffffffff,			"Analog Devices AD1981B" },
	{ AC97_CODEC_ID('A', 'D', 'S', 0),
	  AC97_VENDOR_ID_MASK,		"Analog Devices unknown" },

	/*
	 * Datasheets:
	 *	http://www.asahi-kasei.co.jp/akm/usa/product/ak4541/ek4541.pdf
	 *	http://www.asahi-kasei.co.jp/akm/usa/product/ak4543/ek4543.pdf
	 *	http://www.asahi-kasei.co.jp/akm/usa/product/ak4544a/ek4544a.pdf
	 *	http://www.asahi-kasei.co.jp/akm/usa/product/ak4545/ek4545.pdf
	 */
	{ AC97_CODEC_ID('A', 'K', 'M', 0),
	  0xffffffff,			"Asahi Kasei AK4540"	},
	{ AC97_CODEC_ID('A', 'K', 'M', 1),
	  0xffffffff,			"Asahi Kasei AK4542"	},
	{ AC97_CODEC_ID('A', 'K', 'M', 2),
	  0xffffffff,			"Asahi Kasei AK4541/AK4543" },
	{ AC97_CODEC_ID('A', 'K', 'M', 5),
	  0xffffffff,			"Asahi Kasei AK4544" },
	{ AC97_CODEC_ID('A', 'K', 'M', 6),
	  0xffffffff,			"Asahi Kasei AK4544A" },
	{ AC97_CODEC_ID('A', 'K', 'M', 7),
	  0xffffffff,			"Asahi Kasei AK4545" },
	{ AC97_CODEC_ID('A', 'K', 'M', 0),
	  AC97_VENDOR_ID_MASK,		"Asahi Kasei unknown" },

	/*
	 * Realtek & Avance Logic
	 *	http://www.realtek.com.tw/downloads/downloads1-3.aspx?lineid=5&famid=All&series=All&Spec=True
	 */
	{ AC97_CODEC_ID('A', 'L', 'C', 0x00),
	  0xfffffff0,			"Realtek RL5306"	},
	{ AC97_CODEC_ID('A', 'L', 'C', 0x10),
	  0xfffffff0,			"Realtek RL5382"	},
	{ AC97_CODEC_ID('A', 'L', 'C', 0x20),
	  0xfffffff0,			"Realtek RL5383/RL5522/ALC100"	},
	{ AC97_CODEC_ID('A', 'L', 'G', 0x10),
	  0xffffffff,			"Avance Logic ALC200/ALC201"	},
	{ AC97_CODEC_ID('A', 'L', 'G', 0x20),
	  0xffffffff,			"Avance Logic ALC650", ac97_alc650_init },
	{ AC97_CODEC_ID('A', 'L', 'G', 0x30),
	  0xffffffff,			"Avance Logic ALC101"	},
	{ AC97_CODEC_ID('A', 'L', 'G', 0x40),
	  0xffffffff,			"Avance Logic ALC202"	},
	{ AC97_CODEC_ID('A', 'L', 'G', 0x50),
	  0xffffffff,			"Avance Logic ALC250"	},
	{ AC97_CODEC_ID('A', 'L', 'C', 0),
	  AC97_VENDOR_ID_MASK,		"Realtek unknown"	},
	{ AC97_CODEC_ID('A', 'L', 'G', 0),
	  AC97_VENDOR_ID_MASK,		"Avance Logic unknown"	},

	/* Cirrus Logic, Crystal series:
	 *  'C' 'R' 'Y' 0x0[0-7]  - CS4297
	 *              0x1[0-7]  - CS4297A
	 *              0x2[0-7]  - CS4298
	 *              0x2[8-f]  - CS4294
	 *              0x3[0-7]  - CS4299
	 *              0x4[8-f]  - CS4201
	 *              0x5[8-f]  - CS4205
	 *              0x6[0-7]  - CS4291
	 *              0x7[0-7]  - CS4202
	 * Datasheets:
	 *	http://www.cirrus.com/pubs/cs4297A-5.pdf?DocumentID=593
	 *	http://www.cirrus.com/pubs/cs4294.pdf?DocumentID=32
	 *	http://www.cirrus.com/pubs/cs4299-5.pdf?DocumentID=594
	 *	http://www.cirrus.com/pubs/cs4201-2.pdf?DocumentID=492
	 *	http://www.cirrus.com/pubs/cs4205-2.pdf?DocumentID=492
	 *	http://www.cirrus.com/pubs/cs4202-1.pdf?DocumentID=852
	 */
	{ AC97_CODEC_ID('C', 'R', 'Y', 0x00),
	  0xfffffff8,			"Crystal CS4297",	},
	{ AC97_CODEC_ID('C', 'R', 'Y', 0x10),
	  0xfffffff8,			"Crystal CS4297A",	},
	{ AC97_CODEC_ID('C', 'R', 'Y', 0x20),
	  0xfffffff8,			"Crystal CS4298",	},
	{ AC97_CODEC_ID('C', 'R', 'Y', 0x28),
	  0xfffffff8,			"Crystal CS4294",	},
	{ AC97_CODEC_ID('C', 'R', 'Y', 0x30),
	  0xfffffff8,			"Crystal CS4299",	},
	{ AC97_CODEC_ID('C', 'R', 'Y', 0x48),
	  0xfffffff8,			"Crystal CS4201",	},
	{ AC97_CODEC_ID('C', 'R', 'Y', 0x58),
	  0xfffffff8,			"Crystal CS4205",	},
	{ AC97_CODEC_ID('C', 'R', 'Y', 0x60),
	  0xfffffff8,			"Crystal CS4291",	},
	{ AC97_CODEC_ID('C', 'R', 'Y', 0x70),
	  0xfffffff8,			"Crystal CS4202",	},
	{ AC97_CODEC_ID('C', 'R', 'Y', 0),
	  AC97_VENDOR_ID_MASK,		"Cirrus Logic unknown",	},

	{ 0x45838308, 0xffffffff,	"ESS Technology ES1921", },
	{ 0x45838300, AC97_VENDOR_ID_MASK, "ESS Technology unknown", },

	{ AC97_CODEC_ID('H', 'R', 'S', 0),
	  0xffffffff,			"Intersil HMP9701",	},
	{ AC97_CODEC_ID('H', 'R', 'S', 0),
	  AC97_VENDOR_ID_MASK,		"Intersil unknown",	},

	/*
	 * IC Ensemble (VIA)
	 *	http://www.viatech.com/en/datasheet/DS1616.pdf
	 */
	{ AC97_CODEC_ID('I', 'C', 'E', 0x01),
	  0xffffffff,			"ICEnsemble ICE1230/VT1611",	},
	{ AC97_CODEC_ID('I', 'C', 'E', 0x11),
	  0xffffffff,			"ICEnsemble ICE1232/VT1611A",	},
	{ AC97_CODEC_ID('I', 'C', 'E', 0x14),
	  0xffffffff,			"ICEnsemble ICE1232A",	},
	{ AC97_CODEC_ID('I', 'C', 'E', 0x51),
	  0xffffffff,			"VIA Technologies VT1616", ac97_vt1616_init },
	{ AC97_CODEC_ID('I', 'C', 'E', 0),
	  AC97_VENDOR_ID_MASK,		"ICEnsemble unknown",	},

	{ AC97_CODEC_ID('N', 'S', 'C', 0),
	  0xffffffff,			"National Semiconductor LM454[03568]", },
	{ AC97_CODEC_ID('N', 'S', 'C', 49),
	  0xffffffff,			"National Semiconductor LM4549", },
	{ AC97_CODEC_ID('N', 'S', 'C', 0),
	  AC97_VENDOR_ID_MASK,		"National Semiconductor unknown", },

	{ AC97_CODEC_ID('S', 'I', 'L', 34),
	  0xffffffff,			"Silicon Laboratory Si3036", },
	{ AC97_CODEC_ID('S', 'I', 'L', 35),
	  0xffffffff,			"Silicon Laboratory Si3038", },
	{ AC97_CODEC_ID('S', 'I', 'L', 0),
	  AC97_VENDOR_ID_MASK,		"Silicon Laboratory unknown", },

	{ AC97_CODEC_ID('T', 'R', 'A', 2),
	  0xffffffff,			"TriTech TR28022",	},
	{ AC97_CODEC_ID('T', 'R', 'A', 3),
	  0xffffffff,			"TriTech TR28023",	},
	{ AC97_CODEC_ID('T', 'R', 'A', 6),
	  0xffffffff,			"TriTech TR28026",	},
	{ AC97_CODEC_ID('T', 'R', 'A', 8),
	  0xffffffff,			"TriTech TR28028",	},
	{ AC97_CODEC_ID('T', 'R', 'A', 35),
	  0xffffffff,			"TriTech TR28602",	},
	{ AC97_CODEC_ID('T', 'R', 'A', 0),
	  AC97_VENDOR_ID_MASK,		"TriTech unknown",	},

	{ AC97_CODEC_ID('T', 'X', 'N', 0x20),
	  0xffffffff,			"Texas Instruments TLC320AD9xC", },
	{ AC97_CODEC_ID('T', 'X', 'N', 0),
	  AC97_VENDOR_ID_MASK,		"Texas Instruments unknown", },

	/*
	 * VIA
	 * http://www.viatech.com/en/multimedia/audio.jsp
	 */
	{ AC97_CODEC_ID('V', 'I', 'A', 0x61),
	  0xffffffff,			"VIA Technologies VT1612A", },
	{ AC97_CODEC_ID('V', 'I', 'A', 0),
	  AC97_VENDOR_ID_MASK,		"VIA Technologies unknown", },

	{ AC97_CODEC_ID('W', 'E', 'C', 1),
	  0xffffffff,			"Winbond W83971D",	},
	{ AC97_CODEC_ID('W', 'E', 'C', 0),
	  AC97_VENDOR_ID_MASK,		"Winbond unknown",	},

	/*
	 * http://www.wolfsonmicro.com/product_list.asp?cid=64
	 *	http://www.wolfsonmicro.com/download.asp/did.56/WM9701A.pdf - 00
	 *	http://www.wolfsonmicro.com/download.asp/did.57/WM9703.pdf  - 03
	 *	http://www.wolfsonmicro.com/download.asp/did.58/WM9704M.pdf - 04
	 *	http://www.wolfsonmicro.com/download.asp/did.59/WM9704Q.pdf - 04
	 *	http://www.wolfsonmicro.com/download.asp/did.184/WM9705_Rev34.pdf - 05
	 *	http://www.wolfsonmicro.com/download.asp/did.60/WM9707.pdf  - 03
	 *	http://www.wolfsonmicro.com/download.asp/did.136/WM9708.pdf - 03
	 *	http://www.wolfsonmicro.com/download.asp/did.243/WM9710.pdf - 05
	 */
	{ AC97_CODEC_ID('W', 'M', 'L', 0),
	  0xffffffff,			"Wolfson WM9701A",	},
	{ AC97_CODEC_ID('W', 'M', 'L', 3),
	  0xffffffff,			"Wolfson WM9703/WM9707/WM9708",	},
	{ AC97_CODEC_ID('W', 'M', 'L', 4),
	  0xffffffff,			"Wolfson WM9704",	},
	{ AC97_CODEC_ID('W', 'M', 'L', 5),
	  0xffffffff,			"Wolfson WM9705/WM9710", },
	{ AC97_CODEC_ID('W', 'M', 'L', 0),
	  AC97_VENDOR_ID_MASK,		"Wolfson unknown",	},

	/*
	 * http://www.yamaha.co.jp/english/product/lsi/us/products/pcaudio.html
	 * Datasheets:
	 *	http://www.yamaha.co.jp/english/product/lsi/us/products/pdf/4MF743A20.pdf
	 *	http://www.yamaha.co.jp/english/product/lsi/us/products/pdf/4MF753A20.pdf
	 */
	{ AC97_CODEC_ID('Y', 'M', 'H', 0),
	  0xffffffff,			"Yamaha YMF743-S",	},
	{ AC97_CODEC_ID('Y', 'M', 'H', 3),
	  0xffffffff,			"Yamaha YMF753-S",	},
	{ AC97_CODEC_ID('Y', 'M', 'H', 0),
	  AC97_VENDOR_ID_MASK,		"Yamaha unknown",	},

	/*
	 * http://www.sigmatel.com/audio/audio_codecs.htm
	 */
	{ 0x83847600, 0xffffffff,	"SigmaTel STAC9700",	},
	{ 0x83847604, 0xffffffff,	"SigmaTel STAC9701/3/4/5", },
	{ 0x83847605, 0xffffffff,	"SigmaTel STAC9704",	},
	{ 0x83847608, 0xffffffff,	"SigmaTel STAC9708",	},
	{ 0x83847609, 0xffffffff,	"SigmaTel STAC9721/23",	},
	{ 0x83847644, 0xffffffff,	"SigmaTel STAC9744/45",	},
	{ 0x83847650, 0xffffffff,	"SigmaTel STAC9750/51",	},
	{ 0x83847656, 0xffffffff,	"SigmaTel STAC9756/57",	},
	{ 0x83847666, 0xffffffff,	"SigmaTel STAC9766/67",	},
	{ 0x83847684, 0xffffffff,	"SigmaTel STAC9783/84",	},
	{ 0x83847600, AC97_VENDOR_ID_MASK, "SigmaTel unknown",	},

	{ 0,
	  0,			NULL,			}
};

static const char * const ac97enhancement[] = {
	"no 3D stereo",
	"Analog Devices Phat Stereo",
	"Creative",
	"National Semi 3D",
	"Yamaha Ymersion",
	"BBE 3D",
	"Crystal Semi 3D",
	"Qsound QXpander",
	"Spatializer 3D",
	"SRS 3D",
	"Platform Tech 3D",
	"AKM 3D",
	"Aureal",
	"AZTECH 3D",
	"Binaura 3D",
	"ESS Technology",
	"Harman International VMAx",
	"Nvidea 3D",
	"Philips Incredible Sound",
	"Texas Instruments' 3D",
	"VLSI Technology 3D",
	"TriTech 3D",
	"Realtek 3D",
	"Samsung 3D",
	"Wolfson Microelectronics 3D",
	"Delta Integration 3D",
	"SigmaTel 3D",
	"KS Waves 3D",
	"Rockwell 3D",
	"Unknown 3D",
	"Unknown 3D",
	"Unknown 3D",
};

static const char * const ac97feature[] = {
	"dedicated mic channel",
	"reserved",
	"tone",
	"simulated stereo",
	"headphone",
	"bass boost",
	"18 bit DAC",
	"20 bit DAC",
	"18 bit ADC",
	"20 bit ADC"
};


int ac97_str_equal __P((const char *, const char *));
int ac97_check_capability(struct ac97_softc *, int);
void ac97_setup_source_info __P((struct ac97_softc *));
void ac97_read __P((struct ac97_softc *, u_int8_t, u_int16_t *));
void ac97_setup_defaults __P((struct ac97_softc *));
int ac97_write __P((struct ac97_softc *, u_int8_t, u_int16_t));

/* #define AC97_DEBUG 10 */

#ifdef AUDIO_DEBUG
#define DPRINTF(x)	if (ac97debug) printf x
#define DPRINTFN(n,x)	if (ac97debug>(n)) printf x
#ifdef AC97_DEBUG
int	ac97debug = AC97_DEBUG;
#else
int	ac97debug = 0;
#endif
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif

void
ac97_read(as, reg, val)
	struct ac97_softc *as;
	u_int8_t reg;
	u_int16_t *val;
{

	if (as->host_flags & AC97_HOST_DONT_READ &&
	    (reg != AC97_REG_VENDOR_ID1 && reg != AC97_REG_VENDOR_ID2 &&
	     reg != AC97_REG_RESET)) {
		*val = as->shadow_reg[reg >> 1];
		return;
	}

	if (as->host_if->read(as->host_if->arg, reg, val)) {
		*val = as->shadow_reg[reg >> 1];
	}
}

int
ac97_write(as, reg, val)
	struct ac97_softc *as;
	u_int8_t reg;
	u_int16_t val;
{

	as->shadow_reg[reg >> 1] = val;

	return (as->host_if->write(as->host_if->arg, reg, val));
}

void
ac97_setup_defaults(as)
	struct ac97_softc *as;
{
	int idx;
	const struct ac97_source_info *si;

	memset(as->shadow_reg, 0, sizeof(as->shadow_reg));

	for (idx = 0; idx < SOURCE_INFO_SIZE; idx++) {
		si = &source_info[idx];
		ac97_write(as, si->reg, si->default_value);
	}
}

void
ac97_restore_shadow(self)
	struct ac97_codec_if *self;
{
	struct ac97_softc *as = (struct ac97_softc *) self;
	int idx;
	const struct ac97_source_info *si;

	for (idx = 0; idx < SOURCE_INFO_SIZE; idx++) {
		si = &source_info[idx];
		ac97_write(as, si->reg, as->shadow_reg[si->reg >> 1]);
	}
}

int
ac97_str_equal(a, b)
	const char *a, *b;
{
	return ((a == b) || (a && b && (!strcmp(a, b))));
}

int
ac97_check_capability(struct ac97_softc *as, int check)
{
	switch (check) {
	case CHECK_NONE:
		return 1;
	case CHECK_SURROUND:
		return as->ext_id & AC97_EXT_AUDIO_SDAC;
	case CHECK_CENTER:
		return as->ext_id & AC97_EXT_AUDIO_CDAC;
	case CHECK_LFE:
		return as->ext_id & AC97_EXT_AUDIO_LDAC;
	case CHECK_HEADPHONES:
		return as->caps & AC97_CAPS_HEADPHONES;
	case CHECK_TONE:
		return as->caps & AC97_CAPS_TONECTRL;
	case CHECK_MIC:
		return as->caps & AC97_CAPS_MICIN;
	case CHECK_LOUDNESS:
		return as->caps & AC97_CAPS_LOUDNESS;
	case CHECK_3D:
		return AC97_CAPS_ENHANCEMENT(as->caps) != 0;
	default:
		printf("%s: internal error: feature=%d\n", __func__, check);
		return 0;
	}
}

void
ac97_setup_source_info(as)
	struct ac97_softc *as;
{
	int idx, ouridx;
	struct ac97_source_info *si, *si2;

	for (idx = 0, ouridx = 0; idx < SOURCE_INFO_SIZE; idx++) {
		si = &as->source_info[ouridx];

		if (!ac97_check_capability(as, source_info[idx].req_feature))
			continue;

		memcpy(si, &source_info[idx], sizeof(*si));

		switch (si->type) {
		case AUDIO_MIXER_CLASS:
			si->mixer_class = ouridx;
			ouridx++;
			break;
		case AUDIO_MIXER_VALUE:
			/* Todo - Test to see if it works */
			ouridx++;

			/* Add an entry for mute, if necessary */
			if (si->mute) {
				si = &as->source_info[ouridx];
				memcpy(si, &source_info[idx], sizeof(*si));
				si->qualifier = AudioNmute;
				si->type = AUDIO_MIXER_ENUM;
				si->info = &ac97_on_off;
				si->info_size = sizeof(ac97_on_off);
				si->bits = 1;
				si->ofs = 15;
				si->mute = 0;
				si->polarity = 0;
				ouridx++;
			}
			break;
		case AUDIO_MIXER_ENUM:
			/* Todo - Test to see if it works */
			ouridx++;
			break;
		default:
			printf ("ac97: shouldn't get here\n");
			break;
		}
	}

	as->num_source_info = ouridx;

	for (idx = 0; idx < as->num_source_info; idx++) {
		int idx2, previdx;

		si = &as->source_info[idx];

		/* Find mixer class */
		for (idx2 = 0; idx2 < as->num_source_info; idx2++) {
			si2 = &as->source_info[idx2];

			if (si2->type == AUDIO_MIXER_CLASS &&
			    ac97_str_equal(si->class,
					   si2->class)) {
				si->mixer_class = idx2;
			}
		}


		/* Setup prev and next pointers */
		if (si->prev != 0)
			continue;

		if (si->qualifier)
			continue;

		si->prev = AUDIO_MIXER_LAST;
		previdx = idx;

		for (idx2 = 0; idx2 < as->num_source_info; idx2++) {
			if (idx2 == idx)
				continue;

			si2 = &as->source_info[idx2];

			if (!si2->prev &&
			    ac97_str_equal(si->class, si2->class) &&
			    ac97_str_equal(si->device, si2->device)) {
				as->source_info[previdx].next = idx2;
				as->source_info[idx2].prev = previdx;

				previdx = idx2;
			}
		}

		as->source_info[previdx].next = AUDIO_MIXER_LAST;
	}
}

int
ac97_attach(host_if)
	struct ac97_host_if *host_if;
{
	struct ac97_softc *as;
	struct device *sc_dev = (struct device *)host_if->arg;
	int error, i, j;
	u_int32_t id;
	u_int16_t id1, id2;
	u_int16_t extstat, rate;
	mixer_ctrl_t ctl;
	const char *delim;
	void (*initfunc)(struct ac97_softc *);

	initfunc = NULL;
	as = malloc(sizeof(struct ac97_softc), M_DEVBUF, M_WAITOK|M_ZERO);

	if (as == NULL)
		return (ENOMEM);

	as->codec_if.vtbl = &ac97civ;
	as->host_if = host_if;

	if ((error = host_if->attach(host_if->arg, &as->codec_if))) {
		free(as, M_DEVBUF);
		return (error);
	}

	host_if->reset(host_if->arg);

	host_if->write(host_if->arg, AC97_REG_POWER, 0);
	host_if->write(host_if->arg, AC97_REG_RESET, 0);

	if (host_if->flags)
		as->host_flags = host_if->flags(host_if->arg);

	ac97_setup_defaults(as);
	ac97_read(as, AC97_REG_VENDOR_ID1, &id1);
	ac97_read(as, AC97_REG_VENDOR_ID2, &id2);
	ac97_read(as, AC97_REG_RESET, &as->caps);

	id = (id1 << 16) | id2;

	printf("%s: ", sc_dev->dv_xname);

	for (i = 0; ; i++) {
		if (ac97codecid[i].id == 0) {
			char pnp[4];

			AC97_GET_CODEC_ID(id, pnp);
#define ISASCII(c) ((c) >= ' ' && (c) < 0x7f)
			if (ISASCII(pnp[0]) && ISASCII(pnp[1]) &&
			    ISASCII(pnp[2]))
				printf("%c%c%c%d", pnp[0], pnp[1], pnp[2],
				    pnp[3]);
			else
				printf("unknown (0x%08x)", id);
			break;
		}
		if (ac97codecid[i].id == (id & ac97codecid[i].mask)) {
			printf("%s", ac97codecid[i].name);
			if (ac97codecid[i].mask == AC97_VENDOR_ID_MASK) {
				printf(" (0x%08x)", id);
			}
			initfunc = ac97codecid[i].init;
			break;
		}
	}
	printf(" codec; ");
	for (i = j = 0; i < 10; i++) {
		if (as->caps & (1 << i)) {
			printf("%s%s", j? ", " : "", ac97feature[i]);
			j++;
		}
	}
	printf("%s%s\n", j ? ", " : "",
	       ac97enhancement[AC97_CAPS_ENHANCEMENT(as->caps)]);

	as->ac97_clock = AC97_STANDARD_CLOCK;
	ac97_read(as, AC97_REG_EXT_AUDIO_ID, &as->ext_id);
	if (as->ext_id & (AC97_EXT_AUDIO_VRA | AC97_EXT_AUDIO_DRA
			  | AC97_EXT_AUDIO_SPDIF | AC97_EXT_AUDIO_VRM
			  | AC97_EXT_AUDIO_CDAC | AC97_EXT_AUDIO_SDAC
			  | AC97_EXT_AUDIO_LDAC)) {
		printf("%s:", sc_dev->dv_xname);
		delim = "";

		ac97_read(as, AC97_REG_EXT_AUDIO_CTRL, &extstat);
		if (as->ext_id & AC97_EXT_AUDIO_VRA) {
			printf("%s variable rate audio", delim);
			delim = ",";
			extstat |= AC97_EXT_AUDIO_VRA;
		}
		if (as->ext_id & AC97_EXT_AUDIO_DRA) {
			printf("%s double rate output", delim);
			delim = ",";
		}
		extstat &= ~AC97_EXT_AUDIO_DRA;
		if (as->ext_id & AC97_EXT_AUDIO_SPDIF) {
			printf("%s S/PDIF", delim);
			delim = ",";
		}
		if (as->ext_id & AC97_EXT_AUDIO_VRM) {
			printf("%s variable rate dedicated mic", delim);
			delim = ",";
			extstat |= AC97_EXT_AUDIO_VRM;
		}
		if (as->ext_id & AC97_EXT_AUDIO_CDAC) {
			printf("%s center DAC", delim);
			delim = ",";
			extstat |= AC97_EXT_AUDIO_CDAC;
		}
		if (as->ext_id & AC97_EXT_AUDIO_SDAC) {
			printf("%s surround DAC", delim);
			delim = ",";
			extstat |= AC97_EXT_AUDIO_SDAC;
		}
		if (as->ext_id & AC97_EXT_AUDIO_LDAC) {
			printf("%s LFE DAC", delim);
			extstat |= AC97_EXT_AUDIO_LDAC;
		}
		printf("\n");

		ac97_write(as, AC97_REG_EXT_AUDIO_CTRL, extstat);
		if (as->ext_id & AC97_EXT_AUDIO_VRA) {
			/* VRA should be enabled. */
			/* so it claims to do variable rate, let's make sure */
			ac97_write(as, AC97_REG_PCM_FRONT_DAC_RATE, 44100);
			ac97_read(as, AC97_REG_PCM_FRONT_DAC_RATE, &rate);
			if (rate != 44100) {
				/* We can't believe ext_id */
				as->ext_id = 0;
				printf("%s: Ignore these capabilities.\n",
				       sc_dev->dv_xname);
			}
			/* restore the default value */
			ac97_write(as, AC97_REG_PCM_FRONT_DAC_RATE,
				   AC97_SINGLE_RATE);
		}
	}

	ac97_setup_source_info(as);

	DELAY(900 * 1000);
	memset(&ctl, 0, sizeof(ctl));

	/* disable mutes */
	for (i = 0; i < 11; i++) {
		static struct {
			char *class, *device;
		} d[11] = {
			{ AudioCoutputs, AudioNmaster},
			{ AudioCoutputs, AudioNheadphone},
			{ AudioCoutputs, AudioNsurround},
			{ AudioCoutputs, AudioNcenter},
			{ AudioCoutputs, AudioNlfe},
			{ AudioCinputs, AudioNdac},
			{ AudioCinputs, AudioNcd},
			{ AudioCinputs, AudioNline},
			{ AudioCinputs, AudioNaux},
			{ AudioCinputs, AudioNvideo},
			{ AudioCrecord, AudioNvolume},
		};

		ctl.type = AUDIO_MIXER_ENUM;
		ctl.un.ord = 0;

		ctl.dev = ac97_get_portnum_by_name(&as->codec_if,
			d[i].class, d[i].device, AudioNmute);
		ac97_mixer_set_port(&as->codec_if, &ctl);
	}
	ctl.type = AUDIO_MIXER_ENUM;
	ctl.un.ord = 0;
	ctl.dev = ac97_get_portnum_by_name(&as->codec_if, AudioCrecord,
					   AudioNsource, NULL);
	ac97_mixer_set_port(&as->codec_if, &ctl);

	/* set a reasonable default volume */
	ctl.type = AUDIO_MIXER_VALUE;
	ctl.un.value.num_channels = 2;
	ctl.un.value.level[AUDIO_MIXER_LEVEL_LEFT] = \
	ctl.un.value.level[AUDIO_MIXER_LEVEL_RIGHT] = 127;
	ctl.dev = ac97_get_portnum_by_name(&as->codec_if, AudioCoutputs,
					   AudioNmaster, NULL);
	ac97_mixer_set_port(&as->codec_if, &ctl);
	ctl.dev = ac97_get_portnum_by_name(&as->codec_if, AudioCoutputs,
					   AudioNsurround, NULL);
	ac97_mixer_set_port(&as->codec_if, &ctl);
	ctl.un.value.num_channels = 1;
	ctl.dev = ac97_get_portnum_by_name(&as->codec_if, AudioCoutputs,
					   AudioNcenter, NULL);
	ac97_mixer_set_port(&as->codec_if, &ctl);
	ctl.dev = ac97_get_portnum_by_name(&as->codec_if, AudioCoutputs,
					   AudioNlfe, NULL);
	ac97_mixer_set_port(&as->codec_if, &ctl);

	if (initfunc != NULL)
		initfunc(as);
	return (0);
}


int
ac97_query_devinfo(codec_if, dip)
	struct ac97_codec_if *codec_if;
	mixer_devinfo_t *dip;
{
	struct ac97_softc *as = (struct ac97_softc *)codec_if;

	if (dip->index < as->num_source_info) {
		struct ac97_source_info *si = &as->source_info[dip->index];
		const char *name;

		dip->type = si->type;
		dip->mixer_class = si->mixer_class;
		dip->prev = si->prev;
		dip->next = si->next;

		if (si->qualifier)
			name = si->qualifier;
		else if (si->device)
			name = si->device;
		else if (si->class)
			name = si->class;
		else
			name = 0;

		if (name)
			strcpy(dip->label.name, name);

		memcpy(&dip->un, si->info, si->info_size);

		/* Set the delta for volume sources */
		if (dip->type == AUDIO_MIXER_VALUE)
			dip->un.v.delta = 1 << (8 - si->bits);

		return (0);
	}

	return (ENXIO);
}



int
ac97_mixer_set_port(codec_if, cp)
	struct ac97_codec_if *codec_if;
	mixer_ctrl_t *cp;
{
	struct ac97_softc *as = (struct ac97_softc *)codec_if;
	struct ac97_source_info *si = &as->source_info[cp->dev];
	u_int16_t mask;
	u_int16_t val, newval;
	int error;

	if (cp->dev < 0 || cp->dev >= as->num_source_info)
		return (EINVAL);

	if (cp->type != si->type)
		return (EINVAL);

	ac97_read(as, si->reg, &val);

	DPRINTFN(5, ("read(%x) = %x\n", si->reg, val));

	mask = (1 << si->bits) - 1;

	switch (cp->type) {
	case AUDIO_MIXER_ENUM:
		if (cp->un.ord > mask || cp->un.ord < 0)
			return (EINVAL);

		newval = (cp->un.ord << si->ofs);
		if (si->reg == AC97_REG_RECORD_SELECT) {
			newval |= (newval << (8 + si->ofs));
			mask |= (mask << 8);
			mask = mask << si->ofs;
		} else if (si->reg == AC97_REG_SURR_MASTER) {
			newval = cp->un.ord ? 0x8080 : 0x0000;
			mask = 0x8080;
		} else
			mask = mask << si->ofs;
		break;
	case AUDIO_MIXER_VALUE:
	{
		const struct audio_mixer_value *value = si->info;
		u_int16_t  l, r;

		if ((cp->un.value.num_channels <= 0) ||
		    (cp->un.value.num_channels > value->num_channels))
			return (EINVAL);

		if (cp->un.value.num_channels == 1) {
			l = r = cp->un.value.level[AUDIO_MIXER_LEVEL_MONO];
		} else {
			if (!(as->host_flags & AC97_HOST_SWAPPED_CHANNELS)) {
				l = cp->un.value.level[AUDIO_MIXER_LEVEL_LEFT];
				r = cp->un.value.level[AUDIO_MIXER_LEVEL_RIGHT];
			} else {	/* left/right is reversed here */
				r = cp->un.value.level[AUDIO_MIXER_LEVEL_LEFT];
				l = cp->un.value.level[AUDIO_MIXER_LEVEL_RIGHT];
			}

		}

		if (!si->polarity) {
			l = 255 - l;
			r = 255 - r;
		}

		l = l >> (8 - si->bits);
		r = r >> (8 - si->bits);

		newval = ((l & mask) << si->ofs);
		if (value->num_channels == 2) {
			newval = (newval << 8) | ((r & mask) << si->ofs);
			mask |= (mask << 8);
		}
		mask = mask << si->ofs;
		break;
	}
	default:
		return (EINVAL);
	}

	error = ac97_write(as, si->reg, (val & ~mask) | newval);
	if (error)
		return (error);

	return (0);
}

int
ac97_get_portnum_by_name(codec_if, class, device, qualifier)
	struct ac97_codec_if *codec_if;
	const char *class, *device, *qualifier;
{
	struct ac97_softc *as = (struct ac97_softc *)codec_if;
	int idx;

	for (idx = 0; idx < as->num_source_info; idx++) {
		struct ac97_source_info *si = &as->source_info[idx];
		if (ac97_str_equal(class, si->class) &&
		    ac97_str_equal(device, si->device) &&
		    ac97_str_equal(qualifier, si->qualifier))
			return (idx);
	}

	return (-1);
}

int
ac97_mixer_get_port(codec_if, cp)
	struct ac97_codec_if *codec_if;
	mixer_ctrl_t *cp;
{
	struct ac97_softc *as = (struct ac97_softc *)codec_if;
	struct ac97_source_info *si = &as->source_info[cp->dev];
	u_int16_t mask;
	u_int16_t val;

	if (cp->dev < 0 || cp->dev >= as->num_source_info)
		return (EINVAL);

	if (cp->type != si->type)
		return (EINVAL);

	ac97_read(as, si->reg, &val);

	DPRINTFN(5, ("read(%x) = %x\n", si->reg, val));

	mask = (1 << si->bits) - 1;

	switch (cp->type) {
	case AUDIO_MIXER_ENUM:
		cp->un.ord = (val >> si->ofs) & mask;
		DPRINTFN(4, ("AUDIO_MIXER_ENUM: %x %d %x %d\n",
			     val, si->ofs, mask, cp->un.ord));
		break;
	case AUDIO_MIXER_VALUE:
	{
		const struct audio_mixer_value *value = si->info;
		u_int16_t  l, r;

		if ((cp->un.value.num_channels <= 0) ||
		    (cp->un.value.num_channels > value->num_channels))
			return (EINVAL);

		if (value->num_channels == 1) {
			l = r = (val >> si->ofs) & mask;
		} else {
			if (!(as->host_flags & AC97_HOST_SWAPPED_CHANNELS)) {
				l = (val >> (si->ofs + 8)) & mask;
				r = (val >> si->ofs) & mask;
			} else {	/* host has reversed channels */
				r = (val >> (si->ofs + 8)) & mask;
				l = (val >> si->ofs) & mask;
			}
		}

		l = (l << (8 - si->bits));
		r = (r << (8 - si->bits));
		if (!si->polarity) {
			l = 255 - l;
			r = 255 - r;
		}

		/* The EAP driver averages l and r for stereo
		   channels that are requested in MONO mode. Does this
		   make sense? */
		if (cp->un.value.num_channels == 1) {
			cp->un.value.level[AUDIO_MIXER_LEVEL_MONO] = l;
		} else if (cp->un.value.num_channels == 2) {
			cp->un.value.level[AUDIO_MIXER_LEVEL_LEFT] = l;
			cp->un.value.level[AUDIO_MIXER_LEVEL_RIGHT] = r;
		}

		break;
	}
	default:
		return (EINVAL);
	}

	return (0);
}


int
ac97_set_rate(struct ac97_codec_if *codec_if, int target, u_long *rate)
{
	struct ac97_softc *as;
	u_long value;
	u_int16_t ext_stat;
	u_int16_t actual;
	u_int16_t power;
	u_int16_t power_bit;

	as = (struct ac97_softc *)codec_if;
	if (target == AC97_REG_PCM_MIC_ADC_RATE) {
		if (!(as->ext_id & AC97_EXT_AUDIO_VRM)) {
			*rate = AC97_SINGLE_RATE;
			return 0;
		}
	} else {
		if (!(as->ext_id & AC97_EXT_AUDIO_VRA)) {
			*rate = AC97_SINGLE_RATE;
			return 0;
		}
	}
	value = *rate * AC97_STANDARD_CLOCK / as->ac97_clock;
	ext_stat = 0;
	/*
	 * PCM_FRONT_DAC_RATE/PCM_SURR_DAC_RATE/PCM_LFE_DAC_RATE
	 *	Check VRA, DRA
	 * PCM_LR_ADC_RATE
	 *	Check VRA
	 * PCM_MIC_ADC_RATE
	 *	Check VRM
	 */
	switch (target) {
	case AC97_REG_PCM_FRONT_DAC_RATE:
	case AC97_REG_PCM_SURR_DAC_RATE:
	case AC97_REG_PCM_LFE_DAC_RATE:
		power_bit = AC97_POWER_OUT;
		if (!(as->ext_id & AC97_EXT_AUDIO_VRA)) {
			*rate = AC97_SINGLE_RATE;
			return 0;
		}
		if (as->ext_id & AC97_EXT_AUDIO_DRA) {
			ac97_read(as, AC97_REG_EXT_AUDIO_CTRL, &ext_stat);
			if (value > 0x1ffff) {
				return EINVAL;
			} else if (value > 0xffff) {
				/* Enable DRA */
				ext_stat |= AC97_EXT_AUDIO_DRA;
				ac97_write(as, AC97_REG_EXT_AUDIO_CTRL, ext_stat);
				value /= 2;
			} else {
				/* Disable DRA */
				ext_stat &= ~AC97_EXT_AUDIO_DRA;
				ac97_write(as, AC97_REG_EXT_AUDIO_CTRL, ext_stat);
			}
		} else {
			if (value > 0xffff)
				return EINVAL;
		}
		break;
	case AC97_REG_PCM_LR_ADC_RATE:
		power_bit = AC97_POWER_IN;
		if (!(as->ext_id & AC97_EXT_AUDIO_VRA)) {
			*rate = AC97_SINGLE_RATE;
			return 0;
		}
		if (value > 0xffff)
			return EINVAL;
		break;
	case AC97_REG_PCM_MIC_ADC_RATE:
		power_bit = AC97_POWER_IN;
		if (!(as->ext_id & AC97_EXT_AUDIO_VRM)) {
			*rate = AC97_SINGLE_RATE;
			return 0;
		}
		if (value > 0xffff)
			return EINVAL;
		break;
	default:
		printf("%s: Unknown register: 0x%x\n", __func__, target);
		return EINVAL;
	}

	ac97_read(as, AC97_REG_POWER, &power);
	ac97_write(as, AC97_REG_POWER, power | power_bit);

	ac97_write(as, target, (u_int16_t)value);
	ac97_read(as, target, &actual);
	actual = (u_int32_t)actual * as->ac97_clock / AC97_STANDARD_CLOCK;

	ac97_write(as, AC97_REG_POWER, power);
	if (ext_stat & AC97_EXT_AUDIO_DRA) {
		*rate = actual * 2;
	} else {
		*rate = actual;
	}
	return 0;
}

void
ac97_set_clock(struct ac97_codec_if *codec_if, unsigned int clock)
{
	struct ac97_softc *as;

	as = (struct ac97_softc *)codec_if;
	as->ac97_clock = clock;
}

u_int16_t
ac97_get_extcaps(struct ac97_codec_if *codec_if)
{
	struct ac97_softc *as;

	as = (struct ac97_softc *)codec_if;
	return as->ext_id;
}

int
ac97_add_port(struct ac97_softc *as, const struct ac97_source_info *src)
{
	struct ac97_source_info *si;
	int ouridx, idx;

	if (as->num_source_info >= MAX_SOURCES) {
		printf("%s: internal error: increase MAX_SOURCES in %s\n",
		       __func__, __FILE__);
		return -1;
	}
	if (!ac97_check_capability(as, src->req_feature))
		return -1;
	ouridx = as->num_source_info;
	si = &as->source_info[ouridx];
	memcpy(si, src, sizeof(*si));

	switch (si->type) {
	case AUDIO_MIXER_CLASS:
	case AUDIO_MIXER_VALUE:
		printf("%s: adding class/value is not supported yet.\n",
		       __func__);
		return -1;
	case AUDIO_MIXER_ENUM:
		break;
	default:
		printf("%s: unknown type: %d\n", __func__, si->type);
		return -1;
	}
	as->num_source_info++;

	si->mixer_class = ac97_get_portnum_by_name(&as->codec_if, si->class,
						   NULL, NULL);
	/* Find the root of the device */
	idx = ac97_get_portnum_by_name(&as->codec_if, si->class,
				       si->device, NULL);
	/* Find the last item */
	while (as->source_info[idx].next != AUDIO_MIXER_LAST)
		idx = as->source_info[idx].next;
	/* Append */
	as->source_info[idx].next = ouridx;
	si->prev = idx;
	si->next = AUDIO_MIXER_LAST;

	return 0;
}

#define ALC650_REG_MULTI_CHANNEL_CONTROL	0x6a
#define		ALC650_MCC_SLOT_MODIFY_MASK		0xc000
#define		ALC650_MCC_FRONTDAC_FROM_SPDIFIN	0x2000 /* 13 */
#define		ALC650_MCC_SPDIFOUT_FROM_ADC		0x1000 /* 12 */
#define		ALC650_MCC_PCM_FROM_SPDIFIN		0x0800 /* 11 */
#define		ALC650_MCC_MIC_OR_CENTERLFE		0x0400 /* 10 */
#define		ALC650_MCC_LINEIN_OR_SURROUND		0x0200 /* 9 */
#define		ALC650_MCC_INDEPENDENT_MASTER_L		0x0080 /* 7 */
#define		ALC650_MCC_INDEPENDENT_MASTER_R		0x0040 /* 6 */
#define		ALC650_MCC_ANALOG_TO_CENTERLFE		0x0020 /* 5 */
#define		ALC650_MCC_ANALOG_TO_SURROUND		0x0010 /* 4 */
#define		ALC650_MCC_EXCHANGE_CENTERLFE		0x0008 /* 3 */
#define		ALC650_MCC_CENTERLFE_DOWNMIX		0x0004 /* 2 */
#define		ALC650_MCC_SURROUND_DOWNMIX		0x0002 /* 1 */
#define		ALC650_MCC_LINEOUT_TO_SURROUND		0x0001 /* 0 */
static void
ac97_alc650_init(struct ac97_softc *as)
{
	static const struct ac97_source_info sources[6] = {
		{ AudioCoutputs, AudioNsurround, "lineinjack",
		  AUDIO_MIXER_ENUM, WRAP(ac97_on_off),
		  ALC650_REG_MULTI_CHANNEL_CONTROL,
		  0x0000, 1, 9, 0, 0, CHECK_SURROUND },
		{ AudioCoutputs, AudioNsurround, "mixtofront",
		  AUDIO_MIXER_ENUM, WRAP(ac97_on_off),
		  ALC650_REG_MULTI_CHANNEL_CONTROL,
		  0x0000, 1, 1, 0, 0, CHECK_SURROUND },
		{ AudioCoutputs, AudioNcenter, "micjack",
		  AUDIO_MIXER_ENUM, WRAP(ac97_on_off),
		  ALC650_REG_MULTI_CHANNEL_CONTROL,
		  0x0000, 1, 10, 0, 0, CHECK_CENTER },
		{ AudioCoutputs, AudioNlfe, "micjack",
		  AUDIO_MIXER_ENUM, WRAP(ac97_on_off),
		  ALC650_REG_MULTI_CHANNEL_CONTROL,
		  0x0000, 1, 10, 0, 0, CHECK_LFE },
		{ AudioCoutputs, AudioNcenter, "mixtofront",
		  AUDIO_MIXER_ENUM, WRAP(ac97_on_off),
		  ALC650_REG_MULTI_CHANNEL_CONTROL,
		  0x0000, 1, 2, 0, 0, CHECK_CENTER },
		{ AudioCoutputs, AudioNlfe, "mixtofront",
		  AUDIO_MIXER_ENUM, WRAP(ac97_on_off),
		  ALC650_REG_MULTI_CHANNEL_CONTROL,
		  0x0000, 1, 2, 0, 0, CHECK_LFE },
	};

	ac97_add_port(as, &sources[0]);
	ac97_add_port(as, &sources[1]);
	ac97_add_port(as, &sources[2]);
	ac97_add_port(as, &sources[3]);
	ac97_add_port(as, &sources[4]);
	ac97_add_port(as, &sources[5]);
}

#define VT1616_REG_IO_CONTROL	0x5a
#define		VT1616_IC_LVL			(1 << 15)
#define		VT1616_IC_LFECENTER_TO_FRONT	(1 << 12)
#define		VT1616_IC_SURROUND_TO_FRONT	(1 << 11)
#define		VT1616_IC_BPDC			(1 << 10)
#define		VT1616_IC_DC			(1 << 9)
#define		VT1616_IC_IB_MASK		0x000c
static void
ac97_vt1616_init(struct ac97_softc *as)
{
	static const struct ac97_source_info sources[3] = {
		{ AudioCoutputs, AudioNsurround, "mixtofront",
		  AUDIO_MIXER_ENUM, WRAP(ac97_on_off),
		  VT1616_REG_IO_CONTROL,
		  0x0000, 1, 11, 0, 0, CHECK_SURROUND },
		{ AudioCoutputs, AudioNcenter, "mixtofront",
		  AUDIO_MIXER_ENUM, WRAP(ac97_on_off),
		  VT1616_REG_IO_CONTROL,
		  0x0000, 1, 12, 0, 0, CHECK_CENTER },
		{ AudioCoutputs, AudioNlfe, "mixtofront",
		  AUDIO_MIXER_ENUM, WRAP(ac97_on_off),
		  VT1616_REG_IO_CONTROL,
		  0x0000, 1, 12, 0, 0, CHECK_LFE },
	};

	ac97_add_port(as, &sources[0]);
	ac97_add_port(as, &sources[1]);
	ac97_add_port(as, &sources[2]);
}

