/*	$NetBSD: uda1341.h,v 1.2 2001/07/15 20:19:32 ichiro Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.  All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ichiro FUKUHARA (ichiro@ichiro.org).
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
 * Philips UDA1341 L3 type
 */

/*
 * Microcontroller L3-interface timing (MIN)
 * expressed in micro-second
 */
#define L3_DATA_SETUP	1	/* 190 nsec */
#define L3_DATA_HOLD	1	/*  30 nsec */
#define L3_MODE_SETUP	1	/* 190 nsec */
#define L3_MODE_HOLD	1	/* 190 nsec */
#define L3_CLK_HIGH	1	/* 250 nsec */
#define L3_CLK_LOW	1	/* 250 nsec */
#define L3_HALT		1	/* 190 nsec */

/*
 * Philips UDA1341 L3 address and command types
 */
#define L3_ADDRESS_COM		5
#define L3_ADDRESS_DATA0	0
#define L3_ADDRESS_DATA1	1
#define L3_ADDRESS_STATUS	2

/*
 * Philips UDA1341 Status control
 */
#define STATUS0_RST		(1<<0)	/* UDA1341 Reset */
#define STATUS0_SC_512		(0<<4)	/* System clock freq.
					   512fs */
#define STATUS0_SC_384		(1<<4)	/* 384fs */
#define STATUS0_SC_256		(2<<4)	/* 256fs */
#define STATUS0_IF_I2S		(0<<1)	/* Data Input format
					   I2C */
#define STATUS0_IF_LSB16	(1<<1)	/* LSB 16 bits */
#define STATUS0_IF_LSB18	(2<<1)	/* LSB 18 bits */
#define STATUS0_IF_LSB20	(3<<1)	/* LSB 20 bits */
#define STATUS0_IF_MSB		(4<<1)	/* MSB */
#define STATUS0_IF_MSB16	(5<<1)	/* LSB 16 bits and MSB-output */
#define STATUS0_IF_MSB18	(6<<1)	/* LSB 18 bits and MSB-output */
#define STATUS0_IF_MSB20	(7<<1)	/* LSB 20 bits and MSB-output */
#define STATUS0_DC		(1<<0)	/* UDA1341 DC-filter ON */

#define STATUS1_OGS		(1<<6)	/* UDA1341 DAC Gain switch */
#define STATUS1_IGS		(1<<5)	/* UDA1341 ADC Gain switch */
#define STATUS1_PAD		(1<<4)	/* Polarity of ADC is inverting */
#define STATUS1_PDA		(1<<3)	/* Polarity of DAC is inverting */
#define STATUS1_DS		(1<<2)	/* double speed playback */
#define STATUS1_PC_OFF		(0<<0)	/* ADC:off DAC:off */
#define STATUS1_PC_DAC		(1<<0)	/* ADC:off DAC:on */
#define STATUS1_PC_ADC		(2<<0)	/* ADC:on  DAC:off */
#define STATUS1_PC_ON		(3<<0)	/* ADC:on  DAC:on */

/*
 * Philips UDA1341 DATA0 control
 */
/* Data0 direct programming registers (8 bits) */
#define DATA0_VC(val)		(63 - (((val)+1) * 63) / 100)
					/* Volume control val=(0<->100) */
#define DATA0_COMMON		(0<<6)	/* DATA0_0 common bits(6-7) */

#define DATA1_BB(val)		(((((val)+1) * 15) / 100) << 3)
					/* Bass Boost control val=(0<->100) */
#define DATA1_TR(val)		((((val)+1) * 3) / 100)
					/* Treble control val=(0<->100) */
#define DATA1_COMMON		(1<<6)	/* DATA0_1 common bits(6-7) */

#define DATA2_MODE_FLAT		(0<<0)	/* Mode filter is flat */
#define DATA2_MODE_MIN		(2<<0)	/* Mode filter is minimum */
#define DATA2_MODE_MAX		(3<<0)	/* Mode filter is maximum */
#define DATA2_MUTE		(1<<2)	/* Mute on */
#define DATA2_PP		(1<<5)	/* Peak Detection */
#define DATA2_COMMON		(2<<6)	/* DATA0_2 common bits(6-7) */

/* Data0 extended programming registers (16 bits) */
#define EXT_ADDR_COMMON		(3<<6)	/* Extended Address Common bits */
# define EXT_ADDR_E0		0	/* Extended Address of E0 */
# define EXT_ADDR_E1		1	/* Extended Address of E1 */
# define EXT_ADDR_E2		2	/* Extended Address of E2 */
# define EXT_ADDR_E3		4	/* Extended Address of E3 */
# define EXT_ADDR_E4		5	/* Extended Address of E4 */
# define EXT_ADDR_E5		6	/* Extended Address of E5 */

#define EXT_DATA_COMMN		(7<<5) /* Extended Data Common bits */
#define DATA_E0_MA(val)		((((val) + 1) * 31) / 100)
					/* mixer gain control val=(0<->100) */
#define DATA_E1_MB(val)		((((val) + 1) * 31) / 100)
					/* mixer gain control val=(0<->100) */
#define DATA_E2_MS(val)		(((((val) + 1) * 6) / 100) << 3)
					/* MIC sensitivity control val=(0<->100) */
#define DATA_E2_MM0		0	/* Double differential mode */
#define DATA_E2_MM1		1	/* input channel 1 select */
#define DATA_E2_MM2		2	/* input channel 2 select */
#define DATA_E2_MM3		3	/* digital mixer mode */

#define DATA_E3_AG		(1<<4)	/* AGC control ON */
#define DATA_E3_IG_L(val)	(((val * 127) / 100) & 3)
					/* Input AMP-Gain control (low 2 bits) */
#define DATA_E4_IG_H(val)	(((val * 127) / 100) >> 2)
					/* Input AMP-Gain control (high 5 bits) */
#define DATA_E5_AL(val)		(((val + 1) * 3) / 100)
					/* AGC output level val=(0<->100) */
/* end of uda1341.h */
