/*	$NetBSD: vrpmureg.h,v 1.2 1999/12/08 01:51:56 sato Exp $	*/

/*-
 * Copyright (c) 1999 SATO Kazumi. All rights reserved.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the PocketBSD project
 *	and its contributors.
 * 4. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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
 *	PMU (Power Management Unit) Registers definitions.
 *		start 0xB0000A0
 */

#define	PMUINT_REG_W	0x000	/* PMU interrupt/Status Register */

#define		PMUINT_GPIO3		(1<<15)		/* GPIO3 */
#define		PMUINT_GPIO2		(1<<14)		/* GPIO2 */
#define		PMUINT_GPIO1		(1<<13)		/* GPIO1 */
#define		PMUINT_GPIO0		(1<<12)		/* GPIO0 */
#define		PMUINT_DCDST		(1<<10)		/* DCD#  */
#define		PMUINT_RTC		(1<<9)		/* RTC Alarm */
#define		PMUINT_BATT		(1<<8)		/* BATTERY LOW  */
#define		PMUINT_TIMOUTRST	(1<<5)		/* HAL Timer Reset */
#define		PMUINT_RTCRST		(1<<4)		/* RTC Reset */
#define		PMUINT_RSTSWRST		(1<<3)		/* Reset SW */
#define		PMUINT_DMSWRST		(1<<2)		/* Deadman's SW */
#define		PMUINT_BATTINTR		(1<<1)		/* Low batt during normal operation */
#define		PMUINT_POWERSW		(1)		/* Power Switch */

#define		PMUINT_ALL		(PMUINT_GPIO3|PMUINT_GPIO2|\
					 PMUINT_GPIO1|PMUINT_GPIO0|\
					 PMUINT_DCDST|PMUINT_RTC|\
					 PMUINT_BATT|PMUINT_TIMOUTRST|\
					 PMUINT_RTCRST|PMUINT_RSTSWRST|\
					 PMUINT_DMSWRST|PMUINT_BATTINTR|\
					 PMUINT_POWERSW)

#define	PMUCNT_REG_W	0x002	/* PMU Control Register */

#define		PMUCNT_GPIO3MASK	(1<<15)		/* GPIO3 MASK */
#define		PMUCNT_GPIO3EN		(1<<15)		/* GPIO3 Enable */
#define		PMUCNT_GPIO3DS		(0<<15)		/* GPIO3 Disable */

#define		PMUCNT_GPIO2MASK	(1<<14)		/* GPIO2 MASK */
#define		PMUCNT_GPIO2EN		(1<<14)		/* GPIO2 Enable */
#define		PMUCNT_GPIO2DS		(0<<14)		/* GPIO2 Disable */

#define		PMUCNT_GPIO1MASK	(1<<13)		/* GPIO1 MASK */
#define		PMUCNT_GPIO1EN		(1<<13)		/* GPIO1 Enable */
#define		PMUCNT_GPIO1DS		(0<<13)		/* GPIO1 Disable */

#define		PMUCNT_GPIO0MASK	(1<<12)		/* GPIO0 MASK */
#define		PMUCNT_GPIO0EN		(1<<12)		/* GPIO0 Enable */
#define		PMUCNT_GPIO0DS		(0<<12)		/* GPIO0 Disable */

#define		PMUCNT_GPIO3TRIG	(1<<11)		/* GPIO3 TRIG */
#define		PMUCNT_GPIO3D		(1<<11)		/* GPIO3 Fall */
#define		PMUCNT_GPIO3U		(0<<11)		/* GPIO3 Raise */

#define		PMUCNT_GPIO2TRIG	(1<<10)		/* GPIO2 TRIG */
#define		PMUCNT_GPIO2D		(1<<10)		/* GPIO2 Fall */
#define		PMUCNT_GPIO2U		(0<<10)		/* GPIO2 Raise */

#define		PMUCNT_GPIO1TRIG	(1<<9)		/* GPIO1 TRIG */
#define		PMUCNT_GPIO1D		(1<<9)		/* GPIO1 Fall */
#define		PMUCNT_GPIO1U		(0<<9)		/* GPIO1 Raise */

#define		PMUCNT_GPIO0TRIG	(1<<8)		/* GPIO0 TRIG */
#define		PMUCNT_GPIO0D		(1<<8)		/* GPIO0 Fall */
#define		PMUCNT_GPIO0U		(0<<8)		/* GPIO0 Raise */

#define		PMUCNT_HALTIMERRST	(1<<2)		/* HAL Timer Reset */ 
#define		PMUCNT_ONE		(1<<1)		/* ALWAYS write 1 */


#define	PMUINT2_REG_W	0x004	/* PMU interrupt/Status Register 2 */

#define		PMUINT_GPIO12		(1<<15)		/* GPIO12 */
#define		PMUINT_GPIO11		(1<<14)		/* GPIO11 */
#define		PMUINT_GPIO10		(1<<13)		/* GPIO10 */
#define		PMUINT_GPIO9		(1<<12)		/* GPIO9 */

#define		PMUINT2_ALL		(PMUINT_GPIO12|PMUINT_GPIO11|\
					 PMUINT_GPIO10|PMUINT_GPIO9)

#define	PMUCNT2_REG_W	0x006	/* PMU Control Register 2 */
#define		PMUCNT_GPIO12MASK	(1<<15)		/* GPIO12 MASK */
#define		PMUCNT_GPIO12EN		(1<<15)		/* GPIO12 Enable */
#define		PMUCNT_GPIO12DS		(0<<15)		/* GPIO12 Disable */

#define		PMUCNT_GPIO11MASK	(1<<14)		/* GPIO11 MASK */
#define		PMUCNT_GPIO11EN		(1<<14)		/* GPIO11 Enable */
#define		PMUCNT_GPIO11DS		(0<<14)		/* GPIO11 Disable */

#define		PMUCNT_GPIO10MASK	(1<<13)		/* GPIO10 MASK */
#define		PMUCNT_GPIO10EN		(1<<13)		/* GPIO10 Enable */
#define		PMUCNT_GPIO10DS		(0<<13)		/* GPIO10 Disable */

#define		PMUCNT_GPIO9MASK	(1<<12)		/* GPIO9 MASK */
#define		PMUCNT_GPIO9EN		(1<<12)		/* GPIO9 Enable */
#define		PMUCNT_GPIO9DS		(0<<12)		/* GPIO9 Disable */

#define		PMUCNT_GPIO12TRIG	(1<<11)		/* GPIO12 TRIG */
#define		PMUCNT_GPIO12D		(1<<11)		/* GPIO12 Fail */
#define		PMUCNT_GPIO12U		(0<<11)		/* GPIO12 Raise */

#define		PMUCNT_GPIO11TRIG	(1<<10)		/* GPIO11 TRIG */
#define		PMUCNT_GPIO11D		(1<<10)		/* GPIO11 Fail */
#define		PMUCNT_GPIO11U		(0<<10)		/* GPIO11 Raise */

#define		PMUCNT_GPIO10TRIG	(1<<9)		/* GPIO10 TRIG */
#define		PMUCNT_GPIO10D		(1<<9)		/* GPIO10 Fail */
#define		PMUCNT_GPIO10U		(0<<9)		/* GPIO10 Raise */

#define		PMUCNT_GPIO9TRIG	(1<<8)		/* GPIO9 TRIG */
#define		PMUCNT_GPIO9D		(1<<8)		/* GPIO9 Fail */
#define		PMUCNT_GPIO9U		(0<<8)		/* GPIO9 Raise */


#define	PMUWAIT_REG_W	0x008	/* PMU Wait Control Register */
#define		PMUWAIT_DEFAULT		0x2c00		/* 343.75ms */

#define	PMUDIV_REG_W	0x00C	/* PMU Div Mode Register (vr4121) */ 

/* END vrpmureg.h */
