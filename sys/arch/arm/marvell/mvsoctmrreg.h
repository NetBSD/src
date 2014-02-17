/*	$NetBSD: mvsoctmrreg.h,v 1.4 2014/02/17 05:11:25 kiyohara Exp $	*/
/*
 * Copyright (c) 2007 KIYOHARA Takashi
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
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _MVSOCTMRREG_H_
#define _MVSOCTMRREG_H_

#define MVSOCTMR_SIZE		0x100

#define MVSOCTMR_CTCR		0x00		/* CPU Timers Control */
#define MVSOCTMR_TESR		0x04		/* CPU Timers Event Status */
#define MVSOCTMR_RELOAD(n)	(0x10 + (n) * 8)/* CPU Timer(n) Reload */
#define MVSOCTMR_TIMER(n)	(0x14 + (n) * 8)/* CPU Timer(n) */

#define MVSOCTMR_TIMER0		0
#define MVSOCTMR_TIMER1		1
#define MVSOCTMR_WATCHDOG	2
#define MVSOCTMR_TIMER2		4	/* Discovery Innovation only */
#define MVSOCTMR_TIMER3		5	/* Discovery Innovation only */

/* CPU Timers Control Register (MVSOCTMR_CTCR) */
#define MVSOCTMR_CTCR_CPUTIMEREN(n)	(1 << ((n) * 2))
#define MVSOCTMR_CTCR_CPUTIMERAUTO(n)	(1 << ((n) * 2 + 1))
#define MVSOCTMR_CTCR_25MHZEN(n)	(1 << ((n) + 11)) /* Armada XP only */

#endif	/* !_MVSOCTMRREG_H_ */
