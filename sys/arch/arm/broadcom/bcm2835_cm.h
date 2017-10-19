/*	$NetBSD: bcm2835_cm.h,v 1.2 2017/10/19 05:45:37 skrll Exp $	*/

/*-
 * Copyright (c) 2015 The NetBSD Foundation, Inc.
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef BCM2835_CMREG_H
#define BCM2835_CMREG_H

#define CM_GP0CTL		0x70
#define  CM_CTL_PASSWD		__BITS(24,31)
#define  CM_CTL_MASH		__BITS(9,10)
#define  CM_CTL_FLIP		__BIT(8)
#define  CM_CTL_BUSY		__BIT(7)
#define  CM_CTL_KILL		__BIT(5)
#define  CM_CTL_ENAB		__BIT(4)
#define  CM_CTL_SRC		__BIT(0,3)
#define CM_GP0DIV		0x74
#define  CM_DIV_PASSWD		__BITS(24,31)
#define  CM_DIV_DIVI		__BITS(12,23)
#define  CM_DIV_DIVF		__BITS(0,11)
#define CM_GP1CTL		0x78
#define CM_GP1DIV		0x7c
#define CM_GP2CTL		0x80
#define CM_GP2DIV		0x84

#define CM_PCMCTL		0x98	/* PCM / I2S */
#define CM_PCMDIV		0x9c
#define CM_PWMCTL		0xa0
#define CM_PWMDIV		0xa4

#define CM_PASSWD		0x5a

/* clock sources (frequencies for RPI) */
#define CM_CTL_SRC_GND		0
#define CM_CTL_SRC_OSCILLATOR	1	/* 19.2MHz */
#define CM_CTL_SRC_TESTDEBUG0	2
#define CM_CTL_SRC_TESTDEBUG1	3
#define CM_CTL_SRC_PLLA		4
#define CM_CTL_SRC_PLLC		5	/* 1000MHz (changes with overclock) */
#define CM_CTL_SRC_PLLD		6	/* 500MHz core clock */
#define CM_CTL_SRC_HDMIAUX	7	/* 216MHz HDMI auxiliary */

#if 0
#define CM_GNRICCTL		0x00
#define CM_GNRICDIV		0x04
#define CM_VPUCTL		0x08
#define CM_VPUDIV		0x0c
#define CM_SYSCTL		0x10
#define CM_SYSDIV		0x14
#define CM_PERIACTL		0x18
#define CM_PERIADIV		0x1c
#define CM_PERIICTL		0x20
#define CM_PERIIDIV		0x24
#define CM_H264CTL		0x28
#define CM_H264DIV		0x2c
#define CM_ISPCTL		0x30
#define CM_ISPDIV		0x34
#define CM_V3DCTL		0x38
#define CM_V3DDIV		0x3c
#define CM_CAM0CTL		0x40
#define CM_CAM0DIV		0x44
#define CM_CAM1CTL		0x48
#define CM_CAM1DIV		0x4c
#define CM_CCP2CTL		0x50
#define CM_CCP2DIV		0x54
#define CM_DSIOECTL		0x58
#define CM_DSIOEDIV		0x5c
#define CM_DSIOPCTL		0x60
#define CM_DSIOPDIV		0x64
#define CM_DPICTL		0x68
#define CM_DPIDIV		0x6c

#define CM_HSMCTL		0x88
#define CM_HSMDIV		0x8c
#define CM_HSMCTL		0x90
#define CM_HSMDIV		0x94

#define CM_SLIMCTL		0xa8
#define CM_SLIMDIV		0xac
#define CM_SMICTL		0xb0
#define CM_SMIDIV		0xb4

#define CM_TCNTCTL		0xc0
#define CM_TCNTDIV		0xc4
#define CM_TECCTL		0xc8
#define CM_TECDIV		0xcc
#define CM_TD0CTL		0xd0
#define CM_TD0DIV		0xd4
#define CM_TD1CTL		0xd8
#define CM_TD1DIV		0xdc
#define CM_TSENSCTL		0xe0
#define CM_TSENSDIV		0xe4
#define CM_TIMERCTL		0xe8
#define CM_TIMERDIV		0xec
#define CM_UARTCTL		0xf0
#define CM_UARTDIV		0xf4
#define CM_VECCTL		0xf8
#define CM_VECDIV		0xfc

#define CM_DSI1ECTL		0x158
#define CM_DSI1EDIV		0x15c
#define CM_DSI1PCTL		0x160
#define CM_DSI1PDIV		0x164
#define CM_DFTCTL		0x168
#define CM_DFTDIV		0x16c

#define CM_PULSECTL		0x190
#define CM_PULSEDIV		0x194

#define CM_SDCCTL		0x1ab
#define CM_SDCDIV		0x1ac
#define CM_ARMCTL		0x1b0
#define CM_ARMDIV		0x1b4
#define CM_AVE0CTL		0x1b8
#define CM_AVE0DIV		0x1bc
#define CM_EMMCCTL		0x1c0
#define CM_EMMCDIV		0x1c4

#define CM_OSCCOUNT		0x100
#define CM_PLLA			0x104
#define CM_PLLB			0x170
#define CM_PLLC			0x108
#define CM_PLLD			0x10c
#define CM_PLLH			0x110
#define CM_LOCK			0x114
#define CM_EVENT		0x118
#define CM_INTEN		CM_EVENT
#define CM_DSIOHSCK		0x120
#define CM_CKSM			0x124
#define CM_OSCFREQI		0x128
#define CM_OSCFREQF		0x12c
#define CM_PLLTCTL		0x130
#define CM_PLLTCNT0		0x134
#define CM_PLLTCNT1		0x138
#define CM_PLLTCNT2		0x13c
#define CM_PLLTCNT3		0x140
#define CM_TDCLKEN		0x144
#define CM_BURSTCTL		0x148
#define CM_BURSTCNT		0x14C

#endif

enum bcm_cm_clock {
	BCM_CM_GP0,
	BCM_CM_GP1,
	BCM_CM_GP2,
	BCM_CM_PCM,
	BCM_CM_PWM
};

int bcm_cm_set(enum bcm_cm_clock, uint32_t, uint32_t);
int bcm_cm_get(enum bcm_cm_clock, uint32_t *, uint32_t *);

#endif /* !BCM2835_CMREG_H */
