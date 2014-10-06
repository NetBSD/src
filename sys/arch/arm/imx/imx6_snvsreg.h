/*	$NetBSD: imx6_snvsreg.h,v 1.1 2014/10/06 10:15:40 ryo Exp $	*/

/*
 * Copyright (c) 2014 Ryo Shimizu <ryo@nerv.org>
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _IMX6_SNVSREG_H_
#define _IMX6_SNVSREG_H_

#define SVNS_COUNTER_HZ			(32 * 1024)	/* 32kHz */
#define SVNS_COUNTER_SHIFT		15

#define SNVS_HPLR			0x00000000
#define SNVS_HPCOMR			0x00000004
#define SNVS_HPCR			0x00000008
#define SNVS_HPSR			0x00000014
#define SNVS_HPRTCMR			0x00000024
#define SNVS_HPRTCLR			0x00000028
#define SNVS_HPTAMR			0x0000002c
#define SNVS_HPTALR			0x00000030
#define SNVS_LPLR			0x00000034
#define  SNVS_LPLR_GPR_HL		__BIT(5)
#define  SNVS_LPLR_MC_HL		__BIT(4)
#define  SNVS_LPLR_LPCALB_HL		__BIT(3)
#define  SNVS_LPLR_SRTC_HL		__BIT(2)
#define SNVS_LPCR			0x00000038
#define  SNVS_LPCR_PK_OVERRIDE		__BIT(23)
#define  SNVS_LPCR_PK_EN		__BIT(22)
#define  SNVS_LPCR_ON_TIME		__BITS(21, 20)
#define  SNVS_LPCR_DEBOUNCE		__BITS(19, 18)
#define  SNVS_LPCR_BTN_PRESS_TIME	__BITS(17, 16)
#define  SNVS_LPCR_LPCALB_VAL		__BITS(14, 10)
#define  SNVS_LPCR_LPCALB_EN		__BIT(8)
#define  SNVS_LPCR_PWR_GLITCH_EN	__BIT(7)
#define  SNVS_LPCR_TOP			__BIT(6)
#define  SNVS_LPCR_DP_EN		__BIT(5)
#define  SNVS_LPCR_SRTC_INV_EN		__BIT(4)
#define  SNVS_LPCR_LPWUI_EN		__BIT(3)
#define  SNVS_LPCR_MC_ENV		__BIT(2)
#define  SNVS_LPCR_LPTA_EN		__BIT(1)
#define  SNVS_LPCR_SRTC_ENV		__BIT(0)
#define SNVS_LPSR			0x0000004c
#define  SNVS_LPSR_SPO			__BIT(18)
#define  SNVS_LPSR_EO			__BIT(17)
#define  SNVS_LPSR_MCR			__BIT(2)
#define  SNVS_LPSR_SRTCR		__BIT(1)
#define  SNVS_LPSR_LPTA			__BIT(0)
#define SNVS_LPSRTCMR			0x00000050
#define  SNVS_LPSRTCMR_SRTC		__BITS(14, 0)
#define SNVS_LPSRTCLR			0x00000054
#define SNVS_LPTAR			0x00000058
#define SNVS_LPSMCMR			0x0000005c
#define  SNVS_LPSMCMR_MC_ERA_BIT	__BITS(31, 16)
#define  SNVS_LPSMCMR_MON_COUNTER	__BITS(15, 0)
#define SNVS_LPSMCLR			0x00000060
#define SNVS_LPGPR			0x00000068
#define SNVS_HPVIDR1			0x00000bf8
#define  SNVS_HPVIDR1_IP_ID		__BITS(31, 16)
#define  SNVS_HPVIDR1_MAJOR_REV		__BITS(15, 8)
#define  SNVS_HPVIDR1_MINOR_REV		__BITS(7, 0)
#define SNVS_HPVIDR2			0x00000bfc
#define  SNVS_HPVIDR2_IP_ERA		__BITS(31, 24)
#define  SNVS_HPVIDR2_INTG_OPT		__BITS(23, 16)
#define  SNVS_HPVIDR2_ECO_REV		__BITS(15, 8)
#define  SNVS_HPVIDR2_CONFIG_OPT	__BITS(7, 0)

#endif /* _IMX6_SNVSREG_H_ */
