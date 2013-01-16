/* $Id: common.c,v 1.2.2.2 2013/01/16 05:32:56 yamt Exp $ */

/*
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Petri Laakso.
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

#include <sys/param.h>
#include <sys/cdefs.h>

#include <arm/imx/imx23_digctlreg.h>
#include <arm/imx/imx23_uartdbgreg.h>

#include "common.h"

/*
 * Delay "delay" microseconds.
 */
void
delay_us(unsigned int delay)
{

	/* Set microsecond timer to 0 */
	REG_WRITE(HW_DIGCTL_BASE + HW_DIGCTL_MICROSECONDS_CLR, 0xFFFFFFFF);

	while (REG_READ(HW_DIGCTL_BASE + HW_DIGCTL_MICROSECONDS) < delay);

	return;
}

/*
 * Write character to debug UART.
 */
void
putchar(int ch)
{

	/* Wait until transmit FIFO has space for the new character. */
	while (REG_READ(HW_UARTDBG_BASE + HW_UARTDBGFR) & HW_UARTDBGFR_TXFF);

	REG_WRITE_BYTE(HW_UARTDBG_BASE + HW_UARTDBGDR,
	    __SHIFTIN(ch, HW_UARTDBGDR_DATA));

	return;
}
