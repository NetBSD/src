/*	$NetBSD: epcom.c,v 1.3 2005/12/24 20:07:03 perry Exp $	*/

/*
 * Copyright (c) 2004 Jesse Off
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* 
 * This file provides the cons_init() function and console I/O routines
 * for boards that use the ep93xx ARM SoC UARTs
 */

#include <sys/types.h>
#include <arm/ep93xx/epcomreg.h>
#include <arm/ep93xx/ep93xxreg.h>
#include <lib/libsa/stand.h>

#include "board.h"

#define SCADDR	(EP93XX_APB_HWBASE + EP93XX_APB_SYSCON)
#define	EPCOM_READ(x)		*((volatile uint32_t *) (CONADDR + (EPCOM_ ## x)))
#define	EPCOM_WRITE(x, v)	*((volatile uint32_t *) \
					(CONADDR + (EPCOM_ ## x))) = (v)
#define	SYSCON_READ(x)		*((volatile uint32_t *) \
					(SCADDR + (EP93XX_SYSCON_ ## x)))
#define	SYSCON_WRITE(x, v)	*((volatile uint32_t *) \
					(SCADDR + (EP93XX_SYSCON_ ## x))) = (v)

#define	ISSET(t,f)	((t) & (f))

void
cons_init(void)
{
	unsigned long baud, pwrcnt;

	while(!ISSET(EPCOM_READ(Flag), Flag_TXFE));

	/* Make UART base freq 7Mhz */
	pwrcnt = SYSCON_READ(PwrCnt);
	pwrcnt &= ~(PwrCnt_UARTBAUD);
	SYSCON_WRITE(PwrCnt, pwrcnt);

	baud = EPCOMSPEED2BRD(CONSPEED);
	EPCOM_WRITE(LinCtrlLow, baud & 0xff);
	EPCOM_WRITE(LinCtrlMid, baud >> 8);
	EPCOM_WRITE(LinCtrlHigh, LinCtrlHigh_FEN|LinCtrlHigh_WLEN);
}

int
getchar(void)
{
	while(!ISSET(EPCOM_READ(Flag), Flag_RXFE));
	return (EPCOM_READ(Data) & 0xff);
}

void
putchar(int c)
{
	while(ISSET(EPCOM_READ(Flag), Flag_TXFF));

	if (c == '\n') {
		while(!ISSET(EPCOM_READ(Flag), Flag_TXFE));
		EPCOM_WRITE(Data, '\r');
	} 

	EPCOM_WRITE(Data, c);
}
