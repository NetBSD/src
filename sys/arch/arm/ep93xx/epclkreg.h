/*	$NetBSD: epclkreg.h,v 1.2.82.1 2010/03/11 15:02:05 yamt Exp $ */

/*
 * Copyright (c) 2004 Jesse Off
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


#ifndef _EPCLKREG_H_
#define _EPCLKREG_H_

/* Timer1 16-bit timer (Free running/Load based) */
#define	EP93XX_TIMERS_Timer1Load	0x00000000UL
#define	 TimerLoad_MASK			0x0000ffffUL
#define	EP93XX_TIMERS_Timer1Value	0x00000004UL
#define	 TimerValue_MASK		0x0000ffffUL
#define	EP93XX_TIMERS_Timer1Control	0x00000008UL
#define	 TimerControl_ENABLE		(1<<7)
#define	 TimerControl_MODE		(1<<6)
#define	 TimerControl_CLKSEL		(1<<3)
#define	EP93XX_TIMERS_Timer1Clear	0x0000000cUL

/* Timer2 16-bit timer (Free running/Load based) */
#define	EP93XX_TIMERS_Timer2Load	0x00000020UL
#define	EP93XX_TIMERS_Timer2Value	0x00000024UL
#define	EP93XX_TIMERS_Timer2Control	0x00000028UL
#define	EP93XX_TIMERS_Timer2Clear	0x0000002cUL

/* Timer3 32-bit timer (Free running/Load based) */
#define	EP93XX_TIMERS_Timer3Load	0x00000080UL
#define	EP93XX_TIMERS_Timer3Value	0x00000084UL
#define	EP93XX_TIMERS_Timer3Control	0x00000088UL
#define	EP93XX_TIMERS_Timer3Clear	0x0000008cUL

/* Timer4 40-bit timer (Free running) */
#define	EP93XX_TIMERS_Timer4Enable	0x00000064UL
#define	EP93XX_TIMERS_Timer4ValueHigh	0x00000064UL
#define	EP93XX_TIMERS_Timer4ValueLow	0x00000060UL

#endif /* _EPCLKREG_H_ */
