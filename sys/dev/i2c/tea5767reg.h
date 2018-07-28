/*	$NetBSD: tea5767reg.h,v 1.1.2.2 2018/07/28 04:37:44 pgoyette Exp $	*/
/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Karuna Grewal.
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

#ifndef __TEA5767REG_H__
#define __TEA5767REG_H__

/* TEA5767 ADDR */
#define TEA5767_ADDR                0x60

/* 1st byte */
#define TEA5767_MUTE                0x80    /* Set Mute */
#define TEA5767_SEARCH              0x40    /* Activate search Mode */

/* 3rd byte */
#define TEA5767_SUD                 0x80    /* Search Up */
#define TEA5767_SSL_1               0x60    /* ADC o/p = 10 */
#define TEA5767_SSL_2               0x40    /* ADC o/p = 7 */
#define TEA5767_SSL_3               0x20    /* ADC o/p = 5 */
#define TEA5767_MONO                0x08    /* Force Mono */
#define TEA5767_MUTE_R              0x04    /* Mute Right */
#define TEA5767_MUTE_L              0x02    /* Mute Left */

/* 4th byte */
#define TEA5767_STANDBY             0x40
#define TEA5767_FM_BAND             0x20    /* Set Japanese FM Band */
#define TEA5767_XTAL                0x10
#define TEA5767_SMUTE               0x08
#define TEA5767_SNC                 0x02    /* Stereo Noise Cancelling */
#define TEA5767_SEARCH_IND          0x01

/* 5th byte */
#define TEA5767_PLLREF              0x80    /* If enabled TEA5767_CLK_FREQ : 6.5MHZ*/

/* Read Mode MASKS*/

/* 1st byte */
#define TEA5767_READY_FLAG          0x80
#define TEA5767_BAND_LIMIT          0X40

/* 3rd byte */
#define TEA5767_STEREO              0x80
#define TEA5767_IF_COUNTER          0x7f

/* 4th byte */
#define TEA5767_ADC_LEVEL           0xf0

/* Kernel Config Maps */
#define TEA5767_JAPAN_FM_FLAG       0x01
#define TEA5767_PLL_FLAG            0x02
#define TEA5767_XTAL_FLAG           0x04
#define TEA5767_FORCE_SRCH_FLAG     0x08
#endif
