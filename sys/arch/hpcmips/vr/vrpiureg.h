/*	$NetBSD: vrpiureg.h,v 1.2.6.1 2002/03/16 15:58:03 jdolecek Exp $	*/

/*
 * Copyright (c) 1999 Shin Takemura All rights reserved.
 * Copyright (c) 1999 PocketBSD Project. All rights reserved.
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
 * PIU (Touch panel interface unit) register definitions
 */

#define	PIUCNT_REG_W	0x002	/* PIU Control register			*/
#define		PIUCNT_PENSTC		(1<<13)
#define		PIUCNT_PADSTATE_MASK	(0x7<<10)
#define		PIUCNT_PADSTATE_SHIFT	10
#define		PIUCNT_PADSTATE_CmdScan			(0x7<<10)
#define		PIUCNT_PADSTATE_IntervalNextScan	(0x6<<10)
#define		PIUCNT_PADSTATE_PenDataScan		(0x5<<10)
#define		PIUCNT_PADSTATE_WaitPenTouch		(0x4<<10)
#define		PIUCNT_PADSTATE_RFU			(0x3<<10)
#define		PIUCNT_PADSTATE_ADPortScan		(0x2<<10)
#define		PIUCNT_PADSTATE_Standby			(0x1<<10)
#define		PIUCNT_PADSTATE_Disable			(0x0<<10)
#define		PIUCNT_PADATSTOP	(1<<9)
#define		PIUCNT_PADATSTART	(1<<8)
#define		PIUCNT_PADSCANSTOP	(1<<7)
#define		PIUCNT_PADSCANSTART	(1<<6)
#define		PIUCNT_PADSCANTYPE	(1<<5)
#define		PIUCNT_PIUMODE_MASK	(0x3<<3)
#define		PIUCNT_PIUMODE_ADCONVERTER	(0x1<<3)
#define		PIUCNT_PIUMODE_COORDINATE	(0x0<<3)
#define		PIUCNT_PIUSEQEN		(1<<2)
#define		PIUCNT_PIUPWR		(1<<1)
#define		PIUCNT_PADRST		(1<<0)

#define	PIUINT_REG_W	0x004	/* PIU Interruptcause register		*/
#define		PIUINT_OVP		(1<<15)
#define		PIUINT_PADCMDINTR	(1<<6)
#define		PIUINT_PADADPINTR	(1<<5)
#define		PIUINT_PADPAGE1INTR	(1<<4)
#define		PIUINT_PADPAGE0INTR	(1<<3)
#define		PIUINT_PADDLOSTINTR	(1<<2)
#define		PIUINT_PENCHGINTR	(1<<0)
#define		PIUINT_ALLINTR	(PIUINT_PADCMDINTR | \
				 PIUINT_PADADPINTR | \
				 PIUINT_PADPAGE1INTR | \
				 PIUINT_PADPAGE0INTR | \
				 PIUINT_PADDLOSTINTR | \
				 PIUINT_PENCHGINTR)

#define	PIUSIVL_REG_W	0x006	/* PIU Data sampling interval register	*/
#define		PIUSIVL_SCANINTVAL_MASK	0x7FF
#define		PIUSIVL_SCANINTVAL_UNIT	30	/* 30 us */

#define	PIUSTBL_REG_W	0x008	/* PIU A/D converter start delay register*/
#define		PIUSTBL_STABLE_MASK	0x1F
#define		PIUSTBL_STABLE_UNIT	30	/* 30 us */

#define	PIUCMD_REG_W	0x00A	/* PIU A/D command register		*/
#define		PIUCMD_STABLEON		(1<<12)
#define		PIUCMD_TPYEN_MASK	(3<<10)
#define		PIUCMD_TPY1_INPUT	(0<<11)
#define		PIUCMD_TPY1_OUTPUT	(1<<11)
#define		PIUCMD_TPY0_INPUT	(0<<10)
#define		PIUCMD_TPY0_OUTPUT	(1<<10)
#define		PIUCMD_TPXEN_MASK	(3<<8)
#define		PIUCMD_TPX1_INPUT	(0<<9)
#define		PIUCMD_TPX1_OUTPUT	(1<<9)
#define		PIUCMD_TPX0_INPUT	(0<<8)
#define		PIUCMD_TPX0_OUTPUT	(1<<8)
#define		PIUCMD_TPYD_MASK	(3<<6)
#define		PIUCMD_TPY1_LOW		(0<<7)
#define		PIUCMD_TPY1_HIGH	(1<<7)
#define		PIUCMD_TPY0_LOW		(0<<6)
#define		PIUCMD_TPY0_HIGH	(1<<6)
#define		PIUCMD_TPXD_MASK	(3<<4)
#define		PIUCMD_TPX1_LOW		(0<<5)
#define		PIUCMD_TPX1_HIGH	(1<<5)
#define		PIUCMD_TPX0_LOW		(0<<4)
#define		PIUCMD_TPX0_HIGH	(1<<4)
#define		PIUCMD_ADCMD_MASK	0xF
#define		PIUCMD_STANBYREQ	0xF
#define		PIUCMD_AUDIOIN		0x7
#define		PIUCMD_ADIN2		0x6
#define		PIUCMD_ADIN1		0x5
#define		PIUCMD_ADIN0		0x4
#define		PIUCMD_TPY1		0x3
#define		PIUCMD_TPY0		0x2
#define		PIUCMD_TPX1		0x1
#define		PIUCMD_TPX0		0x0

#define	PIUASCN_REG_W	0x010	/* PIU A/D port scan  register		*/
#define		PIUACN_TPPSCAN		(1<<1)
#define		PIUACN_ADPSSTART	(1<<0)

#define	PIUAMSK_REG_W	0x012	/* PIU A/D scan mask register		*/
#define		PIUAMSK_ADINM3		(1<<7)
#define		PIUAMSK_AUDIOM		PIUAMSK_ADINM3
#define		PIUAMSK_ADINM2		(1<<6)
#define		PIUAMSK_ADINM1		(1<<5)
#define		PIUAMSK_ADINM0		(1<<4)
#define		PIUAMSK_ADINMALL	0x70
#define		PIUAMSK_TPYM1		(1<<3)
#define		PIUAMSK_TPYM0		(1<<2)
#define		PIUAMSK_TPXM1		(1<<1)
#define		PIUAMSK_TPXM0		(1<<0)
#define		PIUAMSK_TPMALL		0xF0

#define	PIUCIVL_REG_W	0x01E	/* PIU Check interval register		*/
#define		PIUCIVL_CHKINTVAL_MASK	0x7FF

#ifndef PIUB_REG_OFFSSET
#define	PIUB_REG_OFFSSET	0x180
#endif
#define	PIUPB00_REG_W	(PIUB_REG_OFFSSET+0x00)	/* PIU Page 0 Buffer 0 reg */
#define	PIUPB01_REG_W	(PIUB_REG_OFFSSET+0x02)	/* PIU Page 0 Buffer 1 reg */
#define	PIUPB02_REG_W	(PIUB_REG_OFFSSET+0x04)	/* PIU Page 0 Buffer 2 reg */
#define	PIUPB03_REG_W	(PIUB_REG_OFFSSET+0x06)	/* PIU Page 0 Buffer 3 reg */
#define	PIUPB04_REG_W	(PIUB_REG_OFFSSET+0x1C)	/* PIU Page 0 Buffer 4 reg */
#define	PIUPB10_REG_W	(PIUB_REG_OFFSSET+0x08)	/* PIU Page 1 Buffer 0 reg */
#define	PIUPB11_REG_W	(PIUB_REG_OFFSSET+0x0A)	/* PIU Page 1 Buffer 1 reg */
#define	PIUPB12_REG_W	(PIUB_REG_OFFSSET+0x0C)	/* PIU Page 1 Buffer 2 reg */
#define	PIUPB13_REG_W	(PIUB_REG_OFFSSET+0x0E)	/* PIU Page 1 Buffer 3 reg */
#define	PIUPB14_REG_W	(PIUB_REG_OFFSSET+0x1E)	/* PIU Page 1 Buffer 4 reg */
#define PIUPB(page, n)	(((n)<4) ? \
			 (PIUPB00_REG_W + (page) * 8 + (n) * 2) : \
			 (PIUPB04_REG_W + (page) * 2))
#define PIUPB_VALID		(1<<15)
#define PIUPB_PADDATA_MASK	0x3FF
#define PIUPB_PADDATA_MAX	0x3FF

#define	PIUAB0_REG_W	(PIUB_REG_OFFSSET+0x10)	/* PIU A/D scan Buffer 0 reg */
#define	PIUAB1_REG_W	(PIUB_REG_OFFSSET+0x12)	/* PIU A/D scan Buffer 1 reg */
#define	PIUAB2_REG_W	(PIUB_REG_OFFSSET+0x14)	/* PIU A/D scan Buffer 2 reg */
#define	PIUAB3_REG_W	(PIUB_REG_OFFSSET+0x16)	/* PIU A/D scan Buffer 3 reg */
#define PIUAB(n)	(PIUAB0_REG_W+(n)*2)
#define PIUAB_VALID		(1<<15)
#define PIUAB_PADDATA_MASK	0x3FF
