/*	$NetBSD: ixp12x0_clkreg.h,v 1.2 2003/02/17 20:51:52 ichiro Exp $ */

/*
 * Copyright (c) 2002
 *	Ichiro FUKUHARA <ichiro@ichiro.org>.  All rights reserved.
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
 *	This product includes software developed by Ichiro FUKUHARA.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ICHIRO FUKUHARA AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL ICHIRO FUKUHARA OR THE VOICES IN HIS 
 * HEAD BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * IXP12X0 TIMER registers
 *  TIMER_1 v0xf0010300 p0x42000300
 *  TIMER_2 v0xf0010320 p0x42000320
 *  TIMER_3 v0xf0010340 p0x42000340
 *  TIMER_4 v0xf0010360 p0x42000360
 */

#ifndef _IXP12X0_TIMERREG_H_
#define _IXP12X0_TIMERREG_H_

#include <arm/ixp12x0/ixp12x0reg.h>

#define IXPCLK_PLL_CFG_OFFSET	(0x90000c00U - 0x42000300U)
#define IXPCLK_PLL_CFG_SIZE	0x04

/* timer load register */
#define IXPCLK_LOAD		0x00000000
#define IXPCL_ITV		0x00ffffff

/* timer value register */
#define IXPCLK_VALUE		0x00000004
#define IXPCL_CTV		0x00ffffff

/* timer control register */
#define IXPCLK_CONTROL		0x00000008
#define IXPCL_STP		0x0c
#define  IXPCL_STP_CORE		0x00
#define  IXPCL_STP_DIV16	0x04
#define  IXPCL_STP_DIV256	0x08
#define IXPCL_MODE		0x40
#define  IXPCL_FREERUN		0x00
#define  IXPCL_PERIODIC		0x40
#define IXPCL_EN		0x80
#define  IXPCL_DISABLE		0x00
#define  IXPCL_ENABLE		0x80

/* timer clear register */
#define IXPCLK_CLEAR		0x0000000c
#define IXPT_CLEAR		0

/*
 * IXP12X0 real time clock registers
 * RTC_DIV	0x90002000
 * RTC_TINT	0x90002400
 * RTC_TVAL	0x90002800
 * RTC_CNTR	0x90002c00
 * RTC_ALM	0x90003000
 */

/* RTC_DIV register */
#define RTC_DIV		0x90002000
#define RTC_RDIV	0x0000ffff
#define RTC_WEN		0x00010000
#define  RTC_WDIVISER	0x00000000
#define  RTC_WINTONLY	0x00010000
#define RTC_IEN		0x00020000
#define  RTC_IEN_E	0x00020000
#define  RTC_IEN_D	0x00000000
#define RTC_IRST	0x00040000
#define  RTC_IRST_NOCLR	0x00040000
#define  RTC_IRST_CLR	0x00000000
#define RTC_IRQS	0x00080000
#define  RTC_IRQS_IRQ	0x00080000
#define  RTC_IRQS_FIQ	0x00000000


/* RTC_TINT register */
#define RTC_TINT	0x90002400
#define RTC_RTINT	0x0000ffff

/* RTC_TVAL register */
#define RTC_TVAL	0x90002800
#define RTC_TVAL_TVAL	0x0000ffff
#define RTC_LD		0x00010000
#define  RTC_LD_LOAD	0x00010000
#define  RTC_LD_NOLOAD	0x00000000
#define RTC_PRE		0x00020000
#define  RTC_PRE_SYSCLK	0x00020000
#define  RTC_PRE_DIV128	0x00000000

/* RTC_CNTR register */
#define RTC_CNTR	0x90002c00
#define RTC_RCN_COUNT	0xffffffff

/* RTC_ALM register */
#define RTC_ALM		0x90003000
#define RTC_RTC_ALARM	0xffffffff

#endif /* _IXP12X0_TIMERREG_H_ */
