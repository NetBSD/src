/*	$NetBSD: ac97reg.h,v 1.9 2003/09/07 11:27:33 kent Exp $	*/

/*
 * Copyright (c) 1999 Constantine Sapuntzakis
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
 *
 * THIS SOFTWARE IS PROVIDED BY CONSTANTINE SAPUNTZAKIS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#define	AC97_REG_RESET			0x00
#define		AC97_CAPS_MICIN			0x0001
#define		AC97_CAPS_TONECTRL		0x0004
#define		AC97_CAPS_SIMSTEREO		0x0008
#define		AC97_CAPS_HEADPHONES		0x0010
#define		AC97_CAPS_LOUDNESS		0x0020
#define		AC97_CAPS_DAC18			0x0040
#define		AC97_CAPS_DAC20			0x0080
#define		AC97_CAPS_ADC18			0x0100
#define		AC97_CAPS_ADC20			0x0200
#define		AC97_CAPS_ENHANCEMENT_MASK	0xfc00
#define		AC97_CAPS_ENHANCEMENT_SHIFT	10
#define		AC97_CAPS_ENHANCEMENT(reg)	(((reg) >> 10) & 0x1f)
#define	AC97_REG_MASTER_VOLUME		0x02
#define	AC97_REG_HEADPHONE_VOLUME	0x04
#define	AC97_REG_MASTER_VOLUME_MONO	0x06
#define	AC97_REG_MASTER_TONE		0x08
#define	AC97_REG_PCBEEP_VOLUME		0x0a
#define	AC97_REG_PHONE_VOLUME		0x0c
#define	AC97_REG_MIC_VOLUME		0x0e
#define	AC97_REG_LINEIN_VOLUME		0x10
#define	AC97_REG_CD_VOLUME		0x12
#define	AC97_REG_VIDEO_VOLUME		0x14
#define	AC97_REG_AUX_VOLUME		0x16
#define	AC97_REG_PCMOUT_VOLUME		0x18
#define	AC97_REG_RECORD_SELECT		0x1a
#define	AC97_REG_RECORD_GAIN		0x1c
#define	AC97_REG_RECORD_GAIN_MIC	0x1e /* for dedicated mic */
#define	AC97_REG_GP			0x20
#define	AC97_REG_3D_CONTROL		0x22
				/*	0x24	Modem sample rate in AC97 1.03
						Reserved in AC97 2.0
						Interrupt/paging in AC97 2.3 */
#define	AC97_REG_POWER			0x26
#define		AC97_POWER_ADC			0x0001
#define		AC97_POWER_DAC			0x0002
#define		AC97_POWER_ANL			0x0004
#define		AC97_POWER_REF			0x0008
#define		AC97_POWER_IN			0x0100
#define		AC97_POWER_OUT			0x0200
#define		AC97_POWER_MIXER		0x0400
#define		AC97_POWER_MIXER_VREF		0x0800
#define		AC97_POWER_ACLINK		0x1000
#define		AC97_POWER_CLK			0x2000
#define		AC97_POWER_AUX			0x4000
#define		AC97_POWER_EAMP			0x8000

/* AC'97 2.0 extensions -- 0x28-0x3a */
#define AC97_REG_EXT_AUDIO_ID		0x28
#define AC97_REG_EXT_AUDIO_CTRL		0x2a
#define		AC97_EXT_AUDIO_VRA		0x0001
#define		AC97_EXT_AUDIO_DRA		0x0002
#define		AC97_EXT_AUDIO_SPDIF		0x0004
#define		AC97_EXT_AUDIO_VRM		0x0008 /* for dedicated mic */
#define		AC97_EXT_AUDIO_DSA_MASK		0x0030 /* for EXT ID */
#define		 AC97_EXT_AUDIO_DSA00		0x0000 /* for EXT ID */
#define		 AC97_EXT_AUDIO_DSA01		0x0010 /* for EXT ID */
#define		 AC97_EXT_AUDIO_DSA10		0x0020 /* for EXT ID */
#define		 AC97_EXT_AUDIO_DSA11		0x0030 /* for EXT ID */
#define		AC97_EXT_AUDIO_SPSA_MASK	0x0030 /* for EXT CTRL */
#define		 AC97_EXT_AUDIO_SPSA34		0x0000 /* for EXT CTRL */
#define		 AC97_EXT_AUDIO_SPSA78		0x0010 /* for EXT CTRL */
#define		 AC97_EXT_AUDIO_SPSA69		0x0020 /* for EXT CTRL */
#define		 AC97_EXT_AUDIO_SPSAAB		0x0030 /* for EXT CTRL */
#define		AC97_EXT_AUDIO_CDAC		0x0040
#define		AC97_EXT_AUDIO_SDAC		0x0080
#define		AC97_EXT_AUDIO_LDAC		0x0100
#define		AC97_EXT_AUDIO_AMAP		0x0200 /* for EXT ID */
#define		AC97_EXT_AUDIO_REV_MASK		0x0C00 /* for EXT ID */
#define		 AC97_EXT_AUDIO_REV_11		0x0000 /* for EXT ID */
#define		 AC97_EXT_AUDIO_REV_22		0x0400 /* for EXT ID */
#define		 AC97_EXT_AUDIO_REV_23		0x0800 /* for EXT ID */
#define		 AC97_EXT_AUDIO_REV_RESERVED11	0x0c00 /* for EXT ID */
#define		AC97_EXT_AUDIO_ID_MASK		0xC000 /* for EXT ID */
#define		 AC97_EXT_AUDIO_ID_PRIMARY	0x0000 /* for EXT ID */
#define		 AC97_EXT_AUDIO_ID_SECONDARY01	0x4000 /* for EXT ID */
#define		 AC97_EXT_AUDIO_ID_SECONDARY10	0x8000 /* for EXT ID */
#define		 AC97_EXT_AUDIO_ID_SECONDARY11	0xc000 /* for EXT ID */
#define		AC97_EXT_AUDIO_MADC		0x0200 /* for EXT CTRL */
#define		AC97_EXT_AUDIO_SPCV		0x0400 /* for EXT CTRL */
#define		AC97_EXT_AUDIO_PRI		0x0800 /* for EXT CTRL */
#define		AC97_EXT_AUDIO_PRJ		0x1000 /* for EXT CTRL */
#define		AC97_EXT_AUDIO_PRK		0x2000 /* for EXT CTRL */
#define		AC97_EXT_AUDIO_PRL		0x4000 /* for EXT CTRL */
#define		AC97_EXT_AUDIO_VCFG		0x8000 /* for EXT CTRL */

#define		AC97_SINGLE_RATE		48000
#define	AC97_REG_PCM_FRONT_DAC_RATE	0x2c
#define	AC97_REG_PCM_SURR_DAC_RATE	0x2e
#define	AC97_REG_PCM_LFE_DAC_RATE	0x30
#define	AC97_REG_PCM_LR_ADC_RATE	0x32
#define	AC97_REG_PCM_MIC_ADC_RATE	0x34	/* dedicated mic */
#define	AC97_REG_CENTER_LFE_MASTER	0x36	/* center + LFE master volume */
#define	AC97_REG_SURR_MASTER		0x38	/* surround (rear) master vol */
#define AC97_REG_SPDIF_CTRL		0x3a
#define		AC97_SPDIF_V			0x8000
#define		AC97_SPDIF_DRS			0x4000
#define		AC97_SPDIF_SPSR_MASK		0x3000
#define		 AC97_SPDIF_SPSR_44K		0x0000
#define		 AC97_SPDIF_SPSR_48K		0x2000
#define		 AC97_SPDIF_SPSR_32K		0x1000
#define		AC97_SPDIF_L			0x0800
#define		AC97_SPDIF_CC_MASK		0x07f0
#define		AC97_SPDIF_PRE			0x0008
#define		AC97_SPDIF_COPY			0x0004
#define		AC97_SPDIF_NONAUDIO		0x0002
#define		AC97_SPDIF_PRO			0x0001

/* Modem -- 0x3c-0x58 */

/* Vendor specific -- 0x5a-0x7b */

#define	AC97_REG_VENDOR_ID1		0x7c
#define	AC97_REG_VENDOR_ID2		0x7e
#define		AC97_VENDOR_ID_MASK		0xffffff00

#define	AC97_CODEC_ID(a0, a1, a2, x)					\
	(((a0) << 24) | ((a1) << 16) | ((a2) << 8) | (x))

#define	AC97_GET_CODEC_ID(id, cp)					\
do {									\
	(cp)[0] = ((id) >> 24) & 0xff;					\
	(cp)[1] = ((id) >> 16) & 0xff;					\
	(cp)[2] = ((id) >> 8)  & 0xff;					\
	(cp)[3] = (id) & 0xff;						\
} while (0)
