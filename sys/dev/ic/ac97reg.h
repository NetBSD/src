/*	$NetBSD: ac97reg.h,v 1.3 2001/01/05 03:32:46 augustss Exp $	*/

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
#define	AC97_SOUND_ENHANCEMENT(reg)	(((reg) >> 10) & 0x1f)
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
#define	AC97_REG_RECORD_GAIN_MIC	0x1e
#define	AC97_REG_GP			0x20
#define	AC97_REG_3D_CONTROL		0x22
				/*	0x24	reserved	*/
#define	AC97_REG_POWER			0x26

/* AC'97 2.0 extensions -- 0x28-0x3a */
#define	AC97_REG_EXTENDED_ID		0x28
#define		AC97_CODEC_DOES_VRA		0x0001
#define	AC97_REG_EXTENDED_STATUS	0x2a
#define		AC97_ENAB_VRA			0x0001
#define		AC97_ENAB_MICVRA		0x0004
#define	AC97_REG_PCM_FRONT_DAC_RATE	0x2c
#define	AC97_REG_PCM_SURR_DAC_RATE	0x2e
#define	AC97_REG_PCM_LFE_DAC_RATE	0x30
#define	AC97_REG_PCM_LR_ADC_RATE	0x32
#define	AC97_REG_PCM_MIC_ADC_RATE	0x34
#define	AC97_REG_CENTER_LFE_MASTER	0x36	/* center + LFE master volume */
#define	AC97_REG_SURR_MASTER		0x38	/* surround (rear) master vol */
				/*	0x3a	reserved	*/

/* Modem -- 0x3c-0x58 */

/* Vendor specific -- 0x5a-0x7b */

#define	AC97_REG_VENDOR_ID1           0x7c
#define	AC97_REG_VENDOR_ID2           0x7e

#define	AC97_CODEC_ID(a0, a1, a2, x)					\
	(((a0) << 24) | ((a1) << 16) | ((a2) << 8) | (x))

#define	AC97_GET_CODEC_ID(id, cp)					\
do {									\
	(cp)[0] = ((id) >> 24) & 0xff;					\
	(cp)[1] = ((id) >> 16) & 0xff;					\
	(cp)[2] = ((id) >> 8)  & 0xff;					\
	(cp)[3] = (id) & 0xff;						\
} while (0)
