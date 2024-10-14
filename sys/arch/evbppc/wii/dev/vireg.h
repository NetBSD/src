/* $NetBSD: vireg.h,v 1.2.2.3 2024/10/14 16:44:42 martin Exp $ */

/*-
 * Copyright (c) 2024 Jared McNeill <jmcneill@invisible.ca>
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

#ifndef _WII_DEV_VIREG_H
#define _WII_DEV_VIREG_H

/*
 * Nintendo Wii Video Interface (VI) registers, from
 * https://www.gc-forever.com/yagcd/
 */

/* [2B] VTR - Vertical Timing Register */
#define VI_VTR		0x00
#define	 VI_VTR_ACV	__BITS(13,4)
#define	 VI_VTR_EQU	__BITS(3,0)

/* [2B] DCR - Display Configuration Register */
#define VI_DCR		0x02
#define	 VI_DCR_FMT	__BITS(9,8)
#define   VI_DCR_FMT_NTSC	0
#define   VI_DCR_FMT_PAL	1
#define   VI_DCR_FMT_MPAL	2
#define   VI_DCR_FMT_DEBUG	3
#define  VI_DCR_LE1	__BITS(7,6)
#define  VI_DCR_LE0	__BITS(5,4)
#define	 VI_DCR_DLR	__BIT(3)
#define  VI_DCR_NIN	__BIT(2)
#define  VI_DCR_RST	__BIT(1)
#define  VI_DCR_ENB	__BIT(0)

/* [4B] HTR0 - Horizontal Timing 0 */
#define VI_HTR0		0x04
#define	 VI_HTR0_HCS	__BITS(30,24)
#define	 VI_HTR0_HCE	__BITS(22,16)
#define	 VI_HTR0_HLW	__BITS(8,0)

/* [4B] HTR1 - Horizontal Timing 1 */
#define VI_HTR1		0x08
#define	 VI_HTR1_HBS	__BITS(26,17)
#define  VI_HTR1_HBE	__BITS(16,7)
#define	 VI_HTR1_HSY	__BITS(6,0)

/* [4B] VTO - Odd Field Vertical Timing Register */
#define VI_VTO		0x0c
#define  VI_VTO_PSB	__BITS(25,16)
#define  VI_VTO_PRB	__BITS(9,0)

/* [4B] VTE - Even Field Vertical Timing Register */
#define VI_VTE		0x10
#define  VI_VTE_PSB	__BITS(25,16)
#define  VI_VTE_PRB	__BITS(9,0)

/* [4B] BBOI - Odd Field Burst Blanking Interval Register */
#define VI_BBOI		0x14
#define  VI_BBOI_BE3	__BITS(31,21)
#define	 VI_BBOI_BS3	__BITS(20,16)
#define  VI_BBOI_BE1	__BITS(15,5)
#define  VI_BBOI_BS1	__BITS(4,0)

/* [4B] BBEI - Even Field Burst Blanking Interval Register */
#define VI_BBEI		0x18
#define  VI_BBEI_BE4	__BITS(31,21)
#define	 VI_BBEI_BS4	__BITS(20,16)
#define  VI_BBEI_BE2	__BITS(15,5)
#define  VI_BBEI_BS2	__BITS(4,0)

/* [4B] TFBL - Top Field Base Register (L) */
#define VI_TFBL		0x1c
#define  VI_TFBL_PGOFF	__BIT(28)
#define  VI_TFBL_XOF	__BITS(27,24)
#define  VI_TFBL_FBB	__BITS(23,0)

/* [4B] TFBR - Top Field Base Register (R) */
#define VI_TFBR		0x20
#define	 VI_TFBR_FBB	__BITS(23,0)

/* [4B] BFBL - Bottom Field Base Register (L) */
#define VI_BFBL		0x24
#define	 VI_BFBL_PGOFF	__BIT(28)
#define  VI_BFBL_XOF	__BITS(27,24)
#define	 VI_BFBL_FBB	__BITS(23,0)

/* [4B] BFBR - Bottom Field Base Register (R) */
#define VI_BFBR		0x28
#define	 VI_BFBR_FBB	__BITS(23,0)

/* [2B] DPV - Current Vertical Position */
#define VI_DPV		0x2c
#define  VI_DPV_VCT	__BITS(10,0)

/* [2B] DPH - Current Horizontal Position */
#define VI_DPH		0x2e
#define	 VI_DPH_HCT	__BITS(10,0)

/* [4B] DI[0-3] - Display Interrupt 0-3 */
#define VI_DI0		0x30
#define VI_DI1		0x34
#define	VI_DI2		0x38
#define	VI_DI3		0x3c
#define  VI_DI_INT	__BIT(31)
#define  VI_DI_ENB	__BIT(28)
#define	 VI_DI_VCT	__BITS(25,16)
#define	 VI_DI_HCT	__BITS(9,0)

/* [4B] DL[0-1] - Display Latch Register 0-1 */
#define VI_DL0		0x40
#define	VI_DL1		0x44
#define	 VI_DL_TRG	__BIT(31)
#define	 VI_DL_VCT	__BITS(26,16)
#define  VI_DL_HCT	__BITS(10,0)

/* [2B] PICCONF - Picture Configuration Register */
#define VI_PICCONF	0x48
#define  VI_PICCONF_READS	__BITS(15,8)
#define	 VI_PICCONF_STRIDES	__BITS(7,0)

/* [2B] HSR - Horizontal Scaling Register */
#define VI_HSR		0x4a
#define	 VI_HSR_HS_EN	__BIT(12)
#define	 VI_HSR_STP	__BITS(8,0)

/* [4B] FCT[0-6] - Filter Coefficient Table 0-6 */
#define VI_FCT0		0x4c
#define VI_FCT1		0x50
#define VI_FCT2		0x54
#define VI_FCT3		0x58
#define VI_FCT4		0x5c
#define VI_FCT5		0x60
#define VI_FCT6		0x64

/* [4B] ??? */
#define VI_UNKNOWN_68H	0x68

/* [2B] VICLK - VI Clock Select Register */
#define	VI_VICLK	0x6c
#define	 VI_VICLK_SEL	__BIT(0)
#define	  VI_VICLK_SEL_27MHZ	0
#define	  VI_VICLK_SEL_54MHZ	1

/* [2B] VISEL - VI DTV Status Register */
#define VI_VISEL	0x6e
#define	 VI_VISEL_SEL			__BIT(2)
#define	 VI_VISEL_COMPONENT_CABLE	__BIT(0)

/* [2B] VI_HSCALINGW - Horizontal Scaling Width */
#define VI_HSCALINGW	0x70
#define  VI_HSCALINGW_WIDTH	__BITS(9,0)

/* [2B] HBE - Border HBE */
#define VI_HBE		0x72
#define	 VI_HBE_BRDR_EN	__BIT(15)
#define	 VI_HBE_HBE656	__BITS(9,0)

/* [2B] HBS - Border HBS */
#define VI_HBS		0x74
#define  VI_HBS_HBS656	__BITS(9,0)

/* [2B] ??? */
#define VI_UNKNOWN_76H	0x76

/* [4B] ??? */
#define VI_UNKNOWN_78H	0x78

/* [4B] ??? */
#define VI_UNKNOWN_7CH	0x7c

#endif /* !_WII_DEV_VIREG_H */
